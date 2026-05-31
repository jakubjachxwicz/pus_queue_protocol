#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT        8443
#define BACKLOG     16
#define CERT_FILE   "server.crt"
#define KEY_FILE    "server.key"

typedef struct 
{
    int      fd;
    SSL     *ssl;
    char     ip[INET_ADDRSTRLEN];
} client_conn_t;

static SSL_CTX *create_ssl_ctx(void)
{
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);

    if (SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx,  KEY_FILE,  SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

/* -------------------------------------------------- */
/*  Client handling thread method                     */
/* -------------------------------------------------- */
static void *handle_client(void *arg)
{
    client_conn_t *conn = arg;

    printf("[%s] Connected, starting client thread\n", conn->ip);

    /* TODO: logic loop */

    SSL_shutdown(conn->ssl);
    SSL_free(conn->ssl);
    close(conn->fd);
    printf("[%s] Connection closed\n", conn->ip);

    free(conn);
    return NULL;
}


int main(void)
{
    SSL_CTX *ctx = create_ssl_ctx();

    // Socket initialization
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) 
    { 
        perror("socket"); 
        exit(EXIT_FAILURE); 
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = 
    {
        .sin_family      = AF_INET,
        .sin_port        = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
    {
        perror("bind"); 
        exit(EXIT_FAILURE);
    }
    if (listen(listen_fd, BACKLOG) < 0) 
    {
        perror("listen"); 
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    for (;;) 
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) 
        { 
            perror("accept"); 
            continue; 
        }

        /// handshake
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_fd);

        if (SSL_accept(ssl) <= 0) 
        {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client_fd);
            continue;
        }

        client_conn_t *conn = malloc(sizeof(*conn));
        conn->fd  = client_fd;
        conn->ssl = ssl;
        inet_ntop(AF_INET, &client_addr.sin_addr, conn->ip, sizeof(conn->ip));

        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&tid, &attr, handle_client, conn);
        pthread_attr_destroy(&attr);
    }

    SSL_CTX_free(ctx);
    close(listen_fd);
    return 0;
}