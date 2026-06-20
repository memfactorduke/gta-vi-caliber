# NPC Daily Routine Editor

Give individual NPCs their **own day** — not a shared archetype template. Two
baristas can now live different lives: one an early riser who hits the gym at dawn,
the other a night owl who closes the bar. Routines are plain JSON, so **both a human
(text editor / in-game console) and an AI/MCP** can read and write them.

## How it fits the existing AI

A routine is just an ordered list of `[start, end)` blocks — the exact shape
`FNpcMind`/`FNpcSchedule` already consume. So an authored routine drops straight into
the existing decision loop: the schedule is the baseline, and live needs (hunger,
sleep, the urge to unwind) still override it when they boil over, and the per-citizen
±45 min jitter still applies. A routine replaces *which baseline day* a citizen runs,
nothing else.

Each citizen resolves its routine in `AGTCCitizen::Replan`:
1. an explicit assignment for its seed (`GTC.Routine.Assign`), else
2. a **stable seed-pick** from the bank (same person, same routine every reload), else
3. if the bank is empty, its archetype schedule (original behaviour).

## The three ways to edit (all hit the same live bank)

### 1. In-game console — `GTC.Routine.*`
```
GTC.Routine.List                                  list every routine
GTC.Routine.Show   <id>                           print its blocks
GTC.Routine.New    <id> [display name...]         create an empty routine
GTC.Routine.AddBlock <id> <start> <end> <activity> <place>
GTC.Routine.RemoveBlock <id> <index>
GTC.Routine.Validate <id>                         full-day coverage + overlap lint
GTC.Routine.Assign <seed> <routineId>             give a citizen seed a routine
GTC.Routine.Save   <id> [absPath]                 write one .routine.json
GTC.Routine.Load   <absPath>                      load one into the bank
GTC.Routine.SaveBank [absPath]                    write the whole bank
GTC.Routine.Reload                                re-read the bank file (hot-reload)
```

### 2. On disk — JSON (human + AI)
`GTC.Routine.SaveBank` writes to `<Project>/Saved/Routines/routines.bank.json`.
Edit it (or a single `<id>.routine.json`) in any text editor or from an AI/MCP file
write, then `GTC.Routine.Reload`. Format:

```json
{ "format": "gtc_routine", "version": 1,
  "routine": {
    "id": "early_bird_office",
    "name": "Early-Bird Office Worker",
    "blocks": [
      { "start": 6.0,  "end": 9.0,  "activity": "goof_off",  "place": "gym" },
      { "start": 9.0,  "end": 17.5, "activity": "work",      "place": "office" },
      { "start": 17.5, "end": 22.0, "activity": "socialize", "place": "bar" },
      { "start": 22.0, "end": 6.0,  "activity": "sleep",     "place": "home" }
    ] } }
```
A bank file is the same with `"format": "gtc_routine_bank"` and a `"routines": [ ... ]`
array. Blocks are `[start, end)` clock hours in `[0,24)`; a block may wrap past
midnight (`start > end`, e.g. sleep `22 -> 6`). First covering block wins, so author
most-specific first. Aim for full-day coverage (`GTC.Routine.Validate` checks it).

**Vocabulary** (matches the archetype schedules): activities `sleep`, `work`, `eat`,
`socialize`, `loiter`, `goof_off`, `commute`; places `home`, `office`, `diner`, `gym`,
`bar`, `park`, `street`.

### 3. On-screen GUI panel — `GTC.Routine.Panel`
Toggles an in-viewport Slate editor (`SGTCRoutinePanel`): type an id, fill a block's
start / end / activity / place, then New / Add Block / Remove Block / Show / Validate /
Save / Save Bank / Reload, or assign a routine to a citizen seed. The readout echoes
each result. It drives the exact same live bank as the console and JSON, so a human
gets a point-and-click editor without losing the text/AI paths. (Needs a running
game/PIE viewport to display.)

### 4. AI/MCP or Blueprint — the subsystem API
`UGTCRoutineSubsystem` exposes the same operations as `BlueprintCallable` UFUNCTIONs
(`CreateRoutine`, `AddBlock`, `AssignRoutineToSeed`, `SaveRoutine`, `LoadBank`,
`ReloadDefaultBank`, …) — id/string based, so a Blueprint or an MCP exec bridge drives
the editor without any reflected struct. The Unreal MCP can also just run the
`GTC.Routine.*` console commands directly.

## Out of the box
With no bank file on disk, the subsystem seeds six built-in days (early-bird office,
night-owl bartender, cafe freelancer, park retiree, campus student, remote homebody);
citizens seed-pick among them, so a fresh world already reads as individuals. Run
`GTC.Routine.SaveBank` to write them out and start editing.

## Code map
- `NPC/Decision/NpcRoutine.{h,cpp}` — pure-core model (`FNpcRoutine`,
  `FNpcRoutineLibrary`: find / full-day / overlap / seed-pick). Tested
  `GTC.NPC.Decision.NpcRoutine`.
- `NPC/Routine/NpcRoutineJson.{h,cpp}` — JSON codec (reuses `Systems/Save` `GtcJson`).
  Tested `GTC.NPC.Routine.NpcRoutineJson`.
- `NPC/Routine/GTCRoutineSubsystem.{h,cpp}` — live store, Blueprint API, `GTC.Routine.*`
  console commands. Thin adapter over the pure layers (mirrors the mission editor).
- `NPC/Routine/SGTCRoutinePanel.{h,cpp}` — the on-screen Slate editor + `GTC.Routine.Panel`
  toggle (mirrors `SGTCForgePanel`).
- `NPC/Agent/GTCCitizen.cpp` — `Replan` resolves the individual routine.
