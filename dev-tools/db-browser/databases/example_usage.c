/*
 * Example: Database Encryption and Transfer API
 * 
 * This demonstrates how to use the db_crypto and db_transfer APIs
 * to securely store and transfer SQLite databases with encryption.
 */

#include "../core/db_crypto.h"
#include "../core/db_transfer.h"
#include <stdio.h>
#include <stdlib.h>

void example_basic_encryption(void) {
    printf("\n=== Basic File Encryption Example ===\n");
    
    const char *password = "MySecurePassword123!";
    const char *input_file = "test.db";
    const char *encrypted_file = "test.db.enc";
    const char *decrypted_file = "test_decrypted.db";
    
    // Encrypt a database file
    printf("Encrypting %s...\n", input_file);
    if (crypto_encrypt_file(input_file, encrypted_file, password)) {
        printf("Success! Encrypted file: %s\n", encrypted_file);
        
        // Decrypt the file
        printf("Decrypting %s...\n", encrypted_file);
        if (crypto_decrypt_file(encrypted_file, decrypted_file, password)) {
            printf("Success! Decrypted file: %s\n", decrypted_file);
        } else {
            printf("Error: Decryption failed\n");
        }
    } else {
        printf("Error: Encryption failed\n");
    }
}

void example_database_storage(void) {
    printf("\n=== Database Storage Management Example ===\n");
    
    // Initialize storage directory
    if (!db_transfer_init("./databases")) {
        printf("Error: Failed to initialize storage\n");
        return;
    }
    
    printf("Storage initialized: %s\n", db_transfer_get_storage_dir());
    
    // Import a database with encryption
    const char *password = "SecurePassword456!";
    printf("\nImporting database with encryption...\n");
    if (db_transfer_import("source.db", "my_project", password, false)) {
        printf("Database imported and encrypted\n");
        
        // List databases
        int count;
        DatabaseInfo **list = db_transfer_list_databases(&count);
        printf("\nStored databases (%d):\n", count);
        for (int i = 0; i < count; i++) {
            printf("  - %s: %zu bytes, %s\n",
                   list[i]->name,
                   list[i]->size,
                   list[i]->is_encrypted ? "encrypted" : "unencrypted");
        }
        db_transfer_free_database_list(list, count);
    }
}

void example_remote_transfer(void) {
    printf("\n=== Remote Transfer Example ===\n");
    
    const char *password = "RemoteTransferPassword789!";
    
    // Initialize storage
    db_transfer_init("./databases");
    
    // SENDER SIDE: Prepare database for remote transfer
    printf("\n[SENDER] Preparing database for transfer...\n");
    char *encrypted_path = db_transfer_prepare_for_remote("my_project", password);
    if (encrypted_path) {
        printf("Encrypted file ready for transfer: %s\n", encrypted_path);
        printf("Now transfer this file via network/USB/cloud...\n");
        
        // RECEIVER SIDE: Receive and decrypt
        printf("\n[RECEIVER] Receiving encrypted database...\n");
        if (db_transfer_receive_from_remote(encrypted_path, "received_project", password)) {
            printf("Database received and stored securely\n");
            
            // Verify integrity
            if (db_transfer_verify_integrity("received_project")) {
                printf("Database integrity verified!\n");
            }
        }
        
        free(encrypted_path);
    }
}

void example_working_with_encrypted_database(void) {
    printf("\n=== Working with Encrypted Database Example ===\n");
    
    const char *password = "WorkingPassword321!";
    
    // Initialize storage
    db_transfer_init("./databases");
    
    // Open encrypted database for use (auto-decrypts to temp)
    printf("Opening encrypted database...\n");
    char *working_path = db_transfer_open_for_use("my_project", password);
    if (working_path) {
        printf("Database ready for use: %s\n", working_path);
        
        // ... perform database operations on working_path ...
        printf("Performing operations on database...\n");
        
        // Save changes back (re-encrypts)
        printf("Saving changes...\n");
        if (db_transfer_save_changes("my_project", working_path, password)) {
            printf("Changes saved and re-encrypted\n");
        }
        
        free(working_path);
    }
}

void example_password_generation(void) {
    printf("\n=== Secure Password Generation Example ===\n");
    
    // Generate a strong password
    char *password = db_transfer_generate_password(32);
    if (password) {
        printf("Generated secure password: %s\n", password);
        printf("Use this for encrypting sensitive databases\n");
        
        // Use the password...
        
        // Clean up
        free(password);
    }
}

void example_backup_and_recovery(void) {
    printf("\n=== Backup and Recovery Example ===\n");
    
    const char *password = "BackupPassword!@#";
    
    db_transfer_init("./databases");
    
    // Create a backup before risky operation
    printf("Creating backup...\n");
    if (db_transfer_create_backup("my_project", "pre_update")) {
        printf("Backup created: my_project.pre_update\n");
    }
    
    // Encrypt existing unencrypted database
    printf("\nEncrypting existing database...\n");
    if (db_transfer_encrypt_database("my_project", password, false)) {
        printf("Database encrypted (keeping unencrypted copy)\n");
    }
    
    // Later, if needed, decrypt it
    printf("\nDecrypting database...\n");
    if (db_transfer_decrypt_database("my_project", password, false)) {
        printf("Database decrypted (keeping encrypted copy)\n");
    }
}

int main(void) {
    printf("Database Encryption and Transfer API Examples\n");
    printf("==============================================\n");
    
    printf("\nThese examples demonstrate the secure database storage system\n");
    printf("with AES-256 encryption and salted key derivation.\n");
    
    // Uncomment the examples you want to run:
    
    // example_basic_encryption();
    // example_database_storage();
    // example_remote_transfer();
    // example_working_with_encrypted_database();
    // example_password_generation();
    // example_backup_and_recovery();
    
    printf("\n=== Usage Notes ===\n");
    printf("1. Always use strong passwords (16+ characters)\n");
    printf("2. Encrypt databases before remote transfer\n");
    printf("3. Verify integrity after transfer\n");
    printf("4. Keep unencrypted copies only when necessary\n");
    printf("5. Use backup before major operations\n");
    
    printf("\n=== Security Features ===\n");
    printf("- AES-256 encryption\n");
    printf("- PBKDF2-HMAC-SHA256 key derivation\n");
    printf("- 100,000 iterations for brute-force protection\n");
    printf("- Unique salt per database (32 bytes)\n");
    printf("- Random IV per encryption (16 bytes)\n");
    printf("- Authentication tags for integrity\n");
    
    return 0;
}
