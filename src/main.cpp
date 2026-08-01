#include <QApplication>
#include "mainwindow.h"
#include "crypto.h"

int main(int argc, char* argv[])
{
    // Initialize Libsodium cryptographic engine
    bool cryptoStatus = Crypto::initializeCrypto();
    if (!cryptoStatus)
    {
        return 1;
    }

    // Initialize Qt Application event loop
    QApplication applicationInstance(argc, argv);

    // Instantiate and show the MainWindow GUI
    MainWindow mainApplicationWindow;
    mainApplicationWindow.show();

    return applicationInstance.exec();
}