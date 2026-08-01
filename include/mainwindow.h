#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDialog>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QString>
#include <vector>
#include <string>

#include "crypto.h"
#include "vault.h"
#include "loginview.h"

// ============================================================
// AddCredentialDialog
// ============================================================
class AddCredentialDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddCredentialDialog(QWidget* parentWidget = nullptr);
    QString getWebsiteName() const;
    QString getUserName() const;
    QString getUserPassword() const;
private:
    QLineEdit* websiteNameInput;
    QLineEdit* userNameInput;
    QLineEdit* userPasswordInput;
    QPushButton* saveButton;
    QPushButton* cancelButton;
    void setupUi();
};

// ============================================================
// ChangePasswordDialog
// ============================================================
class ChangePasswordDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ChangePasswordDialog(QWidget* parentWidget = nullptr);
    QString getOldPassword() const;
    QString getNewPassword() const;
private slots:
    void validateAndAccept();
private:
    QLineEdit* oldPasswordInput;
    QLineEdit* newPasswordInput;
    QLineEdit* confirmPasswordInput;
    QPushButton* saveButton;
    QPushButton* cancelButton;
    void setupUi();
};

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

private slots:
    void onUnlockRequested(const std::string& masterPassword);
    void onLockButtonClicked();
    void onAddCredentialButtonClicked();
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
    QLineEdit* searchBar;
    QPushButton* addCredentialButton;
    QPushButton* lockVaultButton;
    QPushButton* changePasswordButton;
    QPushButton* sortButton;
    
    // Pointers for dynamic UI updates
    QVBoxLayout* credentialsListLayout;
    QLabel* statsTextLabel;

    // --- In-memory sensitive state ---
    std::vector<unsigned char> derivedKey;
    std::vector<unsigned char> vaultSalt;
    std::vector<unsigned char> vaultNonce;
    std::vector<unsigned char> vaultCipherText;
    std::vector<Credential> currentCredentials;
    std::string vaultFilePath;
    
    bool isVaultUnlocked;
    bool sortAscending; // Tracks sorting state

    // --- UI construction helpers ---
    void setupUi();
    QWidget* buildVaultView();
    QWidget* createCredentialRow(const QString& website, const QString& url, const QString& username, const QString& password, const QString& category, const QString& iconStr);

    // --- Business logic helpers ---
    bool attemptUnlockVault(const std::string& masterPassword, std::string& errorMessage);
    void populateCredentialsTable(const QString& filterText = QString());
    void refreshCredentialsTable();
    void secureWipeSensitiveState();
    bool saveCurrentVaultState(); // New helper to write vault changes to disk
    void deleteCredential(const QString& website, const QString& username);
};

#endif // MAINWINDOW_H