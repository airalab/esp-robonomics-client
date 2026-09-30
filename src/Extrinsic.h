#pragma once

#include <vector>
#include <string>
#include "Encoder.h"
#include "Utils.h"
#include "Defines.h"

#include <Ed25519.h>

// metadataHash: empty = CheckMetadataHash Mode::Disabled.
// 32 bytes = Mode::Enabled; the hash is implicit-only, not part of extra.
std::vector<uint8_t> doPayload(
    Data call,
    uint32_t era,
    uint64_t nonce,
    uint64_t tip,
    uint32_t sv,
    uint32_t tv,
    std::string gen,
    std::string block,
    const Data& metadataHash
);
std::vector<uint8_t> doSign(Data data, uint8_t privateKey[32], uint8_t publicKey[32]);
std::vector<uint8_t> doEncode(
    Data signature,
    Data signerAddress,
    uint32_t era,
    uint64_t nonce,
    uint64_t tip,
    Data call,
    const Data& metadataHash
);
