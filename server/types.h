#ifndef PUS_QUEUE_PROTOCOL_TYPES_H
#define PUS_QUEUE_PROTOCOL_TYPES_H
#include <netinet/in.h>
#include <openssl/types.h>
#include <pthread.h>

#define AVG_VISIT_TIME 300 // seconds
#define NOTIFY_THRESHOLD 900 // 15 minutes
#define NOTIFY_MAX_RETRIES 3
#define NOTIFY_RETRY_INTERVAL 5 // seconds
#define MAX_CLIENTS 64

typedef enum
{
    CLIENT_UNKNOWN = 0,
    CLIENT_KIOSK,
    CLIENT_APP,
    CLIENT_ADMIN
} client_type_t;

typedef struct
{
    int            fd;
    SSL           *ssl;
    char           ip[INET_ADDRSTRLEN];
    char           session_id[65];
    client_type_t  client_type;
    int            authenticated;
    char           phone_number[10];
    int            notify_ack_received;
} client_conn_t;

typedef struct {
    client_conn_t *clients[MAX_CLIENTS];
    pthread_mutex_t mutex;
} client_registry_t;

void registry_init(client_registry_t *reg);
void registry_add(client_registry_t *reg, client_conn_t *conn);
void registry_remove(client_registry_t *reg, client_conn_t *conn);
client_conn_t *registry_find_by_phone(client_registry_t *reg, const char *phone);

#endif //PUS_QUEUE_PROTOCOL_TYPES_H