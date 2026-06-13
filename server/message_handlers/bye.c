#include "handlers.h"


void handle_bye(client_conn_t *conn, const char *msg)
{
    char ts[32];
    iso_timestamp(ts, sizeof(ts));

    char bye[256];
    snprintf(bye, sizeof(bye),
        "{\"type\":\"BYE\","
        "\"message_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"session_id\":\"%s\"}",
        conn->session_id,
        ts,
        conn->session_id);

    ssl_send(conn->ssl, bye);

    printf("[%s] BYE received, closing session %s\n", conn->ip, conn->session_id);
}