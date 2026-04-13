#include "btree.hpp"

BplusTree::BplusTree(Pager &pager) : m_pager(pager)
{
    auto root = m_pager.getRootId();

    if (root == -1) {
        auto id = m_pager.allocatePage();
        m_pager.writeRootId(id);

        Page page;
        page.id = id;

        LeafNode* node = reinterpret_cast<LeafNode*>(page.data);
        node->is_leaf = true;
        node->nextLeaf = 0;

        m_pager.update(page); // save in disk
        this->rootId = id;
    } else {
        this->rootId = root;
    }

}

void BplusTree::insert(const int key, const int value)
{
}

// i think this will work
std::optional<int> BplusTree::search(const int key)
{
    auto curr_id = rootId;

    while(true) {
        Page page = m_pager.get(curr_id);
        bool is_leaf = reinterpret_cast<LeafNode*>(page.data)->is_leaf;

        if (is_leaf) {
            LeafNode* node = reinterpret_cast<LeafNode*>(page.data);
            for (int i = 0; i < node->numKeys; i++) {
                if (node->keys[i] == key) {
                    return node->values[i];
                }
            }
            return std::nullopt;

        } else {
            int i = 0;
            InternalNode* node = reinterpret_cast<InternalNode*>(page.data);

            while(i < node->numKeys && key >= node->keys[i]) {
                i++;
            }

            curr_id = node->children_id[i];
        }
    }
}

int BplusTree::remove(const int key)
{
    return 0;
}
