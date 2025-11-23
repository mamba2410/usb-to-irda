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

The configurator view is broken with STM32CubeIDE V2.0.0 (at least on Linux)
so you might need to use STM32CubeMX if you want to 

### BOOT0 with STM32

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

If you don't explicitly set RDP (readout protections) it can (and usually does)
set maximum readout protection level, rendering the device useless until 
rescued with an st-link.

The to rescue the device, connect an ST-link to the SWD port and open STM32 
programmer. In the "OB" tab on the left, set the read-out protection to `0xAA`
and reprogram.

The MCU should do this itself.
See CubeMX `FLASH->User Configuration->Read-Out Protection->Activate".
This should be set to level 0.


