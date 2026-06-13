#include "handlers.h"
#include "../common/client_common.h"

#include <stdio.h>
#include <string.h>

int do_deregister(SSL *ssl, const char *session_id)
{
    char mid[33] = {0};
    char ts[32]  = {0};
    gen_message_id(mid, sizeof(mid));
    iso_timestamp(ts, sizeof(ts));

    char msg[512];
    snprintf(msg, sizeof(msg),
        "{"
        "\"type\":\"DEREGISTER\","
        "\"message_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"session_id\":\"%s\","
        "\"hmac\":\"placeholder\""
        "}",
        mid, ts, session_id);

    if (ssl_send(ssl, msg) <= 0) {
        fprintf(stderr, "[deregister] Error sending DEREGISTER\n");
        return -1;
    }

    char resp[CLIENT_BUF_SIZE];
    if (ssl_recv_line(ssl, resp, sizeof(resp)) < 0)
        return -1;

    char type[32] = {0};
    json_get_string(resp, "type", type, sizeof(type));

    if (strcmp(type, "DEREGISTER_ACK") == 0) {
        char phone[32] = {0};
        json_get_string(resp, "phone", phone, sizeof(phone));
        printf("[deregister] Patient removed from queue: %s\n", phone);
        return 0;
    }

    if (strcmp(type, "ERROR") == 0) {
        char code[16] = {0}, emsg[128] = {0};
        json_get_string(resp, "error_code",    code, sizeof(code));
        json_get_string(resp, "error_message", emsg, sizeof(emsg));
        fprintf(stderr, "[deregister] ERROR %s: %s\n", code, emsg);
        return -1;
    }

    fprintf(stderr, "[deregister] Unexpected answer: %s\n", type);
    return -1;
}