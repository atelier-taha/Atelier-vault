#ifndef DIALOGS_H
#define DIALOGS_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include <QProgressBar>
#include <QString>
#include <vector>
#include "vault.h"

class QTimer;

// ============================================================
// CapsLockWatcher (Real-Time Caps Lock Status Indicator)
// ============================================================
class CapsLockWatcher : public QObject
{
    Q_OBJECT

public:
    explicit CapsLockWatcher(QObject* parent = nullptr);
    ~CapsLockWatcher() override = default;

    // Registers a password QLineEdit and its Caps Lock warning badge
    void watch(QLineEdit* input, QLabel* warningLabel);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onPollTimer();

private:
    struct WatchedItem {
        QLineEdit* input;
        QLabel* label;
    };
    std::vector<WatchedItem> items;
    QTimer* pollTimer;
    QLineEdit* focusedInput;
    bool fallbackState;

    bool isCapsLockOn() const;
    void updateVisibility(QLineEdit* input, QLabel* label);
};

// ============================================================
// PasswordGeneratorDialog
// ============================================================
class PasswordGeneratorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasswordGeneratorDialog(QWidget* parent = nullptr);
    ~PasswordGeneratorDialog() override;

    QString getGeneratedPassword() const;
    static QString generateSecurePassword(int length, bool upper, bool lower, bool digits, bool symbols);

private slots:
    void onRegenerateClicked();
    void onCopyClicked();
    void onSliderValueChanged(int value);

private:
    QLineEdit* passwordDisplay;
    QSlider* lengthSlider;
    QLabel* lengthLabel;
    QCheckBox* upperCheck;
    QCheckBox* lowerCheck;
    QCheckBox* digitsCheck;
    QCheckBox* symbolsCheck;
    QPushButton* regenerateBtn;
    QPushButton* copyBtn;
    QPushButton* useBtn;
    QPushButton* cancelBtn;

    void setupUi();
    void generateAndDisplay();
    void wipeMemory();
};

// ============================================================
// CredentialDialog (Add & Edit Item)
// ============================================================
class CredentialDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CredentialDialog(QWidget* parent = nullptr, const Credential* existingCred = nullptr);
    ~CredentialDialog() override;

    Credential getCredential() const;

private slots:
    void onTogglePasswordVisibility();
    void onGeneratePasswordClicked();
    void validateAndAccept();

private:
    std::string credentialId;
    QLineEdit* websiteInput;
    QLineEdit* urlInput;
    QLineEdit* usernameInput;
    QLineEdit* passwordInput;
    QPushButton* eyeBtn;
    QPushButton* generateBtn;
    QLabel* capsLockWarning;
    CapsLockWatcher* capsWatcher;
    QComboBox* categoryCombo;
    QTextEdit* notesInput;
    QPushButton* saveBtn;
    QPushButton* cancelBtn;

    void setupUi(bool isEdit);
    void wipeMemory();
};

// ============================================================
// CreateVaultDialog (Setup & New Vault Onboarding)
// ============================================================
class CreateVaultDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateVaultDialog(QWidget* parent = nullptr);
    ~CreateVaultDialog() override;

    QString getVaultName() const;
    std::string getMasterPassword() const;

private slots:
    void onPasswordChanged();
    void onToggleVisibility();
    void validateAndAccept();

private:
    QLineEdit* vaultNameInput;
    QLineEdit* passwordInput;
    QLineEdit* confirmPasswordInput;
    QPushButton* eyeBtn;
    QLabel* capsLockWarning;
    CapsLockWatcher* capsWatcher;
    QLabel* matchLabel;
    QProgressBar* strengthBar;
    QLabel* strengthLabel;
    QPushButton* createBtn;
    QPushButton* cancelBtn;

    void setupUi();
    void updateStrength(const QString& password);
    void wipeMemory();
};

// ============================================================
// ChangeMasterPasswordDialog
// ============================================================
class ChangeMasterPasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeMasterPasswordDialog(QWidget* parent = nullptr);
    ~ChangeMasterPasswordDialog() override;

    std::string getOldPassword() const;
    std::string getNewPassword() const;

private slots:
    void onPasswordChanged();
    void validateAndAccept();

private:
    QLineEdit* oldPasswordInput;
    QLineEdit* newPasswordInput;
    QLineEdit* confirmPasswordInput;
    QLabel* capsLockWarning;
    CapsLockWatcher* capsWatcher;
    QLabel* matchLabel;
    QProgressBar* strengthBar;
    QPushButton* saveBtn;
    QPushButton* cancelBtn;

    void setupUi();
    void wipeMemory();
};

#endif // DIALOGS_H
