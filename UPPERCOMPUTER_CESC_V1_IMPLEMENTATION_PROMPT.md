# UpperComputer CESC Protocol V1 implementation prompt

Copy the prompt below into the Codex session whose workspace is
`D:\Desk\Folders\UpperComputer`.

---

You are working in the CESC Tool repository `D:\Desk\Folders\UpperComputer`.
The firmware repository contains the normative specification at
`D:\Desk\Folders\CESC_Application\CESC_PROTOCOL_V1.md`.

Read that document completely before changing code. Copy it unchanged into an
appropriate `document` location in this repository. Do not invent a second
frame format or reinterpret field endianness. CESC Protocol V1 is little-endian
and uses `43 45` magic plus the specified CRC16.

## Goal

Replace the prototype VESC-framed CESC path with a coherent CESC Protocol V1
client. Preserve the generic serial terminal, Protocol Editor, FireWater,
JustFloat, network transports, virtual data, and plotting.

CESC native protocol and Protocol Editor protocols are separate modes. Do not
represent native CESC as `.ucproto.json`. Both modes should publish numeric
results through the existing `ChannelDataHub`.

Work through implementation, tests, and build verification. Do not stop after
creating class skeletons.

## Inspect first

Inspect at least:

```text
src/services/ConnectionManager.*
src/services/ReceiveDataPipeline.*
src/services/ChannelDataHub.*
src/services/CescFirmwareUploader.*
src/services/CommunicationCodec.h
src/services/CustomBinaryCodec.*
src/services/ProtocolRepository.*
src/pages/CescToolPage.*
src/pages/CommunicationPage.*
src/pages/communication/*
src/models/ConnectionTypes.h
tests/*
```

The current `CescFirmwareUploader` owns a private VESC-style decoder and
directly consumes raw data. Remove that duplicated protocol ownership.

## Required architecture

Add a focused native CESC area, for example:

```text
src/services/cesc/
├─ CescProtocolTypes.h
├─ CescPacketCodec.h/.cpp
├─ CescTransactionManager.h/.cpp
├─ CescSession.h/.cpp
├─ CescSystemClient.h/.cpp
├─ CescFirmwareClient.h/.cpp
├─ CescSensorClient.h/.cpp
└─ CescTelemetryClient.h/.cpp
```

Equivalent cohesive organization is acceptable. Do not put protocol logic in
page widgets.

### Packet codec

Implement incremental encode/decode exactly per the specification, including:

- every possible split boundary and multiple frames per read;
- garbage before magic and false magic inside corrupt frames;
- Version, MessageType, length, and CRC validation;
- configured maximum PayloadLength without allocation from untrusted length;
- decoder reset on disconnect;
- diagnostic counters.

Never assume one `readyRead` equals one frame.

### Transaction manager

Implement:

- nonzero incrementing `quint16` Sequence allocation;
- matching Sequence + ServiceId + CommandId;
- command-specific timeout and retry;
- no more than eight outstanding normal requests;
- retry with identical Sequence and Payload;
- cancellation on disconnect or SessionId change;
- reporting unmatched/duplicate responses.

Use asynchronous Qt mechanisms. Never block the GUI thread.

### Session

`CescSession` must be the sole native CESC parser for its connection. It feeds
the codec, routes responses/events/streams, performs HELLO after transport open,
and exposes separate transport-open and CESC-ready state. Clear decoder,
transactions, streams, and identity on disconnect.

Opening a COM port is not proof of a CESC connection. CESC UI becomes Ready
only after HELLO succeeds. Generic Communication UI may still treat an open
transport as a raw connection.

## Services

### System

Implement HELLO, GET_DEVICE_INFO, PING, GET_CAPABILITIES, GET_COMM_STATS, and
RESET. Store/display selected protocol version, maximum payload, capability
bits, SessionId, firmware and bootloader versions, UUID, hardware name, and
build identifier. Disable UI actions for absent capabilities.

### Firmware

Refactor/replace `CescFirmwareUploader` so it is workflow/UI over a
`CescFirmwareClient`, with no private framing or receive buffer.

Implement:

```text
BEGIN -> WRITE -> FINISH -> ACTIVATE -> reconnect -> HELLO
```

Requirements:

- CRC-32/ISO-HDLC over the raw `.bin`;
- raw offsets beginning at zero;
- use BEGIN's accepted chunk size;
- follow `nextExpectedOffset` instead of assuming ACK advancement;
- idempotent retry with same Sequence;
- ABORT and GET_STATUS;
- erase/upload/verify/reboot/reconnect UI states;
- STOP_ALL telemetry before BEGIN;
- verify a changed SessionId and new firmware information after reconnect;
- preserve existing USB serial number / VID / PID port rediscovery;
- do not prepend the legacy six-byte bootloader header—the firmware now creates
  it internally after verification.

### Sensor

Implement ENUMERATE, GET_SAMPLE, and GET_STATUS. Decode little-endian float32
and uint64 safely without alignment assumptions. Expose AS5600 raw angle,
degrees, status, and device timestamp.

### Telemetry

Implement paged ENUM_CHANNELS, SUBSCRIBE, UNSUBSCRIBE, STOP_ALL,
GET_STREAM_STATUS, and STREAM_DATA `0x80`.

Build schemas from returned descriptors. Decode values in subscription order,
use device timestamps, and publish numeric values to `ChannelDataHub` with
stable IDs such as `cesc-v1/<stream>/<channel-id>`. Detect frame Sequence and
sampleSequence gaps and expose dropped samples.

Current firmware channels are:

```text
0 shaft_angle_raw      uint16  count
1 shaft_angle          float32 deg
2 shaft_sensor_status  uint8   enum
```

Remain descriptor-driven; do not hardcode only these channels.

## Receive ownership

Prevent independent parsers consuming the same bytes:

- CESC native mode: `CescSession` owns parsing;
- generic mode: `ReceiveDataPipeline` owns parsing;
- monitors observe traffic but never become another parser.

Preserve raw RX/TX monitor and rate counters.

## Protocol Editor

Do not redesign Protocol Editor in this task. Preserve `.ucproto.json` schema
v1, custom fixed-frame codec, send panel, FireWater, JustFloat, and existing
tests. Native CESC is a built-in mode, not an editable workspace.

## UI

Show transport state separately from CESC session state, device identity,
protocol version, firmware workflow, sensor state, telemetry subscription, and
CRC/malformed/timeout/retry/drop counters. Follow current theme conventions and
never block the Qt event loop.

## Required tests

Add deterministic tests for:

1. Both exact vectors in specification section 15.
2. One-byte input and every two-part split.
3. Multiple frames in one block.
4. Garbage and false-magic recovery.
5. Corruption of every covered byte and CRC rejection.
6. Oversized length rejection.
7. Little-endian integers, float32, float64, and uint64.
8. Out-of-order transaction responses.
9. Retry retaining Sequence and Payload.
10. Disconnect and SessionId cancellation.
11. Firmware flow with fake transport and offset acknowledgements.
12. Telemetry descriptor/sample publication into ChannelDataHub.
13. Stream gap accounting.
14. All existing tests.

Tests must not require a physical COM port.

## Completion

Update CMake, build the application, and run the full test suite. Fix failures
introduced by the work. Do not alter the normative specification merely to
make a mismatch pass; report a genuine contradiction.

The task is complete only when native CESC V1, updater migration, sensor query,
telemetry publication, and codec tests build and pass.

---
