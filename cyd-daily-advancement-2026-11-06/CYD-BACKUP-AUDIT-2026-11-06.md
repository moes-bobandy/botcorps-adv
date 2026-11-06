# CYD Backup Audit — 2026-11-06 (America/New_York)

GREEN-scope read-only audit of CYD-owned `/workspace/out/` folders and GitHub `origin` backup refs for `moes-bobandy/botcorps-adv` (repo backing `/workspace/cyd-portrait-wt`).

## Repo HEADs (untouched)

| Worktree | HEAD | Branch / note | Working tree |
|---|---|---|---|
| `/workspace/cyd-portrait-wt` | `b29f8abf17aa4c8ea0542dad9b574844e4e62807` (`b29f8ab`) | `backup/cyd-livescan-2026-10-30` | clean (porcelain lines=0) |
| `/workspace/cyd-lock-store` | `20faf173e801fbda4b8c487f3b9e9d9039372b8f` (`20faf17`) | 20faf17 lock ui_gibson_radar_hunt_meter_interact (V2 arc + band ripple 057279f, Mobandy 2026-11-05 3:51 PM ET) | **not modified** this run (pre-existing dirty deletions left alone) |

## CYD folders in `/workspace/out/`

| Folder | Size | On `origin` backup/* ? | Notes |
|---|---|---|---|
| `fw-port-1105/` | 13M | **YES** `backup/cyd-fw-port-1105` → `191022a135fac3acc52ef17c0403cf9180b8d497` | Contains `cyd-livescan-b29f8ab-merged.bin` sha256 `09b2ca752fe8ddc8a386b7ee164e55f233ed74824a684a9268c983ab24bfc04c` (2,275,312 B). Also mirrored conceptually by `backup/cyd-livescan-2026-10-30` → `b29f8abf17aa4c8ea0542dad9b574844e4e62807` (= portrait HEAD `b29f8ab`). |
| `cyd-gbc-own/` | 676K | **NO** (prior to this audit) | `gbc-cyd-own-M3.19-factory-0x0.bin` sha256 `08bd609bcf201b951d7ff2fbe8fb3e3e00c236e092db9669a5078c0dfdf2ab81` (685,568 B). **Included in today's orphan backup.** |
| `radar-current-meter-interact/` | 7.2M | **NO** (prior to this audit) | Hunt meter A / V1 / V2 mock renders + src (<95 MB, no `.pio`). **Included in today's orphan backup.** |
| `cyd-daily-advancement-2026-11-06/` | (this run) | **NEW** → `backup/cyd-daily-advancement-2026-11-06` | Preflight script + this audit + daily advancement report. |

Ignored non-CYD `/workspace/out/` peers (other bots) — not inspected beyond name filter.

## Already backed up on GitHub (relevant CYD refs)

- `refs/heads/backup/cyd-livescan-2026-10-30` = `b29f8abf17aa4c8ea0542dad9b574844e4e62807` (matches portrait `b29f8ab`)
- `refs/heads/backup/cyd-fw-port-1105` = `191022a135fac3acc52ef17c0403cf9180b8d497`
- `refs/heads/backup/cyd-lock-store` = `20faf173e801fbda4b8c487f3b9e9d9039372b8f` (matches lock-store `20faf17`)
- Additional historical CYD portrait/on-* / hunt-target-mocks refs exist under `backup/cyd-*` and `backup/botcorps-adv/cyd-*` (see `git ls-remote --heads origin`).

## Unbacked CYD folders found (<95 MB)

1. `cyd-gbc-own/` (676K)
2. `radar-current-meter-interact/` (7.2M)
3. `cyd-daily-advancement-2026-11-06/` (today's deliverables)

All three are packaged into orphan branch `backup/cyd-daily-advancement-2026-11-06` via `GIT_INDEX_FILE` + `write-tree` + `commit-tree` + fast-forward-safe new-ref push (portrait HEAD/worktree never checked out away from `b29f8ab`).

## Guardrails honored

- Box-only (no `machineId`, no BOTcorps PC).
- Did not touch off-limits paths/refs (`/tmp/gsa`, `/tmp/sf`, `kbuild`, `adv-live`, `hwgate`, `hitlist-wt`, `stash@{0}`, `motorbot`, `securitybot`, `tools/gate*`, `main`, any `adv` branch).
- Portrait left at `b29f8ab` clean; lock-store left at `20faf17` without worktree edits.
- No force-push; no software deletes.
