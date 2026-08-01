#include "loginview.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QFont>
#include <algorithm>

LoginView::LoginView(QWidget* parentWidget)
    : QWidget(parentWidget)
{
    this->setupUi();
}

void LoginView::setupUi()
{
    // Deep dark background
    this->setStyleSheet("background-color: #0A0A0C;");

    this->setAttribute(Qt::WA_StyledBackground, true);
    
    // Set your dark red background (e.g., #1A0505 or #200808)
    this->setStyleSheet("background-color: #1A0505;");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    // Main Login Card
    QFrame* card = new QFrame(this);
    card->setFixedSize(450, 480);
    card->setObjectName("loginCard");
    card->setStyleSheet(
        "#loginCard {"
        "   background-color: #121216;"
        "   border: 1px solid #1F1F26;"
        "   border-radius: 12px;"
        "}"
    );

    // Red ambient card glow
    QGraphicsDropShadowEffect* cardGlow = new QGraphicsDropShadowEffect(this);
    cardGlow->setBlurRadius(40);
    cardGlow->setColor(QColor(211, 47, 47, 40));
    cardGlow->setOffset(0, 0);
    card->setGraphicsEffect(cardGlow);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(40, 40, 40, 30);
    cardLayout->setSpacing(0);

    // Lock Icon Header
    QLabel* lockIcon = new QLabel("🔒", card);
    lockIcon->setAlignment(Qt::AlignCenter);
    lockIcon->setFixedSize(60, 60);
    lockIcon->setStyleSheet(
        "color: #D32F2F;"
        "font-size: 24px;"
        "border: 1px solid #401515;"
        "border-radius: 30px;"
        "background-color: #1A0B0B;"
    );

    QHBoxLayout* iconLayout = new QHBoxLayout();
    iconLayout->setAlignment(Qt::AlignCenter);
    iconLayout->addWidget(lockIcon);

    // Title & Subtitle
    QLabel* titleLabel = new QLabel("Atelier's Vault", card);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 26px; font-weight: bold; margin-top: 15px; background: transparent;");

    QLabel* subtitleLabel = new QLabel("SECURE PASSWORD MANAGER", card);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    QFont subtitleFont = subtitleLabel->font();
    subtitleFont.setPixelSize(10);
    subtitleFont.setLetterSpacing(QFont::AbsoluteSpacing, 3.0);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setStyleSheet("color: #666677; margin-bottom: 30px; background: transparent;");

    // Master Password Label
    QLabel* inputLabel = new QLabel("MASTER PASSWORD", card);
    inputLabel->setStyleSheet("color: #888899; font-size: 11px; font-weight: bold; margin-bottom: 5px; background: transparent;");

    // Input Box Container
    QFrame* inputContainer = new QFrame(card);
    inputContainer->setFixedHeight(45);
    inputContainer->setStyleSheet(
        "QFrame {"
        "   background-color: #1A1A21;"
        "   border: 1px solid #2A2A35;"
        "   border-radius: 6px;"
        "}"
        "QFrame:focus-within {"
        "   border: 1px solid #D32F2F;"
        "}"
    );

    QHBoxLayout* inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(15, 0, 10, 0);

    this->passwordInput = new QLineEdit(inputContainer);
    this->passwordInput->setEchoMode(QLineEdit::Password);
    this->passwordInput->setPlaceholderText("Enter Master Password ...");
    this->passwordInput->setStyleSheet("background: transparent; border: none; color: #FFFFFF; font-size: 14px; font-family: monospace;");

    this->eyeButton = new QPushButton("👁", inputContainer);
    this->eyeButton->setCursor(Qt::PointingHandCursor);
    this->eyeButton->setFixedSize(24, 24);
    this->eyeButton->setStyleSheet("background: transparent; border: none; color: #666677; font-size: 14px;");

    inputLayout->addWidget(this->passwordInput);
    inputLayout->addWidget(this->eyeButton);

    // Unlock Button
    this->unlockButton = new QPushButton("🔓 Unlock Vault", card);
    this->unlockButton->setFixedHeight(45);
    this->unlockButton->setCursor(Qt::PointingHandCursor);
    this->unlockButton->setStyleSheet(
        "QPushButton {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #D32F2F, stop:1 #B71C1C);"
        "   color: #FFFFFF;"
        "   border: none;"
        "   border-radius: 6px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   margin-top: 15px;"
        "}"
        "QPushButton:hover {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #E53935, stop:1 #C62828);"
        "}"
        "QPushButton:pressed {"
        "   background: #9E1313;"
        "}"
    );

    QGraphicsDropShadowEffect* buttonGlow = new QGraphicsDropShadowEffect(this);
    buttonGlow->setBlurRadius(20);
    buttonGlow->setColor(QColor(211, 47, 47, 100));
    buttonGlow->setOffset(0, 4);
    this->unlockButton->setGraphicsEffect(buttonGlow);

    // Error Label
    this->errorLabel = new QLabel("", card);
    this->errorLabel->setAlignment(Qt::AlignCenter);
    this->errorLabel->setStyleSheet("color: #FF5252; font-size: 12px; margin-top: 5px; background: transparent;");
    this->errorLabel->setFixedHeight(20);

    // Footer
    QFrame* separator = new QFrame(card);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background-color: #2A2A35; margin-top: 10px; margin-bottom: 15px;");

    QLabel* footerLabel = new QLabel("🛡️ Encryption: Argon2id + AES-256-GCM", card);
    footerLabel->setAlignment(Qt::AlignCenter);
    footerLabel->setStyleSheet("color: #666677; font-family: monospace; font-size: 11px; background: transparent;");

    // Assemble Card Layout
    cardLayout->addLayout(iconLayout);
    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(subtitleLabel);
    cardLayout->addWidget(inputLabel);
    cardLayout->addWidget(inputContainer);
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
}

void LoginView::onUnlockTriggered()
{
    this->errorLabel->clear();
    std::string masterPassword = this->passwordInput->text().toStdString();

    emit unlockRequested(masterPassword);

    this->passwordInput->clear();
    std::fill(masterPassword.begin(), masterPassword.end(), '\0');
}

void LoginView::onTogglePasswordVisibility()
{
    if (this->passwordInput->echoMode() == QLineEdit::Password)
    {
        this->passwordInput->setEchoMode(QLineEdit::Normal);
        this->eyeButton->setStyleSheet("background: transparent; border: none; color: #D32F2F; font-size: 14px;");
    }
    else
    {
        this->passwordInput->setEchoMode(QLineEdit::Password);
        this->eyeButton->setStyleSheet("background: transparent; border: none; color: #666677; font-size: 14px;");
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
}