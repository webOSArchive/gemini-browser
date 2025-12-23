/* Gemini Browser - Navigation history */
#include "history.h"
#include <string.h>

void history_init(History *h) {
    if (!h) return;
    memset(h, 0, sizeof(History));
    h->current = -1;
}

void history_push(History *h, const Url *url, int scroll_y) {
    if (!h || !url) return;

    /* Clear forward history */
    h->count = h->current + 1;

    /* Check if we're at capacity */
    if (h->count >= HISTORY_MAX_ENTRIES) {
        /* Shift everything down */
        memmove(&h->entries[0], &h->entries[1],
                (HISTORY_MAX_ENTRIES - 1) * sizeof(Url));
        memmove(&h->scroll_positions[0], &h->scroll_positions[1],
                (HISTORY_MAX_ENTRIES - 1) * sizeof(int));
        h->count = HISTORY_MAX_ENTRIES - 1;
        h->current = h->count - 1;
    }

    /* Add new entry */
    memcpy(&h->entries[h->count], url, sizeof(Url));
    h->scroll_positions[h->count] = scroll_y;
    h->current = h->count;
    h->count++;
}

void history_update_scroll(History *h, int scroll_y) {
    if (!h || h->current < 0 || h->current >= h->count) return;
    h->scroll_positions[h->current] = scroll_y;
}

bool history_back(History *h, Url *url, int *scroll_y) {
    if (!h || !history_can_back(h)) return false;

    h->current--;
    if (url) {
        memcpy(url, &h->entries[h->current], sizeof(Url));
    }
    if (scroll_y) {
        *scroll_y = h->scroll_positions[h->current];
    }
    return true;
}

bool history_forward(History *h, Url *url, int *scroll_y) {
    if (!h || !history_can_forward(h)) return false;

    h->current++;
    if (url) {
        memcpy(url, &h->entries[h->current], sizeof(Url));
    }
    if (scroll_y) {
        *scroll_y = h->scroll_positions[h->current];
    }
    return true;
}

bool history_can_back(const History *h) {
    return h && h->current > 0;
}

bool history_can_forward(const History *h) {
    return h && h->current < h->count - 1;
}

const Url *history_current(const History *h) {
    if (!h || h->current < 0 || h->current >= h->count) return NULL;
    return &h->entries[h->current];
}
