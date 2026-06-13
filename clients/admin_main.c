#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openssl/ssl.h>

#include "common/client_common.h"
#include "handlers/handlers.h"

#define DEFAULT_HOST   "127.0.0.1"
#define DEFAULT_PORT   8443
#define CA_FILE        "ca.crt"
#define ADMIN_API_KEY  "somesupersecretdonttellanybody"

static void admin_loop(SSL *ssl, const char *session_id)
{
    printf("\nAdmin panel ready.\n");
    printf("Commands:\n");
    printf("  n - deregister first patient (next)\n");
    printf("  q - quit\n\n");

    char line[32];

    for (;;) {
        printf("admin> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[--len] = '\0';

        if (len == 0)
            continue;

        if (strcmp(line, "q") == 0 || strcmp(line, "Q") == 0) {
            printf("[admin] Closing...\n");
            break;
        }

        if (strcmp(line, "n") == 0 || strcmp(line, "N") == 0) {
            if (do_deregister(ssl, session_id) < 0)
                printf("[admin] Deregister failed.\n\n");
            continue;
        }

        printf("[admin] Unknown command: %s\n", line);
    }
}

int main(int argc, char *argv[])
{
    const char *host = DEFAULT_HOST;
    int         port = DEFAULT_PORT;

    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = atoi(argv[2]);

    printf("Admin client\n");
    printf("Connecting to %s:%d...\n", host, port);

    SSL_CTX *ctx = create_client_ssl_ctx(CA_FILE);
    if (!ctx) return EXIT_FAILURE;

    int fd = -1;
    SSL *ssl = tls_connect(ctx, host, port, &fd);
    if (!ssl) {
        SSL_CTX_free(ctx);
        return EXIT_FAILURE;
    }
    printf("[admin] Connected.\n");

    char session_id[65] = {0};
    if (do_hello(ssl, "admin", ADMIN_API_KEY, session_id, sizeof(session_id)) < 0) {
        fprintf(stderr, "[admin] Protocol handshake failed\n");
        SSL_shutdown(ssl); SSL_free(ssl);
        close(fd); SSL_CTX_free(ctx);
        return EXIT_FAILURE;
    }

    admin_loop(ssl, session_id);

    do_bye(ssl, session_id);

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(fd);
    SSL_CTX_free(ctx);

    printf("[admin] Disconnected.\n");
    return EXIT_SUCCESS;
}