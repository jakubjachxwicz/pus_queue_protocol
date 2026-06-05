#include "queue.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

void queue_init(queue_t *queue) {
    memset(queue, 0, sizeof(*queue));
    queue->next_ticket = 1;
    pthread_mutex_init(&queue->mutex, NULL);
}

void queue_free(queue_t *queue) {
    pthread_mutex_destroy(&queue->mutex);
}

static int compare_by_registered_at(const void *a, const void *b) {
    const queue_entry_t *entry_a = (const queue_entry_t *)a;
    const queue_entry_t *entry_b = (const queue_entry_t *)b;

    if (entry_a->state == ENTRY_FREE && entry_b->state == ENTRY_FREE) return 0;
    if (entry_a->state == ENTRY_FREE) return 1;
    if (entry_b->state == ENTRY_FREE) return -1;

    if (entry_a->registered_at < entry_b->registered_at) return -1;
    if (entry_a->registered_at > entry_b->registered_at) return 1;
    return 0;
}

static void recalculate_locked(queue_t *queue) {
    qsort(queue->entries, QUEUE_MAX_SIZE, sizeof(queue_entry_t), compare_by_registered_at);

    int pos = 1;
    for (int i = 0; i < QUEUE_MAX_SIZE; i++) {
        if (queue->entries[i].state == ENTRY_FREE) break;

        queue->entries[i].position = pos++;
    }
}

void queue_recalculate(queue_t *queue) {
    pthread_mutex_lock(&queue->mutex);
    recalculate_locked(queue);
    pthread_mutex_unlock(&queue->mutex);
}

int queue_add(queue_t *queue, const char* phone, const char* session_id, queue_entry_t *out) {
    pthread_mutex_lock(&queue->mutex);

    if (queue->count >= QUEUE_MAX_SIZE) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    // check if phone already exists
    for (int i = 0; i < QUEUE_MAX_SIZE; i++) {
        if (queue->entries[i].state != ENTRY_FREE && strcmp(queue->entries[i].phone_number, phone) == 0) {
            pthread_mutex_unlock(&queue->mutex);
            return -1;
        }
    }

    // empty slot
    int slot = -1;
    for (int i = 0; i < QUEUE_MAX_SIZE; i++) {
        if (queue->entries[i].state == ENTRY_FREE) {
            slot = i;
            break;
        }
    }

    queue_entry_t *entry = &queue->entries[slot];
    memset(entry, 0, sizeof(*entry));
    strncpy(entry->phone_number, phone, PHONE_NUMBER_LENGTH - 1);
    strncpy(entry->session_id, session_id, SESSION_ID_LENGTH - 1);
    entry->ticket_number = queue->next_ticket++;
    entry->state = ENTRY_ARRIVED;
    entry->registered_at = time(NULL);
    entry->last_active = time(NULL);
    entry->subscribed = 0;
    queue->count++;

    recalculate_locked(queue);
    if (out) *out = *entry;

    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

int queue_remove(queue_t *queue, const char* phone) {
    pthread_mutex_lock(&queue->mutex);

    int found = 0;
    for (int i = 0; i < QUEUE_MAX_SIZE; i++) {
        if (queue->entries[i].state != ENTRY_FREE && strcmp(queue->entries[i].phone_number, phone) == 0) {
            memset(&queue->entries[i], 0, sizeof(queue->entries[i]));
            found = 1;
            queue->count--;
            break;
        }
    }

    if (found) {
        recalculate_locked(queue);
        pthread_mutex_unlock(&queue->mutex);
        return found ? 0 : -1;
    }
}

int queue_find_by_phone(queue_t *queue, const char* phone, queue_entry_t *out) {
    pthread_mutex_lock(&queue->mutex);

    int found = 0;
    for (int i = 0; i < QUEUE_MAX_SIZE; i++) {
        if (queue->entries[i].state == ENTRY_FREE && strcmp(queue->entries[i].phone_number, phone) == 0) {
            if (out) *out = queue->entries[i];
            found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&queue->mutex);
    return found ? 0 : -1;
}

int queue_update_session(queue_t *queue, const char* phone, const char* session_id) {
    pthread_mutex_lock(&queue->mutex);

    int found = 0;
    for (int i = 0; i < QUEUE_MAX_SIZE; i++) {
        if (queue->entries[i].state == ENTRY_FREE && strcmp(queue->entries[i].phone_number, phone) == 0) {
            strncpy(queue->entries[i].session_id, session_id, SESSION_ID_LENGTH - 1);
            queue->entries[i].last_active = time(NULL);
            found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&queue->mutex);
    return found ? 0 : -1;
}

int queue_set_subscribed(queue_t *queue, const char* phone, int value) {
    pthread_mutex_lock(&queue->mutex);
    int found = 0;
    for (int i = 0; i < QUEUE_MAX_SIZE; i++) {
        if (queue->entries[i].state == ENTRY_FREE && strcmp(queue->entries[i].phone_number, phone) == 0) {
            queue->entries[i].subscribed = value;
            found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&queue->mutex);
    return found ? 0 : -1;
}

int queue_set_state(queue_t *queue, const char* phone, entry_state_t state) {
    pthread_mutex_lock(&queue->mutex);

    int found = 0;
    for (int i = 0; i < QUEUE_MAX_SIZE; i++) {
        if (queue->entries[i].state == ENTRY_FREE && strcmp(queue->entries[i].phone_number, phone) == 0) {
            queue->entries[i].state = state;
            found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&queue->mutex);
    return found ? 0 : -1;
}

int queue_phone_exists(queue_t *queue, const char* phone) {
    pthread_mutex_lock(&queue->mutex);

    int found = 0;
    for (int i = 0; i < QUEUE_MAX_SIZE; i++) {
        if (queue->entries[i].state == ENTRY_FREE && strcmp(queue->entries[i].phone_number, phone) == 0) {
            found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&queue->mutex);
    return found;
}