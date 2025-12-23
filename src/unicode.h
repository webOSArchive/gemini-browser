/* Gemini Browser - Unicode fallback for webOS (Unicode 6.0) */
#ifndef PALMINI_UNICODE_H
#define PALMINI_UNICODE_H

/*
 * Sanitize UTF-8 text by replacing Unicode 7.0+ characters
 * with Unicode 6.0 compatible fallbacks or ASCII approximations.
 *
 * Returns a newly allocated string that must be freed by caller.
 * Returns NULL on allocation failure.
 */
char *unicode_sanitize(const char *text);

#endif /* PALMINI_UNICODE_H */
