# BOTcorps Adv v1019r - playing the Defend games on the Cardputer keyboard

Cardputer arrows are the unshifted `;` (up), `.` (down), `,` (left) and `/` (right).
`Enter` = OK, `Space` = action, backtick = quit to the Defend board, `Tab` = next game.

## Held keys (v1019r, item 6)
Holding an arrow now auto-repeats: first repeat after 260 ms, then every 130 ms
(about 7 moves per held second). Auto-repeat is ON only in games where a held key
should keep moving: RAIN CATCH, FIREWALL WALL, CHANNEL HOP, PATCH RUN, SIGNAL TRACE,
JAM DODGE and NIGHT SHIFT. One-shot games (BEACON SORT, EVIL TWIN, RANGE RING,
PAIR LOCK, BOTNET BREAKER) and the maze-steer games (GIBSON/BIT MAZE, SWEEP: the
arrow sets a wanted direction) never repeat, so a held key cannot double-answer.

## Each bot's own game (the Defend demo + `jackin`)
| bot | game | keys |
|---|---|---|
| wifi | RAIN CATCH | `,` `/` move, Space pause |
| bluetooth | BEACON SORT | `,` trust, `/` unknown |
| deauth | JAM DODGE | `;` `.` lane |
| hack | EVIL TWIN | `,` left, `/` right |
| flock | FIREWALL WALL | `,` `/` move, Enter launch |
| drone | SIGNAL TRACE | arrows move, Enter scan |
| airtag | RANGE RING | arrows ping that side |
| keyfob | PATCH RUN | arrows move, Enter patch |
| meta | CHANNEL HOP | `,` `/` hop, Enter boost |
| skimmer | BIT MAZE | arrows steer |
| flipper | BOTNET BREAKER | `1`-`5` cut a strand |
| samsungtag | PAIR LOCK | `0`-`9` type PIN, Enter lock |
| gibson | NIGHT SHIFT (flagship) | arrows walk, Space scan, Enter grab |

`play 1-12` starts any game; `night` / `sweep` start Gibson's two games; `dex` opens the Device Dex.

## NIGHT SHIFT (Gibson flagship)
Night guard in a 6-room building (LOBBY SERVER OFFICE LAB STORAGE DOCK). You see only
a radius-3 torch pool. Space = SCAN (2 s cooldown): a ring sweeps out 8 tiles and
blips show devices for 1.6 s (airtag only within 5, faint). Walk onto a device and
press Enter to neutralize it. ALERT rises while devices are live; a catch drops it 25.
ALERT 100 = breach (lost); 6:00 AM (150 s) = lost. Shifts 1-5 field 3/5/7/9/10 devices.
Device behaviour: drone drifts through walls, flipper flees when you are within 3,
meta glass moves while unlit, BT tracker wanders, deauth jams your scanner 3 s.
Digits 1-6 = room labels, map mode shows the walls you have seen.

## SWEEP (bonus)
Gibson in a 15x11 server maze eats bits; drone, deauth (aims 2 tiles ahead) and skimmer
(drops sticky goo that halves your speed) traces chase; flipper joins from level 3.
ROOT corners frighten the traces for 5 s. Space = BURST once per level (frightens traces
within 4 tiles for 3 s). A bonus device appears at 30 % and 70 % of the bits (+50, Dex).

## Device Dex
Both Gibson games add every caught device type to a shared 10-slot Dex (`dex`, Esc closes).
