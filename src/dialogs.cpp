#include "dialogs.h"
#include "crypto.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QTimer>
#include <QCryptographicHash>
#include <QUuid>
#include <QKeyEvent>
#include <sodium.h>

#if defined(Q_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

    const QString kDialogStyle =
        "QDialog {"
        "   background-color: #121214;"
        "   color: #F4F4F5;"
        "   font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;"
        "}"
        "QLabel {"
        "   color: #A1A1AA;"
        "   font-size: 12px;"
        "   font-weight: 500;"
        "}"
        "QLineEdit, QTextEdit, QComboBox {"
        "   background-color: #18181B;"
        "   border: 1px solid #27272A;"
        "   border-radius: 6px;"
        "   color: #F4F4F5;"
        "   padding: 8px 12px;"
        "   font-size: 13px;"
        "   selection-background-color: #D32F2F;"
        "}"
        "QLineEdit:focus, QTextEdit:focus, QComboBox:focus {"
        "   border: 1px solid #E11D48;"
        "   background-color: #1E1E24;"
        "}"
        "QComboBox::drop-down {"
        "   border: none;"
        "   padding-right: 8px;"
        "}"
        "QComboBox QAbstractItemView {"
        "   background-color: #18181B;"
        "   color: #F4F4F5;"
        "   selection-background-color: #27272A;"
        "   border: 1px solid #27272A;"
        "}"
        "QCheckBox {"
        "   color: #D4D4D8;"
        "   font-size: 12px;"
        "   spacing: 6px;"
        "}"
        "QCheckBox::indicator {"
        "   width: 16px;"
        "   height: 16px;"
        "   border: 1px solid #3F3F46;"
        "   border-radius: 4px;"
        "   background-color: #18181B;"
        "}"
        "QCheckBox::indicator:checked {"
        "   background-color: #E11D48;"
        "   border-color: #E11D48;"
        "}"
        "QSlider::groove:horizontal {"
        "   height: 6px;"
        "   background: #27272A;"
        "   border-radius: 3px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "   background: #E11D48;"
        "   border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "   background: #FFFFFF;"
        "   width: 16px;"
        "   margin-top: -5px;"
        "   margin-bottom: -5px;"
        "   border-radius: 8px;"
        "}"
        "QPushButton {"
        "   border-radius: 6px;"
        "   padding: 8px 16px;"
        "   font-size: 13px;"
        "   font-weight: 600;"
        "}"
        "QPushButton#primaryBtn {"
        "   background-color: #D32F2F;"
        "   color: #FFFFFF;"
        "   border: none;"
        "}"
        "QPushButton#primaryBtn:hover {"
        "   background-color: #B71C1C;"
        "}"
        "QPushButton#secondaryBtn {"
        "   background-color: #18181B;"
        "   color: #A1A1AA;"
        "   border: 1px solid #27272A;"
        "}"
        "QPushButton#secondaryBtn:hover {"
        "   background-color: #27272A;"
        "   color: #FFFFFF;"
        "}"
        "QPushButton#iconBtn {"
        "   background-color: #18181B;"
        "   color: #A1A1AA;"
        "   border: 1px solid #27272A;"
        "   border-radius: 6px;"
        "   padding: 0px;"
        "   font-size: 15px;"
        "   font-family: 'Segoe UI Emoji', 'Segoe UI Symbol', 'Noto Color Emoji', 'Apple Color Emoji', sans-serif;"
        "}"
        "QPushButton#iconBtn:hover {"
        "   background-color: #27272A;"
        "   color: #FFFFFF;"
        "   border-color: #3F3F46;"
        "}"
        "QPushButton#iconBtn:pressed {"
        "   background-color: #202024;"
        "}";

} // namespace

// ============================================================
// CapsLockWatcher Implementation
// ============================================================
CapsLockWatcher::CapsLockWatcher(QObject* parent)
    : QObject(parent)
    , pollTimer(new QTimer(this))
    , focusedInput(nullptr)
    , fallbackState(false)
{
    this->pollTimer->setInterval(150);
    connect(this->pollTimer, &QTimer::timeout, this, &CapsLockWatcher::onPollTimer);
}

void CapsLockWatcher::watch(QLineEdit* input, QLabel* warningLabel)
{
    if (!input || !warningLabel) return;

    input->installEventFilter(this);
    warningLabel->setVisible(false);

    this->items.push_back({input, warningLabel});

    if (input->hasFocus()) {
        this->focusedInput = input;
        this->updateVisibility(input, warningLabel);
        if (!this->pollTimer->isActive()) {
            this->pollTimer->start();
        }
    }
}

bool CapsLockWatcher::isCapsLockOn() const
{
#if defined(Q_OS_WIN)
    return (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
#else
    return this->fallbackState;
#endif
}

void CapsLockWatcher::updateVisibility(QLineEdit* input, QLabel* label)
{
    if (!input || !label) return;
    bool shouldShow = input->hasFocus() && this->isCapsLockOn();
    if (label->isVisible() != shouldShow) {
        label->setVisible(shouldShow);
    }
}

void CapsLockWatcher::onPollTimer()
{
    if (!this->focusedInput) {
        this->pollTimer->stop();
        return;
    }

    for (const auto& item : this->items) {
        if (item.input == this->focusedInput) {
            this->updateVisibility(item.input, item.label);
            return;
        }
    }
}

bool CapsLockWatcher::eventFilter(QObject* watched, QEvent* event)
{
    QLineEdit* input = qobject_cast<QLineEdit*>(watched);
    if (!input) {
        return QObject::eventFilter(watched, event);
    }

    QLabel* matchedLabel = nullptr;
    for (const auto& item : this->items) {
        if (item.input == input) {
            matchedLabel = item.label;
            break;
        }
    }

    if (!matchedLabel) {
        return QObject::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::FocusIn:
        this->focusedInput = input;
        this->updateVisibility(input, matchedLabel);
        if (!this->pollTimer->isActive()) {
            this->pollTimer->start();
        }
        break;

    case QEvent::FocusOut:
        if (this->focusedInput == input) {
            this->focusedInput = nullptr;
        }
        // Use zero-delay callback so tabbing between watched inputs doesn't cause badge flicker
        QTimer::singleShot(0, this, [this, matchedLabel]() {
            bool anyFocused = false;
            for (const auto& item : this->items) {
                if (item.input && item.input->hasFocus()) {
                    anyFocused = true;
                    this->focusedInput = item.input;
                    this->updateVisibility(item.input, item.label);
                    break;
                }
            }
            if (!anyFocused && matchedLabel) {
                matchedLabel->setVisible(false);
                this->pollTimer->stop();
            }
        });
        break;

    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_CapsLock && event->type() == QEvent::KeyPress) {
            this->fallbackState = !this->fallbackState;
        } else if (!keyEvent->text().isEmpty()) {
            QChar ch = keyEvent->text().at(0);
            if (ch.isLetter()) {
                bool isShift = (keyEvent->modifiers() & Qt::ShiftModifier) != 0;
                if (ch.isUpper() && !isShift) {
                    this->fallbackState = true;
                } else if (ch.isLower() && isShift) {
                    this->fallbackState = true;
                } else if (ch.isLower() && !isShift) {
                    this->fallbackState = false;
                } else if (ch.isUpper() && isShift) {
                    this->fallbackState = false;
                }
            }
        }
        this->updateVisibility(input, matchedLabel);
        break;
    }

    default:
        break;
    }

    return QObject::eventFilter(watched, event);
}

// ============================================================
// PasswordGeneratorDialog
// ============================================================
PasswordGeneratorDialog::PasswordGeneratorDialog(QWidget* parent)
    : QDialog(parent)
{
    this->setupUi();
    this->onRegenerateClicked();
}

PasswordGeneratorDialog::~PasswordGeneratorDialog()
{
    this->wipeMemory();
}

void PasswordGeneratorDialog::setupUi()
{
    this->setWindowTitle("Password Generator");
    this->setFixedWidth(420);
    this->setStyleSheet(kDialogStyle);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    // Header Title
    QLabel* titleLabel = new QLabel("⚡ Generate Secure Password", this);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 16px; font-weight: bold; margin-bottom: 4px;");
    mainLayout->addWidget(titleLabel);

    // Password display container
    this->passwordDisplay = new QLineEdit(this);
    this->passwordDisplay->setReadOnly(true);
    this->passwordDisplay->setStyleSheet(
        "QLineEdit {"
        "   background-color: #18181B;"
        "   border: 1px solid #27272A;"
        "   color: #4ADE80;"
        "   font-family: monospace;"
        "   font-size: 15px;"
        "   font-weight: bold;"
        "   padding: 10px 12px;"
        "   letter-spacing: 1px;"
        "}"
    );

    QHBoxLayout* displayLayout = new QHBoxLayout();
    displayLayout->addWidget(this->passwordDisplay);

    this->regenerateBtn = new QPushButton("🔄", this);
    this->regenerateBtn->setToolTip("Regenerate Password");
    this->regenerateBtn->setFixedSize(38, 38);
    this->regenerateBtn->setObjectName("iconBtn");
    this->regenerateBtn->setCursor(Qt::PointingHandCursor);
    displayLayout->addWidget(this->regenerateBtn);

    mainLayout->addLayout(displayLayout);

    // Slider for Length
    QHBoxLayout* lengthHeaderLayout = new QHBoxLayout();
    QLabel* lengthTitle = new QLabel("Length:", this);
    this->lengthLabel = new QLabel("20", this);
    this->lengthLabel->setStyleSheet("color: #FFFFFF; font-weight: bold;");
    lengthHeaderLayout->addWidget(lengthTitle);
    lengthHeaderLayout->addStretch();
    lengthHeaderLayout->addWidget(this->lengthLabel);
    mainLayout->addLayout(lengthHeaderLayout);

    this->lengthSlider = new QSlider(Qt::Horizontal, this);
    this->lengthSlider->setRange(8, 64);
    this->lengthSlider->setValue(20);
    mainLayout->addWidget(this->lengthSlider);

    // Options Checkboxes
    QGridLayout* optionsLayout = new QGridLayout();
    optionsLayout->setSpacing(10);

    this->upperCheck = new QCheckBox("Uppercase (A-Z)", this);
    this->upperCheck->setChecked(true);
    this->lowerCheck = new QCheckBox("Lowercase (a-z)", this);
    this->lowerCheck->setChecked(true);
    this->digitsCheck = new QCheckBox("Numbers (0-9)", this);
    this->digitsCheck->setChecked(true);
    this->symbolsCheck = new QCheckBox("Symbols (!@#$)", this);
    this->symbolsCheck->setChecked(true);

    optionsLayout->addWidget(this->upperCheck, 0, 0);
    optionsLayout->addWidget(this->lowerCheck, 0, 1);
    optionsLayout->addWidget(this->digitsCheck, 1, 0);
    optionsLayout->addWidget(this->symbolsCheck, 1, 1);
    mainLayout->addLayout(optionsLayout);

    mainLayout->addSpacing(8);

    // Action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    this->copyBtn = new QPushButton("📋 Copy", this);
    this->copyBtn->setObjectName("secondaryBtn");
    this->copyBtn->setCursor(Qt::PointingHandCursor);

    this->cancelBtn = new QPushButton("Cancel", this);
    this->cancelBtn->setObjectName("secondaryBtn");
    this->cancelBtn->setCursor(Qt::PointingHandCursor);

    this->useBtn = new QPushButton("Use Password", this);
    this->useBtn->setObjectName("primaryBtn");
    this->useBtn->setCursor(Qt::PointingHandCursor);
    this->useBtn->setDefault(true);

    buttonLayout->addWidget(this->copyBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(this->cancelBtn);
    buttonLayout->addWidget(this->useBtn);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(this->lengthSlider, &QSlider::valueChanged, this, &PasswordGeneratorDialog::onSliderValueChanged);
    connect(this->regenerateBtn, &QPushButton::clicked, this, &PasswordGeneratorDialog::onRegenerateClicked);
    connect(this->upperCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onRegenerateClicked);
    connect(this->lowerCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onRegenerateClicked);
    connect(this->digitsCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onRegenerateClicked);
    connect(this->symbolsCheck, &QCheckBox::toggled, this, &PasswordGeneratorDialog::onRegenerateClicked);
    connect(this->copyBtn, &QPushButton::clicked, this, &PasswordGeneratorDialog::onCopyClicked);
    connect(this->cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(this->useBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void PasswordGeneratorDialog::onSliderValueChanged(int value)
{
    this->lengthLabel->setText(QString::number(value));
    this->onRegenerateClicked();
}

void PasswordGeneratorDialog::onRegenerateClicked()
{
    int len = this->lengthSlider->value();
    bool upper = this->upperCheck->isChecked();
    bool lower = this->lowerCheck->isChecked();
    bool digits = this->digitsCheck->isChecked();
    bool symbols = this->symbolsCheck->isChecked();

    if (!upper && !lower && !digits && !symbols) {
        this->lowerCheck->setChecked(true);
        lower = true;
    }

    QString pass = generateSecurePassword(len, upper, lower, digits, symbols);
    this->passwordDisplay->setText(pass);
}

void PasswordGeneratorDialog::onCopyClicked()
{
    QString secret = this->passwordDisplay->text();
    if (secret.isEmpty()) return;

    QByteArray utf8Secret = secret.toUtf8();
    QByteArray hash = QCryptographicHash::hash(utf8Secret, QCryptographicHash::Sha256);
    if (!utf8Secret.isEmpty()) {
        sodium_memzero(utf8Secret.data(), utf8Secret.size());
    }

    QApplication::clipboard()->setText(secret);
    this->copyBtn->setText("✓ Copied!");

    QTimer::singleShot(2000, this, [this]() {
        if (this->copyBtn) {
            this->copyBtn->setText("📋 Copy");
        }
    });

    // 25s auto-clear
    QTimer::singleShot(25000, [hash]() {
        QByteArray currentClip = QApplication::clipboard()->text().toUtf8();
        QByteArray currentHash = QCryptographicHash::hash(currentClip, QCryptographicHash::Sha256);
        if (!currentClip.isEmpty()) {
            sodium_memzero(currentClip.data(), currentClip.size());
        }
        if (currentHash == hash) {
            QApplication::clipboard()->clear();
        }
    });
}

QString PasswordGeneratorDialog::getGeneratedPassword() const
{
    return this->passwordDisplay->text();
}

QString PasswordGeneratorDialog::generateSecurePassword(int length, bool upper, bool lower, bool digits, bool symbols)
{
    QString charset = "";
    QString upperSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QString lowerSet = "abcdefghijklmnopqrstuvwxyz";
    QString digitsSet = "0123456789";
    QString symbolsSet = "!@#$%^&*()_+-=[]{}|;:,.<>?";

    if (upper) charset += upperSet;
    if (lower) charset += lowerSet;
    if (digits) charset += digitsSet;
    if (symbols) charset += symbolsSet;

    if (charset.isEmpty()) {
        charset = lowerSet + digitsSet;
    }

    QString password = "";
    for (int i = 0; i < length; ++i) {
        uint32_t idx = randombytes_uniform(static_cast<uint32_t>(charset.size()));
        password.append(charset[idx]);
    }

    return password;
}

void PasswordGeneratorDialog::wipeMemory()
{
    if (this->passwordDisplay) {
        this->passwordDisplay->clear();
    }
}

// ============================================================
// CredentialDialog (Add & Edit Item)
// ============================================================
CredentialDialog::CredentialDialog(QWidget* parent, const Credential* existingCred)
    : QDialog(parent)
{
    bool isEdit = (existingCred != nullptr);
    if (isEdit) {
        this->credentialId = existingCred->id;
    } else {
        this->credentialId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    }

    this->setupUi(isEdit);

    if (existingCred) {
        this->websiteInput->setText(QString::fromStdString(existingCred->websiteName));
        this->urlInput->setText(QString::fromStdString(existingCred->url));
        this->usernameInput->setText(QString::fromStdString(existingCred->userName));
        this->passwordInput->setText(QString::fromStdString(existingCred->userPassword));
        this->notesInput->setPlainText(QString::fromStdString(existingCred->notes));
        
        int catIndex = this->categoryCombo->findText(QString::fromStdString(existingCred->category));
        if (catIndex >= 0) {
            this->categoryCombo->setCurrentIndex(catIndex);
        } else if (!existingCred->category.empty()) {
            this->categoryCombo->setEditText(QString::fromStdString(existingCred->category));
        }
    }
}

CredentialDialog::~CredentialDialog()
{
    this->wipeMemory();
}

void CredentialDialog::setupUi(bool isEdit)
{
    this->setWindowTitle(isEdit ? "Edit Credential" : "Add Credential");
    this->setMinimumWidth(440);
    this->setStyleSheet(kDialogStyle);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    QLabel* titleLabel = new QLabel(isEdit ? "✏️ Edit Item" : "+ New Credential", this);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 16px; font-weight: bold; margin-bottom: 4px;");
    mainLayout->addWidget(titleLabel);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(12);
    formLayout->setLabelAlignment(Qt::AlignLeft);

    this->websiteInput = new QLineEdit(this);
    this->websiteInput->setMaxLength(128);
    this->websiteInput->setPlaceholderText("e.g. GitHub, Google, Amazon");
    formLayout->addRow("Title / Service:", this->websiteInput);

    this->urlInput = new QLineEdit(this);
    this->urlInput->setMaxLength(512);
    this->urlInput->setPlaceholderText("https://example.com/login");
    formLayout->addRow("Website URL:", this->urlInput);

    this->usernameInput = new QLineEdit(this);
    this->usernameInput->setMaxLength(256);
    this->usernameInput->setPlaceholderText("user@example.com");
    formLayout->addRow("Username / Email:", this->usernameInput);

    // Password row with Eye and Generate button
    this->passwordInput = new QLineEdit(this);
    this->passwordInput->setEchoMode(QLineEdit::Password);
    this->passwordInput->setMaxLength(4096);
    this->passwordInput->setPlaceholderText("Password");

    this->eyeBtn = new QPushButton("👁", this);
    this->eyeBtn->setToolTip("Show Password");
    this->eyeBtn->setFixedSize(34, 34);
    this->eyeBtn->setObjectName("iconBtn");
    this->eyeBtn->setCursor(Qt::PointingHandCursor);

    this->generateBtn = new QPushButton("⚡", this);
    this->generateBtn->setToolTip("Generate Strong Password");
    this->generateBtn->setFixedSize(34, 34);
    this->generateBtn->setObjectName("iconBtn");
    this->generateBtn->setCursor(Qt::PointingHandCursor);

    QHBoxLayout* passLayout = new QHBoxLayout();
    passLayout->setSpacing(6);
    passLayout->addWidget(this->passwordInput);
    passLayout->addWidget(this->eyeBtn);
    passLayout->addWidget(this->generateBtn);

    QVBoxLayout* passContainerLayout = new QVBoxLayout();
    passContainerLayout->setSpacing(4);
    passContainerLayout->addLayout(passLayout);

    this->capsLockWarning = new QLabel("⚠️ Caps Lock is on", this);
    this->capsLockWarning->setStyleSheet(
        "color: #FBBF24; background-color: #261E0A; border: 1px solid #78350F; "
        "border-radius: 4px; padding: 4px 8px; font-size: 11px; font-weight: 600;"
    );
    this->capsLockWarning->setVisible(false);
    passContainerLayout->addWidget(this->capsLockWarning);

    formLayout->addRow("Password:", passContainerLayout);

    this->capsWatcher = new CapsLockWatcher(this);
    this->capsWatcher->watch(this->passwordInput, this->capsLockWarning);

    // Category ComboBox
    this->categoryCombo = new QComboBox(this);
    this->categoryCombo->setEditable(true);
    this->categoryCombo->addItems({"Logins", "Finance", "Social", "Work", "Secure Note", "Gaming"});
    formLayout->addRow("Category:", this->categoryCombo);

    // Notes
    this->notesInput = new QTextEdit(this);
    this->notesInput->setPlaceholderText("Optional recovery keys, PINs, or account notes...");
    this->notesInput->setFixedHeight(80);
    formLayout->addRow("Notes:", this->notesInput);

    mainLayout->addLayout(formLayout);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    this->cancelBtn = new QPushButton("Cancel", this);
    this->cancelBtn->setObjectName("secondaryBtn");
    this->cancelBtn->setCursor(Qt::PointingHandCursor);

    this->saveBtn = new QPushButton(isEdit ? "Update Item" : "Save Credential", this);
    this->saveBtn->setObjectName("primaryBtn");
    this->saveBtn->setCursor(Qt::PointingHandCursor);
    this->saveBtn->setDefault(true);

    buttonLayout->addStretch();
    buttonLayout->addWidget(this->cancelBtn);
    buttonLayout->addWidget(this->saveBtn);

    mainLayout->addLayout(buttonLayout);

    connect(this->eyeBtn, &QPushButton::clicked, this, &CredentialDialog::onTogglePasswordVisibility);
    connect(this->generateBtn, &QPushButton::clicked, this, &CredentialDialog::onGeneratePasswordClicked);
    connect(this->saveBtn, &QPushButton::clicked, this, &CredentialDialog::validateAndAccept);
    connect(this->cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void CredentialDialog::onTogglePasswordVisibility()
{
    if (this->passwordInput->echoMode() == QLineEdit::Password) {
        this->passwordInput->setEchoMode(QLineEdit::Normal);
        this->eyeBtn->setText("🙈");
        this->eyeBtn->setToolTip("Hide Password");
    } else {
        this->passwordInput->setEchoMode(QLineEdit::Password);
        this->eyeBtn->setText("👁");
        this->eyeBtn->setToolTip("Show Password");
    }
}

void CredentialDialog::onGeneratePasswordClicked()
{
    PasswordGeneratorDialog genDialog(this);
    if (genDialog.exec() == QDialog::Accepted) {
        this->passwordInput->setText(genDialog.getGeneratedPassword());
    }
}

void CredentialDialog::validateAndAccept()
{
    if (this->websiteInput->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Input Required", "Please enter a Title or Website name.");
        this->websiteInput->setFocus();
        return;
    }
    this->accept();
}

Credential CredentialDialog::getCredential() const
{
    Credential c;
    c.id = this->credentialId;
    c.websiteName = this->websiteInput->text().trimmed().toStdString();
    c.url = this->urlInput->text().trimmed().toStdString();
    c.userName = this->usernameInput->text().trimmed().toStdString();
    c.userPassword = this->passwordInput->text().toStdString();
    c.notes = this->notesInput->toPlainText().toStdString();
    c.category = this->categoryCombo->currentText().trimmed().toStdString();
    return c;
}

void CredentialDialog::wipeMemory()
{
    if (this->passwordInput) {
        this->passwordInput->clear();
    }
}

// ============================================================
// CreateVaultDialog
// ============================================================
CreateVaultDialog::CreateVaultDialog(QWidget* parent)
    : QDialog(parent)
{
    this->setupUi();
}

CreateVaultDialog::~CreateVaultDialog()
{
    this->wipeMemory();
}

void CreateVaultDialog::setupUi()
{
    this->setWindowTitle("Create New Vault");
    this->setFixedWidth(420);
    this->setStyleSheet(kDialogStyle);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    QLabel* titleLabel = new QLabel("🔒 Setup New Vault", this);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 16px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);

    QLabel* descLabel = new QLabel("Each vault is an independently encrypted database protected by its own master key.", this);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #71717A; font-size: 11px; margin-bottom: 6px;");
    mainLayout->addWidget(descLabel);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    this->vaultNameInput = new QLineEdit(this);
    this->vaultNameInput->setMaxLength(48);
    this->vaultNameInput->setPlaceholderText("e.g. Personal, Work, Finance");
    formLayout->addRow("Vault Name:", this->vaultNameInput);

    this->passwordInput = new QLineEdit(this);
    this->passwordInput->setEchoMode(QLineEdit::Password);
    this->passwordInput->setMaxLength(1024);
    this->passwordInput->setPlaceholderText("Master Password");

    this->eyeBtn = new QPushButton("👁", this);
    this->eyeBtn->setToolTip("Show Passwords");
    this->eyeBtn->setFixedSize(34, 34);
    this->eyeBtn->setObjectName("iconBtn");
    this->eyeBtn->setCursor(Qt::PointingHandCursor);

    QHBoxLayout* passLayout = new QHBoxLayout();
    passLayout->addWidget(this->passwordInput);
    passLayout->addWidget(this->eyeBtn);
    formLayout->addRow("Master Key:", passLayout);

    this->confirmPasswordInput = new QLineEdit(this);
    this->confirmPasswordInput->setEchoMode(QLineEdit::Password);
    this->confirmPasswordInput->setMaxLength(1024);
    this->confirmPasswordInput->setPlaceholderText("Confirm Master Password");
    formLayout->addRow("Confirm Key:", this->confirmPasswordInput);

    mainLayout->addLayout(formLayout);

    // Caps Lock Warning
    this->capsLockWarning = new QLabel("⚠️ Caps Lock is on", this);
    this->capsLockWarning->setStyleSheet(
        "color: #FBBF24; background-color: #261E0A; border: 1px solid #78350F; "
        "border-radius: 4px; padding: 4px 8px; font-size: 11px; font-weight: 600;"
    );
    this->capsLockWarning->setVisible(false);
    mainLayout->addWidget(this->capsLockWarning);

    this->capsWatcher = new CapsLockWatcher(this);
    this->capsWatcher->watch(this->passwordInput, this->capsLockWarning);
    this->capsWatcher->watch(this->confirmPasswordInput, this->capsLockWarning);

    // Password Match Indicator
    this->matchLabel = new QLabel("", this);
    this->matchLabel->setStyleSheet("font-size: 11px; font-weight: 500;");
    mainLayout->addWidget(this->matchLabel);

    // Strength Bar
    QHBoxLayout* strengthLayout = new QHBoxLayout();
    this->strengthBar = new QProgressBar(this);
    this->strengthBar->setRange(0, 100);
    this->strengthBar->setValue(0);
    this->strengthBar->setTextVisible(false);
    this->strengthBar->setFixedHeight(5);
    this->strengthBar->setStyleSheet(
        "QProgressBar { background: #27272A; border: none; border-radius: 2px; }"
        "QProgressBar::chunk { background: #EF4444; border-radius: 2px; }"
    );

    this->strengthLabel = new QLabel("", this);
    this->strengthLabel->setStyleSheet("font-size: 10px; color: #71717A; font-weight: 600;");
    strengthLayout->addWidget(this->strengthBar);
    strengthLayout->addWidget(this->strengthLabel);
    mainLayout->addLayout(strengthLayout);

    // Action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    this->cancelBtn = new QPushButton("Cancel", this);
    this->cancelBtn->setObjectName("secondaryBtn");
    this->cancelBtn->setCursor(Qt::PointingHandCursor);

    this->createBtn = new QPushButton("Create Vault", this);
    this->createBtn->setObjectName("primaryBtn");
    this->createBtn->setCursor(Qt::PointingHandCursor);
    this->createBtn->setDefault(true);

    buttonLayout->addStretch();
    buttonLayout->addWidget(this->cancelBtn);
    buttonLayout->addWidget(this->createBtn);

    mainLayout->addLayout(buttonLayout);

    connect(this->passwordInput, &QLineEdit::textChanged, this, &CreateVaultDialog::onPasswordChanged);
    connect(this->confirmPasswordInput, &QLineEdit::textChanged, this, &CreateVaultDialog::onPasswordChanged);
    connect(this->eyeBtn, &QPushButton::clicked, this, &CreateVaultDialog::onToggleVisibility);
    connect(this->createBtn, &QPushButton::clicked, this, &CreateVaultDialog::validateAndAccept);
    connect(this->cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void CreateVaultDialog::onToggleVisibility()
{
    if (this->passwordInput->echoMode() == QLineEdit::Password) {
        this->passwordInput->setEchoMode(QLineEdit::Normal);
        this->confirmPasswordInput->setEchoMode(QLineEdit::Normal);
        this->eyeBtn->setText("🙈");
        this->eyeBtn->setToolTip("Hide Passwords");
    } else {
        this->passwordInput->setEchoMode(QLineEdit::Password);
        this->confirmPasswordInput->setEchoMode(QLineEdit::Password);
        this->eyeBtn->setText("👁");
        this->eyeBtn->setToolTip("Show Passwords");
    }
}

void CreateVaultDialog::onPasswordChanged()
{
    QString p1 = this->passwordInput->text();
    QString p2 = this->confirmPasswordInput->text();

    this->updateStrength(p1);

    if (p1.isEmpty() && p2.isEmpty()) {
        this->matchLabel->setText("");
        return;
    }

    if (p1 == p2 && !p1.isEmpty()) {
        this->matchLabel->setText("✓ Master passwords match");
        this->matchLabel->setStyleSheet("color: #4ADE80; font-size: 11px; font-weight: 600;");
    } else {
        this->matchLabel->setText("✗ Passwords do not match");
        this->matchLabel->setStyleSheet("color: #F87171; font-size: 11px; font-weight: 600;");
    }
}

void CreateVaultDialog::updateStrength(const QString& password)
{
    if (password.isEmpty()) {
        this->strengthBar->setValue(0);
        this->strengthLabel->setText("");
        return;
    }

    int score = 0;
    if (password.length() >= 8) score += 25;
    if (password.length() >= 12) score += 25;
    if (password.length() >= 16) score += 10;

    bool hasLower = false, hasUpper = false, hasDigit = false, hasSymbol = false;
    for (const QChar& c : password) {
        if (c.isLower()) hasLower = true;
        else if (c.isUpper()) hasUpper = true;
        else if (c.isDigit()) hasDigit = true;
        else hasSymbol = true;
    }

    if (hasLower) score += 10;
    if (hasUpper) score += 10;
    if (hasDigit) score += 10;
    if (hasSymbol) score += 10;

    score = std::min(100, score);
    this->strengthBar->setValue(score);

    if (score < 40) {
        this->strengthBar->setStyleSheet("QProgressBar { background: #27272A; border: none; } QProgressBar::chunk { background: #EF4444; }");
        this->strengthLabel->setText("WEAK");
        this->strengthLabel->setStyleSheet("color: #EF4444; font-size: 10px; font-weight: bold;");
    } else if (score < 75) {
        this->strengthBar->setStyleSheet("QProgressBar { background: #27272A; border: none; } QProgressBar::chunk { background: #F59E0B; }");
        this->strengthLabel->setText("FAIR");
        this->strengthLabel->setStyleSheet("color: #F59E0B; font-size: 10px; font-weight: bold;");
    } else {
        this->strengthBar->setStyleSheet("QProgressBar { background: #27272A; border: none; } QProgressBar::chunk { background: #10B981; }");
        this->strengthLabel->setText("STRONG");
        this->strengthLabel->setStyleSheet("color: #10B981; font-size: 10px; font-weight: bold;");
    }
}

void CreateVaultDialog::validateAndAccept()
{
    QString name = this->vaultNameInput->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Input Required", "Please enter a name for the new vault.");
        this->vaultNameInput->setFocus();
        return;
    }

    // Sanitize name: remove any illegal characters
    QString cleanName;
    for (const QChar& c : name) {
        if (c.isLetterOrNumber() || c == ' ' || c == '_' || c == '-') {
            cleanName.append(c);
        }
    }
    cleanName = cleanName.trimmed();
    if (cleanName.isEmpty()) {
        QMessageBox::warning(this, "Invalid Name", "Vault name must contain letters or numbers.");
        return;
    }

    if (this->passwordInput->text().isEmpty()) {
        QMessageBox::warning(this, "Password Required", "Master password cannot be empty.");
        this->passwordInput->setFocus();
        return;
    }

    if (this->passwordInput->text() != this->confirmPasswordInput->text()) {
        QMessageBox::warning(this, "Mismatch", "Master passwords do not match.");
        return;
    }

    this->vaultNameInput->setText(cleanName);
    this->accept();
}

QString CreateVaultDialog::getVaultName() const
{
    return this->vaultNameInput->text().trimmed();
}

std::string CreateVaultDialog::getMasterPassword() const
{
    return this->passwordInput->text().toStdString();
}

void CreateVaultDialog::wipeMemory()
{
    if (this->passwordInput) this->passwordInput->clear();
    if (this->confirmPasswordInput) this->confirmPasswordInput->clear();
}

// ============================================================
// ChangeMasterPasswordDialog
// ============================================================
ChangeMasterPasswordDialog::ChangeMasterPasswordDialog(QWidget* parent)
    : QDialog(parent)
{
    this->setupUi();
}

ChangeMasterPasswordDialog::~ChangeMasterPasswordDialog()
{
    this->wipeMemory();
}

void ChangeMasterPasswordDialog::setupUi()
{
    this->setWindowTitle("Change Master Key");
    this->setFixedWidth(400);
    this->setStyleSheet(kDialogStyle);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    QLabel* titleLabel = new QLabel("🔑 Reset Master Key", this);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 16px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    this->oldPasswordInput = new QLineEdit(this);
    this->oldPasswordInput->setEchoMode(QLineEdit::Password);
    this->oldPasswordInput->setMaxLength(1024);
    formLayout->addRow("Current Key:", this->oldPasswordInput);

    this->newPasswordInput = new QLineEdit(this);
    this->newPasswordInput->setEchoMode(QLineEdit::Password);
    this->newPasswordInput->setMaxLength(1024);
    formLayout->addRow("New Key:", this->newPasswordInput);

    this->confirmPasswordInput = new QLineEdit(this);
    this->confirmPasswordInput->setEchoMode(QLineEdit::Password);
    this->confirmPasswordInput->setMaxLength(1024);
    formLayout->addRow("Confirm New:", this->confirmPasswordInput);

    mainLayout->addLayout(formLayout);

    // Caps Lock Warning
    this->capsLockWarning = new QLabel("⚠️ Caps Lock is on", this);
    this->capsLockWarning->setStyleSheet(
        "color: #FBBF24; background-color: #261E0A; border: 1px solid #78350F; "
        "border-radius: 4px; padding: 4px 8px; font-size: 11px; font-weight: 600;"
    );
    this->capsLockWarning->setVisible(false);
    mainLayout->addWidget(this->capsLockWarning);

    this->capsWatcher = new CapsLockWatcher(this);
    this->capsWatcher->watch(this->oldPasswordInput, this->capsLockWarning);
    this->capsWatcher->watch(this->newPasswordInput, this->capsLockWarning);
    this->capsWatcher->watch(this->confirmPasswordInput, this->capsLockWarning);

    this->matchLabel = new QLabel("", this);
    this->matchLabel->setStyleSheet("font-size: 11px;");
    mainLayout->addWidget(this->matchLabel);

    this->strengthBar = new QProgressBar(this);
    this->strengthBar->setRange(0, 100);
    this->strengthBar->setValue(0);
    this->strengthBar->setTextVisible(false);
    this->strengthBar->setFixedHeight(4);
    this->strengthBar->setStyleSheet("QProgressBar { background: #27272A; border: none; } QProgressBar::chunk { background: #EF4444; }");
    mainLayout->addWidget(this->strengthBar);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    this->cancelBtn = new QPushButton("Cancel", this);
    this->cancelBtn->setObjectName("secondaryBtn");
    this->cancelBtn->setCursor(Qt::PointingHandCursor);

    this->saveBtn = new QPushButton("Update Key", this);
    this->saveBtn->setObjectName("primaryBtn");
    this->saveBtn->setCursor(Qt::PointingHandCursor);
    this->saveBtn->setDefault(true);

    buttonLayout->addStretch();
    buttonLayout->addWidget(this->cancelBtn);
    buttonLayout->addWidget(this->saveBtn);

    mainLayout->addLayout(buttonLayout);

    connect(this->newPasswordInput, &QLineEdit::textChanged, this, &ChangeMasterPasswordDialog::onPasswordChanged);
    connect(this->confirmPasswordInput, &QLineEdit::textChanged, this, &ChangeMasterPasswordDialog::onPasswordChanged);
    connect(this->saveBtn, &QPushButton::clicked, this, &ChangeMasterPasswordDialog::validateAndAccept);
    connect(this->cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ChangeMasterPasswordDialog::onPasswordChanged()
{
    QString p1 = this->newPasswordInput->text();
    QString p2 = this->confirmPasswordInput->text();

    if (p1.isEmpty() && p2.isEmpty()) {
        this->matchLabel->setText("");
        this->strengthBar->setValue(0);
        return;
    }

    if (p1 == p2 && !p1.isEmpty()) {
        this->matchLabel->setText("✓ New master passwords match");
        this->matchLabel->setStyleSheet("color: #4ADE80; font-size: 11px; font-weight: 600;");
    } else {
        this->matchLabel->setText("✗ New passwords do not match");
        this->matchLabel->setStyleSheet("color: #F87171; font-size: 11px; font-weight: 600;");
    }

    int score = std::min(100, static_cast<int>(p1.length() * 7));
    this->strengthBar->setValue(score);
}

void ChangeMasterPasswordDialog::validateAndAccept()
{
    if (this->oldPasswordInput->text().isEmpty()) {
        QMessageBox::warning(this, "Input Required", "Please enter your current master password.");
        return;
    }
    if (this->newPasswordInput->text().isEmpty()) {
        QMessageBox::warning(this, "Input Required", "New master password cannot be empty.");
        return;
    }
    if (this->newPasswordInput->text() != this->confirmPasswordInput->text()) {
        QMessageBox::warning(this, "Mismatch", "New master passwords do not match.");
        return;
    }
    this->accept();
}

std::string ChangeMasterPasswordDialog::getOldPassword() const
{
    return this->oldPasswordInput->text().toStdString();
}

std::string ChangeMasterPasswordDialog::getNewPassword() const
{
    return this->newPasswordInput->text().toStdString();
}

void ChangeMasterPasswordDialog::wipeMemory()
{
    if (this->oldPasswordInput) this->oldPasswordInput->clear();
    if (this->newPasswordInput) this->newPasswordInput->clear();
    if (this->confirmPasswordInput) this->confirmPasswordInput->clear();
}
