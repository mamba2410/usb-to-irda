# USB to IRDA firmware

This uses an STM32CubeIDE project with the tinyusb stack.

I used [this GH comment](https://github.com/hathach/tinyusb/discussions/633#discussioncomment-342237)
to set it up, with the one addition of adding `tinyusb/hw` to the `Includes`
list.

Compile with STM32CubeIDE.
Program with either STM32CubeProgrammer via USB/DFU, or connect a debugger
(e.g. st-link, raspberry pi debug probe) and upload that way.

To put into DFU mode, hold "Boot" button then press "Reset", or hold "Boot"
when connecting power.

## Quirks

### BOOT0 with STM32

The current configuration semi-bricks the device when compiled in `Debug` mode,
it it works fine in `Release` mode.
No idea why.

In CubeIDE/CubeMX V1.19.0, set boot behaviour with `nBOOT_SEL`, `nBOOT1`, 
`nBOOT0` bits in `FLASH->User Configuration->Activate`.
`nBOOT_SEL` "enable" means `1` and "disable" means `0`. Comment is accurate.
`nBOOT1` and `nBOOT0` "enable" means `0` and "disable" means `0`. Comments do 
not help.
Likely a bug. If not, its extremely poor documentation.

To set the option bits, one needs to flash, power-cycle (NOT reset), let the
code set the option bits, then power-cycle again for the option bits to take
effect.
After this, you can hold the boot button and press reset to enter DFU
bootloader mode.

### Un-bricking the device

I don't know what causes it, but the device ends up in a bad state if you don't
have the exact right configuration.

The device erases all flash and sets all option bits to 1.
This ends up setting the flash read-out protection to "all" which means the 
chip can't read the flash and then can't boot, or something.

The fix is to connect with an ST-link and STM32 programmer program, then in
the "OB" tab on the left, set the read-out protection to `0xAA` and reprogram.


