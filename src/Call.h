#pragma once 

#include <vector>
#include <string>
#include "Encoder.h"
#include "Utils.h"
#include "address.h"

std::vector<uint8_t> callDatalogRecord (Data head, std::string str);
std::vector<uint8_t> callTransferBalance (Data head, std::string str, uint64_t fee );
std::vector<uint8_t> callLaunch (Data head, std::string robot, std::string param);
std::vector<uint8_t> callRws (Data head, RobonomicsPublicKey owner_key, Data param);
std::vector<uint8_t> callRwsSetDevices(Data head, const std::vector<RobonomicsPublicKey>& devices);

// NodeId SCALE-encodes as compact u64. Meta/payload are Option<BoundedVec<u8>>.
static constexpr uint8_t kCpsPalletIndex = 0x39;
static constexpr uint8_t kCpsCreateNodeCallIndex = 0x00;
static constexpr size_t kCpsMaxMetaSize = 1024;
static constexpr size_t kCpsMaxPayloadSize = 8192;

std::vector<uint8_t> callCpsCreateNode(
    Data head,
    bool has_parent,
    uint64_t parent_id,
    const uint8_t* meta,
    size_t meta_len,
    const uint8_t* payload,
    size_t payload_len
);
