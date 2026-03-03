#include "auth.h"
#include <string.h>
#include <time.h>

// Session timeout in seconds (30 minutes)
#define SESSION_TIMEOUT (30 * 60)

// Development credentials (temporary)
typedef struct {
    const char *username;
    const char *password;
    UserRole role;
} DevUser;

static DevUser dev_users[] = {
    {"dev", "dev123", ROLE_ADMIN},
    {"admin", "admin123", ROLE_ADMIN},
    {"editor", "edit123", ROLE_EDITOR},
    {"viewer", "view123", ROLE_VIEWER},
    {NULL, NULL, ROLE_VIEWER}
};

void auth_init(void) {
    // Initialize authentication system
    // In production, this would load from a secure database
}

bool auth_verify_credentials(const char *username, const char *password, UserRole *role) {
    if (!username || !password) {
        return false;
    }

    // Check against dev users
    for (int i = 0; dev_users[i].username != NULL; i++) {
        if (strcmp(username, dev_users[i].username) == 0 &&
            strcmp(password, dev_users[i].password) == 0) {
            if (role) {
                *role = dev_users[i].role;
            }
            return true;
        }
    }

    return false;
}

bool auth_has_permission(AuthSession *session, UserRole required_role) {
    if (!session || !session->authenticated) {
        return false;
    }

    // Check session validity
    if (!auth_session_valid(session)) {
        return false;
    }

    // Admin has access to everything
    if (session->role == ROLE_ADMIN) {
        return true;
    }

    // Check role hierarchy
    return session->role >= required_role;
}

void auth_logout(AuthSession *session) {
    if (!session) {
        return;
    }

    session->authenticated = false;
    if (session->username) {
        g_free(session->username);
        session->username = NULL;
    }
    session->role = ROLE_VIEWER;
    session->login_time = 0;
}

const char* auth_get_role_name(UserRole role) {
    switch (role) {
        case ROLE_VIEWER: return "Viewer";
        case ROLE_EDITOR: return "Editor";
        case ROLE_ADMIN: return "Administrator";
        default: return "Unknown";
    }
}

bool auth_session_valid(AuthSession *session) {
    if (!session || !session->authenticated) {
        return false;
    }

    // Check timeout
    time_t now = time(NULL);
    if (now - session->login_time > SESSION_TIMEOUT) {
        return false;
    }

    return true;
}

// Login dialog callbacks
static void on_login_entry_activate(GtkEntry *entry, gpointer data) {
    GtkDialog *dialog = GTK_DIALOG(data);
    gtk_dialog_response(dialog, GTK_RESPONSE_OK);
}

bool auth_show_login(GtkWindow *parent, AuthSession *session) {
    if (!session) {
        return false;
    }

    // Create login dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Database Browser - Login",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Login", GTK_RESPONSE_OK,
        NULL
    );

    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 250);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 20);

    // Main container
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    // Title label
    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), 
        "<span size='large' weight='bold'>Authentication Required</span>");
    gtk_widget_set_halign(title_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 0);

    // Subtitle
    GtkWidget *subtitle_label = gtk_label_new("Please enter your credentials to continue");
    gtk_widget_set_halign(subtitle_label, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(subtitle_label), "dim-label");
    gtk_box_pack_start(GTK_BOX(vbox), subtitle_label, FALSE, FALSE, 0);

    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 5);

    // Form grid
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, FALSE, 0);

    // Username label and entry
    GtkWidget *user_label = gtk_label_new("Username:");
    gtk_widget_set_halign(user_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), user_label, 0, 0, 1, 1);

    GtkWidget *user_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(user_entry), "Enter username");
    gtk_entry_set_width_chars(GTK_ENTRY(user_entry), 25);
    gtk_entry_set_activates_default(GTK_ENTRY(user_entry), TRUE);
    gtk_grid_attach(GTK_GRID(grid), user_entry, 1, 0, 1, 1);

    // Password label and entry
    GtkWidget *pass_label = gtk_label_new("Password:");
    gtk_widget_set_halign(pass_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), pass_label, 0, 1, 1, 1);

    GtkWidget *pass_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(pass_entry), "Enter password");
    gtk_entry_set_width_chars(GTK_ENTRY(pass_entry), 25);
    gtk_entry_set_visibility(GTK_ENTRY(pass_entry), FALSE);
    gtk_entry_set_activates_default(GTK_ENTRY(pass_entry), TRUE);
    gtk_grid_attach(GTK_GRID(grid), pass_entry, 1, 1, 1, 1);

    // Info label for dev credentials
    GtkWidget *info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(info_label), 
        "<small><i>Dev credentials: dev/dev123, admin/admin123, editor/edit123, viewer/view123</i></small>");
    gtk_widget_set_halign(info_label, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(info_label), "dim-label");
    gtk_box_pack_start(GTK_BOX(vbox), info_label, FALSE, FALSE, 0);

    // Status label for error messages
    GtkWidget *status_label = gtk_label_new("");
    gtk_widget_set_halign(status_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);

    // Set default button
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    // Connect activate signals
    g_signal_connect(user_entry, "activate", G_CALLBACK(on_login_entry_activate), dialog);
    g_signal_connect(pass_entry, "activate", G_CALLBACK(on_login_entry_activate), dialog);

    gtk_widget_show_all(dialog);

    // Login loop (allow retry)
    bool authenticated = false;
    int attempts = 0;
    const int max_attempts = 3;

    while (!authenticated && attempts < max_attempts) {
        gint response = gtk_dialog_run(GTK_DIALOG(dialog));

        if (response != GTK_RESPONSE_OK) {
            gtk_widget_destroy(dialog);
            return false;
        }

        const char *username = gtk_entry_get_text(GTK_ENTRY(user_entry));
        const char *password = gtk_entry_get_text(GTK_ENTRY(pass_entry));

        UserRole role;
        if (auth_verify_credentials(username, password, &role)) {
            // Success
            session->authenticated = true;
            session->username = g_strdup(username);
            session->role = role;
            session->login_time = time(NULL);
            authenticated = true;
        } else {
            // Failed
            attempts++;
            if (attempts < max_attempts) {
                char *msg = g_strdup_printf(
                    "<span foreground='red'>Invalid credentials. %d attempt(s) remaining.</span>",
                    max_attempts - attempts
                );
                gtk_label_set_markup(GTK_LABEL(status_label), msg);
                g_free(msg);
                gtk_entry_set_text(GTK_ENTRY(pass_entry), "");
                gtk_widget_grab_focus(user_entry);
            } else {
                gtk_label_set_markup(GTK_LABEL(status_label), 
                    "<span foreground='red'>Maximum login attempts exceeded.</span>");
                g_usleep(1500000); // 1.5 second delay
            }
        }
    }

    gtk_widget_destroy(dialog);
    return authenticated;
}
