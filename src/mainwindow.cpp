#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QMessageBox>
#include <QCloseEvent>
#include <QClipboard>
#include <QApplication>
#include <QScrollArea>
#include <algorithm>
#include <cstring>

namespace
{
    const std::string kVaultFilePath = "vault.dat";
    const QString kMaskedPassword = "••••••••••••";
}

// ============================================================
// AddCredentialDialog
// ============================================================
AddCredentialDialog::AddCredentialDialog(QWidget* parentWidget) : QDialog(parentWidget)
{
    this->setupUi();
}

void AddCredentialDialog::setupUi()
{
    this->setWindowTitle("Add Credential");
    this->setMinimumWidth(360);

    this->websiteNameInput = new QLineEdit(this);
    this->userNameInput = new QLineEdit(this);
    this->userPasswordInput = new QLineEdit(this);
    this->userPasswordInput->setEchoMode(QLineEdit::Password);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->addRow("Website / App:", this->websiteNameInput);
    formLayout->addRow("Username:", this->userNameInput);
    formLayout->addRow("Password:", this->userPasswordInput);

    this->saveButton = new QPushButton("Save", this);
    this->cancelButton = new QPushButton("Cancel", this);
    this->saveButton->setDefault(true);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(this->cancelButton);
    buttonLayout->addWidget(this->saveButton);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);

    QObject::connect(this->saveButton, &QPushButton::clicked, this, &QDialog::accept);
    QObject::connect(this->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString AddCredentialDialog::getWebsiteName() const { return this->websiteNameInput->text(); }
QString AddCredentialDialog::getUserName() const { return this->userNameInput->text(); }
QString AddCredentialDialog::getUserPassword() const { return this->userPasswordInput->text(); }

// ============================================================
// ChangePasswordDialog
// ============================================================
ChangePasswordDialog::ChangePasswordDialog(QWidget* parentWidget) : QDialog(parentWidget)
{
    this->setupUi();
}

void ChangePasswordDialog::setupUi()
{
    this->setWindowTitle("Change Master Password");
    this->setMinimumWidth(380);

    this->oldPasswordInput = new QLineEdit(this);
    this->oldPasswordInput->setEchoMode(QLineEdit::Password);
    this->newPasswordInput = new QLineEdit(this);
    this->newPasswordInput->setEchoMode(QLineEdit::Password);
    this->confirmPasswordInput = new QLineEdit(this);
    this->confirmPasswordInput->setEchoMode(QLineEdit::Password);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->addRow("Current Password:", this->oldPasswordInput);
    formLayout->addRow("New Password:", this->newPasswordInput);
    formLayout->addRow("Confirm Password:", this->confirmPasswordInput);

    this->saveButton = new QPushButton("Change Password", this);
    this->cancelButton = new QPushButton("Cancel", this);
    this->saveButton->setDefault(true);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(this->cancelButton);
    buttonLayout->addWidget(this->saveButton);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);

    QObject::connect(this->saveButton, &QPushButton::clicked, this, &ChangePasswordDialog::validateAndAccept);
    QObject::connect(this->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void ChangePasswordDialog::validateAndAccept()
{
    if (this->newPasswordInput->text() != this->confirmPasswordInput->text()) {
        QMessageBox::warning(this, "Mismatch", "New passwords do not match.");
        return;
    }
    if (this->newPasswordInput->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Password cannot be empty.");
        return;
    }
    this->accept();
}

QString ChangePasswordDialog::getOldPassword() const { return this->oldPasswordInput->text(); }
QString ChangePasswordDialog::getNewPassword() const { return this->newPasswordInput->text(); }

// ============================================================
// MainWindow
// ============================================================
MainWindow::MainWindow(QWidget* parentWidget)
    : QMainWindow(parentWidget)
{
    this->vaultFilePath = kVaultFilePath;
    this->isVaultUnlocked = false;
    this->sortAscending = true;

    Crypto::initializeCrypto();

    this->setupUi();
}

MainWindow::~MainWindow()
{
    this->secureWipeSensitiveState();
}

void MainWindow::setupUi()
{
    this->setWindowTitle("Atelier's Vault");
    this->resize(950, 600);

    this->stackedWidget = new QStackedWidget(this);

    this->loginView = new LoginView(this);
    this->vaultView = this->buildVaultView();

    this->stackedWidget->addWidget(this->loginView);
    this->stackedWidget->addWidget(this->vaultView);
    this->stackedWidget->setCurrentWidget(this->loginView);

    this->setCentralWidget(this->stackedWidget);

    QObject::connect(this->loginView, &LoginView::unlockRequested, this, &MainWindow::onUnlockRequested);
}

QWidget* MainWindow::buildVaultView()
{
    QWidget* container = new QWidget();
    container->setStyleSheet("background-color: #0A0A0C; font-family: 'Inter', sans-serif;");

    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(20);

    // 1. TOP NAVIGATION BAR
    QHBoxLayout* navLayout = new QHBoxLayout();
    
    QLabel* brandIcon = new QLabel("🔒");
    brandIcon->setFixedSize(36, 36);
    brandIcon->setAlignment(Qt::AlignCenter);
    brandIcon->setStyleSheet("color: #D32F2F; font-size: 18px; border: 1px solid #401515; border-radius: 8px; background-color: #1A0B0B;");
    
    QLabel* brandText = new QLabel("Atelier's Vault");
    brandText->setStyleSheet("color: #FFFFFF; font-size: 18px; font-weight: bold; margin-left: 10px;");

    QLabel* badge = new QLabel("🟢 UNLOCKED");
    badge->setStyleSheet("background-color: #0E2918; color: #4CAF50; padding: 4px 10px; border-radius: 4px; font-size: 10px; font-weight: bold; border: 1px solid #1B4524; margin-left: 15px;");

    this->searchBar = new QLineEdit();
    this->searchBar->setPlaceholderText("🔍 Search by website, app, or username...");
    this->searchBar->setFixedWidth(300);
    this->searchBar->setStyleSheet(
        "QLineEdit {"
        "  background-color: #16161D; border: 1px solid #2A2A35; border-radius: 6px;"
        "  padding: 10px 14px; color: #FFFFFF; font-size: 13px;"
        "}"
        "QLineEdit:focus { border: 1px solid #D32F2F; }"
    );

    this->addCredentialButton = new QPushButton("+ Add Credential");
    this->addCredentialButton->setCursor(Qt::PointingHandCursor);
    this->addCredentialButton->setStyleSheet(
        "QPushButton { background-color: #D32F2F; color: #FFFFFF; border: none; border-radius: 6px; padding: 10px 20px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #B71C1C; }"
    );

    this->changePasswordButton = new QPushButton("🔑 Reset Master Key");
    this->changePasswordButton->setCursor(Qt::PointingHandCursor);
    this->changePasswordButton->setStyleSheet(
        "QPushButton { background-color: #16161D; border: 1px solid #2A2A35; color: #A0A0B0; border-radius: 6px; padding: 10px 20px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { border-color: #D32F2F; color: #FFFFFF; }"
    );

    this->lockVaultButton = new QPushButton("🔒 Lock Vault");
    this->lockVaultButton->setCursor(Qt::PointingHandCursor);
    this->lockVaultButton->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #2A2A35; color: #A0A0B0; border-radius: 6px; padding: 10px 20px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { border-color: #A0A0B0; color: #FFFFFF; }"
    );

    navLayout->addWidget(brandIcon);
    navLayout->addWidget(brandText);
    navLayout->addWidget(badge);
    navLayout->addStretch();
    navLayout->addWidget(this->searchBar);
    navLayout->addSpacing(10);
    navLayout->addWidget(this->addCredentialButton);
    navLayout->addSpacing(10);
    navLayout->addWidget(this->changePasswordButton);
    navLayout->addSpacing(10);
    navLayout->addWidget(this->lockVaultButton);

    // 2. STATS BAR
    QFrame* statsBar = new QFrame();
    statsBar->setFixedHeight(50);
    statsBar->setStyleSheet("background-color: #121216; border: 1px solid #1F1F26; border-left: 2px solid #D32F2F; border-radius: 6px;");
    
    QHBoxLayout* statsLayout = new QHBoxLayout(statsBar);
    statsLayout->setContentsMargins(20, 0, 20, 0);

    this->statsTextLabel = new QLabel("");
    this->statsTextLabel->setStyleSheet("border: none; font-size: 12px;");
    
    this->sortButton = new QPushButton("Sort by: Name ▵");
    this->sortButton->setCursor(Qt::PointingHandCursor);
    this->sortButton->setStyleSheet("background: transparent; color: #A0A0B0; border: 1px solid #2A2A35; border-radius: 4px; padding: 4px 10px; font-size: 12px;");

    statsLayout->addWidget(this->statsTextLabel);
    statsLayout->addStretch();
    statsLayout->addWidget(this->sortButton);

    // 3. TABLE HEADERS
    QFrame* headerFrame = new QFrame();
    QHBoxLayout* headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(20, 10, 20, 10);
    headerLayout->setSpacing(20);
    
    QString headerStyle = "color: #888899; font-weight: bold; font-size: 10px; letter-spacing: 1px;";
    
    QLabel* hWeb = new QLabel("WEBSITE / APP"); hWeb->setStyleSheet(headerStyle); hWeb->setFixedWidth(250);
    QLabel* hUser = new QLabel("USERNAME"); hUser->setStyleSheet(headerStyle); hUser->setFixedWidth(200);
    QLabel* hPass = new QLabel("PASSWORD"); hPass->setStyleSheet(headerStyle); hPass->setFixedWidth(150);
    QLabel* hCat = new QLabel("CATEGORY"); hCat->setStyleSheet(headerStyle); hCat->setFixedWidth(120);
    QLabel* hAct = new QLabel("ACTIONS"); hAct->setStyleSheet(headerStyle); hAct->setAlignment(Qt::AlignCenter);

    headerLayout->addWidget(hWeb);
    headerLayout->addWidget(hUser);
    headerLayout->addWidget(hPass);
    headerLayout->addWidget(hCat);
    headerLayout->addStretch();
    headerLayout->addWidget(hAct);

    // 4. SCROLLABLE CREDENTIAL LIST
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: 1px solid #1F1F26; border-radius: 8px; background-color: transparent; } QScrollBar:vertical { width: 8px; background: #0A0A0C; }");

    QWidget* scrollContent = new QWidget();
    scrollContent->setStyleSheet("background-color: transparent;");
    
    this->credentialsListLayout = new QVBoxLayout(scrollContent);
    this->credentialsListLayout->setContentsMargins(0, 0, 0, 0);
    this->credentialsListLayout->setSpacing(0);
    
    scrollArea->setWidget(scrollContent);

    // Assemble Main Layout
    mainLayout->addLayout(navLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(statsBar);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(headerFrame);
    mainLayout->addWidget(scrollArea);

    QObject::connect(this->searchBar, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    QObject::connect(this->addCredentialButton, &QPushButton::clicked, this, &MainWindow::onAddCredentialButtonClicked);
    QObject::connect(this->changePasswordButton, &QPushButton::clicked, this, &MainWindow::onChangePasswordClicked);
    QObject::connect(this->lockVaultButton, &QPushButton::clicked, this, &MainWindow::onLockButtonClicked);
    QObject::connect(this->sortButton, &QPushButton::clicked, this, &MainWindow::onSortButtonClicked);

    return container;
}

// ==========================================
// BUSINESS LOGIC & DYNAMIC DATA POPULATION
// ==========================================

bool MainWindow::saveCurrentVaultState()
{
    std::string serializedData = Vault::serializeCredentials(this->currentCredentials);
    std::vector<unsigned char> plainBytes(serializedData.begin(), serializedData.end());
    std::vector<unsigned char> newCipherText = Crypto::encryptVault(plainBytes, this->derivedKey, this->vaultNonce);

    bool saveSucceeded = Vault::saveEncryptedVaultToFile(this->vaultFilePath, newCipherText, this->vaultSalt, this->vaultNonce);

    std::fill(serializedData.begin(), serializedData.end(), '\0');
    Crypto::secureClean(plainBytes);

    if (saveSucceeded) {
        this->vaultCipherText = newCipherText;
    }
    
    return saveSucceeded;
}

void MainWindow::deleteCredential(const QString& website, const QString& username)
{
    // 1. Locate and erase the credential in memory
    auto iterator = std::remove_if(this->currentCredentials.begin(), this->currentCredentials.end(),
        [&](const Credential& c) {
            return c.websiteName == website.toStdString() && c.userName == username.toStdString();
        });

    if (iterator != this->currentCredentials.end())
    {
        this->currentCredentials.erase(iterator, this->currentCredentials.end());

        // 2. Commit change to disk
        if (!this->saveCurrentVaultState())
        {
            QMessageBox::critical(this, "Disk Error", "Failed to delete credential from disk.");
        }
        
        // 3. Re-render UI
        this->refreshCredentialsTable();
    }
}

void MainWindow::onSortButtonClicked()
{
    this->sortAscending = !this->sortAscending;
    
    if (this->sortAscending) {
        this->sortButton->setText("Sort by: Name ▵");
    } else {
        this->sortButton->setText("Sort by: Name ▿");
    }
    
    this->refreshCredentialsTable();
}

void MainWindow::refreshCredentialsTable()
{
    this->populateCredentialsTable(this->searchBar->text());
}

void MainWindow::populateCredentialsTable(const QString& filterText)
{
    // Clear existing dynamic rows
    QLayoutItem* child;
    while ((child = this->credentialsListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) { child->widget()->deleteLater(); }
        delete child;
    }

    // Apply internal sorting based on the current toggle state
    std::sort(this->currentCredentials.begin(), this->currentCredentials.end(), 
        [this](const Credential& a, const Credential& b) {
            if (this->sortAscending) {
                return a.websiteName < b.websiteName;
            } else {
                return a.websiteName > b.websiteName;
            }
        });

    QString normalizedFilter = filterText.trimmed().toLower();
    int showingCount = 0;
    int totalCount = this->currentCredentials.size();

    // Render rows
    for (std::size_t i = 0; i < this->currentCredentials.size(); i++)
    {
        const Credential& credential = this->currentCredentials[i];
        QString websiteText = QString::fromStdString(credential.websiteName);
        QString usernameText = QString::fromStdString(credential.userName);
        QString passwordText = QString::fromStdString(credential.userPassword);

        if (!normalizedFilter.isEmpty() && 
            !websiteText.toLower().contains(normalizedFilter) && 
            !usernameText.toLower().contains(normalizedFilter))
        {
            continue;
        }

        QString urlText = websiteText.toLower().remove(" ") + ".com";
        QWidget* rowWidget = createCredentialRow(websiteText, urlText, usernameText, passwordText, "General", "🌐");
        this->credentialsListLayout->addWidget(rowWidget);
        showingCount++;
    }

    this->credentialsListLayout->addStretch();

    // Update Stats Label
    QString statsHtml = QString(
        "<span style='color:#888899;'>🛡️ Total Entries:</span> <b style='color:#FFFFFF;'>%1</b> &nbsp;&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;&nbsp; "
        "<span style='color:#888899;'>Showing:</span> <b style='color:#FFFFFF;'>%2</b> &nbsp;&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;&nbsp; "
        "<span style='color:#888899;'>Cipher:</span> <span style='color:#4CAF50;'>AES-256-GCM</span> &nbsp;&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;&nbsp; "
        "<span style='color:#888899;'>KDF:</span> <span style='color:#4CAF50;'>Argon2id</span>"
    ).arg(totalCount).arg(showingCount);
    
    this->statsTextLabel->setText(statsHtml);
}

QWidget* MainWindow::createCredentialRow(const QString& website, const QString& url, const QString& username, const QString& password, const QString& category, const QString& iconStr)
{
    QFrame* row = new QFrame();
    row->setFixedHeight(70);
    row->setStyleSheet("QFrame { background-color: #121216; border-bottom: 1px solid #1F1F26; } QFrame:hover { background-color: #1A1A21; }");
    
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(20, 10, 20, 10);
    layout->setSpacing(20);

    QLabel* avatar = new QLabel(iconStr);
    avatar->setFixedSize(40, 40);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet("background-color: #1A0B0B; color: #D32F2F; border: 1px solid #401515; border-radius: 8px; font-size: 16px;");

    QVBoxLayout* webLayout = new QVBoxLayout();
    webLayout->setSpacing(2);
    webLayout->setAlignment(Qt::AlignVCenter);
    
    QLabel* webLabel = new QLabel(website);
    webLabel->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 14px; border: none; background: transparent;");
    QLabel* urlLabel = new QLabel(url);
    urlLabel->setStyleSheet("color: #888899; font-size: 11px; font-family: monospace; border: none; background: transparent;");
    
    webLayout->addWidget(webLabel);
    webLayout->addWidget(urlLabel);

    QWidget* webWrapper = new QWidget();
    webWrapper->setFixedWidth(190);
    webWrapper->setLayout(webLayout);
    webWrapper->setStyleSheet("border: none; background: transparent;");

    QLabel* userLabel = new QLabel(username);
    userLabel->setFixedWidth(200);
    userLabel->setStyleSheet("color: #D0D0E0; font-family: monospace; font-size: 13px; border: none; background: transparent;");

    QLabel* passLabel = new QLabel(kMaskedPassword);
    passLabel->setFixedWidth(150);
    passLabel->setStyleSheet("color: #888899; font-size: 16px; letter-spacing: 3px; border: none; background: transparent;");

    QLabel* catLabel = new QLabel(category);
    catLabel->setAlignment(Qt::AlignCenter);
    catLabel->setFixedWidth(120);
    catLabel->setFixedHeight(26);
    catLabel->setStyleSheet("background-color: #1E1E26; color: #A0A0B0; border: 1px solid #2A2A35; border-radius: 4px; font-size: 11px; padding: 0px 8px;");

    QHBoxLayout* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(10);
    
    QString btnStyle = "QPushButton { background-color: transparent; color: #888899; border: 1px solid #2A2A35; border-radius: 6px; font-size: 14px; } QPushButton:hover { background-color: #2A2A35; color: #FFFFFF; }";

    QPushButton* viewBtn = new QPushButton("👁"); viewBtn->setFixedSize(32, 32); viewBtn->setStyleSheet(btnStyle); viewBtn->setCursor(Qt::PointingHandCursor);
    QPushButton* copyBtn = new QPushButton("📋"); copyBtn->setFixedSize(32, 32); copyBtn->setStyleSheet(btnStyle); copyBtn->setCursor(Qt::PointingHandCursor);
    QPushButton* deleteBtn = new QPushButton("🗑"); deleteBtn->setFixedSize(32, 32); deleteBtn->setStyleSheet(btnStyle); deleteBtn->setCursor(Qt::PointingHandCursor);

    // Toggle Password Visibility Logic
    QObject::connect(viewBtn, &QPushButton::clicked, [passLabel, password, viewBtn]() {
        if (passLabel->text() == kMaskedPassword) {
            passLabel->setText(password);
            passLabel->setStyleSheet("color: #FFFFFF; font-size: 13px; letter-spacing: 0px; border: none; background: transparent;");
            viewBtn->setText("🙈");
        } else {
            passLabel->setText(kMaskedPassword);
            passLabel->setStyleSheet("color: #888899; font-size: 16px; letter-spacing: 3px; border: none; background: transparent;");
            viewBtn->setText("👁");
        }
    });

    // Copy Logic
    QObject::connect(copyBtn, &QPushButton::clicked, [password]() {
        QApplication::clipboard()->setText(password);
    });

    // Wire up the new Delete functionality
    QObject::connect(deleteBtn, &QPushButton::clicked, [this, website, username]() {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Deletion", "Are you sure you want to delete this credential?", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            this->deleteCredential(website, username);
        }
    });

    actionLayout->addWidget(viewBtn);
    actionLayout->addWidget(copyBtn);
    actionLayout->addWidget(deleteBtn);

    layout->addWidget(avatar);
    layout->addWidget(webWrapper);
    layout->addWidget(userLabel);
    layout->addWidget(passLabel);
    layout->addWidget(catLabel);
    layout->addStretch();
    layout->addLayout(actionLayout);

    return row;
}

void MainWindow::onChangePasswordClicked()
{
    ChangePasswordDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;

    std::string oldPass = dialog.getOldPassword().toStdString();
    std::string newPass = dialog.getNewPassword().toStdString();

    // 1. Verify current master password
    std::vector<unsigned char> testKey = Crypto::deriveKey(oldPass, this->vaultSalt);
    if (testKey != this->derivedKey)
    {
        QMessageBox::critical(this, "Authentication Failed", "The current master password entered is incorrect.");
        Crypto::secureClean(testKey);
        return;
    }
    Crypto::secureClean(testKey);

    // 2. Derive new key with new password (reusing salt for simplicity)
    std::vector<unsigned char> newKey = Crypto::deriveKey(newPass, this->vaultSalt);

    // 3. Swap the active key and attempt a save
    std::vector<unsigned char> previousKey = this->derivedKey;
    this->derivedKey = newKey;

    if (this->saveCurrentVaultState())
    {
        QMessageBox::information(this, "Success", "Master password has been updated securely.");
        Crypto::secureClean(previousKey);
    }
    else
    {
        QMessageBox::critical(this, "Disk Error", "Failed to save the vault with the new password.");
        this->derivedKey = previousKey; // Rollback on failure
        Crypto::secureClean(newKey);
    }

    // Safely wipe strings from memory
    std::fill(oldPass.begin(), oldPass.end(), '\0');
    std::fill(newPass.begin(), newPass.end(), '\0');
}

void MainWindow::onUnlockRequested(const std::string& masterPassword)
{
    std::string errorMessage = "";
    bool unlockSucceeded = this->attemptUnlockVault(masterPassword, errorMessage);

    if (unlockSucceeded)
    {
        this->isVaultUnlocked = true;
        this->refreshCredentialsTable();
        this->stackedWidget->setCurrentWidget(this->vaultView);
    }
    else
    {
        this->loginView->showErrorMessage(QString::fromStdString(errorMessage));
    }
}

bool MainWindow::attemptUnlockVault(const std::string& masterPassword, std::string& errorMessage)
{
    std::vector<unsigned char> loadedCipherText;
    std::vector<unsigned char> loadedSalt;
    std::vector<unsigned char> loadedNonce;

    bool fileLoaded = Vault::loadEncryptedVaultFromFile(this->vaultFilePath, loadedCipherText, loadedSalt, loadedNonce);
    if (!fileLoaded)
    {
        errorMessage = "Corrupted Vault Data";
        return false;
    }

    std::vector<unsigned char> candidateKey = Crypto::deriveKey(masterPassword, loadedSalt);
    std::vector<unsigned char> decryptedBytes = Crypto::decryptVault(loadedCipherText, candidateKey, loadedNonce);

    if (decryptedBytes.empty())
    {
        Crypto::secureClean(candidateKey);
        errorMessage = "Incorrect Master Password";
        return false;
    }

    std::string rawCredentialsData(decryptedBytes.begin(), decryptedBytes.end());
    std::vector<Credential> loadedCredentials = Vault::deserializeCredentials(rawCredentialsData);

    std::fill(rawCredentialsData.begin(), rawCredentialsData.end(), '\0');
    Crypto::secureClean(decryptedBytes);

    this->derivedKey = candidateKey;
    this->vaultSalt = loadedSalt;
    this->vaultNonce = loadedNonce;
    this->vaultCipherText = loadedCipherText;
    this->currentCredentials = loadedCredentials;

    return true;
}

void MainWindow::onLockButtonClicked()
{
    this->secureWipeSensitiveState();
    this->searchBar->clear();
    this->populateCredentialsTable(""); // Clears table visually
    this->loginView->clearInput();
    this->stackedWidget->setCurrentWidget(this->loginView);
}

void MainWindow::onAddCredentialButtonClicked()
{
    if (!this->isVaultUnlocked) return;

    AddCredentialDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;

    if (dialog.getWebsiteName().isEmpty() || dialog.getUserName().isEmpty())
    {
        QMessageBox::warning(this, "Input Error", "Website and Username cannot be empty.");
        return;
    }

    Credential newCredential;
    newCredential.websiteName = dialog.getWebsiteName().toStdString();
    newCredential.userName = dialog.getUserName().toStdString();
    newCredential.userPassword = dialog.getUserPassword().toStdString();

    this->currentCredentials.push_back(newCredential);

    if (!this->saveCurrentVaultState())
    {
        QMessageBox::critical(this, "Save Error", "Failed to save the updated vault to disk.");
        this->currentCredentials.pop_back(); // Remove from memory if disk save fails
        return;
    }

    this->refreshCredentialsTable();
}

void MainWindow::onSearchTextChanged(const QString& searchText)
{
    this->populateCredentialsTable(searchText);
}

void MainWindow::secureWipeSensitiveState()
{
    Crypto::secureClean(this->derivedKey);

    for (std::size_t i = 0; i < this->currentCredentials.size(); i++)
    {
        std::fill(this->currentCredentials[i].userPassword.begin(), this->currentCredentials[i].userPassword.end(), '\0');
        std::fill(this->currentCredentials[i].userName.begin(), this->currentCredentials[i].userName.end(), '\0');
        std::fill(this->currentCredentials[i].websiteName.begin(), this->currentCredentials[i].websiteName.end(), '\0');
    }

    this->currentCredentials.clear();
    this->vaultSalt.clear();
    this->vaultNonce.clear();
    this->vaultCipherText.clear();
    this->isVaultUnlocked = false;
}

void MainWindow::closeEvent(QCloseEvent* closeEventPointer)
{
    this->secureWipeSensitiveState();
    QMainWindow::closeEvent(closeEventPointer);
}