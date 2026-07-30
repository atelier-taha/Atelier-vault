#ifndef UI_H
#define UI_H

#include <string>
#include <vector>
#include "vault.h"

using namespace std;

namespace Ui {

    // Clears the terminal screen for a clean user interface
    void clearScreen();

    // Displays the main logo and header of the password manager
    void displayHeader();

    // Safely gets a password from the user without showing the characters on screen
    string getHiddenInput(const string& promptMessage);

    // Displays the main menu and returns the number of the user's choice
    int getMainMenuChoice();

    // Asks the user for a website, username, and password to create a new saved login
    Credential getNewCredential();

    // Prints all saved credentials in a perfectly aligned table format
    void displayVaultContents(const vector<Credential>& credentialList);

    // Pauses the application until the user presses the Enter key
    void promptContinue();

} // namespace Ui

#endif // UI_H