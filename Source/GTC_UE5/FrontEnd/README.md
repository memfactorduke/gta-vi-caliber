# Front-end & in-world UI (ported from legacy GTC)

This folder plus `UI/Phone`, `UI/Menus`, and `Core/GTCGameInstance` hold the UI surfaces
pulled over from the legacy `GTC` project: the boot **intro**, the looping **game menu**,
the **loading** cover, the **Esc** pause menu, and the in-world **phone**. Everything is
pure C++/Slate — no editor-authored UMG, MediaPlayer, or MediaTexture assets — so the
headless build needs nothing from the content browser.

## What works with zero asset authoring

The phone and pause menu are fully live right now (they compile into the module and are
hosted by `AGTCPlayerController`):

- **Phone** — `Up` arrow / `P` / D-pad-up, or the `gtc.Phone.Toggle` / `gtc.Phone.Open` /
  `gtc.Phone.Close` / `gtc.Phone.App <0..16>` console commands. All 16 app icons and the
  wallpaper are generated procedurally at runtime (`UI/Phone/GTCPhoneIcons`), so nothing
  ships as a binary asset. Apps bridge to live systems via `FGTCPhoneServices` lambdas in
  the controller: wallet → `UPlayerStatsComponent`, wanted stars → `UWantedSubsystem`,
  speed/heading/location → the pawn, photos → engine screenshots. Services this project
  has no system for yet (stamina, first/third-person view, time-of-day clock, live map
  capture) are left unset and the apps fall back to their stylised defaults.
- **Esc pause menu** — `Esc` opens/closes it and pauses the game. Only *Resume* is wired;
  the *first/third person* and *Mission Editor* buttons are intentionally left as no-ops
  (`ExecuteIfBound`) until this project grows a view-mode system and an in-game mission
  editor widget.

## What the front-end (intro → menu → loading) needs from the editor

The boot flow degrades gracefully — missing movies skip straight to the menu, missing art
just shows the wordmark + spinner — but to see it for real:

1. **GameInstance** — already wired in `Config/DefaultEngine.ini`
   (`GameInstanceClass=/Script/GTC_UE5.GTCGameInstance`). This owns the loading cover that
   survives `OpenLevel`.
2. **Front-end map** — create a small map (e.g. `/Game/FrontEnd/Maps/FrontEnd`) and set its
   World Settings → *GameMode Override* to `GTCFrontEndGameMode` (it spawns no pawn; the
   `AGTCFrontEndController` builds the UI). Point `GameDefaultMap` at this map to make the
   intro/menu the boot experience.
3. **Movies** — the original CALIBER `PreIntro.mp4` (logo sting), `Intro.mp4` (WZVC-6
   news cold-open), and `MenuLoop.mp4` (wordmark over the skyline) already ship in the
   content submodule at `Content/GTCaliberAssets/Content/Movies/`. `ResolveMoviePath`
   searches `Content/<rel>` first, then `Content/GTCaliberAssets/Content/<rel>`, so the
   config paths stay short (`Movies/Intro.mp4`) and resolve wherever the files live.
   Paths are `EditDefaultsOnly`/`Config`, so they can be retargeted without code.
4. **Loading art** — the curated key-art set (filenames listed in
   `Core/GTCGameInstance.cpp`) ships in the submodule at
   `Content/GTCaliberAssets/Content/FrontEnd/LoadingArt/`. `LoadLoadingArt` checks
   `Content/FrontEnd/LoadingArt/` first, then that submodule path; missing files are
   skipped.

`Start Game` opens `WorldMapPath`, defaulted to this project's city map
(`/Game/GTCaliberAssets/Content/CityBeachStrip/Maps/CityBeachStrip`).

## Module deps

`GTC_UE5.Build.cs` gained `Slate`/`SlateCore` (the widgets) and
`Media`/`MediaAssets`/`MediaUtils`/`MoviePlayer` (the boot videos + cross-travel cover).

## Dev console commands

`gtc.PreviewLoading` overlays the loading screen on the current viewport for inspection
without a real world travel.
