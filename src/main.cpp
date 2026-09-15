#include "Table.h"
#include "Parser.h"
#include "Executor.h"
#include <iostream>
#include <string>

void print_prompt() {
    std::cout << "db > ";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Must supply a database filename.\n";
        exit(EXIT_FAILURE);
    }
    
    std::string filename = argv[1];
    Table* table = new Table(filename);

    while (true) {
        print_prompt();
        std::string input_line;
        if (!std::getline(std::cin, input_line)) {
            break; // EOF
        }

        if (input_line.empty()) continue;

        if (input_line[0] == '.') {
            if (input_line == ".exit") {
                delete table;
                exit(EXIT_SUCCESS);
            } else {
                std::cout << "Unrecognized command '" << input_line << "'.\n";
                continue;
            }
        }

        Statement statement;
        PrepareResult prepare_result = Parser::prepare_statement(input_line, statement);

        switch (prepare_result) {
            case PrepareResult::PREPARE_SUCCESS:
                break;
            case PrepareResult::PREPARE_SYNTAX_ERROR:
                std::cout << "Syntax error. Could not parse statement.\n";
                continue;
            case PrepareResult::PREPARE_UNRECOGNIZED_STATEMENT:
                std::cout << "Unrecognized keyword at start of '" << input_line << "'.\n";
                continue;
            case PrepareResult::PREPARE_STRING_TOO_LONG:
                std::cout << "String is too long.\n";
                continue;
            case PrepareResult::PREPARE_NEGATIVE_ID:
                std::cout << "ID must be positive.\n";
                continue;
        }

        ExecuteResult execute_result = Executor::execute_statement(statement, table);
        switch (execute_result) {
            case ExecuteResult::EXECUTE_SUCCESS:
                std::cout << "Executed.\n";
                break;
            case ExecuteResult::EXECUTE_DUPLICATE_KEY:
                std::cout << "Error: Duplicate key.\n";
                break;
            case ExecuteResult::EXECUTE_TABLE_FULL:
                std::cout << "Error: Table full.\n";
                break;
        }
    }
    
    delete table;
    return 0;
}
