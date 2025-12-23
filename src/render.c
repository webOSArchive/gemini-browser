/* Gemini Browser - SDL rendering */
#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL_image.h>

/* Font sizes */
#define FONT_SIZE_REGULAR   20
#define FONT_SIZE_MONO      18
#define FONT_SIZE_H1        28
#define FONT_SIZE_H2        24
#define FONT_SIZE_H3        22

/* Font paths - webOS system font or bundled */
#define FONT_PATH_REGULAR   "/usr/share/fonts/Prelude-Medium.ttf"
#define FONT_PATH_MONO      "/usr/share/fonts/DejaVuSansMono.ttf"
/* Fallback paths - absolute path to app directory */
#define APP_DIR "/media/cryptofs/apps/usr/palm/applications/org.webosarchive.geminibrowser"
#define FONT_PATH_REGULAR_FALLBACK APP_DIR "/DejaVuSans.ttf"
#define FONT_PATH_MONO_FALLBACK    APP_DIR "/DejaVuSansMono.ttf"

/* Button icons */
#define ICON_PATH_BACK          APP_DIR "/icon-back.png"
#define ICON_PATH_BOOKMARK_ADD  APP_DIR "/icon-bookmark-add.png"
#define ICON_PATH_BOOKMARKS     APP_DIR "/icon-bookmarks.png"

/* Highlight duration in ms */
#define HIGHLIGHT_DURATION_MS   150

#define RENDERED_INITIAL_CAPACITY 256

static TTF_Font *try_open_font(const char *primary, const char *fallback, int size) {
    TTF_Font *font = TTF_OpenFont(primary, size);
    if (!font && fallback) {
        font = TTF_OpenFont(fallback, size);
    }
    return font;
}

Renderer *render_init(SDL_Surface *screen) {
    if (!screen) return NULL;

    Renderer *r = calloc(1, sizeof(Renderer));
    if (!r) return NULL;

    r->screen = screen;

    /* Initialize SDL_ttf */
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        free(r);
        return NULL;
    }

    /* Load fonts */
    r->font_regular = try_open_font(FONT_PATH_REGULAR, FONT_PATH_REGULAR_FALLBACK, FONT_SIZE_REGULAR);
    r->font_mono = try_open_font(FONT_PATH_MONO, FONT_PATH_MONO_FALLBACK, FONT_SIZE_MONO);
    r->font_h1 = try_open_font(FONT_PATH_REGULAR, FONT_PATH_REGULAR_FALLBACK, FONT_SIZE_H1);
    r->font_h2 = try_open_font(FONT_PATH_REGULAR, FONT_PATH_REGULAR_FALLBACK, FONT_SIZE_H2);
    r->font_h3 = try_open_font(FONT_PATH_REGULAR, FONT_PATH_REGULAR_FALLBACK, FONT_SIZE_H3);

    if (!r->font_regular) {
        fprintf(stderr, "Failed to load regular font\n");
        render_cleanup(r);
        return NULL;
    }

    /* Fall back to regular font if others fail */
    if (!r->font_mono) r->font_mono = r->font_regular;
    if (!r->font_h1) r->font_h1 = r->font_regular;
    if (!r->font_h2) r->font_h2 = r->font_regular;
    if (!r->font_h3) r->font_h3 = r->font_regular;

    r->line_height = TTF_FontLineSkip(r->font_regular);
    r->mono_line_height = TTF_FontLineSkip(r->font_mono);

    /* Allocate rendered lines array */
    r->rendered_capacity = RENDERED_INITIAL_CAPACITY;
    r->rendered_lines = calloc(r->rendered_capacity, sizeof(RenderedLine));

    /* Load button icons (optional - falls back to text) */
    r->icon_back = IMG_Load(ICON_PATH_BACK);
    r->icon_bookmark_add = IMG_Load(ICON_PATH_BOOKMARK_ADD);
    r->icon_bookmarks = IMG_Load(ICON_PATH_BOOKMARKS);

    /* Initialize highlight state */
    r->highlight_button = 0;
    r->highlight_time = 0;

    return r;
}

void render_cleanup(Renderer *r) {
    if (!r) return;

    if (r->font_h3 && r->font_h3 != r->font_regular) TTF_CloseFont(r->font_h3);
    if (r->font_h2 && r->font_h2 != r->font_regular) TTF_CloseFont(r->font_h2);
    if (r->font_h1 && r->font_h1 != r->font_regular) TTF_CloseFont(r->font_h1);
    if (r->font_mono && r->font_mono != r->font_regular) TTF_CloseFont(r->font_mono);
    if (r->font_regular) TTF_CloseFont(r->font_regular);

    /* Free icons */
    if (r->icon_back) SDL_FreeSurface(r->icon_back);
    if (r->icon_bookmark_add) SDL_FreeSurface(r->icon_bookmark_add);
    if (r->icon_bookmarks) SDL_FreeSurface(r->icon_bookmarks);

    free(r->rendered_lines);
    free(r);

    TTF_Quit();
}

void render_clear(Renderer *r) {
    if (!r || !r->screen) return;

    SDL_Rect rect = { 0, 0, r->screen->w, r->screen->h };
    Uint32 color = SDL_MapRGB(r->screen->format, COLOR_BG_R, COLOR_BG_G, COLOR_BG_B);
    SDL_FillRect(r->screen, &rect, color);

    r->num_rendered = 0;
}

static void add_rendered_line(Renderer *r, int x, int y, int w, int h, int doc_index, bool is_link) {
    if (r->num_rendered >= r->rendered_capacity) {
        size_t new_cap = r->rendered_capacity * 2;
        RenderedLine *new_lines = realloc(r->rendered_lines, new_cap * sizeof(RenderedLine));
        if (!new_lines) return;
        r->rendered_lines = new_lines;
        r->rendered_capacity = new_cap;
    }

    RenderedLine *rl = &r->rendered_lines[r->num_rendered++];
    rl->bounds.x = x;
    rl->bounds.y = y;
    rl->bounds.w = w;
    rl->bounds.h = h;
    rl->doc_line_index = doc_index;
    rl->is_link = is_link;
}

static void render_text_wrapped(Renderer *r, TTF_Font *font, const char *text,
                                SDL_Color color, int x, int y, int max_width,
                                int *out_height, int doc_index, bool is_link) {
    if (!text || !*text) {
        *out_height = TTF_FontLineSkip(font);
        return;
    }

    int total_height = 0;
    int line_skip = TTF_FontLineSkip(font);
    const char *p = text;

    while (*p) {
        /* Find how much text fits on this line */
        int fit_len = 0;
        int last_space = -1;
        int width = 0;

        while (p[fit_len]) {
            char temp[2] = { p[fit_len], '\0' };
            int char_w, char_h;
            TTF_SizeUTF8(font, temp, &char_w, &char_h);

            if (width + char_w > max_width && fit_len > 0) {
                /* Break at last space if possible */
                if (last_space > 0) {
                    fit_len = last_space;
                }
                break;
            }

            if (p[fit_len] == ' ') {
                last_space = fit_len;
            }

            width += char_w;
            fit_len++;
        }

        if (fit_len == 0) fit_len = 1;  /* At least one character */

        /* Render this segment */
        char *segment = malloc(fit_len + 1);
        if (segment) {
            memcpy(segment, p, fit_len);
            segment[fit_len] = '\0';

            SDL_Surface *text_surface = TTF_RenderUTF8_Blended(font, segment, color);
            if (text_surface) {
                SDL_Rect dest = { x, y + total_height, text_surface->w, text_surface->h };
                SDL_BlitSurface(text_surface, NULL, r->screen, &dest);

                /* Record for hit testing */
                if (is_link) {
                    add_rendered_line(r, x, y + total_height, text_surface->w, text_surface->h,
                                     doc_index, true);
                }

                SDL_FreeSurface(text_surface);
            }
            free(segment);
        }

        p += fit_len;
        while (*p == ' ') p++;  /* Skip space at break point */
        total_height += line_skip;
    }

    *out_height = total_height > 0 ? total_height : line_skip;
}

void render_document(Renderer *r, const Document *doc, int scroll_y) {
    if (!r || !doc) return;

    render_clear(r);

    int y = MARGIN_TOP - scroll_y;
    int max_width = r->screen->w - MARGIN_LEFT - MARGIN_RIGHT;

    r->num_rendered = 0;

    for (size_t i = 0; i < doc->num_lines; i++) {
        const DocLine *line = &doc->lines[i];
        int line_height = 0;

        /* Skip if completely above viewport */
        if (y + 100 < 0) {
            /* Estimate height and skip */
            switch (line->type) {
                case LINE_HEADING1: line_height = TTF_FontLineSkip(r->font_h1) + 8; break;
                case LINE_HEADING2: line_height = TTF_FontLineSkip(r->font_h2) + 6; break;
                case LINE_HEADING3: line_height = TTF_FontLineSkip(r->font_h3) + 4; break;
                case LINE_PREFORMATTED: line_height = r->mono_line_height; break;
                default: line_height = r->line_height; break;
            }
            y += line_height + LINE_SPACING;
            continue;
        }

        /* Stop if below viewport */
        if (y > r->screen->h) {
            break;
        }

        SDL_Color color;
        TTF_Font *font = r->font_regular;
        int x = MARGIN_LEFT;
        bool is_link = false;

        switch (line->type) {
            case LINE_HEADING1:
                color = (SDL_Color){ COLOR_HEADING_R, COLOR_HEADING_G, COLOR_HEADING_B, 255 };
                font = r->font_h1;
                y += 8;  /* Extra spacing before heading */
                break;

            case LINE_HEADING2:
                color = (SDL_Color){ COLOR_HEADING_R, COLOR_HEADING_G, COLOR_HEADING_B, 255 };
                font = r->font_h2;
                y += 6;
                break;

            case LINE_HEADING3:
                color = (SDL_Color){ COLOR_HEADING_R, COLOR_HEADING_G, COLOR_HEADING_B, 255 };
                font = r->font_h3;
                y += 4;
                break;

            case LINE_LINK:
                color = (SDL_Color){ COLOR_LINK_R, COLOR_LINK_G, COLOR_LINK_B, 255 };
                is_link = true;
                break;

            case LINE_LIST_ITEM:
                color = (SDL_Color){ COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B, 255 };
                /* Render bullet */
                {
                    SDL_Surface *bullet = TTF_RenderUTF8_Blended(r->font_regular, "\xE2\x80\xA2", color);
                    if (bullet) {
                        SDL_Rect dest = { x, y, bullet->w, bullet->h };
                        SDL_BlitSurface(bullet, NULL, r->screen, &dest);
                        SDL_FreeSurface(bullet);
                    }
                }
                x += 20;
                break;

            case LINE_QUOTE:
                color = (SDL_Color){ COLOR_QUOTE_R, COLOR_QUOTE_G, COLOR_QUOTE_B, 255 };
                /* Draw quote bar */
                {
                    SDL_Rect bar = { MARGIN_LEFT, y, 3, r->line_height };
                    Uint32 bar_color = SDL_MapRGB(r->screen->format, COLOR_QUOTE_R, COLOR_QUOTE_G, COLOR_QUOTE_B);
                    SDL_FillRect(r->screen, &bar, bar_color);
                }
                x += 15;
                break;

            case LINE_PREFORMATTED:
                color = (SDL_Color){ COLOR_PRE_R, COLOR_PRE_G, COLOR_PRE_B, 255 };
                font = r->font_mono;
                break;

            default:
                color = (SDL_Color){ COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B, 255 };
                break;
        }

        const char *text = line->text;
        if (!text || !*text) {
            /* Empty line */
            line_height = TTF_FontLineSkip(font);
        }
        else if (line->type == LINE_PREFORMATTED) {
            /* Preformatted - no wrapping */
            SDL_Surface *text_surface = TTF_RenderUTF8_Blended(font, text, color);
            if (text_surface) {
                SDL_Rect dest = { x, y, text_surface->w, text_surface->h };
                SDL_BlitSurface(text_surface, NULL, r->screen, &dest);
                line_height = text_surface->h;
                SDL_FreeSurface(text_surface);
            }
            else {
                line_height = r->mono_line_height;
            }
        }
        else {
            /* Word-wrapped text */
            render_text_wrapped(r, font, text, color, x, y, max_width - (x - MARGIN_LEFT),
                               &line_height, (int)i, is_link);
        }

        y += line_height + LINE_SPACING;
    }

    r->content_height = y + scroll_y;
}

/* Button positions in address bar */
#define BTN_BACK_X      5
#define BTN_BACK_W      35
#define BTN_URL_X       45
#define BTN_BOOKMARK_W  40
#define BTN_STAR_W      35

void render_address_bar(Renderer *r, const char *url, bool loading, bool focused, bool can_go_back) {
    if (!r) return;

    int screen_w = r->screen->w;

    /* Check if highlight has expired */
    int highlight = 0;
    if (r->highlight_button > 0) {
        Uint32 elapsed = SDL_GetTicks() - r->highlight_time;
        if (elapsed < HIGHLIGHT_DURATION_MS) {
            highlight = r->highlight_button;
        } else {
            r->highlight_button = 0;
        }
    }

    /* Background */
    SDL_Rect bar = { 0, 0, screen_w, MARGIN_TOP - 5 };
    Uint32 bg_color = SDL_MapRGB(r->screen->format,
                                  focused ? 0x30 : 0x28,
                                  focused ? 0x30 : 0x28,
                                  focused ? 0x38 : 0x2e);
    SDL_FillRect(r->screen, &bar, bg_color);

    /* Border */
    SDL_Rect border = { 0, MARGIN_TOP - 6, screen_w, 1 };
    Uint32 border_color = SDL_MapRGB(r->screen->format, 0x50, 0x50, 0x58);
    SDL_FillRect(r->screen, &border, border_color);

    /* Bookmark buttons position on right */
    int btn_x = screen_w - BTN_STAR_W - BTN_BOOKMARK_W - 10;

    /* Draw highlight backgrounds */
    Uint32 highlight_color = SDL_MapRGB(r->screen->format, 0x50, 0x50, 0x60);
    if (highlight == 1) {
        SDL_Rect hl = { BTN_BACK_X, 2, BTN_BACK_W, MARGIN_TOP - 9 };
        SDL_FillRect(r->screen, &hl, highlight_color);
    } else if (highlight == 2) {
        SDL_Rect hl = { btn_x, 2, BTN_BOOKMARK_W, MARGIN_TOP - 9 };
        SDL_FillRect(r->screen, &hl, highlight_color);
    } else if (highlight == 3) {
        SDL_Rect hl = { btn_x + BTN_BOOKMARK_W, 2, BTN_STAR_W, MARGIN_TOP - 9 };
        SDL_FillRect(r->screen, &hl, highlight_color);
    }

    /* Back button */
    if (r->icon_back) {
        SDL_Rect dest = { BTN_BACK_X + (BTN_BACK_W - r->icon_back->w) / 2,
                          (MARGIN_TOP - 5 - r->icon_back->h) / 2,
                          r->icon_back->w, r->icon_back->h };
        if (!can_go_back) {
            /* Dim the icon by drawing semi-transparent overlay */
            SDL_BlitSurface(r->icon_back, NULL, r->screen, &dest);
            SDL_Rect dim = dest;
            SDL_FillRect(r->screen, &dim, SDL_MapRGB(r->screen->format, 0x28, 0x28, 0x2e) | 0x80000000);
        } else {
            SDL_BlitSurface(r->icon_back, NULL, r->screen, &dest);
        }
    } else {
        SDL_Color color = can_go_back ?
            (SDL_Color){ COLOR_LINK_R, COLOR_LINK_G, COLOR_LINK_B, 255 } :
            (SDL_Color){ 0x55, 0x55, 0x55, 255 };
        SDL_Surface *text = TTF_RenderUTF8_Blended(r->font_regular, "<", color);
        if (text) {
            SDL_Rect dest = { BTN_BACK_X + 10, 10, text->w, text->h };
            SDL_BlitSurface(text, NULL, r->screen, &dest);
            SDL_FreeSurface(text);
        }
    }

    /* Add bookmark button */
    if (r->icon_bookmark_add) {
        SDL_Rect dest = { btn_x + (BTN_BOOKMARK_W - r->icon_bookmark_add->w) / 2,
                          (MARGIN_TOP - 5 - r->icon_bookmark_add->h) / 2,
                          r->icon_bookmark_add->w, r->icon_bookmark_add->h };
        SDL_BlitSurface(r->icon_bookmark_add, NULL, r->screen, &dest);
    } else {
        SDL_Color color = { COLOR_HEADING_R, COLOR_HEADING_G, COLOR_HEADING_B, 255 };
        SDL_Surface *text = TTF_RenderUTF8_Blended(r->font_regular, "+", color);
        if (text) {
            SDL_Rect dest = { btn_x + 10, 10, text->w, text->h };
            SDL_BlitSurface(text, NULL, r->screen, &dest);
            SDL_FreeSurface(text);
        }
    }

    /* Show bookmarks button */
    if (r->icon_bookmarks) {
        SDL_Rect dest = { btn_x + BTN_BOOKMARK_W + (BTN_STAR_W - r->icon_bookmarks->w) / 2,
                          (MARGIN_TOP - 5 - r->icon_bookmarks->h) / 2,
                          r->icon_bookmarks->w, r->icon_bookmarks->h };
        SDL_BlitSurface(r->icon_bookmarks, NULL, r->screen, &dest);
    } else {
        SDL_Color color = { COLOR_HEADING_R, COLOR_HEADING_G, COLOR_HEADING_B, 255 };
        SDL_Surface *text = TTF_RenderUTF8_Blended(r->font_regular, "*", color);
        if (text) {
            SDL_Rect dest = { btn_x + BTN_BOOKMARK_W + 8, 10, text->w, text->h };
            SDL_BlitSurface(text, NULL, r->screen, &dest);
            SDL_FreeSurface(text);
        }
    }

    /* URL text - between back button and bookmark buttons */
    int url_max_w = btn_x - BTN_URL_X - 10;
    if (url && *url && url_max_w > 50) {
        SDL_Color color = { 0xcc, 0xcc, 0xcc, 255 };
        SDL_Surface *text = TTF_RenderUTF8_Blended(r->font_regular, url, color);
        if (text) {
            SDL_Rect dest = { BTN_URL_X, 10, text->w, text->h };
            /* Clip to available width */
            SDL_Rect clip = { 0, 0, url_max_w, text->h };
            if (text->w > url_max_w) {
                clip.x = text->w - url_max_w;  /* Show end of URL */
            }
            SDL_BlitSurface(text, text->w > url_max_w ? &clip : NULL, r->screen, &dest);
            SDL_FreeSurface(text);
        }
    }

    /* Loading indicator */
    if (loading) {
        SDL_Color color = { COLOR_LINK_R, COLOR_LINK_G, COLOR_LINK_B, 255 };
        SDL_Surface *text = TTF_RenderUTF8_Blended(r->font_regular, "...", color);
        if (text) {
            SDL_Rect dest = { BTN_URL_X, 10, text->w, text->h };
            SDL_BlitSurface(text, NULL, r->screen, &dest);
            SDL_FreeSurface(text);
        }
    }
}

int render_address_bar_hit_test(Renderer *r, int x, int y) {
    if (!r || y >= MARGIN_TOP) return 0;

    int screen_w = r->screen->w;
    int btn_x = screen_w - BTN_STAR_W - BTN_BOOKMARK_W - 10;

    /* Back button */
    if (x >= BTN_BACK_X && x < BTN_BACK_X + BTN_BACK_W) {
        return 1;
    }

    /* Add bookmark button */
    if (x >= btn_x && x < btn_x + BTN_BOOKMARK_W) {
        return 2;
    }

    /* Show bookmarks button */
    if (x >= btn_x + BTN_BOOKMARK_W && x < btn_x + BTN_BOOKMARK_W + BTN_STAR_W) {
        return 3;
    }

    return 0;  /* URL area or other */
}

void render_button_highlight(Renderer *r, int button) {
    if (!r || button < 1 || button > 3) return;
    r->highlight_button = button;
    r->highlight_time = SDL_GetTicks();
}

void render_loading(Renderer *r, const char *message) {
    if (!r) return;

    render_clear(r);

    SDL_Color color = { COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B, 255 };
    const char *msg = message ? message : "Loading...";
    SDL_Surface *text = TTF_RenderUTF8_Blended(r->font_h2, msg, color);
    if (text) {
        SDL_Rect dest = {
            (r->screen->w - text->w) / 2,
            (r->screen->h - text->h) / 2,
            text->w, text->h
        };
        SDL_BlitSurface(text, NULL, r->screen, &dest);
        SDL_FreeSurface(text);
    }
}

void render_error(Renderer *r, const char *title, const char *message) {
    if (!r) return;

    render_clear(r);

    int y = r->screen->h / 3;

    /* Title */
    SDL_Color title_color = { 0xff, 0x66, 0x66, 255 };
    SDL_Surface *title_surf = TTF_RenderUTF8_Blended(r->font_h1, title ? title : "Error", title_color);
    if (title_surf) {
        SDL_Rect dest = { (r->screen->w - title_surf->w) / 2, y, title_surf->w, title_surf->h };
        SDL_BlitSurface(title_surf, NULL, r->screen, &dest);
        y += title_surf->h + 20;
        SDL_FreeSurface(title_surf);
    }

    /* Message */
    if (message && *message) {
        SDL_Color msg_color = { COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B, 255 };
        SDL_Surface *msg_surf = TTF_RenderUTF8_Blended(r->font_regular, message, msg_color);
        if (msg_surf) {
            SDL_Rect dest = { (r->screen->w - msg_surf->w) / 2, y, msg_surf->w, msg_surf->h };
            SDL_BlitSurface(msg_surf, NULL, r->screen, &dest);
            SDL_FreeSurface(msg_surf);
        }
    }
}

int render_hit_test(Renderer *r, int x, int y) {
    if (!r) return -1;

    for (size_t i = 0; i < r->num_rendered; i++) {
        RenderedLine *rl = &r->rendered_lines[i];
        if (rl->is_link &&
            x >= rl->bounds.x && x < rl->bounds.x + rl->bounds.w &&
            y >= rl->bounds.y && y < rl->bounds.y + rl->bounds.h) {
            return rl->doc_line_index;
        }
    }
    return -1;
}

void render_flip(Renderer *r) {
    if (!r || !r->screen) return;
    SDL_Flip(r->screen);
}
