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
void BplusTree::insert(const int key, const int value) {
    auto curr_id = rootId;
    std::stack<uint> parents;

    while (true) {
        Page page = m_pager.get(curr_id);
        LeafNode *node = reinterpret_cast<LeafNode *>(page.data);

        if (node->is_leaf) {
            int i = 0;
            while (i < node->numKeys && node->keys[i] < key) i++;

            if (i < node->numKeys && node->keys[i] == key) {
                node->values[i] = value;
                m_pager.update(page);
                return;
            }

            if (node->numKeys < ORDER) {
                for (int j = node->numKeys; j > i; j--) {
                    node->keys[j] = node->keys[j - 1];
                    node->values[j] = node->values[j - 1];
                }
                node->keys[i] = key;
                node->values[i] = value;
                node->numKeys++;
                m_pager.update(page);
                return;
            } else {
                uint id_newleaf = m_pager.allocatePage();
                Page new_pg = m_pager.get(id_newleaf);
                LeafNode *new_node = reinterpret_cast<LeafNode *>(new_pg.data);
                new_node->is_leaf = true;

                int temp_keys[ORDER + 1];
                int temp_vals[ORDER + 1];
                
                // Copy to temp arrays
                for (int j = 0; j < ORDER; j++) {
                    if (j < i) {
                        temp_keys[j] = node->keys[j];
                        temp_vals[j] = node->values[j];
                    } else {
                        temp_keys[j + 1] = node->keys[j];
                        temp_vals[j + 1] = node->values[j];
                    }
                }
                temp_keys[i] = key;
                temp_vals[i] = value;

                int mid = (ORDER + 1) / 2;
                node->numKeys = mid;
                for (int j = 0; j < mid; j++) {
                    node->keys[j] = temp_keys[j];
                    node->values[j] = temp_vals[j];
                }

                new_node->numKeys = (ORDER + 1) - mid;
                for (int j = 0; j < new_node->numKeys; j++) {
                    new_node->keys[j] = temp_keys[mid + j];
                    new_node->values[j] = temp_vals[mid + j];
                }

                new_node->nextLeaf = node->nextLeaf;
                node->nextLeaf = id_newleaf;

                m_pager.update(page);
                m_pager.update(new_pg);

                int promoted_key = new_node->keys[0];
                uint new_child_id = id_newleaf;

                while (!parents.empty()) {
                    uint p_id = parents.top(); parents.pop();
                    Page p_pg = m_pager.get(p_id);
                    InternalNode *p_node = reinterpret_cast<InternalNode *>(p_pg.data);

                    if (p_node->numKeys < ORDER) {
                        int pi = 0;
                        while (pi < p_node->numKeys && promoted_key >= p_node->keys[pi]) pi++;
                        for (int j = p_node->numKeys; j > pi; j--) p_node->keys[j] = p_node->keys[j - 1];
                        for (int j = p_node->numKeys + 1; j > pi + 1; j--) p_node->children_id[j] = p_node->children_id[j - 1];
                        p_node->keys[pi] = promoted_key;
                        p_node->children_id[pi + 1] = new_child_id;
                        p_node->numKeys++;
                        m_pager.update(p_pg);
                        return;
                    }

                    uint id_newint = m_pager.allocatePage();
                    Page new_p_pg = m_pager.get(id_newint);
                    InternalNode *new_p_node = reinterpret_cast<InternalNode *>(new_p_pg.data);
                    new_p_node->is_leaf = false;

                    int tk[ORDER + 1];
                    uint tc[ORDER + 2];
                    int ki = 0;
                    while (ki < p_node->numKeys && promoted_key >= p_node->keys[ki]) ki++;
                    
                    // Copy keys
                    for (int j = 0, k = 0; j < ORDER + 1; j++) {
                        if (j == ki) tk[j] = promoted_key;
                        else tk[j] = p_node->keys[k++];
                    }
                    // Copy ALL children pointers
                    for (int j = 0, k = 0; j < ORDER + 2; j++) {
                        if (j == ki + 1) tc[j] = new_child_id;
                        else tc[j] = p_node->children_id[k++];
                    }

                    int m = (ORDER + 1) / 2;
                    p_node->numKeys = m;
                    for (int j = 0; j < m; j++) p_node->keys[j] = tk[j];
                    for (int j = 0; j < m + 1; j++) p_node->children_id[j] = tc[j];

                    int next_p = tk[m]; // This key goes up to the parent
                    new_p_node->numKeys = ORDER - m;
                    for (int j = 0; j < new_p_node->numKeys; j++) new_p_node->keys[j] = tk[m + 1 + j];
                    for (int j = 0; j < new_p_node->numKeys + 1; j++) new_p_node->children_id[j] = tc[m + 1 + j];

                    m_pager.update(p_pg);
                    m_pager.update(new_p_pg);
                    promoted_key = next_p;
                    new_child_id = id_newint;
                }

                uint r_id = m_pager.allocatePage();
                Page r_pg = m_pager.get(r_id);
                InternalNode *r_node = reinterpret_cast<InternalNode *>(r_pg.data);
                r_node->is_leaf = false;
                r_node->numKeys = 1;
                r_node->keys[0] = promoted_key;
                r_node->children_id[0] = rootId;
                r_node->children_id[1] = new_child_id;
                m_pager.update(r_pg);
                m_pager.writeRootId(r_id);
                rootId = r_id;
                return;
            }
        } else {
            InternalNode *in = reinterpret_cast<InternalNode *>(page.data);
            int i = 0;
            while (i < in->numKeys && key >= in->keys[i]) i++;
            parents.push(curr_id);
            curr_id = in->children_id[i];
        }
    }
}

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

    if (idx == 0 && node->numKeys > 0 && !parents.empty()) {
        std::stack<uint> p_copy = parents;
        std::stack<int> i_copy = indices;
        int new_key = node->keys[0];
        while (!p_copy.empty()) {
            uint pid = p_copy.top(); p_copy.pop();
            int pidx = i_copy.top(); i_copy.pop();
            if (pidx > 0) {
                Page pg = m_pager.get(pid);
                InternalNode* nd = (InternalNode*)pg.data;
                nd->keys[pidx-1] = new_key;
                m_pager.update(pg);
                break;
            }
        }
    }

    int minLeaf = (ORDER + 1) / 2; 
    int minInt  =  ORDER / 2;  


    if (curr_id == rootId || node->numKeys >= minLeaf) {
        return 1;  
    }

    bool is_curr_leaf = true;

    while (curr_id != rootId) {
        int minK = is_curr_leaf ? minLeaf : minInt;

        Page curr_page = m_pager.get(curr_id);
        int curr_numKeys = is_curr_leaf
            ? reinterpret_cast<LeafNode*>(curr_page.data)->numKeys
            : reinterpret_cast<InternalNode*>(curr_page.data)->numKeys;

        if (curr_numKeys >= minK) break;

        uint p_id = parents.top(); parents.pop();
        int idx_p = indices.top(); indices.pop();
        Page p_pg = m_pager.get(p_id);
        InternalNode* p_nd = reinterpret_cast<InternalNode*>(p_pg.data);

        if (idx_p > 0) {
            uint s_id = p_nd->children_id[idx_p - 1];
            Page s_pg = m_pager.get(s_id);
            int s_numKeys = reinterpret_cast<LeafNode*>(s_pg.data)->numKeys;
            int s_min = is_curr_leaf ? minLeaf : minInt;
            if (s_numKeys > s_min) {
                if (is_curr_leaf) {
                    LeafNode* cn = reinterpret_cast<LeafNode*>(curr_page.data);
                    LeafNode* sn = reinterpret_cast<LeafNode*>(s_pg.data);
                    for (int i = cn->numKeys; i > 0; i--) { cn->keys[i] = cn->keys[i-1]; cn->values[i] = cn->values[i-1]; }
                    cn->keys[0] = sn->keys[sn->numKeys-1];
                    cn->values[0] = sn->values[sn->numKeys-1];
                    cn->numKeys++; sn->numKeys--;
                    p_nd->keys[idx_p-1] = cn->keys[0];
                } else {
                    InternalNode *in = reinterpret_cast<InternalNode*>(curr_page.data);
                    InternalNode *is_nd = reinterpret_cast<InternalNode*>(s_pg.data);
                    for (int i = in->numKeys; i > 0; i--) in->keys[i] = in->keys[i-1];
                    for (int i = in->numKeys + 1; i > 0; i--) in->children_id[i] = in->children_id[i-1];
                    in->keys[0] = p_nd->keys[idx_p-1];
                    in->children_id[0] = is_nd->children_id[is_nd->numKeys];
                    p_nd->keys[idx_p-1] = is_nd->keys[is_nd->numKeys-1];
                    in->numKeys++; is_nd->numKeys--;
                }
                m_pager.update(curr_page); m_pager.update(s_pg); m_pager.update(p_pg);
                return 1;
            }
        }

        if (idx_p < p_nd->numKeys) {
            uint s_id = p_nd->children_id[idx_p + 1];
            Page s_pg = m_pager.get(s_id);
            int s_numKeys = reinterpret_cast<LeafNode*>(s_pg.data)->numKeys;
            int s_min = is_curr_leaf ? minLeaf : minInt;
            if (s_numKeys > s_min) {
                if (is_curr_leaf) {
                    LeafNode* cn = reinterpret_cast<LeafNode*>(curr_page.data);
                    LeafNode* sn = reinterpret_cast<LeafNode*>(s_pg.data);
                    cn->keys[cn->numKeys] = sn->keys[0];
                    cn->values[cn->numKeys] = sn->values[0];
                    cn->numKeys++;
                    for (int i = 0; i < sn->numKeys-1; i++) { sn->keys[i] = sn->keys[i+1]; sn->values[i] = sn->values[i+1]; }
                    sn->numKeys--;
                    p_nd->keys[idx_p] = sn->keys[0];
                } else {
                    InternalNode *in = reinterpret_cast<InternalNode*>(curr_page.data);
                    InternalNode *is_nd = reinterpret_cast<InternalNode*>(s_pg.data);
                    in->keys[in->numKeys] = p_nd->keys[idx_p];
                    in->children_id[in->numKeys+1] = is_nd->children_id[0];
                    p_nd->keys[idx_p] = is_nd->keys[0];
                    for (int i = 0; i < is_nd->numKeys-1; i++) is_nd->keys[i] = is_nd->keys[i+1];
                    for (int i = 0; i <= is_nd->numKeys-1; i++) is_nd->children_id[i] = is_nd->children_id[i+1];
                    in->numKeys++; is_nd->numKeys--;
                }
                m_pager.update(curr_page); m_pager.update(s_pg); m_pager.update(p_pg);
                return 1;
            }
        }

        // Merge
        int m_idx = (idx_p > 0) ? idx_p - 1 : idx_p;
        uint l_id = p_nd->children_id[m_idx], r_id = p_nd->children_id[m_idx+1];
        Page lp = m_pager.get(l_id), rp = m_pager.get(r_id);

        if (is_curr_leaf) {
            LeafNode *ln = reinterpret_cast<LeafNode*>(lp.data);
            LeafNode *rn = reinterpret_cast<LeafNode*>(rp.data);
            for (int i = 0; i < rn->numKeys; i++) {
                ln->keys[ln->numKeys+i] = rn->keys[i];
                ln->values[ln->numKeys+i] = rn->values[i];
            }
            ln->numKeys += rn->numKeys;
            ln->nextLeaf = rn->nextLeaf;
        } else {
            InternalNode *lin = reinterpret_cast<InternalNode*>(lp.data);
            InternalNode *rin = reinterpret_cast<InternalNode*>(rp.data);
            lin->keys[lin->numKeys] = p_nd->keys[m_idx];
            for (int i = 0; i < rin->numKeys; i++) lin->keys[lin->numKeys+1+i] = rin->keys[i];
            for (int i = 0; i < rin->numKeys+1; i++) lin->children_id[lin->numKeys+1+i] = rin->children_id[i];
            lin->numKeys += rin->numKeys + 1;
        }

        for (int i = m_idx; i < p_nd->numKeys-1; i++) p_nd->keys[i] = p_nd->keys[i+1];
        for (int i = m_idx+1; i < p_nd->numKeys; i++) p_nd->children_id[i] = p_nd->children_id[i+1];
        p_nd->numKeys--;
        m_pager.update(lp); m_pager.update(p_pg);
        m_pager.deallocatePage(r_id);

        curr_id = p_id;
        is_curr_leaf = false;
    }

    Page root_page = m_pager.get(rootId);  // ✅ re-lê a raiz atualizada
    if (!reinterpret_cast<LeafNode*>(root_page.data)->is_leaf &&
         reinterpret_cast<InternalNode*>(root_page.data)->numKeys == 0) {
        uint oldRoot = rootId;
        rootId = reinterpret_cast<InternalNode*>(root_page.data)->children_id[0];
        m_pager.writeRootId(rootId);
        m_pager.deallocatePage(oldRoot);
    }

    return 1;
}