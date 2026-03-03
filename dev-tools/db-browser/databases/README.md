# Database Storage Directory

This directory stores SQLite databases for Crissy's DB Browser with optional encryption.

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

