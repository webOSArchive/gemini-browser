/* Gemini Browser - User interface handling */
#define _GNU_SOURCE
#include "ui.h"
#include "gemini.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>

#ifdef WEBOS
#include "PDL.h"
#endif

/* Debug logging */
#define LOG_PATH "/media/internal/gemini-log.txt"

static void log_msg(const char *fmt, ...) {
    FILE *f = fopen(LOG_PATH, "a");
    if (f) {
        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        fprintf(f, "\n");
        va_end(args);
        fclose(f);
    }
}

#define SCROLL_FRICTION     0.95f
#define SCROLL_MIN_VELOCITY 0.5f
#define TAP_THRESHOLD       10      /* Max movement for tap vs drag */
#define TAP_TIME_THRESHOLD  300     /* Max ms for tap */
#define MOMENTUM_SCALE      0.3f

/* Default start page */
#define DEFAULT_URL "gemini://geminiprotocol.net/"

UI *ui_init(void) {
    log_msg("=== Gemini Browser starting ===");

    UI *ui = calloc(1, sizeof(UI));
    if (!ui) {
        log_msg("ERROR: calloc failed");
        return NULL;
    }
    log_msg("UI struct allocated");

#ifdef WEBOS
    /* Initialize PDL first - must be before SDL_Init on webOS */
    log_msg("Calling PDL_Init...");
    PDL_Err pdlErr = PDL_Init(0);
    log_msg("PDL_Init returned %d", pdlErr);
#endif

    /* Initialize SDL */
    log_msg("Calling SDL_Init...");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        log_msg("ERROR: SDL_Init failed: %s", SDL_GetError());
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        free(ui);
        return NULL;
    }
    log_msg("SDL_Init succeeded");

#ifdef WEBOS

    /* Change to app directory so we can find bundled fonts */
    {
        char appPath[256];
        log_msg("Getting calling path...");
        if (PDL_GetCallingPath(appPath, sizeof(appPath)) == PDL_NOERROR) {
            log_msg("App path: %s", appPath);
            chdir(appPath);
        } else {
            log_msg("PDL_GetCallingPath failed");
        }
    }

    /* Get screen size */
    PDL_ScreenMetrics metrics;
    if (PDL_GetScreenMetrics(&metrics) == PDL_NOERROR) {
        ui->screen_width = metrics.horizontalPixels;
        ui->screen_height = metrics.verticalPixels;
        log_msg("Screen size: %dx%d", ui->screen_width, ui->screen_height);
    }
    else {
        ui->screen_width = 1024;
        ui->screen_height = 768;
        log_msg("Using default screen size");
    }
#else
    ui->screen_width = 1024;
    ui->screen_height = 768;
#endif

    /* Set up video mode */
    log_msg("Calling SDL_SetVideoMode...");
    ui->screen = SDL_SetVideoMode(0, 0, 0, SDL_SWSURFACE);
    if (!ui->screen) {
        log_msg("ERROR: SDL_SetVideoMode failed: %s", SDL_GetError());
        fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
        ui_cleanup(ui);
        return NULL;
    }
    log_msg("SDL_SetVideoMode succeeded: %dx%d", ui->screen->w, ui->screen->h);

    ui->screen_width = ui->screen->w;
    ui->screen_height = ui->screen->h;

    /* Enable Unicode input */
    SDL_EnableUNICODE(1);
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    log_msg("SDL input initialized");

    /* Initialize renderer */
    log_msg("Initializing renderer...");
    ui->renderer = render_init(ui->screen);
    if (!ui->renderer) {
        log_msg("ERROR: Failed to initialize renderer");
        fprintf(stderr, "Failed to initialize renderer\n");
        ui_cleanup(ui);
        return NULL;
    }
    log_msg("Renderer initialized");

    /* Initialize history */
    history_init(&ui->history);

    /* Load bookmarks */
    ui_load_bookmarks(ui);

    ui->running = true;
    ui->needs_redraw = true;

    log_msg("UI init complete");
    return ui;
}

void ui_cleanup(UI *ui) {
    if (!ui) return;

    if (ui->document) {
        document_free(ui->document);
    }

    if (ui->renderer) {
        render_cleanup(ui->renderer);
    }

#ifdef WEBOS
    PDL_Quit();
#endif

    SDL_Quit();
    free(ui);
}

void ui_show_keyboard(UI *ui, bool show) {
    (void)ui;
#ifdef WEBOS
    PDL_SetKeyboardState(show ? PDL_TRUE : PDL_FALSE);
#else
    (void)show;
#endif
}

void ui_focus_address(UI *ui) {
    if (!ui) return;

    ui->address_focused = true;
    strncpy(ui->address_input, ui->current_url.full, sizeof(ui->address_input) - 1);
    ui->address_cursor = strlen(ui->address_input);
    ui_show_keyboard(ui, true);
    ui->needs_redraw = true;
}

static void ui_unfocus_address(UI *ui) {
    if (!ui) return;

    ui->address_focused = false;
    ui_show_keyboard(ui, false);
    ui->needs_redraw = true;
}

/* Bookmark file path */
#define BOOKMARKS_FILE "/media/internal/gemini-bookmarks.txt"

void ui_load_bookmarks(UI *ui) {
    if (!ui) return;

    ui->bookmark_count = 0;

    FILE *f = fopen(BOOKMARKS_FILE, "r");
    if (!f) return;

    char line[MAX_URL_LENGTH + BOOKMARK_TITLE_LEN + 2];
    while (ui->bookmark_count < MAX_BOOKMARKS && fgets(line, sizeof(line), f)) {
        /* Remove trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';

        /* Parse URL|Title format */
        char *sep = strchr(line, '|');
        if (sep) {
            *sep = '\0';
            strncpy(ui->bookmarks[ui->bookmark_count].url, line, MAX_URL_LENGTH - 1);
            strncpy(ui->bookmarks[ui->bookmark_count].title, sep + 1, BOOKMARK_TITLE_LEN - 1);
        } else {
            strncpy(ui->bookmarks[ui->bookmark_count].url, line, MAX_URL_LENGTH - 1);
            strncpy(ui->bookmarks[ui->bookmark_count].title, line, BOOKMARK_TITLE_LEN - 1);
        }
        ui->bookmark_count++;
    }

    fclose(f);
    log_msg("Loaded %d bookmarks", ui->bookmark_count);
}

void ui_save_bookmarks(UI *ui) {
    if (!ui) return;

    FILE *f = fopen(BOOKMARKS_FILE, "w");
    if (!f) {
        log_msg("Failed to save bookmarks");
        return;
    }

    for (int i = 0; i < ui->bookmark_count; i++) {
        fprintf(f, "%s|%s\n", ui->bookmarks[i].url, ui->bookmarks[i].title);
    }

    fclose(f);
    log_msg("Saved %d bookmarks", ui->bookmark_count);
}

void ui_add_bookmark(UI *ui) {
    if (!ui || ui->bookmark_count >= MAX_BOOKMARKS) return;
    if (!ui->current_url.full[0]) return;

    /* Check if already bookmarked */
    for (int i = 0; i < ui->bookmark_count; i++) {
        if (strcmp(ui->bookmarks[i].url, ui->current_url.full) == 0) {
            snprintf(ui->status_message, sizeof(ui->status_message), "Already bookmarked");
            ui->needs_redraw = true;
            return;
        }
    }

    /* Add bookmark */
    strncpy(ui->bookmarks[ui->bookmark_count].url, ui->current_url.full, MAX_URL_LENGTH - 1);
    if (ui->document && ui->document->title) {
        strncpy(ui->bookmarks[ui->bookmark_count].title, ui->document->title, BOOKMARK_TITLE_LEN - 1);
    } else {
        strncpy(ui->bookmarks[ui->bookmark_count].title, ui->current_url.full, BOOKMARK_TITLE_LEN - 1);
    }
    ui->bookmark_count++;

    ui_save_bookmarks(ui);
    snprintf(ui->status_message, sizeof(ui->status_message), "Bookmarked!");
    ui->needs_redraw = true;
}

void ui_delete_bookmark(UI *ui, int index) {
    if (!ui || index < 0 || index >= ui->bookmark_count) return;

    /* Shift remaining bookmarks */
    for (int i = index; i < ui->bookmark_count - 1; i++) {
        ui->bookmarks[i] = ui->bookmarks[i + 1];
    }
    ui->bookmark_count--;

    ui_save_bookmarks(ui);
}

void ui_show_bookmarks(UI *ui) {
    if (!ui) return;

    /* Save return URL (only if not already on bookmarks page) */
    static char return_url[MAX_URL_LENGTH] = {0};
    if (strncmp(ui->current_url.full, "gemini://bookmarks", 18) != 0) {
        strncpy(return_url, ui->current_url.full, MAX_URL_LENGTH - 1);
    }

    /* Create a document with bookmark links */
    Document *doc = document_new();
    if (!doc) return;

    document_add_line(doc, LINE_HEADING1, "Bookmarks", NULL);
    document_add_line(doc, LINE_TEXT, "", NULL);

    if (ui->bookmark_count == 0) {
        document_add_line(doc, LINE_TEXT, "No bookmarks yet. Use the + button to add pages.", NULL);
    } else {
        for (int i = 0; i < ui->bookmark_count; i++) {
            /* Create link text with delete option */
            char link_text[BOOKMARK_TITLE_LEN + 20];
            snprintf(link_text, sizeof(link_text), "%s", ui->bookmarks[i].title);
            document_add_line(doc, LINE_LINK, link_text, ui->bookmarks[i].url);

            /* Add delete link */
            char delete_url[32];
            snprintf(delete_url, sizeof(delete_url), "gemini://bookmarks/delete/%d", i);
            document_add_line(doc, LINE_LINK, "  [delete]", delete_url);
        }
    }

    document_add_line(doc, LINE_TEXT, "", NULL);
    document_add_line(doc, LINE_LINK, "Back to browsing", return_url[0] ? return_url : DEFAULT_URL);

    if (ui->document) {
        document_free(ui->document);
    }
    ui->document = doc;
    ui->scroll_y = 0;

    /* Set special bookmarks URL */
    url_parse("gemini://bookmarks/", &ui->current_url);
    ui->needs_redraw = true;
}

static void ui_go_back(UI *ui) {
    if (!ui || !history_can_back(&ui->history)) return;

    Url url;
    int scroll;
    if (history_back(&ui->history, &url, &scroll)) {
        ui->loading = true;
        ui_draw(ui);

        GeminiResponse *resp = gemini_fetch(&url);
        ui->loading = false;

        if (resp && gemini_status_category(resp->status) == 2) {
            if (ui->document) document_free(ui->document);
            ui->document = document_parse(resp->body, resp->body_len);
            memcpy(&ui->current_url, &url, sizeof(Url));
            ui->scroll_y = scroll;
        }
        gemini_response_free(resp);
        ui->needs_redraw = true;
    }
}

void ui_navigate(UI *ui, const char *url_str) {
    if (!ui || !url_str) return;

    /* Handle special bookmark URLs */
    if (strcmp(url_str, "gemini://bookmarks/") == 0) {
        ui_show_bookmarks(ui);
        return;
    }
    if (strncmp(url_str, "gemini://bookmarks/delete/", 26) == 0) {
        int index = atoi(url_str + 26);
        ui_delete_bookmark(ui, index);
        ui_show_bookmarks(ui);  /* Refresh the bookmarks page */
        return;
    }

    Url url;

    /* Check if it's a relative URL */
    if (ui->current_url.host[0] && !strstr(url_str, "://")) {
        if (!url_resolve(&ui->current_url, url_str, &url)) {
            snprintf(ui->status_message, sizeof(ui->status_message),
                     "Invalid URL: %s", url_str);
            ui->needs_redraw = true;
            return;
        }
    }
    else {
        if (!url_parse(url_str, &url)) {
            snprintf(ui->status_message, sizeof(ui->status_message),
                     "Invalid URL: %s", url_str);
            ui->needs_redraw = true;
            return;
        }
    }

    if (!url_is_gemini(&url)) {
        snprintf(ui->status_message, sizeof(ui->status_message),
                 "Unsupported protocol: %s", url.scheme);
        ui->needs_redraw = true;
        return;
    }

    /* Save scroll position for current page */
    history_update_scroll(&ui->history, ui->scroll_y);

    /* Show loading state */
    ui->loading = true;
    snprintf(ui->status_message, sizeof(ui->status_message), "Loading %s...", url.host);
    ui_draw(ui);

    /* Fetch the page */
    GeminiResponse *resp = gemini_fetch(&url);

    ui->loading = false;

    if (!resp) {
        snprintf(ui->status_message, sizeof(ui->status_message), "Request failed");
        ui->needs_redraw = true;
        return;
    }

    int category = gemini_status_category(resp->status);

    if (category == 3) {
        /* Redirect */
        char *redirect_url = strdup(resp->meta);
        gemini_response_free(resp);

        /* Limit redirect depth */
        static int redirect_count = 0;
        redirect_count++;
        if (redirect_count > 5) {
            redirect_count = 0;
            snprintf(ui->status_message, sizeof(ui->status_message), "Too many redirects");
            ui->needs_redraw = true;
            free(redirect_url);
            return;
        }

        ui_navigate(ui, redirect_url);
        redirect_count = 0;
        free(redirect_url);
        return;
    }

    if (category != 2) {
        /* Error */
        if (ui->document) {
            document_free(ui->document);
            ui->document = NULL;
        }

        snprintf(ui->status_message, sizeof(ui->status_message),
                 "%s: %s", gemini_status_string(resp->status),
                 resp->error_msg[0] ? resp->error_msg : resp->meta);

        render_error(ui->renderer, gemini_status_string(resp->status),
                    resp->error_msg[0] ? resp->error_msg : resp->meta);
        render_address_bar(ui->renderer, url.full, false, false, history_can_back(&ui->history));
        render_flip(ui->renderer);

        gemini_response_free(resp);
        return;
    }

    /* Success - parse document */
    if (ui->document) {
        document_free(ui->document);
    }

    /* Check MIME type */
    const char *mime = resp->meta;
    if (strncmp(mime, "text/gemini", 11) == 0 || mime[0] == '\0') {
        ui->document = document_parse(resp->body, resp->body_len);
    }
    else if (strncmp(mime, "text/", 5) == 0) {
        /* Plain text - wrap in simple document */
        ui->document = document_new();
        if (ui->document && resp->body) {
            document_add_line(ui->document, LINE_PREFORMATTED, resp->body, NULL);
        }
    }
    else {
        /* Unsupported MIME type */
        ui->document = document_new();
        if (ui->document) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Cannot display: %s", mime);
            document_add_line(ui->document, LINE_TEXT, msg, NULL);
        }
    }

    gemini_response_free(resp);

    /* Update state */
    memcpy(&ui->current_url, &url, sizeof(Url));
    history_push(&ui->history, &url, 0);
    ui->scroll_y = 0;
    ui->scroll_velocity = 0;
    ui->status_message[0] = '\0';
    ui->needs_redraw = true;
}

bool ui_handle_event(UI *ui, SDL_Event *event) {
    if (!ui || !event) return true;

    switch (event->type) {
        case SDL_QUIT:
            ui->running = false;
            return false;

        case SDL_ACTIVEEVENT:
            if (event->active.state & SDL_APPACTIVE) {
                ui->paused = !event->active.gain;
                if (!ui->paused) {
                    ui->needs_redraw = true;
                }
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            ui->touch_active = true;
            ui->touch_start_x = event->button.x;
            ui->touch_start_y = event->button.y;
            ui->touch_last_y = event->button.y;
            ui->touch_start_time = SDL_GetTicks();
            ui->is_dragging = false;
            ui->scroll_velocity = 0;
            break;

        case SDL_MOUSEMOTION:
            if (ui->touch_active) {
                int dy = event->motion.y - ui->touch_last_y;
                int total_dx = abs(event->motion.x - ui->touch_start_x);
                int total_dy = abs(event->motion.y - ui->touch_start_y);

                if (total_dx > TAP_THRESHOLD || total_dy > TAP_THRESHOLD) {
                    ui->is_dragging = true;
                }

                if (ui->is_dragging && !ui->address_focused) {
                    ui->scroll_y -= dy;

                    /* Clamp scroll */
                    if (ui->scroll_y < 0) ui->scroll_y = 0;
                    if (ui->renderer && ui->scroll_y > ui->max_scroll) {
                        ui->scroll_y = ui->max_scroll;
                    }

                    ui->scroll_velocity = -dy * MOMENTUM_SCALE;
                    ui->needs_redraw = true;
                }

                ui->touch_last_y = event->motion.y;
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (ui->touch_active) {
                Uint32 tap_duration = SDL_GetTicks() - ui->touch_start_time;

                if (!ui->is_dragging && tap_duration < TAP_TIME_THRESHOLD) {
                    /* Tap detected */
                    int x = event->button.x;
                    int y = event->button.y;

                    if (y < MARGIN_TOP) {
                        /* Tapped address bar area - check buttons */
                        int btn = render_address_bar_hit_test(ui->renderer, x, y);
                        if (btn > 0) {
                            /* Trigger visual feedback */
                            render_button_highlight(ui->renderer, btn);
                            ui->needs_redraw = true;
                        }
                        if (btn == 1) {
                            ui_go_back(ui);
                        } else if (btn == 2) {
                            ui_add_bookmark(ui);
                        } else if (btn == 3) {
                            ui_show_bookmarks(ui);
                        } else {
                            ui_focus_address(ui);
                        }
                    }
                    else if (ui->address_focused) {
                        /* Tapped outside address bar - unfocus */
                        ui_unfocus_address(ui);
                    }
                    else if (ui->document) {
                        /* Check for link tap */
                        int link_idx = render_hit_test(ui->renderer, x, y);
                        if (link_idx >= 0 && link_idx < (int)ui->document->num_lines) {
                            const char *link_url = ui->document->lines[link_idx].url;
                            if (link_url) {
                                ui_navigate(ui, link_url);
                            }
                        }
                    }
                }

                ui->touch_active = false;
            }
            break;

        case SDL_KEYDOWN:
            switch (event->key.keysym.sym) {
                case SDLK_ESCAPE:  /* Same as PDLK_GESTURE_BACK (27) */
                    if (ui->address_focused) {
                        ui_unfocus_address(ui);
                    }
                    else if (history_can_back(&ui->history)) {
                        ui_go_back(ui);
                    }
                    else {
#ifdef WEBOS
                        PDL_Minimize();
#else
                        ui->running = false;
#endif
                    }
                    break;

#ifdef WEBOS
                case 24:  /* PDLK_GESTURE_DISMISS_KEYBOARD */
                    if (ui->address_focused) {
                        ui_unfocus_address(ui);
                    }
                    break;
#endif

                case SDLK_RETURN:
                    if (ui->address_focused) {
                        ui_unfocus_address(ui);
                        ui_navigate(ui, ui->address_input);
                    }
                    break;

                case SDLK_BACKSPACE:
                    if (ui->address_focused && ui->address_cursor > 0) {
                        memmove(&ui->address_input[ui->address_cursor - 1],
                                &ui->address_input[ui->address_cursor],
                                strlen(&ui->address_input[ui->address_cursor]) + 1);
                        ui->address_cursor--;
                        ui->needs_redraw = true;
                    }
                    break;

                default:
                    if (ui->address_focused && event->key.keysym.unicode >= 32 &&
                        event->key.keysym.unicode < 127) {
                        size_t len = strlen(ui->address_input);
                        if (len < sizeof(ui->address_input) - 1) {
                            memmove(&ui->address_input[ui->address_cursor + 1],
                                    &ui->address_input[ui->address_cursor],
                                    len - ui->address_cursor + 1);
                            ui->address_input[ui->address_cursor] = (char)event->key.keysym.unicode;
                            ui->address_cursor++;
                            ui->needs_redraw = true;
                        }
                    }
                    break;
            }
            break;

        case SDL_VIDEOEXPOSE:
            ui->needs_redraw = true;
            break;
    }

    return ui->running;
}

void ui_update(UI *ui, Uint32 dt) {
    if (!ui) return;

    (void)dt;  /* Could use for smoother animation */

    /* Apply scroll momentum */
    if (!ui->touch_active && fabsf(ui->scroll_velocity) > SCROLL_MIN_VELOCITY) {
        ui->scroll_y += (int)ui->scroll_velocity;
        ui->scroll_velocity *= SCROLL_FRICTION;

        /* Clamp scroll */
        if (ui->scroll_y < 0) {
            ui->scroll_y = 0;
            ui->scroll_velocity = 0;
        }
        if (ui->scroll_y > ui->max_scroll) {
            ui->scroll_y = ui->max_scroll;
            ui->scroll_velocity = 0;
        }

        ui->needs_redraw = true;
    }
    else if (!ui->touch_active) {
        ui->scroll_velocity = 0;
    }
}

void ui_draw(UI *ui) {
    if (!ui || !ui->renderer) return;

    if (ui->loading) {
        render_loading(ui->renderer, ui->status_message);
        render_flip(ui->renderer);
        return;
    }

    if (ui->document) {
        render_document(ui->renderer, ui->document, ui->scroll_y);

        /* Calculate max scroll */
        ui->max_scroll = ui->renderer->content_height - (ui->screen_height - MARGIN_TOP);
        if (ui->max_scroll < 0) ui->max_scroll = 0;
    }
    else {
        render_clear(ui->renderer);

        if (ui->status_message[0]) {
            render_error(ui->renderer, "Error", ui->status_message);
        }
    }

    /* Address bar */
    const char *display_url = ui->address_focused ? ui->address_input : ui->current_url.full;
    render_address_bar(ui->renderer, display_url, ui->loading, ui->address_focused, history_can_back(&ui->history));

    render_flip(ui->renderer);
    ui->needs_redraw = false;
}

void ui_run(UI *ui) {
    log_msg("ui_run entered");
    if (!ui) {
        log_msg("ui_run: ui is NULL, returning");
        return;
    }

    /* Initialize Gemini */
    log_msg("Initializing Gemini/OpenSSL...");
    if (!gemini_init()) {
        log_msg("ERROR: gemini_init failed!");
        fprintf(stderr, "Failed to initialize Gemini\n");
        return;
    }
    log_msg("Gemini initialized");

    /* Navigate to start page */
    log_msg("Navigating to start page: %s", DEFAULT_URL);
    ui_navigate(ui, DEFAULT_URL);

    Uint32 last_time = SDL_GetTicks();

    while (ui->running) {
        SDL_Event event;

        if (ui->paused) {
            /* Wait for events when paused */
            SDL_WaitEvent(&event);
            ui_handle_event(ui, &event);
        }
        else {
            /* Poll events */
            while (SDL_PollEvent(&event)) {
                if (!ui_handle_event(ui, &event)) {
                    break;
                }
            }
        }

        if (!ui->running) break;

        /* Update */
        Uint32 now = SDL_GetTicks();
        Uint32 dt = now - last_time;
        last_time = now;

        ui_update(ui, dt);

        /* Draw if needed */
        if (ui->needs_redraw && !ui->paused) {
            ui_draw(ui);
        }

        /* Limit frame rate */
        if (!ui->paused && !ui->needs_redraw) {
            SDL_Delay(16);  /* ~60 FPS */
        }
    }

    gemini_cleanup();
}
