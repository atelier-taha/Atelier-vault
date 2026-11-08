#include <QApplication>
#include <QIcon>
#include "mainwindow.h"
#include "crypto.h"

int main(int argc, char* argv[])
{
    bool cryptoStatus = Crypto::initializeCrypto();
    if (!cryptoStatus)
    {
        return 1;
    }

    QApplication applicationInstance(argc, argv);

    // Set Application Name for consistent AppData paths across platforms
    QApplication::setApplicationName("AtelierVault");

    // Set Desktop Entry Name so Wayland/GNOME maps it cleanly to dock
    QApplication::setDesktopFileName("AtelierVault");

    // Load compiled-in Qt Resource icon
    applicationInstance.setWindowIcon(QIcon(":/icon.png"));

    MainWindow mainApplicationWindow;
    mainApplicationWindow.show();

    return applicationInstance.exec();
}