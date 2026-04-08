#include "../include/kvdbase.hpp"
#include <string>
#include <optional>


bool KVdbase::put(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_data[key] = value;
    return true;
}

std::optional<std::string> KVdbase::get(const std::string& key) const{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_data.find(key);

    if (it == m_data.end()) {
        return std::nullopt;
    }

    return it->second;

};

bool KVdbase::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);

    size_t num_removed = m_data.erase(key);

    return num_removed > 0;
}