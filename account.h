#pragma once

#include <string>

class Account {
    public: 
        std::string username;
        std::string auth_token;

        Account(const std::string& _username, const std::string& _auth_token)
            : username(_username), auth_token(_auth_token) {}

        void createDatabase();
        void deleteDatabase();

    private:
};