#include "handlers.h"
#include "../common/client_common.h"

#include <stdio.h>
#include <string.h>

int do_subscribe(SSL *ssl, const char *session_id, const char *phone)
{
    char mid[33] = {0};
    char ts[32]  = {0};
    gen_message_id(mid, sizeof(mid));
    iso_timestamp(ts, sizeof(ts));

    char msg[512];
    snprintf(msg, sizeof(msg),
        "{"
        "\"type\":\"SUBSCRIBE\","
        "\"message_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"session_id\":\"%s\","
        "\"phone\":\"%s\","
        "\"hmac\":\"placeholder\""
        "}",
        mid, ts, session_id, phone);

    if (ssl_send(ssl, msg) <= 0) {
        fprintf(stderr, "[subscribe] Error sending SUBSCRIBE\n");
        return -1;
    }

    char resp[CLIENT_BUF_SIZE];
    if (ssl_recv_line(ssl, resp, sizeof(resp)) < 0)
        return -1;

    char type[32] = {0};
    json_get_string(resp, "type", type, sizeof(type));

    if (strcmp(type, "SUBSCRIBE_ACK") == 0) {
        printf("[subscribe] Subscribed successfully, waiting for notifications...\n");
        return 0;
    }

    if (strcmp(type, "ERROR") == 0) {
        char code[16] = {0}, emsg[128] = {0};
        json_get_string(resp, "error_code",    code, sizeof(code));
        json_get_string(resp, "error_message", emsg, sizeof(emsg));
        fprintf(stderr, "[subscribe] ERROR %s: %s\n", code, emsg);
        return -1;
    }

    fprintf(stderr, "[subscribe] Unexpected response: %s\n", type);
    return -1;
}