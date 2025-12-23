/* Gemini Browser - User interface handling */
#ifndef PALMINI_UI_H
#define PALMINI_UI_H

#include <stdbool.h>
#include <SDL.h>
#include "render.h"
#include "history.h"
#include "document.h"
#include "url.h"

/* Bookmarks */
#define MAX_BOOKMARKS 100
#define BOOKMARK_TITLE_LEN 128

typedef struct {
    char url[MAX_URL_LENGTH];
    char title[BOOKMARK_TITLE_LEN];
} Bookmark;

/* UI state */
typedef struct {
    /* Screen and rendering */
    SDL_Surface *screen;
    Renderer *renderer;
    int screen_width;
    int screen_height;

    /* Current page */
    Document *document;
    Url current_url;
    bool loading;
    char status_message[256];

    /* Scrolling */
    int scroll_y;
    int max_scroll;
    float scroll_velocity;

    /* Touch tracking */
    bool touch_active;
    int touch_start_x;
    int touch_start_y;
    int touch_last_y;
    Uint32 touch_start_time;
    bool is_dragging;

    /* Address bar */
    bool address_focused;
    char address_input[MAX_URL_LENGTH];
    int address_cursor;

    /* Navigation */
    History history;

    /* Bookmarks */
    Bookmark bookmarks[MAX_BOOKMARKS];
    int bookmark_count;

    /* Application state */
    bool running;
    bool paused;
    bool needs_redraw;
} UI;

/* Initialize the UI */
UI *ui_init(void);

/* Cleanup the UI */
void ui_cleanup(UI *ui);

/* Main event loop */
void ui_run(UI *ui);

/* Navigate to a URL */
void ui_navigate(UI *ui, const char *url_str);

/* Handle SDL event. Returns false if app should quit. */
bool ui_handle_event(UI *ui, SDL_Event *event);

/* Update UI state (scrolling momentum, etc) */
void ui_update(UI *ui, Uint32 dt);

/* Redraw the screen */
void ui_draw(UI *ui);

/* Show the on-screen keyboard */
void ui_show_keyboard(UI *ui, bool show);

/* Focus the address bar */
void ui_focus_address(UI *ui);

/* Bookmark functions */
void ui_load_bookmarks(UI *ui);
void ui_save_bookmarks(UI *ui);
void ui_add_bookmark(UI *ui);
void ui_delete_bookmark(UI *ui, int index);
void ui_show_bookmarks(UI *ui);

#endif /* PALMINI_UI_H */
