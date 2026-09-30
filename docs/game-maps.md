# CV Genie Mapping Sources

The shipped catalog is in [GameMaps.hpp](../src/GameMaps.hpp). New mappings
were reviewed against the sources below on 2026-09-30. These are documented
RAM controls, not a promise of gameplay validation on every ROM revision.
The [implementation spec](../specs/archive/003-cv-genie-game-catalog.md) records scope
and completion evidence. No ROMs or disassembly code are bundled here.

## Selection And Quality Bar

The initial expansion favors familiar action franchises and Nintendo Tetris
within RackNES's mapper 0--3 support. The target for new maps is the original
North American NES cartridge. Sources often omit exact PRG revisions, so that
is a target scope, not an independently verified ROM fingerprint. Regional
versions, FDS games, multicarts, and hacks are not covered by the same promise.
Do not describe this shortlist as measured 90% coverage.

Genie writes an entire byte to internal RAM, $0001--$07FF. Address zero is an
empty-message sentinel; cartridge RAM, ROM patches, and memory-mapped I/O
cannot be used. New controls avoid pointers, packed decimal, sparse enums,
and multi-byte state. Full-byte coordinate/timer ranges describe storage,
not safety in every scene. Even valid coordinates can cause collision glitches;
holding a timer can prevent a transition or sustain sound indefinitely.

New source-derived names are concise project labels; source descriptions and
code are not imported wholesale. The legacy maps retain their original
attribution and every saved assignment, including experimental controls that
would not meet the narrower admission criteria for new maps.

## Source And Range Decisions

### Existing Maps: IDs 0--1

Super Mario Bros. and The Legend of Zelda retain all 53 and 55 entries,
respectively, with unchanged order, names, byte endpoints, and trigger modes.
Their inherited catalog does not identify precise ROM revisions. Sources:
[SMB RAM map](https://datacrystal.tcrf.net/wiki/Super_Mario_Bros./RAM_map),
[Zelda RAM map](https://datacrystal.tcrf.net/wiki/The_Legend_of_Zelda/RAM_map),
and the [SMB enemy-direction disassembly](https://6502disassembly.com/nes-smb/SuperMarioBros.html#SymEnemy_MovingDir).
This expansion does not re-audit or silently revise those controls.

### Mega Man: ID 2, Mapper 2

The [RAM map](https://datacrystal.tcrf.net/wiki/Mega_Man/RAM_map) identifies
health at $006A, weapon energy at $006B--$0071 (0--28), lives at $00A6
(0--99; avoid the game-over high bit), and the damage timer at $0055
(0--111). Each energy byte has its own assignment. No weapon-unlock bitfield
or score digits are exposed. [NESdev's UxROM reference](https://www.nesdev.org/wiki/UxROM)
identifies this cartridge family, also used by Castlevania and Contra.

### Mega Man 2: ID 3, Mapper 1

The [RAM map](https://datacrystal.tcrf.net/wiki/Mega_Man_2/RAM_map) supplies
position $0460/$04A0, health $06C0, tanks $00A7, lives $00A8, damage timer
$004B, and palette delay $0355. Health is limited to 28; tanks to 4 to avoid
invalid passwords; lives to 99. Coordinates and timers use 0--255. Zero
palette delay disables cycling; its effect depends on the stage. The
[NESMaps memory-code reference](https://mail.nesmaps.com/maps/MegaMan2/MegaMan2.html)
corroborates full health as $1C at $06C0. The
[cartridge entry](https://datacrystal.tcrf.net/wiki/Mega_Man_2) identifies MMC1.

### Castlevania: ID 4, Mapper 2

The [RAM map](https://datacrystal.tcrf.net/wiki/Castlevania_%28NES%2C_Famicom_Disk_System%29/RAM_map)
identifies Simon as object slot zero: position $038C/$0354; damage timer
$005B; stun timer $0047. These use byte ranges. Lives $002A use 1--99:
one is the last life, unlike Contra's zero-based spare-life count. Sparse
subweapon IDs and multi-byte timer/score fields are omitted. Only the NES
cartridge is targeted, despite the source page also mentioning FDS.

### Castlevania II: Simon's Quest: ID 5, Mapper 1

The [RAM map](https://datacrystal.tcrf.net/wiki/Castlevania_II%3A_Simon%27s_Quest/RAM_map)
defines object fields as $0300 + property * 18 + slot, with **decimal** 18.
Simon is slot zero: X property 4 gives $0348; Y property 2 gives $0324;
facing property 16 gives $0420; damage timer property 28 gives $04F8.
Position/timer use byte ranges; facing toggles 0/1. Whip $0434 is the dense
0--4 enumeration. Health and packed counters are omitted. The
[US board record](https://nescartdb.com/profile/view/1126/castlevania-ii-simons-quest)
identifies mapper 1.

### Contra: ID 6, Mapper 2

The [RAM map](https://datacrystal.tcrf.net/wiki/Contra_%28NES%29/RAM_map) and
[US disassembly RAM symbols](https://github.com/vermiceli/nes-contra-us/blob/main/src/ram.asm)
identify player X at $0334/$0335, Y at $031A/$031B, spare lives at $32/$33,
weapons at $AA/$AB, and barrier timers at $B0/$B1. Positions use byte ranges.
Lives are intentionally capped at 9; zero means the final active life.
Weapons use only 0--4, explicitly clearing the rapid-fire flag. Timers use
0--128, the ordinary barrier duration (the source also notes 144 in level 7).
Player 2 controls require active two-player play. The Japanese cartridge's
VRC2 hardware and Probotector variants are outside this map's target.

### Metroid: ID 7, Mapper 1

The [US disassembly](https://6502disassembly.com/nes-metroid/Metroid_USA.html)
defines ObjectX/$030E and ObjectY/$030D as room coordinates, not screen
coordinates. MissileToggle/$010E is 0/1 and grants no ammunition. The four
music frame counters at $0640--$0643 control per-channel progression; they
use 0--255. Holding a counter can stall that channel; disconnect to release.
Health is packed and missile inventory is at $6879 outside Genie's writable
RAM, so both are omitted. The [cartridge entry](https://datacrystal.tcrf.net/wiki/Metroid)
identifies MMC1. The Japanese FDS version is excluded.

### Ninja Gaiden: ID 8, Mapper 1

The [RAM map](https://datacrystal.tcrf.net/wiki/Ninja_Gaiden/RAM_map)
defines whole-pixel position $86/$8A, health $65 (0--16), lives $76, and
$62 as the 60-frame countdown that decrements the stage timer. Lives are
conservatively capped at 9; the countdown uses 0--60. Holding it can stop
stage-time progression. Do not substitute $66 for enemy health: it is only
the displayed boss bar. The [cartridge entry](https://datacrystal.tcrf.net/wiki/Ninja_Gaiden)
identifies MMC1. No claim is made for Shadow Warriors or mapper-conversion hacks.

### Tetris (Nintendo): ID 9, Mapper 1

The [RAM map](https://datacrystal.tcrf.net/wiki/Tetris_%28NES%2C_Nintendo%29/RAM_map)
identifies falling-piece column/row $40/$41, fall timer $45, horizontal
repeat timer $46, and music choice $C2 (0--3). Columns are limited to 0--9
and rows to 0--19 for the 10-by-20 board; this does not account for piece
shape or occupied cells. Timers use byte ranges. The game may apply music
selection only when its sound routine next reads it. Score/line BCD, speed
lookup indices, and next-piece orientation IDs are omitted. The
[cartridge entry](https://datacrystal.tcrf.net/wiki/Tetris_%28NES%29)
identifies MMC1. This is not Tengen Tetris.

## Maintenance And Validation

Append game IDs and location indices; saved Rack patches store integers.
Add a source and explain every range choice, especially any intentionally
restricted subset. Require clear evidence before adding another control.
Do not scrape every numeric-looking RAM-map row into a continuous input.

After editing the catalog, run from the repository root:

```shell
python3 tools/update_game_list.py
python3 tools/update_game_list.py --check
make -j2
make -C tests -j2
make -C manual
```

The generator needs only Python's standard library and no network. CI checks
that the committed manual list matches the catalog. SDK-backed tests cover
byte writes, indices, persistence, disconnects, and catalog integrity. They
do not execute these commercial games or establish the source documents'
accuracy. For gameplay reports, record ROM region/revision, game/element,
scene, voltage, expected behavior, and observed behavior. Test during active
play, disconnect before restoring snapshots, and try controls individually.
