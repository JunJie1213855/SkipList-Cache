#ifndef SKIPLIST_H
#define SKIPLIST_H
/* ************************************************************************
> File Name:     SkipList.hpp
> Author:        PaperDreamer
> Created Time:  Fri Apr 10 06:00:00 2026
> Description:   Header-only C++17 SkipList implementation with disk persistence
 ************************************************************************/

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>
#include <vector>
#include <optional>
#include <shared_mutex>


/**
 * @brief 跳表节点类
 * @tparam K 键类型
 * @tparam V 值类型
 */
template <typename K, typename V>
class Node
{
public:
    /**
     * @brief 默认构造函数
     */
    Node() {}

    /**
     * @brief 构造函数
     * @param k 键
     * @param v 值
     * @param level 节点层级
     */
    Node(K k, V v, int level);

    /**
     * @brief 析构函数
     */
    ~Node();

    /**
     * @brief 获取键
     * @return 键值
     */
    K get_key() const;

    /**
     * @brief 获取值
     * @return 值
     */
    V get_value() const;

    /**
     * @brief 设置值
     * @param value 新的值
     */
    void set_value(V value);

    /**
     * @brief 指向每层后继节点的指针数组
     */
    std::vector<Node<K, V> *> forward;

    /**
     * @brief 该节点的层数
     */
    int node_level;

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
 * @brief 跳表类模板
 *
 * 跳表是一种概率型数据结构，通过在多层链表上建立索引来实现高效的查找、
 * 插入和删除操作。每一层都是下层的快速通道，层数越高节点越少，形成
 * 类似电梯的效果。
 *
 * ============================================================================
 * 搜索自顶向下，搜索 key=60 的示意图：
 * ============================================================================
 *
 *                           +------------+
 *                           |  select 60 |
 *                           +------------+
 * level 4     +-->1+                                                      100
 *                 |
 *                 |
 * level 3         1+-------->10+------------------>50+           70       100
 *                                                   |
 *                                                   |
 * level 2         1          10         30         50|           70       100
 *                                                   |
 *                                                   |
 * level 1         1    4     10         30         50|           70       100
 *                                                   |
 *                                                   |
 * level 0         1    4   9 10         30   40    50+-->60      70       100
 *
 * 搜索从 level 4 最高层开始，沿着前驱节点向下找到小于目标 key 的最大节点，
 * 直到 level 0，然后向右找到目标节点（或确认不存在）。
 *
 * ============================================================================
 * 插入自顶向下，再自底向上插入，插入 key=50 的示意图： 
 * ============================================================================
 *
 *                           +------------+
 *                           |  insert 50 |
 *                           +------------+
 * level 4     +-->1+                                                      100
 *                 |
 *                 |                      insert +----+
 * level 3         1+-------->10+---------------> | 50 |          70       100
 *                                               |    |
 *                                               |    |
 * level 2         1          10         30       | 50 |          70       100
 *                                               |    |
 *                                               |    |
 * level 1         1    4     10         30       | 50 |          70       100
 *                                               |    |
 *                                               |    |
 * level 0         1    4   9 10         30   40  | 50 |  60      70       100
 *                                               +----+
 *
 * 插入前先搜索确定位置，生成随机层级，然后从各层前驱节点插入新节点。
 *
 * ============================================================================
 * 先自顶向上搜索，再自底向上删除，删除 key=30 的示意图：
 * ============================================================================
 *
 * 删除前：                                 删除后：
 * level 4     +-->1+                                          100
 *                  |                                              |
 * level 3         1+-------->10+-------->30+-------->70        100
 *                                      |                          |
 * level 2         1    4    10        30+-------->70            100
 *                                      |                          |
 * level 1         1    4    10    20  30+-------->70            100
 *                                      |                          |
 * level 0         1    4    10    20 30+-->40    70            100
 *
 * 删除操作从 level 0 开始，找到待删除节点后，从各层前驱节点跳过该节点，
 * 最后释放内存。如果删除后某层为空，则降低跳表层级。
 */

template <typename K, typename V, typename Compare = std::less<K>>
class SkipList
{
public:
    /**
     * @brief 构造函数
     * @param max_level 跳表的最大层级
     * @param prob 每层晋升的概率因子，默认 0.5
     * @param comp key 的比较器，默认使用 std::less<K>
     */
    SkipList(int max_level, double prob = 0.5, Compare comp = Compare());

    /**
     * @brief 析构函数，释放所有节点资源
     */
    ~SkipList();

    /**
     * @brief 创建新节点
     * @param key 键
     * @param value 值
     * @param level 节点层级
     * @return 新创建的节点指针
     */
    Node<K, V> *create_node(K key, V value, int level);

    /**
     * @brief 插入节点
     * @param key 插入的键
     * @param value 插入的值
     * @return true 不存在键，插入成功
     * @return false 存在相同的键，更新
     */
    bool insert(K key, V value);

    /**
     * @brief 删除键对应的节点
     * @param key 要删除的键
     */
    void erase(K key);

    /**
     * @brief 显示跳表结构（调试用）
     */
    void display();

    /**
     * @brief 查询键对应的值
     * @param key 要查询的键
     * @return 存在则返回值的 optional，不存在返回 nullopt
     */
    std::optional<V> search(K key);

    /**
     * @brief 检查键是否存在
     * @param key 要检查的键
     * @return true 存在，false 不存在
     */
    bool contain(K key)
    {
        return search(key).has_value();
    }

    /**
     * @brief 范围查询，返回 [start_key, end_key) 区间内的所有键值对
     * @param start_key 范围的起始键（包含）
     * @param end_key 范围的结束键（不包含）
     * @return 在指定范围内的键值对向量
     */
    std::vector<std::pair<K, V>> range_query(const K &start_key, const K &end_key) const;

    /**
     * @brief 将跳表数据持久化到磁盘
     * @param dump_path 文件路径，格式为 "key:value\n"
     */
    void dump(std::string_view dump_path);

    /**
     * @brief 从磁盘加载数据到跳表, 仅仅支持 int 类型的 key, 其他的格式todo
     * @param dump_path 文件路径
     * @note 加载时会调用 insert 逐个插入
     */
    void load(std::string_view dump_path);

    /**
     * @brief 获取跳表元素数量
     * @return 元素个数
     */
    int size();

private:
    /**
     * @brief 从 "key:value" 格式字符串解析出 key 和 value
     * @param str 输入字符串
     * @param key 输出参数，解析出的键
     * @param value 输出参数，解析出的值
     */
    void get_key_value_from_string(const std::string &str, std::string *key, std::string *value);

    /**
     * @brief 检查字符串是否为有效的 "key:value" 格式
     * @param str 输入字符串
     * @return true 有效，false 无效
     */
    bool is_valid_string(const std::string &str);

    /**
     * @brief 根据概率生成随机层级
     * @return 生成的层级，范围 [0, _max_level]
     */
    int get_random_level();

    /**
     * @brief 使用比较器判断 a < b
     * @param a 第一个键
     * @param b 第二个键
     * @return true if a < b
     */
    bool key_less(const K &a, const K &b) const { return _comp(a, b); }

    /**
     * @brief 使用比较器判断 a == b
     * @param a 第一个键
     * @param b 第二个键
     * @return true if a == b
     */
    bool key_equal(const K &a, const K &b) const { return !_comp(a, b) && !_comp(b, a); }

private:
    /**
     * @brief 每层晋升的概率因子
     */
    double _prob;

    /**
     * @brief 线程安全互斥锁
     */
    mutable std::shared_mutex mtx;

    /**
     * @brief 持久化时的分隔符
     */
    static constexpr std::string_view delimiter = ":";

    /**
     * @brief 跳表的最大层级
     */
    int _max_level;

    /**
     * @brief 跳表当前的最大层级
     */
    int _skip_list_level;

    /**
     * @brief 头节点指针（哨兵节点）
     */
    Node<K, V> *_header;

    /**
     * @brief 文件写入器
     */
    std::ofstream _file_writer;

    /**
     * @brief 文件读取器
     */
    std::ifstream _file_reader;

    // 跳表元素梳理
    int _element_count;

    // 比较器
    Compare _comp;
};

// create new node
template <typename K, typename V, typename Compare>
Node<K, V> *SkipList<K, V, Compare>::create_node(const K k, const V v, int level)
{
    Node<K, V> *n = new Node<K, V>(k, v, level);
    return n;
}


template <typename K, typename V, typename Compare>
bool SkipList<K, V, Compare>::insert(const K key, const V value)
{
    // 多线程加锁
    std::unique_lock<std::shared_mutex> lock(mtx);

    // 头节点获取
    Node<K, V> *current = this->_header;

    // 存储可能更新的节点，与 key 相邻的节点（可能key相近也可能一致）
    std::vector<Node<K, V> *> update(this->_max_level + 1, nullptr);

    // 从上往下遍历层级
    for (int i = _skip_list_level; i >= 0; i--)
    {
        while (current->forward[i] != NULL && key_less(current->forward[i]->get_key(), key)) // 如果当前层级的节点值小于插入的key值，说明要插入到后面
        {
            current = current->forward[i]; // 下一个节点
        }
        update[i] = current; // 注意此时 current->get_key() < key < current->forward[i]->get_key(), key 正好在中间
    }

    // 从最下面一层开始，因为最底层有所有的节点，更好排查
    current = current->forward[0];

    // 如果 key 存在， 更新就可以了
    if (current != NULL && key_equal(current->get_key(), key))
    {
        current->set_value(value);
#ifdef SKIP_DEBUG_OUTSTREAM
        std::cout << "key: " << key << " exists" << ", the value update : " << value << std::endl;
#endif
        return false;
    }

    // current 为空，说明我们已经到达这层的尾部
    // current 的 key 和 插入的 key 不一致，说明我们可以在后面插入
    if (current == NULL || !key_equal(current->get_key(), key))
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
#ifdef SKIP_DEBUG_OUTSTREAM
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
#endif
        _element_count++;
    }
    return true;
}

// 显示跳表
template <typename K, typename V, typename Compare>
void SkipList<K, V, Compare>::display()
{

    std::cout << "\n================== Skip List ==================" << "\n";

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

// 数据落盘
template <typename K, typename V, typename Compare>
void SkipList<K, V, Compare>::dump(std::string_view dump_path)
{
    std::unique_lock<std::shared_mutex> lock(mtx);

    std::cout << "================== dump ==================" << std::endl;
    _file_writer.open(dump_path.cbegin());
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

// 从磁盘中加载数据
template <typename K, typename V, typename Compare>
void SkipList<K, V, Compare>::load(std::string_view dump_path)
{
    // std::unique_lock<std::shared_mutex> lock(mtx);

    _file_reader.open(dump_path.cbegin());
    std::cout << "================== load ==================" << std::endl;
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
        // 根据 K 类型解析 key - 使用 SFINAE/if constexpr 支持不同类型
        if constexpr (std::is_same_v<K, int>) {
            insert(std::stoi(*key), *value);
        } else if constexpr (std::is_same_v<K, std::string>) {
            insert(*key, *value);
        }
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }
    delete key;
    delete value;
    _file_reader.close();
}

// 获取元素数量
template <typename K, typename V, typename Compare>
int SkipList<K, V, Compare>::size()
{
    std::shared_lock<std::shared_mutex> lock(mtx);
    return _element_count;
}

template <typename K, typename V, typename Compare>
void SkipList<K, V, Compare>::get_key_value_from_string(const std::string &str, std::string *key, std::string *value)
{
    // 从字符  "key:value" 中获取对应的 key 和 value
    if (!is_valid_string(str))
    {
        return;
    }
    *key = str.substr(0, str.find(delimiter));
    *value = str.substr(str.find(delimiter) + 1, str.length());
}

template <typename K, typename V, typename Compare>
bool SkipList<K, V, Compare>::is_valid_string(const std::string &str)
{

    if (str.empty())
    {
        return false;
    }
    // 是否有 ":" 字符
    if (str.find(delimiter) == std::string::npos)
    {
        return false;
    }
    return true;
}

template <typename K, typename V, typename Compare>
void SkipList<K, V, Compare>::erase(K key)
{
    // 多线程安全
    std::unique_lock<std::shared_mutex> lock(mtx);

    // 头节点
    Node<K, V> *current = this->_header;

    // 保存要后面要更新的节点
    std::vector<Node<K, V> *> update(_max_level + 1, nullptr);

    // 自顶向下
    for (int i = _skip_list_level; i >= 0; i--)
    {
        while (current->forward[i] != NULL && key_less(current->forward[i]->get_key(), key))
        {
            current = current->forward[i];
        }
        update[i] = current;
    }
    // 从底开始查找
    current = current->forward[0];

    // 查看是否在尾部或者相等
    if (current != NULL && key_equal(current->get_key(), key))
    {

        // 自底向上
        for (int i = 0; i <= _skip_list_level; i++)
        {

            // 如果要删除的地方不等于该节点，说明从这个层级开始，往上的层级就没有这个节点了
            if (update[i]->forward[i] != current)
                break;
            
            // 跳过这个节点
            update[i]->forward[i] = current->forward[i];
        }

        // 移除没有元素的层级
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0)
        {
            _skip_list_level--;
        }

#ifdef SKIP_DEBUG_OUTSTREAM
        std::cout << "Successfully deleted key " << key << std::endl;
#endif
        delete current;
        _element_count--;
    }
    return;
}
template <typename K, typename V, typename Compare>
std::optional<V> SkipList<K, V, Compare>::search(K key)
{
#ifdef SKIP_DEBUG_OUTSTREAM
    std::cout << "================== search ==================" << std::endl;
#endif
    std::shared_lock<std::shared_mutex> lock(mtx);
    Node<K, V> *current = _header;

    // 自顶向下
    for (int i = _skip_list_level; i >= 0; i--)
    {
        while (current->forward[i] && key_less(current->forward[i]->get_key(), key))
        {
            current = current->forward[i];
        }
    }

    // 从底层查找
    current = current->forward[0];

    // 找到了节点就把 value 返回
    if (current && key_equal(current->get_key(), key))
    {
#ifdef SKIP_DEBUG_OUTSTREAM
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
#endif
        return current->get_value();
    }
#ifdef SKIP_DEBUG_OUTSTREAM
    std::cout << "Not Found Key:" << key << std::endl;
#endif
    return std::nullopt;
}


template <typename K, typename V, typename Compare>
std::vector<std::pair<K, V>> SkipList<K, V, Compare>::range_query(const K &start_key, const K &end_key) const
{
    
    if (!key_less(start_key, end_key))
    {
        return {{}};
    }
    
    std::shared_lock<std::shared_mutex> lock(mtx);
    std::vector<std::pair<K, V>> result;
    Node<K, V> *cur = _header;
    // 查找到开始节点 start_key
    for (int i = _skip_list_level; i >= 0; --i)
    {
        while (cur->forward[i] && key_less(cur->forward[i]->get_key(), start_key))
        {
            cur = cur->forward[i];
        }
    }
    // 从最下面开始
    cur = cur->forward[0];
    while (cur && key_less(cur->get_key(), end_key))
    {
        result.emplace_back(cur->get_key(), cur->get_value());
        cur = cur->forward[0];
    }
    return result;
}

// 构造跳表
template <typename K, typename V, typename Compare>
SkipList<K, V, Compare>::SkipList(int max_level, double prob, Compare comp)
    : _max_level(max_level), _prob(prob), _skip_list_level(0), _element_count(0), _comp(comp)
{
    // 创建头部，随机初始化
    K k;
    V v;
    this->_header = new Node<K, V>(k, v, _max_level);
};

// 析构跳表
template <typename K, typename V, typename Compare>
SkipList<K, V, Compare>::~SkipList()
{
    // 关闭写入和加载器
    if (_file_writer.is_open())
    {
        _file_writer.close();
    }
    if (_file_reader.is_open())
    {
        _file_reader.close();
    }

    // 清理资源
    Node<K, V> *cur = _header;
    while (cur)
    {
        Node<K, V> *next = cur->forward[0];
        delete cur;
        cur = nullptr;
        cur = next;
    }
}


// 获取随机值
template <typename K, typename V, typename Compare>
int SkipList<K, V, Compare>::get_random_level()
{
    int level = 0;
    while (level < _max_level && ((double)rand() / RAND_MAX) < _prob)
    {
        level++;
    }
    return level;
};


#endif