#include "handlers.h"
#include "../common/client_common.h"

#include <stdio.h>
#include <string.h>

#define HELLO_BUF 512

int do_hello(SSL *ssl, const char *client_type, const char *api_key, char *session_id_out, size_t session_id_outlen)
{
    char mid[33] = {0};
    char ts[32]  = {0};
    gen_message_id(mid, sizeof(mid));
    iso_timestamp(ts, sizeof(ts));

    char hello[HELLO_BUF];

    if (api_key != NULL) {
        /* kiosk z api_key */
        snprintf(hello, sizeof(hello),
            "{"
            "\"type\":\"HELLO\","
            "\"message_id\":\"%s\","
            "\"timestamp\":\"%s\","
            "\"session_id\":\"\","
            "\"version\":\"%s\","
            "\"client_type\":\"%s\","
            "\"api_key\":\"%s\","
            "\"hmac\":\"placeholder\""
            "}",
            mid, ts, PROTOCOL_VER, client_type, api_key);
    } else {
        /* aplikacja bez api_key */
        snprintf(hello, sizeof(hello),
            "{"
            "\"type\":\"HELLO\","
            "\"message_id\":\"%s\","
            "\"timestamp\":\"%s\","
            "\"session_id\":\"\","
            "\"version\":\"%s\","
            "\"client_type\":\"%s\","
            "\"hmac\":\"placeholder\""
            "}",
            mid, ts, PROTOCOL_VER, client_type);
    }

    if (ssl_send(ssl, hello) <= 0) {
        fprintf(stderr, "[hello] Error sending HELLO\n");
        return -1;
    }
    printf("[hello] HELLO sent (client_type=%s, message_id=%s)\n", client_type, mid);

    char resp[CLIENT_BUF_SIZE];
    if (ssl_recv_line(ssl, resp, sizeof(resp)) < 0)
        return -1;

    char type[32] = {0};
    json_get_string(resp, "type", type, sizeof(type));

    if (strcmp(type, "HELLO_ACK") == 0) {
        char sid[65] = {0};
        json_get_string(resp, "session_id", sid, sizeof(sid));
        strncpy(session_id_out, sid, session_id_outlen - 1);
        printf("[hello] HELLO_ACK from server, session_id=%s\n", sid);
        return 0;
    }

    if (strcmp(type, "ERROR") == 0) {
        char code[16] = {0}, emsg[128] = {0};
        json_get_string(resp, "error_code",    code, sizeof(code));
        json_get_string(resp, "error_message", emsg, sizeof(emsg));
        fprintf(stderr, "[hello] ERROR %s: %s\n", code, emsg);
        return -1;
    }

    fprintf(stderr, "[hello] Unexpected answer: %s\n", type);
    return -1;
}

void do_bye(SSL *ssl, const char *session_id)
{
    char mid[33] = {0};
    char ts[32]  = {0};
    gen_message_id(mid, sizeof(mid));
    iso_timestamp(ts, sizeof(ts));

    char bye[256];
    snprintf(bye, sizeof(bye),
        "{"
        "\"type\":\"BYE\","
        "\"message_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"session_id\":\"%s\""
        "}",
        mid, ts, session_id);

    ssl_send(ssl, bye);
    printf("[hello] BYE sent\n");
}
