#include "ui.h"
#include <iostream>
#include <iomanip>

// Include termios for secure password typing on Linux systems
#ifdef __linux__
#include <termios.h>
#include <unistd.h>
#endif

using namespace std;

namespace Ui {

    // Clears the terminal screen for a clean user interface
    void clearScreen() {
        // We use standard ANSI escape codes to clear the screen and move the cursor to the top left
        cout << "\033[2J\033[1;1H";
    }

    // Displays the main logo and header of the password manager
    void displayHeader() {
        clearScreen();
        cout << "=====================================================" << endl;
        cout << "              SECURE PASSWORD MANAGER                " << endl;
        cout << "=====================================================" << endl;
        cout << endl;
    }

    // Safely gets a password from the user without showing the characters on screen
    string getHiddenInput(const string& promptMessage) {
        string hiddenPassword = "";
        cout << promptMessage << flush;

#ifdef __linux__
        struct termios oldSettings = {};
        struct termios newSettings = {};
        
        // Grab the current terminal settings
        tcgetattr(STDIN_FILENO, &oldSettings);
        
        // Copy the settings and turn off the ECHO flag (which stops characters from printing)
        newSettings = oldSettings;
        newSettings.c_lflag = newSettings.c_lflag & ~(ECHO);
        
        // Apply the new silent settings
        tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);
        
        // Get the password safely
        getline(cin, hiddenPassword);
        
        // Restore the original terminal settings so normal typing works again
        tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
        cout << endl;
#else
        // Fallback for Windows or standard C++ if termios is not available
        getline(cin, hiddenPassword);
#endif

        return hiddenPassword;
    }

    // Displays the main menu and returns the number of the user's choice
    int getMainMenuChoice() {
        int userChoice = 0;
        string inputLine = "";

        cout << "1. View Saved Credentials" << endl;
        cout << "2. Add New Credential" << endl;
        cout << "3. Lock Vault and Exit" << endl;
        cout << "-----------------------------------------------------" << endl;
        cout << "Enter your choice (1-3): " << flush;
        
        getline(cin, inputLine);
        
        // We read it as a string first to prevent infinite loops if the user types letters by accident
        if (inputLine == "1") {
            userChoice = 1;
        } else if (inputLine == "2") {
            userChoice = 2;
        } else if (inputLine == "3") {
            userChoice = 3;
        } else {
            userChoice = 0; // Represents an invalid choice
        }

        return userChoice;
    }

    // Asks the user for a website, username, and password to create a new saved login
    Credential getNewCredential() {
        Credential newLogin;
        
        cout << endl;
        cout << "--- Add New Credential ---" << endl;
        
        cout << "Enter Website or App Name: " << flush;
        getline(cin, newLogin.websiteName);
        
        cout << "Enter Username or Email: " << flush;
        getline(cin, newLogin.userName);
        
        cout << "Enter Password: " << flush;
        getline(cin, newLogin.userPassword);
        
        return newLogin;
    }

    // Prints all saved credentials in a perfectly aligned table format
    void displayVaultContents(const vector<Credential>& credentialList) {
        cout << endl;
        cout << "--- Your Saved Vault ---" << endl;
        
        if (credentialList.empty() == true) {
            cout << "[!] Your vault is currently empty." << endl;
        } else {
            // Print the table headers using setw to force specific column widths
            cout << left << setw(20) << "Website" 
                 << left << setw(30) << "Username" 
                 << left << setw(20) << "Password" << endl;
                 
            cout << "----------------------------------------------------------------------" << endl;
            
            // Loop through the list and print each credential in its aligned column
            for (const Credential& currentItem : credentialList) {
                cout << left << setw(20) << currentItem.websiteName 
                     << left << setw(30) << currentItem.userName 
                     << left << setw(20) << currentItem.userPassword << endl;
            }
            
            cout << "----------------------------------------------------------------------" << endl;
        }
    }

    // Pauses the application until the user presses the Enter key
    void promptContinue() {
        string dummyInput = "";
        cout << "\nPress Enter to continue..." << flush;
        getline(cin, dummyInput);
    }

} // namespace Ui