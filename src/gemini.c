/* Gemini Browser - Gemini protocol handler */
#define _GNU_SOURCE
#include "gemini.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

/* SNI support - may not be defined as a macro in older headers */
#ifndef SSL_set_tlsext_host_name
#include <openssl/tls1.h>
#define SSL_set_tlsext_host_name(s, name) \
    SSL_ctrl(s, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, (char *)name)
#endif

#define RECV_BUFFER_SIZE 16384
#define CONNECT_TIMEOUT_SEC 10
#define RECV_TIMEOUT_SEC 30
#define MAX_RESPONSE_SIZE (10 * 1024 * 1024)  /* 10 MB max */

static SSL_CTX *ssl_ctx = NULL;

bool gemini_init(void) {
    /* Initialize OpenSSL */
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    /* Create SSL context - use SSLv23 method which negotiates highest available */
    ssl_ctx = SSL_CTX_new(SSLv23_client_method());
    if (!ssl_ctx) {
        fprintf(stderr, "Failed to create SSL context\n");
        return false;
    }

    /* Disable SSLv2 and SSLv3 - require TLS 1.0+ */
    /* Note: The 0.9.8 headers don't have SSL_OP_NO_TLSv1_1, but the runtime
     * (OpenSSL 1.0.2p) will negotiate up to TLS 1.2 automatically */
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

    /* TOFU model - don't verify certificates */
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, NULL);

    return true;
}

void gemini_cleanup(void) {
    if (ssl_ctx) {
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
    }
    EVP_cleanup();
    ERR_free_strings();
}

static int connect_with_timeout(const char *host, uint16_t port, int timeout_sec) {
    struct addrinfo hints, *res, *rp;
    int sock = -1;
    char port_str[8];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(port_str, sizeof(port_str), "%d", port);

    int err = getaddrinfo(host, port_str, &hints, &res);
    if (err != 0) {
        return -1;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) continue;

        /* Set non-blocking for timeout */
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        int ret = connect(sock, rp->ai_addr, rp->ai_addrlen);
        if (ret < 0 && errno != EINPROGRESS) {
            close(sock);
            sock = -1;
            continue;
        }

        if (ret < 0) {
            /* Wait for connection with timeout */
            fd_set wfds;
            struct timeval tv;
            FD_ZERO(&wfds);
            FD_SET(sock, &wfds);
            tv.tv_sec = timeout_sec;
            tv.tv_usec = 0;

            ret = select(sock + 1, NULL, &wfds, NULL, &tv);
            if (ret <= 0) {
                close(sock);
                sock = -1;
                continue;
            }

            /* Check if connect succeeded */
            int error;
            socklen_t len = sizeof(error);
            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error) {
                close(sock);
                sock = -1;
                continue;
            }
        }

        /* Restore blocking mode */
        fcntl(sock, F_SETFL, flags);
        break;
    }

    freeaddrinfo(res);
    return sock;
}

GeminiResponse *gemini_fetch(const Url *url) {
    GeminiResponse *resp = calloc(1, sizeof(GeminiResponse));
    if (!resp) return NULL;

    if (!ssl_ctx) {
        resp->status = GM_STATUS_ERROR_TLS;
        strncpy(resp->error_msg, "SSL not initialized", sizeof(resp->error_msg) - 1);
        return resp;
    }

    if (!url_is_gemini(url)) {
        resp->status = GM_STATUS_ERROR_CONNECT;
        snprintf(resp->error_msg, sizeof(resp->error_msg),
                 "Unsupported protocol: %s", url->scheme);
        return resp;
    }

    /* Connect to server */
    int sock = connect_with_timeout(url->host, url->port, CONNECT_TIMEOUT_SEC);
    if (sock < 0) {
        resp->status = GM_STATUS_ERROR_CONNECT;
        snprintf(resp->error_msg, sizeof(resp->error_msg),
                 "Could not connect to %s:%d", url->host, url->port);
        return resp;
    }

    /* Create SSL connection */
    SSL *ssl = SSL_new(ssl_ctx);
    if (!ssl) {
        close(sock);
        resp->status = GM_STATUS_ERROR_TLS;
        strncpy(resp->error_msg, "Failed to create SSL object", sizeof(resp->error_msg) - 1);
        return resp;
    }

    SSL_set_fd(ssl, sock);
    SSL_set_tlsext_host_name(ssl, url->host);

    if (SSL_connect(ssl) <= 0) {
        unsigned long err = ERR_get_error();
        char err_buf[256];
        ERR_error_string_n(err, err_buf, sizeof(err_buf));
        SSL_free(ssl);
        close(sock);
        resp->status = GM_STATUS_ERROR_TLS;
        snprintf(resp->error_msg, sizeof(resp->error_msg),
                 "TLS handshake failed: %s", err_buf);
        return resp;
    }

    /* Send request: URL + CRLF */
    char request[MAX_URL_LENGTH + 4];
    snprintf(request, sizeof(request), "%s\r\n", url->full);

    int sent = SSL_write(ssl, request, strlen(request));
    if (sent <= 0) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sock);
        resp->status = GM_STATUS_ERROR_SEND;
        strncpy(resp->error_msg, "Failed to send request", sizeof(resp->error_msg) - 1);
        return resp;
    }

    /* Receive response */
    char buffer[RECV_BUFFER_SIZE];
    size_t total_size = 0;
    size_t capacity = RECV_BUFFER_SIZE;
    char *data = malloc(capacity);
    if (!data) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sock);
        resp->status = GM_STATUS_ERROR_MEMORY;
        strncpy(resp->error_msg, "Out of memory", sizeof(resp->error_msg) - 1);
        return resp;
    }

    /* Set socket timeout for receive */
    struct timeval tv;
    tv.tv_sec = RECV_TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (1) {
        int received = SSL_read(ssl, buffer, sizeof(buffer));
        if (received <= 0) {
            int err = SSL_get_error(ssl, received);
            if (err == SSL_ERROR_ZERO_RETURN) {
                break;  /* Clean shutdown */
            }
            else if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                continue;  /* Retry */
            }
            else {
                break;  /* Error or EOF */
            }
        }

        /* Grow buffer if needed */
        if (total_size + received > capacity) {
            size_t new_capacity = capacity * 2;
            if (new_capacity > MAX_RESPONSE_SIZE) {
                new_capacity = MAX_RESPONSE_SIZE;
            }
            if (total_size + received > new_capacity) {
                /* Response too large */
                break;
            }
            char *new_data = realloc(data, new_capacity);
            if (!new_data) {
                break;
            }
            data = new_data;
            capacity = new_capacity;
        }

        memcpy(data + total_size, buffer, received);
        total_size += received;

        if (total_size >= MAX_RESPONSE_SIZE) {
            break;
        }
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock);

    /* Parse response header */
    if (total_size < 3) {
        free(data);
        resp->status = GM_STATUS_ERROR_HEADER;
        strncpy(resp->error_msg, "Response too short", sizeof(resp->error_msg) - 1);
        return resp;
    }

    /* Find end of header line (CRLF) */
    char *header_end = NULL;
    for (size_t i = 0; i < total_size - 1; i++) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            header_end = data + i;
            break;
        }
    }

    if (!header_end) {
        free(data);
        resp->status = GM_STATUS_ERROR_HEADER;
        strncpy(resp->error_msg, "Malformed response header", sizeof(resp->error_msg) - 1);
        return resp;
    }

    /* Parse status code (first two digits) */
    if (header_end - data < 2 || !isdigit((unsigned char)data[0]) || !isdigit((unsigned char)data[1])) {
        free(data);
        resp->status = GM_STATUS_ERROR_HEADER;
        strncpy(resp->error_msg, "Invalid status code", sizeof(resp->error_msg) - 1);
        return resp;
    }

    resp->status = (data[0] - '0') * 10 + (data[1] - '0');

    /* Parse meta (everything after status and space until CRLF) */
    const char *meta_start = data + 2;
    if (*meta_start == ' ') meta_start++;
    size_t meta_len = header_end - meta_start;
    if (meta_len >= sizeof(resp->meta)) meta_len = sizeof(resp->meta) - 1;
    strncpy(resp->meta, meta_start, meta_len);
    resp->meta[meta_len] = '\0';

    /* Body starts after CRLF */
    char *body_start = header_end + 2;
    size_t body_len = total_size - (body_start - data);

    if (body_len > 0) {
        resp->body = malloc(body_len + 1);
        if (resp->body) {
            memcpy(resp->body, body_start, body_len);
            resp->body[body_len] = '\0';
            resp->body_len = body_len;
        }
    }

    free(data);
    return resp;
}

void gemini_response_free(GeminiResponse *resp) {
    if (resp) {
        free(resp->body);
        free(resp);
    }
}

int gemini_status_category(GeminiStatus status) {
    if (status < 0) return -1;
    if (status < 10) return status;
    return status / 10;
}

const char *gemini_status_string(GeminiStatus status) {
    switch (status) {
        case GM_STATUS_INPUT:           return "Input required";
        case GM_STATUS_SENSITIVE_INPUT: return "Sensitive input required";
        case GM_STATUS_SUCCESS:         return "Success";
        case GM_STATUS_REDIRECT_TEMP:   return "Temporary redirect";
        case GM_STATUS_REDIRECT_PERM:   return "Permanent redirect";
        case GM_STATUS_TEMP_FAILURE:    return "Temporary failure";
        case GM_STATUS_SERVER_UNAVAIL:  return "Server unavailable";
        case GM_STATUS_CGI_ERROR:       return "CGI error";
        case GM_STATUS_PROXY_ERROR:     return "Proxy error";
        case GM_STATUS_SLOW_DOWN:       return "Slow down";
        case GM_STATUS_PERM_FAILURE:    return "Permanent failure";
        case GM_STATUS_NOT_FOUND:       return "Not found";
        case GM_STATUS_GONE:            return "Gone";
        case GM_STATUS_PROXY_REFUSED:   return "Proxy request refused";
        case GM_STATUS_BAD_REQUEST:     return "Bad request";
        case GM_STATUS_CERT_REQUIRED:   return "Client certificate required";
        case GM_STATUS_CERT_NOT_AUTH:   return "Certificate not authorized";
        case GM_STATUS_CERT_NOT_VALID:  return "Certificate not valid";
        case GM_STATUS_ERROR_CONNECT:   return "Connection failed";
        case GM_STATUS_ERROR_TLS:       return "TLS error";
        case GM_STATUS_ERROR_SEND:      return "Send failed";
        case GM_STATUS_ERROR_RECV:      return "Receive failed";
        case GM_STATUS_ERROR_HEADER:    return "Invalid response header";
        case GM_STATUS_ERROR_TIMEOUT:   return "Request timed out";
        case GM_STATUS_ERROR_MEMORY:    return "Out of memory";
        default:                        return "Unknown status";
    }
}
