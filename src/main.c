/* Gemini Browser for webOS
 *
 * A minimal Gemini protocol browser for the HP TouchPad,
 * using SDL 1.2 and OpenSSL.
 */

#include <stdio.h>
#include <stdlib.h>
#include "ui.h"

/* Early logging before UI init */
static void early_log(const char *msg) {
    FILE *f = fopen("/media/internal/gemini-log.txt", "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    early_log("=== main() entered ===");

    printf("Gemini Browser for webOS\n");

    early_log("Calling ui_init...");
    UI *ui = ui_init();
    if (!ui) {
        early_log("ERROR: ui_init failed");
        fprintf(stderr, "Failed to initialize UI\n");
        return 1;
    }

    early_log("Calling ui_run...");
    ui_run(ui);
    early_log("ui_run returned, cleaning up...");
    ui_cleanup(ui);

    early_log("Gemini exiting normally");
    printf("Gemini exiting\n");
    return 0;
}
