#include "types.h"
#include <string.h>
#include <stddef.h>

void registry_init(client_registry_t *reg)
{
    memset(reg->clients, 0, sizeof(reg->clients));
    pthread_mutex_init(&reg->mutex, NULL);
}

void registry_add(client_registry_t *reg, client_conn_t *conn)
{
    pthread_mutex_lock(&reg->mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (reg->clients[i] == NULL) {
            reg->clients[i] = conn;
            break;
        }
    }
    pthread_mutex_unlock(&reg->mutex);
}

void registry_remove(client_registry_t *reg, client_conn_t *conn)
{
    pthread_mutex_lock(&reg->mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (reg->clients[i] == conn) {
            reg->clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&reg->mutex);
}

client_conn_t *registry_find_by_phone(client_registry_t *reg, const char *phone)
{
    client_conn_t *found = NULL;
    pthread_mutex_lock(&reg->mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (reg->clients[i] != NULL &&
            reg->clients[i]->client_type == CLIENT_APP &&
            reg->clients[i]->phone_number[0] != '\0' &&
            strcmp(reg->clients[i]->phone_number, phone) == 0) {
            found = reg->clients[i];
            break;
            }
    }
    pthread_mutex_unlock(&reg->mutex);
    return found;
}