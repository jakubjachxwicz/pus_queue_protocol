#include "client_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

/* ------------------------------------------------------------------ */
/*  JSON                                                              */
/* ------------------------------------------------------------------ */

int json_get_string(const char *json, const char *key, char *out, size_t outlen)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);

    const char *pos = strstr(json, search);
    if (!pos) return -1;

    pos += strlen(search);
    while (*pos == ' ' || *pos == ':') pos++;
    if (*pos != '"') return -1;
    pos++;

    size_t i = 0;
    while (*pos && *pos != '"' && i < outlen - 1)
        out[i++] = *pos++;
    out[i] = '\0';
    return 0;
}

int json_get_int(const char *json, const char *key, int *out)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);

    const char *pos = strstr(json, search);
    if (!pos) return -1;

    pos += strlen(search);
    while (*pos == ' ' || *pos == ':') pos++;
    if (*pos < '0' || *pos > '9') return -1;

    *out = atoi(pos);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Generatory                                                        */
/* ------------------------------------------------------------------ */

void gen_message_id(char *out, size_t outlen)
{
    unsigned char raw[16];
    RAND_bytes(raw, sizeof(raw));
    for (size_t i = 0; i < 16 && i * 2 + 2 < outlen; i++)
        snprintf(out + i * 2, 3, "%02x", raw[i]);
    out[32] = '\0';
}

void iso_timestamp(char *out, size_t outlen)
{
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    strftime(out, outlen, "%Y-%m-%dT%H:%M:%SZ", t);
}

/* ------------------------------------------------------------------ */
/*  SSL I/O                                                           */
/* ------------------------------------------------------------------ */

int ssl_send(SSL *ssl, const char *json)
{
    size_t len = strlen(json);
    char *buf = malloc(len + 2);
    if (!buf) return -1;
    memcpy(buf, json, len);
    buf[len]     = '\n';
    buf[len + 1] = '\0';
    int ret = SSL_write(ssl, buf, (int)(len + 1));
    free(buf);
    return ret;
}

int ssl_recv_line(SSL *ssl, char *out, size_t outlen)
{
    static char ibuf[CLIENT_BUF_SIZE];
    static size_t ibuf_len = 0;

    for (;;) {
        char *nl = memchr(ibuf, '\n', ibuf_len);
        if (nl) {
            size_t msg_len = (size_t)(nl - ibuf);
            if (msg_len >= outlen) msg_len = outlen - 1;
            memcpy(out, ibuf, msg_len);
            out[msg_len] = '\0';
            size_t remaining = ibuf_len - (size_t)(nl - ibuf) - 1;
            memmove(ibuf, nl + 1, remaining);
            ibuf_len = remaining;
            return (int)msg_len;
        }

        if (ibuf_len >= CLIENT_BUF_SIZE - 1) {
            fprintf(stderr, "[client] Message too large, clearing buffer\n");
            ibuf_len = 0;
            return -1;
        }

        int n = SSL_read(ssl, ibuf + ibuf_len, (int)(CLIENT_BUF_SIZE - ibuf_len - 1));
        if (n <= 0) {
            int err = SSL_get_error(ssl, n);
            if (err == SSL_ERROR_ZERO_RETURN)
                fprintf(stderr, "[client] The server closed the connection\n");
            else
                fprintf(stderr, "[client] SSL_read ERROR: %d\n", err);
            return -1;
        }
        ibuf_len += (size_t)n;
        ibuf[ibuf_len] = '\0';
    }
}

/* ------------------------------------------------------------------ */
/*  TLS                                                               */
/* ------------------------------------------------------------------ */

SSL_CTX *create_client_ssl_ctx(const char *ca_file)
{
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) { ERR_print_errors_fp(stderr); return NULL; }

    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);

    if (SSL_CTX_load_verify_locations(ctx, ca_file, NULL) <= 0) {
        fprintf(stderr, "[client] Unable to load CA: %s\n", ca_file);
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    SSL_CTX_set_verify_depth(ctx, 4);
    return ctx;
}

SSL *tls_connect(SSL_CTX *ctx, const char *host, int port, int *fd_out)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("[client] socket"); return NULL; }

    struct sockaddr_in srv = {
        .sin_family = AF_INET,
        .sin_port   = htons((uint16_t)port),
    };
    if (inet_pton(AF_INET, host, &srv.sin_addr) <= 0) {
        fprintf(stderr, "[client] Incorrect address: %s\n", host);
        close(fd);
        return NULL;
    }
    if (connect(fd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
        perror("[client] connect");
        close(fd);
        return NULL;
    }

    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, fd);
    SSL_set_tlsext_host_name(ssl, host);

    if (SSL_connect(ssl) <= 0) {
        fprintf(stderr, "[client] TLS handshake failed\n");
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(fd);
        return NULL;
    }

    *fd_out = fd;
    return ssl;
}
