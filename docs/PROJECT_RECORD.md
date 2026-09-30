# Wireless Epee Project Record

## Document Purpose

This is the durable engineering record for the wireless epee prototype. It
captures agreed direction, measured facts, assumptions, unresolved questions,
and test evidence. Update it whenever a decision changes or a test is run.

Last updated: 2026-09-30

## Project Goal

Build two battery-powered epee transmitters. Each unit attaches to one weapon's
three-pin socket, detects its local electrical contact state, and reports hit
candidates to the existing ESP32 scoring apparatus over ESP-NOW.

The scoring apparatus remains authoritative. It combines both sides, applies
the double-hit/lockout decision, and drives the existing score, lights, buzzer,
AutoRef, MQTT, and UART matrix paths.

## Agreed Architecture

### Transmitter responsibilities

- Detect the local weapon contact using a protected analog front end.
- Qualify the contact locally; do not stream raw ADC samples for host-side hit
  timing.
- Timestamp the qualified contact at the transmitter.
- Send a versioned, sequenced hit-candidate packet over ESP-NOW.
- Repeat critical state-change packets without creating duplicate hits.
- Report release, heartbeat, battery, and fault state.
- Continue sensing after its own hit until the host's decision window closes.

### Host responsibilities

- Authenticate or whitelist each transmitter MAC and assigned side.
- Validate packet version, piste ID, side, sequence, and integrity.
- Reject stale and duplicate packets.
- Correct transmitter timestamps using a measured synchronization offset.
- Compare left and right hit timestamps and apply the epee decision window.
- Inject accepted results through the normal FSM light path.
- Preserve the existing UART matrix, MQTT, AutoRef, display, and buzzer paths.

### Input source selection

The apparatus will eventually offer explicit `Wired` and `Wireless` modes.

- Wired mode ignores wireless hit packets.
- Wireless mode ignores wired sensor light events.
- Source changes are allowed only while time is stopped and lights are clear.
- Loss of the wireless link reports a fault; it must not silently switch to
  wired sensing during active fencing.
- The selected mode should persist in NVS.

## Existing FSM Facts

These values are verified from the current scoring-device source and are not
new wireless specifications:

- Epee contact qualification: 6000 us (`EpeeContactTime_us`).
- Epee lockout target: 45 ms (`EPEE_LOCK_TIME`), documented in the FSM as the
  midpoint of a 40-50 ms interval.
- Existing tip threshold model uses approximately 160 ohms.
- Existing guard and piste checks use approximately 250 ohms.
- The wired implementation also detects weapon leakage and applies a small
  double-hit timing correction.

The wireless implementation must be compared against the wired implementation
with identical fixtures before any claim of equivalent behavior.

## Phase One Scope

Phase one is an A-B contact and radio-timing prototype. It proves:

- protected contact detection
- 6 ms local qualification
- local timestamp capture
- ESP-NOW channel discovery and delivery
- sequence and duplicate handling
- clock offset measurement
- two-transmitter double-hit arbitration
- heartbeat, link-loss, and battery reporting

Phase one is not competition-correct. A simple local A-B closure cannot by
itself reject a point pressed against an opponent's grounded guard or piste.

## Proposed Prototype Hardware

Use one set per fencer:

- Seeed Studio XIAO ESP32-C3
- protected 3.7 V LiPo battery, approximately 600-1000 mAh
- compatible epee three-pin socket
- dual comparator such as MCP6562 or TLV3202; final choice is pending bench
  evaluation and availability
- precision, current-limited excitation network
- series input resistors
- low-capacitance ESD protection at the weapon connector
- local decoupling for the comparator and ESP32-C3
- power switch
- optional pairing/configuration button
- external 2.4 GHz antenna positioned away from the fencer's body
- insulated enclosure with connector and battery strain relief

For a production PCB, consider ESP32-C3-MINI-1 only after the electrical method
and antenna placement are validated.

## Detection Method

The preferred phase-one signal path is:

```text
Weapon A/B
    |
Current-limited, protected excitation network
    |
Comparator with controlled hysteresis
    |
ESP32-C3 GPIO edge/level input
    |
6 ms qualification using a monotonic microsecond timer
    |
Timestamped ESP-NOW hit candidate
```

The comparator provides deterministic timing and avoids relying on noisy ADC
polling while the radio transmits. An optional ADC test point may measure the
signal for diagnostics and resistance characterization, but it is not the
time-critical detector in phase one.

No weapon conductor may connect directly to an ESP32-C3 GPIO or ADC input.
Exact resistor, hysteresis, filtering, and clamp values remain TBD until the
maximum expected contact voltage, ESD strategy, comparator choice, and fixture
measurements are established.

## GPIO Budget

Phase-one practical budget:

| Function | GPIO count | Required |
|---|---:|---|
| Comparator hit output | 1 | Yes |
| Analog diagnostic point | 1 | Recommended |
| Battery measurement | 1 | Recommended |
| Pair/configuration button | 1 | Optional |

Expected phase-one use: 3-4 GPIOs.

A later full A/B/C implementation is expected to need approximately 6-8 GPIOs
for controlled excitation, comparator/measurement inputs, battery measurement,
and configuration. ESP-NOW itself uses no external GPIO.

Final XIAO pin assignments are intentionally deferred until the analog circuit
is fixed and boot-strapping, serial, antenna, and ADC constraints are checked.

## Radio and Timing Design

ESP-NOW and Wi-Fi share one 2.4 GHz radio and must operate on the same channel.
The transmitter should scan for the apparatus `Piste_xxx` access point, read its
channel, set that channel, and communicate only with the configured apparatus.

Packet arrival time alone is not sufficient evidence for double-hit timing
because radio latency varies. Each transmitter must timestamp its qualified
contact locally. The host should periodically estimate each transmitter's clock
offset and compare corrected timestamps:

$$
\Delta T = |T_L - T_R|
$$

If the corrected difference is inside the accepted epee interval, both hits are
accepted. Otherwise only the first valid hit is accepted. Clock synchronization
accuracy and worst-case ESP-NOW latency must be measured, not assumed.

## Packet Draft

This is a discussion draft, not a frozen protocol:

```cpp
struct EpeeHitPacket {
  uint32_t magic;
  uint8_t version;
  uint8_t pisteId;
  uint8_t side;
  uint8_t event;
  uint32_t sequence;
  uint32_t contactTimestampUs;
  uint16_t measuredResistance;
  uint16_t batteryMillivolts;
};
```

Before implementation, define explicitly:

- byte order, packing, and exact field widths
- timestamp wrap handling
- event types and state transitions
- retry count and retry timing
- heartbeat interval and timeout
- pairing and side assignment
- ESP-NOW encryption/authentication policy
- clock synchronization exchange
- behavior after transmitter or host reboot

## Safety and Test Boundaries

- Begin with an isolated switch/resistance fixture, not a person or weapon.
- Keep all weapon-facing current limited.
- Verify input voltage remains inside component ratings during normal and fault
  conditions.
- Add connector-side ESD protection before weapon testing.
- Keep the battery mechanically protected and use a protected cell.
- Do not connect the fencer-worn prototype to mains-referenced test equipment.
- Use an isolated or battery-powered measurement setup when attached to a real
  weapon.

## Test Plan and Log

Record actual values, instruments, firmware commit, circuit revision, and raw
observations for every test.

| ID | Test | Acceptance evidence | Status |
|---|---|---|---|
| T01 | Comparator idle/contact levels | Measured margins across supply range | Not started |
| T02 | Resistance sweep | Detection boundary and hysteresis recorded | Not started |
| T03 | Contact qualification | No event below threshold; event at/above 6 ms | Not started |
| T04 | Contact bounce/noise | No false duplicate hit candidates | Not started |
| T05 | Wi-Fi transmit interference | No false comparator events during bursts | Not started |
| T06 | ESP-NOW range/body shadowing | Packet loss and RSSI measured | Not started |
| T07 | Packet retries | One logical event despite duplicate packets | Not started |
| T08 | Two-side timing | Correct ordering and double-hit decisions | Not started |
| T09 | Clock synchronization | Offset/error distribution recorded | Not started |
| T10 | Link loss/recovery | Visible fault; no silent source switch | Not started |
| T11 | Battery endurance | Runtime and low-battery behavior recorded | Not started |
| T12 | Wired comparison | Decisions match wired sensor fixtures | Not started |
| T13 | Guard/piste rejection | Required before competition-correct claim | Deferred |

## Decision Log

### 2026-09-30: Separate transmitter repository

The transmitter is a standalone PlatformIO project at
`E:\piet\wireless-epee-transmitter`, targeting `seeed_xiao_esp32c3`. This keeps
its board configuration and dependencies separate from the scoring apparatus.

### 2026-09-30: Local detection, host adjudication

The transmitter detects and timestamps contact locally. The host remains the
single authority for left/right arbitration, score, lights, and downstream
state. Raw ADC streaming was rejected because radio latency would contaminate
time-critical detection.

### 2026-09-30: Comparator-led phase one

A protected comparator output is preferred for the critical contact edge. ADC
sampling remains optional for diagnostics. Exact analog values require bench
evidence before selection.

### 2026-09-30: Explicit wired/wireless selection

Automatic source fallback during a bout was rejected because one source could
clear or overwrite the other source's hit state.

## Open Questions

1. Which exact comparator provides the required input range, propagation time,
   quiescent current, output type, and local availability?
2. What excitation voltage/current and threshold network best reproduce the
   wired apparatus resistance behavior?
3. How will full A/B/C sensing distinguish target, guard, piste, and leakage?
4. What clock synchronization error is achievable under realistic radio load?
5. What packet retry schedule meets reliability without excessive collisions?
6. How are transmitter side, piste, and host MAC configured and recovered?
7. Which GPIO assignments avoid XIAO ESP32-C3 boot and peripheral conflicts?
8. What enclosure and antenna placement survive fencing motion and body
   shadowing?

## Next Concrete Step

Draw and review the isolated A-B comparator test circuit, including nominal
values, component ratings, expected open/closed voltages, hysteresis, ESD path,
and a resistance-switch fixture. Do not implement hit firmware against an
unreviewed direct-to-GPIO circuit.