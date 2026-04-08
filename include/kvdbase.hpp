#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <optional>

class KVdbase {
    public:
        KVdbase() = default;

        std::optional<std::string> get(const std::string& key) const; // get method   
        bool put(const std::string& key, const std::string& value); // put method
        bool remove(const std::string& key); // del method 

    private:
        std::unordered_map<std::string, std::string> m_data;
        mutable std::mutex m_mutex;
};