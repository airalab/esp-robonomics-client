#pragma once

#include "JsonUtils.h"
#include "BlockchainUtils.h"
#include "Data.h"

uint32_t getEra();
uint64_t getTip();
bool getGenesisBlockHash(BlockchainUtils *blockchainUtils, std::string *blockHash);

bool getNonce(BlockchainUtils *blockchainUtils, char *ss58Address, uint64_t *payloadNonce);
bool getBlockHash(BlockchainUtils *blockchainUtils, int block_number, std::string *blockHash);
bool extractRuntimeVersions(BlockchainUtils *blockchainUtils, uint32_t *specVersion, uint32_t *transactionVersion);
bool getRuntimeInfo(BlockchainUtils *blockchainUtils, JSONVar *runtimeInfo);
bool getRuntimeInfo(const std::string &parentBlockHash, BlockchainUtils *blockchainUtils, JSONVar *runtimeInfo);
bool getChainHead(BlockchainUtils *blockchainUtils, std::string *chainHead);
bool getParentBlockHash(const std::string &chainHead, BlockchainUtils *blockchainUtils, std::string *parentBlockHash);
bool getCpsNextNodeId(BlockchainUtils *blockchainUtils, uint64_t *nextNodeId);
// RFC-78 merkleized metadata hash used by CheckMetadataHash Mode::Enabled.
// Returns false if the node does not expose it; caller must then use Mode::Disabled.
bool getRuntimeMetadataHash(BlockchainUtils *blockchainUtils, Data *metadataHash);

