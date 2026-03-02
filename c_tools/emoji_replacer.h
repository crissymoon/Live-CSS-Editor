#ifndef EMOJI_REPLACER_H
#define EMOJI_REPLACER_H

#include <stdbool.h>
#include "logger.h"

/* Emoji replacement structure */
typedef struct EmojiReplacement {
    char *emoji;
    char *placeholder;
    int count;
    struct EmojiReplacement *next;
} EmojiReplacement;

/* Replacer context */
typedef struct EmojiReplacer {
    EmojiReplacement *replacements;
    Logger *logger;
    int total_replacements;
} EmojiReplacer;

/* Common emoji to placeholder mappings */
typedef struct {
    const char *emoji;
    const char *placeholder;
} EmojiMapping;

/* Emoji replacer API */
EmojiReplacer* emoji_replacer_create(Logger *logger);
void emoji_replacer_destroy(EmojiReplacer *replacer);

/* Add custom mapping */
void emoji_replacer_add_mapping(EmojiReplacer *replacer, const char *emoji, const char *placeholder);

/* Load default mappings */
void emoji_replacer_load_defaults(EmojiReplacer *replacer);

/* Replace emojis in text */
char* emoji_replacer_process(EmojiReplacer *replacer, const char *text);

/* Replace emojis in file */
int emoji_replacer_process_file(EmojiReplacer *replacer, const char *input_path, const char *output_path);

/* Scan file for emojis without replacing */
EmojiReplacement* emoji_replacer_scan_file(EmojiReplacer *replacer, const char *file_path, int *count);

/* Get statistics */
void emoji_replacer_print_stats(EmojiReplacer *replacer);
void emoji_replacer_reset_stats(EmojiReplacer *replacer);

/* Utility functions */
bool is_emoji_sequence(const unsigned char *str, int *length);
char* emoji_to_placeholder(EmojiReplacer *replacer, const char *emoji);

#endif /* EMOJI_REPLACER_H */
