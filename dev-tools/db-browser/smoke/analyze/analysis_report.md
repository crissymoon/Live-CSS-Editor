# Database Browser Code Analysis Report

**Generated:** 2026-03-03T15:57:30.732554  
**Analyzer Version:** 1.0.0  
**Project Root:** /Users/mac/Documents/live-css/dev-tools/db-browser


## Executive Summary

**Overall Score:** 53.8/100  
**Risk Level:** CRITICAL

### Score Breakdown

| Category | Score | Status |
|----------|-------|--------|
| Complexity | 100.0/100 | Excellent |
| Scalability | 50.0/100 | Needs Improvement |
| Dependencies | 100.0/100 | Excellent |
| Performance | 20.0/100 | Critical |
| Technical Debt | 20.0/100 | Critical |
| Memory Management | 20.0/100 | Critical |


## Project Statistics

### Files Overview

| Language | Files |
|----------|-------|
| C | 35 |
| Header | 26 |
| Python | 11 |
| Shell | 6 |
| Markdown | 11 |

### Code Metrics

- **Total Lines:** 17,555
- **Code Lines:** 12,954
- **Comment Lines:** 1,599
- **Blank Lines:** 3,002
- **Comment Ratio:** 12.3%


## Code Complexity Analysis

### Metrics

- **Total Functions:** 313
- **Average Complexity:** 4.55
- **Maximum Complexity:** 34
- **High Complexity Functions:** 12
- **Average Function Length:** 24.0 lines
- **Long Functions (>100 lines):** 12

### High Complexity Functions

- **db_manager_create_table** in `core/db_manager.c` (complexity: 17)
- **db_manager_execute_query** in `core/db_manager.c` (complexity: 20)
- **versioning_parse_columns** in `core/table_versioning.c` (complexity: 16)
- **db_transfer_list_databases** in `core/db_transfer.c` (complexity: 19)
- **on_import_csv** in `legacy/main.c` (complexity: 27)
- **on_add_row_clicked** in `modules/row_operations.c` (complexity: 20)
- **extract_query_type** in `modules/data_protection.c` (complexity: 20)
- **analyze_query_risk** in `modules/data_protection.c` (complexity: 16)
- **on_new_table** in `modules/table_callbacks.c` (complexity: 16)
- **on_drop_table** in `modules/table_callbacks.c` (complexity: 20)

## Scalability Analysis

### Architecture

- **Modular Structure:** Yes
- **Layered Design:** Yes
- **Module Count:** 16


### Database Configuration

- **Databases Checked:** 19
- **WAL Mode Enabled:** 19
- **Non-WAL Databases:** 0


### Patterns Detected

| Pattern | Count |
|---------|-------|
| Global State Usage | 174 |
| Singleton Pattern | 0 |
| Memory Pooling | 0 |
| Lazy Loading | 4 |
| Caching | 1 |
| Async Patterns | 0 |


### Issues Found (19)

- **Missing Pagination:** 10
- **Unbounded Allocation:** 7
- **N Plus One Query:** 1
- **Large Static Buffer:** 1

## Dependency Analysis

### Metrics

- **Total Files:** 61
- **Average Dependencies:** 1.5
- **Maximum Dependencies:** 15
- **Average Dependents:** 0.3
- **Maximum Dependents:** 2
- **Tight Coupling Count:** 0


## Performance Analysis

### Issues Found

**Total Issues:** 24


| Severity | Count |
|----------|-------|
| MEDIUM | 8 |
| LOW | 16 |

### Performance Hotspots

- `core/search_helpers.c` (4 issues, severity score: 6)
- `core/db_transfer.c` (3 issues, severity score: 4)
- `legacy/main.c` (3 issues, severity score: 4)
- `modules/table_callbacks.c` (3 issues, severity score: 4)
- `core/db_manager.c` (2 issues, severity score: 3)

## Technical Debt Analysis

### Summary

**Total Estimated Debt:** 194.5 hours (24.3 days)


### Debt by Type

| Type | Hours |
|------|-------|
| Long Function | 84.0 |
| Missing Documentation | 82.5 |
| Magic Numbers | 18.0 |
| Code Duplication | 4.0 |
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

**Memory Score:** 20.0/100  
**Total Allocations:** 63  
**Total Frees:** 205  
**Potential Leaks:** 0  
**Unguarded Allocations:** 4  
**Buffer Risks:** 2


### Good Patterns Detected

- Reference Counting
- Cleanup Functions

### Issues by Type

| Issue Type | Count |
|------------|-------|
| Double Free Risk | 34 |
| Fixed Buffer Overuse | 7 |
| Unguarded Allocation | 4 |
| Unsafe Realloc | 3 |
| Buffer Overflow Risk | 2 |
| Large Stack Allocation | 1 |

### High Severity Issues (36)

- **core/search_helpers.c** (Line 385): Using unsafe function strcpy() - use safe alternative
- **core/search_helpers.c** (Line 391): Using unsafe function strcpy() - use safe alternative
- **core/db_crypto.c** (Line 368): Variable plaintext may be freed multiple times in function crypto_encrypt_file
- **core/db_crypto.c** (Line 396): Variable ciphertext may be freed multiple times in function crypto_encrypt_file
- **core/db_crypto.c** (Line 405): Variable ciphertext may be freed multiple times in function crypto_encrypt_file
- **core/db_crypto.c** (Line 411): Variable ciphertext may be freed multiple times in function crypto_encrypt_file
- **core/db_crypto.c** (Line 497): Variable ciphertext may be freed multiple times in function crypto_decrypt_file
- **core/db_crypto.c** (Line 518): Variable plaintext may be freed multiple times in function crypto_decrypt_file
- **core/db_crypto.c** (Line 524): Variable plaintext may be freed multiple times in function crypto_decrypt_file
- **core/db_transfer.c** (Line 218): Variable dest_path may be freed multiple times in function db_transfer_import

### Memory Recommendations

- Replace 2 unsafe string functions with safe alternatives (strncpy, snprintf, etc.)
- Consider implementing memory pools for frequent allocations to reduce fragmentation
- Review 34 potential double-free issues and set pointers to NULL after freeing
- Move 1 large stack allocations to heap to prevent stack overflow

## Recommendations

1. High usage of global state (174 instances). Consider encapsulating state in structures passed to functions.
2. No asynchronous patterns detected. Consider async operations for I/O-bound tasks.
3. No memory pooling detected. Consider implementing memory pools for frequently allocated objects.
4. N+1 query patterns detected. Use batch queries or JOIN operations to improve performance.
5. Add LIMIT clauses to queries to prevent loading excessive data.

---

## About This Report

This automated analysis report was generated to help identify potential issues in code complexity, 
scalability, dependencies, performance, and technical debt. The scores and recommendations are based 
on static analysis and should be reviewed by developers familiar with the codebase.

**Note:** This is an automated analysis. Human judgment is required to determine which issues are 
truly important for your specific use case.
