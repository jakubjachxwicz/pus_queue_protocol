#include "handlers.h"
#include "../queue/queue.h"

extern queue_t g_queue;

void handle_deregister(client_conn_t *conn, const char *msg)
{
    // only kiosk can deregister patients
    if (conn->client_type != CLIENT_KIOSK) {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":2002,"
            "\"error_message\":\"ERR_UNAUTHORIZED\"}");
        return;
    }

    char phone[PHONE_NUMBER_LENGTH] = {0};

    if (json_get_string(msg, "phone", phone, sizeof(phone)) < 0) {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":1001,"
            "\"error_message\":\"ERR_BAD_FORMAT: missing phone\"}");
        return;
    }

    if (queue_remove(&g_queue, phone) < 0) {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":3003,"
            "\"error_message\":\"ERR_NOT_IN_QUEUE\"}");
        return;
    }

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