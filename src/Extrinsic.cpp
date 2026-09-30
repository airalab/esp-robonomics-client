#include "Extrinsic.h"
#include "blake/blake2.h"

// Robonomics spec 51 TxExtension (runtime/robonomics/src/lib.rs):
//
// StorageWeightReclaim<(
//   CheckNonZeroSender,          extra () / implicit ()
//   CheckSpecVersion,            extra () / implicit u32 specVersion
//   CheckTxVersion,              extra () / implicit u32 transactionVersion
//   CheckGenesis,                extra () / implicit genesis hash
//   CheckEra,                    extra Era / implicit block hash
//   CheckNonce,                  extra Compact<Index> / implicit ()
//   CheckWeight,                 extra () / implicit ()
//   ChargeTransactionPayment,    extra Compact<Balance> tip / implicit ()
//   CheckMetadataHash,           extra Mode / implicit Option<[u8;32]>
// )>
//
// Empty extras/implicits add no bytes. StorageWeightReclaim is a transparent
// wrapper: extra is the inner tuple, implicit is the inner implicit.
//
// CheckMetadataHash extra is Mode (0=Disabled, 1=Enabled), NOT Option<H256>.
// The 32-byte runtime metadata hash is implicit-only when Mode::Enabled.
// If Mode::Enabled is set without a hash the runtime rejects the tx
// (UnknownTransaction::CannotLookup).

static inline Data encodeEraBytes(uint32_t era) {
    // Immortal era is a single 0x00 byte. Firmware currently always uses this.
    (void)era;
    return Data{0x00};
}

static inline Data encodeMetadataHashMode(const Data& metadataHash) {
    return Data{metadataHash.size() == 32 ? static_cast<uint8_t>(0x01) : static_cast<uint8_t>(0x00)};
}

static inline Data encodeMetadataHashImplicit(const Data& metadataHash) {
    if (metadataHash.size() != 32) {
        return Data{0x00};
    }
    Data encoded{0x01};
    append(encoded, metadataHash);
    return encoded;
}

static inline Data encodeChargeTransactionPayment(uint64_t tip) {
    return encodeCompact(tip);
}

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
) {
    Data data;
    append(data, call);

    // Extra (same bytes that appear after the signature in the extrinsic).
    append(data, encodeEraBytes(era));
    append(data, encodeCompact(nonce));
    append(data, encodeChargeTransactionPayment(tip));
    append(data, encodeMetadataHashMode(metadataHash));

    // Implicit / additional_signed, in TxExtension order.
    encode32LE(sv, data);
    encode32LE(tv, data);
    append(data, hex2bytes(gen.c_str()));
    append(data, hex2bytes(block.c_str()));
    append(data, encodeMetadataHashImplicit(metadataHash));
    return data;
}

std::vector<uint8_t> doSign(Data data, uint8_t privateKey[32], uint8_t publicKey[32]) {
    uint8_t sig[SIGNATURE_SIZE];
    uint8_t payloadHash[32];
    const uint8_t* payload = data.data();
    size_t payloadSize = data.size();

    // Substrate SignedPayload hashes payloads longer than 256 bytes before signing.
    if (payloadSize > 256) {
        blake2(payloadHash, sizeof(payloadHash), payload, payloadSize, NULL, 0);
        payload = payloadHash;
        payloadSize = sizeof(payloadHash);
    }

#ifndef UNIT_TEST
    Ed25519::sign(sig, privateKey, publicKey, payload, payloadSize);
#else
    CryptoPP::Donna::ed25519_sign(payload, payloadSize, privateKey, publicKey, sig);
#endif
    std::vector<byte> signature (sig,sig + SIGNATURE_SIZE);
    return signature;
}

std::vector<uint8_t> doEncode (
    Data signature,
    Data signerAddress,
    uint32_t era,
    uint64_t nonce,
    uint64_t tip,
    Data call,
    const Data& metadataHash
) {
    Data edata;
    append(edata, Data{extrinsicFormat | signedBit});

    append(edata, signerAddress);
    append(edata, sigTypeEd25519);
    append(edata, signature);

    append(edata, encodeEraBytes(era));
    append(edata, encodeCompact(nonce));
    append(edata, encodeChargeTransactionPayment(tip));
    append(edata, encodeMetadataHashMode(metadataHash));

    append(edata, call);
    encodeLengthPrefix(edata);

    return edata;
}
