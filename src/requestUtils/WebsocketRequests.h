#pragma once

#include "config.h"

#ifdef ROBONOMICS_USE_WS

#include "WebsocketUtils.h"
#include <Arduino_JSON.h>
#include <Arduino.h>

class WebsocketRequests {
    private:
        JSONVar response;
        bool got_response;
        WebsocketUtilsRobonomics wsUtils;
        void resultCallback(uint8_t *payload);
    public:
        void setup(String host);
        void disconnect();
        JSONVar sendRequest(String message);
        // Send JSON-RPC request and block until we observe an inclusion/finalization update
        // for `author_submitAndWatchExtrinsic`. Returns a JSON object describing the final state:
        // { "ok": true, "status": "inBlock|finalized", "hash": "<0x...>" }
        // or { "ok": false, "status": "<invalid|dropped|timeout|rpc_error>", ... }.
        JSONVar sendRequestAndWatch(String message, uint32_t timeout_ms);
};

#endif
