#pragma once

#include "btree.hpp"
#include "pager.hpp"
#include <string>
#include <optional>

class KVdbase {
    public:
        KVdbase(const std::string& filename);

        std::optional<int> get(int key);
        void put(int key, int value);
        bool remove(int key);

    private:
        Pager m_pager;
        BplusTree m_tree;
};