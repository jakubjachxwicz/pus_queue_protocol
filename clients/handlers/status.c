#include "handlers.h"
#include "../common/client_common.h"

#include <stdio.h>
#include <string.h>

int do_status_req(SSL *ssl, const char *session_id, const char *phone)
{
    char mid[33] = {0};
    char ts[32]  = {0};
    gen_message_id(mid, sizeof(mid));
    iso_timestamp(ts, sizeof(ts));

    char msg[512];
    snprintf(msg, sizeof(msg),
        "{"
        "\"type\":\"STATUS_REQ\","
        "\"message_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"session_id\":\"%s\","
        "\"phone\":\"%s\","
        "\"hmac\":\"placeholder\""
        "}",
        mid, ts, session_id, phone);

    if (ssl_send(ssl, msg) <= 0) {
        fprintf(stderr, "[status] Error sending STATUS_REQ\n");
        return -1;
    }

    char resp[CLIENT_BUF_SIZE];
    if (ssl_recv_line(ssl, resp, sizeof(resp)) < 0)
        return -1;

    char type[32] = {0};
    json_get_string(resp, "type", type, sizeof(type));

    if (strcmp(type, "STATUS_RESP") == 0) {
        int ticket = 0, position = 0, wait = 0;
        json_get_int(resp, "ticket_number",  &ticket);
        json_get_int(resp, "position",       &position);
        json_get_int(resp, "estimated_wait", &wait);

        int wait_min = wait / 60;
        int wait_sec = wait % 60;

        printf("|\n");
        printf("|  Ticket number:     %-17d             \n", ticket);
        printf("|  Position:          %-17d             \n", position);
        printf("|  Waiting time:      %02d min %02d s   \n", wait_min, wait_sec);
        printf("|  Phone numebr:      %-17s             \n", phone);
        printf("|\n\n");
        return 0;
    }

    if (strcmp(type, "ERROR") == 0) {
        char code[16] = {0}, emsg[128] = {0};
        json_get_string(resp, "error_code",    code, sizeof(code));
        json_get_string(resp, "error_message", emsg, sizeof(emsg));
        fprintf(stderr, "[status] ERROR %s: %s\n", code, emsg);
        return -1;
    }

    fprintf(stderr, "[status] Unexpected answer: %s\n", type);
    return -1;
}