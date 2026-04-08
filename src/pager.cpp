#include "pager.hpp"

Pager::Pager(std::string filename)
{
    file.open(filename, std::ios::out | std::ios::in);
    Header header = {-1, 0, 0, 0};

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
    file.write(reinterpret_cast<const char*>(&page), sizeof(Page));
    file.flush()
}

void Pager::newPage(const Page &page)
{
    Header header;
    file.read(reinterpret_cast<char*>(&header), sizeof(Header));
    header.numPages += 1;

    

}
