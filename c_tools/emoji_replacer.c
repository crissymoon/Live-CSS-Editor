#include "emoji_replacer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Default emoji mappings */
static const EmojiMapping DEFAULT_MAPPINGS[] = {
    {"📁", "[FOLDER]"},
    {"📂", "[OPEN_FOLDER]"},
    {"📄", "[FILE]"},
    {"💾", "[SAVE]"},
    {"✏️", "[EDIT]"},
    {"✅", "[CHECK]"},
    {"❌", "[CLOSE]"},
    {"🔍", "[SEARCH]"},
    {"⚙️", "[SETTINGS]"},
    {"🎨", "[THEME]"},
    {"🐛", "[DEBUG]"},
    {"⚠️", "[WARNING]"},
    {"🔥", "[HOT]"},
    {"💡", "[IDEA]"},
    {"🚀", "[LAUNCH]"},
    {"📝", "[NOTE]"},
    {"🔧", "[TOOL]"},
    {"📊", "[CHART]"},
    {"📈", "[TRENDING_UP]"},
    {"📉", "[TRENDING_DOWN]"},
    {"🔒", "[LOCK]"},
    {"🔓", "[UNLOCK]"},
    {"🌐", "[WEB]"},
    {"📱", "[MOBILE]"},
    {"💻", "[COMPUTER]"},
    {"🖥️", "[DESKTOP]"},
    {"⌨️", "[KEYBOARD]"},
    {"🖱️", "[MOUSE]"},
    {"🎯", "[TARGET]"},
    {"✨", "[SPARKLE]"},
    {"🏁", "[FLAG]"},
    {"🔔", "[NOTIFICATION]"},
    {"📮", "[MAILBOX]"},
    {"📬", "[MAIL]"},
    {"📫", "[INBOX]"},
    {"🗑️", "[TRASH]"},
    {"📌", "[PIN]"},
    {"🔗", "[LINK]"},
    {"➡️", "->"},
    {"⬅️", "<-"},
    {"⬆️", "^"},
    {"⬇️", "v"},
    {NULL, NULL}
};

/* Emoji replacer creation */
EmojiReplacer* emoji_replacer_create(Logger *logger) {
    EmojiReplacer *replacer = (EmojiReplacer*)malloc(sizeof(EmojiReplacer));
    if (!replacer) return NULL;

    replacer->replacements = NULL;
    replacer->logger = logger;
    replacer->total_replacements = 0;

    if (logger) {
        LOG_INFO(logger, "EMOJI", "Emoji replacer initialized");
    }

    return replacer;
}

void emoji_replacer_destroy(EmojiReplacer *replacer) {
    if (!replacer) return;

    EmojiReplacement *current = replacer->replacements;
    while (current) {
        EmojiReplacement *next = current->next;
        if (current->emoji) free(current->emoji);
        if (current->placeholder) free(current->placeholder);
        free(current);
        current = next;
    }

    if (replacer->logger) {
        LOG_INFO(replacer->logger, "EMOJI", "Emoji replacer destroyed");
    }

    free(replacer);
}

/* Add mapping */
void emoji_replacer_add_mapping(EmojiReplacer *replacer, const char *emoji, const char *placeholder) {
    if (!replacer || !emoji || !placeholder) return;

    /* Check if mapping already exists */
    EmojiReplacement *current = replacer->replacements;
    while (current) {
        if (strcmp(current->emoji, emoji) == 0) {
            /* Update placeholder */
            if (current->placeholder) free(current->placeholder);
            current->placeholder = strdup(placeholder);
            return;
        }
        current = current->next;
    }

    /* Add new mapping */
    EmojiReplacement *new_mapping = (EmojiReplacement*)malloc(sizeof(EmojiReplacement));
    if (!new_mapping) return;

    new_mapping->emoji = strdup(emoji);
    new_mapping->placeholder = strdup(placeholder);
    new_mapping->count = 0;
    new_mapping->next = replacer->replacements;

    replacer->replacements = new_mapping;

    if (replacer->logger) {
        LOG_DEBUG(replacer->logger, "EMOJI", "Added mapping: %s -> %s", emoji, placeholder);
    }
}

/* Load defaults */
void emoji_replacer_load_defaults(EmojiReplacer *replacer) {
    if (!replacer) return;

    for (int i = 0; DEFAULT_MAPPINGS[i].emoji != NULL; i++) {
        emoji_replacer_add_mapping(replacer,
                                   DEFAULT_MAPPINGS[i].emoji,
                                   DEFAULT_MAPPINGS[i].placeholder);
    }

    if (replacer->logger) {
        LOG_INFO(replacer->logger, "EMOJI", "Loaded default emoji mappings");
    }
}

/* Check if UTF-8 sequence is an emoji */
bool is_emoji_sequence(const unsigned char *str, int *length) {
    if (!str || !length) return false;

    /* Basic emoji detection (UTF-8 multi-byte sequences) */
    unsigned char c = *str;

    /* Most emojis are in these ranges */
    if (c == 0xF0) {  /* 4-byte sequence */
        unsigned char c2 = *(str + 1);
        if (c2 >= 0x9F) {  /* Emoji range */
            *length = 4;
            return true;
        }
    } else if (c == 0xE2 || c == 0xE3) {  /* Some 3-byte symbols */
        *length = 3;
        return true;
    }

    *length = 1;
    return false;
}

/* Get placeholder for emoji */
char* emoji_to_placeholder(EmojiReplacer *replacer, const char *emoji) {
    if (!replacer || !emoji) return NULL;

    EmojiReplacement *current = replacer->replacements;
    while (current) {
        if (strcmp(current->emoji, emoji) == 0) {
            current->count++;
            return current->placeholder;
        }
        current = current->next;
    }

    return NULL;
}

/* Process text */
char* emoji_replacer_process(EmojiReplacer *replacer, const char *text) {
    if (!replacer || !text) return NULL;

    size_t input_len = strlen(text);
    size_t output_size = input_len * 2;  /* Assume placeholders might be longer */
    char *output = (char*)malloc(output_size);
    if (!output) return NULL;

    size_t out_pos = 0;
    size_t in_pos = 0;
    int replacements_made = 0;

    while (in_pos < input_len) {
        bool replaced = false;

        /* Try to match each emoji */
        EmojiReplacement *current = replacer->replacements;
        while (current) {
            size_t emoji_len = strlen(current->emoji);
            if (in_pos + emoji_len <= input_len &&
                strncmp(text + in_pos, current->emoji, emoji_len) == 0) {

                /* Found match - replace with placeholder */
                size_t placeholder_len = strlen(current->placeholder);

                /* Ensure we have space */
                if (out_pos + placeholder_len >= output_size - 1) {
                    output_size *= 2;
                    char *new_output = (char*)realloc(output, output_size);
                    if (!new_output) {
                        free(output);
                        return NULL;
                    }
                    output = new_output;
                }

                strcpy(output + out_pos, current->placeholder);
                out_pos += placeholder_len;
                in_pos += emoji_len;

                current->count++;
                replacements_made++;
                replaced = true;
                break;
            }
            current = current->next;
        }

        if (!replaced) {
            /* Copy character as-is */
            if (out_pos >= output_size - 1) {
                output_size *= 2;
                char *new_output = (char*)realloc(output, output_size);
                if (!new_output) {
                    free(output);
                    return NULL;
                }
                output = new_output;
            }
            output[out_pos++] = text[in_pos++];
        }
    }

    output[out_pos] = '\0';
    replacer->total_replacements += replacements_made;

    if (replacer->logger && replacements_made > 0) {
        LOG_DEBUG(replacer->logger, "EMOJI", "Replaced %d emojis in text", replacements_made);
    }

    return output;
}

/* Process file */
int emoji_replacer_process_file(EmojiReplacer *replacer, const char *input_path, const char *output_path) {
    if (!replacer || !input_path || !output_path) return -1;

    if (replacer->logger) {
        LOG_INFO(replacer->logger, "EMOJI", "Processing file: %s", input_path);
    }

    /* Read input file */
    FILE *input = fopen(input_path, "r");
    if (!input) {
        if (replacer->logger) {
            LOG_ERROR(replacer->logger, "EMOJI", "Cannot open input file: %s", input_path);
        }
        return -1;
    }

    fseek(input, 0, SEEK_END);
    long file_size = ftell(input);
    fseek(input, 0, SEEK_SET);

    char *content = (char*)malloc(file_size + 1);
    if (!content) {
        fclose(input);
        return -1;
    }

    size_t read_size = fread(content, 1, file_size, input);
    content[read_size] = '\0';
    fclose(input);

    /* Process content */
    char *processed = emoji_replacer_process(replacer, content);
    free(content);

    if (!processed) return -1;

    /* Write output file */
    FILE *output = fopen(output_path, "w");
    if (!output) {
        if (replacer->logger) {
            LOG_ERROR(replacer->logger, "EMOJI", "Cannot open output file: %s", output_path);
        }
        free(processed);
        return -1;
    }

    fwrite(processed, 1, strlen(processed), output);
    fclose(output);
    free(processed);

    if (replacer->logger) {
        LOG_INFO(replacer->logger, "EMOJI", "File processed successfully: %s -> %s",
                input_path, output_path);
    }

    return 0;
}

/* Scan file */
EmojiReplacement* emoji_replacer_scan_file(EmojiReplacer *replacer, const char *file_path, int *count) {
    if (!replacer || !file_path || !count) return NULL;

    FILE *file = fopen(file_path, "r");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = (char*)malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(content, 1, file_size, file);
    content[read_size] = '\0';
    fclose(file);

    /* Count emojis */
    int emoji_count = 0;
    EmojiReplacement *found = NULL;

    for (size_t i = 0; i < read_size; ) {
        bool found_emoji = false;

        EmojiReplacement *current = replacer->replacements;
        while (current) {
            size_t emoji_len = strlen(current->emoji);
            if (i + emoji_len <= read_size &&
                strncmp(content + i, current->emoji, emoji_len) == 0) {
                emoji_count++;
                found_emoji = true;
                i += emoji_len;
                break;
            }
            current = current->next;
        }

        if (!found_emoji) i++;
    }

    free(content);
    *count = emoji_count;

    if (replacer->logger) {
        LOG_INFO(replacer->logger, "EMOJI", "Scanned %s: found %d emojis", file_path, emoji_count);
    }

    return found;
}

/* Statistics */
void emoji_replacer_print_stats(EmojiReplacer *replacer) {
    if (!replacer) return;

    if (replacer->logger) {
        LOG_INFO(replacer->logger, "EMOJI", "===== Emoji Replacement Statistics =====");
        LOG_INFO(replacer->logger, "EMOJI", "Total replacements: %d", replacer->total_replacements);

        EmojiReplacement *current = replacer->replacements;
        while (current) {
            if (current->count > 0) {
                LOG_INFO(replacer->logger, "EMOJI", "  %s -> %s: %d times",
                        current->emoji, current->placeholder, current->count);
            }
            current = current->next;
        }
    }
}

void emoji_replacer_reset_stats(EmojiReplacer *replacer) {
    if (!replacer) return;

    replacer->total_replacements = 0;

    EmojiReplacement *current = replacer->replacements;
    while (current) {
        current->count = 0;
        current = current->next;
    }
}
