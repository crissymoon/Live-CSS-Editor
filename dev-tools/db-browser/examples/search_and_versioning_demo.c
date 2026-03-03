/**
 * Search Helpers and Table Versioning - Example Usage
 * 
 * This file demonstrates how to use the advanced search helpers
 * for tree traversal and the table versioning system for schema management.
 */

#include "core/search_helpers.h"
#include "core/table_versioning.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * Example 1: Hierarchical Data with Tree Operations
 * 
 * Use case: Managing organizational structure, file systems, or category trees
 */
void example_tree_operations(sqlite3 *db) {
    printf("\n=== Tree Operations Example ===\n");
    
    /* Create sample hierarchical table */
    const char *create_categories = 
        "CREATE TABLE IF NOT EXISTS categories ("
        "  id INTEGER PRIMARY KEY,"
        "  parent_id INTEGER,"
        "  name TEXT NOT NULL"
        ");";
    
    sqlite3_exec(db, create_categories, NULL, NULL, NULL);
    
    /* Insert sample data */
    sqlite3_exec(db, "DELETE FROM categories", NULL, NULL, NULL);
    sqlite3_exec(db, 
        "INSERT INTO categories (id, parent_id, name) VALUES "
        "(1, NULL, 'Root'),"
        "(2, 1, 'Electronics'),"
        "(3, 1, 'Clothing'),"
        "(4, 2, 'Computers'),"
        "(5, 2, 'Phones'),"
        "(6, 4, 'Laptops'),"
        "(7, 4, 'Desktops'),"
        "(8, 3, 'Mens'),"
        "(9, 3, 'Womens');",
        NULL, NULL, NULL);
    
    /* Build tree structure */
    TreeResult *tree = search_build_tree(db, "categories", "id", "parent_id", "name");
    if (tree && tree->count > 0) {
        printf("Built tree with %d nodes\n", tree->count);
        
        for (int i = 0; i < tree->count; i++) {
            TreeNode *node = tree->nodes[i];
            printf("  %s (ID: %ld, Depth: %d, Children: %d)\n",
                   node->name, node->id, node->depth, node->child_count);
        }
    }
    
    /* Get descendants of Electronics category */
    TreeResult *descendants = search_get_descendants(db, "categories", "id", "parent_id", 2, 10);
    if (descendants) {
        printf("\nDescendants of Electronics (ID: 2): %d nodes\n", descendants->count);
        search_free_tree_result(descendants);
    }
    
    /* Get path to Laptops */
    char *path = search_get_node_path(db, "categories", "id", "parent_id", "name", 6, " > ");
    if (path) {
        printf("Path to Laptops: %s\n", path);
        free(path);
    }
    
    /* Get siblings of Computers */
    TreeResult *siblings = search_get_siblings(db, "categories", "id", "parent_id", 4);
    if (siblings) {
        printf("Siblings of Computers: %d nodes\n", siblings->count);
        search_free_tree_result(siblings);
    }
    
    search_free_tree_result(tree);
}

/**
 * Example 2: Table Versioning and Schema Management
 * 
 * Use case: Tracking database schema changes over time,
 * enabling rollbacks, and managing migrations
 */
void example_table_versioning(sqlite3 *db) {
    printf("\n=== Table Versioning Example ===\n");
    
    /* Initialize versioning system */
    if (!versioning_init(db)) {
        printf("Failed to initialize versioning system\n");
        return;
    }
    printf("Versioning system initialized\n");
    
    /* Create a sample table */
    sqlite3_exec(db, "DROP TABLE IF EXISTS products", NULL, NULL, NULL);
    const char *create_products_v1 = 
        "CREATE TABLE products ("
        "  id INTEGER PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  price REAL"
        ");";
    
    sqlite3_exec(db, create_products_v1, NULL, NULL, NULL);
    printf("Created 'products' table (version 1)\n");
    
    /* Enable versioning and create first snapshot */
    if (versioning_enable(db, "products", "Initial schema with id, name, price")) {
        printf("Versioning enabled for 'products' table\n");
        int current = versioning_get_current_version(db, "products");
        printf("Current version: %d\n", current);
    }
    
    /* Modify schema - add description column */
    sqlite3_exec(db, "ALTER TABLE products ADD COLUMN description TEXT", NULL, NULL, NULL);
    printf("\nModified schema: added 'description' column\n");
    
    /* Check if schema changed */
    if (versioning_has_schema_changed(db, "products")) {
        printf("Schema change detected\n");
        
        /* Create new version snapshot */
        long version_id = versioning_create_snapshot(db, "products",
                                                     "Added description column",
                                                     "admin");
        if (version_id > 0) {
            printf("Created version snapshot (ID: %ld)\n", version_id);
            int current = versioning_get_current_version(db, "products");
            printf("Current version: %d\n", current);
        }
    }
    
    /* Modify schema again - add stock column */
    sqlite3_exec(db, "ALTER TABLE products ADD COLUMN stock INTEGER DEFAULT 0", NULL, NULL, NULL);
    printf("\nModified schema: added 'stock' column\n");
    
    if (versioning_has_schema_changed(db, "products")) {
        long version_id = versioning_create_snapshot(db, "products",
                                                     "Added stock column for inventory tracking",
                                                     "admin");
        if (version_id > 0) {
            printf("Created version snapshot (ID: %ld)\n", version_id);
        }
    }
    
    /* Get version history */
    VersionHistory *history = versioning_get_history(db, "products");
    if (history) {
        printf("\nVersion History for 'products' table:\n");
        for (int i = 0; i < history->count; i++) {
            SchemaVersion *v = history->versions[i];
            printf("  Version %d (ID: %ld)\n", v->version_number, v->version_id);
            printf("    Description: %s\n", v->change_description);
            printf("    Created by: %s\n", v->created_by);
            printf("    Schema: %.80s...\n", v->schema_sql);
        }
        versioning_free_history(history);
    }
    
    /* Get specific version */
    SchemaVersion *v1 = versioning_get_version(db, "products", 1);
    if (v1) {
        printf("\nVersion 1 details:\n");
        printf("  Description: %s\n", v1->change_description);
        printf("  Schema: %s\n", v1->schema_sql);
        versioning_free_version(v1);
    }
}

/**
 * Example 3: Index Optimization Check
 */
void example_index_optimization(sqlite3 *db) {
    printf("\n=== Index Optimization Example ===\n");
    
    /* Check if index exists on products.name */
    bool has_idx = search_has_index(db, "products", "name");
    printf("Index on products.name: %s\n", has_idx ? "EXISTS" : "MISSING");
    
    if (!has_idx) {
        printf("Recommendation: CREATE INDEX idx_products_name ON products(name);\n");
    }
}

/**
 * Main demonstration
 */
int main(void) {
    sqlite3 *db;
    
    /* Open in-memory database for demo */
    if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    printf("=================================================\n");
    printf("Search Helpers & Table Versioning Demo\n");
    printf("=================================================\n");
    
    /* Run examples */
    example_tree_operations(db);
    example_table_versioning(db);
    example_index_optimization(db);
    
    printf("\n=================================================\n");
    printf("Demo completed successfully\n");
    printf("=================================================\n");
    
    sqlite3_close(db);
    return 0;
}

/**
 * Real-world integration examples:
 * 
 * 1. In query_callbacks.c:
 *    - Use search_build_join_query() to optimize complex queries
 *    - Use search_suggest_indexes() to recommend performance improvements
 * 
 * 2. In table_callbacks.c:
 *    - Call versioning_create_snapshot() before any ALTER TABLE operations
 *    - Check versioning_has_schema_changed() to detect manual schema changes
 * 
 * 3. In db_manager.c:
 *    - Use search_has_index() before executing queries on large tables
 *    - Integrate tree operations for hierarchical table views
 * 
 * 4. New features to add:
 *    - Schema migration UI showing version history
 *    - Tree view widget for hierarchical data browsing
 *    - Automatic index suggestions based on query patterns
 *    - Schema rollback capability with preview
 */
