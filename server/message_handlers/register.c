#include "../queue/queue.h"
#include "handlers.h"

extern queue_t g_queue;

void handle_register(client_conn_t *conn, const char *msg) {
    // verify if KIOSK
    if (conn->client_type != CLIENT_KIOSK) {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":2002,"
            "\"error_message\":\"ERR_UNAUTHORIZED\"}");
        return;
    }

    char phone[PHONE_NUMBER_LENGTH];

    if (json_get_string(msg, "phone", phone, sizeof(phone)) < 0) {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":1001,"
            "\"error_message\":\"ERR_BAD_FORMAT: missing phone\"}");
        return;
    }

    queue_entry_t entry;
    if (queue_add(&g_queue, phone, conn->session_id, &entry) < 0) {
        if (queue_phone_exists(&g_queue, phone)) {
            ssl_send(conn->ssl,
                "{\"type\":\"ERROR\",\"error_code\":3001,"
                "\"error_message\":\"ERR_PHONE_EXISTS\"}");
        } else {
            ssl_send(conn->ssl,
                "{\"type\":\"ERROR\",\"error_code\":1000,"
                "\"error_message\":\"ERR_UNKNOWN: queue full\"}");
        }
        return;
    }

    // create TICKET response message
    char ts[32];
    iso_timestamp(ts, sizeof(ts));

    char ticket[256];
    snprintf(ticket, sizeof(ticket),
        "{\"type\":\"TICKET\","
        "\"message_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"session_id\":\"%s\","
        "\"ticket_number\":%d,"
        "\"position\":%d,"
        "\"estimated_wait\":%d}",
        conn->session_id,   // session if for now
        ts,
        conn->session_id,
        entry.ticket_number,
        entry.position,
        entry.position * AVG_VISIT_TIME);

    ssl_send(conn->ssl, ticket);

    printf("[%s] REGISTER phone=%s ticket=%d position=%d estimated wait=%ds\n",
           conn->ip, phone, entry.ticket_number,
           entry.position, entry.position * AVG_VISIT_TIME);
}