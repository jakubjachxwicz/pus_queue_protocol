#ifndef PUS_QUEUE_PROTOCOL_NOTIFY_H
#define PUS_QUEUE_PROTOCOL_NOTIFY_H

#include "types.h"
#include "queue/queue.h"

void notify_thread_start(queue_t *queue, client_registry_t *registry);

#endif //PUS_QUEUE_PROTOCOL_NOTIFY_H