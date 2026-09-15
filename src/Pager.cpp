#include "Pager.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

Pager::Pager(const std::string& filename) : filename(filename), file_length(0), num_pages(0) {
    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) {
        // File doesn't exist, create it
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    }
    
    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        file_length = file.tellg();
        num_pages = file_length / PAGE_SIZE;
        if (file_length % PAGE_SIZE != 0) {
            std::cerr << "Corrupt file: length not a multiple of page size." << std::endl;
        }
    } else {
        std::cerr << "Unable to open file: " << filename << std::endl;
    }
    
    for (uint32_t i = 0; i < MAX_PAGES; ++i) {
        pages[i] = nullptr;
    }
}

Pager::~Pager() {
    for (uint32_t i = 0; i < MAX_PAGES; ++i) {
        if (pages[i] != nullptr) {
            flush_page(i);
            free(pages[i]);
            pages[i] = nullptr;
        }
    }
    if (file.is_open()) {
        file.close();
    }
}

void* Pager::get_page(uint32_t page_num) {
    if (page_num >= MAX_PAGES) {
        std::cerr << "Page number out of bounds: " << page_num << std::endl;
        return nullptr;
    }

    if (pages[page_num] == nullptr) {
        // Allocate memory and load from file
        void* page = malloc(PAGE_SIZE);
        memset(page, 0, PAGE_SIZE);

        uint32_t current_file_pages = file_length / PAGE_SIZE;
        if (file_length % PAGE_SIZE) {
            current_file_pages += 1;
        }

        if (page_num < current_file_pages) {
            file.seekg(page_num * PAGE_SIZE, std::ios::beg);
            file.read(reinterpret_cast<char*>(page), PAGE_SIZE);
        }
        pages[page_num] = page;
        
        if (page_num >= this->num_pages) {
            this->num_pages = page_num + 1;
        }
    }
    return pages[page_num];
}

void Pager::flush_page(uint32_t page_num) {
    if (pages[page_num] == nullptr) {
        std::cerr << "Tried to flush null page: " << page_num << std::endl;
        return;
    }

    file.seekp(page_num * PAGE_SIZE, std::ios::beg);
    file.write(reinterpret_cast<char*>(pages[page_num]), PAGE_SIZE);
    file.flush();
}

uint32_t Pager::get_file_length() const {
    return file_length;
}

uint32_t Pager::get_num_pages() const {
    return num_pages;
}

void Pager::set_num_pages(uint32_t num_pages) {
    this->num_pages = num_pages;
}
