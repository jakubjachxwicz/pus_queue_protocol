#include "notify.h"
#include "message_handlers/handlers.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

typedef struct {
    queue_t *queue;
    client_registry_t *registry;
} notify_ctx_t;

static void send_notify(client_conn_t *conn, int position, int estimated_wait)
{
    char ts[32];
    iso_timestamp(ts, sizeof(ts));

    char notify[512];
    snprintf(notify, sizeof(notify),
        "{\"type\":\"NOTIFY\","
        "\"message_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"session_id\":\"%s\","
        "\"position\":%d,"
        "\"estimated_wait\":%d}",
        conn->session_id,
        ts,
        conn->session_id,
        position,
        estimated_wait);

    ssl_send(conn->ssl, notify);
}

static void *notify_loop(void *arg)
{
    notify_ctx_t *ctx = arg;
    queue_t *queue = ctx->queue;
    client_registry_t *registry = ctx->registry;

    while (1) {
        sleep(5); // check every 5 seconds

        pthread_mutex_lock(&queue->mutex);

        for (int i = 0; i < QUEUE_MAX_SIZE; i++) {
            queue_entry_t *entry = &queue->entries[i];

            if (entry->state == ENTRY_FREE)
                continue;
            if (!entry->subscribed)
                continue;
            if (entry->state == ENTRY_NOTIFIED)
                continue;

            int estimated_wait = entry->position * AVG_VISIT_TIME;

            if (estimated_wait > NOTIFY_THRESHOLD)
                continue;

            // This entry needs NOTIFY
            char phone[PHONE_NUMBER_LENGTH];
            strncpy(phone, entry->phone_number, PHONE_NUMBER_LENGTH);
            int position = entry->position;

            pthread_mutex_unlock(&queue->mutex);

            // Find the connected client
            client_conn_t *conn = registry_find_by_phone(registry, phone);
            if (!conn) {
                printf("[NOTIFY] No active connection for phone=%s, skipping\n", phone);
                pthread_mutex_lock(&queue->mutex);
                continue;
            }

            // Send NOTIFY with retry
            int ack_received = 0;
            for (int attempt = 0; attempt < NOTIFY_MAX_RETRIES; attempt++) {
                printf("[NOTIFY] Sending to phone=%s (attempt %d/%d)\n",
                       phone, attempt + 1, NOTIFY_MAX_RETRIES);

                send_notify(conn, position, estimated_wait);

                // Wait for ACK
                sleep(NOTIFY_RETRY_INTERVAL);

                if (conn->notify_ack_received) {
                    ack_received = 1;
                    conn->notify_ack_received = 0;
                    break;
                }
            }

            if (ack_received) {
                printf("[NOTIFY] ACK received from phone=%s\n", phone);
            } else {
                printf("[NOTIFY] No ACK from phone=%s after %d attempts\n",
                       phone, NOTIFY_MAX_RETRIES);
            }

            // Mark as notified regardless (don't keep spamming)
            queue_set_state(queue, phone, ENTRY_NOTIFIED);

            pthread_mutex_lock(&queue->mutex);
        }

        pthread_mutex_unlock(&queue->mutex);
    }

    return NULL;
}

void notify_thread_start(queue_t *queue, client_registry_t *registry)
{
    static notify_ctx_t ctx;
    ctx.queue = queue;
    ctx.registry = registry;

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, notify_loop, &ctx);
    pthread_attr_destroy(&attr);

    printf("[NOTIFY] Background thread started\n");
}