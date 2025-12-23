/* Gemini Browser - Gemtext document parser */
#ifndef PALMINI_DOCUMENT_H
#define PALMINI_DOCUMENT_H

#include <stdbool.h>
#include <stddef.h>

/* Line types in a Gemtext document */
typedef enum {
    LINE_TEXT,
    LINE_LINK,
    LINE_HEADING1,
    LINE_HEADING2,
    LINE_HEADING3,
    LINE_LIST_ITEM,
    LINE_QUOTE,
    LINE_PREFORMATTED
} LineType;

/* A single line in a document */
typedef struct {
    LineType type;
    char *text;             /* Display text */
    char *url;              /* For links only */
    int preformat_block;    /* Which preformat block this belongs to (0 = not preformatted) */
} DocLine;

/* A parsed Gemtext document */
typedef struct {
    DocLine *lines;
    size_t num_lines;
    size_t capacity;
    char *title;            /* First heading, if any */
} Document;

/* Create a new empty document */
Document *document_new(void);

/* Parse Gemtext content into a document */
Document *document_parse(const char *gemtext, size_t len);

/* Free a document */
void document_free(Document *doc);

/* Add a line to a document */
bool document_add_line(Document *doc, LineType type, const char *text, const char *url);

/* Get the number of links in a document */
size_t document_link_count(const Document *doc);

/* Get the URL of a link by index (0-based) */
const char *document_link_url(const Document *doc, size_t index);

#endif /* PALMINI_DOCUMENT_H */
