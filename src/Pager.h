#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>

constexpr uint32_t PAGE_SIZE = 4096;
constexpr uint32_t MAX_PAGES = 100;

class Pager {
public:
    Pager(const std::string& filename);
    ~Pager();

    void* get_page(uint32_t page_num);
    void flush_page(uint32_t page_num);
    uint32_t get_file_length() const;
    uint32_t get_num_pages() const;
    void set_num_pages(uint32_t num_pages);

private:
    std::string filename;
    std::fstream file;
    uint32_t file_length;
    uint32_t num_pages;
    void* pages[MAX_PAGES];
};
