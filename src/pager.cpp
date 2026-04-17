#include "pager.hpp"
#include <cstring>

Pager::Pager(std::string filename)
{
    file.open(filename, std::ios::out | std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
        // root = -1, numPages = 0, height = 0, freePageId = 0, nextPageId = 1
        Header header = {-1, 0, 0, 0, 1};
        writeHeader(header);
    }
}

Pager::~Pager()
{
    if (file.is_open())
        file.close();
}

Page Pager::get(const uint id)
{
    file.clear();
    Page p;
    std::memset(&p, 0, sizeof(Page));
    auto offset = sizeof(Header) + (id - 1) * sizeof(Page);
    file.seekg(offset);
    file.read(reinterpret_cast<char*>(&p), sizeof(Page));
    return p;
}

void Pager::update(const Page &page)
{
    file.clear();
    auto offset = sizeof(Header) + (page.id - 1) * sizeof(Page);
    file.seekp(offset);
    file.write(reinterpret_cast<const char*>(&page), sizeof(Page));
}

uint Pager::allocatePage()
{
    auto header = readHeader();
    uint id;
    if (header.freePageId != 0) {
        id = header.freePageId;
        Page p = get(id);
        header.freePageId = p.nextFreeId;
    } else {
        id = header.nextPageId++;
    }
    writeHeader(header);
    
    Page p;
    std::memset(&p, 0, sizeof(Page));
    p.id = id;
    update(p);
    return id;
}

void Pager::deallocatePage(const uint id)
{
    auto header = readHeader();
    Page p = get(id);
    p.nextFreeId = header.freePageId;
    header.freePageId = id;
    update(p);
    writeHeader(header);
}

uint Pager::getHeaderFreeId() { return readHeader().freePageId; }
int Pager::getRootId() { return readHeader().root; }

void Pager::writeRootId(const int id)
{
    auto header = readHeader();
    header.root = id;
    writeHeader(header);
}

Header Pager::readHeader()
{
    Header h;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&h), sizeof(Header));
    return h;
}

void Pager::writeHeader(Header &header)
{
    file.seekp(0, std::ios::beg);
    file.write(reinterpret_cast<char*>(&header), sizeof(Header));
}
