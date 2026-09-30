# A-B Comparator Bench Schematic

## Scope

This is the phase-one isolated bench circuit for detecting an A-B closure with
one channel of an MCP6562 dual comparator and a Seeed XIAO ESP32-C3.

It is intended to validate contact resistance, 6 ms qualification, radio noise,
and ESP-NOW timing. It is not yet competition-correct because it does not
classify opponent guard or piste contact through the C conductor.

Do not connect this prototype to a fencer or mains-referenced test equipment.
Use an isolated switch/resistance fixture first.

## Functional Diagram

```text
                    XIAO 3V3
                       |
                  R1 2.20 kOhm
                       |
Epee socket A --- R2 --+-------- SENSE -------- R5 10 kOhm ------- XIAO D0/ADC
                22 Ohm |                           optional
                       |
                       +-------- R3 10 kOhm ------- MCP6562 pin 2 (INA-)
                       |
                      C1
                    1 nF
                       |
                      GND

Epee socket B --- R4 22 Ohm ----------------------- GND

Epee socket C -------------------------------------- not connected in phase one
```

The comparator reference is adjustable for characterization:

```text
XIAO 3V3 -----------+
                    |
                 RV1 10 kOhm multi-turn potentiometer
                    |
GND ----------------+
                    ^ wiper
                    |
                  R6 10 kOhm
                    |
                    +---------------- MCP6562 pin 3 (INA+)
                    |
                  C2 100 nF
                    |
                   GND

MCP6562 pin 1 (OUTA) ---------------- XIAO D2 (digital hit input)
MCP6562 pin 8 (VDD) ----------------- XIAO 3V3
MCP6562 pin 4 (VSS) ----------------- GND
C3 100 nF directly between pins 8 and 4
```

Optional hysteresis should be added only after the resistance sweep establishes
the required switching point:

```text
MCP6562 pin 1 (OUTA) ---- R7 2.2 MOhm ---- pin 3 (INA+)
```

R7 provides positive feedback and reduces chatter. Its final value must be
selected from measured rising and falling resistance thresholds.

## MCP6562 DIP/SOIC-8 Pin Use

Verify the datasheet and package marking for the exact ordered part before
wiring.

| Pin | MCP6562 function | Prototype connection |
|---:|---|---|
| 1 | OUTA | XIAO D2 and optional R7 |
| 2 | INA- | SENSE through R3 |
| 3 | INA+ | Adjustable reference through R6 |
| 4 | VSS | GND |
| 5 | INB+ | GND; unused channel held defined |
| 6 | INB- | 3V3; unused channel held defined |
| 7 | OUTB | Leave unconnected |
| 8 | VDD | 3V3 |

The comparator output is expected to be high when the A-B loop resistance pulls
SENSE below the reference voltage. Firmware must verify the actual polarity on
the bench before treating a high level as contact.

## Initial Threshold Calculation

Ignoring R2/R4 and cable resistance, SENSE is approximately:

$$
V_{sense} = 3.3 \times \frac{R_{contact}}{2200 + R_{contact}}
$$

For a total loop resistance of 160 ohms:

$$
V_{sense} \approx 3.3 \times \frac{160}{2360} \approx 0.224\text{ V}
$$

Set RV1 initially so pin 3 measures about 0.22 V. This is a starting point for
characterization, not a final production threshold. R2, R4, body cord, socket,
weapon wire, and contact resistance all contribute to the real loop resistance.

At a near-zero-ohm closure, R1 limits current to approximately:

$$
I \approx \frac{3.3}{2200 + 22 + 22} \approx 1.47\text{ mA}
$$

## Connector Protection

R2 and R4 limit transient current but are not complete ESD protection. Before
testing with a real weapon, add a low-capacitance ESD device at the connector.
The selected device must:

- be rated for contact-discharge conditions expected at the weapon connector
- clamp within the comparator and XIAO input limits
- have sufficiently low leakage not to shift the resistance threshold
- return transients to the local battery ground with a short layout path

Do not place an unspecified high-capacitance TVS on SENSE and assume timing is
unchanged. Record the exact part and repeat the resistance/timing tests after it
is installed.

## Breadboard Wiring Checklist

1. Leave the weapon socket disconnected.
2. Connect XIAO GND to MCP6562 pin 4.
3. Connect XIAO 3V3 to MCP6562 pin 8.
4. Place C3 directly across pins 8 and 4.
5. Tie unused pin 5 to GND and pin 6 to 3V3; leave pin 7 open.
6. Build the RV1 reference and verify pin 3 is adjustable before connecting
   SENSE.
7. Build R1, R2, R4, and a resistance-switch fixture in place of the weapon.
8. Connect SENSE to pin 2 through R3.
9. Connect pin 1 to XIAO D2.
10. Optionally connect SENSE to XIAO D0 through R5 for diagnostics.
11. Power from USB or an isolated battery for initial measurements.
12. Confirm no node exceeds 3.3 V or falls below GND in normal operation.

## Required Measurements Before Firmware Hit Logic

Record results in `PROJECT_RECORD.md` with instrument and component details:

- 3V3 rail voltage
- reference voltage at pin 3
- open-circuit SENSE voltage
- SENSE voltage at 0, 10, 50, 100, 160, 200, and 250 ohms
- comparator output at each resistance
- rising and falling switching resistance
- current through a zero-ohm fixture
- output chatter during switch bounce
- behavior during repeated Wi-Fi/ESP-NOW transmissions
- results before and after adding the ESD component

Only after these measurements should the reference and hysteresis be converted
from adjustable prototype parts to fixed production values.