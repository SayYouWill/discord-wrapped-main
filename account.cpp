#include <iostream>
#include <sqlite3.h>
#include <string>
#include <filesystem>

#include "account.h"

void Account::createDatabase() {
    std::string dbFilename = username + ".db";

    // check if database already exists
    if (std::filesystem::exists(dbFilename)) {
        std::cout << "database for user '" << username << "' already exists." << std::endl;
        return;
    }

    // create the database for this account
    sqlite3* db;
    char* errMsg = nullptr;
    int rc = sqlite3_open(dbFilename.c_str(), &db);
    if (rc) {
        std::cerr << "can't open database: " << sqlite3_errmsg(db) << std::endl;
    } // else { std::cout << "database created successfully: " << dbFilename << std::endl; }

    // create table to store the auth token
    const char* createTableSQL = "CREATE TABLE AuthToken (token TEXT NOT NULL);";
    rc = sqlite3_exec(db, createTableSQL, nullptr, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "failed to create authtoken table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return;
    }

    // insert auth token to its table (very lonely in there)
    std::string insertSQL = "INSERT INTO AuthToken (token) VALUES ('" + auth_token + "');";
    rc = sqlite3_exec(db, insertSQL.c_str(), nullptr, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "failed to insert auth_token into authtoken table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } // else { std::cout << "auth token inserted successfully." << std::endl;}

    sqlite3_close(db);
}

void Account::deleteDatabase() {
    std::string dbFilename = username + ".db";

    // check if database exists
    if (!std::filesystem::exists(dbFilename)) {
        std::cout << "database for user '" << username << "' does not exist." << std::endl;
        return;
    }

    if (std::filesystem::remove(dbFilename)) {
        std::cout << "database file for user '" << username << "' removed successfully." << std::endl;
    } else {
        std::cerr << "failed to remove database file for user '" << username << "'." << std::endl;
    }
}