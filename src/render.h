/* Gemini Browser - SDL rendering */
#ifndef PALMINI_RENDER_H
#define PALMINI_RENDER_H

#include <stdbool.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include "document.h"

/* Color scheme - dark theme */
#define COLOR_BG_R       0x1e
#define COLOR_BG_G       0x1e
#define COLOR_BG_B       0x23

#define COLOR_TEXT_R     0xdc
#define COLOR_TEXT_G     0xdc
#define COLOR_TEXT_B     0xdc

#define COLOR_LINK_R     0x64
#define COLOR_LINK_G     0xc8
#define COLOR_LINK_B     0xff

#define COLOR_HEADING_R  0xff
#define COLOR_HEADING_G  0xcc
#define COLOR_HEADING_B  0x00

#define COLOR_QUOTE_R    0x88
#define COLOR_QUOTE_G    0x88
#define COLOR_QUOTE_B    0x88

#define COLOR_PRE_R      0xaa
#define COLOR_PRE_G      0xdd
#define COLOR_PRE_B      0xaa

/* Layout constants */
#define MARGIN_LEFT      20
#define MARGIN_RIGHT     20
#define MARGIN_TOP       50      /* Space for address bar */
#define LINE_SPACING     4

/* Rendered line with layout info (for hit testing) */
typedef struct {
    SDL_Rect bounds;        /* Screen position and size */
    int doc_line_index;     /* Index into Document->lines */
    bool is_link;
} RenderedLine;

/* Renderer state */
typedef struct {
    SDL_Surface *screen;
    TTF_Font *font_regular;
    TTF_Font *font_mono;
    TTF_Font *font_h1;
    TTF_Font *font_h2;
    TTF_Font *font_h3;
    int line_height;
    int mono_line_height;

    /* Rendered lines for hit testing */
    RenderedLine *rendered_lines;
    size_t num_rendered;
    size_t rendered_capacity;

    /* Total content height (for scrolling) */
    int content_height;

    /* Button icons (NULL if not loaded, falls back to text) */
    SDL_Surface *icon_back;
    SDL_Surface *icon_bookmark_add;
    SDL_Surface *icon_bookmarks;

    /* Button highlight state */
    int highlight_button;       /* 0=none, 1=back, 2=add, 3=list */
    Uint32 highlight_time;      /* SDL_GetTicks() when highlight started */
} Renderer;

/* Initialize the renderer */
Renderer *render_init(SDL_Surface *screen);

/* Cleanup the renderer */
void render_cleanup(Renderer *r);

/* Clear the screen */
void render_clear(Renderer *r);

/* Render a document at the given scroll offset */
void render_document(Renderer *r, const Document *doc, int scroll_y);

/* Render the address bar */
void render_address_bar(Renderer *r, const char *url, bool loading, bool focused, bool can_go_back);

/* Address bar button hit test - returns: 0=none, 1=back, 2=add bookmark, 3=show bookmarks */
int render_address_bar_hit_test(Renderer *r, int x, int y);

/* Trigger button highlight feedback */
void render_button_highlight(Renderer *r, int button);

/* Render a loading indicator */
void render_loading(Renderer *r, const char *message);

/* Render an error message */
void render_error(Renderer *r, const char *title, const char *message);

/* Hit test: find link at screen position. Returns doc line index or -1 */
int render_hit_test(Renderer *r, int x, int y);

/* Flip the screen buffer */
void render_flip(Renderer *r);

#endif /* PALMINI_RENDER_H */
