/* Gemini Browser - Gemtext document parser */
#define _GNU_SOURCE
#include "document.h"
#include "unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_CAPACITY 64

Document *document_new(void) {
    Document *doc = calloc(1, sizeof(Document));
    if (!doc) return NULL;

    doc->capacity = INITIAL_CAPACITY;
    doc->lines = calloc(doc->capacity, sizeof(DocLine));
    if (!doc->lines) {
        free(doc);
        return NULL;
    }

    return doc;
}

void document_free(Document *doc) {
    if (!doc) return;

    for (size_t i = 0; i < doc->num_lines; i++) {
        free(doc->lines[i].text);
        free(doc->lines[i].url);
    }
    free(doc->lines);
    free(doc->title);
    free(doc);
}

bool document_add_line(Document *doc, LineType type, const char *text, const char *url) {
    if (!doc) return false;

    /* Grow array if needed */
    if (doc->num_lines >= doc->capacity) {
        size_t new_capacity = doc->capacity * 2;
        DocLine *new_lines = realloc(doc->lines, new_capacity * sizeof(DocLine));
        if (!new_lines) return false;
        doc->lines = new_lines;
        doc->capacity = new_capacity;
    }

    DocLine *line = &doc->lines[doc->num_lines];
    memset(line, 0, sizeof(DocLine));

    line->type = type;
    /* Sanitize text for Unicode 6.0 compatibility */
    line->text = text ? unicode_sanitize(text) : strdup("");
    if (!line->text) line->text = strdup("");  /* Fallback on sanitize failure */
    if (url) {
        line->url = strdup(url);
    }

    doc->num_lines++;
    return true;
}

/* Helper to trim trailing whitespace */
static void trim_trailing(char *str) {
    if (!str) return;
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}

/* Helper to skip leading whitespace */
static const char *skip_whitespace(const char *str) {
    while (*str && isspace((unsigned char)*str)) str++;
    return str;
}

Document *document_parse(const char *gemtext, size_t len) {
    if (!gemtext) return NULL;

    Document *doc = document_new();
    if (!doc) return NULL;

    bool in_preformatted = false;
    int preformat_block = 0;

    const char *p = gemtext;
    const char *end = gemtext + len;

    while (p < end) {
        /* Find end of line */
        const char *line_start = p;
        const char *line_end = p;
        while (line_end < end && *line_end != '\n' && *line_end != '\r') {
            line_end++;
        }

        /* Extract line content */
        size_t line_len = line_end - line_start;
        char *line = malloc(line_len + 1);
        if (!line) break;
        memcpy(line, line_start, line_len);
        line[line_len] = '\0';

        /* Handle preformatted toggle */
        if (strncmp(line, "```", 3) == 0) {
            in_preformatted = !in_preformatted;
            if (in_preformatted) {
                preformat_block++;
            }
            free(line);
            /* Skip to next line */
            p = line_end;
            if (p < end && *p == '\r') p++;
            if (p < end && *p == '\n') p++;
            continue;
        }

        if (in_preformatted) {
            /* Preformatted text - preserve as-is */
            document_add_line(doc, LINE_PREFORMATTED, line, NULL);
            if (doc->num_lines > 0) {
                doc->lines[doc->num_lines - 1].preformat_block = preformat_block;
            }
        }
        else if (strncmp(line, "=>", 2) == 0) {
            /* Link line */
            const char *rest = line + 2;
            rest = skip_whitespace(rest);

            /* Find URL end (first whitespace) */
            const char *url_start = rest;
            const char *url_end = rest;
            while (*url_end && !isspace((unsigned char)*url_end)) {
                url_end++;
            }

            /* Extract URL */
            size_t url_len = url_end - url_start;
            char *url = malloc(url_len + 1);
            if (url) {
                memcpy(url, url_start, url_len);
                url[url_len] = '\0';
            }

            /* Get label (rest of line after URL) */
            const char *label = skip_whitespace(url_end);
            char *label_copy = strdup(label[0] ? label : url ? url : "");
            trim_trailing(label_copy);

            document_add_line(doc, LINE_LINK, label_copy, url);

            free(label_copy);
            free(url);
        }
        else if (strncmp(line, "###", 3) == 0) {
            /* Heading 3 */
            const char *text = skip_whitespace(line + 3);
            char *text_copy = strdup(text);
            trim_trailing(text_copy);
            document_add_line(doc, LINE_HEADING3, text_copy, NULL);
            free(text_copy);
        }
        else if (strncmp(line, "##", 2) == 0) {
            /* Heading 2 */
            const char *text = skip_whitespace(line + 2);
            char *text_copy = strdup(text);
            trim_trailing(text_copy);
            document_add_line(doc, LINE_HEADING2, text_copy, NULL);
            free(text_copy);
        }
        else if (line[0] == '#') {
            /* Heading 1 */
            const char *text = skip_whitespace(line + 1);
            char *text_copy = strdup(text);
            trim_trailing(text_copy);
            document_add_line(doc, LINE_HEADING1, text_copy, NULL);

            /* First heading becomes the title */
            if (!doc->title && text_copy[0]) {
                doc->title = strdup(text_copy);
            }
            free(text_copy);
        }
        else if (strncmp(line, "* ", 2) == 0) {
            /* List item */
            const char *text = line + 2;
            char *text_copy = strdup(text);
            trim_trailing(text_copy);
            document_add_line(doc, LINE_LIST_ITEM, text_copy, NULL);
            free(text_copy);
        }
        else if (line[0] == '>') {
            /* Quote */
            const char *text = line + 1;
            if (*text == ' ') text++;
            char *text_copy = strdup(text);
            trim_trailing(text_copy);
            document_add_line(doc, LINE_QUOTE, text_copy, NULL);
            free(text_copy);
        }
        else {
            /* Regular text */
            char *text_copy = strdup(line);
            trim_trailing(text_copy);
            document_add_line(doc, LINE_TEXT, text_copy, NULL);
            free(text_copy);
        }

        free(line);

        /* Skip to next line */
        p = line_end;
        if (p < end && *p == '\r') p++;
        if (p < end && *p == '\n') p++;
    }

    return doc;
}

size_t document_link_count(const Document *doc) {
    if (!doc) return 0;

    size_t count = 0;
    for (size_t i = 0; i < doc->num_lines; i++) {
        if (doc->lines[i].type == LINE_LINK) {
            count++;
        }
    }
    return count;
}

const char *document_link_url(const Document *doc, size_t index) {
    if (!doc) return NULL;

    size_t link_idx = 0;
    for (size_t i = 0; i < doc->num_lines; i++) {
        if (doc->lines[i].type == LINE_LINK) {
            if (link_idx == index) {
                return doc->lines[i].url;
            }
            link_idx++;
        }
    }
    return NULL;
}
