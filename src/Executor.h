#pragma once
#include "Table.h"
#include "Parser.h"

enum class ExecuteResult {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
    EXECUTE_DUPLICATE_KEY
};

class Executor {
public:
    static ExecuteResult execute_statement(Statement& statement, Table* table);
private:
    static ExecuteResult execute_insert(Statement& statement, Table* table);
    static ExecuteResult execute_select(Statement& statement, Table* table);
    static ExecuteResult execute_delete(Statement& statement, Table* table);
};
