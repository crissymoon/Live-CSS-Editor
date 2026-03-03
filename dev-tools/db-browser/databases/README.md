# Database Storage Directory

This directory stores SQLite databases for Crissy's DB Browser with optional encryption.

## Starter Databases

This directory includes production-ready database templates for common business applications:

### 1. Double Entry Accounting (accounting.db)
Complete double-entry bookkeeping system following standard accounting principles.

**Tables:**
- accounts - Chart of accounts with account types and balances
- journal_entries - Journal entry headers with posting status
- journal_lines - Debit/credit transaction lines
- fiscal_periods - Accounting period management

**Features:**
- Asset, Liability, Equity, Revenue, Expense accounts
- Enforced double-entry constraint (debits = credits)
- Trial balance view
- Sample chart of accounts included

### 2. Payroll Management (payroll.db)
Employee compensation, time tracking, and payroll processing.

**Tables:**
- employees - Employee master records
- time_entries - Clock in/out and hours worked
- pay_periods - Payroll period definitions
- payroll_runs - Individual paychecks with deductions
- benefits - Health insurance, 401k, etc.
- employee_benefits - Benefit enrollment
- pto_requests - Vacation and sick leave
- pto_balances - PTO accrual tracking

**Features:**
- Hourly and salaried employees
- Overtime calculation support
- Tax withholding fields (federal, state, FICA)
- Benefit management
- PTO tracking and approval workflow

### 3. Ticket System (tickets.db)
Help desk and support ticket management.

**Tables:**
- users - Customers and support agents
- departments - Support teams
- categories - Ticket classification
- tickets - Support tickets with priority and status
- ticket_comments - Conversation threads
- ticket_attachments - File attachments
- ticket_history - Audit trail
- sla_rules - Service level agreements
- tags - Flexible tagging system

**Features:**
- Priority levels (LOW, MEDIUM, HIGH, URGENT)
- Status tracking (OPEN, IN_PROGRESS, RESOLVED, etc.)
- Agent workload view
- SLA compliance tracking
- Multi-channel support (email, phone, web, chat)

### 4. CRM (crm.db)
Customer relationship management and sales pipeline.

**Tables:**
- companies - Customer and prospect organizations
- contacts - Individual contacts with roles
- opportunities - Sales deals and pipeline
- activities - Calls, meetings, emails, tasks
- products - Product/service catalog
- opportunity_products - Deal line items
- lead_sources - Lead generation tracking
- pipeline_stages - Customizable sales stages
- notes - General notes and comments

**Features:**
- Complete sales pipeline management
- Activity tracking and task management
- Product catalog with pricing
- Lead source attribution
- Sales forecasting support
- Contact directory with company relationships

### 5. Calendar (calendar.db)
Event management, scheduling, and resource booking.

**Tables:**
- calendar_users - System users
- calendars - Multiple calendars per user
- events - Appointments, meetings, tasks
- event_attendees - RSVP tracking
- event_reminders - Notification system
- resources - Rooms, equipment, vehicles
- resource_bookings - Resource reservation
- calendar_sharing - Shared calendar permissions
- event_categories - Event classification

**Features:**
- Recurring events support
- Multi-user calendar sharing
- Resource booking and approval
- RSVP tracking (Accepted, Declined, Tentative)
- Multiple reminder types
- Timezone support
- Public/private/confidential visibility levels

### 6. Security Logging (security_logs.db)
Comprehensive security audit trail and monitoring.

**Tables:**
- security_users - User accounts
- auth_logs - Login/logout events
- activity_logs - User actions and operations
- api_logs - API endpoint access
- security_events - Threats and anomalies
- data_access_logs - Sensitive data access tracking
- permission_logs - Authorization changes
- sessions - Active session management
- failed_access_attempts - Brute force detection
- ip_blacklist - Blocked IP addresses
- security_alerts - Incident notifications

**Features:**
- Authentication tracking
- Failed login detection
- API access logging with response times
- Security event monitoring (intrusion, DOS, etc.)
- Data access audit trail
- Permission change tracking
- IP blacklisting
- Session management
- Real-time security alerts

### 7. Users (users.db)
Web application user management with roles and permissions.

**Tables:**
- users - Core user accounts with authentication
- user_profiles - Extended profile information
- roles - Role definitions (admin, editor, user, etc.)
- permissions - Granular permission system
- role_permissions - Role to permission mapping
- user_roles - User role assignments
- sessions - Session tokens and management
- email_verifications - Email confirmation workflow
- password_resets - Password reset tokens
- user_settings - User preferences
- user_activity - Activity logging
- oauth_providers - Social login providers
- oauth_connections - Linked social accounts

**Features:**
- Password hashing placeholders
- Role-based access control (RBAC)
- Permission system (resource + action)
- Session management with expiration
- Email verification workflow
- Password reset functionality
- OAuth/social login support
- User preferences and settings
- Activity logging
- Two-factor authentication support
- User status management (active, suspended, deleted)

## Using Starter Databases

All starter databases include:
- Properly structured schemas with foreign keys
- Sample data for testing
- Indexes for performance
- Useful views for common queries
- Constraints to maintain data integrity

To use a starter database:
1. Copy the database file to your working directory
2. Open it in Crissy's DB Browser
3. Customize tables and data as needed
4. Build your application on top of it

## Features

### Local Storage
- All databases are stored in this centralized location
- Files are organized with `.db` extension for unencrypted databases
- Encrypted versions use `.db.enc` extension

### High-Level Salted Encryption
When databases are encrypted for remote transfer, they use:
- **AES-256-CBC** encryption (AES-256-GCM on Linux/OpenSSL)
- **PBKDF2-HMAC-SHA256** key derivation with 100,000 iterations
- **32-byte cryptographically secure random salt** per database
- **16-byte random initialization vector (IV)**
- **HMAC-SHA256 authentication tag** for integrity verification

### Encrypted File Format
```
[8 bytes]  Magic: "CRYSDBEX"
[4 bytes]  Version: 1
[32 bytes] Salt for key derivation
[16 bytes] Initialization vector
[8 bytes]  Original file size
[16 bytes] Authentication tag
[variable] Encrypted database content
```

## API Usage

### C API

```c
#include "core/db_transfer.h"
#include "core/db_crypto.h"

// Initialize storage
db_transfer_init("/path/to/databases");

// Import database with encryption
db_transfer_import("source.db", "my_database", "password123", false);

// Export decrypted database
db_transfer_export("my_database", "output.db", "password123");

// Prepare for remote transfer (ensure encrypted)
char *encrypted_path = db_transfer_prepare_for_remote("my_database", "password123");

// Receive from remote location
db_transfer_receive_from_remote("transferred.db.enc", "received_db", "password123");

// Open database for use (auto-decrypt if needed)
char *working_path = db_transfer_open_for_use("my_database", "password123");

// Save changes back (re-encrypt if needed)
db_transfer_save_changes("my_database", working_path, "password123");

// List all databases
int count;
DatabaseInfo **list = db_transfer_list_databases(&count);
for (int i = 0; i < count; i++) {
    printf("%s: %zu bytes, encrypted=%d\n", 
           list[i]->name, list[i]->size, list[i]->is_encrypted);
}
db_transfer_free_database_list(list, count);

// Encrypt existing database
db_transfer_encrypt_database("my_database", "password123", true);

// Decrypt database in storage
db_transfer_decrypt_database("my_database", "password123", false);

// Generate secure password
char *password = db_transfer_generate_password(32);
// Use password...
free(password);
```

### Security Considerations

1. **Passwords are never stored** - Only used for key derivation
2. **Unique salt per database** - Prevents rainbow table attacks
3. **High iteration count** - Makes brute force attacks expensive (100,000 iterations)
4. **Memory is securely zeroed** - Sensitive data cleared after use
5. **Authentication tags** - Detects tampering or corruption
6. **Random IVs** - Each encryption uses a unique initialization vector

### File Operations

- **`.db`** - Unencrypted SQLite database (local use only)
- **`.db.enc`** - Encrypted database (safe for remote transfer)
- **`.tmp`** - Temporary decrypted working copy

### Remote Transfer Workflow

1. **Before sending:**
   ```c
   char *path = db_transfer_prepare_for_remote("mydb", "password");
   // Transfer the file at 'path' via network/USB/cloud
   ```

2. **After receiving:**
   ```c
   db_transfer_receive_from_remote("received.db.enc", "mydb", "password");
   ```

3. **Use the database:**
   ```c
   char *working_path = db_transfer_open_for_use("mydb", "password");
   // Work with database at working_path
   db_transfer_save_changes("mydb", working_path, "password");
   ```

### Best Practices

- Use strong passwords (minimum 16 characters recommended)
- Keep unencrypted copies only when necessary
- Always encrypt before remote transfer
- Verify integrity after transfer: `db_transfer_verify_integrity("mydb")`
- Use `db_transfer_create_backup()` before major operations
- Clean up temporary files after use

## Platform Support

- **macOS**: Uses CommonCrypto and Security Framework (native)
- **Linux**: Uses OpenSSL for encryption (requires libssl-dev)

## Security Notes

This encryption is designed to protect databases during:
- Network transfer
- Cloud storage
- USB drive transport
- Email/messaging attachments

**Not suitable for:**
- Compliance with specific regulations (HIPAA, PCI-DSS) without additional measures
- Long-term archival (consider key rotation)
- Protection against sophisticated state-level attacks

For production systems handling sensitive data, consider additional security measures:
- Hardware security modules (HSM)
- Regular key rotation
- Multi-factor authentication
- Access logging and auditing

