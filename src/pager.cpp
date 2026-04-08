#include "pager.hpp"

Pager::Pager(std::string filename)
{
    file.open(filename, std::ios::out | std::ios::in);
    Header header = {-1, 0, 0, 1, 1};

    // Writing the header's content
    file.write(reinterpret_cast<char*>(&header), sizeof(header));

}

Pager::~Pager()
{
    std::cout << "Finalizando objeto";
    if (file.is_open())
        file.close();
}

void Pager::update(const Page &page)
{
    file.seekp(0, std::ios::beg);
    auto offset = sizeof(Header) + (page.id - 1) * sizeof(Page);

    file.seekp(offset);
    file.write(reinterpret_cast<const char*>(&page), sizeof(Page));

    file.flush();
}

uint Pager::allocatePage()
{
    Header header = readHeader();

    if (header.freePageId != 0) {
        auto id = header.freePageId;

        header.freePageId = 0;
        return id;
    } 

    header.nextPageId++;
    return header.nextPageId;

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
