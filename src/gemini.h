/* Gemini Browser - Gemini protocol handler */
#ifndef PALMINI_GEMINI_H
#define PALMINI_GEMINI_H

#include <stdbool.h>
#include <stddef.h>
#include "url.h"

/* Gemini status codes */
typedef enum {
    GM_STATUS_INPUT              = 10,
    GM_STATUS_SENSITIVE_INPUT    = 11,
    GM_STATUS_SUCCESS            = 20,
    GM_STATUS_REDIRECT_TEMP      = 30,
    GM_STATUS_REDIRECT_PERM      = 31,
    GM_STATUS_TEMP_FAILURE       = 40,
    GM_STATUS_SERVER_UNAVAIL     = 41,
    GM_STATUS_CGI_ERROR          = 42,
    GM_STATUS_PROXY_ERROR        = 43,
    GM_STATUS_SLOW_DOWN          = 44,
    GM_STATUS_PERM_FAILURE       = 50,
    GM_STATUS_NOT_FOUND          = 51,
    GM_STATUS_GONE               = 52,
    GM_STATUS_PROXY_REFUSED      = 53,
    GM_STATUS_BAD_REQUEST        = 59,
    GM_STATUS_CERT_REQUIRED      = 60,
    GM_STATUS_CERT_NOT_AUTH      = 61,
    GM_STATUS_CERT_NOT_VALID     = 62,
    /* Internal error codes */
    GM_STATUS_ERROR_CONNECT      = -1,
    GM_STATUS_ERROR_TLS          = -2,
    GM_STATUS_ERROR_SEND         = -3,
    GM_STATUS_ERROR_RECV         = -4,
    GM_STATUS_ERROR_HEADER       = -5,
    GM_STATUS_ERROR_TIMEOUT      = -6,
    GM_STATUS_ERROR_MEMORY       = -7
} GeminiStatus;

/* Response from a Gemini request */
typedef struct {
    GeminiStatus status;
    char meta[1024];        /* MIME type or redirect URL or prompt */
    char *body;             /* Response body (caller must free) */
    size_t body_len;
    char error_msg[256];    /* Human-readable error message */
} GeminiResponse;

/* Initialize the Gemini subsystem (OpenSSL) */
bool gemini_init(void);

/* Cleanup the Gemini subsystem */
void gemini_cleanup(void);

/* Fetch a Gemini URL. Caller must call gemini_response_free() on result. */
GeminiResponse *gemini_fetch(const Url *url);

/* Free a response */
void gemini_response_free(GeminiResponse *resp);

/* Get status category (1=input, 2=success, 3=redirect, etc) */
int gemini_status_category(GeminiStatus status);

/* Get human-readable status description */
const char *gemini_status_string(GeminiStatus status);

#endif /* PALMINI_GEMINI_H */
