# Wireless Epee Transmitter

Prototype firmware for a battery-powered wireless epee sensor using a Seeed
Studio XIAO ESP32-C3.

## Current Status

The repository currently contains only a buildable PlatformIO foundation. It
does not yet read weapon contacts or transmit hit data. The electrical sensing
method and ESP-NOW packet format must be agreed and validated before those
features are implemented.

Project decisions, constraints, open questions, and test results are recorded
in [docs/PROJECT_RECORD.md](docs/PROJECT_RECORD.md). Add every material design
change and bench result there before changing the prototype architecture.

## Target Hardware

- Seeed Studio XIAO ESP32-C3
- protected 3.7 V LiPo battery
- external protected/comparator-based epee input circuit
- epee three-pin connector

Weapon wires must not be connected directly to ESP32 GPIO pins.

## Build

From this directory:

```powershell
py -m platformio run
```

## Upload

Connect the XIAO ESP32-C3 and replace `COMx` with its serial port:

```powershell
py -m platformio run --target upload --upload-port COMx
```

## Planned Development

1. Select left/right transmitter role and persistent configuration.
2. Design and bench-test protected A/B tip-contact sensing.
3. Define a versioned ESP-NOW packet shared with the scoring apparatus.
4. Add channel discovery using the apparatus `Piste_xxx` access point.
5. Validate latency, duplicate filtering, link loss, and double-hit timing.
6. Develop and verify guard and piste rejection using all three weapon lines.

The initial A/B-only prototype is for transport and timing experiments and is
not competition-correct.

## Safety

Do not connect the weapon socket directly to ESP32-C3 pins. Do not connect this
battery-powered prototype to mains-referenced equipment while it is attached to
a fencer. Initial tests must use an isolated contact/resistance fixture.