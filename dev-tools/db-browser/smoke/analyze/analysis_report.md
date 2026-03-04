# Database Browser Code Analysis Report

**Generated:** 2026-03-03T22:56:20.724816  
**Analyzer Version:** 1.0.0  
**Project Root:** /Users/mac/Documents/live-css/dev-tools/db-browser


## Executive Summary

**Overall Score:** 87.4/100  
**Risk Level:** LOW

### Score Breakdown

| Category | Score | Status |
|----------|-------|--------|
| Complexity | 92.8/100 | Excellent |
| Scalability | 82.5/100 | Excellent |
| Dependencies | 100.0/100 | Excellent |
| Performance | 96.0/100 | Excellent |
| Technical Debt | 40.0/100 | Needs Improvement |
| Memory Management | 89.5/100 | Excellent |


## Project Statistics

### Files Overview

| Language | Files |
|----------|-------|
| C | 36 |
| Header | 29 |
| Python | 12 |
| Shell | 6 |
| Markdown | 11 |

### Code Metrics

- **Total Lines:** 18,056
- **Code Lines:** 13,323
- **Comment Lines:** 1,662
- **Blank Lines:** 3,071
- **Comment Ratio:** 12.5%


## Code Complexity Analysis

### Metrics

- **Total Functions:** 288
- **Average Complexity:** 4.33
- **Maximum Complexity:** 20
- **High Complexity Functions:** 8
- **Average Function Length:** 20.4 lines
- **Long Functions (>100 lines):** 9

### High Complexity Functions

- **db_manager_create_table** in `core/db_manager.c` (complexity: 17)
- **db_manager_execute_query** in `core/db_manager.c` (complexity: 20)
- **versioning_parse_columns** in `core/table_versioning.c` (complexity: 16)
- **db_transfer_list_databases** in `core/db_transfer.c` (complexity: 19)
- **extract_query_type** in `modules/data_protection.c` (complexity: 20)
- **analyze_query_risk** in `modules/data_protection.c` (complexity: 16)
- **on_drop_table** in `modules/table_callbacks.c` (complexity: 20)
- **on_execute_query** in `modules/query_callbacks.c` (complexity: 18)

## Scalability Analysis

### Architecture

- **Modular Structure:** Yes
- **Layered Design:** Yes
- **Module Count:** 16


### Database Configuration

- **Databases Checked:** 20
- **WAL Mode Enabled:** 20
- **Non-WAL Databases:** 0


### Patterns Detected

| Pattern | Count |
|---------|-------|
| Global State Usage | 176 |
| Singleton Pattern | 0 |
| Memory Pooling | 0 |
| Lazy Loading | 4 |
| Caching | 2 |
| Async Patterns | 0 |


### Issues Found (7)

- **Unbounded Allocation:** 5
- **N Plus One Query:** 1
- **Missing Pagination:** 1

## Dependency Analysis

### Metrics

- **Total Files:** 64
- **Average Dependencies:** 1.5
- **Maximum Dependencies:** 12
- **Average Dependents:** 0.1
- **Maximum Dependents:** 2
- **Tight Coupling Count:** 0


## Performance Analysis

### Issues Found

**Total Issues:** 10


| Severity | Count |
|----------|-------|
| MEDIUM | 1 |
| LOW | 9 |

### Performance Hotspots

- `core/search_helpers.c` (4 issues, severity score: 5)
- `core/db_manager.c` (1 issues, severity score: 1)
- `core/table_versioning.c` (1 issues, severity score: 1)
- `core/db_transfer.c` (1 issues, severity score: 1)
- `modules/trash_manager.c` (1 issues, severity score: 1)

## Technical Debt Analysis

### Summary

**Total Estimated Debt:** 198.5 hours (24.8 days)


### Debt by Type

| Type | Hours |
|------|-------|
| Long Function | 87.0 |
| Missing Documentation | 83.5 |
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

**Memory Score:** 89.5/100  
**Total Allocations:** 65  
**Total Frees:** 209  
**Potential Leaks:** 0  
**Unguarded Allocations:** 4  
**Buffer Risks:** 2


### Good Patterns Detected

- Arena Allocation
- Reference Counting
- Cleanup Functions

### Issues by Type

| Issue Type | Count |
|------------|-------|
| Double Free Risk | 31 |
| Fixed Buffer Overuse | 7 |
| Unguarded Allocation | 4 |
| Unsafe Realloc | 3 |
| Buffer Overflow Risk | 2 |
| Large Stack Allocation | 1 |

### High Severity Issues (33)

- **core/search_helpers.c** (Line 385): Using unsafe function strcpy() - use safe alternative
- **core/search_helpers.c** (Line 391): Using unsafe function strcpy() - use safe alternative
- **core/db_crypto.c** (Line 368): Variable plaintext may be freed multiple times in function crypto_encrypt_file
- **core/db_crypto.c** (Line 396): Variable ciphertext may be freed multiple times in function crypto_encrypt_file
- **core/db_crypto.c** (Line 405): Variable ciphertext may be freed multiple times in function crypto_encrypt_file
- **core/db_crypto.c** (Line 411): Variable ciphertext may be freed multiple times in function crypto_encrypt_file
- **core/db_crypto.c** (Line 497): Variable ciphertext may be freed multiple times in function crypto_decrypt_file
- **core/db_crypto.c** (Line 518): Variable plaintext may be freed multiple times in function crypto_decrypt_file
- **core/db_crypto.c** (Line 524): Variable plaintext may be freed multiple times in function crypto_decrypt_file
- **core/db_transfer.c** (Line 250): Variable dest_path may be freed multiple times in function db_transfer_import

### Memory Recommendations

- Replace 2 unsafe string functions with safe alternatives (strncpy, snprintf, etc.)
- Consider implementing memory pools for frequent allocations to reduce fragmentation
- Review 31 potential double-free issues and set pointers to NULL after freeing
- Move 1 large stack allocations to heap to prevent stack overflow

## Recommendations

1. High usage of global state (176 instances). Consider encapsulating state in structures passed to functions.
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
