#ifndef LOGINVIEW_H
#define LOGINVIEW_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QString>
#include <string>

class CapsLockWatcher;

class LoginView : public QWidget
{
    Q_OBJECT

public:
    explicit LoginView(QWidget* parentWidget = nullptr);

    // Allows MainWindow to display inline error messages (e.g. wrong password)
    void showErrorMessage(const QString& errorMessage);

    // Clears input text and wipes the error label
    void clearInput();

    // Refreshes the available vaults in the dropdown
    void refreshVaultList(const QString& selectVault = QString());
    QString getSelectedVault() const;
    void setSelectedVault(const QString& vaultName);

signals:
    // Emitted when the user triggers an unlock request for a specific vault
    void unlockRequested(const QString& vaultName, const std::string& masterPassword);
    
    // Emitted when user completes new vault creation
    void createVaultRequested(const QString& vaultName, const std::string& masterPassword);

private slots:
    void onUnlockTriggered();
    void onTogglePasswordVisibility();
    void onNewVaultClicked();

private:
    QComboBox* vaultSelector;
    QPushButton* newVaultBtn;
    QLineEdit* passwordInput;
    QPushButton* unlockButton;
    QPushButton* eyeButton;
    QLabel* capsLockWarning;
    CapsLockWatcher* capsWatcher;
    QLabel* errorLabel;

    void setupUi();
};

#endif // LOGINVIEW_H