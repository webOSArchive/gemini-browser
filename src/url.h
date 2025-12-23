/* Gemini Browser - URL parsing and handling */
#ifndef PALMINI_URL_H
#define PALMINI_URL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GEMINI_DEFAULT_PORT 1965
#define MAX_URL_LENGTH 2048

typedef struct {
    char scheme[16];
    char host[256];
    uint16_t port;
    char path[1024];
    char query[512];
    char full[MAX_URL_LENGTH];
} Url;

/* Parse a URL string into components */
bool url_parse(const char *url_str, Url *url);

/* Build a full URL string from components */
void url_build(Url *url);

/* Resolve a relative URL against a base URL */
bool url_resolve(const Url *base, const char *relative, Url *result);

/* Check if URL uses gemini:// scheme */
bool url_is_gemini(const Url *url);

/* Percent-decode a string in place */
void url_decode(char *str);

/* Percent-encode a string (caller must free result) */
char *url_encode(const char *str);

/* Normalize path (remove . and ..) */
void url_normalize_path(char *path);

#endif /* PALMINI_URL_H */
