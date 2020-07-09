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

### 1.1.4 (2020-06-21)

-   resolve bug where RackNES would crash when the clock frequency was set to
    absolute minimum value through CV

### 1.1.5 (2020-06-28)

-   fix some aesthetic issues in the plugin.json file

### 1.1.6 (2020-06-30)

-   fix audio crackling / popping (BlipBuffer now runs locked at 768kHz)

### 1.2.0 (2020-07-01)

-   mixer (individual channel outputs + mix output)

### 1.3.0 (2020-07-02)

-   NTSC emulation

### 1.4.0 (2020-07-03)

-   ROM files can be loaded by dropping them onto the module

### 1.4.1 (2020-07-09)

-   improve ROM file loading and storage
-   fix mapper number calculation to include the 4-high bits in the 12-bit
    format
-   fix location of manual
