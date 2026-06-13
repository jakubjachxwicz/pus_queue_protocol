#include "handlers.h"
#include "../queue/queue.h"
#include "../types.h"

extern queue_t g_queue;

void handle_subscribe(client_conn_t *conn, const char *msg) {
    // validate if app
    if (conn->client_type != CLIENT_APP) {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":2002,"
            "\"error_message\":\"ERR_UNAUTHORIZED\"}");
        return;
    }

    char phone[PHONE_NUMBER_LENGTH] = {0};

    if (json_get_string(msg, "phone", phone, sizeof(phone)) < 0) {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":1001,"
            "\"error_message\":\"ERR_BAD_FORMAT: missing phone number\"}");
        return;
    }

    queue_entry_t entry;
    if (queue_find_by_phone(&g_queue, phone, &entry) < 0) {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":3003,"
            "\"error_message\":\"ERR_NOT_IN_QUEUE: phone number not in the queue\"}");
        return;
    }

    queue_update_session(&g_queue, phone, conn->session_id);
    if (queue_set_subscribed(&g_queue, phone, 1) < 0) {
        ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":1000,"
            "\"error_message\":\"UNKNOWN: couldn't subscribe\"}");
        return;
    }

    strncpy(conn->phone_number, phone, sizeof(conn->phone_number) - 1);

    // create SUBSCRIBE_ACK response message
    char ts[32];
    iso_timestamp(ts, sizeof(ts));

    char ack[256];
    snprintf(ack, sizeof(ack),
        "{\"type\":\"SUBSCRIBE_ACK\","
        "\"message_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"session_id\":\"%s\"}",
        conn->session_id,   // session if for now
        ts,
        conn->session_id);

    ssl_send(conn->ssl, ack);

    printf("[%s] SUBSCRIBE, phone: %s, session_id: %s\n", conn->ip, phone, conn->session_id);
}