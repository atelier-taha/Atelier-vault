#ifndef VAULT_H
#define VAULT_H

#include <string>
#include <vector>

using namespace std;

// This structure holds the details for one single saved login
struct Credential {
    string websiteName;
    string userName;
    string userPassword;
};

namespace Vault {

    // This function takes all your saved logins and combines them into one long string of text
    string serializeCredentials(const vector<Credential>& credentialList);

    // This function takes the long string of decrypted text and breaks it back down into individual logins
    vector<Credential> deserializeCredentials(const string& plainTextData);

    // This function saves your encrypted data, salt, and nonce to a file on your hard drive
    bool saveEncryptedVaultToFile(const string& filePath, const vector<unsigned char>& cipherText, const vector<unsigned char>& saltBuffer, const vector<unsigned char>& nonceBuffer);

    // This function reads your encrypted data, salt, and nonce from the file so we can unlock it
    bool loadEncryptedVaultFromFile(const string& filePath, vector<unsigned char>& cipherText, vector<unsigned char>& saltBuffer, vector<unsigned char>& nonceBuffer);

} // namespace Vault

#endif // VAULT_H