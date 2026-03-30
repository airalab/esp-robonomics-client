#pragma once

// Transport selection:
// - Define ROBONOMICS_USE_HTTP for HTTP JSON-RPC (stateless, no subscriptions)
// - Define ROBONOMICS_USE_WS   for WebSocket JSON-RPC (supports subscriptions / watch)
//
// By default we keep HTTP enabled to preserve existing behavior.
// Projects that want `author_submitAndWatchExtrinsic` should enable ROBONOMICS_USE_WS
// (and disable HTTP) at build time.
#if !defined(ROBONOMICS_USE_HTTP) && !defined(ROBONOMICS_USE_WS)
#define ROBONOMICS_USE_HTTP
#endif
