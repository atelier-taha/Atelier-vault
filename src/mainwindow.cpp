#include "mainwindow.h"
#include "dialogs.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QMessageBox>
#include <QCloseEvent>
#include <QClipboard>
#include <QApplication>
#include <QScrollArea>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QTimer>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QUrl>
#include <sodium.h>
#include <algorithm>
#include <cstring>

namespace
{
    const QString kMaskedPassword = "••••••••••••";

    QString getCategoryIcon(const std::string& cat)
    {
        QString category = QString::fromStdString(cat).toLower();
        if (category.contains("finance") || category.contains("bank") || category.contains("card")) return "💳";
        if (category.contains("social") || category.contains("chat")) return "💬";
        if (category.contains("work") || category.contains("business")) return "💼";
        if (category.contains("note") || category.contains("secure")) return "📝";
        if (category.contains("game") || category.contains("steam")) return "🎮";
        return "🌐";
    }
}

// ============================================================
// MainWindow Implementation
// ============================================================
MainWindow::MainWindow(QWidget* parentWidget)
    : QMainWindow(parentWidget)
{
    QApplication::setApplicationName("AtelierVault");

    this->activeVaultName = "Personal";
    this->vaultFilePath = Vault::getVaultFilePath(this->activeVaultName).toStdString();
    this->isVaultUnlocked = false;
    this->sortAscending = true;
    this->lastCopiedHash.clear();

    // 10-minute idle inactivity auto-lock timer
    this->inactivityTimer = new QTimer(this);
    this->inactivityTimer->setInterval(10 * 60 * 1000);
    this->inactivityTimer->setSingleShot(true);
    connect(this->inactivityTimer, &QTimer::timeout, this, &MainWindow::onLockButtonClicked);

    if (qApp) {
        qApp->installEventFilter(this);
    }

    Crypto::initializeCrypto();

    this->setupUi();
}

MainWindow::~MainWindow()
{
    this->secureWipeSensitiveState();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (this->isVaultUnlocked) {
        if (event->type() == QEvent::KeyPress ||
            event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseMove) {
            if (this->inactivityTimer && this->inactivityTimer->isActive()) {
                this->inactivityTimer->start(); // Reset idle countdown
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupUi()
{
    this->setWindowTitle("Atelier's Vault");
    this->resize(1000, 640);
    this->setStyleSheet("background-color: #121214; font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;");

    this->stackedWidget = new QStackedWidget(this);

    this->loginView = new LoginView(this);
    this->vaultView = this->buildVaultView();

    this->stackedWidget->addWidget(this->loginView);
    this->stackedWidget->addWidget(this->vaultView);
    this->stackedWidget->setCurrentWidget(this->loginView);

    this->setCentralWidget(this->stackedWidget);

    connect(this->loginView, &LoginView::unlockRequested, this, &MainWindow::onUnlockRequested);
    connect(this->loginView, &LoginView::createVaultRequested, this, &MainWindow::onCreateVaultRequested);
}

QWidget* MainWindow::buildVaultView()
{
    QWidget* container = new QWidget();
    container->setStyleSheet("background-color: #121214;");

    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(28, 20, 28, 20);
    mainLayout->setSpacing(16);

    // 1. TOP NAVIGATION BAR
    QHBoxLayout* navLayout = new QHBoxLayout();
    navLayout->setSpacing(12);

    QLabel* brandIcon = new QLabel("🔒");
    brandIcon->setFixedSize(36, 36);
    brandIcon->setAlignment(Qt::AlignCenter);
    brandIcon->setStyleSheet("color: #E11D48; font-size: 18px; border: 1px solid #3F151F; border-radius: 8px; background-color: #200B11;");

    QLabel* brandText = new QLabel("Atelier's Vault");
    brandText->setStyleSheet("color: #FFFFFF; font-size: 17px; font-weight: bold; margin-right: 6px;");

    QLabel* badge = new QLabel("🟢 UNLOCKED");
    badge->setStyleSheet("background-color: #052E16; color: #4ADE80; padding: 4px 8px; border-radius: 4px; font-size: 10px; font-weight: bold; border: 1px solid #14532D;");

    // Active Vault Switcher Dropdown
    this->vaultSwitcher = new QComboBox();
    this->vaultSwitcher->setFixedHeight(36);
    this->vaultSwitcher->setStyleSheet(
        "QComboBox {"
        "  background-color: #18181B; border: 1px solid #27272A; border-radius: 6px;"
        "  padding: 4px 12px; color: #F4F4F5; font-size: 13px; font-weight: 600;"
        "}"
        "QComboBox:hover { border-color: #3F3F46; }"
        "QComboBox:focus { border-color: #E11D48; }"
        "QComboBox::drop-down { border: none; padding-right: 8px; }"
        "QComboBox QAbstractItemView { background-color: #18181B; color: #F4F4F5; selection-background-color: #27272A; border: 1px solid #27272A; }"
    );
    connect(this->vaultSwitcher, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onVaultSwitchSelected);

    // Search Bar
    this->searchBar = new QLineEdit();
    this->searchBar->setPlaceholderText("🔍 Search service, user, notes...");
    this->searchBar->setFixedWidth(260);
    this->searchBar->setFixedHeight(36);
    this->searchBar->setStyleSheet(
        "QLineEdit {"
        "  background-color: #18181B; border: 1px solid #27272A; border-radius: 6px;"
        "  padding: 6px 12px; color: #F4F4F5; font-size: 13px;"
        "}"
        "QLineEdit:focus { border: 1px solid #E11D48; background-color: #1E1E24; }"
    );

    // Primary + Add Item Button
    this->addCredentialButton = new QPushButton("+ Add Item");
    this->addCredentialButton->setFixedHeight(36);
    this->addCredentialButton->setCursor(Qt::PointingHandCursor);
    this->addCredentialButton->setStyleSheet(
        "QPushButton { background-color: #D32F2F; color: #FFFFFF; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #B71C1C; }"
    );

    // Generator utility button
    this->generatePasswordButton = new QPushButton("⚡ Generator");
    this->generatePasswordButton->setFixedHeight(36);
    this->generatePasswordButton->setCursor(Qt::PointingHandCursor);
    this->generatePasswordButton->setStyleSheet(
        "QPushButton { background-color: #18181B; border: 1px solid #27272A; color: #D4D4D8; border-radius: 6px; padding: 6px 14px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { border-color: #3F3F46; color: #FFFFFF; background-color: #202024; }"
    );

    // Reset Master Key button
    this->changePasswordButton = new QPushButton("🔑 Reset Key");
    this->changePasswordButton->setFixedHeight(36);
    this->changePasswordButton->setCursor(Qt::PointingHandCursor);
    this->changePasswordButton->setStyleSheet(
        "QPushButton { background-color: #18181B; border: 1px solid #27272A; color: #A1A1AA; border-radius: 6px; padding: 6px 12px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { border-color: #E11D48; color: #FFFFFF; }"
    );

    // Lock Vault button
    this->lockVaultButton = new QPushButton("🔒 Lock");
    this->lockVaultButton->setFixedHeight(36);
    this->lockVaultButton->setCursor(Qt::PointingHandCursor);
    this->lockVaultButton->setStyleSheet(
        "QPushButton { background-color: transparent; border: 1px solid #27272A; color: #A1A1AA; border-radius: 6px; padding: 6px 14px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { border-color: #A1A1AA; color: #FFFFFF; }"
    );

    navLayout->addWidget(brandIcon);
    navLayout->addWidget(brandText);
    navLayout->addWidget(badge);
    navLayout->addWidget(this->vaultSwitcher);
    navLayout->addStretch();
    navLayout->addWidget(this->searchBar);
    navLayout->addWidget(this->addCredentialButton);
    navLayout->addWidget(this->generatePasswordButton);
    navLayout->addWidget(this->changePasswordButton);
    navLayout->addWidget(this->lockVaultButton);

    // 2. STATS & SORT BAR
    QFrame* statsBar = new QFrame();
    statsBar->setFixedHeight(46);
    statsBar->setStyleSheet("background-color: #18181B; border: 1px solid #27272A; border-left: 3px solid #E11D48; border-radius: 6px;");

    QHBoxLayout* statsLayout = new QHBoxLayout(statsBar);
    statsLayout->setContentsMargins(16, 0, 16, 0);

    this->statsTextLabel = new QLabel("");
    this->statsTextLabel->setStyleSheet("border: none; font-size: 12px; color: #A1A1AA;");

    this->sortButton = new QPushButton("Sort by: Title ▵");
    this->sortButton->setCursor(Qt::PointingHandCursor);
    this->sortButton->setStyleSheet("background: transparent; color: #A1A1AA; border: 1px solid #27272A; border-radius: 4px; padding: 4px 10px; font-size: 12px;");

    statsLayout->addWidget(this->statsTextLabel);
    statsLayout->addStretch();
    statsLayout->addWidget(this->sortButton);

    // 3. TABLE HEADERS
    QFrame* headerFrame = new QFrame();
    QHBoxLayout* headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(20, 8, 20, 8);
    headerLayout->setSpacing(16);

    QString headerStyle = "color: #71717A; font-weight: bold; font-size: 10px; letter-spacing: 1px;";

    QLabel* hWeb = new QLabel("TITLE / SERVICE"); hWeb->setStyleSheet(headerStyle); hWeb->setFixedWidth(240);
    QLabel* hUser = new QLabel("USERNAME"); hUser->setStyleSheet(headerStyle); hUser->setFixedWidth(190);
    QLabel* hPass = new QLabel("PASSWORD"); hPass->setStyleSheet(headerStyle); hPass->setFixedWidth(140);
    QLabel* hCat = new QLabel("CATEGORY"); hCat->setStyleSheet(headerStyle); hCat->setFixedWidth(110);
    QLabel* hAct = new QLabel("ACTIONS"); hAct->setStyleSheet(headerStyle); hAct->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    headerLayout->addWidget(hWeb);
    headerLayout->addWidget(hUser);
    headerLayout->addWidget(hPass);
    headerLayout->addWidget(hCat);
    headerLayout->addStretch();
    headerLayout->addWidget(hAct);

    // 4. SCROLLABLE CREDENTIAL LIST
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: 1px solid #27272A; border-radius: 8px; background-color: transparent; } QScrollBar:vertical { width: 8px; background: #121214; }");

    QWidget* scrollContent = new QWidget();
    scrollContent->setStyleSheet("background-color: transparent;");

    this->credentialsListLayout = new QVBoxLayout(scrollContent);
    this->credentialsListLayout->setContentsMargins(0, 0, 0, 0);
    this->credentialsListLayout->setSpacing(0);

    scrollArea->setWidget(scrollContent);

    // Assemble Main Layout
    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(statsBar);
    mainLayout->addWidget(headerFrame);
    mainLayout->addWidget(scrollArea);

    connect(this->searchBar, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(this->addCredentialButton, &QPushButton::clicked, this, &MainWindow::onAddCredentialButtonClicked);
    connect(this->generatePasswordButton, &QPushButton::clicked, this, &MainWindow::onGeneratePasswordButtonClicked);
    connect(this->changePasswordButton, &QPushButton::clicked, this, &MainWindow::onChangePasswordClicked);
    connect(this->lockVaultButton, &QPushButton::clicked, this, &MainWindow::onLockButtonClicked);
    connect(this->sortButton, &QPushButton::clicked, this, &MainWindow::onSortButtonClicked);

    return container;
}

void MainWindow::refreshVaultSwitcher()
{
    this->vaultSwitcher->blockSignals(true);
    this->vaultSwitcher->clear();
    std::vector<QString> vaults = Vault::listAvailableVaults();
    if (vaults.empty()) {
        vaults.push_back("Personal");
    }

    int activeIdx = 0;
    for (std::size_t i = 0; i < vaults.size(); ++i) {
        this->vaultSwitcher->addItem("🗄️ " + vaults[i], vaults[i]);
        if (vaults[i] == this->activeVaultName) {
            activeIdx = static_cast<int>(i);
        }
    }
    this->vaultSwitcher->setCurrentIndex(activeIdx);
    this->vaultSwitcher->blockSignals(false);
}

void MainWindow::onVaultSwitchSelected(int index)
{
    if (index < 0) return;
    QString targetVault = this->vaultSwitcher->itemData(index).toString();
    if (targetVault.isEmpty()) {
        targetVault = this->vaultSwitcher->currentText().remove("🗄️ ").trimmed();
    }

    if (targetVault == this->activeVaultName && this->isVaultUnlocked) {
        return;
    }

    // Lock and return to login view pre-selecting the chosen vault
    this->onLockButtonClicked();
    this->loginView->setSelectedVault(targetVault);
}

bool MainWindow::saveCurrentVaultState()
{
    this->vaultNonce = Crypto::generateNonce();
    std::string serializedData = Vault::serializeCredentials(this->currentCredentials);

    std::vector<unsigned char> plainBytes(serializedData.begin(), serializedData.end());
    std::vector<unsigned char> newCipherText = Crypto::encryptVault(plainBytes, this->derivedKey, this->vaultNonce);

    bool saveSucceeded = Vault::saveEncryptedVaultToFile(this->vaultFilePath, newCipherText, this->vaultSalt, this->vaultNonce);

    Crypto::secureClean(serializedData);
    Crypto::secureClean(plainBytes);

    if (saveSucceeded) {
        this->vaultCipherText = newCipherText;
    }

    return saveSucceeded;
}

void MainWindow::onSortButtonClicked()
{
    this->sortAscending = !this->sortAscending;
    if (this->sortAscending) {
        this->sortButton->setText("Sort by: Title ▵");
    } else {
        this->sortButton->setText("Sort by: Title ▿");
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

    // Sort entries
    std::sort(this->currentCredentials.begin(), this->currentCredentials.end(),
        [this](const Credential& a, const Credential& b) {
            QString nameA = QString::fromStdString(a.websiteName).toLower();
            QString nameB = QString::fromStdString(b.websiteName).toLower();
            return this->sortAscending ? (nameA < nameB) : (nameA > nameB);
        });

    QString normalizedFilter = filterText.trimmed().toLower();
    int showingCount = 0;
    int totalCount = static_cast<int>(this->currentCredentials.size());

    // Render rows
    for (const Credential& cred : this->currentCredentials)
    {
        QString web = QString::fromStdString(cred.websiteName);
        QString user = QString::fromStdString(cred.userName);
        QString notes = QString::fromStdString(cred.notes);
        QString cat = QString::fromStdString(cred.category);
        QString url = QString::fromStdString(cred.url);

        if (!normalizedFilter.isEmpty()) {
            bool matches = web.toLower().contains(normalizedFilter) ||
                            user.toLower().contains(normalizedFilter) ||
                            notes.toLower().contains(normalizedFilter) ||
                            cat.toLower().contains(normalizedFilter) ||
                            url.toLower().contains(normalizedFilter);
            if (!matches) continue;
        }

        QWidget* rowWidget = createCredentialRow(cred);
        this->credentialsListLayout->addWidget(rowWidget);
        showingCount++;
    }

    this->credentialsListLayout->addStretch();

    // Update Stats Label
    QString statsHtml = QString(
        "<span style='color:#71717A;'>Vault:</span> <b style='color:#FFFFFF;'>%1</b> &nbsp;&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;&nbsp; "
        "<span style='color:#71717A;'>Items:</span> <b style='color:#FFFFFF;'>%2</b> (Showing %3) &nbsp;&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;&nbsp; "
        "<span style='color:#71717A;'>Cipher:</span> <span style='color:#4ADE80;'>XChaCha20-Poly1305</span> &nbsp;&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;&nbsp; "
        "<span style='color:#71717A;'>KDF:</span> <span style='color:#4ADE80;'>Argon2id</span>"
    ).arg(this->activeVaultName).arg(totalCount).arg(showingCount);

    this->statsTextLabel->setText(statsHtml);
}

QWidget* MainWindow::createCredentialRow(const Credential& cred)
{
    QFrame* row = new QFrame();
    row->setFixedHeight(68);
    row->setStyleSheet("QFrame { background-color: #18181B; border-bottom: 1px solid #27272A; } QFrame:hover { background-color: #202024; }");

    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(20, 8, 20, 8);
    layout->setSpacing(16);

    // Avatar / Category Icon
    QLabel* avatar = new QLabel(getCategoryIcon(cred.category));
    avatar->setFixedSize(38, 38);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet("background-color: #200B11; color: #E11D48; border: 1px solid #3F151F; border-radius: 8px; font-size: 16px;");

    // Title & URL column
    QVBoxLayout* webLayout = new QVBoxLayout();
    webLayout->setSpacing(2);
    webLayout->setAlignment(Qt::AlignVCenter);

    QLabel* webLabel = new QLabel(QString::fromStdString(cred.websiteName));
    webLabel->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 14px; border: none; background: transparent;");

    QString displayUrl = QString::fromStdString(cred.url);
    if (displayUrl.isEmpty()) {
        displayUrl = "No URL specified";
    }
    QLabel* urlLabel = new QLabel(displayUrl);
    urlLabel->setStyleSheet("color: #71717A; font-size: 11px; font-family: monospace; border: none; background: transparent;");

    webLayout->addWidget(webLabel);
    webLayout->addWidget(urlLabel);

    QWidget* webWrapper = new QWidget();
    webWrapper->setFixedWidth(180);
    webWrapper->setLayout(webLayout);
    webWrapper->setStyleSheet("border: none; background: transparent;");

    // Username column
    QLabel* userLabel = new QLabel(QString::fromStdString(cred.userName));
    userLabel->setFixedWidth(190);
    userLabel->setStyleSheet("color: #D4D4D8; font-family: monospace; font-size: 13px; border: none; background: transparent;");

    // Password column (masked by default)
    QLabel* passLabel = new QLabel(kMaskedPassword);
    passLabel->setFixedWidth(140);
    passLabel->setStyleSheet("color: #71717A; font-size: 16px; letter-spacing: 2px; border: none; background: transparent;");

    // Category Pill
    QString catText = QString::fromStdString(cred.category.empty() ? "Logins" : cred.category);
    QLabel* catLabel = new QLabel(catText);
    catLabel->setAlignment(Qt::AlignCenter);
    catLabel->setFixedWidth(110);
    catLabel->setFixedHeight(24);
    catLabel->setStyleSheet("background-color: #27272A; color: #A1A1AA; border: 1px solid #3F3F46; border-radius: 4px; font-size: 11px; font-weight: 500;");

    // Action buttons layout
    QHBoxLayout* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(6);

    QString btnStyle = "QPushButton { background-color: #18181B; color: #A1A1AA; border: 1px solid #27272A; border-radius: 6px; font-size: 13px; font-family: 'Segoe UI Emoji', 'Segoe UI Symbol', 'Noto Color Emoji', sans-serif; padding: 0px; } QPushButton:hover { background-color: #27272A; color: #FFFFFF; border-color: #3F3F46; }";

    QPushButton* viewBtn = new QPushButton("👁"); viewBtn->setFixedSize(30, 30); viewBtn->setStyleSheet(btnStyle); viewBtn->setCursor(Qt::PointingHandCursor); viewBtn->setToolTip("View Password");
    QPushButton* copyPassBtn = new QPushButton("📋"); copyPassBtn->setFixedSize(30, 30); copyPassBtn->setStyleSheet(btnStyle); copyPassBtn->setCursor(Qt::PointingHandCursor); copyPassBtn->setToolTip("Copy Password");
    QPushButton* copyUserBtn = new QPushButton("👤"); copyUserBtn->setFixedSize(30, 30); copyUserBtn->setStyleSheet(btnStyle); copyUserBtn->setCursor(Qt::PointingHandCursor); copyUserBtn->setToolTip("Copy Username");
    QPushButton* linkBtn = new QPushButton("🔗"); linkBtn->setFixedSize(30, 30); linkBtn->setStyleSheet(btnStyle); linkBtn->setCursor(Qt::PointingHandCursor); linkBtn->setToolTip("Open URL");
    QPushButton* editBtn = new QPushButton("✏️"); editBtn->setFixedSize(30, 30); editBtn->setStyleSheet(btnStyle); editBtn->setCursor(Qt::PointingHandCursor); editBtn->setToolTip("Edit Item");
    QPushButton* deleteBtn = new QPushButton("🗑"); deleteBtn->setFixedSize(30, 30); deleteBtn->setStyleSheet(btnStyle); deleteBtn->setCursor(Qt::PointingHandCursor); deleteBtn->setToolTip("Delete Item");

    if (cred.url.empty()) {
        linkBtn->setEnabled(false);
        linkBtn->setStyleSheet("QPushButton { background-color: transparent; color: #3F3F46; border: 1px solid #27272A; border-radius: 6px; font-size: 13px; }");
    }

    std::string credId = cred.id;

    // View Password toggle
    connect(viewBtn, &QPushButton::clicked, [this, credId, passLabel, viewBtn]() {
        if (passLabel->text() == kMaskedPassword) {
            auto it = std::find_if(this->currentCredentials.begin(), this->currentCredentials.end(),
                [&](const Credential& c) { return c.id == credId; });
            if (it != this->currentCredentials.end()) {
                passLabel->setText(QString::fromStdString(it->userPassword));
                passLabel->setStyleSheet("color: #4ADE80; font-size: 13px; letter-spacing: 0px; border: none; background: transparent; font-family: monospace;");
                viewBtn->setText("🙈");
            }
        } else {
            passLabel->setText(kMaskedPassword);
            passLabel->setStyleSheet("color: #71717A; font-size: 16px; letter-spacing: 2px; border: none; background: transparent;");
            viewBtn->setText("👁");
        }
    });

    // Copy Password
    connect(copyPassBtn, &QPushButton::clicked, [this, credId, copyPassBtn]() {
        auto it = std::find_if(this->currentCredentials.begin(), this->currentCredentials.end(),
            [&](const Credential& c) { return c.id == credId; });
        if (it != this->currentCredentials.end()) {
            this->copyWithAutoClear(QString::fromStdString(it->userPassword));
            copyPassBtn->setText("✓");
            QTimer::singleShot(1500, copyPassBtn, [copyPassBtn]() { copyPassBtn->setText("📋"); });
        }
    });

    // Copy Username
    connect(copyUserBtn, &QPushButton::clicked, [this, credId, copyUserBtn]() {
        auto it = std::find_if(this->currentCredentials.begin(), this->currentCredentials.end(),
            [&](const Credential& c) { return c.id == credId; });
        if (it != this->currentCredentials.end()) {
            this->copyWithAutoClear(QString::fromStdString(it->userName));
            copyUserBtn->setText("✓");
            QTimer::singleShot(1500, copyUserBtn, [copyUserBtn]() { copyUserBtn->setText("👤"); });
        }
    });

    // Open URL
    connect(linkBtn, &QPushButton::clicked, [this, credId]() {
        auto it = std::find_if(this->currentCredentials.begin(), this->currentCredentials.end(),
            [&](const Credential& c) { return c.id == credId; });
        if (it != this->currentCredentials.end()) {
            this->openCredentialUrl(QString::fromStdString(it->url));
        }
    });

    // Edit Item
    connect(editBtn, &QPushButton::clicked, [this, credId]() {
        this->editCredential(credId);
    });

    // Delete Item
    connect(deleteBtn, &QPushButton::clicked, [this, credId]() {
        this->deleteCredential(credId);
    });

    actionLayout->addWidget(viewBtn);
    actionLayout->addWidget(copyPassBtn);
    actionLayout->addWidget(copyUserBtn);
    actionLayout->addWidget(linkBtn);
    actionLayout->addWidget(editBtn);
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

void MainWindow::copyWithAutoClear(const QString& text)
{
    if (text.isEmpty()) return;

    QByteArray utf8Data = text.toUtf8();
    QByteArray hash = QCryptographicHash::hash(utf8Data, QCryptographicHash::Sha256);
    if (!utf8Data.isEmpty()) {
        sodium_memzero(utf8Data.data(), utf8Data.size());
    }

    this->lastCopiedHash = hash;
    QApplication::clipboard()->setText(text);

    QTimer::singleShot(25000, this, [this, hash]() {
        if (this->lastCopiedHash == hash) {
            QByteArray currentClip = QApplication::clipboard()->text().toUtf8();
            QByteArray currentHash = QCryptographicHash::hash(currentClip, QCryptographicHash::Sha256);
            if (!currentClip.isEmpty()) {
                sodium_memzero(currentClip.data(), currentClip.size());
            }
            if (currentHash == hash) {
                QApplication::clipboard()->clear();
            }
            this->lastCopiedHash.clear();
        }
    });
}

void MainWindow::openCredentialUrl(const QString& rawUrl)
{
    QString trimmed = rawUrl.trimmed();
    if (trimmed.isEmpty()) return;

    if (!trimmed.startsWith("http://", Qt::CaseInsensitive) &&
        !trimmed.startsWith("https://", Qt::CaseInsensitive)) {
        trimmed = "https://" + trimmed;
    }

    QDesktopServices::openUrl(QUrl(trimmed));
}

void MainWindow::editCredential(const std::string& credentialId)
{
    auto it = std::find_if(this->currentCredentials.begin(), this->currentCredentials.end(),
        [&](const Credential& c) { return c.id == credentialId; });
    if (it == this->currentCredentials.end()) return;

    CredentialDialog dialog(this, &(*it));
    if (dialog.exec() == QDialog::Accepted) {
        Credential updated = dialog.getCredential();
        *it = updated;

        if (!this->saveCurrentVaultState()) {
            QMessageBox::critical(this, "Save Error", "Failed to save updated credential to disk.");
        }
        this->refreshCredentialsTable();
    }
}

void MainWindow::deleteCredential(const std::string& credentialId)
{
    auto it = std::find_if(this->currentCredentials.begin(), this->currentCredentials.end(),
        [&](const Credential& c) { return c.id == credentialId; });
    if (it == this->currentCredentials.end()) return;

    QString itemTitle = QString::fromStdString(it->websiteName);
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirm Deletion",
        QString("Are you sure you want to delete '%1'?").arg(itemTitle),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        this->currentCredentials.erase(it);
        if (!this->saveCurrentVaultState()) {
            QMessageBox::critical(this, "Disk Error", "Failed to update vault file after deletion.");
        }
        this->refreshCredentialsTable();
    }
}

void MainWindow::onAddCredentialButtonClicked()
{
    if (!this->isVaultUnlocked) return;

    CredentialDialog dialog(this, nullptr);
    if (dialog.exec() == QDialog::Accepted) {
        Credential newCred = dialog.getCredential();
        this->currentCredentials.push_back(newCred);

        if (!this->saveCurrentVaultState()) {
            QMessageBox::critical(this, "Save Error", "Failed to save the new item to disk.");
            this->currentCredentials.pop_back();
            return;
        }

        this->refreshCredentialsTable();
    }
}

void MainWindow::onGeneratePasswordButtonClicked()
{
    PasswordGeneratorDialog dialog(this);
    dialog.exec();
}

void MainWindow::onChangePasswordClicked()
{
    ChangeMasterPasswordDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;

    std::string oldPass = dialog.getOldPassword();
    std::string newPass = dialog.getNewPassword();

    // 1. Verify current master key
    std::vector<unsigned char> testKey = Crypto::deriveKey(oldPass, this->vaultSalt);
    if (testKey != this->derivedKey) {
        QMessageBox::critical(this, "Authentication Failed", "The current master password is incorrect.");
        Crypto::secureClean(testKey);
        Crypto::secureClean(oldPass);
        Crypto::secureClean(newPass);
        return;
    }
    Crypto::secureClean(testKey);

    // 2. Generate a fresh salt and derive new key
    std::vector<unsigned char> newSalt = Crypto::generateSalt();
    std::vector<unsigned char> newKey = Crypto::deriveKey(newPass, newSalt);

    std::vector<unsigned char> previousKey = this->derivedKey;
    std::vector<unsigned char> previousSalt = this->vaultSalt;
    this->derivedKey = newKey;
    this->vaultSalt = newSalt;

    if (this->saveCurrentVaultState()) {
        QMessageBox::information(this, "Success", "Master password has been updated securely.");
        Crypto::secureClean(previousKey);
        Crypto::secureClean(previousSalt);
    } else {
        QMessageBox::critical(this, "Disk Error", "Failed to save the vault with the new master key.");
        this->derivedKey = previousKey;
        this->vaultSalt = previousSalt;
        Crypto::secureClean(newKey);
        Crypto::secureClean(newSalt);
    }

    Crypto::secureClean(oldPass);
    Crypto::secureClean(newPass);
}

void MainWindow::onUnlockRequested(const QString& vaultName, const std::string& masterPassword)
{
    std::string errorMessage = "";
    bool unlockSucceeded = this->attemptUnlockVault(vaultName, masterPassword, errorMessage);

    if (unlockSucceeded) {
        this->isVaultUnlocked = true;
        if (this->inactivityTimer) {
            this->inactivityTimer->start();
        }
        this->refreshVaultSwitcher();
        this->refreshCredentialsTable();
        this->stackedWidget->setCurrentWidget(this->vaultView);
    } else {
        this->loginView->showErrorMessage(QString::fromStdString(errorMessage));
    }
}

void MainWindow::onCreateVaultRequested(const QString& vaultName, const std::string& masterPassword)
{
    std::string errorMessage = "";
    bool created = this->createAndInitializeVault(vaultName, masterPassword, errorMessage);

    if (created) {
        this->isVaultUnlocked = true;
        if (this->inactivityTimer) {
            this->inactivityTimer->start();
        }
        this->refreshVaultSwitcher();
        this->refreshCredentialsTable();
        this->stackedWidget->setCurrentWidget(this->vaultView);
    } else {
        this->loginView->showErrorMessage(QString::fromStdString(errorMessage));
    }
}

bool MainWindow::createAndInitializeVault(const QString& vaultName, const std::string& masterPassword, std::string& errorMessage)
{
    this->activeVaultName = vaultName;
    this->vaultFilePath = Vault::getVaultFilePath(vaultName).toStdString();

    this->vaultSalt = Crypto::generateSalt();
    this->vaultNonce = Crypto::generateNonce();
    this->derivedKey = Crypto::deriveKey(masterPassword, this->vaultSalt);
    this->currentCredentials.clear();

    if (!this->saveCurrentVaultState()) {
        errorMessage = "Failed to create vault file on disk.";
        return false;
    }

    this->loginView->refreshVaultList(vaultName);
    return true;
}

bool MainWindow::attemptUnlockVault(const QString& vaultName, const std::string& masterPassword, std::string& errorMessage)
{
    this->activeVaultName = vaultName;
    this->vaultFilePath = Vault::getVaultFilePath(vaultName).toStdString();

    std::vector<unsigned char> loadedCipherText;
    std::vector<unsigned char> loadedSalt;
    std::vector<unsigned char> loadedNonce;

    bool fileLoaded = Vault::loadEncryptedVaultFromFile(this->vaultFilePath, loadedCipherText, loadedSalt, loadedNonce);

    if (!fileLoaded) {
        // Prompt user to create vault if file does not exist
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Vault Not Found",
            QString("Vault '%1' does not exist yet. Would you like to create it now with this password?").arg(vaultName),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            return this->createAndInitializeVault(vaultName, masterPassword, errorMessage);
        } else {
            errorMessage = "Vault file missing.";
            return false;
        }
    }

    // Normal unlock & decryption
    std::vector<unsigned char> candidateKey = Crypto::deriveKey(masterPassword, loadedSalt);
    std::vector<unsigned char> decryptedBytes = Crypto::decryptVault(loadedCipherText, candidateKey, loadedNonce);

    if (decryptedBytes.empty()) {
        Crypto::secureClean(candidateKey);
        errorMessage = "Incorrect Master Password";
        return false;
    }

    std::string rawData(decryptedBytes.begin(), decryptedBytes.end());
    if (rawData == "ATELIER_EMPTY_VAULT") {
        rawData.clear();
    }

    std::vector<Credential> loadedCredentials;
    if (!rawData.empty()) {
        loadedCredentials = Vault::deserializeCredentials(rawData);
    }

    Crypto::secureClean(rawData);
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
    QApplication::clipboard()->clear();
    this->lastCopiedHash.clear();
    if (this->inactivityTimer) {
        this->inactivityTimer->stop();
    }
    this->secureWipeSensitiveState();
    this->searchBar->clear();
    this->populateCredentialsTable("");
    this->loginView->clearInput();
    this->loginView->refreshVaultList(this->activeVaultName);
    this->stackedWidget->setCurrentWidget(this->loginView);
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
        Crypto::secureClean(this->currentCredentials[i].userPassword);
        Crypto::secureClean(this->currentCredentials[i].userName);
        Crypto::secureClean(this->currentCredentials[i].websiteName);
        Crypto::secureClean(this->currentCredentials[i].url);
        Crypto::secureClean(this->currentCredentials[i].notes);
        Crypto::secureClean(this->currentCredentials[i].category);
        Crypto::secureClean(this->currentCredentials[i].id);
    }

    this->currentCredentials.clear();
    Crypto::secureClean(this->vaultSalt);
    Crypto::secureClean(this->vaultNonce);
    Crypto::secureClean(this->vaultCipherText);
    this->isVaultUnlocked = false;

    if (this->inactivityTimer) {
        this->inactivityTimer->stop();
    }
}

void MainWindow::closeEvent(QCloseEvent* closeEventPointer)
{
    QApplication::clipboard()->clear();
    this->lastCopiedHash.clear();
    if (this->inactivityTimer) {
        this->inactivityTimer->stop();
    }
    this->secureWipeSensitiveState();
    QMainWindow::closeEvent(closeEventPointer);
}