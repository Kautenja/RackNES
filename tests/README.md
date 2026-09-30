# Focused Regression Checks

These checks run the actual module processing and serialization code against
Rack's headers and library. They use assertions rather than a separate test
framework. No game ROM or graphical Rack session is required.

## Run

From the repository root, using the prepared Rack tree at `../..`:

```shell
make -C tests -j2
```

For another Rack 2 SDK or prepared source tree:

```shell
make -C tests RACK_DIR=/absolute/path/to/Rack-SDK
```

The default compiler flags enable AddressSanitizer and UndefinedBehaviorSanitizer
and stop on the first sanitizer error. A matching C++11 compiler, sanitizer
runtime, Rack headers, dependency headers, and `libRack` are required. The
Makefile sets the library search path for macOS and Linux. On Windows, use
MSYS2 MINGW64 with `SANITIZERS=` and put the matching Rack runtime directory
on `PATH`; the SDK supplies only the import library. Keep the compiler's C++
runtime ahead of the Rack runtime on `PATH`.

Outputs and dependency files stay in ignored `tests/.build/`. If you change
compiler, SDK, or sanitizer flags, rebuild with `make -C tests -B`. For a compiler
without sanitizer support, `SANITIZERS=` disables instrumentation; report that
limitation when recording results.

## Continuous Integration

[Rack plugin and regression tests](../.github/workflows/rack-tests.yml) builds
the plugin and runs both headless fixtures on Linux x64, macOS arm64, and
Windows x64. It runs on pull requests, pushes to `master`, version tags, and
manual dispatch. Rack SDK 2.6.3 archives and the Windows runtime are verified
against SHA-256 checksums, following Fourier's workflow. The Windows installer
is extracted without running it. These matrix runs disable sanitizers.

[Coverage and sanitizers](../.github/workflows/instrumentation.yml) runs separate
Linux Clang 18 jobs for AddressSanitizer/UndefinedBehaviorSanitizer and LLVM
coverage on pull requests, `master` pushes, and manual dispatch. The jobs use
fresh temporary build directories, retain logs and reports for 14 days, and
fail on compiler, assertion, or sanitizer errors. Coverage reports focus on
`RackNES.cpp`, `CVGenie.cpp`, `GameMaps.hpp`, and `theme.hpp`; they exclude the
bundled emulator libraries, Rack SDK, and test fixtures. The prebuilt Rack
library is not instrumented. Coverage describes these focused checks, not
complete emulator or UI validation.

## Coverage

-   CV Genie: inactive and invalid selections, UI selection handoffs, map changes,
    randomize/reset, finite and non-finite voltage inputs, nonzero endpoints,
    ordinary and descending toggles, one write per edge across buffer flips,
    saved toggle state, malformed JSON, and oversized location arrays.
-   Patch compatibility: restore the existing Genie data from
    `patches/debugCVGenie.vcv`, preserving its game and location indices. The
    fixture's audio settings and ROM path are not used. Generated legacy JSON
    also exercises absent toggle fields and invalid selections.
-   Mapping: the five Mario Enemy Heading indices resolve to `0x0046`--`0x004A`
    and no longer overwrite Enemy 5 Type. The address source is
    [the SMB disassembly](https://6502disassembly.com/nes-smb/SuperMarioBros.html#SymEnemy_MovingDir).
-   RackNES: empty-emulator serialization, snapshot release on SAVE, reset,
    patch restoration, and destruction; preserving the display after an invalid
    ROM path; and expander RAM boundaries and row precedence.

The Genie fixture uses a real Rack `Module` neighbor with two explicit expander
buffers. Tests connect input channels as Rack's engine would: `setChannels()`
alone cannot connect a disconnected port. The RackNES fixture creates a headless
Rack engine context for the module's sample-rate initialization.

These checks do not test rendering, a live UI session, or every game's response
to memory edits. They do not replace listening and gameplay checks when changing
CPU/PPU/APU timing or audio conversion.
