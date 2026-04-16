#include "kvdbase.hpp"

KVdbase::KVdbase(const std::string& filename) : m_pager(filename), m_tree(m_pager) {}

std::optional<int> KVdbase::get(int key) {
    return m_tree.search(key);
}

void KVdbase::put(int key, int value) {
    m_tree.insert(key, value);
}

bool KVdbase::remove(int key) {
    return m_tree.remove(key) == 1;
}
