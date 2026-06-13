#ifndef PUS_QUEUE_PROTOCOL_QUEUE_H
#define PUS_QUEUE_PROTOCOL_QUEUE_H

#include <time.h>
#include <bits/pthreadtypes.h>

#define QUEUE_MAX_SIZE 100
#define PHONE_NUMBER_LENGTH 10
#define SESSION_ID_LENGTH 65

typedef enum {
    ENTRY_FREE,
    ENTRY_ARRIVED,
    ENTRY_SUBSCRIBED,
    ENTRY_NOTIFIED
} entry_state_t;

typedef struct {
    char phone_number[PHONE_NUMBER_LENGTH];
    char session_id[SESSION_ID_LENGTH];
    entry_state_t state;
    int ticket_number;
    int position; // 1, 2, ...
    int subscribed;
    time_t registered_at;
    time_t last_active;
} queue_entry_t;

typedef struct {
    queue_entry_t entries[QUEUE_MAX_SIZE];
    int count;
    int next_ticket;
    pthread_mutex_t mutex;
} queue_t;

void queue_init(queue_t *queue);
void queue_free(queue_t *queue);

int queue_add(queue_t *queue, const char* phone, const char* session_id, queue_entry_t *out);
int queue_remove(queue_t *queue, const char* phone);
int queue_find_by_phone(queue_t *queue, const char* phone, queue_entry_t *out);
int queue_update_session(queue_t *queue, const char* phone, const char* session_id);
int queue_set_subscribed(queue_t *queue, const char* phone, int value);
int queue_set_state(queue_t *queue, const char* phone, entry_state_t state);

void queue_recalculate(queue_t *queue);
int queue_phone_exists(queue_t *queue, const char* phone);
int queue_remove_first(queue_t *queue, queue_entry_t *out);

#endif //PUS_QUEUE_PROTOCOL_QUEUE_H