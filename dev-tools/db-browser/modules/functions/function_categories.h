#ifndef FUNCTION_CATEGORIES_H
#define FUNCTION_CATEGORIES_H

typedef struct {
    const char *name;
    const char *description;
    const char *sql_template;
} SQLFunction;

typedef struct {
    const char *category_name;
    const SQLFunction *functions;
    int function_count;
} FunctionCategory;

// Category definitions
extern const FunctionCategory aggregate_category;
extern const FunctionCategory statistical_category;
extern const FunctionCategory mathematical_category;
extern const FunctionCategory datetime_category;
extern const FunctionCategory string_category;
extern const FunctionCategory window_category;
extern const FunctionCategory business_category;

// Get all categories
const FunctionCategory* get_all_categories(int *count);

#endif
