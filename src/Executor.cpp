#include "Executor.h"
#include "BPlusTree.h"
#include <iostream>
#include <cstring>

ExecuteResult Executor::execute_statement(Statement& statement, Table* table) {
    switch (statement.type) {
        case StatementType::STATEMENT_INSERT:
            return execute_insert(statement, table);
        case StatementType::STATEMENT_SELECT:
            return execute_select(statement, table);
        case StatementType::STATEMENT_DELETE:
            return execute_delete(statement, table);
    }
    return ExecuteResult::EXECUTE_SUCCESS;
}

ExecuteResult Executor::execute_insert(Statement& statement, Table* table) {
    // insert() handles duplicate detection and leaf splitting internally.
    // No need to manually check the root page or find the cursor externally.
    Row* row_to_insert = &(statement.row_to_insert);
    uint32_t key_to_insert = row_to_insert->id;

    bool success = table->get_tree()->insert(key_to_insert, row_to_insert);
    if (!success) {
        return ExecuteResult::EXECUTE_DUPLICATE_KEY;
    }
    return ExecuteResult::EXECUTE_SUCCESS;
}

ExecuteResult Executor::execute_select(Statement& statement, Table* table) {
    if (statement.target_id == -1) {
        // SELECT all — scan from the leftmost leaf to the end
        Cursor* cursor = table->get_tree()->start();

        while (!cursor->end_of_table) {
            Row row;
            table->deserialize_row(cursor->value(), row);
            std::cout << "(" << row.id << ", " << row.username << ", " << row.email << ")" << std::endl;
            cursor->advance();
        }
        delete cursor;
    } else {
        // SELECT by specific ID — point lookup via B+ tree search
        uint32_t key = static_cast<uint32_t>(statement.target_id);
        Cursor* cursor = table->get_tree()->find(key);
        void* node = table->get_pager()->get_page(cursor->page_num);
        uint32_t num_cells = *(table->get_tree()->leaf_node_num_cells(node));

        if (cursor->cell_num < num_cells) {
            uint32_t key_at_index = *table->get_tree()->leaf_node_key(node, cursor->cell_num);
            if (key_at_index == key) {
                Row row;
                table->deserialize_row(cursor->value(), row);
                std::cout << "(" << row.id << ", " << row.username << ", " << row.email << ")" << std::endl;
            } else {
                std::cout << "Record not found." << std::endl;
            }
        } else {
            std::cout << "Record not found." << std::endl;
        }
        delete cursor;
    }
    return ExecuteResult::EXECUTE_SUCCESS;
}

ExecuteResult Executor::execute_delete(Statement& statement, Table* table) {
    // DELETE by specific ID — find the cell, then shift subsequent cells left.
    // No tree rebalancing is performed (acceptable for a lightweight engine).
    uint32_t key = static_cast<uint32_t>(statement.target_id);
    Cursor* cursor = table->get_tree()->find(key);
    void* node = table->get_pager()->get_page(cursor->page_num);
    uint32_t num_cells = *(table->get_tree()->leaf_node_num_cells(node));

    if (cursor->cell_num < num_cells) {
        uint32_t key_at_index = *table->get_tree()->leaf_node_key(node, cursor->cell_num);
        if (key_at_index == key) {
            // Found the record — shift all subsequent cells left to fill the gap
            for (uint32_t i = cursor->cell_num; i < num_cells - 1; i++) {
                memcpy(table->get_tree()->leaf_node_cell(node, i),
                       table->get_tree()->leaf_node_cell(node, i + 1),
                       LEAF_NODE_CELL_SIZE);
            }
            *(table->get_tree()->leaf_node_num_cells(node)) -= 1;
            std::cout << "Record deleted." << std::endl;
        } else {
            std::cout << "Record not found." << std::endl;
        }
    } else {
        std::cout << "Record not found." << std::endl;
    }
    delete cursor;
    return ExecuteResult::EXECUTE_SUCCESS;
}
