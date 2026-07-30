#include "crypto.h"
#include <iostream>
#include <stdexcept>

using namespace std;

namespace Crypto {

    // This function starts up libsodium and checks if your computer hardware supports AES encryption
    bool initializeCrypto() {
        int initResult = 0;
        initResult = sodium_init();

        if (initResult < 0) {
            cout << "[!] Error: Libsodium library failed to initialize." << endl;
            return false;
        }

        int hardwareSupport = 0;
        hardwareSupport = crypto_aead_aes256gcm_is_available();

        if (hardwareSupport == 0) {
            cout << "[!] Error: AES-256-GCM hardware acceleration is not available on this CPU." << endl;
            return false;
        }

        return true;
    }

    // This function creates a random salt byte array needed for Argon2id key derivation
    vector<unsigned char> generateSalt() {
        vector<unsigned char> saltBuffer(SaltLength);
        randombytes_buf(saltBuffer.data(), saltBuffer.size());
        return saltBuffer;
    }

    // This function creates a random nonce byte array needed for AES-GCM encryption
    vector<unsigned char> generateNonce() {
        vector<unsigned char> nonceBuffer(NonceLength);
        randombytes_buf(nonceBuffer.data(), nonceBuffer.size());
        return nonceBuffer;
    }

    // This function takes the master password and salt to generate a 256-bit key using Argon2id
    vector<unsigned char> deriveKey(const string& masterPassword, const vector<unsigned char>& salt) {
        if (salt.size() != SaltLength) {
            cout << "[!] Error: Provided salt size is invalid." << endl;
            throw invalid_argument("Invalid salt length.");
        }

        vector<unsigned char> derivedKey(KeyLength);
        int derivationStatus = 0;

        derivationStatus = crypto_pwhash(
            derivedKey.data(),
            derivedKey.size(),
            masterPassword.c_str(),
            masterPassword.length(),
            salt.data(),
            crypto_pwhash_OPSLIMIT_MODERATE,
            crypto_pwhash_MEMLIMIT_MODERATE,
            crypto_pwhash_ALG_ARGON2ID13
        );

        if (derivationStatus != 0) {
            cout << "[!] Error: Failed to derive cryptographic key from master password." << endl;
            throw runtime_error("Key derivation failed.");
        }

        return derivedKey;
    }

    // This function encrypts sensitive plaintext data using AES-256-GCM encryption
    vector<unsigned char> encryptVault(const vector<unsigned char>& plainText, const vector<unsigned char>& secretKey, const vector<unsigned char>& nonce) {
        if (secretKey.size() != KeyLength) {
            cout << "[!] Error: Secret key size is invalid." << endl;
            throw invalid_argument("Invalid key size.");
        }

        if (nonce.size() != NonceLength) {
            cout << "[!] Error: Nonce size is invalid." << endl;
            throw invalid_argument("Invalid nonce size.");
        }

        vector<unsigned char> cipherText(plainText.size() + MacLength);
        unsigned long long ciphertextLength = 0;

        int encryptionStatus = 0;
        encryptionStatus = crypto_aead_aes256gcm_encrypt(
            cipherText.data(),
            &ciphertextLength,
            plainText.data(),
            plainText.size(),
            nullptr,
            0,
            nullptr,
            nonce.data(),
            secretKey.data()
        );

        if (encryptionStatus != 0) {
            cout << "[!] Error: Encryption failed." << endl;
            throw runtime_error("AES-256-GCM encryption failed.");
        }

        cipherText.resize(ciphertextLength);
        return cipherText;
    }

    // This function decrypts the ciphertext using AES-256-GCM and verifies that data was not tampered with
    vector<unsigned char> decryptVault(const vector<unsigned char>& cipherText, const vector<unsigned char>& secretKey, const vector<unsigned char>& nonce) {
        if (secretKey.size() != KeyLength) {
            cout << "[!] Error: Secret key size is invalid." << endl;
            throw invalid_argument("Invalid key size.");
        }

        if (nonce.size() != NonceLength) {
            cout << "[!] Error: Nonce size is invalid." << endl;
            throw invalid_argument("Invalid nonce size.");
        }

        if (cipherText.size() < MacLength) {
            cout << "[!] Error: Ciphertext is too short to contain authentication tag." << endl;
            return {};
        }

        vector<unsigned char> decryptedText(cipherText.size() - MacLength);
        unsigned long long decryptedLength = 0;

        int decryptionStatus = 0;
        decryptionStatus = crypto_aead_aes256gcm_decrypt(
            decryptedText.data(),
            &decryptedLength,
            nullptr,
            cipherText.data(),
            cipherText.size(),
            nullptr,
            0,
            nonce.data(),
            secretKey.data()
        );

        if (decryptionStatus != 0) {
            cout << "[!] Warning: Decryption or authentication failed. Invalid password or corrupted data." << endl;
            secureClean(decryptedText);
            return {};
        }

        decryptedText.resize(decryptedLength);
        return decryptedText;
    }

    // This function securely clears sensitive passwords and keys from RAM memory
    void secureClean(vector<unsigned char>& sensitiveData) {
        if (sensitiveData.empty() == false) {
            sodium_memzero(sensitiveData.data(), sensitiveData.size());
            sensitiveData.clear();
        }
    }

} // namespace Crypto