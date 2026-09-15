#pragma once
#include "Pager.h"
#include <string>

constexpr uint32_t COLUMN_USERNAME_SIZE = 32;
constexpr uint32_t COLUMN_EMAIL_SIZE = 255;

// Pack the Row struct to eliminate compiler-dependent padding,
// ensuring a portable and deterministic on-disk format.
#pragma pack(push, 1)
struct Row {
    int32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
};
#pragma pack(pop)

constexpr uint32_t ROW_SIZE = sizeof(Row);

class BPlusTree; // Forward declaration

class Table {
public:
    Table(const std::string& filename);
    ~Table();

    void serialize_row(const Row& source, void* destination);
    void deserialize_row(void* source, Row& destination);

    Pager* get_pager() { return pager; }
    BPlusTree* get_tree() { return tree; }

private:
    Pager* pager;
    BPlusTree* tree;
};
