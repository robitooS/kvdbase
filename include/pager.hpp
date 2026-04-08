#pragma once

#define PAGE_NUM_SIZE 4096 // max size of one page
#define NUM_MAX_PAGES 100  // max number of pages in RAM
#define ORDER 512

#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>

using uint = unsigned int;

typedef struct
{
    uint id;
    int keys[ORDER];
    int values[ORDER + 1];
    bool is_leaf;
} Page;

typedef struct {
    uint root;
    uint numPages;
    uint height;
    uint freePageId; // -> The next page added will use this value
    uint nextPageId;
} Header;


class Pager
{
public:
    // Constructor
    // For example:
    // "database.db"
    Pager(std::string filename); // -> To inicializate that, it needs to get the filename param

    // Destructor
    ~Pager();

    Page get(const uint id);
    void update(const Page &page);
    uint allocatePage(); // When a page is added, returns the id number and the page is allocated in memory
    uint getHeaderFreeId();

private:
    std::fstream file;
    Header readHeader();
    void writeHeader(Header &header);

};
