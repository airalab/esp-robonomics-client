#include "Call.h"

std::vector<uint8_t> callDatalogRecord (Data head, std::string str) {
    Data call;
    append(call, head);
    append(call, encodeCompact(str.length()));
    std::vector<uint8_t> rec(str.begin(), str.end());
    append(call, rec); 
    return call;
}

std::vector<uint8_t> callTransferBalance (Data head, std::string str, uint64_t fee ) {
    Data call;
    append(call, head); 
    append(call, 0); 
    std::vector<uint8_t> dst = hex2bytes (str.c_str()); // derived SS58KEY from SS58DST 
    append(call, dst); 
    append(call, encodeCompact(fee)); // value
    return call;
}

std::vector<uint8_t> callLaunch (Data head, std::string robot, std::string param) {
    Data call;
    append(call, head); 
    std::vector<uint8_t> dst = hex2bytes (robot.c_str()); // derived SS58KEY from SS58DST 
    append(call, dst);
    std::vector<uint8_t>  h256 = hex2bytes (param.c_str()); 
    append(call, h256); // add param as H256 data
    return call;
}

std::vector<uint8_t> callRws (Data head, RobonomicsPublicKey owner_key, Data param) {
    Data call;
    append(call, head); 
    std::vector<uint8_t> dst(owner_key.bytes, owner_key.bytes + PUBLIC_KEY_LENGTH);
    // std::vector<uint8_t> dst = hex2bytes (owner.c_str());
    // RWS pallet expects `subscription_id: T::AccountId` (AccountId32), i.e. raw 32 bytes.
    append(call, dst);
    append(call, param); // add nested call
    return call;
}

std::vector<uint8_t> callRwsSetDevices(Data head, const std::vector<RobonomicsPublicKey>& devices) {
    constexpr size_t maxDevicesAmount = 32;
    if (devices.size() > maxDevicesAmount) {
        return {};
    }

    Data call;
    append(call, head);
    append(call, encodeCompact(devices.size()));
    for (const auto& device : devices) {
        std::vector<uint8_t> accountId(device.bytes, device.bytes + PUBLIC_KEY_LENGTH);
        append(call, accountId);
    }
    return call;
}

static void appendOptionCompactU64(Data& call, bool has_value, uint64_t value) {
    if (!has_value) {
        append(call, static_cast<uint8_t>(0x00));
        return;
    }
    append(call, static_cast<uint8_t>(0x01));
    append(call, encodeCompact(value));
}

static bool appendOptionBytes(Data& call, const uint8_t* bytes, size_t len, size_t max_len) {
    if (bytes == nullptr || len == 0) {
        append(call, static_cast<uint8_t>(0x00));
        return true;
    }
    if (len > max_len) {
        return false;
    }
    append(call, static_cast<uint8_t>(0x01));
    append(call, encodeCompact(len));
    call.insert(call.end(), bytes, bytes + len);
    return true;
}

std::vector<uint8_t> callCpsCreateNode(
    Data head,
    bool has_parent,
    uint64_t parent_id,
    const uint8_t* meta,
    size_t meta_len,
    const uint8_t* payload,
    size_t payload_len
) {
    Data call;
    append(call, head);
    appendOptionCompactU64(call, has_parent, parent_id);
    if (!appendOptionBytes(call, meta, meta_len, kCpsMaxMetaSize)) {
        return {};
    }
    if (!appendOptionBytes(call, payload, payload_len, kCpsMaxPayloadSize)) {
        return {};
    }
    return call;
}
