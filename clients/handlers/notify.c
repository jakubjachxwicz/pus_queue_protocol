#include "handlers.h"
#include "../common/client_common.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    SSL        *ssl;
    const char *session_id;
} notify_listener_ctx_t;

static void send_notify_ack(SSL *ssl, const char *session_id, const char *orig_message_id)
{
    char mid[33] = {0};
    char ts[32]  = {0};
    gen_message_id(mid, sizeof(mid));
    iso_timestamp(ts, sizeof(ts));

    char ack[512];
    snprintf(ack, sizeof(ack),
        "{"
        "\"type\":\"NOTIFY_ACK\","
        "\"message_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"session_id\":\"%s\","
        "\"hmac\":\"placeholder\""
        "}",
        mid, ts, session_id);

    ssl_send(ssl, ack);
}

static void *notify_listener_loop(void *arg)
{
    notify_listener_ctx_t *ctx = arg;
    SSL *ssl = ctx->ssl;
    const char *session_id = ctx->session_id;

    char buf[CLIENT_BUF_SIZE];

    for (;;) {
        int n = ssl_recv_line(ssl, buf, sizeof(buf));
        if (n < 0) {
            break;
        }

        char type[32] = {0};
        json_get_string(buf, "type", type, sizeof(type));

        if (strcmp(type, "NOTIFY") == 0) {
            int position = 0, wait = 0;
            json_get_int(buf, "position", &position);
            json_get_int(buf, "estimated_wait", &wait);

            int wait_min = wait / 60;
            int wait_sec = wait % 60;

            printf("\n");
            printf("============================================\n");
            printf("  NOTIFICATION: Your visit is approaching!\n");
            printf("  Position:       %d\n", position);
            printf("  Estimated wait: %02d min %02d s\n", wait_min, wait_sec);
            printf("============================================\n");
            printf("\n");
            fflush(stdout);

            char orig_mid[64] = {0};
            json_get_string(buf, "message_id", orig_mid, sizeof(orig_mid));
            send_notify_ack(ssl, session_id, orig_mid);

            printf("[notify] NOTIFY_ACK sent\n");
            fflush(stdout);
        } else if (strcmp(type, "BYE") == 0) {
            printf("\n[notify] Server closed the session (BYE)\n");
            fflush(stdout);
            break;
        }
        /* other message types ignored by the listener */
    }

    return NULL;
}

int start_notify_listener(SSL *ssl, const char *session_id)
{
    static notify_listener_ctx_t ctx;
    ctx.ssl = ssl;
    ctx.session_id = session_id;

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int ret = pthread_create(&tid, &attr, notify_listener_loop, &ctx);
    pthread_attr_destroy(&attr);

    if (ret != 0) {
        fprintf(stderr, "[notify] Failed to start listener thread\n");
        return -1;
    }

    printf("[notify] Listener thread started\n");
    return 0;
}