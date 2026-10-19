# Kyria 128×128 OLED board: handoff

Everything needed to pick this up on another machine. The design itself is in [DESIGN.md](DESIGN.md); this file covers the background, the current state and the next steps.

## In short

The Kyria rev3 with Liatris controllers takes a splitkb 128×64 SSD1306 module (26.1 × 26.2 mm) in a 4-pin socket.
The goal is a 128×128 screen that plugs into the same socket, takes the same 5 V, and stays as small as possible.
No off-the-shelf board met all of that, so this is a custom carrier board for a bare 1.12" 128×128 SH1107 panel:

- **Size**: 25.9 × 30.1 mm, plus a 12 × 2.2 mm tab where the panel's ribbon wraps around.
- **Power**: a 3.3 V regulator for the logic, and a step-up converter for the panel's 12 V.
- **I2C**: a level shifter, because the Kyria pulls the bus up to 5 V.
- **Firmware**: QMK already supports the SH1107 at 128×128.

## Status

| Done | Not done |
|---|---|
| Panel chosen, its datasheet read (pinout, ribbon geometry, voltages) | KiCad project: schematic, footprints, PCB |
| Circuit: parts list, netlist, values ([DESIGN.md](DESIGN.md)) | ERC / DRC |
| Board outline, to scale ([outline.svg](outline.svg)) | Gerbers, BOM, placement file for JLCPCB |
| Fit in the Kyria rev3 OLED cut-out, from the case files and a PCB photo ([kyria-fit.svg](kyria-fit.svg)) | Measuring the real Kyria to confirm the fit |
| Kyria rev3 schematic checked: socket pinout, I2C pull-ups | Checking the EastRising panel's pinout (its site was blocked at work) |
| | Ordering, assembly, firmware changes |

## Files here

| File | What |
|---|---|
| [DESIGN.md](DESIGN.md) | The design: goal, fit, panel, pin use, schematic outline, netlist, values, parts list, board outline, assembly, firmware, checklist, sources |
| [outline.svg](outline.svg) | Board front, back (mirrored) and side, to scale in mm, with the stock module for comparison |
| [kyria-fit.svg](kyria-fit.svg) | The board inside the Kyria rev3 OLED cut-out (right half, top view), with the stock module, spacers and Liatris |
| [tools/kyria_fit.py](tools/kyria_fit.py) | Regenerates `kyria-fit.svg` from splitkb's OLED cover DXF. The socket position constants at the top are photo estimates: replace them with measurements |
| [tools/dxf_dump.py](tools/dxf_dump.py) | Minimal DXF reader used by the script; `python3 dxf_dump.py FILE.dxf` lists the entities |
| `.gitignore` | Keeps the downloaded DXFs and datasheets, and KiCad's scratch files, out of git |

## Decisions so far, and why

1. **No off-the-shelf board fits.** I checked every 128×128 board I could find:

   | Board | Size (mm) | Why not |
   |---|---|---|
   | BuyDisplay ER-OLEDM1.12-1-I2C | 26.2 × 40.1 | Pinout matches (GND VCC SCL SDA, plus an unused RES), but VCC is 3.0–3.5 V only. There's no place for a regulator on the back; its U2 is a step-up for the panel voltage. |
   | ZJY 1.12" module | 26.2 × 41 | The same design, 3.3 V only |
   | Pimoroni 1.12" | 28 × 42 | Takes 3–6 V, but too wide, and its pins are 3-6V, SDA, SCL, unused, GND. The width can't be cut down: parts sit right up to the edges. |
   | Adafruit #5297 | 30.5 × 40.5 | Too wide, 9-pin header |
   | Seeed Grove 1.12" | 40 × 40 | Too big, Grove connector |
   | 1.5" SH1107 4-pin modules | 34.3 × 45.5 | Too wide |

   The 1.12" glass is already 25.9 mm wide, so a board that takes 5 V has nowhere to put the extra parts except beside it, or behind it on a custom board.
2. **Mono, not grayscale.** The only 128×128 grayscale OLED is a 1.5" panel with an SSD1327 controller: glass about 33.8 × 36.5 mm, 26.86 mm active area.
   That's about 8 mm too wide. QMK also has no SSD1327 driver, and Quantum Painter has no 4-bit grayscale buffer.
3. **Panel: Raystar REX128128G**: 1.12", 128×128, SH1107 bonded to the glass, 22-pin solder ribbon at 0.7 mm pitch, glass 25.9 × 30.1 × 1.26 mm.
   The EastRising ER-OLED1.12-1W looks the same but its pinout is unverified.
4. **Smallest footprint.**
   - The header pins sit under the glass, soldered on the back and flush on the front, 1.3 mm from the edge like the stock module.
   - The ribbon may not bend within 2.0 mm of the glass, so a 12 mm-wide tab sticks out 2.2 mm to carry the fold.
   - The ribbon can't go at the header end: folded onto the back it would cover the header pins.
5. **Fit on the Kyria rev3.**
   - The socket sits just below the controller, and the screen extends up over the Liatris toward the keyboard's top edge.
   - With the corners trimmed back to the glass, there's about 3.4 mm spare at the inner corner and 4.5 mm at the tab.
   - Sideways it's as tight as the stock module: about 0.3 mm to the cut-out edge and 0.2–0.4 mm to a cover spacer.
   - These numbers come from splitkb's case DXFs plus a product photo, about ±1 mm.
6. **Level shifter on SDA/SCL.** The Kyria rev3 schematic shows R1/R2 as 4.7 kΩ pull-ups to VCC, which is 5 V on the Liatris by default. The SH1107's inputs are rated to VDD + 0.3 V, about 3.6 V. A dual N-MOSFET (BSS138DW) fixes that.
7. **Power.**
   - The step-up converter (TPS61040) runs straight from VCC and makes 11.95 V for the panel.
   - A small LDO (XC6206, 3.3 V) only feeds the logic.
   - Expect 45–75 mA from 5 V with half the pixels lit.
8. **Socket pinout confirmed** from the Kyria rev3 schematic (J2): pin 1 GND, VCC, SCL, SDA.

## Next steps

### 1. Measure the Kyria
With the OLED cover off, use calipers on one half.
- **From the socket's pin row toward the top edge**, to the edge of the top plate's cut-out:
  - along the inner edge: at least 28.8 mm needed, about 32 expected;
  - along the socket's centre line: at least 31.1 mm needed, about 38 expected.
- **Sideways**, from the centre of the socket's pins:
  - to the cut-out's inner edge: about 13.3 mm expected; the board needs 12.95;
  - to the nearer cover spacer: should leave at least 12.95 mm plus the spacer's radius.
- **The stock module**: its pin row to its edge (1.3 assumed), and its height above the Liatris.

Then put the real numbers into `tools/kyria_fit.py` (`PIN_Y`, `PIN_CX`, `LIATRIS`) and regenerate the drawing:

```bash
cd hardware/kyria-oled-128/tools
curl -L -o oled_cover.dxf "https://raw.githubusercontent.com/splitkb/kyria/master/rev3/Kyria%20rev3%20OLED%20Cover%20-%20No%20Kerf.dxf"
python3 kyria_fit.py oled_cover.dxf ../kyria-fit.svg
```

The script works in the top plate's coordinates (mm, y up); `PIN_CX`/`PIN_Y` are the socket's position there. If measuring in those coordinates is awkward, measure the distances above and adjust the constants until the drawing matches.

### 2. Settle the panel
- Open the [ER-OLED1.12-1W page](https://www.buydisplay.com/spi-i2c-white-1-12-inch-oled-display-panel-128x128-pixels) and its datasheet, and compare the 22-pin pinout and outline with the REX128128G table in DESIGN.md. If it matches, it's the cheap, easy-to-buy option.
- Check which face of the ribbon carries the contacts. The fold-around design needs them on the face toward the PCB, as on BuyDisplay's own module.
- Check Raystar REX128128G availability through its distributors or as samples.
- Order at least three: one per half plus a spare, since the ribbon solder joints are fiddly.

### 3. KiCad project
- **Install**: `brew install --cask kicad` (KiCad 10). Create the project in `hardware/kyria-oled-128/kicad/`.
- **Symbols**: KiCad's libraries have most of them. Search for:
  - `Conn_01x04` (J1)
  - `XC6206` (U1)
  - `TPS61040` (U2)
  - a dual N-MOSFET in SOT-363 (Q1), e.g. `BSS138DW` or `2N7002DW`; any logic-level part with a gate threshold under 1.5 V works
  - `L`, `D_Schottky`, `D`, `R`, `C`
  - `SolderJumper_3_Bridged12` (JP1, SA0, default to GND)
  - The panel (DS1) needs a custom 22-pin symbol: pins per the table in DESIGN.md.
- **Footprints**, two of them custom:
  - **J1**: 1×4, 2.54 mm pitch, Ø1.0 drill, Ø1.6 pad on the back. The front gets the smallest ring KiCad allows, covered by solder mask: the glass sits on it. Pin 1 (GND) square.
  - **DS1 ribbon pads**: 22 SMD pads, 0.40 × 3.0 mm, 0.7 mm pitch, on B.Cu. Add a silkscreen outline of the ribbon's 19.3 mm-wide end and a pin-1 mark.
  - **Standard footprints**:
    - R: 0402
    - C: 0402 / 0603 / 0805 per the parts list
    - L1: 3 × 3 mm, e.g. Sunlord SWPA3015
    - U1: SOT-23; U2: SOT-23-5; Q1: SOT-363
    - D1, D2: SOD-323
    - JP1: 3-pad solder jumper
- **Board setup**: 2 layers, 1.0 mm FR4. Every component goes on the back (B.Cu); the front only carries the glass on tape.
- **Edge.Cuts** in front-view coordinates (mm, origin top-left, y down):
  - Body: (0, 0) → (25.9, 0) → (25.9, 30.1) → (18.95, 30.1)
  - Tab, right side: down to (18.95, 31.8), arc of r 0.5 to (18.45, 32.3)
  - Tab, bottom: across to (7.45, 32.3), arc of r 0.5 to (6.95, 31.8)
  - Tab, left side: up to (6.95, 30.1), then → (0, 30.1) → back to (0, 0)
  - The inside corners where the tab meets the body get rounded by the router (about r 0.5); that's fine.
- **Fixed positions**, front-view coordinates:
  - **J1 pins**: y 1.30, at x 9.14 (GND), 11.68 (VCC), 14.22 (SCL), 16.76 (SDA).
  - **Ribbon pads**: centred on x 12.95, spanning y 24.1–27.1. Pin 1 at x 5.60, pin 22 at x 20.30. On the back, pin 1 is the right-hand end.
  - **Glass**: x 0–25.9, y 0–30.1, on F.Fab. The active area is x 2.91–22.99, y 1.93–22.01.
- **Parts zone** on the back: x 0.5–25.4, y 2.6–23.5, nothing taller than 1.6 mm.
- **Placement**:
  - Q1, U1, C1–C3, R4, R5 and JP1 just below the header.
  - The step-up group (U2, L1, D1, C5, C6, C9, R1, R2) near ribbon pins 1–3.
  - C4, C7, R3, R6, C8 and D2 near ribbon pins 2–9.
- **Routing**:
  - Keep the loop U2 SW → L1 → D1 → C5 → GND tight, and keep VPP to ribbon pin 1 short.
  - Ground pour on both layers.
  - 0.2 mm traces for signals and the ribbon fan-out, 0.3–0.4 mm for power.
  - Tented 0.3 / 0.6 mm vias.
- **Checks**: run ERC and DRC against JLCPCB's rules.

### 4. Order
- **Boards**: JLCPCB, 2 layers, 1.0 mm, with assembly on the bottom side.
  - Export Gerbers, drill, BOM and placement (CPL) files, for example with the "Fabrication Toolkit" plugin from KiCad's Plugin and Content Manager.
  - Pick an LCSC part for each BOM line, basic parts where possible.
- **Also buy**:
  - the panels;
  - header pins, or Mill-Max low-profile pins if your sockets are the low-profile kind;
  - 0.1 mm double-sided tape, Kapton tape, flux.

### 5. Assemble and test
Follow the Assembly section of DESIGN.md:
1. Solder the header first.
2. Tape the panel down.
3. Fold the ribbon around the tab and solder it.
4. Tape the back.
5. Check 3.3 V and about 12 V from a 5 V supply before plugging in.

### 6. Firmware (this repo)
- **Config**: in `keyboards/splitkb/kyria/keymaps/rvannoord/config.h`, add `#define OLED_DISPLAY_128X128`, plus `#undef OLED_DISPLAY_128X64` if the keyboard defines it. QMK then selects the SH1107 and a 2048-byte buffer.
- **Address**: 0x3C, QMK's default (JP1 at its default).
- **Brightness**: consider `#define OLED_BRIGHTNESS 128` to halve the panel current.
- **Drawing code**: `oled_canvas.c`, `oled_master_zones.c` and the `oled_slave_*.c` screens assume 8 pages of 128 columns. 128×128 has 16 pages.
  The pictures in `art/` (`panel_font.c`, `panel_glyphs.c`, `panel_layer_icons.c`, the Bjorn, space and world art) are generated: change their generators under `tools/`, never the `.c` files.
  The keymap's `readme.md` has the page format and the split facts.
- **Tests**: `tests/unit/stub/qmk_keyboard.h` hardcodes `OLED_DISPLAY_HEIGHT 64`.
- **Rotation**: the new board mounts the same way round as the stock module, so start from the current rotation setting.
- **Timing**: `docs/oled-v2.md` is the current OLED plan for 128×64. Decide whether to finish it first or fold the 128×128 move into it.

## Still to verify

- [ ] Kyria measurements (step 1)
- [ ] ER-OLED1.12-1W pinout = REX128128G, if buying from BuyDisplay
- [ ] Ribbon contact side
- [ ] R1/R2 fitted on both halves (the level shifter relies on them for the keyboard-side pull-up)
- [ ] TPS61040 feed-forward capacitor (C9) and inductor choice against TI's datasheet
- [ ] IREF resistor: 560 kΩ is worked out from the SH1107's 750 kΩ at 15 V. Check brightness and current on the first board; 680 kΩ is dimmer.

## Sources

- Panel: [Raystar REX128128G](https://www.raystar-optronics.com/oled-graphic-display-module/REX128128G.html), [datasheet](https://www.raystar-optronics.com/upload_files/oled-display/128x128-oled-graphic-display/REX128128G-datasheet.pdf), [outline drawing](https://www.raystar-optronics.com/upload_files/oled-display/128x128-oled-graphic-display/REX128128G-contour-drawing.jpg); [EastRising ER-OLED1.12-1W](https://www.buydisplay.com/spi-i2c-white-1-12-inch-oled-display-panel-128x128-pixels)
- Controller: [SH1107 datasheet](https://www.displayfuture.com/Display/datasheet/controller/SH1107.pdf)
- Parts: [TPS61040 datasheet](https://www.ti.com/lit/ds/symlink/tps61040.pdf); XC6206 (Torex) and BSS138DW (Diodes Inc.) datasheets from their makers
- Kyria rev3: [schematic](https://docs.splitkb.com/doc/kyria_rev3.pdf), [case files](https://github.com/splitkb/kyria/tree/master/rev3), [build guide](https://docs.splitkb.com/product-guides/aurora-series/build-guide), [PCB photo](https://splitkb.com/cdn/shop/products/SKB-KYR-BMK-030-front.jpg)
- splitkb: [SSD1306 module](https://splitkb.com/products/oled-display), [Liatris](https://splitkb.com/products/liatris)
- Modules ruled out: [BuyDisplay ER-OLEDM1.12-1](https://www.buydisplay.com/i2c-white-1-12-inch-oled-display-module-128x128-arduino-raspberry-pi), [ZJY 1.12"](http://www.zjydisplay.com/product/34.html), [Pimoroni 1.12"](https://shop.pimoroni.com/products/1-12-oled-breakout), [Adafruit 5297](https://www.adafruit.com/product/5297), [Seeed Grove 1.12" V3.0](https://wiki.seeedstudio.com/Grove-OLED-Display-1.12-SH1107_V3.0/)
- Grayscale: [Raystar 1.5" REX128128A](https://www.raystar-optronics.com/oled-graphic-display-module/oled-128x128.html), [Adafruit 1.5" grayscale](https://learn.adafruit.com/adafruit-grayscale-1-5-128x128-oled-display/overview)
- QMK: [OLED driver](https://github.com/qmk/qmk_firmware/blob/master/drivers/oled/oled_driver.h), [docs](https://docs.qmk.fm/features/oled_driver), [Quantum Painter drivers](https://github.com/qmk/qmk_firmware/tree/master/drivers/painter)
