#include "handlers.h"


void handle_ping(client_conn_t *conn, const char *msg)
{
    char message_id[64] = {0};
    json_get_string(msg, "message_id", message_id, sizeof(message_id));

    char ts[32];
    iso_timestamp(ts, sizeof(ts));

    char pong[256];
    snprintf(pong, sizeof(pong),
        "{\"type\":\"PONG\","
        "\"message_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"session_id\":\"%s\"}",
        message_id,
        ts,
        conn->session_id);

    ssl_send(conn->ssl, pong);
}