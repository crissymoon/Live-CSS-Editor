# Database Browser Code Analysis Report

**Generated:** 2026-03-03T15:00:54.770499  
**Analyzer Version:** 1.0.0  
**Project Root:** /Users/mac/Documents/live-css/dev-tools/db-browser


## Executive Summary

**Overall Score:** 53.4/100  
**Risk Level:** 🔶 HIGH

### Score Breakdown

| Category | Score | Status |
|----------|-------|--------|
| Complexity | 100.0/100 | Excellent ✅ |
| Scalability | 50.0/100 | Needs Improvement ⚠️ |
| Dependencies | 100.0/100 | Excellent ✅ |
| Performance | 36.0/100 | Critical 🔴 |
| Technical Debt | 20.0/100 | Critical 🔴 |
| Memory Management | 0.0/100 | Critical 🔴 |


## Project Statistics

### Files Overview

| Language | Files |
|----------|-------|
| C | 31 |
| Header | 23 |
| Python | 9 |
| Shell | 5 |
| Markdown | 8 |

### Code Metrics

- **Total Lines:** 14,357
- **Code Lines:** 10,673
- **Comment Lines:** 1,271
- **Blank Lines:** 2,413
- **Comment Ratio:** 11.9%


## Code Complexity Analysis

### Metrics

- **Total Functions:** 255
- **Average Complexity:** 4.45
- **Maximum Complexity:** 34
- **High Complexity Functions:** 10
- **Average Function Length:** 25.1 lines
- **Long Functions (>100 lines):** 12

### High Complexity Functions

- **db_manager_create_table** in `core/db_manager.c` (complexity: 17)
- **db_transfer_list_databases** in `core/db_transfer.c` (complexity: 19)
- **on_import_csv** in `legacy/main.c` (complexity: 27)
- **on_add_row_clicked** in `modules/row_operations.c` (complexity: 16)
- **extract_query_type** in `modules/data_protection.c` (complexity: 20)
- **analyze_query_risk** in `modules/data_protection.c` (complexity: 16)
- **on_new_table** in `modules/table_callbacks.c` (complexity: 16)
- **on_drop_table** in `modules/table_callbacks.c` (complexity: 20)
- **on_sql_query_builder** in `modules/query_callbacks.c` (complexity: 34)
- **on_execute_query** in `modules/query_callbacks.c` (complexity: 18)

## Scalability Analysis

### Architecture

- **Modular Structure:** Yes ✅
- **Layered Design:** Yes ✅
- **Module Count:** 16

### Patterns Detected

| Pattern | Count |
|---------|-------|
| Global State Usage | 145 |
| Singleton Pattern | 0 |
| Memory Pooling | 0 |
| Lazy Loading | 3 |
| Caching | 0 |
| Async Patterns | 0 |


### Issues Found (17)

- **Missing Pagination:** 8
- **Unbounded Allocation:** 6
- **Large Static Buffer:** 3

## Dependency Analysis

### Metrics

- **Total Files:** 54
- **Average Dependencies:** 1.6
- **Maximum Dependencies:** 15
- **Average Dependents:** 0.3
- **Maximum Dependents:** 2
- **Tight Coupling Count:** 0


## Performance Analysis

### Issues Found

**Total Issues:** 8


| Severity | Count |
|----------|-------|
| MEDIUM | 4 |
| LOW | 4 |

### Performance Hotspots

- `modules/table_callbacks.c` (3 issues, severity score: 4)
- `core/db_transfer.c` (2 issues, severity score: 3)
- `modules/trash_manager.c` (2 issues, severity score: 3)
- `core/db_manager.c` (1 issues, severity score: 2)

## Technical Debt Analysis

### Summary

**Total Estimated Debt:** 173.5 hours (21.7 days)


### Debt by Type

| Type | Hours |
|------|-------|
| Long Function | 78.0 |
| Missing Documentation | 77.5 |
| Magic Numbers | 12.0 |
| Todo | 4.0 |
| Deprecated Function | 2.0 |

### Debt Hotspots

- `legacy/main.c` (52.5 hours, 4 items)
- `ui/tooltips.c` (18.5 hours, 2 items)
- `modules/ui_panels.c` (18.0 hours, 2 items)
- `modules/db_callbacks.c` (16.0 hours, 3 items)
- `databases/example_usage.c` (12.5 hours, 2 items)

## Memory Management Analysis

### Summary

**Memory Score:** 0.0/100  
**Total Allocations:** 0  
**Total Frees:** 0  
**Potential Leaks:** 0  
**Unguarded Allocations:** 17  
**Buffer Risks:** 3


### Good Patterns Detected

- Reference Counting
- Cleanup Functions

### Issues by Type

| Issue Type | Count |
|------------|-------|
| Double Free Risk | 52 |
| Unguarded Allocation | 17 |
| Fixed Buffer Overuse | 6 |
| Large Stack Allocation | 6 |
| Buffer Overflow Risk | 3 |
| Unsafe Realloc | 3 |

### High Severity Issues (55)

- **core/db_manager.c** (Line 512): Variable info may be freed multiple times
- **core/db_crypto.c** (Line 106): Variable ctx may be freed multiple times
- **core/db_crypto.c** (Line 115): Variable ctx may be freed multiple times
- **core/db_crypto.c** (Line 126): Variable ctx may be freed multiple times
- **core/db_crypto.c** (Line 368): Variable plaintext may be freed multiple times
- **core/db_crypto.c** (Line 396): Variable ciphertext may be freed multiple times
- **core/db_crypto.c** (Line 405): Variable ciphertext may be freed multiple times
- **core/db_crypto.c** (Line 411): Variable ciphertext may be freed multiple times
- **core/db_crypto.c** (Line 482): Variable ciphertext may be freed multiple times
- **core/db_crypto.c** (Line 497): Variable ciphertext may be freed multiple times

### Memory Recommendations

- Add NULL checks for 17 unguarded memory allocations
- Replace 3 unsafe string functions with safe alternatives (strncpy, snprintf, etc.)
- Review 52 potential double-free issues and set pointers to NULL after freeing
- Move 6 large stack allocations to heap to prevent stack overflow

## Recommendations

1. High usage of global state (145 instances). Consider encapsulating state in structures passed to functions.
2. No caching patterns detected. Consider implementing caching for frequently accessed data.
3. No asynchronous patterns detected. Consider async operations for I/O-bound tasks.
4. No memory pooling detected. Consider implementing memory pools for frequently allocated objects.
5. Add LIMIT clauses to queries to prevent loading excessive data.

---

## About This Report

This automated analysis report was generated to help identify potential issues in code complexity, 
scalability, dependencies, performance, and technical debt. The scores and recommendations are based 
on static analysis and should be reviewed by developers familiar with the codebase.

**Note:** This is an automated analysis. Human judgment is required to determine which issues are 
truly important for your specific use case.
