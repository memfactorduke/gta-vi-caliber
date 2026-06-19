# GTC Mission Editor

A god-mode, GTA-style **mission creator** for GT-caliber. Author any content — main story
missions, side quests, or activities (side jobs, street races) — define objectives, triggers
and rewards, save/load them as hand-editable `.mission.json` files, and play them on the live
mission runtime. Built on top of the existing `Missions/Core` + `Missions/Coordinators`
systems; it adds an authoring/serialization/UI layer, not a second mission engine.

## Quick start (in-game, no Blueprint required)

Everything is driven by console commands (open the console with `~`). Address missions by
their list index.

```
GTC.Mission.Toggle                      # turn god/creator mode on
GTC.Mission.New side                     # create a side quest -> prints its index (#0)
GTC.Mission.AddObjective 0 reach_dock Reach the docks
GTC.Mission.AddObjective 0 clear_dock Clear out the dealers
GTC.Mission.PlaceHere 0 reach_dock       # set reach_dock's waypoint to where you're standing
GTC.Mission.Spawn 0                      # drop visible markers at the placed objectives
GTC.Mission.Validate 0                   # list any errors/warnings
GTC.Mission.Save 0                       # write Saved/Missions/<id>.mission.json
GTC.Mission.Play 0                       # load + start it on the real MissionController
```

Other commands: `GTC.Mission.List`, `GTC.Mission.Load <path>`, `GTC.Mission.Clear`,
`GTC.Mission.New [main|side|activity]`, `GTC.Mission.Save <index> [absolutePath]`,
`GTC.Mission.Complete [objId]`.

The same operations are exposed as `BlueprintCallable` functions on
`UGtcMissionEditorSubsystem` (a world subsystem) if you'd rather drive them from UMG.

### Play it from the world (the GTA "walk up to a blip" loop)

Instead of `Play`-ing from the console, place start markers and walk into them:

```
GTC.Mission.SetStart 0        # mark mission #0 available, start point = where you stand
GTC.Mission.Arm               # show blue giver blips for every AVAILABLE mission
                              # ...walk into a blip -> that mission starts
GTC.Mission.Disarm            # hide the blips
```

A mission only shows a blip once its `prerequisites` (other mission GUIDs) are all complete and
it isn't already done (unless `bRepeatable`). Finishing a mission re-shows the blips, so a chain
lights up its next entries automatically — author a whole branching questline and play it through.

### How each objective kind completes

| Kind | Completes when… | Hook |
|------|-----------------|------|
| `Reach` (+`Escort`/`Defend`) | player enters the waypoint radius | automatic (proximity) |
| hold (objective `driver_kind: hold`) | player stays in the radius for `duration` | automatic (proximity) |
| `Survive` | player stays alive for `duration` (death fails the mission) | automatic (timer) |
| `Eliminate` / `Collect` | game reports `target_count` kills/pickups | `NotifyKill()` / `NotifyObjectiveProgress(n)` — `GTC.Mission.Progress [n]` to test |
| `Interact` | player interacts while in range | `NotifyInteract()` — `GTC.Mission.Interact` to test |
| `Custom` / anything | scripted | `GTC.Mission.Complete [objId]` |

Call the `Notify*` functions on `UGtcMissionEditorSubsystem` from your combat / pickup / interact
code (one line each); the console equivalents exist for testing without that wiring.

## The `.mission.json` format ("openable")

A mission file is a small envelope around one mission object:

```json
{
  "format": "gtc_mission",
  "version": 1,
  "mission": {
    "id": "mission_demo",
    "title": "Heat on the Strip",
    "content_type": "SideQuest",
    "time_limit": 300,
    "objectives": [
      { "id": "reach_dock", "text": "Reach the docks", "kind": "Reach",
        "waypoint": [1200.0, -650.0, 90.0], "has_waypoint": true, "radius": 750.0 }
    ],
    "reward": { "money": 5000, "objective_reward": 300, "mission_bonus": 2500, "xp": 120 }
  }
}
```

Enum fields use short names (case-insensitive on load; the fully-qualified form is also
accepted):

- `content_type`: `MainMission`, `SideQuest`, `Activity`
- objective `kind`: `Reach`, `Collect`, `Eliminate`, `Escort`, `Survive`, `Defend`, `Interact`, `Custom`
- objective `driver_kind`: `reach` or `hold`
- trigger `type`: `EnterRadius`, `Interact`, `KillCount`, `HoldRadius`, `Timer`, `Manual`
- `activity_kind`: `None`, `SideJob`, `StreetRace`

The loader is tolerant: unknown keys are ignored, missing keys take sensible defaults, and any
malformed/empty file is rejected safely (it never produces a half-built mission). Positions are
world-space centimetres `[x, y, z]`. A full example is in `Examples/heat_on_the_strip.mission.json`.

## Rewards

On objective/mission completion, `PlayMission` drains the `MissionReward` payout (per-objective
`objective_reward` + `mission_bonus`) into the player's wallet (`UPlayerStatsComponent`), adds the
headline `money` lump on completion, and awards progression `xp`. Negative amounts are no-ops.
`unlocks` are advisory until the progression unlock table is data-driven.

## Activities

Set `content_type` to `Activity` and pick `activity_kind`:

- `SideJob` — fill the `side_job` block (`kind` 0=Taxi/1=Delivery/2=Vigilante/3=Towing,
  `pickup`, `dropoff`, `base_reward`, `par_time`). Compiles to the `SideJob` model.
- `StreetRace` — fill the `race` block (`checkpoints` list, `laps`, `base_reward`). Compiles to
  the `StreetRace` model; reward scales with finishing placement.

## Optional: a proper UMG creator panel

The console + world markers are fully usable on their own. For an on-screen panel, build it in
the editor (these are `.uasset`s, not C++):

1. A Widget Blueprint (`WBP_MissionEditor`). In its graph, `Get World Subsystem ->
   UGtcMissionEditorSubsystem` and bind buttons/fields to `CreateMission`, `AddObjective`,
   `SetObjectiveWaypoint`, `ValidateMission`, `SaveMissionToFile`, `LoadMissionFromFile`,
   `PlayMission`, `SpawnMarkersForMission`. Use `GetMissionSummaries` to populate a list.
2. An Input Action (`IA_ToggleEditor`) + a Mapping Context entry; on triggered, call
   `ToggleGodMode` and add/remove the widget to the viewport.

## Tests

Pure-logic coverage (run with the editor closed, or via a live MCP connection):

```
Automation RunTests GTC.Missions.Editor
```

Covers: JSON round-trip + bad-input rejection, bank CRUD + validation + file round-trip +
waypoint placement, narrative compile + campaign progression through `MissionChain`, reward
plan mapping, and activity compile running a real `SideJob`/`StreetRace` to completion.

## File map

| File | Role |
|------|------|
| `GtcMissionDefinition.h` | Reflected data model (mission/objective/trigger/reward/side-job/race). |
| `GtcMissionJson.*` | `.mission.json` codec + file IO (reuses the in-module JSON stack). |
| `GtcMissionBank.*` | Pure authoring logic: CRUD, reorder, waypoint-set, validation. |
| `GtcMissionCompiler.*` | Translates a definition into `MissionController` inputs / a `MissionChain`. |
| `GtcMissionReward.*` | Authored reward -> clamped payout plan. |
| `GtcActivityCompiler.*` | Authored activity -> `SideJob`/`StreetRace` + reward maths. |
| `GtcMissionObjectiveMarker.*` | In-world objective marker actor (text label, no art needed). |
| `GtcMissionEditorSubsystem.*` | World subsystem: authoring API, playback, reward wiring, console commands. |
