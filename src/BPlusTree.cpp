#include "BPlusTree.h"
#include <iostream>
#include <cstring>



Cursor::Cursor(Table* table, uint32_t page_num, uint32_t cell_num)
    : table(table), page_num(page_num), cell_num(cell_num), end_of_table(false) {}

void Cursor::advance() {
    void* node = table->get_pager()->get_page(page_num);
    cell_num += 1;
    if (cell_num >= *(table->get_tree()->leaf_node_num_cells(node))) {
        // Advance to the next leaf in the linked list
        uint32_t next_page_num = *(table->get_tree()->leaf_node_next_leaf(node));
        if (next_page_num == 0) {
            // Page 0 is always the root, so a next_leaf of 0 means "no next leaf"
            end_of_table = true;
        } else {
            page_num = next_page_num;
            cell_num = 0;
        }
    }
}

void* Cursor::value() {
    void* page = table->get_pager()->get_page(page_num);
    return table->get_tree()->leaf_node_value(page, cell_num);
}



BPlusTree::BPlusTree(Pager* pager, Table* table)
    : pager(pager), table(table), root_page_num(0) {
    if (pager->get_num_pages() == 0) {
        // Brand new database file — initialize root as an empty leaf node
        void* root_node = pager->get_page(0);
        initialize_leaf_node(root_node);
        set_node_root(root_node, true);
    }
}



NodeType BPlusTree::get_node_type(void* node) {
    uint8_t value = *((uint8_t*)((char*)node + NODE_TYPE_OFFSET));
    return static_cast<NodeType>(value);
}

void BPlusTree::set_node_type(void* node, NodeType type) {
    *((uint8_t*)((char*)node + NODE_TYPE_OFFSET)) = static_cast<uint8_t>(type);
}

bool BPlusTree::is_node_root(void* node) {
    return static_cast<bool>(*((uint8_t*)((char*)node + IS_ROOT_OFFSET)));
}

void BPlusTree::set_node_root(void* node, bool is_root) {
    *((uint8_t*)((char*)node + IS_ROOT_OFFSET)) = static_cast<uint8_t>(is_root);
}

uint32_t* BPlusTree::node_parent(void* node) {
    return (uint32_t*)((char*)node + PARENT_POINTER_OFFSET);
}

// =============================================================================
// Leaf Node — Accessors & Initialization
// =============================================================================

uint32_t* BPlusTree::leaf_node_num_cells(void* node) {
    return (uint32_t*)((char*)node + LEAF_NODE_NUM_CELLS_OFFSET);
}

void* BPlusTree::leaf_node_cell(void* node, uint32_t cell_num) {
    return (char*)node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t* BPlusTree::leaf_node_key(void* node, uint32_t cell_num) {
    return (uint32_t*)leaf_node_cell(node, cell_num);
}

void* BPlusTree::leaf_node_value(void* node, uint32_t cell_num) {
    return (char*)leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

uint32_t* BPlusTree::leaf_node_next_leaf(void* node) {
    return (uint32_t*)((char*)node + LEAF_NODE_NEXT_LEAF_OFFSET);
}

void BPlusTree::initialize_leaf_node(void* node) {
    set_node_type(node, NodeType::NODE_LEAF);
    set_node_root(node, false);
    *leaf_node_num_cells(node) = 0;
    *leaf_node_next_leaf(node) = 0; // 0 = no sibling
}

// =============================================================================
// Internal Node — Accessors & Initialization
// =============================================================================

uint32_t* BPlusTree::internal_node_num_keys(void* node) {
    return (uint32_t*)((char*)node + INTERNAL_NODE_NUM_KEYS_OFFSET);
}

uint32_t* BPlusTree::internal_node_right_child(void* node) {
    return (uint32_t*)((char*)node + INTERNAL_NODE_RIGHT_CHILD_OFFSET);
}

uint32_t* BPlusTree::internal_node_cell(void* node, uint32_t cell_num) {
    return (uint32_t*)((char*)node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE);
}

uint32_t* BPlusTree::internal_node_child(void* node, uint32_t child_num) {
    uint32_t num_keys = *internal_node_num_keys(node);
    if (child_num > num_keys) {
        std::cerr << "Error: Tried to access child_num " << child_num
                  << " > num_keys " << num_keys << std::endl;
        return nullptr;
    } else if (child_num == num_keys) {
        return internal_node_right_child(node);
    } else {
        return internal_node_cell(node, child_num);
    }
}

uint32_t* BPlusTree::internal_node_key(void* node, uint32_t key_num) {
    return (uint32_t*)((char*)internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE);
}

void BPlusTree::initialize_internal_node(void* node) {
    set_node_type(node, NodeType::NODE_INTERNAL);
    set_node_root(node, false);
    *internal_node_num_keys(node) = 0;
    *internal_node_right_child(node) = 0;
}

// =============================================================================
// Utility
// =============================================================================

uint32_t BPlusTree::get_node_max_key(void* node) {
    switch (get_node_type(node)) {
        case NodeType::NODE_LEAF: {
            uint32_t num_cells = *leaf_node_num_cells(node);
            if (num_cells == 0) return 0; // Guard against underflow on empty leaf
            return *leaf_node_key(node, num_cells - 1);
        }
        case NodeType::NODE_INTERNAL: {
            // The true max key of this subtree lives in the rightmost descendant.
            // Recurse into the right child to find it.
            uint32_t right_child_page = *internal_node_right_child(node);
            void* right_child = pager->get_page(right_child_page);
            return get_node_max_key(right_child);
        }
    }
    return 0;
}

uint32_t BPlusTree::get_unused_page_num() {
    // The next unused page is always one past the current highest page.
    // When pager->get_page() is called with this number, the pager
    // allocates a fresh zeroed page and increments its internal count.
    return pager->get_num_pages();
}

// =============================================================================
// Find — Binary search through the B+ tree
// =============================================================================

Cursor* BPlusTree::leaf_node_find(uint32_t page_num, uint32_t key) {
    void* node = pager->get_page(page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    // Cursor always gets a valid Table* from the tree's stored back-pointer
    Cursor* cursor = new Cursor(this->table, page_num, 0);

    // Binary search for the key (or the position where it would be inserted)
    uint32_t min_index = 0;
    uint32_t one_past_max_index = num_cells;
    while (one_past_max_index != min_index) {
        uint32_t index = (min_index + one_past_max_index) / 2;
        uint32_t key_at_index = *leaf_node_key(node, index);
        if (key == key_at_index) {
            cursor->cell_num = index;
            return cursor;
        }
        if (key < key_at_index) {
            one_past_max_index = index;
        } else {
            min_index = index + 1;
        }
    }

    cursor->cell_num = min_index;
    return cursor;
}

Cursor* BPlusTree::internal_node_find(uint32_t page_num, uint32_t key) {
    void* node = pager->get_page(page_num);

    // Binary search for the child that contains the key
    uint32_t num_keys = *internal_node_num_keys(node);
    uint32_t min_index = 0;
    uint32_t max_index = num_keys;

    while (min_index != max_index) {
        uint32_t index = (min_index + max_index) / 2;
        uint32_t key_to_right = *internal_node_key(node, index);
        if (key_to_right >= key) {
            max_index = index;
        } else {
            min_index = index + 1;
        }
    }

    uint32_t child_page = *internal_node_child(node, min_index);
    void* child = pager->get_page(child_page);
    switch (get_node_type(child)) {
        case NodeType::NODE_LEAF:
            return leaf_node_find(child_page, key);
        case NodeType::NODE_INTERNAL:
            return internal_node_find(child_page, key);
    }
    return nullptr; // unreachable
}

Cursor* BPlusTree::find(uint32_t key) {
    void* root_node = pager->get_page(root_page_num);
    if (get_node_type(root_node) == NodeType::NODE_LEAF) {
        return leaf_node_find(root_page_num, key);
    } else {
        return internal_node_find(root_page_num, key);
    }
}

Cursor* BPlusTree::start() {
    // Find the leftmost leaf by searching for key 0
    Cursor* cursor = find(0);
    void* node = pager->get_page(cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    cursor->end_of_table = (num_cells == 0);
    return cursor;
}

// =============================================================================
// Insert — with duplicate detection, splitting, and root creation
// =============================================================================

bool BPlusTree::insert(uint32_t key, Row* row) {
    Cursor* cursor = find(key);
    void* node = pager->get_page(cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    // Check for duplicate key at the found position
    if (cursor->cell_num < num_cells) {
        uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);
        if (key_at_index == key) {
            delete cursor;
            return false; // duplicate key
        }
    }

    if (num_cells < LEAF_NODE_MAX_CELLS) {
        leaf_node_insert(cursor, key, row);
    } else {
        // Leaf is full — split it, then insert into the appropriate half
        leaf_node_split_and_insert(cursor, key, row);
    }
    delete cursor;
    return true;
}

void BPlusTree::leaf_node_insert(Cursor* cursor, uint32_t key, Row* row) {
    void* node = pager->get_page(cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    if (cursor->cell_num < num_cells) {
        // Shift cells right to make room for the new cell
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cursor->cell_num)) = key;
    memcpy(leaf_node_value(node, cursor->cell_num), row, ROW_SIZE);
}

void BPlusTree::leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* row) {
    void* old_node = pager->get_page(cursor->page_num);
    uint32_t old_max = get_node_max_key(old_node);

    // Allocate a new page for the right (sibling) leaf
    uint32_t new_page_num = get_unused_page_num();
    void* new_node = pager->get_page(new_page_num);
    initialize_leaf_node(new_node);

    // Maintain the leaf linked list: old → new → old's-former-next
    *leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
    *leaf_node_next_leaf(old_node) = new_page_num;

    // Redistribute all LEAF_NODE_MAX_CELLS + 1 cells (existing + new one)
    // between old_node (left) and new_node (right).
    //
    // Iterating backwards avoids overwriting source data:
    //   - For i > cursor->cell_num: source is old_node[i-1] (shifted by 1 for insertion)
    //   - For i == cursor->cell_num: this is the new cell being inserted
    //   - For i < cursor->cell_num: source is old_node[i] (unchanged position)
    //
    // Destination: left node for i < LEFT_SPLIT_COUNT, right node otherwise.
    // Index within destination node = i % LEFT_SPLIT_COUNT.
    for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
        void* destination_node;
        if (i >= static_cast<int32_t>(LEAF_NODE_LEFT_SPLIT_COUNT)) {
            destination_node = new_node;
        } else {
            destination_node = old_node;
        }
        uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
        void* destination = leaf_node_cell(destination_node, index_within_node);

        if (static_cast<uint32_t>(i) == cursor->cell_num) {
            // Write the new key + value at the insertion point
            *leaf_node_key(destination_node, index_within_node) = key;
            memcpy(leaf_node_value(destination_node, index_within_node), row, ROW_SIZE);
        } else if (static_cast<uint32_t>(i) > cursor->cell_num) {
            memcpy(destination, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
        } else {
            memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
        }
    }

    // Update cell counts
    *leaf_node_num_cells(old_node) = LEAF_NODE_LEFT_SPLIT_COUNT;
    *leaf_node_num_cells(new_node) = LEAF_NODE_RIGHT_SPLIT_COUNT;

    if (is_node_root(old_node)) {
        // The root was split — create a new internal root above it
        create_new_root(new_page_num);
    } else {
        // Non-root leaf split — update the parent internal node
        uint32_t parent_page = *node_parent(old_node);
        uint32_t new_max = get_node_max_key(old_node);

        // Set the new leaf's parent pointer
        *node_parent(new_node) = parent_page;

        // Update the separator key in the parent for the old child
        void* parent = pager->get_page(parent_page);
        update_internal_node_key(parent, old_max, new_max);

        // Add the new leaf as a child of the parent
        internal_node_insert(parent_page, new_page_num);
    }
}

// =============================================================================
// Tree Restructuring
// =============================================================================

void BPlusTree::create_new_root(uint32_t right_child_page_num) {
    // The old root (page root_page_num) is being split.
    // We copy the old root's contents to a new "left child" page,
    // then re-initialize the root page as an internal node pointing
    // to the left and right children.

    void* root = pager->get_page(root_page_num);
    void* right_child = pager->get_page(right_child_page_num);

    uint32_t left_child_page_num = get_unused_page_num();
    void* left_child = pager->get_page(left_child_page_num);

    // Copy old root → left child (preserves all leaf cell data)
    memcpy(left_child, root, PAGE_SIZE);
    set_node_root(left_child, false);

    // Re-initialize the root page as an internal node
    initialize_internal_node(root);
    set_node_root(root, true);
    *internal_node_num_keys(root) = 1;

    // Cell 0: points to left child, key = max key of left child
    *internal_node_cell(root, 0) = left_child_page_num;
    *internal_node_key(root, 0) = get_node_max_key(left_child);

    // Right child: holds all keys greater than the left child's max
    *internal_node_right_child(root) = right_child_page_num;

    // Set parent pointers for both children
    *node_parent(left_child) = root_page_num;
    *node_parent(right_child) = root_page_num;
}

void BPlusTree::internal_node_insert(uint32_t parent_page_num, uint32_t child_page_num) {
    // Insert a new child page into the parent internal node.
    // The child's max key determines its position among existing children.

    void* parent = pager->get_page(parent_page_num);
    void* child = pager->get_page(child_page_num);
    uint32_t child_max_key = get_node_max_key(child);
    uint32_t num_keys = *internal_node_num_keys(parent);

    if (num_keys >= INTERNAL_NODE_MAX_CELLS) {
        std::cerr << "Error: Internal node full (max " << INTERNAL_NODE_MAX_CELLS
                  << " children). Cannot insert." << std::endl;
        return;
    }

    uint32_t right_child_page = *internal_node_right_child(parent);
    void* right_child = pager->get_page(right_child_page);

    if (child_max_key > get_node_max_key(right_child)) {
        // New child has the highest key — it becomes the right child.
        // Move the old right child into the cells array.
        //
        // Note: we write to internal_node_cell directly (not internal_node_child)
        // because internal_node_child(parent, num_keys) would resolve to the
        // right_child pointer, not the cells array.
        *internal_node_cell(parent, num_keys) = right_child_page;
        *internal_node_key(parent, num_keys) = get_node_max_key(right_child);
        *internal_node_right_child(parent) = child_page_num;
    } else {
        // Find the correct sorted position among existing cells
        uint32_t index = num_keys; // default: insert at the end
        for (uint32_t i = 0; i < num_keys; i++) {
            if (child_max_key < *internal_node_key(parent, i)) {
                index = i;
                break;
            }
        }

        // Shift cells right to make room at `index`
        for (uint32_t i = num_keys; i > index; i--) {
            memcpy(internal_node_cell(parent, i),
                   internal_node_cell(parent, i - 1),
                   INTERNAL_NODE_CELL_SIZE);
        }

        // Write the new child at position `index`
        *internal_node_cell(parent, index) = child_page_num;
        *internal_node_key(parent, index) = child_max_key;
    }

    *internal_node_num_keys(parent) = num_keys + 1;
}

void BPlusTree::update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key) {
    // After a leaf split, the parent's separator key for the old child
    // may be stale. Find and update it.
    //
    // If old_key is not found (e.g., the split node was the right child
    // which has no separator key), this is a safe no-op.
    uint32_t num_keys = *internal_node_num_keys(node);
    for (uint32_t i = 0; i < num_keys; i++) {
        if (*internal_node_key(node, i) == old_key) {
            *internal_node_key(node, i) = new_key;
            return;
        }
    }
}
