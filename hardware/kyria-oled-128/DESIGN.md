# 1.12" 128×128 OLED carrier for the Kyria

Status: concept. No KiCad files yet. Board drawing: [outline.svg](outline.svg).

## Goal

A drop-in replacement for the splitkb SSD1306 module on a Kyria rev3 with a Liatris:

- Same 4-pin socket in the same order: **GND, VCC, SCL, SDA**, read left to right from the front with the pins at the top. This matches J2 on the Kyria rev3 schematic (pin 1 = GND).
- Runs from the 5 V the Liatris puts on VCC by default, and also from 3.3 V.
- Copes with the Kyria's own I2C pull-ups, which go to VCC (5 V on your Liatris).
- 128×128 mono display with an SH1107 controller.
- Smallest possible footprint: a glass-sized body, **25.9 × 30.1 mm**, plus a 12 mm-wide tab where the ribbon wraps, for **32.4 mm** total length.
- Nothing that isn't needed: no mounting holes, no extra connectors.

## Fit in the Kyria rev3

How I measured: the OLED cover in splitkb's rev3 case files has exactly the same outline as the top plate's cut-out around the controller, so it marks the space the screen sits in. I placed it using the plate's thumb-arc centre. I then located the OLED socket and the cover's spacer holes on splitkb's top-down photo of the rev3 PCB, scaled on the controller footprint and the cover holes.
Expect about ±1 mm of error. Sources: [case files](https://github.com/splitkb/kyria/tree/master/rev3), [rev3 schematic](https://docs.splitkb.com/doc/kyria_rev3.pdf), [build guide](https://docs.splitkb.com/product-guides/aurora-series/build-guide).

- **Where the screen sits**: the OLED socket is just below the controller, on the thumb-key side. The screen extends from the socket **up over the controller**, toward the keyboard's top edge.
- **Lengthwise**: the stock 128×64 module reaches 24.9 mm past the socket's pin row. This board reaches 28.8 mm at its corners and 31.1 mm at the ribbon tab.
  - The cut-out is shortest at the inner edge, where the keyboard's top-edge curve comes down: about 32.1 mm from the pin row, so about **3.4 mm to spare** at the board's corner.
  - At the tab (12 mm wide, centred) there's about 35.6 mm, so about **4.5 mm to spare**.
  - With square corners the full width would reach 31.1 mm and leave only about 1 mm at the inner corner. That's why the ribbon end is a narrow tab.
- **Sideways**: it lines up the same way as the stock module. That leaves about 0.3 mm to the cut-out's inner edge and about 0.2–0.4 mm to the cover spacer next to the BAT connector. This board is 0.2 mm narrower than the stock module, so it's no tighter.
- **Height**: about 3.9 mm without pins, against 5.9 mm for the stock module, so it fits under the OLED cover on its 12 mm spacers with more room than before.
- **Controller**: the screen covers more of the Liatris than the stock module, about 4 mm further up it. The build guide's advice applies: put Kapton or electrical tape on the back of the screen board so its parts can't touch the controller pins.
- **Drawing**: [kyria-fit.svg](kyria-fit.svg) shows the board, the stock module, the cut-out and the cover spacers to scale.
- **Other half**: the halves are mirror images, so the same board fits both.

## Panel

**Raystar REX128128G**: a bare 1.12" panel with the SH1107 bonded to the glass (chip-on-glass) and a ribbon tail you solder.
Alternative: EastRising ER-OLED1.12-1W, which has the same size and pin count. Its pinout isn't checked against the REX yet.

| Item | Value |
|---|---|
| Glass | 25.9 ± 0.2 × 30.1 ± 0.2 × 1.26 mm |
| Active area | 20.076 × 20.076 mm, 0.157 mm pixel pitch |
| Active area position | 2.91 mm from the glass's left edge, 1.93 mm from its top edge |
| Ribbon (FPC) | 22 pins, 0.7 mm pitch, 0.4 mm wide contacts, solder type |
| Ribbon geometry | 11.1 mm tail below the glass; 10.3 mm wide neck, widening to 19.3 mm for the last 5 mm |
| Ribbon bend limit | No bending within 2.0 mm of the glass edge |
| VDD (logic) | 1.65–3.3 V, absolute max 3.6 V |
| VPP (panel) | 7.0–12.5 V, typ 12 V, absolute max 17 V |
| IPP | 15 mA typ, 25 mA max at 12 V (50 % checkerboard) |

### Pin use (I2C mode)

| Pin | Name | Connect to | Why |
|---|---|---|---|
| 1 | VPP | VPP net (12 V) + C6 | Panel supply from the step-up converter |
| 2 | VCOMH | C7 to GND | Datasheet: capacitor to GND |
| 3 | VDD | 3V3 + C3, C4 | Logic supply |
| 4 | NC | — | |
| 5 | IM1 | 3V3 | I2C mode needs IM1 = 1 |
| 6 | IM2 | GND | I2C mode needs IM2 = 0 |
| 7 | IREF | R3 to GND | Sets the segment current |
| 8 | CS | GND | Can stay low in I2C mode |
| 9 | RES | RC reset (R6, C8, D2) | No reset pin on the 4-pin header |
| 10 | A0 / SA0 | JP1, default GND | Address 0x3C (0x3D if bridged to 3V3) |
| 11 | WR | 3V3 | Must be fixed high or low; SH1107 reference circuit ties it high |
| 12 | E/RD | 3V3 | Same as WR |
| 13 | D0 | SCL_L | |
| 14 | D1 | SDA_L | |
| 15–20 | D2–D7 | GND | High impedance in I2C mode; datasheet recommends tying to VDD or GND |
| 21 | GND | GND | |
| 22 | NC | — | |

## Schematic outline

```
J1 VCC (3.3–5.5 V) ──┬── C1 10µ ── GND
                     ├── U1 XC6206P332 (LDO) ──┬── 3V3 ──► VDD, IM1, WR, E/RD, pull-ups, RES pull-up, U2 EN, Q1 gates
                     │                          └── C3 4.7µ + C4 100n
                     └── U2 TPS61040 VIN
                         SW ── L1 10µH ── VIN
                         SW ── D1 Schottky ──┬── VPP (12 V) ──► panel pin 1
                                             ├── C5 1µ/25V, C6 4.7µ/25V
                                             └── R1 1M ──┬── FB
                                                 C9 Cff ─┘   R2 115k ── GND

Level shifter, one channel each for SCL and SDA (Q1 = dual N-MOSFET):
J1 SCL ── drain  Q1a  source ── SCL_L ── R4 4.7k ── 3V3, panel D0     gate ── 3V3
J1 SDA ── drain  Q1b  source ── SDA_L ── R5 4.7k ── 3V3, panel D1     gate ── 3V3
J1 side pulled up by the Kyria's R1/R2 (4.7k to VCC); R7/R8 = optional pull-ups, not fitted

RES ── R6 10k ── 3V3 ;  RES ── C8 1µ ── GND ;  D2: anode RES, cathode 3V3
```

### Netlist

| Net | Connections |
|---|---|
| VIN | J1.2, C1, C2, U1.VIN, U2.VIN, L1.1, R7, R8 (not fitted) |
| GND | J1.1, U1.GND, U2.GND, R2, R3, ground side of C1–C8, panel 6, 8, 15–21, JP1 default |
| 3V3 | U1.VOUT, C3, C4, U2.EN, Q1 gates, R4, R5, R6, D2 cathode, panel 3, 5, 11, 12 |
| SW | U2.SW, L1.2, D1 anode |
| VPP | D1 cathode, C5, C6, R1, C9, panel 1 |
| FB | U2.FB, R1, R2, C9 |
| VCOMH | panel 2, C7 |
| IREF | panel 7, R3 |
| RES | panel 9, R6, C8, D2 anode |
| SA0 | panel 10, JP1 |
| SCL (keyboard side) | J1.3, Q1a drain, R7 |
| SDA (keyboard side) | J1.4, Q1b drain, R8 |
| SCL_L (panel side) | Q1a source, R4, panel 13 (D0) |
| SDA_L (panel side) | Q1b source, R5, panel 14 (D1) |

**Why the level shifter**: the Kyria rev3 has its own 4.7 kΩ pull-ups (R1/R2) from SDA/SCL to VCC. On your Liatris VCC is 5 V, so the bus idles near 5 V. The SH1107's inputs may go at most 0.3 V above its 3.3 V supply.
Q1 keeps the panel side at 3.3 V, while the controller and the Kyria pull-ups see the bus exactly as they do now. It's one SOT-363 part.
If R1/R2 aren't fitted on your Kyria, the keyboard side has no pull-up and the bus won't work. Fit them as the build guide says, rather than R7/R8 here: R7/R8 would pull the controller's pins to VCC.

The step-up converter runs from VIN rather than from the 3.3 V rail. The LDO then only feeds the logic (under 1 mA), so a small 200 mA part is plenty, and the converter's input current doesn't pass through it.
U2's EN pin is tied to 3V3, so the 12 V rail comes up after the logic supply.

### Values and calculations

- **VPP** = 1.233 V × (1 + R1/R2) = 1.233 × (1 + 1000/115) = **11.95 V**, inside the panel's 7–12.5 V.
- **IREF resistor (R3)**: the SH1107 datasheet gives 750 kΩ at VPP = 15 V for 15.625 µA. That puts the IREF pin about 3 V below VPP, so at 12 V: (12 − 3) V / 15.625 µA ≈ 576 kΩ, rounded to **560 kΩ**.
  680 kΩ, which the ER module appears to use, gives a slightly dimmer panel that draws less current.
- **Reset**: 10 kΩ × 1 µF ≈ 10 ms low after power-up; the SH1107 needs at least 10 µs. D2 empties C8 at power-off so a quick replug still resets.
- **I2C**:
  - Panel side: 4.7 kΩ to 3V3.
  - Keyboard side: the Kyria's 4.7 kΩ to VCC.
  - Both rise in about 0.25 µs on a bus this short, under the 0.3 µs limit at 400 kHz.
  - Q1 needs a gate threshold well under 3.3 V: BSS138 is 0.8–1.5 V.
- **Current from VCC**: panel 15–25 mA at 12 V → about 45–75 mA at 5 V with a converter around 85 % efficient. Logic adds under 1 mA.
- **U2 headroom**: the TPS61040's 400 mA switch limit gives roughly 70 mA at 12 V from 5 V, or about 45 mA from 3.3 V. That's enough for the panel's 25 mA maximum.
  If you want more margin, an MT3608 (SOT-23-6, 0.6 V reference) also works: R1 191 kΩ, R2 10 kΩ → 12.06 V.

## Parts list

| Ref | Qty | Value | Package | Example part | Note |
|---|---|---|---|---|---|
| DS1 | 1 | 1.12" 128×128 OLED, SH1107 | 22-pin solder FPC | Raystar REX128128G | or EastRising ER-OLED1.12-1W (check its pinout first) |
| J1 | 1 | 1×4 header, 2.54 mm | THT, pads on the back only | loose header pins, or Mill-Max pins for low-profile sockets | Order GND, VCC, SCL, SDA; pin 1 = GND (square pad). Sits under the glass: soldered on the back, cut flush on the front |
| U1 | 1 | 3.3 V LDO, 200 mA, Vin ≤ 6 V | SOT-23-3 | Torex XC6206P332MR | Pin 1 VSS, 2 VOUT, 3 VIN (check footprint) |
| U2 | 1 | Step-up converter, 28 V / 400 mA switch | SOT-23-5 | TI TPS61040DBVR | Pin 1 SW, 2 GND, 3 FB, 4 EN, 5 VIN |
| Q1 | 1 | Dual N-MOSFET, logic level | SOT-363 | Diodes Inc. BSS138DW | I2C level shifter; check the footprint's pin map |
| L1 | 1 | 10 µH, Isat ≥ 0.5 A | ≤ 3 × 3 × 1.5 mm | e.g. Sunlord SWPA3015S100MT | |
| D1 | 1 | Schottky, 30 V 0.5 A | SOD-323 | Diodes Inc. B0530WS | |
| D2 | 1 | Small-signal diode | SOD-323 | 1N4148WS | Empties the reset capacitor at power-off |
| R1 | 1 | 1.0 MΩ 1 % | 0402 | | Feedback, top |
| R2 | 1 | 115 kΩ 1 % | 0402 | | Feedback, bottom |
| R3 | 1 | 560 kΩ 1 % | 0402 | | IREF; 680 kΩ for a dimmer panel |
| R4, R5 | 2 | 4.7 kΩ | 0402 | | I2C pull-ups, panel side, to 3V3 |
| R6 | 1 | 10 kΩ | 0402 | | RES pull-up |
| R7, R8 | 0 | 4.7 kΩ | 0402 | | Keyboard-side pull-ups to VIN. **Not fitted**; the Kyria's R1/R2 do this |
| C1 | 1 | 10 µF 10 V X5R | 0805 | | VIN bulk |
| C2 | 1 | 1 µF 10 V | 0402 | | U1 input, next to U1 |
| C3 | 1 | 4.7 µF 10 V | 0603 | | 3V3, at U1 output |
| C4 | 1 | 100 nF | 0402 | | 3V3, at panel pin 3 |
| C5 | 1 | 1 µF 25 V X7R | 0805 | | VPP, at D1 |
| C6 | 1 | 4.7 µF 25 V X5R | 0805 or 1206 | | VPP, at panel pin 1 (1206 loses less capacitance at 12 V) |
| C7 | 1 | 4.7 µF 25 V X5R | 0805 | | VCOMH (about 9 V) |
| C8 | 1 | 1 µF 10 V | 0402 | | Reset timing |
| C9 | 1 | 4.7 pF C0G | 0402 | | Feed-forward on R1. TPS61040 datasheet: C_FF = 1 / (2π · f_s/20 · R1); confirm against f_s at your load |
| JP1 | 1 | 3-pad solder jumper | — | | SA0: GND (0x3C, default) or 3V3 (0x3D) |

That's 22 placed parts plus the panel and header. All the small parts go on the back, so JLCPCB can assemble them in one pass.

## Board outline

All sizes in mm. Front view: display side up, header at the top. See [outline.svg](outline.svg).
On the Kyria the header edge faces the thumb keys and the ribbon tab faces the keyboard's top edge.

The footprint is set by two things that can't shrink: the glass (25.9 × 30.1) and the ribbon, which mustn't bend within 2.0 mm of the glass edge. The ribbon is only 10.3 mm wide where it wraps, so only a narrow tab needs to stick out past the glass.

| Item | Value |
|---|---|
| Body | **25.9 × 30.1**, flush with the glass on all four sides. 1.0 mm thick FR4, no holes |
| Ribbon tab | **12.0 × 2.2**, centred on the bottom edge (x 6.95–18.95, y 30.1–32.3), 0.5 mm corner radius |
| Footprint | 25.9 wide, 32.4 long at the tab including the wrapped ribbon, 30.1 at the corners |
| Compared to | splitkb SSD1306 26.1 × 26.2; BuyDisplay ER module 26.2 × 40.1; first draft 26.0 × 36.0 |
| Header J1 | 4 pins, 2.54 mm pitch, centred, **under the glass**. Pin centres 1.30 from the top edge (same as a generic SSD1306 module) at x = 9.14 (GND), 11.68 (VCC), 14.22 (SCL), 16.76 (SDA) |
| Header pads | Back side only: Ø1.6 pad, Ø1.0 drill, no copper ring on the front. The pins end flush with the front surface and are soldered on the back. |
| Glass position | x 0–25.9, y 0–30.1 (±0.2) |
| Active area on the board | x 2.91–22.99, y 1.93–22.01 |
| Ribbon fold | Runs flat on the front from y 30.1 across the tab to its edge at 32.3 (2.2, or 2.0 if the glass comes out at +0.2), then wraps around the tab edge onto the back |
| Ribbon pads (back) | 22 pads, 0.40 × 3.0, 0.7 pitch, centred, y 24.1–27.1. Pin 1 at x = 5.60 in front-view coordinates (the right-hand end seen from the back). The ribbon end lands at about y 24.7; the 3.0 mm pads absorb fold variation (the contacts are 1.8 mm). The ribbon's 19.3 mm-wide end section lies on the body, y 24.7–29.7. |
| Parts zone (back) | x 0.5–25.4, y 2.6–23.5 (25 × 21), parts ≤ 1.6 mm tall |
| Placement | U1, C1–C3, Q1 and the pull-ups below the header. Step-up parts (U2, L1, D1, C5, C6, C9, R1, R2) near ribbon pins 1–3. C4, C7, R3, R6, C8, D2 near ribbon pins 2–9. |
| Total thickness | about 3.9 mm without pins: glass 1.26 + tape 0.1 + PCB 1.0 + parts 1.5. The splitkb module is 5.9 mm. |

**Why the ribbon can't go at the header end**: folded onto the back, the ribbon covers a band from the edge to about 7.6 mm in, and its 10.3–19.3 mm width covers all four centred header pins. The ribbon has to go at the end opposite the header.

## Assembly

1. **PCB**: order from JLCPCB with the back-side parts assembled.
2. **Header first**: push loose pins (or a strip with the plastic slid down out of the way) in from the back until they're flush with the front surface. Solder them on the back, then file the front flat; the glass will cover them.
3. **Panel**: stick it down with 0.1 mm double-sided tape, which also insulates the pin ends. Line it up flush with the body's edges. Fold the ribbon around the tab and solder the 22 pads: flux plus drag soldering at 0.7 mm pitch, or a hot-bar iron.
4. **Insulate**: cover the back with Kapton or electrical tape (it sits over the controller pins).
5. **Test**: before plugging into the Kyria, power VCC with 5 V and check 3.3 V on C3 and about 12 V on C6.

## Firmware (QMK)

- **Config**: in the keymap's `config.h`, add `#define OLED_DISPLAY_128X128` (and `#undef OLED_DISPLAY_128X64` if the keyboard sets it). QMK then picks the SH1107 and a 2048-byte buffer (`drivers/oled/oled_driver.h`, `OLED_DISPLAY_128X128` block).
- **Address**: QMK's default, 0x3C, matches JP1 in its default position.
- **Brightness**: consider `#define OLED_BRIGHTNESS 128` (default 255) to halve the panel current.
- **Your OLED code**: `oled_canvas.c`, `oled_master_zones.c`, the `oled_slave_*.c` screens and the generated pictures in `art/` lay out pages 0–7 (128×64). 128×128 has pages 0–15.
- **Tests**: `tests/unit/stub/qmk_keyboard.h` hardcodes `OLED_DISPLAY_HEIGHT 64`.

## Check before ordering

- [ ] ER-OLED1.12-1W pinout matches the REX128128G, if you buy the EastRising panel.
- [ ] Which face of the ribbon carries the contacts. The fold-around-the-edge layout assumes it's the face toward the PCB, as on the BuyDisplay module.
- [ ] **Measure on your Kyria** (my figures are ±1 mm from a photo):
  - from the centre of the OLED socket's pins toward the top edge, along the inner edge of the top plate's cut-out: needs at least 28.8 mm (I estimate about 32);
  - the same along the socket's centre line: needs at least 31.1 mm (I estimate about 38).
- [ ] Where your current module's pin row sits relative to its edge; I assumed 1.3 mm.
- [ ] R1/R2 are fitted on your Kyria (the level shifter needs them).
- [ ] C9 and L1 against the TPS61040 datasheet.

## Sources

- Raystar REX128128G page and datasheet: <https://www.raystar-optronics.com/oled-graphic-display-module/REX128128G.html>
- SH1107 datasheet (Sino Wealth, V2.1): <https://www.displayfuture.com/Display/datasheet/controller/SH1107.pdf>
- Kyria rev3 schematic (J2 pinout, R1/R2 pull-ups to VCC): <https://docs.splitkb.com/doc/kyria_rev3.pdf>
- Kyria rev3 case files (OLED cover and top plate): <https://github.com/splitkb/kyria/tree/master/rev3>
- Kyria rev3 PCB photo: <https://splitkb.com/products/kyria-rev3>
- Aurora / Kyria rev3 build guide (OLED sockets, cover, resistors): <https://docs.splitkb.com/product-guides/aurora-series/build-guide>
- EastRising ER-OLEDM1.12-1 datasheet (reference module, 26.2 × 40.1)
- QMK OLED driver: <https://github.com/qmk/qmk_firmware/blob/master/drivers/oled/oled_driver.h>
- splitkb SSD1306 module: <https://splitkb.com/products/oled-display>
