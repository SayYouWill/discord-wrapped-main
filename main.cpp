#include <sqlite3.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <cctype>

#include "account.h"

int timeZoneOffset = 0;

std::vector<Account> accounts; // accounts on this machine

Account* focusedAccount = nullptr; // account being scraped/analyzed in this session

// function to map symbolic time zones to their offset values
int getTimeZoneOffset(const std::string& symbolicTimeZone) {
    if (symbolicTimeZone == "EST") {
        return -5; // eastern standard time (UTC-5)
    } else if (symbolicTimeZone == "PST") {
        return -8; // pacific standard time (UTC-8)
    } else if (symbolicTimeZone == "GMT") {
        return 0; // greenwich mean time (UTC+0)
    } else {
        std::cout << "timezone not recognized, defaulting to GMT (-0)" << std::endl;
        return 0; // default to GMT if symbolic timezone is not recognized
    }
}

// function to check if there is an existing database
bool hasExistingDatabase(const std::string& directoryPath) {

    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".db") {
            return true;
        }
    }
    return false;
}

// function to get a list of existing databases
std::vector<std::string> getExistingDatabases(const std::string& directoryPath) {
    std::vector<std::string> dbNames;

    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".db") {
            std::string fileName = entry.path().filename().stem();
            dbNames.push_back(fileName);
        }
    }

    return dbNames;
}

// function to check if a character is 'Y' or 'N', non case sensitive
bool isYes(char response) {
    response = std::toupper(response);
    
    // check if the response is either 'Y' or 'N'
    if (response == 'Y' || response == 'N') {
        return true;
    } else {
        std::cout << "Invalid input. Please enter 'Y' or 'N'." << std::endl;
        return false;
    }
}

// function to check if a string is "create" or "update" or "delete"
bool isOption(std::string option) {
    if (option == "create" || option == "update" || option == "delete") {
        return true;
    } else {
        std::cout << "that is not a valid input." << std::endl;
        return false;
    }
}

int main(int argc, char *argv[]) {
    std::cout << std::endl;
    std::cout << "welcome to discord wrapped." << std::endl;

    try {
        if (argc != 2) {
            throw std::invalid_argument("usage: <executable> <symbolicTimeZone>");
        }

        timeZoneOffset = getTimeZoneOffset(argv[1]); // set global timezone offset from symbolic timezone
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }

    // program startup prompting.
    // - account vector will be brought up to date with existing databases
    // - creates new database if necessary
    // - picks an account to focus on for the remainder of the execution
    char charResponse;
    if (!hasExistingDatabase(".")) {
        std::string username;
        std::string auth_token;
        std::cout << "we didn't find any account data on this computer. is this your first time operating?" << std::endl;
        do {
            std::cout << "(Y/N): "; std::cin >> charResponse; std::cout << std::endl;
        } while (!isYes(charResponse));

        if (std::toupper(charResponse) == 'Y') {
            std::cout << "this program requires two things to get started: your discord username, and your authentication token."               << std::endl;
            std::cout << "for this program, your discord username is simply your discord ID. it is distinct from your display name."            << std::endl;
            std::cout << "it can be found easily by hovering over your name/account in the bottom left of your discord client."                 << std::endl;
            std::cout << "input username: "; std::cin >> username; std::cout << std::endl;

            std::cout << "the auth token is a bit more complicated. because discord doesn't particularly like it when people bot DMs,"          << std::endl;
            std::cout << "there is no way to find this token integrated into discord itself. here is a step by step for finding yours:"         << std::endl;
            std::cout << " - sign into discord on a browser and inspect element (usually with f12)"                                             << std::endl;
            std::cout << " - in the inspect element menu, navigate to the 'network' tab"                                                        << std::endl;
            std::cout << " - click on any request with the xhr initiator type"                                                                  << std::endl;
            std::cout << " - under the 'headers' tab of that request, find 'request headers' and look for the key labelled 'authorization'."    << std::endl;
            std::cout << " - copy the long string of letters and numbers next to 'authorization'."                                          << std::endl;
            std::cout << "DO NOT SHARE THIS TOKEN WITH ANYONE / ACCIDENTALLY LEAK IT!"                                                          << std::endl;
            std::cout << "paste auth token: "; std::cin >> auth_token; std::cout << std::endl;

            std::cout << "this program will now create a local database to store all of the information retrieved from this account."           << std::endl;
            std::cout << "the database will be used by the program to manipulate data for presentation. you will have the option when"          << std::endl;
            std::cout << "exiting to choose between either deleting the database, or saving it in this program's directory to view or"          << std::endl;
            std::cout << "update later. note that none of the data fetched by this program will ever leave this computer, including your"       << std::endl;
            std::cout << "auth token."                                                                                                          << std::endl;

            std::cout << std::endl << "none of these detailed instructions will be sent if you say you have operated previous or there is an"   << std::endl;
            std::cout << "existing database." << std::endl;

        } else {
            std::cout << "input username: "; std::cin >> username;
            std::cout << "paste auth token: "; std::cin >> auth_token;
        }

        Account account(username, auth_token);
        account.createDatabase();
        accounts.push_back(account);
        focusedAccount = &account;

        // std::cout << "database created for user '" << username << "'." << std::endl;

    } else {
        std::string option;
        std::cout << "we found the following existing account databases in this program's directory: " << std::endl;
        // list existing databases and sync account vector with them
        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            if (entry.is_regular_file() && entry.path().extension() == ".db") {
                std::string username = entry.path().stem().string();

                sqlite3* db;
                int rc = sqlite3_open(entry.path().c_str(), &db);
                if (rc) {
                    std::cerr << "can't open database: " << sqlite3_errmsg(db) << std::endl;
                    continue; // skip to the next database if opening failed
                }

                std::string auth_token;
                const char* selectSQL = "SELECT token FROM AuthToken LIMIT 1;";
                sqlite3_stmt* stmt;
                if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) == SQLITE_OK) {
                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                        auth_token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    }
                }
                sqlite3_finalize(stmt);

                sqlite3_close(db);
                Account account(username, auth_token);
                accounts.push_back(account);
                
                std::cout << "- " << entry.path().stem() << std::endl;
            }
        }

        std::cout << "would you like to create a new account, update an existing account, or delete an existing account?" << std::endl;
        do {
            std::cout << "(create/update/delete): "; std::cin >> option; std::cout << std::endl;
        } while (!isOption(option));

        if (option == "create") {
            std::string username;
            std::string auth_token;
            std::cout << "input new username: "; std::cin >> username;
            std::cout << "paste auth token: "; std::cin >> auth_token;
            Account account(username, auth_token);
            account.createDatabase();
            accounts.push_back(account);
            focusedAccount = &account;
        } else if (option == "update") {
            std::string username;
            std::cout << "input username of account database you would like to update: "; std::cin >> username;
            // find username in vector
            for (auto& account : accounts) {
                if (account.username == username) {
                    focusedAccount = &account;
                    break;
                }
            }
            // check if account wasn't found
            if (!(focusedAccount != nullptr)) {
                std::cout << "database with username '" << username << "' not found." << std::endl;
            }
        } else if (option == "delete") {
            std::string username;
            std::cout << "input username of account database you would like to delete: "; std::cin >> username;
            // find username in vector
            Account* foundAccount = nullptr;
            for (auto& account : accounts) {
                if (account.username == username) {
                    foundAccount = &account;
                    break;
                }
            }
            // check if account was found
            if (foundAccount != nullptr) {
                // delete account
                foundAccount -> deleteDatabase();
            } else {
                std::cout << "database with username '" << username << "' not found." << std::endl;
            }
        } else {
            std::cout << "???" << std::endl;
        }
    }

    std::cout << std::endl;
    if (focusedAccount == nullptr) {
        std::cout << "no account to focus, session used for deletion. restart if you would like to scan." << std::endl;
        return 0;
    } else {
        std::cout << "focused account: " << focusedAccount -> username << std::endl;
    }

    return 0;
}