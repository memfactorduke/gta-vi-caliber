// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCKinematicVehiclePawn.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "Math/UnrealMathUtility.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"

#include "../Contact/VehicleImpact.h"
#include "../Entry/VehicleSeatComponent.h"
#include "../../Camera/VehicleChaseCamera.h"

AGTCKinematicVehiclePawn::AGTCKinematicVehiclePawn()
{
    PrimaryActorTick.bCanEverTick = true;

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Body->SetCollisionResponseToAllChannels(ECR_Block);
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
    {
        Body->SetStaticMesh(Cube);
    }
    SetRootComponent(Body);

    ChaseBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ChaseBoom"));
    ChaseBoom->SetupAttachment(Body);
    ChaseBoom->TargetArmLength = BaseBoom;
    ChaseBoom->bEnableCameraLag = true;
    ChaseBoom->bDoCollisionTest = true;

    ChaseCam = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCam"));
    ChaseCam->SetupAttachment(ChaseBoom, USpringArmComponent::SocketName);
    ChaseCam->SetFieldOfView(BaseFov);

    // Reused enter/exit adapter (possess/attach/IMC/camera). Seats authored on the BP.
    Seats = CreateDefaultSubobject<UVehicleSeatComponent>(TEXT("Seats"));
}

void AGTCKinematicVehiclePawn::BeginPlay()
{
    Super::BeginPlay();
    Health = FVehicleHealth(MaxHealth);
    Locomotion = FindComponentByClass<UGTCVehicleLocomotionComponent>();
}

void AGTCKinematicVehiclePawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (DeltaSeconds <= 0.0f)
    {
        return;
    }

    if (!bDead && Locomotion != nullptr && GetController() != nullptr)
    {
        Locomotion->SetControls(ControlState);
        Locomotion->Step(DeltaSeconds);

        const FVector V = Locomotion->WorldVelocity();
        FVector Loc = GetActorLocation() + V * DeltaSeconds;

        if (Locomotion->WantsBuoyancy())
        {
            Locomotion->IntegrateVertical(Loc, DeltaSeconds); // boat: sets Z + attitude
        }
        else
        {
            ResolveAircraftContact(Loc, V.Z, DeltaSeconds);
        }

        FHitResult Hit;
        SetActorLocation(Loc, /*bSweep*/ true, &Hit);
        if (Hit.bBlockingHit)
        {
            OnHardContact(Hit, V);
        }
        SetActorRotation(Locomotion->DesiredAttitude());
        DriveChaseCam(Locomotion->CameraSpeed(), static_cast<float>(V.Z), DeltaSeconds);

        // Momentary axes return to neutral if no input event arrives next frame.
        ControlState.PitchAxis = 0.0f;
        ControlState.RollAxis = 0.0f;
        ControlState.YawAxis = 0.0f;
        ControlState.Throttle = 0.0f;
        ControlState.CollectiveRaise = 0.0f;
    }

    // A wreck keeps burning down even unmanned.
    Health.Tick(DeltaSeconds);
    if (!bDead && Health.JustExploded())
    {
        bDead = true;
        OnExploded();
        SetLifeSpan(8.0f);
    }
}

void AGTCKinematicVehiclePawn::ResolveAircraftContact(FVector& Loc, double VerticalSpeedCm, double Dt)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    const FVector Start(Loc.X, Loc.Y, Loc.Z);
    const FVector End = Start - FVector(0.0, 0.0, 100000.0);
    FHitResult Ground;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTCVehicleGround), /*bTraceComplex*/ false, this);
    const bool bGroundHit = World->LineTraceSingleByChannel(Ground, Start, End, ECC_WorldStatic, Params);
    const double GroundZ = bGroundHit ? Ground.ImpactPoint.Z : 0.0;

    const FVehicleGroundContact::FResult R = FVehicleGroundContact::Evaluate(
        Loc.Z, bGroundHit, GroundZ, VerticalSpeedCm, PrevContact, GroundParams);
    PrevContact = R.Contact;

    if (R.bClampToGround)
    {
        Loc.Z = R.ClampZCm;
    }
    if (R.Touchdown == FVehicleGroundContact::ETouchdown::Crash)
    {
        Health.ApplyDamage(R.ImpactHardness01 * MaxHealth * 0.5);
        OnHardImpact(static_cast<float>(R.ImpactHardness01));
    }
    else if (R.Touchdown == FVehicleGroundContact::ETouchdown::Hard)
    {
        Health.ApplyDamage(R.ImpactHardness01 * MaxHealth * 0.15);
        OnHardImpact(static_cast<float>(R.ImpactHardness01));
    }
}

void AGTCKinematicVehiclePawn::DriveChaseCam(float SpeedCmS, float ClimbCmS, float DeltaSeconds)
{
    if (ChaseBoom == nullptr || ChaseCam == nullptr)
    {
        return;
    }
    const float Dist = FVehicleChaseCamera::FollowDistance(BaseBoom, MaxBoom, SpeedCmS, SpeedAtMaxCam);
    ChaseBoom->TargetArmLength = FMath::FInterpTo(ChaseBoom->TargetArmLength, Dist, DeltaSeconds, 3.0f);

    const float Fov = FVehicleChaseCamera::SpeedFov(BaseFov, MaxFov, SpeedCmS, SpeedAtMaxCam);
    ChaseCam->SetFieldOfView(FMath::FInterpTo(ChaseCam->FieldOfView, Fov, DeltaSeconds, 3.0f));

    const float PitchRad = FVehicleChaseCamera::PitchFollow(ClimbCmS, SpeedCmS, CamPitchGain, CamMaxPitchRad);
    const float YawOff = FVehicleChaseCamera::LookBehindYawOffset(ControlState.bLookBehind);
    const FRotator BoomRot(FMath::RadiansToDegrees(-PitchRad), FMath::RadiansToDegrees(YawOff), 0.0f);
    ChaseBoom->SetRelativeRotation(BoomRot);
}

void AGTCKinematicVehiclePawn::OnHardContact(const FHitResult& Hit, const FVector& Velocity)
{
    const double IntoSpeed = FMath::Abs(FVector::DotProduct(Velocity, Hit.ImpactNormal));
    const double Dmg = FVehicleImpact::DamageForSpeed(IntoSpeed, SafeImpactSpeedCm, ImpactDamageScale);
    if (Dmg > 0.0)
    {
        Health.ApplyDamage(Dmg);
        const double Norm = MaxHealth > 0.0f ? FMath::Clamp(Dmg / MaxHealth, 0.0, 1.0) : 0.0;
        OnHardImpact(static_cast<float>(Norm));
    }
}

float AGTCKinematicVehiclePawn::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (bDead || DamageAmount <= 0.0f)
    {
        return Applied;
    }
    Health.ApplyDamage(static_cast<double>(DamageAmount));
    if (Health.IsWrecked())
    {
        bDead = true;
        OnExploded();
        SetLifeSpan(8.0f);
    }
    return Applied;
}

// --- EnhancedInput ----------------------------------------------------------

void AGTCKinematicVehiclePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (EIC == nullptr)
    {
        return;
    }
    if (PitchAction) EIC->BindAction(PitchAction, ETriggerEvent::Triggered, this, &AGTCKinematicVehiclePawn::InputPitch);
    if (RollAction) EIC->BindAction(RollAction, ETriggerEvent::Triggered, this, &AGTCKinematicVehiclePawn::InputRoll);
    if (YawAction) EIC->BindAction(YawAction, ETriggerEvent::Triggered, this, &AGTCKinematicVehiclePawn::InputYaw);
    if (ThrottleAction) EIC->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AGTCKinematicVehiclePawn::InputThrottle);
    if (CollectiveAction) EIC->BindAction(CollectiveAction, ETriggerEvent::Triggered, this, &AGTCKinematicVehiclePawn::InputCollective);
    if (LookBehindAction)
    {
        EIC->BindAction(LookBehindAction, ETriggerEvent::Triggered, this, &AGTCKinematicVehiclePawn::InputLookBehind);
        EIC->BindAction(LookBehindAction, ETriggerEvent::Completed, this, &AGTCKinematicVehiclePawn::InputLookBehindReleased);
    }
    if (ExitAction) EIC->BindAction(ExitAction, ETriggerEvent::Started, this, &AGTCKinematicVehiclePawn::InputExit);
}

void AGTCKinematicVehiclePawn::InputPitch(const FInputActionValue& V) { ControlState.PitchAxis = V.Get<float>(); }
void AGTCKinematicVehiclePawn::InputRoll(const FInputActionValue& V) { ControlState.RollAxis = V.Get<float>(); }
void AGTCKinematicVehiclePawn::InputYaw(const FInputActionValue& V) { ControlState.YawAxis = V.Get<float>(); }
void AGTCKinematicVehiclePawn::InputThrottle(const FInputActionValue& V) { ControlState.Throttle = V.Get<float>(); }
void AGTCKinematicVehiclePawn::InputCollective(const FInputActionValue& V) { ControlState.CollectiveRaise = V.Get<float>(); }
void AGTCKinematicVehiclePawn::InputLookBehind(const FInputActionValue& V) { ControlState.bLookBehind = true; }
void AGTCKinematicVehiclePawn::InputLookBehindReleased(const FInputActionValue& V) { ControlState.bLookBehind = false; }

void AGTCKinematicVehiclePawn::InputExit(const FInputActionValue& V)
{
    if (Seats != nullptr)
    {
        Seats->BeginExit();
    }
}
