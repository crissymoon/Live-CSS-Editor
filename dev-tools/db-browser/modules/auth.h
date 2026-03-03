#ifndef AUTH_H
#define AUTH_H

#include <gtk/gtk.h>
#include <stdbool.h>

// User roles for access control
typedef enum {
    ROLE_VIEWER,     // Read-only access
    ROLE_EDITOR,     // Can modify data
    ROLE_ADMIN       // Full access including schema changes
} UserRole;

// Authentication session
typedef struct {
    bool authenticated;
    char *username;
    UserRole role;
    time_t login_time;
} AuthSession;

// Initialize authentication system
void auth_init(void);

// Show login dialog, returns true if authentication successful
bool auth_show_login(GtkWindow *parent, AuthSession *session);

// Verify credentials
bool auth_verify_credentials(const char *username, const char *password, UserRole *role);

// Check if current session has permission for action
bool auth_has_permission(AuthSession *session, UserRole required_role);

// Logout and cleanup session
void auth_logout(AuthSession *session);

// Get user-friendly role name
const char* auth_get_role_name(UserRole role);

// Session timeout check (returns false if session expired)
bool auth_session_valid(AuthSession *session);

#endif // AUTH_H
