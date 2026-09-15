#pragma once
#include "Table.h"
#include <string>

enum class StatementType { STATEMENT_INSERT, STATEMENT_SELECT, STATEMENT_DELETE, STATEMENT_UPDATE };

struct Statement {
    StatementType type;
    Row row_to_insert; // For INSERT
    int32_t target_id; // For SELECT/DELETE specific ID, -1 means all
};

enum class PrepareResult {
    PREPARE_SUCCESS,
    PREPARE_SYNTAX_ERROR,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_STRING_TOO_LONG,
    PREPARE_NEGATIVE_ID
};

class Parser {
public:
    static PrepareResult prepare_statement(const std::string& input, Statement& statement);
};
