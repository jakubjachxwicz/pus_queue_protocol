#ifndef PUS_QUEUE_PROTOCOL_TYPES_H
#define PUS_QUEUE_PROTOCOL_TYPES_H
#include <netinet/in.h>
#include <openssl/types.h>

#define AVG_VISIT_TIME 300 // seconds

typedef enum
{
    CLIENT_UNKNOWN = 0,
    CLIENT_KIOSK,
    CLIENT_APP
} client_type_t;

typedef struct
{
    int            fd;
    SSL           *ssl;
    char           ip[INET_ADDRSTRLEN];
    char           session_id[65];
    client_type_t  client_type;
    int            authenticated;
} client_conn_t;

#endif //PUS_QUEUE_PROTOCOL_TYPES_H