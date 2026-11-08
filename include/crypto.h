#ifndef CRYPTO_H
#define CRYPTO_H

#include <vector>
#include <string>
#include <sodium.h>

namespace Crypto {

    // Size constants for libsodium cryptography (XChaCha20-Poly1305-IETF)
    constexpr std::size_t KeyLength = crypto_aead_xchacha20poly1305_ietf_KEYBYTES;
    constexpr std::size_t SaltLength = crypto_pwhash_SALTBYTES;
    constexpr std::size_t NonceLength = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    constexpr std::size_t MacLength = crypto_aead_xchacha20poly1305_ietf_ABYTES;

    // Initializes the libsodium cryptographic library
    bool initializeCrypto();

    // Generates a random salt for key derivation
    std::vector<unsigned char> generateSalt();

    // Generates a random nonce for XChaCha20-Poly1305 encryption (24 bytes)
    std::vector<unsigned char> generateNonce();

    // Derives a cryptographic key from a master password using Argon2id
    std::vector<unsigned char> deriveKey(const std::string& masterPassword, const std::vector<unsigned char>& salt);

    // Encrypts plaintext vault data using XChaCha20-Poly1305-IETF authenticated encryption
    std::vector<unsigned char> encryptVault(const std::vector<unsigned char>& plainText, const std::vector<unsigned char>& secretKey, const std::vector<unsigned char>& nonce);

    // Decrypts encrypted vault data using XChaCha20-Poly1305-IETF authenticated decryption
    std::vector<unsigned char> decryptVault(const std::vector<unsigned char>& cipherText, const std::vector<unsigned char>& secretKey, const std::vector<unsigned char>& nonce);

    // Erases sensitive data from system memory safely
    void secureClean(std::vector<unsigned char>& sensitiveData);
    void secureClean(std::string& sensitiveData);

} // namespace Crypto

#endif // CRYPTO_H