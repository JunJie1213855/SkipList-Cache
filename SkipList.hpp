#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>
#include <vector>
#include <optional>

#define STORE_FILE "dumpFile"

std::mutex mtx;              // mutex for critical section
std::string delimiter = ":"; // 分割符

/**
 * @brief Class template to implement node
 */
template <typename K, typename V>
class Node
{
public:
    Node() {}
    // 初始化
    Node(K k, V v, int level);

    ~Node();

    // 获取 key
    K get_key() const;

    // 获取 value
    V get_value() const;

    // 设置
    void set_value(V value);

    // 指向每层后继节点的指针数组
    std::vector<Node<K, V> *> forward;

    int node_level; // 该节点的层数

private:
    K key;
    V value;
};

template <typename K, typename V>
Node<K, V>::Node(const K k, const V v, int level)
    : key(k), value(v), node_level(level), forward(level + 1, nullptr) {
      };

template <typename K, typename V>
Node<K, V>::~Node()
{
    forward.clear();
};

template <typename K, typename V>
K Node<K, V>::get_key() const
{
    return key;
};

template <typename K, typename V>
V Node<K, V>::get_value() const
{
    return value;
};
template <typename K, typename V>
void Node<K, V>::set_value(V value)
{
    this->value = value;
};

/**
 * @brief Class template for Skip list
 */
template <typename K, typename V>
class SkipList
{

public:
    SkipList(int max_level);
    ~SkipList();

    // 获取跳表的随机
    int get_random_level();

    // 创建节点
    Node<K, V> *create_node(K key, V value, int level);

    /**
     * @brief  插入节点
     * @param K 插入的键
     * @param V 插入的值
     * @return true 不存在键，插入成功
     * @return false 存在相同的键，更新
     */
    bool insert_element(K key, V value);

    // 显示列表
    void display_list();

    // 查询键
    std::optional<V> search_element(K key);

    // 删除键
    void delete_element(K key);

    // 落盘
    void dump_file();

    // 安装
    void load_file();

    // 递归删除节点
    void clear(Node<K, V> *node);

    // 尺寸大小
    int size();

private:
    void get_key_value_from_string(const std::string &str, std::string *key, std::string *value);
    bool is_valid_string(const std::string &str);

private:
    // Maximum level of the skip list
    int _max_level;

    // current level of skip list
    int _skip_list_level;

    // pointer to header node
    Node<K, V> *_header;

    // file operator
    std::ofstream _file_writer;
    std::ifstream _file_reader;

    // skiplist current element count
    int _element_count;
};

// create new node
template <typename K, typename V>
Node<K, V> *SkipList<K, V>::create_node(const K k, const V v, int level)
{
    Node<K, V> *n = new Node<K, V>(k, v, level);
    return n;
}

// Insert given key and value in skip list
// return 1 means element exists
// return 0 means insert successfully
/*
                           +------------+
                           |  insert 50 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |                      insert +----+
level 3         1+-------->10+---------------> | 50 |          70       100
                                               |    |
                                               |    |
level 2         1          10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 1         1    4     10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 0         1    4   9 10         30   40  | 50 |  60      70       100
                                               +----+

*/
template <typename K, typename V>
bool SkipList<K, V>::insert_element(const K key, const V value)
{
    // 多线程加锁
    std::lock_guard<std::mutex> lock(mtx);

    // 头节点获取
    Node<K, V> *current = this->_header;

    // 存储可能更新的节点，与 key 相邻的节点（可能key相近也可能一致）
    std::vector<Node<K, V> *> update(this->_max_level + 1, nullptr);

    // 从上往下遍历层级
    for (int i = _skip_list_level; i >= 0; i--)
    {
        while (current->forward[i] != NULL && current->forward[i]->get_key() < key) // 如果当前层级的节点值小于插入的key值，说明要插入到后面
        {
            current = current->forward[i]; // 下一个节点
        }
        update[i] = current; // 注意此时 current->get_key() < key < current->forward[i]->get_key(), key 正好在中间
    }

    // 从最下面一层开始，因为最底层有所有的节点，更好排查
    current = current->forward[0];

    // 如果 key 存在， 更新就可以了
    if (current != NULL && current->get_key() == key)
    {
        current->set_value(value);
        std::cout << "key: " << key << " exists" << ", the value update : " << value << std::endl;
        return false;
    }

    // current 为空，说明我们已经到达这层的尾部
    // current 的 key 和 插入的 key 不一致，说明我们可以在后面插入
    if (current == NULL || current->get_key() != key)
    {

        // 生成随机的层级 ！
        int random_level = get_random_level();

        // 如果生成的层级大于当前跳表的层级，需要将 head 节点往上添加
        if (random_level > _skip_list_level)
        {
            // 把头节点往上添加
            for (int i = _skip_list_level + 1; i < random_level + 1; i++)
            {
                update[i] = _header;
            }
            _skip_list_level = random_level;
        }

        // 创建新节点
        Node<K, V> *inserted_node = create_node(key, value, random_level);

        // 把新节点插入
        for (int i = 0; i <= random_level; i++)
        {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        _element_count++;
    }
    return true;
}

// Display skip list
template <typename K, typename V>
void SkipList<K, V>::display_list()
{

    std::cout << "\n*****Skip List*****" << "\n";

    // 自底向上
    for (int i = 0; i <= _skip_list_level; i++)
    {
        Node<K, V> *node = this->_header->forward[i];
        std::cout << "Level " << i << ": ";
        while (node != NULL)
        {
            std::cout << node->get_key() << ":" << node->get_value() << ";";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

// Dump data in memory to file
template <typename K, typename V>
void SkipList<K, V>::dump_file()
{

    std::cout << "dump_file-----------------" << std::endl;
    _file_writer.open(STORE_FILE);
    Node<K, V> *node = this->_header->forward[0];

    while (node != NULL)
    {
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";
        std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        node = node->forward[0];
    }

    _file_writer.flush();
    _file_writer.close();
    return;
}

// Load data from disk
template <typename K, typename V>
void SkipList<K, V>::load_file()
{

    _file_reader.open(STORE_FILE);
    std::cout << "load_file-----------------" << std::endl;
    std::string line;
    std::string *key = new std::string();
    std::string *value = new std::string();
    while (getline(_file_reader, line))
    {
        get_key_value_from_string(line, key, value);
        if (key->empty() || value->empty())
        {
            continue;
        }
        // Define key as int type
        insert_element(stoi(*key), *value);
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }
    delete key;
    delete value;
    _file_reader.close();
}

// Get current SkipList size
template <typename K, typename V>
int SkipList<K, V>::size()
{
    return _element_count;
}

template <typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string &str, std::string *key, std::string *value)
{

    if (!is_valid_string(str))
    {
        return;
    }
    *key = str.substr(0, str.find(delimiter));
    *value = str.substr(str.find(delimiter) + 1, str.length());
}

template <typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string &str)
{

    if (str.empty())
    {
        return false;
    }
    if (str.find(delimiter) == std::string::npos)
    {
        return false;
    }
    return true;
}

// Delete element from skip list
template <typename K, typename V>
void SkipList<K, V>::delete_element(K key)
{

    // mtx.lock();
    std::lock_guard<std::mutex> lock(mtx);
    Node<K, V> *current = this->_header;
    // Node<K, V> *update[_max_level + 1];
    // memset(update, 0, sizeof(Node<K, V> *) * (_max_level + 1));
    std::vector<Node<K,V> *> update(_max_level + 1, nullptr);

    // start from highest level of skip list
    for (int i = _skip_list_level; i >= 0; i--)
    {
        while (current->forward[i] != NULL && current->forward[i]->get_key() < key)
        {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    if (current != NULL && current->get_key() == key)
    {

        // start for lowest level and delete the current node of each level
        for (int i = 0; i <= _skip_list_level; i++)
        {

            // if at level i, next node is not target node, break the loop.
            if (update[i]->forward[i] != current)
                break;

            update[i]->forward[i] = current->forward[i];
        }

        // Remove levels which have no elements
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0)
        {
            _skip_list_level--;
        }

        std::cout << "Successfully deleted key " << key << std::endl;
        delete current;
        _element_count--;
    }
    // mtx.unlock();
    return;
}

// Search for element in skip list
/*
                           +------------+
                           |  select 60 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |
level 3         1+-------->10+------------------>50+           70       100
                                                   |
                                                   |
level 2         1          10         30         50|           70       100
                                                   |
                                                   |
level 1         1    4     10         30         50|           70       100
                                                   |
                                                   |
level 0         1    4   9 10         30   40    50+-->60      70       100
*/
template <typename K, typename V>
std::optional<V> SkipList<K, V>::search_element(K key)
{

    std::cout << "search_element-----------------" << std::endl;
    Node<K, V> *current = _header;

    // start from highest level of skip list
    for (int i = _skip_list_level; i >= 0; i--)
    {
        while (current->forward[i] && current->forward[i]->get_key() < key)
        {
            current = current->forward[i];
        }
    }

    // reached level 0 and advance pointer to right node, which we search
    current = current->forward[0];

    // if current node have key equal to searched key, we get it
    if (current && current->get_key() == key)
    {
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return current->get_value();
    }

    std::cout << "Not Found Key:" << key << std::endl;
    return std::nullopt;
}

// construct skip list
template <typename K, typename V>
SkipList<K, V>::SkipList(int max_level)
{

    this->_max_level = max_level;
    this->_skip_list_level = 0;
    this->_element_count = 0;

    // create header node and initialize key and value to null
    K k;
    V v;
    this->_header = new Node<K, V>(k, v, _max_level);
};

template <typename K, typename V>
SkipList<K, V>::~SkipList()
{

    if (_file_writer.is_open())
    {
        _file_writer.close();
    }
    if (_file_reader.is_open())
    {
        _file_reader.close();
    }

    // 递归删除跳表链条
    if (_header->forward[0] != nullptr)
    {
        clear(_header->forward[0]);
    }
    delete (_header);
    _header = nullptr;
}
template <typename K, typename V>
void SkipList<K, V>::clear(Node<K, V> *cur)
{
    if (cur->forward[0] != nullptr)
    {
        clear(cur->forward[0]);
    }
    delete (cur);
    cur = nullptr;
}

template <typename K, typename V>
int SkipList<K, V>::get_random_level()
{

    int k = 1;
    while (rand() % 2)
    {
        k++;
    }
    k = (k < _max_level) ? k : _max_level;
    return k;
};
// vim: et tw=100 ts=4 sw=4 cc=120
