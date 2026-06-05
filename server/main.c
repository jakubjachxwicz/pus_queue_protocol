
#include "types.h"
#include "message_handlers/handlers.h"
#include "queue/queue.h"

#define PORT        8443
#define BACKLOG     16
#define CERT_FILE   "server.crt"
#define KEY_FILE    "server.key"
#define MAX_MSG_SIZE 4096

queue_t g_queue;

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

    char    buf[MAX_MSG_SIZE];
    size_t  buf_len = 0;

    // client handling loop
    for (;;) 
    {
        char chunk[1024];
        int n = SSL_read(conn->ssl, chunk, sizeof(chunk) - 1);
        if (n <= 0) break;

        chunk[n] = '\0';

        // buffer overflow (ERR_MSG_TOO_LARGE)
        if (buf_len + n >= MAX_MSG_SIZE) 
        {
            ssl_send(conn->ssl,
                "{\"type\":\"ERROR\",\"error_code\":4002,"
                "\"error_message\":\"ERR_MSG_TOO_LARGE\"}");
            break;
        }

        memcpy(buf + buf_len, chunk, n);
        buf_len += n;
        buf[buf_len] = '\0';

        char *newline;
        while ((newline = memchr(buf, '\n', buf_len)) != NULL) 
        {
            *newline = '\0';
            size_t msg_len = newline - buf;

            char type[32] = {0};
            json_get_string(buf, "type", type, sizeof(type));

            if (strcmp(type, "HELLO") == 0) {
                handle_hello(conn, buf);
            } else if (!conn->authenticated) {
                ssl_send(conn->ssl,
                "{\"type\":\"ERROR\",\"error_code\":2001,"
                "\"error_message\":\"ERR_AUTH_FAILED: send HELLO first\"}");
            } else if (strcmp(type, "REGISTER") == 0) {
                handle_register(conn, buf);
            } else if (strcmp(type, "SUNSCRIBE") == 0) {
                handle_subscribe(conn, buf);
            } else {
                /* TODO: STATUS_REQ, PING, BYE... */
            }

            size_t remaining = buf_len - msg_len - 1;
            memmove(buf, newline + 1, remaining);
            buf_len = remaining;
            buf[buf_len] = '\0';
        }
    }

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

    queue_init(&g_queue);

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
    queue_free(&g_queue);
    return 0;
}