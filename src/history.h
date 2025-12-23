/* Gemini Browser - Navigation history */
#ifndef PALMINI_HISTORY_H
#define PALMINI_HISTORY_H

#include <stdbool.h>
#include "url.h"

#define HISTORY_MAX_ENTRIES 100

typedef struct {
    Url entries[HISTORY_MAX_ENTRIES];
    int scroll_positions[HISTORY_MAX_ENTRIES];
    int current;        /* Current position in history */
    int count;          /* Total entries */
} History;

/* Initialize history */
void history_init(History *h);

/* Push a new URL onto history (clears forward history) */
void history_push(History *h, const Url *url, int scroll_y);

/* Update scroll position for current entry */
void history_update_scroll(History *h, int scroll_y);

/* Go back in history. Returns false if at beginning. */
bool history_back(History *h, Url *url, int *scroll_y);

/* Go forward in history. Returns false if at end. */
bool history_forward(History *h, Url *url, int *scroll_y);

/* Check if we can go back */
bool history_can_back(const History *h);

/* Check if we can go forward */
bool history_can_forward(const History *h);

/* Get current URL */
const Url *history_current(const History *h);

#endif /* PALMINI_HISTORY_H */
