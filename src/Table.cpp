#include "Table.h"
#include "BPlusTree.h"
#include <cstring>

Table::Table(const std::string& filename) {
    pager = new Pager(filename);
    tree = new BPlusTree(pager, this);
}

Table::~Table() {
    delete tree;
    delete pager;
}

void Table::serialize_row(const Row& source, void* destination) {
    memcpy(destination, &source, ROW_SIZE);
}

void Table::deserialize_row(void* source, Row& destination) {
    memcpy(&destination, source, ROW_SIZE);
}
