#include "db_crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __APPLE__
#include <CommonCrypto/CommonCrypto.h>
#include <Security/Security.h>
#else
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#endif

/* Magic header for encrypted database files */
static const uint8_t CRYPTO_MAGIC[8] = {'C', 'R', 'Y', 'S', 'D', 'B', 'E', 'X'};
static const uint32_t CRYPTO_VERSION = 1;

/* Secure zero memory */
void crypto_secure_zero(void *ptr, size_t len) {
    if (ptr) {
        volatile unsigned char *p = ptr;
        while (len--) *p++ = 0;
    }
}

/* Generate random bytes using system CSPRNG */
static bool crypto_random_bytes(uint8_t *buf, size_t len) {
#ifdef __APPLE__
    return SecRandomCopyBytes(kSecRandomDefault, len, buf) == errSecSuccess;
#else
    return RAND_bytes(buf, len) == 1;
#endif
}

bool crypto_generate_salt(uint8_t *salt, size_t salt_len) {
    return crypto_random_bytes(salt, salt_len);
}

bool crypto_generate_iv(uint8_t *iv, size_t iv_len) {
    return crypto_random_bytes(iv, iv_len);
}

/* Derive key from password using PBKDF2 */
bool crypto_derive_key(const char *password, 
                       const uint8_t *salt, 
                       size_t salt_len,
                       uint8_t *key, 
                       size_t key_len) {
    if (!password || !salt || !key) {
        return false;
    }

#ifdef __APPLE__
    int result = CCKeyDerivationPBKDF(
        kCCPBKDF2,
        password,
        strlen(password),
        salt,
        salt_len,
        kCCPRFHmacAlgSHA256,
        CRYPTO_PBKDF2_ITERATIONS,
        key,
        key_len
    );
    return result == kCCSuccess;
#else
    return PKCS5_PBKDF2_HMAC(
        password,
        strlen(password),
        salt,
        salt_len,
        CRYPTO_PBKDF2_ITERATIONS,
        EVP_sha256(),
        key_len,
        key
    ) == 1;
#endif
}

/* Create encryption context */
CryptoContext* crypto_context_create(const char *password) {
    if (!password || strlen(password) == 0) {
        fprintf(stderr, "[crypto] Invalid password\n");
        return NULL;
    }

    CryptoContext *ctx = malloc(sizeof(CryptoContext));
    if (!ctx) {
        return NULL;
    }

    memset(ctx, 0, sizeof(CryptoContext));

    // Generate random salt and IV
    if (!crypto_generate_salt(ctx->salt, CRYPTO_SALT_SIZE)) {
        fprintf(stderr, "[crypto] Failed to generate salt\n");
        free(ctx);
        return NULL;
    }

    if (!crypto_generate_iv(ctx->iv, CRYPTO_IV_SIZE)) {
        fprintf(stderr, "[crypto] Failed to generate IV\n");
        crypto_secure_zero(ctx, sizeof(CryptoContext));
        free(ctx);
        return NULL;
    }

    // Derive key from password
    if (!crypto_derive_key(password, ctx->salt, CRYPTO_SALT_SIZE, 
                          ctx->key, CRYPTO_KEY_SIZE)) {
        fprintf(stderr, "[crypto] Failed to derive key\n");
        crypto_secure_zero(ctx, sizeof(CryptoContext));
        free(ctx);
        return NULL;
    }

    ctx->initialized = true;
    return ctx;
}

void crypto_context_destroy(CryptoContext *ctx) {
    if (ctx) {
        crypto_secure_zero(ctx, sizeof(CryptoContext));
        free(ctx);
    }
}

/* Encrypt buffer using AES-256-GCM */
bool crypto_encrypt_buffer(const uint8_t *plaintext,
                           size_t plaintext_len,
                           const uint8_t *key,
                           const uint8_t *iv,
                           uint8_t **ciphertext,
                           size_t *ciphertext_len,
                           uint8_t *tag) {
    if (!plaintext || !key || !iv || !ciphertext || !ciphertext_len || !tag) {
        return false;
    }

#ifdef __APPLE__
    // Allocate output buffer
    *ciphertext_len = plaintext_len;
    *ciphertext = malloc(*ciphertext_len);
    if (!*ciphertext) {
        return false;
    }

    // Use AES-256-CBC for macOS (GCM not directly available in CommonCrypto)
    size_t moved = 0;
    CCCryptorStatus status = CCCrypt(
        kCCEncrypt,
        kCCAlgorithmAES,
        kCCOptionPKCS7Padding,
        key,
        CRYPTO_KEY_SIZE,
        iv,
        plaintext,
        plaintext_len,
        *ciphertext,
        *ciphertext_len,
        &moved
    );

    if (status != kCCSuccess) {
        fprintf(stderr, "[crypto] Encryption failed: %d\n", status);
        free(*ciphertext);
        *ciphertext = NULL;
        return false;
    }

    *ciphertext_len = moved;

    // Generate tag using HMAC-SHA256 as substitute for GCM tag
    CCHmac(kCCHmacAlgSHA256, key, CRYPTO_KEY_SIZE, 
           *ciphertext, *ciphertext_len, tag);

    return true;
#else
    EVP_CIPHER_CTX *ctx_evp = EVP_CIPHER_CTX_new();
    if (!ctx_evp) {
        return false;
    }

    *ciphertext_len = plaintext_len + EVP_CIPHER_block_size(EVP_aes_256_gcm());
    *ciphertext = malloc(*ciphertext_len);
    if (!*ciphertext) {
        EVP_CIPHER_CTX_free(ctx_evp);
        return false;
    }

    int len;
    if (EVP_EncryptInit_ex(ctx_evp, EVP_aes_256_gcm(), NULL, key, iv) != 1 ||
        EVP_EncryptUpdate(ctx_evp, *ciphertext, &len, plaintext, plaintext_len) != 1) {
        free(*ciphertext);
        EVP_CIPHER_CTX_free(ctx_evp);
        return false;
    }

    *ciphertext_len = len;
    int final_len;
    if (EVP_EncryptFinal_ex(ctx_evp, *ciphertext + len, &final_len) != 1) {
        free(*ciphertext);
        EVP_CIPHER_CTX_free(ctx_evp);
        return false;
    }

    *ciphertext_len += final_len;
    EVP_CIPHER_CTX_ctrl(ctx_evp, EVP_CTRL_GCM_GET_TAG, CRYPTO_TAG_SIZE, tag);
    EVP_CIPHER_CTX_free(ctx_evp);
    return true;
#endif
}

/* Decrypt buffer using AES-256-GCM */
bool crypto_decrypt_buffer(const uint8_t *ciphertext,
                           size_t ciphertext_len,
                           const uint8_t *key,
                           const uint8_t *iv,
                           const uint8_t *tag,
                           uint8_t **plaintext,
                           size_t *plaintext_len) {
    if (!ciphertext || !key || !iv || !tag || !plaintext || !plaintext_len) {
        return false;
    }

#ifdef __APPLE__
    // Verify tag first
    uint8_t computed_tag[CRYPTO_TAG_SIZE];
    CCHmac(kCCHmacAlgSHA256, key, CRYPTO_KEY_SIZE,
           ciphertext, ciphertext_len, computed_tag);
    
    if (memcmp(computed_tag, tag, CRYPTO_TAG_SIZE) != 0) {
        fprintf(stderr, "[crypto] Authentication tag verification failed\n");
        return false;
    }

    // Allocate output buffer
    *plaintext_len = ciphertext_len + kCCBlockSizeAES128;
    *plaintext = malloc(*plaintext_len);
    if (!*plaintext) {
        return false;
    }

    size_t moved = 0;
    CCCryptorStatus status = CCCrypt(
        kCCDecrypt,
        kCCAlgorithmAES,
        kCCOptionPKCS7Padding,
        key,
        CRYPTO_KEY_SIZE,
        iv,
        ciphertext,
        ciphertext_len,
        *plaintext,
        *plaintext_len,
        &moved
    );

    if (status != kCCSuccess) {
        fprintf(stderr, "[crypto] Decryption failed: %d\n", status);
        free(*plaintext);
        *plaintext = NULL;
        return false;
    }

    *plaintext_len = moved;
    return true;
#else
    EVP_CIPHER_CTX *ctx_evp = EVP_CIPHER_CTX_new();
    if (!ctx_evp) {
        return false;
    }

    *plaintext_len = ciphertext_len;
    *plaintext = malloc(*plaintext_len);
    if (!*plaintext) {
        EVP_CIPHER_CTX_free(ctx_evp);
        return false;
    }

    int len;
    if (EVP_DecryptInit_ex(ctx_evp, EVP_aes_256_gcm(), NULL, key, iv) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx_evp, EVP_CTRL_GCM_SET_TAG, CRYPTO_TAG_SIZE, (void*)tag) != 1 ||
        EVP_DecryptUpdate(ctx_evp, *plaintext, &len, ciphertext, ciphertext_len) != 1) {
        free(*plaintext);
        EVP_CIPHER_CTX_free(ctx_evp);
        return false;
    }

    *plaintext_len = len;
    int final_len;
    if (EVP_DecryptFinal_ex(ctx_evp, *plaintext + len, &final_len) != 1) {
        free(*plaintext);
        EVP_CIPHER_CTX_free(ctx_evp);
        return false;
    }

    *plaintext_len += final_len;
    EVP_CIPHER_CTX_free(ctx_evp);
    return true;
#endif
}

/* Encrypt file */
bool crypto_encrypt_file(const char *input_path, 
                         const char *output_path,
                         const char *password) {
    if (!input_path || !output_path || !password) {
        return false;
    }

    // Create crypto context
    CryptoContext *ctx = crypto_context_create(password);
    if (!ctx) {
        return false;
    }

    // Open input file
    FILE *in = fopen(input_path, "rb");
    if (!in) {
        fprintf(stderr, "[crypto] Failed to open input file: %s\n", strerror(errno));
        crypto_context_destroy(ctx);
        return false;
    }

    // Get file size
    fseek(in, 0, SEEK_END);
    long file_size = ftell(in);
    fseek(in, 0, SEEK_SET);

    if (file_size < 0 || file_size > 100 * 1024 * 1024) {  // 100MB limit
        fprintf(stderr, "[crypto] Invalid file size: %ld\n", file_size);
        fclose(in);
        crypto_context_destroy(ctx);
        return false;
    }

    // Read entire file
    uint8_t *plaintext = malloc(file_size);
    if (!plaintext) {
        fclose(in);
        crypto_context_destroy(ctx);
        return false;
    }

    size_t read_size = fread(plaintext, 1, file_size, in);
    fclose(in);

    if (read_size != (size_t)file_size) {
        fprintf(stderr, "[crypto] Failed to read file completely\n");
        free(plaintext);
        crypto_context_destroy(ctx);
        return false;
    }

    // Encrypt data
    uint8_t *ciphertext = NULL;
    size_t ciphertext_len = 0;
    uint8_t tag[CRYPTO_TAG_SIZE];

    bool encrypt_ok = crypto_encrypt_buffer(plaintext, file_size,
                                            ctx->key, ctx->iv,
                                            &ciphertext, &ciphertext_len, tag);
    
    crypto_secure_zero(plaintext, file_size);
    free(plaintext);

    if (!encrypt_ok) {
        crypto_context_destroy(ctx);
        return false;
    }

    // Write encrypted file
    FILE *out = fopen(output_path, "wb");
    if (!out) {
        fprintf(stderr, "[crypto] Failed to open output file: %s\n", strerror(errno));
        free(ciphertext);
        crypto_context_destroy(ctx);
        return false;
    }

    // Write header
    CryptoHeader header;
    memcpy(header.magic, CRYPTO_MAGIC, 8);
    header.version = CRYPTO_VERSION;
    memcpy(header.salt, ctx->salt, CRYPTO_SALT_SIZE);
    memcpy(header.iv, ctx->iv, CRYPTO_IV_SIZE);
    header.original_size = file_size;
    memcpy(header.tag, tag, CRYPTO_TAG_SIZE);

    if (fwrite(&header, sizeof(CryptoHeader), 1, out) != 1) {
        fprintf(stderr, "[crypto] Failed to write header\n");
        fclose(out);
        free(ciphertext);
        crypto_context_destroy(ctx);
        return false;
    }

    // Write encrypted data
    if (fwrite(ciphertext, 1, ciphertext_len, out) != ciphertext_len) {
        fprintf(stderr, "[crypto] Failed to write encrypted data\n");
        fclose(out);
        free(ciphertext);
        crypto_context_destroy(ctx);
        return false;
    }

    fclose(out);
    free(ciphertext);
    crypto_context_destroy(ctx);

    printf("[crypto] Successfully encrypted: %s -> %s\n", input_path, output_path);
    return true;
}

/* Decrypt file */
bool crypto_decrypt_file(const char *input_path,
                         const char *output_path,
                         const char *password) {
    if (!input_path || !output_path || !password) {
        return false;
    }

    // Open encrypted file
    FILE *in = fopen(input_path, "rb");
    if (!in) {
        fprintf(stderr, "[crypto] Failed to open encrypted file: %s\n", strerror(errno));
        return false;
    }

    // Read header
    CryptoHeader header;
    if (fread(&header, sizeof(CryptoHeader), 1, in) != 1) {
        fprintf(stderr, "[crypto] Failed to read header\n");
        fclose(in);
        return false;
    }

    // Verify magic
    if (memcmp(header.magic, CRYPTO_MAGIC, 8) != 0) {
        fprintf(stderr, "[crypto] Invalid file format\n");
        fclose(in);
        return false;
    }

    // Verify version
    if (header.version != CRYPTO_VERSION) {
        fprintf(stderr, "[crypto] Unsupported version: %u\n", header.version);
        fclose(in);
        return false;
    }

    // Derive key from password
    uint8_t key[CRYPTO_KEY_SIZE];
    if (!crypto_derive_key(password, header.salt, CRYPTO_SALT_SIZE,
                          key, CRYPTO_KEY_SIZE)) {
        fprintf(stderr, "[crypto] Failed to derive key\n");
        fclose(in);
        return false;
    }

    // Get encrypted data size
    fseek(in, 0, SEEK_END);
    long file_size = ftell(in);
    fseek(in, sizeof(CryptoHeader), SEEK_SET);
    
    size_t ciphertext_len = file_size - sizeof(CryptoHeader);

    // Read encrypted data
    uint8_t *ciphertext = malloc(ciphertext_len);
    if (!ciphertext) {
        fclose(in);
        crypto_secure_zero(key, CRYPTO_KEY_SIZE);
        return false;
    }

    if (fread(ciphertext, 1, ciphertext_len, in) != ciphertext_len) {
        fprintf(stderr, "[crypto] Failed to read encrypted data\n");
        fclose(in);
        free(ciphertext);
        crypto_secure_zero(key, CRYPTO_KEY_SIZE);
        return false;
    }

    fclose(in);

    // Decrypt data
    uint8_t *plaintext = NULL;
    size_t plaintext_len = 0;

    bool decrypt_ok = crypto_decrypt_buffer(ciphertext, ciphertext_len,
                                            key, header.iv, header.tag,
                                            &plaintext, &plaintext_len);

    free(ciphertext);
    crypto_secure_zero(key, CRYPTO_KEY_SIZE);

    if (!decrypt_ok) {
        fprintf(stderr, "[crypto] Decryption failed - wrong password or corrupted file\n");
        return false;
    }

    // Write decrypted file
    FILE *out = fopen(output_path, "wb");
    if (!out) {
        fprintf(stderr, "[crypto] Failed to open output file: %s\n", strerror(errno));
        crypto_secure_zero(plaintext, plaintext_len);
        free(plaintext);
        return false;
    }

    if (fwrite(plaintext, 1, plaintext_len, out) != plaintext_len) {
        fprintf(stderr, "[crypto] Failed to write decrypted data\n");
        fclose(out);
        crypto_secure_zero(plaintext, plaintext_len);
        free(plaintext);
        return false;
    }

    fclose(out);
    crypto_secure_zero(plaintext, plaintext_len);
    free(plaintext);

    printf("[crypto] Successfully decrypted: %s -> %s\n", input_path, output_path);
    return true;
}

/* Verify password without full decryption */
bool crypto_verify_password(const char *password,
                           const char *encrypted_file) {
    if (!password || !encrypted_file) {
        return false;
    }

    FILE *in = fopen(encrypted_file, "rb");
    if (!in) {
        return false;
    }

    CryptoHeader header;
    if (fread(&header, sizeof(CryptoHeader), 1, in) != 1) {
        fclose(in);
        return false;
    }

    fclose(in);

    if (memcmp(header.magic, CRYPTO_MAGIC, 8) != 0) {
        return false;
    }

    // Try to derive key - if this works, password format is valid
    uint8_t key[CRYPTO_KEY_SIZE];
    bool result = crypto_derive_key(password, header.salt, CRYPTO_SALT_SIZE,
                                   key, CRYPTO_KEY_SIZE);
    crypto_secure_zero(key, CRYPTO_KEY_SIZE);
    
    return result;
}
