#include "btree.hpp"

BplusTree::BplusTree(Pager &pager) : m_pager(pager)
{
    auto root = m_pager.getRootId();

    if (root == -1)
    {
        auto id = m_pager.allocatePage();
        m_pager.writeRootId(id);

        Page page;
        page.id = id;

        LeafNode *node = reinterpret_cast<LeafNode *>(page.data);
        node->is_leaf = true;
        node->nextLeaf = 0;

        m_pager.update(page); // save in disk
        this->rootId = id;
    }
    else
    {
        this->rootId = root;
    }
}

void BplusTree::insert(const int key, const int value)
{
    auto curr_id = rootId;
    std::stack<uint> parents;

    while (true)
    {
        Page page = m_pager.get(curr_id);
        bool is_leaf = reinterpret_cast<LeafNode *>(page.data)->is_leaf;

        if (is_leaf)
        {
            LeafNode *node = reinterpret_cast<LeafNode *>(page.data);

            if (node->numKeys == ORDER)
            {
                auto id_newleaf = m_pager.allocatePage();

                Page new_page;
                new_page.id = id_newleaf;

                LeafNode *new_node = reinterpret_cast<LeafNode *>(new_page.data);
                new_node->is_leaf = true;
                new_node->numKeys = 0;

                int mid = (node->numKeys) / 2;

                // Copia os dados
                for (int j = 0; j < ORDER - mid; j++)
                {
                    new_node->keys[j] = node->keys[mid + j];
                    new_node->values[j] = node->values[mid + j];
                }

                new_node->numKeys = ORDER - mid;
                new_node->nextLeaf = node->nextLeaf;
                node->nextLeaf = id_newleaf;
                node->numKeys = mid;

                auto p_key = new_node->keys[0];

                LeafNode *target_node;
                Page target_page;

                if (key >= p_key) {
                    target_node = new_node;
                    target_page = new_page;
                } else {
                    target_node = node;
                    target_page = page;
                }

                int i = 0;
                while (i < target_node->numKeys && target_node->keys[i] < key) {
                    i++;
                }

                for (int j = target_node->numKeys; j > i; j--) {
                    target_node->keys[j] = target_node->keys[j - 1];
                    target_node->values[j] = target_node->values[j - 1];
                }
                target_node->keys[i] = key;
                target_node->values[i] = value;
                target_node->numKeys++;
                m_pager.update(target_page);
                
                break;
            }
        }
        else
        {
            InternalNode *node = reinterpret_cast<InternalNode *>(page.data);

            int i = 0;
            while (i < node->numKeys && key >= node->keys[i])
            {
                i++;
            }

            parents.push(curr_id);
            curr_id = node->children_id[i];
        }
    }
}

// i think this will work
std::optional<int> BplusTree::search(const int key)
{
    auto curr_id = rootId;

    while (true)
    {
        Page page = m_pager.get(curr_id);
        bool is_leaf = reinterpret_cast<LeafNode *>(page.data)->is_leaf;

        if (is_leaf)
        {
            LeafNode *node = reinterpret_cast<LeafNode *>(page.data);
            for (int i = 0; i < node->numKeys; i++)
            {
                if (node->keys[i] == key)
                {
                    return node->values[i];
                }
            }
            return std::nullopt;
        }
        else
        {
            int i = 0;
            InternalNode *node = reinterpret_cast<InternalNode *>(page.data);

            while (i < node->numKeys && key >= node->keys[i])
            {
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
