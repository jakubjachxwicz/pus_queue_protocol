#ifndef PUS_CLIENT_COMMON_H
#define PUS_CLIENT_COMMON_H

#include <stddef.h>
#include <openssl/ssl.h>

#define PROTOCOL_VER     "1.0"
#define CLIENT_BUF_SIZE  4096

int json_get_string(const char *json, const char *key, char *out, size_t outlen);
int json_get_int(const char *json, const char *key, int *out);

void gen_message_id(char *out, size_t outlen);
void iso_timestamp(char *out, size_t outlen);

int ssl_send(SSL *ssl, const char *json);
int ssl_recv_line(SSL *ssl, char *out, size_t outlen);

SSL_CTX *create_client_ssl_ctx(const char *ca_file);
SSL *tls_connect(SSL_CTX *ctx, const char *host, int port, int *fd_out);

#endif /* PUS_CLIENT_COMMON_H */
