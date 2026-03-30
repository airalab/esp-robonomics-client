#include "config.h"

#ifdef ROBONOMICS_USE_WS

#include "WebsocketRequests.h"

static bool isExtrinsicTerminalStatus(const String& s) {
    // Substrate author_extrinsicUpdate statuses can be:
    // "ready", "broadcast", "inBlock", "finalized", "invalid", "dropped", "usurped", "retracted"
    // We treat inBlock/finalized as success; others as terminal failure.
    return s == "inBlock" || s == "finalized" || s == "invalid" || s == "dropped" || s == "usurped" || s == "retracted";
}

void WebsocketRequests::setup(String host) {
    wsUtils.setupWebsocket(host);
}

void WebsocketRequests::disconnect() {
    wsUtils.disconnect();
}

JSONVar WebsocketRequests::sendRequest(String message) {
    wsUtils.setOnTextCallback([this](uint8_t *payload) {resultCallback(payload);});
    wsUtils.sendMessage(message);
    while (!got_response) {
        wsUtils.websocketLoop();
    }
    got_response = false;
    return response;
}

void WebsocketRequests::resultCallback(uint8_t *payload) {
    // ESP_LOGI(TAG, "Extrinsic result: %s", (char *)payload);
    response = JSON.parse((char *)payload);
    got_response = true;
}

JSONVar WebsocketRequests::sendRequestAndWatch(String message, uint32_t timeout_ms) {
    JSONVar out;
    String subscription_id = "";
    unsigned long start_ms = millis();

    // Step 1: submitAndWatch, expect first response with subscription id.
    got_response = false;
    wsUtils.setOnTextCallback([this](uint8_t *payload) { resultCallback(payload); });
    wsUtils.sendMessage(message);

    while (!got_response) {
        if (timeout_ms > 0 && (millis() - start_ms) > timeout_ms) {
            out["ok"] = false;
            out["status"] = "timeout";
            return out;
        }
        wsUtils.websocketLoop();
    }

    JSONVar first = response;
    got_response = false;

    if (first.hasOwnProperty("error")) {
        out["ok"] = false;
        out["status"] = "rpc_error";
        out["error"] = first["error"];
        return out;
    }

    if (!first.hasOwnProperty("result")) {
        out["ok"] = false;
        out["status"] = "rpc_error";
        out["raw"] = first;
        return out;
    }

    subscription_id = (const char*)first["result"];
    out["subscription"] = subscription_id.c_str();

    // Step 2: wait for updates with terminal status.
    wsUtils.setOnTextCallback([&](uint8_t *payload) {
        JSONVar msg = JSON.parse((char*)payload);
        // Notifications look like:
        // {"jsonrpc":"2.0","method":"author_extrinsicUpdate","params":{"subscription":"...","result": ... }}
        if (!msg.hasOwnProperty("method") || !msg.hasOwnProperty("params")) return;
        String method = (const char*)msg["method"];
        if (method != "author_extrinsicUpdate") return;
        JSONVar params = msg["params"];
        if (!params.hasOwnProperty("subscription") || !params.hasOwnProperty("result")) return;
        String sub = (const char*)params["subscription"];
        if (sub != subscription_id) return;

        JSONVar r = params["result"];
        // r can be string statuses or objects {inBlock: "..."} / {finalized: "..."}.
        const String r_type = JSON.typeof(r);
        if (r_type == "string") {
            String status = (const char*)r;
            if (!isExtrinsicTerminalStatus(status)) return;
            out["ok"] = (status == "inBlock" || status == "finalized");
            out["status"] = status.c_str();
            got_response = true;
            return;
        }

        // Object statuses
        if (r.hasOwnProperty("inBlock")) {
            out["ok"] = true;
            out["status"] = "inBlock";
            out["hash"] = r["inBlock"];
            got_response = true;
            return;
        }
        if (r.hasOwnProperty("finalized")) {
            out["ok"] = true;
            out["status"] = "finalized";
            out["hash"] = r["finalized"];
            got_response = true;
            return;
        }
        if (r.hasOwnProperty("invalid")) {
            out["ok"] = false;
            out["status"] = "invalid";
            out["details"] = r["invalid"];
            got_response = true;
            return;
        }
        if (r.hasOwnProperty("dropped")) {
            out["ok"] = false;
            out["status"] = "dropped";
            out["details"] = r["dropped"];
            got_response = true;
            return;
        }
    });

    while (!got_response) {
        if (timeout_ms > 0 && (millis() - start_ms) > timeout_ms) {
            out["ok"] = false;
            out["status"] = "timeout";
            return out;
        }
        wsUtils.websocketLoop();
    }

    // If we reached here, got_response was set by callback.
    return out;
}

#endif
