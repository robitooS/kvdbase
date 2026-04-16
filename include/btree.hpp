#pragma once
#define ORDER 509

#include "pager.hpp"
#include <string>
#include <optional>
#include <stack>

using uint = unsigned int;

// page->data have 4088 max size
typedef struct {
    bool is_leaf;
    int numKeys;
    int keys[ORDER];
    uint children_id[ORDER + 1];
    
} InternalNode;

typedef struct {
    bool is_leaf;
    int numKeys;
    int values[ORDER]; // negative numbers
    int keys[ORDER];
    uint nextLeaf;
    
} LeafNode;


class BplusTree
{
    public:
        BplusTree(Pager& pager); // -> Constructor, might we need to pass the Pager as a parameter

        void insert(const int key, const int value);
        std::optional<int> search(const int key);
        int remove(const int key); // return 1 if success or 0 if false
    
    private:
        Pager& m_pager;
        uint rootId;
};