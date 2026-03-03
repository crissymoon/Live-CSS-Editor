#ifndef DB_CRYPTO_H
#define DB_CRYPTO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Encryption constants */
#define CRYPTO_KEY_SIZE 32        // AES-256 key size
#define CRYPTO_IV_SIZE 16         // AES block size
#define CRYPTO_SALT_SIZE 32       // Salt size for key derivation
#define CRYPTO_TAG_SIZE 16        // GCM authentication tag size
#define CRYPTO_PBKDF2_ITERATIONS 100000  // PBKDF2 iterations

/* Encrypted file header structure */
typedef struct {
    uint8_t magic[8];             // Magic bytes "CRYSDBEX"
    uint32_t version;             // Format version
    uint8_t salt[CRYPTO_SALT_SIZE];  // Salt for key derivation
    uint8_t iv[CRYPTO_IV_SIZE];      // Initialization vector
    uint64_t original_size;       // Original file size
    uint8_t tag[CRYPTO_TAG_SIZE];    // Authentication tag
} CryptoHeader;

/* Encryption context */
typedef struct {
    uint8_t key[CRYPTO_KEY_SIZE];
    uint8_t iv[CRYPTO_IV_SIZE];
    uint8_t salt[CRYPTO_SALT_SIZE];
    bool initialized;
} CryptoContext;

/* Key derivation from password */
bool crypto_derive_key(const char *password, 
                       const uint8_t *salt, 
                       size_t salt_len,
                       uint8_t *key, 
                       size_t key_len);

/* Generate random salt */
bool crypto_generate_salt(uint8_t *salt, size_t salt_len);

/* Generate random IV */
bool crypto_generate_iv(uint8_t *iv, size_t iv_len);

/* Initialize encryption context */
CryptoContext* crypto_context_create(const char *password);
void crypto_context_destroy(CryptoContext *ctx);

/* File encryption/decryption */
bool crypto_encrypt_file(const char *input_path, 
                         const char *output_path,
                         const char *password);

bool crypto_decrypt_file(const char *input_path,
                         const char *output_path,
                         const char *password);

/* Memory encryption/decryption for in-memory operations */
bool crypto_encrypt_buffer(const uint8_t *plaintext,
                           size_t plaintext_len,
                           const uint8_t *key,
                           const uint8_t *iv,
                           uint8_t **ciphertext,
                           size_t *ciphertext_len,
                           uint8_t *tag);

bool crypto_decrypt_buffer(const uint8_t *ciphertext,
                           size_t ciphertext_len,
                           const uint8_t *key,
                           const uint8_t *iv,
                           const uint8_t *tag,
                           uint8_t **plaintext,
                           size_t *plaintext_len);

/* Utility functions */
bool crypto_verify_password(const char *password,
                           const char *encrypted_file);

void crypto_secure_zero(void *ptr, size_t len);

#endif // DB_CRYPTO_H
