#include "handlers.h"

void handle_notify_ack(client_conn_t *conn, const char *msg)
{
    conn->notify_ack_received = 1;
    printf("[%s] NOTIFY_ACK received, phone: %s, session_id: %s\n",
        conn->ip, conn->phone_number, conn->session_id);
}