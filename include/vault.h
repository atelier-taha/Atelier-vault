#ifndef VAULT_H
#define VAULT_H

#include <string>
#include <vector>
#include <QString>

// This structure holds the details for one saved login or secure item
struct Credential {
    std::string id;          // UUID identifier
    std::string websiteName; // e.g. "GitHub"
    std::string url;         // e.g. "https://github.com/login"
    std::string userName;    // e.g. "user@example.com"
    std::string userPassword;// Sensitive plain text
    std::string notes;       // Optional user notes / recovery codes
    std::string category;    // "Logins", "Finance", "Secure Note", "Work", etc.
};

namespace Vault {

    // Magic file header constants
    constexpr char Magic[4] = {'A', 'T', 'L', 'V'};
    constexpr uint32_t CurrentVersion = 1;

    // Directory and vault paths
    QString getVaultsDirectory();
    QString getVaultFilePath(const QString& vaultName);
    std::vector<QString> listAvailableVaults();
    void migrateLegacyVaultIfNeeded();

    // Serialization
    std::string serializeCredentials(const std::vector<Credential>& credentialList);
    std::vector<Credential> deserializeCredentials(const std::string& plainTextData);

    // File I/O
    bool saveEncryptedVaultToFile(const std::string& filePath, const std::vector<unsigned char>& cipherText, const std::vector<unsigned char>& saltBuffer, const std::vector<unsigned char>& nonceBuffer);
    bool loadEncryptedVaultFromFile(const std::string& filePath, std::vector<unsigned char>& cipherText, std::vector<unsigned char>& saltBuffer, std::vector<unsigned char>& nonceBuffer);

} // namespace Vault

#endif // VAULT_H