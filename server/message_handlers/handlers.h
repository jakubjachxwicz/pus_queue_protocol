#ifndef PUS_QUEUE_PROTOCOL_HANDLERS_H
#define PUS_QUEUE_PROTOCOL_HANDLERS_H

#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "../types.h"

int json_get_string(const char *json, const char *key, char *out, size_t outlen);
int ssl_send(SSL *ssl, const char *json);
void iso_timestamp(char *out, size_t outlen);
void generate_session_id(char *out, size_t outlen);

void handle_hello(client_conn_t *conn, const char *msg);
void handle_register(client_conn_t *conn, const char *msg);
void handle_subscribe(client_conn_t *conn, const char *msg);
void handle_status(client_conn_t *conn, const char *msg);
void handle_ping(client_conn_t *conn, const char *msg);
void handle_bye(client_conn_t *conn, const char *msg);
void handle_notify_ack(client_conn_t *conn, const char *msg);

#endif //PUS_QUEUE_PROTOCOL_HANDLERS_H