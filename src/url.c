/* Gemini Browser - URL parsing and handling */
#include "url.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

bool url_parse(const char *url_str, Url *url) {
    if (!url_str || !url) return false;

    memset(url, 0, sizeof(Url));

    const char *p = url_str;
    const char *scheme_end;
    const char *host_start;
    const char *host_end;
    const char *port_start = NULL;
    const char *path_start;
    const char *query_start = NULL;

    /* Skip leading whitespace */
    while (*p && isspace((unsigned char)*p)) p++;

    /* Find scheme */
    scheme_end = strstr(p, "://");
    if (scheme_end) {
        size_t scheme_len = scheme_end - p;
        if (scheme_len >= sizeof(url->scheme)) scheme_len = sizeof(url->scheme) - 1;
        strncpy(url->scheme, p, scheme_len);
        url->scheme[scheme_len] = '\0';
        host_start = scheme_end + 3;
    }
    else {
        /* Default to gemini if no scheme */
        strcpy(url->scheme, "gemini");
        host_start = p;
    }

    /* Find end of host (could be :port or / or end of string) */
    host_end = host_start;
    while (*host_end && *host_end != ':' && *host_end != '/' && *host_end != '?') {
        host_end++;
    }

    size_t host_len = host_end - host_start;
    if (host_len >= sizeof(url->host)) host_len = sizeof(url->host) - 1;
    strncpy(url->host, host_start, host_len);
    url->host[host_len] = '\0';

    /* Convert host to lowercase */
    for (char *c = url->host; *c; c++) {
        *c = tolower((unsigned char)*c);
    }

    /* Parse port if present */
    if (*host_end == ':') {
        port_start = host_end + 1;
        url->port = (uint16_t)atoi(port_start);
        while (*host_end && *host_end != '/' && *host_end != '?') {
            host_end++;
        }
    }
    else {
        url->port = GEMINI_DEFAULT_PORT;
    }

    /* Parse path */
    path_start = host_end;
    if (*path_start == '/') {
        const char *path_end = path_start;
        while (*path_end && *path_end != '?') {
            path_end++;
        }
        size_t path_len = path_end - path_start;
        if (path_len >= sizeof(url->path)) path_len = sizeof(url->path) - 1;
        strncpy(url->path, path_start, path_len);
        url->path[path_len] = '\0';

        if (*path_end == '?') {
            query_start = path_end + 1;
        }
    }
    else if (*path_start == '?') {
        strcpy(url->path, "/");
        query_start = path_start + 1;
    }
    else {
        strcpy(url->path, "/");
    }

    /* Parse query */
    if (query_start) {
        strncpy(url->query, query_start, sizeof(url->query) - 1);
    }

    /* Normalize path */
    url_normalize_path(url->path);

    /* Build full URL */
    url_build(url);

    return url->host[0] != '\0';
}

void url_build(Url *url) {
    if (!url) return;

    if (url->port == GEMINI_DEFAULT_PORT || url->port == 0) {
        if (url->query[0]) {
            snprintf(url->full, sizeof(url->full), "%s://%s%s?%s",
                     url->scheme, url->host, url->path, url->query);
        }
        else {
            snprintf(url->full, sizeof(url->full), "%s://%s%s",
                     url->scheme, url->host, url->path);
        }
    }
    else {
        if (url->query[0]) {
            snprintf(url->full, sizeof(url->full), "%s://%s:%d%s?%s",
                     url->scheme, url->host, url->port, url->path, url->query);
        }
        else {
            snprintf(url->full, sizeof(url->full), "%s://%s:%d%s",
                     url->scheme, url->host, url->port, url->path);
        }
    }
}

bool url_resolve(const Url *base, const char *relative, Url *result) {
    if (!base || !relative || !result) return false;

    /* Check if relative is actually absolute */
    if (strstr(relative, "://")) {
        return url_parse(relative, result);
    }

    /* Start with base URL */
    memcpy(result, base, sizeof(Url));

    /* Clear query */
    result->query[0] = '\0';

    if (relative[0] == '/') {
        /* Absolute path */
        if (relative[1] == '/') {
            /* Protocol-relative URL (//host/path) - rare in Gemini */
            char full[MAX_URL_LENGTH];
            snprintf(full, sizeof(full), "%s:%s", base->scheme, relative);
            return url_parse(full, result);
        }
        else {
            /* Root-relative path */
            strncpy(result->path, relative, sizeof(result->path) - 1);
        }
    }
    else if (relative[0] == '?') {
        /* Query only */
        strncpy(result->query, relative + 1, sizeof(result->query) - 1);
    }
    else {
        /* Relative path - resolve against base directory */
        char *last_slash = strrchr(result->path, '/');
        if (last_slash) {
            size_t dir_len = last_slash - result->path + 1;
            size_t rel_len = strlen(relative);
            if (dir_len + rel_len < sizeof(result->path)) {
                strcpy(result->path + dir_len, relative);
            }
        }
        else {
            snprintf(result->path, sizeof(result->path), "/%s", relative);
        }
    }

    /* Handle query in relative URL */
    const char *q = strchr(relative, '?');
    if (q) {
        /* Truncate path at query */
        char *path_q = strchr(result->path, '?');
        if (path_q) *path_q = '\0';
        strncpy(result->query, q + 1, sizeof(result->query) - 1);
    }

    url_normalize_path(result->path);
    url_build(result);

    return true;
}

bool url_is_gemini(const Url *url) {
    return url && strcmp(url->scheme, "gemini") == 0;
}

void url_decode(char *str) {
    if (!str) return;

    char *src = str;
    char *dst = str;

    while (*src) {
        if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            int val;
            char hex[3] = { src[1], src[2], '\0' };
            val = (int)strtol(hex, NULL, 16);
            *dst++ = (char)val;
            src += 3;
        }
        else if (*src == '+') {
            *dst++ = ' ';
            src++;
        }
        else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

char *url_encode(const char *str) {
    if (!str) return NULL;

    /* Worst case: every char needs encoding (3x length) */
    size_t len = strlen(str);
    char *result = malloc(len * 3 + 1);
    if (!result) return NULL;

    char *dst = result;
    for (const char *src = str; *src; src++) {
        unsigned char c = (unsigned char)*src;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            *dst++ = c;
        }
        else {
            sprintf(dst, "%%%02X", c);
            dst += 3;
        }
    }
    *dst = '\0';

    return result;
}

void url_normalize_path(char *path) {
    if (!path || !*path) return;

    /* Check if original path ends with slash (directory indicator) */
    size_t orig_len = strlen(path);
    bool had_trailing_slash = (orig_len > 1 && path[orig_len - 1] == '/');

    char *src = path;
    char *dst = path;
    char *segments[256];
    int num_segments = 0;

    /* Split into segments */
    while (*src) {
        /* Skip leading slashes */
        while (*src == '/') src++;
        if (!*src) break;

        char *seg_start = src;
        while (*src && *src != '/') src++;

        size_t seg_len = src - seg_start;

        if (seg_len == 1 && seg_start[0] == '.') {
            /* Current directory - skip */
            continue;
        }
        else if (seg_len == 2 && seg_start[0] == '.' && seg_start[1] == '.') {
            /* Parent directory - pop */
            if (num_segments > 0) {
                num_segments--;
            }
        }
        else {
            /* Normal segment */
            if (num_segments < 256) {
                segments[num_segments++] = seg_start;
                /* Null-terminate for copying later */
                if (*src) {
                    *src = '\0';
                    src++;
                }
            }
        }
    }

    /* Rebuild path */
    *dst++ = '/';
    for (int i = 0; i < num_segments; i++) {
        size_t slen = strlen(segments[i]);
        memcpy(dst, segments[i], slen);
        dst += slen;
        if (i < num_segments - 1) {
            *dst++ = '/';
        }
    }

    /* Restore trailing slash if original had one */
    if (had_trailing_slash && num_segments > 0) {
        *dst++ = '/';
    }
    *dst = '\0';
}
