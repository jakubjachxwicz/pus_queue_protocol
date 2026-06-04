#include "../types.h"
#include "handlers.h"

// should be kept as secret but whatever
#define KIOSK_API_KEY "somesupersecretdonttellanybody"


void handle_hello(client_conn_t *conn, const char *msg)
{
    char version[16]     = {0};
    char client_type[16] = {0};
    char message_id[64]  = {0};
    char timestamp[32]   = {0};

    /* Walidacja wymaganych pól */
    if (json_get_string(msg, "message_id",   message_id,   sizeof(message_id))   < 0 ||
        json_get_string(msg, "version",      version,      sizeof(version))      < 0 ||
        json_get_string(msg, "client_type",  client_type,  sizeof(client_type))  < 0 ||
        json_get_string(msg, "timestamp",    timestamp,    sizeof(timestamp))    < 0) 
    {

        const char *err = "{\"type\":\"ERROR\",\"error_code\":1001,"
                          "\"error_message\":\"ERR_BAD_FORMAT\"}\n";
        ssl_send(conn->ssl, err);
        return;
    }

    // validate client type
    if (strcmp(client_type, "kiosk") == 0)
        conn->client_type = CLIENT_KIOSK;
    else if (strcmp(client_type, "app") == 0)
        conn->client_type = CLIENT_APP;
    else {
        const char *err = "{\"type\":\"ERROR\",\"error_code\":1001,"
                          "\"error_message\":\"ERR_BAD_FORMAT: unknown client_type\"}\n";
        ssl_send(conn->ssl, err);
        return;
    }

    // authenticate KIOSK client
    if (conn->client_type == CLIENT_KIOSK) {
        char api_key[128] = {0};
        if (json_get_string(msg, "api_key", api_key, sizeof(api_key)) < 0
            || strcmp(api_key, KIOSK_API_KEY) != 0) {
            ssl_send(conn->ssl,
            "{\"type\":\"ERROR\",\"error_code\":2001,"
            "\"error_message\":\"ERR_AUTH_FAILED\"}");

            printf("[%s] HELLO from KIOSK refused – invalid API key\n", conn->ip);
            conn->authenticated = 0;
            return;
        }
    }

    /* Generuj session_id dla tej sesji */
    generate_session_id(conn->session_id, sizeof(conn->session_id));

    /* Zbuduj HELLO_ACK */
    char ts[32];
    iso_timestamp(ts, sizeof(ts));

    char ack[512];
    snprintf(ack, sizeof(ack),
        "{\"type\":\"HELLO_ACK\","
        "\"message_id\":\"%s\","   // TODO
        "\"timestamp\":\"%s\","
        "\"session_id\":\"%s\","
        "\"version\":\"1.0\"}",
        message_id,   // temporary client's message_id
        ts,
        conn->session_id);

    ssl_send(conn->ssl, ack);
    conn->authenticated = 1;

    printf("[%s] HELLO from %s, session_id=%s\n",
           conn->ip, client_type, conn->session_id);
}