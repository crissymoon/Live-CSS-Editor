# Authentication System

## Overview

Crissy's Database Browser now includes a User Authentication and Access Control (UAC) system with role-based permissions.

## Login Credentials (Development)

The following temporary development credentials are available:

| Username | Password   | Role          | Permissions                                      |
|----------|------------|---------------|--------------------------------------------------|
| dev      | dev123     | Administrator | Full access to all features                      |
| admin    | admin123   | Administrator | Full access to all features                      |
| editor   | edit123    | Editor        | Can modify data, limited schema changes          |
| viewer   | view123    | Viewer        | Read-only access                                 |

## User Roles

### Viewer
- Read-only access to databases
- Can view tables and execute SELECT queries
- Cannot modify data or schema

### Editor
- All Viewer permissions
- Can execute INSERT, UPDATE, DELETE queries
- Can modify data but cannot alter schema

### Administrator
- Full access to all features
- Can create/drop databases and tables
- Can modify schema and data
- Access to all administrative functions

## Features

- **Login Dialog**: Appears on startup before main window
- **Session Management**: Sessions expire after 30 minutes of activity
- **Failed Login Handling**: Maximum 3 login attempts before blocking
- **Role-Based Access**: UI and features adapt based on user role

## Security Notes

These credentials are for development only. In production:
- Credentials should be stored securely (hashed with salt)
- Authentication should use a secure database or external service
- Role permissions should be configurable per deployment

## Login Process

1. Application starts
2. Login dialog appears
3. Enter username and password
4. Click "Login" or press Enter
5. On success: Main window opens, username and role displayed in logs
6. On failure: Error message shown, retry allowed (max 3 attempts)
7. On cancel: Application exits

## Implementation Files

- `modules/auth.h` - Authentication API and types
- `modules/auth.c` - Login dialog and credential verification
- Modified: `main.c` - Integrated login on startup
- Modified: `modules/app_state.h` - Added AuthSession to app state

## Future Enhancements

- Persistent user database
- Password hashing (bcrypt/Argon2)
- Multi-factor authentication
- Session persistence across restarts
- Activity logging and audit trail
- Permission granularity (per-table/per-column)
- User management UI for administrators
