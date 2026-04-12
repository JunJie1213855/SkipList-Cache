#include "SkipList.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <variant>
#include <type_traits>
#include <cctype>
#include <fstream>
#include <cstdlib>
#include <readline/readline.h>
#include <readline/history.h>

// =============================================================================
// SFINAE: 类型检测
// =============================================================================

template<typename T>
constexpr bool is_integral_v = std::is_integral_v<T>;

template<typename T>
constexpr bool is_string_v = std::is_same_v<T, std::string>;

// =============================================================================
// SkipList 类型别名
// =============================================================================
using IntSkipList = SkipList<int, std::string>;
using StrSkipList = SkipList<std::string, std::string>;
using SkipListVariant = std::variant<IntSkipList, StrSkipList>;

// =============================================================================
// 辅助函数
// =============================================================================

bool looks_like_int(const std::string& s) {
    if (s.empty()) return false;
    size_t start = 0;
    if (s[0] == '-') {
        if (s.size() == 1) return false;
        start = 1;
    }
    for (size_t i = start; i < s.size(); ++i) {
        if (!std::isdigit(s[i])) return false;
    }
    return true;
}

// =============================================================================
// 操作函数 (SFINAE 重载)
// =============================================================================

// insert - int 版本
bool insert_impl(IntSkipList& list, int key, const std::string& value) {
    return list.insert(key, value);
}

// insert - string 版本
bool insert_impl(StrSkipList& list, const std::string& key, const std::string& value) {
    return list.insert(key, value);
}

// search - int 版本
auto search_impl(IntSkipList& list, int key) {
    return list.search(key);
}

// search - string 版本
auto search_impl(StrSkipList& list, const std::string& key) {
    return list.search(key);
}

// erase - int 版本
void erase_impl(IntSkipList& list, int key) {
    list.erase(key);
}

// erase - string 版本
void erase_impl(StrSkipList& list, const std::string& key) {
    list.erase(key);
}

// =============================================================================
// 打印帮助
// =============================================================================

void print_help() {
    std::cout << "SkipList CLI Commands:\n";
    std::cout << "  insert <key> <value>  - Insert a key-value pair (key: auto-detect)\n";
    std::cout << "  search <key>          - Search for a key\n";
    std::cout << "  delete <key>          - Delete a key\n";
    std::cout << "  list                 - Display all elements\n";
    std::cout << "  range <start> <end>  - Query range [start, end)\n";
    std::cout << "  size                 - Show element count\n";
    std::cout << "  dump <file>          - Dump to file\n";
    std::cout << "  load <file>          - Load from file\n";
    std::cout << "  help                 - Show this help\n";
    std::cout << "  quit                 - Exit\n";
    std::cout << "\nKey auto-detection:\n";
    std::cout << "  Numbers (123, -456)      -> int key\n";
    std::cout << "  Others (user1, name:foo) -> string key\n";
}

// =============================================================================
// 操作函数
// =============================================================================

bool do_insert(SkipListVariant& var, const std::string& key_str, const std::string& value) {
    if (looks_like_int(key_str)) {
        int key = std::stoi(key_str);
        if (std::holds_alternative<IntSkipList>(var)) {
            return std::get<IntSkipList>(var).insert(key, value);
        } else {
            var.emplace<IntSkipList>(16);
            return std::get<IntSkipList>(var).insert(key, value);
        }
    } else {
        if (std::holds_alternative<StrSkipList>(var)) {
            return std::get<StrSkipList>(var).insert(key_str, value);
        } else {
            var.emplace<StrSkipList>(16);
            return std::get<StrSkipList>(var).insert(key_str, value);
        }
    }
}

auto do_search(SkipListVariant& var, const std::string& key_str) {
    if (looks_like_int(key_str)) {
        int key = std::stoi(key_str);
        if (auto* list = std::get_if<IntSkipList>(&var)) {
            return list->search(key);
        }
    } else {
        if (auto* list = std::get_if<StrSkipList>(&var)) {
            return list->search(key_str);
        }
    }
    return std::optional<std::string>{};
}

void do_erase(SkipListVariant& var, const std::string& key_str) {
    if (looks_like_int(key_str)) {
        int key = std::stoi(key_str);
        if (auto* list = std::get_if<IntSkipList>(&var)) {
            list->erase(key);
        }
    } else {
        if (auto* list = std::get_if<StrSkipList>(&var)) {
            list->erase(key_str);
        }
    }
}

void do_list(SkipListVariant& var) {
    std::visit([](auto& list) { list.display(); }, var);
}

size_t do_size(SkipListVariant& var) {
    return std::visit([](auto& list) -> size_t { return list.size(); }, var);
}

// range 只支持 int key
auto do_range(SkipListVariant& var, int start, int end) {
    if (auto* list = std::get_if<IntSkipList>(&var)) {
        return list->range_query(start, end);
    }
    return std::vector<std::pair<int, std::string>>{};
}

void do_dump(SkipListVariant& var, const std::string& filename) {
    std::visit([&filename](auto& list) { list.dump(filename); }, var);
}

void do_load(SkipListVariant& var, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Cannot open file: " << filename << "\n";
        return;
    }

    // 读取第一行判断类型
    std::string first_line;
    bool has_content = false;
    if (std::getline(file, first_line)) {
        has_content = !first_line.empty();
        if (has_content) {
            size_t colon = first_line.find(':');
            if (colon != std::string::npos) {
                std::string key_str = first_line.substr(0, colon);
                if (looks_like_int(key_str)) {
                    var.emplace<IntSkipList>(16);
                    std::get<IntSkipList>(var).load(filename);
                } else {
                    var.emplace<StrSkipList>(16);
                    std::get<StrSkipList>(var).load(filename);
                }
            }
        }
    }
    file.close();
}

// =============================================================================
// 主程序
// =============================================================================

int main() {
    SkipListVariant list{std::in_place_type<IntSkipList>, 16};

    std::cout << "SkipList CLI - Type 'help' for commands\n";
    std::cout << "Default key type: int. Use numeric keys for int, strings for string.\n";
    std::cout << "Tip: Use Up/Down arrows for history, Left/Right to move cursor.\n\n";

    std::string line;
    while (true) {
        char* input = readline("> ");
        if (!input) break;  // EOF
        if (*input) {
            add_history(input);
            line = input;
        }
        free(input);
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "insert") {
            std::string key, value;
            if (!(iss >> key >> value)) {
                std::cout << "Usage: insert <key> <value>\n";
                continue;
            }
            bool success = do_insert(list, key, value);
            std::cout << "Inserted: " << key << " -> " << value;
            std::cout << " (type: " << (looks_like_int(key) ? "int" : "string") << ")\n";
        }
        else if (cmd == "search") {
            std::string key;
            if (!(iss >> key)) {
                std::cout << "Usage: search <key>\n";
                continue;
            }
            auto result = do_search(list, key);
            if (result) {
                std::cout << "Found: " << key << " -> " << result.value() << "\n";
            } else {
                std::cout << "Not found: " << key << "\n";
            }
        }
        else if (cmd == "delete") {
            std::string key;
            if (!(iss >> key)) {
                std::cout << "Usage: delete <key>\n";
                continue;
            }
            do_erase(list, key);
            std::cout << "Deleted: " << key << "\n";
        }
        else if (cmd == "list") {
            do_list(list);
        }
        else if (cmd == "range") {
            std::string start_str, end_str;
            if (!(iss >> start_str >> end_str)) {
                std::cout << "Usage: range <start> <end>\n";
                continue;
            }
            if (looks_like_int(start_str) && looks_like_int(end_str)) {
                auto results = do_range(list, std::stoi(start_str), std::stoi(end_str));
                std::cout << "Range [" << start_str << ", " << end_str << "):\n";
                for (auto& [k, v] : results) {
                    std::cout << "  " << k << " -> " << v << "\n";
                }
                std::cout << "Total: " << results.size() << " elements\n";
            } else {
                std::cout << "Range query only supports int keys currently\n";
            }
        }
        else if (cmd == "size") {
            std::cout << "Size: " << do_size(list) << "\n";
        }
        else if (cmd == "dump") {
            std::string filename;
            if (!(iss >> filename)) {
                std::cout << "Usage: dump <filename>\n";
                continue;
            }
            do_dump(list, filename);
            std::cout << "Dumped to: " << filename << "\n";
        }
        else if (cmd == "load") {
            std::string filename;
            if (!(iss >> filename)) {
                std::cout << "Usage: load <filename>\n";
                continue;
            }
            do_load(list, filename);
            std::cout << "Loaded from: " << filename << "\n";
        }
        else if (cmd == "help") {
            print_help();
        }
        else if (cmd == "quit" || cmd == "exit") {
            break;
        }
        else {
            std::cout << "Unknown command: " << cmd << ". Type 'help' for commands.\n";
        }
    }

    return 0;
}
