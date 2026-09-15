#pragma once
#include "Pager.h"
#include "Table.h"
#include <cstdint>

enum class NodeType : uint8_t { NODE_INTERNAL, NODE_LEAF };

// --- Node Header Layout ---
// Every node (leaf or internal) starts with a common header:
//   [node_type: 1 byte] [is_root: 1 byte] [parent_pointer: 4 bytes]
inline constexpr uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
inline constexpr uint32_t NODE_TYPE_OFFSET = 0;
inline constexpr uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
inline constexpr uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
inline constexpr uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
inline constexpr uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
inline constexpr uint32_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

// --- Leaf Node Layout ---
// After the common header:
//   [num_cells: 4 bytes] [next_leaf_page: 4 bytes]
// Followed by cells:
//   Each cell = [key: 4 bytes] [value: ROW_SIZE bytes]
inline constexpr uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
inline constexpr uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
inline constexpr uint32_t LEAF_NODE_NEXT_LEAF_SIZE = sizeof(uint32_t);
inline constexpr uint32_t LEAF_NODE_NEXT_LEAF_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE;
inline constexpr uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_NEXT_LEAF_SIZE;

inline constexpr uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
inline constexpr uint32_t LEAF_NODE_KEY_OFFSET = 0;
inline constexpr uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
inline constexpr uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
inline constexpr uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
inline constexpr uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
inline constexpr uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

inline constexpr uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
inline constexpr uint32_t LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;

// --- Internal Node Layout ---
// After the common header:
//   [num_keys: 4 bytes] [right_child_page: 4 bytes]
// Followed by cells:
//   Each cell = [child_page: 4 bytes] [key: 4 bytes]
// The right_child_page holds all keys greater than the last key.
inline constexpr uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
inline constexpr uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
inline constexpr uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
inline constexpr uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET = INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
inline constexpr uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE;

inline constexpr uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
inline constexpr uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
inline constexpr uint32_t INTERNAL_NODE_CELL_SIZE = INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;
inline constexpr uint32_t INTERNAL_NODE_MAX_CELLS = 300;

// Cursor represents a position within the B+ tree (a specific cell on a specific leaf page).
class Cursor {
public:
    Table* table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table;

    Cursor(Table* table, uint32_t page_num, uint32_t cell_num);
    void advance();
    void* value();
};

class BPlusTree {
public:
    BPlusTree(Pager* pager, Table* table);

    Cursor* find(uint32_t key);
    Cursor* start();

    // Returns false if a duplicate key already exists.
    bool insert(uint32_t key, Row* row);

    // --- Public accessors (used by Executor for direct node manipulation) ---
    uint32_t get_node_max_key(void* node);
    bool is_node_root(void* node);
    void set_node_root(void* node, bool is_root);
    NodeType get_node_type(void* node);
    void set_node_type(void* node, NodeType type);

    uint32_t* leaf_node_num_cells(void* node);
    void* leaf_node_cell(void* node, uint32_t cell_num);
    uint32_t* leaf_node_key(void* node, uint32_t cell_num);
    void* leaf_node_value(void* node, uint32_t cell_num);
    uint32_t* leaf_node_next_leaf(void* node);

    uint32_t* internal_node_num_keys(void* node);
    uint32_t* internal_node_right_child(void* node);
    uint32_t* internal_node_cell(void* node, uint32_t cell_num);
    uint32_t* internal_node_child(void* node, uint32_t child_num);
    uint32_t* internal_node_key(void* node, uint32_t key_num);
    uint32_t* node_parent(void* node);

private:
    Pager* pager;
    Table* table;
    uint32_t root_page_num;

    void initialize_leaf_node(void* node);
    void initialize_internal_node(void* node);
    Cursor* leaf_node_find(uint32_t page_num, uint32_t key);
    Cursor* internal_node_find(uint32_t page_num, uint32_t key);

    void leaf_node_insert(Cursor* cursor, uint32_t key, Row* row);
    void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* row);
    void create_new_root(uint32_t right_child_page_num);
    void internal_node_insert(uint32_t parent_page_num, uint32_t child_page_num);
    void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key);

    uint32_t get_unused_page_num();
};
