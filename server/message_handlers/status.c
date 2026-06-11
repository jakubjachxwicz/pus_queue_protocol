#include "handlers.h"
#include "../types.h"
#include "../queue/queue.h"

extern queue_t g_queue;

void handle_status(client_conn_t *conn, const char *msg) {
    // validate if app
    if (conn->client_type != CLIENT_APP) {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":2002,"
            "\"error_message\":\"ERR_UNAUTHORIZED\"}");
        return;
    }

    char phone[PHONE_NUMBER_LENGTH] = {0};

    if (json_get_string(msg, "phone", phone, sizeof(phone)) < 0 || phone[0] == '\0') {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":1001,"
            "\"error_message\":\"ERR_BAD_FORMAT: missing phone\"}");
        return;
    }

    queue_entry_t entry;
    if (queue_find_by_phone(&g_queue, phone, &entry) < 0) {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":3003,"
            "\"error_message\":\"ERR_NOT_IN_QUEUE\"}");
        return;
    }

    // create STATUS_RESP response message
    char ts[32];
    iso_timestamp(ts, sizeof(ts));

    char resp[512];
    snprintf(resp, sizeof(resp),
        "{\"type\":\"STATUS_RESP\","
        "\"message_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"session_id\":\"%s\","
        "\"phone\":\"%s\","
        "\"ticket_number\":%d,"
        "\"position\":%d,"
        "\"estimated_wait\":%d}",
        conn->session_id,
        ts,
        conn->session_id,
        phone,
        entry.ticket_number,
        entry.position,
        entry.position * AVG_VISIT_TIME);

    ssl_send(conn->ssl, resp);

    printf("[%s] STATUS_REQ phone=%s position=%d estimated time=%ds\n",
           conn->ip, phone, entry.position, entry.position * AVG_VISIT_TIME);
}