#include "loginview.h"
#include "dialogs.h"
#include "vault.h"
#include "crypto.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QFont>
#include <algorithm>
#include <sodium.h>

LoginView::LoginView(QWidget* parentWidget)
    : QWidget(parentWidget)
{
    this->setupUi();
    this->refreshVaultList();
}

void LoginView::setupUi()
{
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setStyleSheet("background-color: #121214; font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    // Main Login Card
    QFrame* card = new QFrame(this);
    card->setFixedSize(460, 520);
    card->setObjectName("loginCard");
    card->setStyleSheet(
        "#loginCard {"
        "   background-color: #18181B;"
        "   border: 1px solid #27272A;"
        "   border-radius: 12px;"
        "}"
    );

    // Subtle red ambient card glow
    QGraphicsDropShadowEffect* cardGlow = new QGraphicsDropShadowEffect(this);
    cardGlow->setBlurRadius(36);
    cardGlow->setColor(QColor(225, 29, 72, 35));
    cardGlow->setOffset(0, 4);
    card->setGraphicsEffect(cardGlow);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(36, 36, 36, 30);
    cardLayout->setSpacing(0);

    // Lock Icon Header
    QLabel* lockIcon = new QLabel("🔒", card);
    lockIcon->setAlignment(Qt::AlignCenter);
    lockIcon->setFixedSize(54, 54);
    lockIcon->setStyleSheet(
        "color: #E11D48;"
        "font-size: 22px;"
        "border: 1px solid #3F151F;"
        "border-radius: 27px;"
        "background-color: #200B11;"
    );

    QHBoxLayout* iconLayout = new QHBoxLayout();
    iconLayout->setAlignment(Qt::AlignCenter);
    iconLayout->addWidget(lockIcon);

    // Title & Subtitle
    QLabel* titleLabel = new QLabel("Atelier's Vault", card);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 24px; font-weight: bold; margin-top: 12px; background: transparent;");

    QLabel* subtitleLabel = new QLabel("SECURE MULTI-VAULT MANAGER", card);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    QFont subtitleFont = subtitleLabel->font();
    subtitleFont.setPixelSize(10);
    subtitleFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.5);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setStyleSheet("color: #71717A; margin-bottom: 24px; background: transparent;");

    // Vault Selector Header row
    QHBoxLayout* vaultHeaderLayout = new QHBoxLayout();
    QLabel* vaultLabel = new QLabel("SELECT VAULT", card);
    vaultLabel->setStyleSheet("color: #A1A1AA; font-size: 11px; font-weight: bold; background: transparent;");

    this->newVaultBtn = new QPushButton("+ New Vault", card);
    this->newVaultBtn->setCursor(Qt::PointingHandCursor);
    this->newVaultBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   border: none;"
        "   color: #E11D48;"
        "   font-size: 11px;"
        "   font-weight: 600;"
        "   padding: 0px 4px;"
        "}"
        "QPushButton:hover {"
        "   color: #FB7185;"
        "   text-decoration: underline;"
        "}"
    );

    vaultHeaderLayout->addWidget(vaultLabel);
    vaultHeaderLayout->addStretch();
    vaultHeaderLayout->addWidget(this->newVaultBtn);

    // Vault Dropdown
    this->vaultSelector = new QComboBox(card);
    this->vaultSelector->setFixedHeight(42);
    this->vaultSelector->setStyleSheet(
        "QComboBox {"
        "   background-color: #202024;"
        "   border: 1px solid #27272A;"
        "   border-radius: 6px;"
        "   color: #F4F4F5;"
        "   padding: 6px 12px;"
        "   font-size: 13px;"
        "   font-weight: 500;"
        "   margin-bottom: 16px;"
        "}"
        "QComboBox:focus {"
        "   border: 1px solid #E11D48;"
        "}"
        "QComboBox::drop-down {"
        "   border: none;"
        "   padding-right: 10px;"
        "}"
        "QComboBox QAbstractItemView {"
        "   background-color: #18181B;"
        "   color: #F4F4F5;"
        "   selection-background-color: #27272A;"
        "   border: 1px solid #27272A;"
        "}"
    );

    // Master Password Label
    QLabel* inputLabel = new QLabel("MASTER PASSWORD", card);
    inputLabel->setStyleSheet("color: #A1A1AA; font-size: 11px; font-weight: bold; margin-bottom: 5px; background: transparent;");

    // Input Box Container
    QFrame* inputContainer = new QFrame(card);
    inputContainer->setFixedHeight(44);
    inputContainer->setStyleSheet(
        "QFrame {"
        "   background-color: #202024;"
        "   border: 1px solid #27272A;"
        "   border-radius: 6px;"
        "}"
        "QFrame:focus-within {"
        "   border: 1px solid #E11D48;"
        "}"
    );

    QHBoxLayout* inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(12, 0, 8, 0);

    this->passwordInput = new QLineEdit(inputContainer);
    this->passwordInput->setEchoMode(QLineEdit::Password);
    this->passwordInput->setMaxLength(1024);
    this->passwordInput->setPlaceholderText("Enter Master Password...");
    this->passwordInput->setStyleSheet("background: transparent; border: none; color: #FFFFFF; font-size: 14px; font-family: monospace;");

    this->eyeButton = new QPushButton("👁", inputContainer);
    this->eyeButton->setCursor(Qt::PointingHandCursor);
    this->eyeButton->setToolTip("Show Password");
    this->eyeButton->setFixedSize(28, 28);
    this->eyeButton->setStyleSheet("background: transparent; border: none; color: #71717A; font-size: 15px; font-family: 'Segoe UI Emoji', 'Segoe UI Symbol', sans-serif; padding: 0px;");

    inputLayout->addWidget(this->passwordInput);
    inputLayout->addWidget(this->eyeButton);

    // Caps Lock Warning
    this->capsLockWarning = new QLabel("⚠️ Caps Lock is on", card);
    this->capsLockWarning->setAlignment(Qt::AlignCenter);
    this->capsLockWarning->setStyleSheet(
        "color: #FBBF24; background-color: #261E0A; border: 1px solid #78350F; "
        "border-radius: 4px; padding: 4px 8px; font-size: 11px; font-weight: 600; margin-top: 6px;"
    );
    this->capsLockWarning->setVisible(false);

    this->capsWatcher = new CapsLockWatcher(this);
    this->capsWatcher->watch(this->passwordInput, this->capsLockWarning);

    // Unlock Button
    this->unlockButton = new QPushButton("🔓 Unlock Vault", card);
    this->unlockButton->setFixedHeight(44);
    this->unlockButton->setCursor(Qt::PointingHandCursor);
    this->unlockButton->setStyleSheet(
        "QPushButton {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #E11D48, stop:1 #BE123C);"
        "   color: #FFFFFF;"
        "   border: none;"
        "   border-radius: 6px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   margin-top: 16px;"
        "}"
        "QPushButton:hover {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #F43F5E, stop:1 #E11D48);"
        "}"
        "QPushButton:pressed {"
        "   background: #9F1239;"
        "}"
    );

    QGraphicsDropShadowEffect* buttonGlow = new QGraphicsDropShadowEffect(this);
    buttonGlow->setBlurRadius(20);
    buttonGlow->setColor(QColor(225, 29, 72, 80));
    buttonGlow->setOffset(0, 3);
    this->unlockButton->setGraphicsEffect(buttonGlow);

    // Error Label
    this->errorLabel = new QLabel("", card);
    this->errorLabel->setAlignment(Qt::AlignCenter);
    this->errorLabel->setStyleSheet("color: #FB7185; font-size: 12px; margin-top: 6px; background: transparent; font-weight: 500;");
    this->errorLabel->setFixedHeight(22);

    // Footer
    QFrame* separator = new QFrame(card);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background-color: #27272A; margin-top: 8px; margin-bottom: 12px;");

    QLabel* footerLabel = new QLabel("🛡️ Encryption: Argon2id + XChaCha20-Poly1305", card);
    footerLabel->setAlignment(Qt::AlignCenter);
    footerLabel->setStyleSheet("color: #71717A; font-family: monospace; font-size: 11px; background: transparent;");

    // Assemble Card Layout
    cardLayout->addLayout(iconLayout);
    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(subtitleLabel);
    cardLayout->addLayout(vaultHeaderLayout);
    cardLayout->addWidget(this->vaultSelector);
    cardLayout->addWidget(inputLabel);
    cardLayout->addWidget(inputContainer);
    cardLayout->addWidget(this->capsLockWarning);
    cardLayout->addWidget(this->unlockButton);
    cardLayout->addWidget(this->errorLabel);
    cardLayout->addStretch();
    cardLayout->addWidget(separator);
    cardLayout->addWidget(footerLabel);

    mainLayout->addWidget(card);

    // Signal Connections
    connect(this->unlockButton, &QPushButton::clicked, this, &LoginView::onUnlockTriggered);
    connect(this->passwordInput, &QLineEdit::returnPressed, this, &LoginView::onUnlockTriggered);
    connect(this->eyeButton, &QPushButton::clicked, this, &LoginView::onTogglePasswordVisibility);
    connect(this->newVaultBtn, &QPushButton::clicked, this, &LoginView::onNewVaultClicked);
}

void LoginView::refreshVaultList(const QString& selectVault)
{
    std::vector<QString> vaults = Vault::listAvailableVaults();
    this->vaultSelector->clear();

    if (vaults.empty()) {
        this->vaultSelector->addItem("Personal");
    } else {
        for (const QString& v : vaults) {
            this->vaultSelector->addItem("🗄️ " + v, v);
        }
    }

    if (!selectVault.isEmpty()) {
        int idx = this->vaultSelector->findData(selectVault);
        if (idx >= 0) {
            this->vaultSelector->setCurrentIndex(idx);
        } else {
            int textIdx = this->vaultSelector->findText(selectVault);
            if (textIdx >= 0) this->vaultSelector->setCurrentIndex(textIdx);
        }
    }
}

QString LoginView::getSelectedVault() const
{
    QVariant vaultData = this->vaultSelector->currentData();
    if (!vaultData.isNull() && !vaultData.toString().isEmpty()) {
        return vaultData.toString();
    }
    QString text = this->vaultSelector->currentText();
    return text.remove("🗄️ ").trimmed();
}

void LoginView::setSelectedVault(const QString& vaultName)
{
    int idx = this->vaultSelector->findData(vaultName);
    if (idx >= 0) {
        this->vaultSelector->setCurrentIndex(idx);
    } else {
        int textIdx = this->vaultSelector->findText("🗄️ " + vaultName);
        if (textIdx >= 0) this->vaultSelector->setCurrentIndex(textIdx);
    }
}

void LoginView::onNewVaultClicked()
{
    CreateVaultDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.getVaultName();
        std::string pass = dialog.getMasterPassword();
        
        emit createVaultRequested(name, pass);

        Crypto::secureClean(pass);
    }
}

void LoginView::onUnlockTriggered()
{
    this->errorLabel->clear();
    QString vaultName = this->getSelectedVault();
    std::string masterPassword = this->passwordInput->text().toStdString();
    
    this->passwordInput->clear();

    emit unlockRequested(vaultName, masterPassword);

    Crypto::secureClean(masterPassword);
}

void LoginView::onTogglePasswordVisibility()
{
    if (this->passwordInput->echoMode() == QLineEdit::Password)
    {
        this->passwordInput->setEchoMode(QLineEdit::Normal);
        this->eyeButton->setText("🙈");
        this->eyeButton->setToolTip("Hide Password");
        this->eyeButton->setStyleSheet("background: transparent; border: none; color: #E11D48; font-size: 15px; font-family: 'Segoe UI Emoji', 'Segoe UI Symbol', sans-serif; padding: 0px;");
    }
    else
    {
        this->passwordInput->setEchoMode(QLineEdit::Password);
        this->eyeButton->setText("👁");
        this->eyeButton->setToolTip("Show Password");
        this->eyeButton->setStyleSheet("background: transparent; border: none; color: #71717A; font-size: 15px; font-family: 'Segoe UI Emoji', 'Segoe UI Symbol', sans-serif; padding: 0px;");
    }
}

void LoginView::showErrorMessage(const QString& errorMessage)
{
    this->errorLabel->setText(errorMessage);
}

void LoginView::clearInput()
{
    this->passwordInput->clear();
    this->errorLabel->clear();
    if (this->capsLockWarning) {
        this->capsLockWarning->setVisible(false);
    }
}