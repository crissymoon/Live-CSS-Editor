#include "tooltips.h"
#include <stdlib.h>
#include <string.h>

/* Create tooltip manager */
TooltipManager* tooltip_manager_create() {
    TooltipManager *mgr = g_malloc0(sizeof(TooltipManager));
    mgr->tooltips = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    mgr->tutorial_mode = false;
    mgr->current_step = 0;
    return mgr;
}

/* Destroy tooltip manager */
void tooltip_manager_destroy(TooltipManager *mgr) {
    if (!mgr) return;
    g_hash_table_destroy(mgr->tooltips);
    g_free(mgr);
}

/* Add tooltip */
void tooltip_manager_add_tooltip(TooltipManager *mgr, const char *element_id,
                                const char *title, const char *description,
                                const char *example, TooltipLevel level) {
    if (!mgr || !element_id) return;

    gchar *tooltip_text = g_strdup_printf(
        "<b>%s</b>\n\n%s%s%s",
        title,
        description,
        example ? "\n\n<i>Example:</i>\n" : "",
        example ? example : ""
    );

    g_hash_table_insert(mgr->tooltips, g_strdup(element_id), tooltip_text);
}

/* Apply tooltip to widget */
void tooltip_manager_apply_to_widget(TooltipManager *mgr, GtkWidget *widget,
                                    const char *element_id) {
    if (!mgr || !widget || !element_id) return;

    const gchar *tooltip = g_hash_table_lookup(mgr->tooltips, element_id);
    if (tooltip) {
        gtk_widget_set_tooltip_markup(widget, tooltip);
    }
}

/* Register all tooltips */
void tooltip_manager_register_all(TooltipManager *mgr) {
    if (!mgr) return;

    tooltip_register_table_operations(mgr);
    tooltip_register_query_operations(mgr);
    tooltip_register_data_operations(mgr);
    tooltip_register_import_export(mgr);
    tooltip_register_sql_help(mgr);
}

/* Register table operation tooltips */
void tooltip_register_table_operations(TooltipManager *mgr) {
    tooltip_manager_add_tooltip(mgr, "btn_new_table",
        "Create New Table",
        "A table stores related data in rows and columns, like a spreadsheet. "
        "Each column has a specific data type (TEXT, INTEGER, etc.).",
        "CREATE TABLE students (\n"
        "  id INTEGER PRIMARY KEY,\n"
        "  name TEXT NOT NULL,\n"
        "  age INTEGER\n"
        ");",
        TOOLTIP_BASIC
    );

    tooltip_manager_add_tooltip(mgr, "btn_drop_table",
        "Delete Table [WARNING]",
        "Permanently removes a table and ALL its data. This cannot be undone! "
        "Always backup your database before dropping tables.",
        "DROP TABLE students;",
        TOOLTIP_WARNING
    );

    tooltip_manager_add_tooltip(mgr, "btn_rename_table",
        "Rename Table",
        "Changes the table name without affecting its data or structure. "
        "Useful for better organization or fixing typos.",
        "ALTER TABLE students RENAME TO enrolled_students;",
        TOOLTIP_BASIC
    );

    tooltip_manager_add_tooltip(mgr, "btn_add_column",
        "Add Column",
        "Adds a new column to an existing table. You can specify a default value "
        "for existing rows. Cannot add PRIMARY KEY columns to existing tables.",
        "ALTER TABLE students ADD COLUMN email TEXT;",
        TOOLTIP_INTERMEDIATE
    );

    tooltip_manager_add_tooltip(mgr, "btn_view_schema",
        "View Table Schema",
        "Shows the table's structure: column names, data types, constraints "
        "(PRIMARY KEY, NOT NULL, UNIQUE), and default values.",
        "PRAGMA table_info(students);",
        TOOLTIP_BASIC
    );
}

/* Register query operation tooltips */
void tooltip_register_query_operations(TooltipManager *mgr) {
    tooltip_manager_add_tooltip(mgr, "btn_execute_query",
        "Execute SQL Query",
        "Runs your SQL command against the database. SELECT queries show results, "
        "while INSERT/UPDATE/DELETE modify data. Use [CTRL]+[ENTER] as shortcut.",
        "SELECT * FROM students WHERE age > 18;",
        TOOLTIP_BASIC
    );

    tooltip_manager_add_tooltip(mgr, "btn_explain_query",
        "Explain Query Plan",
        "Shows how SQLite will execute your query. Useful for optimizing slow queries "
        "by identifying missing indexes or inefficient operations.",
        "EXPLAIN QUERY PLAN SELECT * FROM students WHERE name = 'John';",
        TOOLTIP_ADVANCED
    );

    tooltip_manager_add_tooltip(mgr, "btn_format_query",
        "Format SQL",
        "Automatically formats your SQL query for better readability. "
        "Adds proper indentation and line breaks.",
        NULL,
        TOOLTIP_TIP
    );

    tooltip_manager_add_tooltip(mgr, "btn_query_history",
        "Query History",
        "View and reuse previously executed queries. Click any query to load it "
        "into the editor. Useful for iterating on complex queries.",
        NULL,
        TOOLTIP_TIP
    );

    tooltip_manager_add_tooltip(mgr, "query_editor",
        "SQL Query Editor",
        "Write SQL commands here. Supports syntax highlighting and autocomplete. "
        "Press [CTRL]+[SPACE] for suggestions based on your table/column names.",
        "-- Select all students\n"
        "SELECT id, name, age\n"
        "FROM students\n"
        "WHERE age >= 18\n"
        "ORDER BY name;",
        TOOLTIP_BASIC
    );
}

/* Register data operation tooltips */
void tooltip_register_data_operations(TooltipManager *mgr) {
    tooltip_manager_add_tooltip(mgr, "btn_insert_row",
        "Insert New Row",
        "Adds a new record to the table. You'll enter values for each column. "
        "PRIMARY KEY columns usually auto-increment and can be left empty.",
        "INSERT INTO students (name, age) VALUES ('John Doe', 20);",
        TOOLTIP_BASIC
    );

    tooltip_manager_add_tooltip(mgr, "btn_edit_row",
        "Edit Selected Row",
        "Modifies the values in the selected row. Double-click any cell for "
        "quick editing. Changes are saved when you press [ENTER].",
        "UPDATE students SET age = 21 WHERE id = 5;",
        TOOLTIP_BASIC
    );

    tooltip_manager_add_tooltip(mgr, "btn_delete_row",
        "Delete Selected Rows [WARNING]",
        "Permanently removes selected rows. This cannot be undone unless you have "
        "a backup. Select multiple rows with [CTRL]+[CLICK] or [SHIFT]+[CLICK].",
        "DELETE FROM students WHERE id = 5;",
        TOOLTIP_WARNING
    );

    tooltip_manager_add_tooltip(mgr, "btn_filter_data",
        "Filter Rows",
        "Shows only rows matching your criteria. Similar to Excel filters. "
        "Use operators: = (equal), > (greater), < (less), LIKE (pattern matching).",
        "WHERE age > 18 AND name LIKE 'J%'",
        TOOLTIP_INTERMEDIATE
    );

    tooltip_manager_add_tooltip(mgr, "btn_sort_data",
        "Sort Data",
        "Arranges rows by one or more columns. Choose ascending (A->Z, 0->9) or "
        "descending (Z->A, 9->0). Click column headers for quick sorting.",
        "ORDER BY name ASC, age DESC",
        TOOLTIP_BASIC
    );

    tooltip_manager_add_tooltip(mgr, "data_grid",
        "Data Grid",
        "Shows table data in spreadsheet format. Click any cell to edit (if not read-only). "
        "Right-click rows for context menu with copy/paste/delete options.",
        NULL,
        TOOLTIP_BASIC
    );
}

/* Register import/export tooltips */
void tooltip_register_import_export(TooltipManager *mgr) {
    tooltip_manager_add_tooltip(mgr, "btn_import_csv",
        "Import CSV File",
        "Loads data from comma-separated values file. First row can be column names. "
        "You'll map CSV columns to table columns during import.",
        "students.csv:\nname,age\nJohn,20\nJane,22",
        TOOLTIP_BASIC
    );

    tooltip_manager_add_tooltip(mgr, "btn_import_json",
        "Import JSON File",
        "Loads data from JSON format. Supports both array of objects and nested structures. "
        "Useful for importing from web APIs or modern applications.",
        "[\n  {\"name\": \"John\", \"age\": 20},\n  {\"name\": \"Jane\", \"age\": 22}\n]",
        TOOLTIP_INTERMEDIATE
    );

    tooltip_manager_add_tooltip(mgr, "btn_import_sql",
        "Import SQL Dump",
        "Executes SQL commands from a file. Can create tables and insert data. "
        "Useful for restoring backups or migrating data between databases.",
        "CREATE TABLE students (...);\nINSERT INTO students VALUES (...);",
        TOOLTIP_INTERMEDIATE
    );

    tooltip_manager_add_tooltip(mgr, "btn_export_csv",
        "Export to CSV",
        "Saves current table or query results as CSV file. Can be opened in Excel, "
        "Google Sheets, or any text editor. Choose delimiter (comma, tab, semicolon).",
        NULL,
        TOOLTIP_BASIC
    );

    tooltip_manager_add_tooltip(mgr, "btn_export_json",
        "Export to JSON",
        "Saves data in JSON format. Great for web applications or sharing data "
        "with JavaScript programs. Choose between compact or pretty-printed format.",
        NULL,
        TOOLTIP_INTERMEDIATE
    );

    tooltip_manager_add_tooltip(mgr, "btn_export_sql",
        "Export as SQL Dump",
        "Creates SQL script that recreates table structure and data. Perfect for "
        "backups, version control, or moving data to another database.",
        NULL,
        TOOLTIP_BASIC
    );

    tooltip_manager_add_tooltip(mgr, "btn_backup_db",
        "Backup Database",
        "Creates complete copy of database file. ALWAYS backup before making major "
        "changes! Backups are saved with timestamp in filename.",
        NULL,
        TOOLTIP_WARNING
    );
}

/* Register SQL help tooltips */
void tooltip_register_sql_help(TooltipManager *mgr) {
    tooltip_manager_add_tooltip(mgr, "help_select",
        "SELECT - Query Data",
        "Retrieves data from tables. Use * for all columns or specify column names. "
        "Combine with WHERE, ORDER BY, LIMIT for filtering and sorting.",
        "SELECT name, age FROM students WHERE age > 18 ORDER BY name LIMIT 10;",
        TOOLTIP_BASIC
    );

    tooltip_manager_add_tooltip(mgr, "help_insert",
        "INSERT - Add Data",
        "Adds new rows to a table. Specify column names and values. "
        "Can insert multiple rows in one statement.",
        "INSERT INTO students (name, age) VALUES ('John', 20), ('Jane', 22);",
        TOOLTIP_BASIC
    );

    tooltip_manager_add_tooltip(mgr, "help_update",
        "UPDATE - Modify Data [WARNING]",
        "Changes existing data. ALWAYS use WHERE clause to avoid updating all rows! "
        "Without WHERE, every row in the table will be modified.",
        "UPDATE students SET age = 21 WHERE name = 'John';",
        TOOLTIP_WARNING
    );

    tooltip_manager_add_tooltip(mgr, "help_delete",
        "DELETE - Remove Data [WARNING]",
        "Permanently removes rows. ALWAYS use WHERE clause! "
        "DELETE without WHERE removes ALL rows (but keeps table structure).",
        "DELETE FROM students WHERE age < 18;",
        TOOLTIP_WARNING
    );

    tooltip_manager_add_tooltip(mgr, "help_join",
        "JOIN - Combine Tables",
        "Merges data from multiple tables based on related columns. "
        "INNER JOIN shows matching rows, LEFT JOIN shows all from left table.",
        "SELECT s.name, c.course_name\n"
        "FROM students s\n"
        "INNER JOIN courses c ON s.id = c.student_id;",
        TOOLTIP_ADVANCED
    );

    tooltip_manager_add_tooltip(mgr, "help_index",
        "INDEX - Speed Up Queries",
        "Creates index to make searches faster. Like a book's index. "
        "Add indexes to columns frequently used in WHERE clauses.",
        "CREATE INDEX idx_student_name ON students(name);",
        TOOLTIP_ADVANCED
    );
}

/* Enable tutorial mode */
void tooltip_manager_enable_tutorial(TooltipManager *mgr) {
    if (mgr) {
        mgr->tutorial_mode = true;
        mgr->current_step = 0;
    }
}

/* Disable tutorial mode */
void tooltip_manager_disable_tutorial(TooltipManager *mgr) {
    if (mgr) {
        mgr->tutorial_mode = false;
    }
}

/* Check if tutorial is active */
bool tooltip_manager_is_tutorial_active(TooltipManager *mgr) {
    return mgr && mgr->tutorial_mode;
}

/* Show custom tooltip dialog */
void tooltip_manager_show_custom(TooltipManager *mgr, GtkWidget *parent,
                                const char *title, const char *message,
                                TooltipLevel level) {
    GtkWidget *dialog;
    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;

    const char *level_icon;
    switch (level) {
        case TOOLTIP_WARNING:
            level_icon = "[WARNING]";
            break;
        case TOOLTIP_TIP:
            level_icon = "[TIP]";
            break;
        case TOOLTIP_EXAMPLE:
            level_icon = "[EXAMPLE]";
            break;
        default:
            level_icon = "[INFO]";
    }

    gchar *full_message = g_strdup_printf("%s %s", level_icon, message);

    dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                    flags,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_OK,
                                    "%s", title);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                             "%s", full_message);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    g_free(full_message);
}
