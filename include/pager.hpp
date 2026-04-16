#pragma once

#define DATA_SIZE 4088
#define NUM_MAX_PAGES 100  // max number of pages in RAM

#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>

using uint = unsigned int;

// The max size of the page should be 4096 bytes
// 2 * 4(uint) + 4088 = 4096
typedef struct {
    uint id;
    uint nextFreeId;
    char data[DATA_SIZE];
} Page;

typedef struct {
    int root;
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
    uint allocatePage(); 
    void deallocatePage(const uint id);
    uint getHeaderFreeId();
    int getRootId();
    void writeRootId(const int id);

private:
    std::fstream file;
    Header readHeader();
    void writeHeader(Header &header);

};
