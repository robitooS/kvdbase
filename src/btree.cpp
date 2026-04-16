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
        node->numKeys = 0;
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

            if (node->numKeys < ORDER)
            {
                int i = 0;
                while (i < node->numKeys && node->keys[i] < key) i++;

                if (i < node->numKeys && node->keys[i] == key) {
                    node->values[i] = value;
                } else {
                    for (int j = node->numKeys; j > i; j--) {
                        node->keys[j] = node->keys[j - 1];
                        node->values[j] = node->values[j - 1];
                    }
                    node->keys[i] = key;
                    node->values[i] = value;
                    node->numKeys++;
                }
                m_pager.update(page);
                break;
            }
            else
            {
                uint id_newleaf = m_pager.allocatePage();
                Page new_page;
                new_page.id = id_newleaf;
                LeafNode *new_node = reinterpret_cast<LeafNode *>(new_page.data);

                new_node->is_leaf = true;
                int mid = ORDER / 2;
                new_node->numKeys = ORDER - mid;

                for (int j = 0; j < new_node->numKeys; j++) {
                    new_node->keys[j] = node->keys[mid + j];
                    new_node->values[j] = node->values[mid + j];
                }

                new_node->nextLeaf = node->nextLeaf;
                node->nextLeaf = id_newleaf;
                node->numKeys = mid;

                LeafNode *target = (key >= new_node->keys[0]) ? new_node : node;

                int i = 0;
                while (i < target->numKeys && target->keys[i] < key) i++;
                for (int j = target->numKeys; j > i; j--) {
                    target->keys[j] = target->keys[j - 1];
                    target->values[j] = target->values[j - 1];
                }
                target->keys[i] = key;
                target->values[i] = value;
                target->numKeys++;

                m_pager.update(page);
                m_pager.update(new_page);

                int promoted_key = new_node->keys[0];
                uint new_child_id = id_newleaf;

                while (!parents.empty()) {
                    uint parent_id = parents.top();
                    parents.pop();
                    Page p_page = m_pager.get(parent_id);
                    InternalNode *p_node = reinterpret_cast<InternalNode *>(p_page.data);

                    if (p_node->numKeys < ORDER) {
                        int i = 0;
                        while (i < p_node->numKeys && promoted_key >= p_node->keys[i]) i++;
                        for (int j = p_node->numKeys; j > i; j--) p_node->keys[j] = p_node->keys[j - 1];
                        for (int j = p_node->numKeys + 1; j > i + 1; j--) p_node->children_id[j] = p_node->children_id[j - 1];
                        p_node->keys[i] = promoted_key;
                        p_node->children_id[i + 1] = new_child_id;
                        p_node->numKeys++;
                        m_pager.update(p_page);
                        return;
                    }

                    uint id_newint = m_pager.allocatePage();
                    Page new_p_page;
                    new_p_page.id = id_newint;
                    InternalNode *new_p_node = reinterpret_cast<InternalNode *>(new_p_page.data);
                    new_p_node->is_leaf = false;

                    int t_keys[ORDER + 1];
                    uint t_child[ORDER + 2];
                    int ki = 0;
                    while (ki < ORDER && promoted_key >= p_node->keys[ki]) ki++;
                    for (int j = 0, k = 0; j < ORDER + 1; j++) t_keys[j] = (j == ki) ? promoted_key : p_node->keys[k++];
                    for (int j = 0, k = 0; j < ORDER + 2; j++) t_child[j] = (j == ki + 1) ? new_child_id : p_node->children_id[k++];

                    int mid = (ORDER + 1) / 2;
                    p_node->numKeys = mid;
                    new_p_node->numKeys = ORDER - mid;
                    for (int j = 0; j < p_node->numKeys; j++) p_node->keys[j] = t_keys[j];
                    for (int j = 0; j < p_node->numKeys + 1; j++) p_node->children_id[j] = t_child[j];
                    
                    int next_promoted = t_keys[mid];
                    for (int j = 0; j < new_p_node->numKeys; j++) new_p_node->keys[j] = t_keys[mid + 1 + j];
                    for (int j = 0; j < new_p_node->numKeys + 1; j++) new_p_node->children_id[j] = t_child[mid + 1 + j];

                    m_pager.update(p_page);
                    m_pager.update(new_p_page);
                    promoted_key = next_promoted;
                    new_child_id = id_newint;
                }

                uint id_root = m_pager.allocatePage();
                Page r_page;

                r_page.id = id_root;
                InternalNode *r_node = reinterpret_cast<InternalNode *>(r_page.data);

                r_node->is_leaf = false;
                r_node->numKeys = 1;
                r_node->keys[0] = promoted_key;
                r_node->children_id[0] = rootId;
                r_node->children_id[1] = new_child_id;
                
                m_pager.update(r_page);
                m_pager.writeRootId(id_root);
                rootId = id_root;
                break;
            }
        } else {
            InternalNode *node = reinterpret_cast<InternalNode *>(page.data);

            int i = 0;
            while (i < node->numKeys && key >= node->keys[i]) {
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
    auto curr_id = rootId;
    std::stack<uint> parents;
    std::stack<int> indices;

    while (true) {
        Page page = m_pager.get(curr_id);
        if (reinterpret_cast<LeafNode*>(page.data)->is_leaf) break;
        InternalNode* node = reinterpret_cast<InternalNode*>(page.data);
        int i = 0;
        while (i < node->numKeys && key >= node->keys[i]) i++;
        parents.push(curr_id);
        indices.push(i);
        curr_id = node->children_id[i];
    }

    Page page = m_pager.get(curr_id);
    LeafNode* node = reinterpret_cast<LeafNode*>(page.data);
    int idx = -1;
    for (int i = 0; i < node->numKeys; i++) if (node->keys[i] == key) { idx = i; break; }
    if (idx == -1) return 0;

    for (int i = idx; i < node->numKeys - 1; i++) {
        node->keys[i] = node->keys[i+1];
        node->values[i] = node->values[i+1];
    }
    node->numKeys--;
    m_pager.update(page);

    int minK = (ORDER + 1) / 2;
    while (curr_id != rootId && node->numKeys < minK) {
        uint p_id = parents.top(); parents.pop();
        int idx_p = indices.top(); indices.pop();
        Page p_pg = m_pager.get(p_id);
        InternalNode* p_nd = reinterpret_cast<InternalNode*>(p_pg.data);

        if (idx_p > 0) {
            uint s_id = p_nd->children_id[idx_p - 1];
            Page s_pg = m_pager.get(s_id);
            LeafNode* s_nd = reinterpret_cast<LeafNode*>(s_pg.data);
            if (s_nd->numKeys > minK) {
                if (s_nd->is_leaf) {
                    for (int i = node->numKeys; i > 0; i--) { node->keys[i] = node->keys[i-1]; node->values[i] = node->values[i-1]; }
                    node->keys[0] = s_nd->keys[s_nd->numKeys-1]; node->values[0] = s_nd->values[s_nd->numKeys-1];
                    node->numKeys++; s_nd->numKeys--; p_nd->keys[idx_p-1] = node->keys[0];
                } else {
                    InternalNode *in = (InternalNode*)node, *is = (InternalNode*)s_nd;
                    for (int i = in->numKeys; i > 0; i--) in->keys[i] = in->keys[i-1];
                    for (int i = in->numKeys + 1; i > 0; i--) in->children_id[i] = in->children_id[i-1];
                    in->keys[0] = p_nd->keys[idx_p-1]; in->children_id[0] = is->children_id[is->numKeys];
                    p_nd->keys[idx_p-1] = is->keys[is->numKeys-1]; in->numKeys++; is->numKeys--;
                }
                m_pager.update(page); m_pager.update(s_pg); m_pager.update(p_pg); return 1;
            }
        }
        if (idx_p < p_nd->numKeys) {
            uint s_id = p_nd->children_id[idx_p + 1];
            Page s_pg = m_pager.get(s_id);
            LeafNode* s_nd = reinterpret_cast<LeafNode*>(s_pg.data);
            if (s_nd->numKeys > minK) {
                if (s_nd->is_leaf) {
                    node->keys[node->numKeys] = s_nd->keys[0]; node->values[node->numKeys] = s_nd->values[0];
                    node->numKeys++;
                    for (int i = 0; i < s_nd->numKeys-1; i++) { s_nd->keys[i] = s_nd->keys[i+1]; s_nd->values[i] = s_nd->values[i+1]; }
                    s_nd->numKeys--; p_nd->keys[idx_p] = s_nd->keys[0];
                } else {
                    InternalNode *in = (InternalNode*)node, *is = (InternalNode*)s_nd;
                    in->keys[in->numKeys] = p_nd->keys[idx_p]; in->children_id[in->numKeys+1] = is->children_id[0];
                    p_nd->keys[idx_p] = is->keys[0];
                    for (int i = 0; i < is->numKeys-1; i++) is->keys[i] = is->keys[i+1];
                    for (int i = 0; i < is->numKeys; i++) is->children_id[i] = is->children_id[i+1];
                    in->numKeys++; is->numKeys--;
                }
                m_pager.update(page); m_pager.update(s_pg); m_pager.update(p_pg); return 1;
            }
        }
        int m_idx = (idx_p > 0) ? idx_p - 1 : idx_p;
        uint l_id = p_nd->children_id[m_idx], r_id = p_nd->children_id[m_idx+1];
        Page lp = m_pager.get(l_id), rp = m_pager.get(r_id);
        LeafNode *ln = (LeafNode*)lp.data, *rn = (LeafNode*)rp.data;
        if (ln->is_leaf) {
            for (int i = 0; i < rn->numKeys; i++) { ln->keys[ln->numKeys+i] = rn->keys[i]; ln->values[ln->numKeys+i] = rn->values[i]; }
            ln->numKeys += rn->numKeys; ln->nextLeaf = rn->nextLeaf;
        } else {
            InternalNode *lin = (InternalNode*)ln, *rin = (InternalNode*)rn;
            lin->keys[lin->numKeys] = p_nd->keys[m_idx];
            for (int i = 0; i < rin->numKeys; i++) lin->keys[lin->numKeys+1+i] = rin->keys[i];
            for (int i = 0; i < rin->numKeys+1; i++) lin->children_id[lin->numKeys+1+i] = rin->children_id[i];
            lin->numKeys += rin->numKeys + 1;
        }
        for (int i = m_idx; i < p_nd->numKeys-1; i++) p_nd->keys[i] = p_nd->keys[i+1];
        for (int i = m_idx+1; i < p_nd->numKeys; i++) p_nd->children_id[i] = p_nd->children_id[i+1];
        p_nd->numKeys--; m_pager.update(lp); m_pager.update(p_pg);
        m_pager.deallocatePage(r_id);
        curr_id = p_id; page = p_pg; node = (LeafNode*)p_nd;
    }
    if (rootId == curr_id && !reinterpret_cast<LeafNode*>(page.data)->is_leaf && reinterpret_cast<InternalNode*>(page.data)->numKeys == 0) {
        rootId = reinterpret_cast<InternalNode*>(page.data)->children_id[0];
        m_pager.writeRootId(rootId);
    }
    return 1;
}
