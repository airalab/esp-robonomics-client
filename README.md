# ESP Robonomics Client

Arduino-compatible client for building, signing and submitting selected
Robonomics parachain extrinsics from ESP devices.

The library is intended for small embedded applications that need to publish
data directly to Robonomics without running a full Substrate client on the
device. It uses JSON-RPC to query chain parameters and submit locally signed
extrinsics.

## Status

This is a lightweight legacy client. It manually encodes the currently
supported runtime calls and signed extensions. It does not download or parse
Substrate metadata. Runtime upgrades can change pallet indices, call arguments or signed
extensions.

## Supported Platforms

The code supports Arduino-compatible ESP32 and ESP8266 projects.

## Features

- Generate an Ed25519 account on the device.
- Restore an account from a raw Ed25519 private seed.
- Read the device SS58 address.
- Sign arbitrary messages with the configured Ed25519 account.
- Query nonce, genesis hash and runtime version over JSON-RPC.
- Submit signed extrinsics with `author_submitExtrinsic`.
- Send `datalog.record(data)`.
- Send `rws.call(owner, datalog.record(data))`.
- Send legacy `rws.set_devices(devices)`.
- Send `cps.create_node` for runtime spec 51 (`sendCpsCreateNode`).
- Optionally use WebSocket transport for
  `author_submitAndWatchExtrinsic`.

## Installation

### PlatformIO

Add the library to `platformio.ini`:

```ini
lib_deps =
    https://github.com/airalab/esp-robonomics-client
```

For local library development, use a symlink dependency:

```ini
lib_deps =
    ESPRobonomicsClient=symlink://../..
```

The library dependencies are declared in `library.json` and installed by
PlatformIO:

- `Arduino_JSON`
- `Crypto`
- `WebSockets`

## Transport

HTTP JSON-RPC is enabled by default:

```text
ROBONOMICS_USE_HTTP
```

The client accepts either a hostname or a full RPC URL:

```cpp
robonomics.setup("polkadot.rpc.robonomics.network");
```

Hostnames are normalized to:

```text
https://<hostname>/rpc/
```

To build with WebSocket JSON-RPC instead, define:

```ini
build_flags =
    -DROBONOMICS_USE_WS
    -UROBONOMICS_USE_HTTP
```

WebSocket mode is needed for `sendRWSDatalogRecordAndWatch(...)`.

## Account Setup

### Generate an Account

```cpp
#include <Robonomics.h>

Robonomics robonomics;

void setup() {
    Serial.begin(115200);

    robonomics.generateAndSetPrivateKey();

    Serial.printf("Address: %s\r\n", robonomics.getSs58Address());
    Serial.printf("Private seed: %s\r\n", robonomics.getPrivateKey());
}
```

Persist the generated private seed in secure device storage if the same
account must survive restarts.

### Restore an Account

```cpp
robonomics.setPrivateKey(
    "2e6371e04b45f16cd5c2d66fc47c8ad7f2881215287c374abfa0e07fd003cb01"
);
```

The current API expects:

- an Ed25519 / Edwards private seed;
- exactly `32` raw bytes represented as `64` hex characters;
- no `0x` prefix.

Do not pass an sr25519 key, mnemonic phrase, JSON backup or expanded private
key.

## Send Datalog

Connect to Wi-Fi, configure the node and call:

```cpp
robonomics.setup("polkadot.rpc.robonomics.network");

const char* result = robonomics.sendDatalogRecord("temperature:21.4");
Serial.println(result);
```

The signing account must have enough XRT to pay the transaction fee.

## Send Datalog Through Legacy RWS

To use an existing legacy RWS subscription:

```cpp
const char* result = robonomics.sendRWSDatalogRecord(
    "temperature:21.4",
    "4G8r18FyoE8UF5mBvacPk5wmVD9R7p5m9ftXBt55ywLgEMn5"
);
```

The second argument is the SS58 address of the subscription owner. The signing
device account must already be listed in `RWS::Devices[owner]`.

## Configure Legacy RWS Devices

`sendRWSSetDevices(...)` submits:

```text
RWS.set_devices(Vec<AccountId32>)
```

Example:

```cpp
std::vector<std::string> devices = {
    robonomics.getSs58Address(),
    "4ExampleSecondDeviceAddress..."
};

const char* result = robonomics.sendRWSSetDevices(devices);
Serial.println(result);
```

Important behavior:

- The transaction is signed by the configured account.
- The call writes `RWS::Devices[signer]`.
- The signer does not need an active RWS subscription to call `set_devices`.
- The signer pays the normal transaction fee.
- The new list replaces the previous list. It does not append devices.
- An empty list clears the stored devices.
- The runtime limit is `32` devices.
- Invalid SS58 addresses and lists longer than `32` devices are rejected
  locally before any RPC request.

This enables a simple onboarding flow:

1. Device generates and persists an Ed25519 account.
2. A small amount of XRT is transferred to that address.
3. Device calls `sendRWSSetDevices(...)` to register itself and any locally
   discovered companion devices.
4. A legacy RWS subscription can later be associated with the signer account.

## Create a CPS Node (spec 51)

`sendCpsCreateNode(...)` submits:

```text
Cps.create_node(parent_id, meta, payload)
```

This encoder matches Robonomics runtime **spec_version 51**:

- pallet index `57` / `0x39`
- call index `0`
- `NodeId` as compact `u64`
- `meta` / `payload` as `Option<BoundedVec<u8>>` (1 KiB / 8 KiB)

Do not send this call to a network below spec 51. On Polkadot Robonomics
spec 51 is live; Kusama remains on 49.

Example, root node with optional meta:

```cpp
const char* result = robonomics.sendCpsCreateNode(
    false,
    0,
    "altruist",
    ""
);
if (robonomics.lastExtrinsicOk() && robonomics.lastCpsNodeIdOk()) {
    Serial.printf("NodeId: %llu\r\n",
        static_cast<unsigned long long>(robonomics.lastCpsNodeId()));
}
```

Child node:

```cpp
uint64_t parent = 0;
const char* result = robonomics.sendCpsCreateNode(true, parent, "", "");
```

Important behavior:

- Empty `meta` / `payload` strings are encoded as `None`.
- The signer pays the normal transaction fee.
- The assigned `NodeId` is a snapshot of `CPS::NextNodeId` taken before submit. Concurrent `create_node` calls can make this value stale; parse `NodeCreated` after `inBlock` when a watch API is available.
- The client refuses the call if `state_getRuntimeVersion.specVersion < 51`.
- This does not replace `sendRWSSetDevices(...)`. RWS device lists and CPS node ids are different layers.

## Submission Result

Submission methods return the RPC result as `const char*`. Additional status
helpers describe the latest `author_submitExtrinsic` attempt:

```cpp
if (!robonomics.lastExtrinsicOk()) {
    Serial.printf(
        "Submission failed: %s\r\n",
        robonomics.lastExtrinsicErrorMessage()
    );
}
```

Available helpers:

```cpp
robonomics.lastExtrinsicOk();
robonomics.lastExtrinsicErrorCode();
robonomics.lastExtrinsicErrorMessage();
robonomics.lastExtrinsicResult();
```

## Known Limitations

- Runtime call indices and signed extensions are manually encoded.
- Signed extras follow Robonomics spec 51 `TxExtension`: empty checks add
  no bytes; `CheckMetadataHash` extra is `Mode`, and the 32-byte runtime
  metadata hash is implicit-only when Mode is Enabled.
- `cps.create_node` is a hand-rolled encoder for spec 51, not the future
  `embed-api` / `embed-codegen` path.
- Only Ed25519 signing is supported.
- `setPrivateKey(const char*)` currently requires exactly `64` hex characters
  without a `0x` prefix and does not yet return a validation error.
- HTTP mode currently uses an insecure TLS client configuration. Do not rely
  on transport TLS alone for security-sensitive update flows.

## Public API Overview

```cpp
void setup(String host);
void generateAndSetPrivateKey();
void setPrivateKey(uint8_t* privateKey);
void setPrivateKey(const char* hexPrivateKey);

const char* getSs58Address() const;
const char* getPrivateKey() const;
void signMessage(const String& message, String& signature);

const char* sendDatalogRecord(const std::string& data);
const char* sendRWSDatalogRecord(
    const std::string& data,
    const char* ownerAddress
);
const char* sendRWSSetDevices(
    const std::vector<std::string>& deviceAddresses
);
const char* sendCpsCreateNode(
    bool hasParent,
    uint64_t parentId,
    const std::string& meta,
    const std::string& payload
);

bool lastCpsNodeIdOk() const;
uint64_t lastCpsNodeId() const;
```

## License

Apache License 2.0. See `LICENSE`.
