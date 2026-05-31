openssl genrsa -out ca.key 2048

openssl req -x509 -new -key ca.key -out ca.crt -days 3650 \
  -subj "/CN=PUSQueueProtocolCA"