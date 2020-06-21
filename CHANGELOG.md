### 1.0.0 (2020-06-14)

-   support for the first four cartridge mappers
-   2 player support
-   frame-rate-based clock control
-   clock output based on frame-rate
-   APU that works at all sample rates

### 1.1.0 (2020-06-19)

-   clock controls CPU rate directly
-   OS dialogs shown when ROMs fail to load
-   fix clock to properly output pulses at the frame rate (PW=50%)

### 1.1.1 (2020-06-19)

-   ix issue where backup / restore was freezing the emulation state

### 1.1.2 (2020-06-19)

-   resolve potential memory leak: deallocate emulator and backup state when
    module is removed from the rack

### 1.1.3 (2020-06-20)

-   resolve bug where RackNES would cause a segmentation fault when attempting
    to load a ROM from a JSON state where the ROM file pointed to is invalid
