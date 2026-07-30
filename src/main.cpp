#include <iostream>
#include <string>
#include <vector>
#include "crypto.h"
#include "vault.h"
#include "ui.h"

using namespace std;

// This function encrypts and saves the current list of credentials back to the file
bool saveCurrentVault(const vector<Credential>& credentialList, const vector<unsigned char>& derivedKey, const vector<unsigned char>& saltBuffer, const string& filePath) {
    string plainTextString = "";
    plainTextString = Vault::serializeCredentials(credentialList);

    vector<unsigned char> plainTextBytes;
    plainTextBytes.assign(plainTextString.begin(), plainTextString.end());

    vector<unsigned char> nonceBuffer;
    nonceBuffer = Crypto::generateNonce();

    vector<unsigned char> cipherText;
    cipherText = Crypto::encryptVault(plainTextBytes, derivedKey, nonceBuffer);

    bool saveStatus = false;
    saveStatus = Vault::saveEncryptedVaultToFile(filePath, cipherText, saltBuffer, nonceBuffer);

    // Wipe the plain text from RAM now that it is safely encrypted on disk
    Crypto::secureClean(plainTextBytes);

    return saveStatus;
}

// This function attempts to unlock an existing vault or sets up a brand new one
bool initializeOrUnlockVault(const string& filePath, vector<Credential>& credentialList, vector<unsigned char>& derivedKey, vector<unsigned char>& saltBuffer) {
    string masterPassword = "";
    masterPassword = Ui::getHiddenInput("Enter your Master Password: ");

    vector<unsigned char> cipherText;
    vector<unsigned char> nonceBuffer;

    bool fileExists = false;
    fileExists = Vault::loadEncryptedVaultFromFile(filePath, cipherText, saltBuffer, nonceBuffer);

    if (fileExists == true) {
        cout << "[+] Verifying password and unlocking vault..." << endl;
        
        derivedKey = Crypto::deriveKey(masterPassword, saltBuffer);

        vector<unsigned char> plainTextBytes;
        plainTextBytes = Crypto::decryptVault(cipherText, derivedKey, nonceBuffer);

        if (plainTextBytes.empty() == true) {
            cout << "[!] Error: Incorrect master password or corrupted vault data!" << endl;
            Crypto::secureClean(derivedKey);
            return false;
        }

        string plainTextString = "";
        plainTextString.assign(plainTextBytes.begin(), plainTextBytes.end());
        
        credentialList = Vault::deserializeCredentials(plainTextString);

        Crypto::secureClean(plainTextBytes);
        
        cout << "[+] Vault unlocked successfully." << endl;
        Ui::promptContinue();
        return true;
        
    } else {
        cout << "[+] No existing vault found. Initializing a new one." << endl;
        
        saltBuffer = Crypto::generateSalt();
        derivedKey = Crypto::deriveKey(masterPassword, saltBuffer);

        cout << "[+] New vault created." << endl;
        Ui::promptContinue();
        return true;
    }
}

// This function handles the main interactive menu loop for the user
void runMainApplicationLoop(const string& filePath, vector<Credential>& credentialList, vector<unsigned char>& derivedKey, vector<unsigned char>& saltBuffer) {
    bool keepRunning = false;
    keepRunning = true;
    
    int userChoice = 0;

    while (keepRunning == true) {
        Ui::displayHeader();
        userChoice = Ui::getMainMenuChoice();

        if (userChoice == 1) {
            Ui::displayVaultContents(credentialList);
            Ui::promptContinue();
            
        } else if (userChoice == 2) {
            Credential newLogin;
            newLogin = Ui::getNewCredential();
            
            credentialList.push_back(newLogin);

            bool saveSuccess = false;
            saveSuccess = saveCurrentVault(credentialList, derivedKey, saltBuffer, filePath);

            if (saveSuccess == true) {
                cout << "[+] Credential added and vault safely encrypted to disk." << endl;
            } else {
                cout << "[!] Failed to save the vault to disk." << endl;
            }
            Ui::promptContinue();
            
        } else if (userChoice == 3) {
            cout << "[+] Locking vault and securely erasing memory. Goodbye!" << endl;
            keepRunning = false;
            
        } else {
            cout << "[!] Invalid choice. Please select 1, 2, or 3." << endl;
            Ui::promptContinue();
        }
    }
}

// This function acts as the entry point, keeping all logic delegated to helper functions
int main() {
    bool cryptoStatus = false;
    cryptoStatus = Crypto::initializeCrypto();

    if (cryptoStatus == false) {
        return 1;
    }

    // Always target the build/ directory regardless of where the program is launched from
    string homeDir = getenv("HOME");
    string vaultFilePath = homeDir + "/Projects/password_manager/build/vault.dat";

    vector<Credential> userVault;
    vector<unsigned char> derivedKey;
    vector<unsigned char> vaultSalt;

    Ui::displayHeader();

    bool unlockStatus = false;
    unlockStatus = initializeOrUnlockVault(vaultFilePath, userVault, derivedKey, vaultSalt);

    if (unlockStatus == true) {
        runMainApplicationLoop(vaultFilePath, userVault, derivedKey, vaultSalt);
    }

    

    // Final security measure to ensure keys are wiped before the program fully closes
    Crypto::secureClean(derivedKey);
    Crypto::secureClean(vaultSalt);

    return 0;
}