# Kryptos C++ Password Manager

A lightweight, terminal-based password manager written in C++17. Designed with a security-first approach, it features memory safety practices, modern key derivation, and authenticated binary file encryption.

## 🔐 Key Features

* **Argon2id Key Derivation:** Uses state-of-the-art Argon2id hashing to derive encryption keys from master passwords, protecting against GPU and ASIC brute-force attacks.
* **AES-256-GCM Authenticated Encryption:** Encrypts vault data with AES-256-GCM via Libsodium, ensuring both confidentiality and data integrity (tamper resistance).
* **RAM Safety:** Clears sensitive memory buffers (plain-text data and derived keys) using `sodium_memzero()` immediately after operations finish.
* **Masked Password Entry:** Prevents screen-shoulder surfing by hiding input during master password prompts on Linux terminals.
* **Self-Contained Vault Storage:** Packages raw binary salt, nonce, and ciphertext directly into an encrypted `.dat` vault file.

## 🛠️ Tech Stack

* **Language:** C++17
* **Cryptography:** Libsodium
* **Build System:** CMake (3.16+)
* **Platform:** Linux (Fedora / Debian / Ubuntu)

## 🚀 Build and Run

### Prerequisites
Ensure you have `libsodium` development headers installed:
```bash
# Fedora
sudo dnf install libsodium-devel

# Ubuntu/Debian
sudo apt install libsodium-dev
