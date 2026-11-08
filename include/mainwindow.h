#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <QString>
#include <QByteArray>
#include <vector>
#include <string>

class QTimer;

#include "crypto.h"
#include "vault.h"
#include "loginview.h"
#include "dialogs.h"

// ============================================================
// MainWindow
// ============================================================
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parentWidget = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* closeEventPointer) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onUnlockRequested(const QString& vaultName, const std::string& masterPassword);
    void onCreateVaultRequested(const QString& vaultName, const std::string& masterPassword);
    void onVaultSwitchSelected(int index);
    void onLockButtonClicked();
    void onAddCredentialButtonClicked();
    void onGeneratePasswordButtonClicked();
    void onChangePasswordClicked();
    void onSearchTextChanged(const QString& searchText);
    void onSortButtonClicked();

private:
    // --- Top-level layout ---
    QStackedWidget* stackedWidget;

    // --- Views ---
    LoginView* loginView;
    QWidget* vaultView;

    // --- Vault view widgets ---
    QComboBox* vaultSwitcher;
    QLineEdit* searchBar;
    QPushButton* addCredentialButton;
    QPushButton* generatePasswordButton;
    QPushButton* lockVaultButton;
    QPushButton* changePasswordButton;
    QPushButton* sortButton;
    
    // Pointers for dynamic UI updates
    QVBoxLayout* credentialsListLayout;
    QLabel* statsTextLabel;

    // --- Security Timers & State ---
    QTimer* inactivityTimer;
    QByteArray lastCopiedHash;

    // --- In-memory sensitive state ---
    std::vector<unsigned char> derivedKey;
    std::vector<unsigned char> vaultSalt;
    std::vector<unsigned char> vaultNonce;
    std::vector<unsigned char> vaultCipherText;
    std::vector<Credential> currentCredentials;
    
    QString activeVaultName;
    std::string vaultFilePath;
    
    bool isVaultUnlocked;
    bool sortAscending; // Tracks sorting state

    // --- UI construction helpers ---
    void setupUi();
    QWidget* buildVaultView();
    QWidget* createCredentialRow(const Credential& credential);

    // --- Business logic helpers ---
    bool attemptUnlockVault(const QString& vaultName, const std::string& masterPassword, std::string& errorMessage);
    bool createAndInitializeVault(const QString& vaultName, const std::string& masterPassword, std::string& errorMessage);
    void populateCredentialsTable(const QString& filterText = QString());
    void refreshCredentialsTable();
    void refreshVaultSwitcher();
    void secureWipeSensitiveState();
    bool saveCurrentVaultState();
    
    void editCredential(const std::string& credentialId);
    void deleteCredential(const std::string& credentialId);
    void copyWithAutoClear(const QString& text);
    void openCredentialUrl(const QString& rawUrl);
};

#endif // MAINWINDOW_H