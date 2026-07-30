#include "vault.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

namespace Vault {

    // This function takes all your saved logins and combines them into one long string of text
    string serializeCredentials(const vector<Credential>& credentialList) {
        string serializedText = "";
        
        for (const Credential& currentCredential : credentialList) {
            // We use the | symbol to separate the data so we can split it later
            serializedText = serializedText + currentCredential.websiteName + "|" + currentCredential.userName + "|" + currentCredential.userPassword + "\n";
        }
        
        return serializedText;
    }

    // This function takes the long string of decrypted text and breaks it back down into individual logins
    vector<Credential> deserializeCredentials(const string& plainTextData) {
        vector<Credential> credentialList;
        stringstream textStream;
        string currentLine = "";
        
        textStream.str(plainTextData);
        
        while (getline(textStream, currentLine)) {
            if (currentLine.empty() == false) {
                Credential newCredential;
                size_t firstSeparator = 0;
                size_t secondSeparator = 0;
                
                firstSeparator = currentLine.find("|");
                
                if (firstSeparator != string::npos) {
                    newCredential.websiteName = currentLine.substr(0, firstSeparator);
                    secondSeparator = currentLine.find("|", firstSeparator + 1);
                    
                    if (secondSeparator != string::npos) {
                        newCredential.userName = currentLine.substr(firstSeparator + 1, secondSeparator - (firstSeparator + 1));
                        newCredential.userPassword = currentLine.substr(secondSeparator + 1);
                        
                        credentialList.push_back(newCredential);
                    }
                }
            }
        }
        
        return credentialList;
    }

    // This function saves your encrypted data, salt, and nonce to a file on your hard drive
    bool saveEncryptedVaultToFile(const string& filePath, const vector<unsigned char>& cipherText, const vector<unsigned char>& saltBuffer, const vector<unsigned char>& nonceBuffer) {
        ofstream fileStream;
        fileStream.open(filePath, ios::binary | ios::trunc);
        
        if (fileStream.is_open() == false) {
            cout << "[!] Error: Could not open the file to save the vault." << endl;
            return false;
        }
        
        // Write the 16-byte salt first
        fileStream.write(reinterpret_cast<const char*>(saltBuffer.data()), saltBuffer.size());
        
        // Write the 12-byte nonce second
        fileStream.write(reinterpret_cast<const char*>(nonceBuffer.data()), nonceBuffer.size());
        
        // Write the actual encrypted vault data last
        fileStream.write(reinterpret_cast<const char*>(cipherText.data()), cipherText.size());
        
        fileStream.close();
        cout << "[+] Successfully saved the encrypted vault to file." << endl;
        
        return true;
    }

    // This function reads your encrypted data, salt, and nonce from the file so we can unlock it
    bool loadEncryptedVaultFromFile(const string& filePath, vector<unsigned char>& cipherText, vector<unsigned char>& saltBuffer, vector<unsigned char>& nonceBuffer) {
        ifstream fileStream;
        fileStream.open(filePath, ios::binary | ios::ate);
        
        if (fileStream.is_open() == false) {
            cout << "[!] Error: Could not open the vault file. It might not exist yet." << endl;
            return false;
        }
        
        streamsize fileSize = 0;
        fileSize = fileStream.tellg();
        
        // The file must have at least 16 bytes for salt and 12 bytes for nonce (28 bytes total)
        if (fileSize < 28) {
            cout << "[!] Error: Vault file is corrupted or too small." << endl;
            fileStream.close();
            return false;
        }
        
        // Go back to the beginning of the file to start reading
        fileStream.seekg(0, ios::beg);
        
        // Read the 16-byte salt
        saltBuffer.resize(16);
        fileStream.read(reinterpret_cast<char*>(saltBuffer.data()), 16);
        
        // Read the 12-byte nonce
        nonceBuffer.resize(12);
        fileStream.read(reinterpret_cast<char*>(nonceBuffer.data()), 12);
        
        // Read the rest of the file as ciphertext
        streamsize cipherSize = 0;
        cipherSize = fileSize - 28;
        cipherText.resize(cipherSize);
        fileStream.read(reinterpret_cast<char*>(cipherText.data()), cipherSize);
        
        fileStream.close();
        cout << "[+] Successfully loaded the encrypted vault from file." << endl;
        
        return true;
    }

} // namespace Vault