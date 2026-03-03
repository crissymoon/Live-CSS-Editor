#include "function_categories.h"
#include <stddef.h>

// All categories array
static const FunctionCategory* all_categories[] = {
    &aggregate_category,
    &statistical_category,
    &mathematical_category,
    &datetime_category,
    &string_category,
    &window_category,
    &business_category
};

const FunctionCategory* get_all_categories(int *count) {
    if (count) {
        *count = sizeof(all_categories) / sizeof(all_categories[0]);
    }
    return *all_categories;
}
