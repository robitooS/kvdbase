#include "pager.hpp"

Pager::Pager(std::string filename)
{
    // We need to verificate if the file exists
    file.open(filename, std::ios::out | std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

        Header header = {-1, 0, 0, 1, 1};
        writeHeader(header);
    }

    // If exists, just open the file

}

Pager::~Pager()
{
    std::cout << "Finalizando objeto";
    if (file.is_open())
        file.close();
}

Page Pager::get(const uint id)
{
    Page p;
    auto offset = sizeof(Header) + (id - 1) * sizeof(Page);

    file.seekg(offset);
    file.read(reinterpret_cast<char*>(&p), sizeof(Page));

    return p;
}

void Pager::update(const Page &page)
{
    auto offset = sizeof(Header) + (page.id - 1) * sizeof(Page);

    file.seekp(offset);
    file.write(reinterpret_cast<const char*>(&page), sizeof(Page));

    file.flush();
}

uint Pager::allocatePage()
{
    auto header = readHeader();

    if (header.freePageId != 0) {
        auto id = header.freePageId;
        auto page = get(id);

        header.freePageId = page.nextFreeId;
        writeHeader(header);

        return id;
    }

    auto id = header.nextPageId;
    header.nextPageId++;
    writeHeader(header);
    return id;
};

uint Pager::getHeaderFreeId()
{   
    Header header = readHeader();
    return header.freePageId;
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

    file.flush();
}
