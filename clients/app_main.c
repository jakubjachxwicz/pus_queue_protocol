#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openssl/ssl.h>

#include "common/client_common.h"
#include "handlers/handlers.h"

#define DEFAULT_HOST  "127.0.0.1"
#define DEFAULT_PORT  8443
#define CA_FILE       "ca.crt"

static void app_loop(SSL *ssl, const char *session_id)
{
    printf("\nApp ready. Enter phone number or 'q' to exit.\n\n");

    char phone[32];

    for (;;) {
        printf("Enter phone number: ");
        fflush(stdout);

        if (!fgets(phone, sizeof(phone), stdin)) {
            printf("\n");
            break;
        }

        size_t plen = strlen(phone);
        if (plen > 0 && phone[plen - 1] == '\n')
            phone[--plen] = '\0';

        if (plen == 0)
            continue;

        if (strcmp(phone, "q") == 0 || strcmp(phone, "Q") == 0) {
            printf("[app] Closing...\n");
            break;
        }

        if (plen < 4 || plen > 20) {
            fprintf(stderr, "[app] Incorrect phone number (length %zu)\n", plen);
            continue;
        }

        if (do_status_req(ssl, session_id, phone) < 0) {
            printf("[app] Status request failed. Please try again.\n\n");
            continue;
        }

        printf("Subscribe to notifications? (y/n): ");
        fflush(stdout);

        char answer[8];
        if (!fgets(answer, sizeof(answer), stdin)) {
            printf("\n");
            break;
        }

        if (answer[0] == 'y' || answer[0] == 'Y') {
            if (do_subscribe(ssl, session_id, phone) < 0)
                printf("[app] Subscription failed.\n\n");
        }
    }
}

int main(int argc, char *argv[])
{
    const char *host = DEFAULT_HOST;
    int         port = DEFAULT_PORT;

    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = atoi(argv[2]);

    printf("app client\n");
    printf("Connecting to %s:%d...\n", host, port);

    SSL_CTX *ctx = create_client_ssl_ctx(CA_FILE);
    if (!ctx) return EXIT_FAILURE;

    int fd = -1;
    SSL *ssl = tls_connect(ctx, host, port, &fd);
    if (!ssl) {
        SSL_CTX_free(ctx);
        return EXIT_FAILURE;
    }
    printf("[app] Connected.\n");

    char session_id[65] = {0};
    if (do_hello(ssl, "app", NULL, session_id, sizeof(session_id)) < 0) {
        fprintf(stderr, "[app] Protocol handshake failed\n");
        SSL_shutdown(ssl); SSL_free(ssl);
        close(fd); SSL_CTX_free(ctx);
        return EXIT_FAILURE;
    }

    app_loop(ssl, session_id);

    do_bye(ssl, session_id);

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(fd);
    SSL_CTX_free(ctx);

    printf("[app] Disconnected.\n");
    return EXIT_SUCCESS;
}