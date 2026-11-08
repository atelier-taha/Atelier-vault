#include "vault.h"
#include "crypto.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <sodium.h>

using namespace std;

namespace Vault {

    QString getVaultsDirectory() {
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(appDataDir);
        QString vaultsPath = dir.absoluteFilePath("vaults");
        QDir vaultsDir(vaultsPath);
        if (!vaultsDir.exists()) {
            vaultsDir.mkpath(".");
        }
        return vaultsPath;
    }

    QString getVaultFilePath(const QString& vaultName) {
        QString cleanName = vaultName.trimmed();
        if (cleanName.isEmpty()) {
            cleanName = "Personal";
        }
        return getVaultsDirectory() + QDir::separator() + cleanName + ".atlv";
    }

    void migrateLegacyVaultIfNeeded() {
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(appDataDir);
        QString legacyFile = dir.absoluteFilePath("vault.dat");
        QString targetFile = getVaultFilePath("Personal");

        // Also check if legacy path was in generic data dir with "AteliersVault"
        if (!QFile::exists(legacyFile)) {
            QString altDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() + "AteliersVault";
            QString altLegacy = QDir(altDir).absoluteFilePath("vault.dat");
            if (QFile::exists(altLegacy)) {
                legacyFile = altLegacy;
            }
        }

        if (QFile::exists(legacyFile) && !QFile::exists(targetFile)) {
            QFile::copy(legacyFile, targetFile);
            cout << "[+] Successfully migrated legacy vault.dat to " << targetFile.toStdString() << endl;
        }
    }

    std::vector<QString> listAvailableVaults() {
        migrateLegacyVaultIfNeeded();
        std::vector<QString> vaults;
        QDir dir(getVaultsDirectory());
        QStringList filters;
        filters << "*.atlv";
        QFileInfoList list = dir.entryInfoList(filters, QDir::Files, QDir::Name);
        for (const QFileInfo& fileInfo : list) {
            vaults.push_back(fileInfo.completeBaseName());
        }
        return vaults;
    }

    string serializeCredentials(const vector<Credential>& credentialList) {
        if (credentialList.empty()) {
            return "[]";
        }
        
        QJsonArray jsonArray;
        for (const Credential& currentCredential : credentialList) {
            QJsonObject jsonObject;
            jsonObject["id"] = QString::fromStdString(currentCredential.id.empty() ? 
                QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString() : currentCredential.id);
            jsonObject["websiteName"] = QString::fromStdString(currentCredential.websiteName);
            jsonObject["url"] = QString::fromStdString(currentCredential.url);
            jsonObject["userName"] = QString::fromStdString(currentCredential.userName);
            jsonObject["userPassword"] = QString::fromStdString(currentCredential.userPassword);
            jsonObject["notes"] = QString::fromStdString(currentCredential.notes);
            jsonObject["category"] = QString::fromStdString(currentCredential.category.empty() ? "Logins" : currentCredential.category);
            jsonArray.append(jsonObject);
        }
        
        QJsonDocument jsonDoc(jsonArray);
        return jsonDoc.toJson(QJsonDocument::Compact).toStdString();
    }

    vector<Credential> deserializeCredentials(const string& plainTextData) {
        vector<Credential> credentialList;
        
        QByteArray jsonData = QByteArray::fromStdString(plainTextData);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        if (jsonDoc.isNull() || !jsonDoc.isArray()) {
            if (!jsonData.isEmpty()) {
                sodium_memzero(jsonData.data(), jsonData.size());
            }
            return credentialList;
        }
        
        QJsonArray jsonArray = jsonDoc.array();
        for (int i = 0; i < jsonArray.size(); ++i) {
            QJsonObject jsonObject = jsonArray[i].toObject();
            Credential newCredential;
            
            QString idStr = jsonObject["id"].toString();
            if (idStr.isEmpty()) {
                idStr = QUuid::createUuid().toString(QUuid::WithoutBraces);
            }
            newCredential.id = idStr.toStdString();
            newCredential.websiteName = jsonObject["websiteName"].toString().toStdString();
            newCredential.url = jsonObject["url"].toString().toStdString();
            newCredential.userName = jsonObject["userName"].toString().toStdString();
            newCredential.userPassword = jsonObject["userPassword"].toString().toStdString();
            newCredential.notes = jsonObject["notes"].toString().toStdString();
            
            QString catStr = jsonObject["category"].toString();
            if (catStr.isEmpty()) {
                catStr = "Logins";
            }
            newCredential.category = catStr.toStdString();
            
            credentialList.push_back(newCredential);
        }
        
        if (!jsonData.isEmpty()) {
            sodium_memzero(jsonData.data(), jsonData.size());
        }
        return credentialList;
    }

    bool saveEncryptedVaultToFile(const string& filePath, const vector<unsigned char>& cipherText, const vector<unsigned char>& saltBuffer, const vector<unsigned char>& nonceBuffer) {
        QString qFilePath = QString::fromStdString(filePath);
        
        // Rolling backup before atomic write
        if (QFile::exists(qFilePath)) {
            QString backupPath = qFilePath + ".bak";
            QFile::remove(backupPath);
            QFile::copy(qFilePath, backupPath);
        }

        QSaveFile saveFile(qFilePath);
        if (!saveFile.open(QIODevice::WriteOnly)) {
            cout << "[!] Error: Could not open the file to save the vault via QSaveFile." << endl;
            return false;
        }

        // 1. Write Magic Header (4 bytes)
        qint64 magicWritten = saveFile.write(Magic, 4);
        
        // 2. Write Version (4 bytes)
        uint32_t ver = CurrentVersion;
        qint64 versionWritten = saveFile.write(reinterpret_cast<const char*>(&ver), sizeof(ver));

        // 3. Write Salt, Nonce, and CipherText
        qint64 saltWritten = saveFile.write(reinterpret_cast<const char*>(saltBuffer.data()), saltBuffer.size());
        qint64 nonceWritten = saveFile.write(reinterpret_cast<const char*>(nonceBuffer.data()), nonceBuffer.size());
        qint64 cipherWritten = saveFile.write(reinterpret_cast<const char*>(cipherText.data()), cipherText.size());

        if (magicWritten != 4 ||
            versionWritten != static_cast<qint64>(sizeof(ver)) ||
            saltWritten != static_cast<qint64>(saltBuffer.size()) ||
            nonceWritten != static_cast<qint64>(nonceBuffer.size()) ||
            cipherWritten != static_cast<qint64>(cipherText.size())) {
            cout << "[!] Error: Partial write detected. Aborting save." << endl;
            saveFile.cancelWriting();
            return false;
        }

        if (!saveFile.commit()) {
            cout << "[!] Error: Atomic commit failed with QSaveFile." << endl;
            return false;
        }

        cout << "[+] Successfully saved the encrypted vault to file." << endl;
        return true;
    }

    bool loadEncryptedVaultFromFile(const string& filePath, vector<unsigned char>& cipherText, vector<unsigned char>& saltBuffer, vector<unsigned char>& nonceBuffer) {
        ifstream fileStream;
        fileStream.open(filePath, ios::binary | ios::ate);
        
        if (!fileStream.is_open()) {
            cout << "[!] Error: Could not open the vault file: " << filePath << endl;
            return false;
        }
        
        streamsize fileSize = fileStream.tellg();
        if (fileSize <= 0) {
            fileStream.close();
            return false;
        }
        
        fileStream.seekg(0, ios::beg);

        // Check for ATLV Magic Header
        bool hasMagic = false;
        if (fileSize >= 8) {
            char magicCheck[4] = {0};
            fileStream.read(magicCheck, 4);
            if (memcmp(magicCheck, Magic, 4) == 0) {
                hasMagic = true;
            }
        }

        if (hasMagic) {
            uint32_t version = 0;
            fileStream.read(reinterpret_cast<char*>(&version), sizeof(version));
            
            streamsize headerSize = 4 + sizeof(version) + Crypto::SaltLength + Crypto::NonceLength;
            if (fileSize < headerSize) {
                cout << "[!] Error: Vault file is corrupted or too small." << endl;
                fileStream.close();
                return false;
            }

            saltBuffer.resize(Crypto::SaltLength);
            fileStream.read(reinterpret_cast<char*>(saltBuffer.data()), Crypto::SaltLength);

            nonceBuffer.resize(Crypto::NonceLength);
            fileStream.read(reinterpret_cast<char*>(nonceBuffer.data()), Crypto::NonceLength);

            streamsize cipherSize = fileSize - headerSize;
            cipherText.resize(cipherSize);
            fileStream.read(reinterpret_cast<char*>(cipherText.data()), cipherSize);

            fileStream.close();
            cout << "[+] Successfully loaded encrypted vault with ATLV header (v" << version << ")." << endl;
            return true;
        }

        // Backwards compatibility: Phase 1 file without magic (40 bytes header)
        fileStream.seekg(0, ios::beg);
        streamsize phase1HeaderSize = static_cast<streamsize>(Crypto::SaltLength + Crypto::NonceLength);
        if (fileSize >= phase1HeaderSize) {
            saltBuffer.resize(Crypto::SaltLength);
            fileStream.read(reinterpret_cast<char*>(saltBuffer.data()), Crypto::SaltLength);

            nonceBuffer.resize(Crypto::NonceLength);
            fileStream.read(reinterpret_cast<char*>(nonceBuffer.data()), Crypto::NonceLength);

            streamsize cipherSize = fileSize - phase1HeaderSize;
            cipherText.resize(cipherSize);
            fileStream.read(reinterpret_cast<char*>(cipherText.data()), cipherSize);

            fileStream.close();
            cout << "[+] Loaded legacy vault format without magic header." << endl;
            return true;
        }

        // Legacy format (28 bytes header)
        if (fileSize >= 28) {
            saltBuffer.resize(16);
            fileStream.read(reinterpret_cast<char*>(saltBuffer.data()), 16);

            nonceBuffer.resize(12);
            fileStream.read(reinterpret_cast<char*>(nonceBuffer.data()), 12);

            streamsize cipherSize = fileSize - 28;
            cipherText.resize(cipherSize);
            fileStream.read(reinterpret_cast<char*>(cipherText.data()), cipherSize);

            fileStream.close();
            cout << "[+] Loaded legacy 28-byte header vault." << endl;
            return true;
        }

        fileStream.close();
        return false;
    }

} // namespace Vault