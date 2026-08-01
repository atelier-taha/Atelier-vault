#ifndef LOGINVIEW_H
#define LOGINVIEW_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QString>
#include <string>

class LoginView : public QWidget
{
    Q_OBJECT

public:
    explicit LoginView(QWidget* parentWidget = nullptr);

    // Allows MainWindow to display inline error messages (e.g. wrong password)
    void showErrorMessage(const QString& errorMessage);

    // Clears input text and wipes the error label
    void clearInput();

signals:
    // Emitted when the user triggers an unlock request
    void unlockRequested(const std::string& masterPassword);

private slots:
    void onUnlockTriggered();
    void onTogglePasswordVisibility();

private:
    QLineEdit* passwordInput;
    QPushButton* unlockButton;
    QPushButton* eyeButton;
    QLabel* errorLabel;

    void setupUi();
};

#endif // LOGINVIEW_H