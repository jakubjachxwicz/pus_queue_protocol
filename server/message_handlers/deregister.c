#include "handlers.h"
#include "../queue/queue.h"

extern queue_t g_queue;

void handle_deregister(client_conn_t *conn, const char *msg)
{
    if (conn->client_type != CLIENT_ADMIN) {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":2002,"
            "\"error_message\":\"ERR_UNAUTHORIZED\"}");
        return;
    }

    char phone[PHONE_NUMBER_LENGTH] = {0};

    queue_entry_t entry;

    if (queue_remove_first(&g_queue, &entry) < 0) {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":3003,"
            "\"error_message\":\"ERR_NOT_IN_QUEUE: queue is empty\"}");
        return;
    }
    strncpy(phone, entry.phone_number, PHONE_NUMBER_LENGTH - 1);

    char ts[32];
    iso_timestamp(ts, sizeof(ts));

    char ack[256];
    snprintf(ack, sizeof(ack),
        "{\"type\":\"DEREGISTER_ACK\","
        "\"message_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"session_id\":\"%s\","
        "\"phone\":\"%s\"}",
        conn->session_id,
        ts,
        conn->session_id,
        phone);

    ssl_send(conn->ssl, ack);

    printf("[%s] DEREGISTER phone=%s\n", conn->ip, phone);
}