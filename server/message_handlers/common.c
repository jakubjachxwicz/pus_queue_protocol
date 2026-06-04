#include "handlers.h"


/* Wyciąga wartość pola "klucz" z surowego JSON-a.
Bardzo uproszczone – działa dla prostych stringów. */
int json_get_string(const char *json, const char *key, char *out, size_t outlen)
{
    /* Szukamy: "key":"wartość" */
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);

    const char *pos = strstr(json, search);
    if (!pos) return -1;

    pos += strlen(search);
    while (*pos == ' ' || *pos == ':') pos++;
    if (*pos != '"') return -1;
    pos++; /* pomiń otwierający cudzysłów */

    size_t i = 0;
    while (*pos && *pos != '"' && i < outlen - 1)
        out[i++] = *pos++;
    out[i] = '\0';
    return 0;
}

/* Wyślij JSON przez SSL, zakończony '\n' */
int ssl_send(SSL *ssl, const char *json)
{
    /* Newline-delimited JSON zgodnie z protokołem */
    size_t len = strlen(json);
    char *buf = malloc(len + 2);
    memcpy(buf, json, len);
    buf[len]     = '\n';
    buf[len + 1] = '\0';

    int ret = SSL_write(ssl, buf, len + 1);
    free(buf);
    return ret;
}

/* Timestamp ISO 8601 */
void iso_timestamp(char *out, size_t outlen)
{
    time_t now = time(NULL);
    struct tm *tm = gmtime(&now);
    strftime(out, outlen, "%Y-%m-%dT%H:%M:%SZ", tm);
}

/* 32 losowe bajty -> 64 znaki hex */
void generate_session_id(char *out, size_t outlen)
{
    unsigned char buf[32];
    RAND_bytes(buf, sizeof(buf));
    for (size_t i = 0; i < 32 && (i * 2 + 2) < outlen; i++)
        snprintf(out + i * 2, 3, "%02x", buf[i]);
    out[64] = '\0';
}