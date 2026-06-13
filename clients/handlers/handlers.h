#ifndef PUS_CLIENT_HANDLERS_H
#define PUS_CLIENT_HANDLERS_H

#include <openssl/ssl.h>

int do_hello(SSL *ssl, const char *client_type, const char *api_key, char *session_id_out, size_t session_id_outlen);
void do_bye(SSL *ssl, const char *session_id);
int do_register(SSL *ssl, const char *session_id, const char *phone);
int do_status_req(SSL *ssl, const char *session_id, const char *phone);
int do_subscribe(SSL *ssl, const char *session_id, const char *phone);
int do_deregister(SSL *ssl, const char *session_id);

int start_notify_listener(SSL *ssl, const char *session_id);

#endif /* PUS_CLIENT_HANDLERS_H */
