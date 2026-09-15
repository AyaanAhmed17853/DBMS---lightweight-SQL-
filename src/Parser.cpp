#include "Parser.h"
#include <sstream>
#include <iostream>
#include <cstring>
#include <algorithm>

PrepareResult Parser::prepare_statement(const std::string& input, Statement& statement) {
    std::istringstream iss(input);
    std::string command;
    iss >> command;

    // Case-insensitive command matching
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);

    if (command == "insert") {
        statement.type = StatementType::STATEMENT_INSERT;

        int id;
        std::string username, email;

        if (!(iss >> id >> username >> email)) {
            return PrepareResult::PREPARE_SYNTAX_ERROR;
        }

        if (id < 0) {
            return PrepareResult::PREPARE_NEGATIVE_ID;
        }

        if (username.length() > COLUMN_USERNAME_SIZE) {
            return PrepareResult::PREPARE_STRING_TOO_LONG;
        }

        if (email.length() > COLUMN_EMAIL_SIZE) {
            return PrepareResult::PREPARE_STRING_TOO_LONG;
        }

        statement.row_to_insert.id = id;
        strncpy(statement.row_to_insert.username, username.c_str(), COLUMN_USERNAME_SIZE);
        statement.row_to_insert.username[COLUMN_USERNAME_SIZE] = '\0';
        strncpy(statement.row_to_insert.email, email.c_str(), COLUMN_EMAIL_SIZE);
        statement.row_to_insert.email[COLUMN_EMAIL_SIZE] = '\0';

        return PrepareResult::PREPARE_SUCCESS;
    }

    if (command == "select") {
        statement.type = StatementType::STATEMENT_SELECT;
        std::string target;
        if (iss >> target) {
            try {
                int parsed_id = std::stoi(target);
                if (parsed_id < 0) {
                    return PrepareResult::PREPARE_NEGATIVE_ID;
                }
                statement.target_id = parsed_id;
            } catch (...) {
                statement.target_id = -1; // Non-numeric token, treat as select all
            }
        } else {
            statement.target_id = -1; // No argument, select all
        }
        return PrepareResult::PREPARE_SUCCESS;
    }

    if (command == "update") {
        statement.type = StatementType::STATEMENT_UPDATE;

        int id;
        std::string username, email;

        if (!(iss >> id >> username >> email)) {
            return PrepareResult::PREPARE_SYNTAX_ERROR;
        }

        if (id < 0) {
            return PrepareResult::PREPARE_NEGATIVE_ID;
        }

        if (username.length() > COLUMN_USERNAME_SIZE) {
            return PrepareResult::PREPARE_STRING_TOO_LONG;
        }

        if (email.length() > COLUMN_EMAIL_SIZE) {
            return PrepareResult::PREPARE_STRING_TOO_LONG;
        }

        // Reuse row_to_insert to carry the new row values.
        // The ID remains the key being updated.
        statement.target_id = id;
        statement.row_to_insert.id = id;
        strncpy(statement.row_to_insert.username, username.c_str(), COLUMN_USERNAME_SIZE);
        statement.row_to_insert.username[COLUMN_USERNAME_SIZE] = '\0';
        strncpy(statement.row_to_insert.email, email.c_str(), COLUMN_EMAIL_SIZE);
        statement.row_to_insert.email[COLUMN_EMAIL_SIZE] = '\0';

        return PrepareResult::PREPARE_SUCCESS;
    }

    if (command == "delete") {
        statement.type = StatementType::STATEMENT_DELETE;
        std::string target;
        if (iss >> target) {
            try {
                int parsed_id = std::stoi(target);
                if (parsed_id < 0) {
                    return PrepareResult::PREPARE_NEGATIVE_ID;
                }
                statement.target_id = parsed_id;
                return PrepareResult::PREPARE_SUCCESS;
            } catch (...) {
                return PrepareResult::PREPARE_SYNTAX_ERROR;
            }
        }
        return PrepareResult::PREPARE_SYNTAX_ERROR;
    }

    return PrepareResult::PREPARE_UNRECOGNIZED_STATEMENT;
}
