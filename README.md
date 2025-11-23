# USB to IrDA converter board

# THIS IS UNTESTED, DO NOT BUILD YET

## What is this?

This is a USB to IrDA adapter. Plug it into a computer and it will appear as a
serial/COM port.
Anything you write to that port will come out over the IR transceiver at the
top, encoded using the IrDA standard.

This was made to interface with the Pokewalker, since other IrDA adapter boards
are either expensive or not very sleek.

You can also plug in a UART cable if you want.
It uses the [Raspberry Pi UART/Debug spec](https://datasheets.raspberrypi.com/debug/debug-connector-specification.pdf).
Cables to adapt to 0.1" headers should be easily purchaseable online.
Pinouts are labeled on the board for UART TX and RX, and SWD data and clock.
There is also a pin header hole for the NRST pin to connect to an ST-link.

## Instructions

Until I/someone makes a JLC PCBA project for this, you will need to buy the 
parts yourself and assemble by hand.
All you should need is a soldering iron and needle-nose tweezers.
Hot air might help, if you have it available.
The parts have been chosen specifically so that you don't need special tools.

Tips:

- Change your soldering iron tip depending on the part.
    - A knife tip for the MCU, USB connector and IR transceiver might help.
    - Small chisel tip for the capacitors, LEDs and resistors.
- Use lots of flux.
- The direction of LEDs matter. Unfortunately there's no easy way to tell which
  is the right way.
  You can check the datasheet and use a microscope to tell, or just try one way
  and if it doesn't light up, try the other.
- Try not to spend too much time on a single part, only a few seconds at a
  time. Spending longer might risk damaging the component, especially the LEDs.
- Consider buying spares, especially for the passives (resistors, capacitors,
  LEDs) since they can be lost or damaged easily.

### Board

Order from your favourite PCB fab. Files provided will be for JLC. If you want
others (e.g. PCBway) then you'll have to generate them yourself from the KiCad
files.

This is a 2-layer board, 1.6mm thickness, lead-free HASL finish.
Don't bother with the stencil unless you want to use a hot plate/reflow oven/
hot air and solder paste.
You should be able to buy 5 bare PCBs for around $3 plus shipping.

### Parts

- IrDA transceiver x 1- TFBS4711 (either TR1 or TT1)
- MCU x 1- STM32C071FBP6 (NOT the -N varaint)
- USB connector x 1 of either:
  - 12402012E212A (Amphenol)
  - USB4215-03-A (GCT)
- 3.3V regulator x 1 - TLV1117LV33DCYR
- UART connectors x 1 - SM03B-SRSS-TB (or x2 if you want the debug header)
- Buttons x 2 - B3FS1012P
- 5.1k resistors x 12 - RC0603FR-105K1L
- 47R resistor x 2 - RC0603FR-1047RL
- 0.1u capacitor x 2 - MCASE168SB5104KTNA01
- 1u capacitor x 2 - LMK107BJ105MAHT
- 4.7u capacitors x 2 - LMK107BJ475KA-T
- LEDs x 4 
  - Green/yellow (574 nm) - KG EELP41.22-PHRH-35-A8J8-20-R18 
  - Yellow (590 nm) - 150060YS83000
  - Red (624 nm) - KR EELP41.22-Q1S1-36-A8J8-020-R18
  - Amber (610 nm) - 150060AS83000 
  - Blue (465 nm) - 150060BS83000 
  - Green (527 nm) - KT EELP41.12-S2U1-25-2X4X-5-R18 
  - White - KW EELP41.RU-S1U1-3K6L-3X4X-5-R18

Checked on Mouser USA and it cost $12 plus shipping and taxes for enough to
make one.
Building in quantity (10+) will significantly drop this price per unit.

There are four LEDs, TX activity, RX activity, power status and a spare.
Pick and choose which colour you want.
Use [this website](https://405nm.com/wavelength-to-color/) to get an idea of
the colour of each by putting in the wavelength (e.g. 590 nm).
Different colours will need different resistor values. There are two pads for
resistors in parallel so that you can have 2 x 5.1k resistors in parallel for 
approx 2.5k total resistance.
If going for different values, you do not need to populate both.

### Enclosure

Enclosure is made using FreeCad 1.0.
An STL file will be provided to be able to 3d print.
You can either print this yourself, or get a bureau service to do it, like JLC.

You will need 4x M3 x 12mm screws and 4x M3 nuts.

### Programming

You will need to download [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html),
unfortunately.

1. Plug the device in to your PC. Hold the `Boot` button then press the "Reset"
  button. This will cause the device to enter the DFU bootloader mode.
  The "user" LED should be lit up.
2. There are firmware files in the "releases" tab on the right in GitHub.
  Download either the `.elf` or `.dfu` file.
3. Open STM32CubeProgrammer.
  On the right there should be a dropdown box and a "connect" button.
  Change the dropdown from "ST-LINK" to "USB".
  In the "port" section below, select the device. There will probably only be one.
  If you do not see it there, click refresh to the right.
  If it is still not there, ensure the device is in bootloader mode.
  Click "connect".
4. In the tab on the left, click the "+" tab and choose "open file".
  Select the `.elf` file you downloaded in step 2.
5. Click "Download" near the top right.
  If all goes well, there should be a success popup.
6. Click "disconnect", then unplug your device and plug it back in.

Your device should now be flashed with the latest firmware, and show up as a
USB COM port.

## Development

If you want to play with the KiCad files, you can. There is a project library 
called `irda` which contains custom symbols, footprints and models of the
components used.

Generating the KiKit files from the KiCad shell

```bash
kikit panelize -p kikit.json .\irda-stm32.kicad_pcb panelized\panelized.kicad_pcb
```

To edit the firmware, you'll need
[STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html).

If you want to debug, you'll need to solder the SWD header. Its the same as
the UART header.
You can also add a 0.1" header for the NRST pin to connect to an ST-link.

If you want to edit the 3D files, use FreeCad 1.0 or higher.


## License

Free to use/modify/sell. Do whatever you want with this. Just don't blame me if
it doesn't work or breaks something else.

