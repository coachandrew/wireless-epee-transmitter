# Wireless Epee Mode Integration Plan

## Purpose

Define the work needed to add a selectable Wireless Epee input mode to the
scoring apparatus while keeping its existing FSM behavior and UART matrix
display link intact.

This document is a plan, not an implementation or a competition approval.
Wireless scoring must not be used in competition until electrical validity,
timing, reliability, and applicable rule requirements have been independently
verified.

## User-Facing Selection

Keep weapon and hit-input source as separate concepts:

- **Weapon:** foil, epee, or sabre.
- **Hit source:** wired or wireless.

For the first increment, only these combinations are supported:

| Weapon | Hit source | Availability |
|---|---|---|
| Foil | Wired | Existing behavior |
| Epee | Wired | Existing behavior |
| Sabre | Wired | Existing behavior |
| Epee | Wireless | Prototype mode, initially bench/training only |
| Foil | Wireless | Not implemented |
| Sabre | Wireless | Not implemented |

Label the new choice **Wireless Epee**, not just "Game" or "Wireless," so the
weapon and limitations are clear to the operator.

## Data Flow

```text
Left epee transmitter ---- ESP-NOW ----+
                                      |
Right epee transmitter --- ESP-NOW ----+--> FSM host
                                             |  - validate source/sequence
                                             |  - arbitrate hit timestamps
                                             |  - decide score/lights
                                             +--> existing UART --> matrix display
                                             +--> existing MQTT/OPP2 observers

Wired weapon sensor -----------------------> FSM host in Wired mode only
```

The host FSM remains the only scoring authority. Transmitters report contact
candidates; they do not publish final score decisions. The UART matrix protocol
and its wiring remain unchanged. The FSM continues to transmit the same display
events after making a decision.

## Staged Implementation

### Stage 0: Freeze interfaces and safety assumptions

- Review the A-B comparator bench circuit and confirm the exact comparator
  manufacturer's part number and package before assembling it.
- Agree on the host/transmitter packet format, side assignment, piste identity,
  sequence behavior, retries, heartbeat, timestamp units, and clock-sync
  exchange.
- Decide how the operator selects and exits Wireless Epee mode.
- Do not alter OPP2, Cyrano/EFP1.1, or FPA422 protocol-visible behavior as part
  of the first prototype.

**Exit criteria:** reviewed interface notes and a safe isolated bench fixture.

### Stage 1: Electrical A-B detector bench test

- Assemble one comparator channel using an isolated resistance/switch fixture.
- Measure open/closed voltages, threshold, hysteresis, current, bounce response,
  and comparator output polarity.
- Test with ESP32-C3 radio traffic active and inactive.
- Verify the GPIO never exceeds the board's input ratings.
- Only then write the local contact qualification firmware.

**Exit criteria:** reproducible 6 ms qualified contact detection with no false
events in the documented fixture tests. This remains an A-B-only prototype.

### Stage 2: One-transmitter radio prototype

- Add comparator GPIO capture and a monotonic local timestamp.
- Add a versioned ESP-NOW candidate packet, sequence filtering, limited retries,
  and heartbeat/battery status.
- Discover the apparatus AP channel and use that channel for ESP-NOW.
- Test one transmitter against a host diagnostic receiver without connecting it
  to live score decisions.

**Exit criteria:** measured latency, loss, duplicate, reboot, and channel-change
behavior documented under realistic Wi-Fi/MQTT activity.

### Stage 3: Two-transmitter timing and arbitration

- Assign left/right roles and bind both transmitters to one apparatus/piste.
- Implement and measure clock synchronization; do not use packet arrival time as
  a substitute for contact time.
- Feed candidate events into a test harness for the existing epee lockout and
  double-hit rules.
- Verify timestamp wrap, packet reordering, retransmissions, and simultaneous
  contacts.

**Exit criteria:** demonstrated correct ordering and double-hit decisions across
the measured timing-error and radio-latency envelope.

### Stage 4: Add a disabled-by-default FSM Wireless Epee mode

- Add a distinct hit-source/mode setting; do not overload weapon selection.
- Default to the existing wired source after boot and reset.
- In Wired mode, ignore wireless hit candidates.
- In Wireless Epee mode, ignore wired sensor light events while retaining any
  non-hit functions needed for weapon/configuration monitoring.
- Permit changing source only when the timer is stopped and all hit lights are
  clear.
- Store the choice in NVS only after reviewing reset, migration, and invalid
  value behavior.
- Whitelist the two transmitter MAC addresses and validate packet version,
  piste, side, and sequence before accepting candidates.
- Keep network callbacks bounded: validate/copy to a queue only, then process in
  a normal task. Do not execute scoring or allocate large objects in a Wi-Fi
  callback.
- Route accepted decisions through the FSM's normal event path so UART matrix,
  OPP2/MQTT, AutoRef, and other observers see normal canonical results.
- On missing heartbeat or invalid source, indicate a fault and reject new
  wireless hits. Do not switch to wired sensing during a bout.

**Exit criteria:** existing Wired mode regression tests pass, wireless mode is
visibly selected, and packet/link faults fail closed without changing UART
output framing.

### Stage 5: Validate fencing electrical behavior

- Extend from A-B closure to the required A/B/C sensing and source interaction.
- Test valid target contact, opponent guard, piste, leakage, partial contact,
  cable movement, and connector transients.
- Compare decisions directly against the existing wired epee sensor using the
  same fixtures.
- Review ESD, current limiting, battery protection, enclosure, and antenna
  placement before any weapon-attached field trial.

**Exit criteria:** documented equivalence and safety review. Until then, mode is
bench/training prototype only, not competition-ready.

### Stage 6: Field testing and release decision

- Conduct supervised tests with a separate wired scoring apparatus available.
- Log packet, timing, link, battery, and false-hit data.
- Review applicable equipment rules and obtain appropriate expert approval.
- Decide whether Wireless Epee can be enabled outside an explicitly marked
  experimental build.

## FSM Repository Work Areas

These are likely investigation points in
`esp32scoringdeviceMqtt-main`; confirm current symbols before editing:

- `src/3WeaponSensor.cpp`: wired epee event production and sensor lifecycle.
- `src/FencingStateMachine.cpp`: existing `MultiWeaponSensor` event input and
  the central event/scoring path.
- `src/RepeaterSender.cpp`: existing ESP-NOW initialization and UART2 matrix
  transmission. Additive ESP-NOW receive support must not disrupt broadcast or
  UART sends.
- `src/main.cpp`: observer/task startup order and mode initialization.
- `src/EventDefinitions.h`: event identifiers only after the packet/event
  contract is agreed.
- `src/AppSettings.*` or a dedicated settings surface: operator selection,
  only after UI ownership and NVS behavior are agreed.

The first integration change should be a diagnostic receive-and-log path, not
live scoring. Add a bounded queue/task and prove coexistence with repeater
ESP-NOW, Wi-Fi, MQTT, and UART before enabling event injection.

## Preserved Behavior / Non-Goals

- UART2 remains the FSM-to-matrix transport at 115200 baud on FSM GPIO16 RX and
  GPIO17 TX. The matrix side remains RX GPIO46 and TX GPIO45; the matrix does not
  become the scoring authority.
- Existing wired foil, epee, and sabre paths remain unchanged by default.
- The first wireless release supports only epee; it does not imply wireless
  foil or sabre support.
- No automatic source fallback during a bout.
- No raw ADC streaming as the time-critical hit decision mechanism.
- No changes to protocol-visible MQTT, Cyrano, or FPA422 behavior without
  explicit approval.

## Verification Checklist

- [ ] Existing FSM firmware builds and wired regression checks pass.
- [ ] Existing repeater ESP-NOW send behavior remains functional.
- [ ] Matrix UART frames and refresh behavior are unchanged.
- [ ] Host accepts only configured transmitter identities and current packets.
- [ ] Duplicate/reordered packets do not create extra hits.
- [ ] Two-side timestamp arbitration passes boundary tests.
- [ ] Wireless loss never falls back to wired input during active fencing.
- [ ] MQTT and Wi-Fi remain stable during continuous sensor/radio tests.
- [ ] A/B/C guard and piste rejection is proven before competition claims.