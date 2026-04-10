# SkipList KV Storage Engine

基于跳表实现的轻量级 KV 存储引擎，支持磁盘持久化。

## 特性

- **Header-only**: 仅需包含 `SkipList.hpp` 即可使用
- **C++17**: 模板化设计，支持自定义键类型和比较器
- **线程安全**: 写操作使用互斥锁保护
- **持久化**: 支持数据落盘和加载
- **范围查询**: 支持 `[start_key, end_key)` 区间查询

## 快速开始

### 构建

```bash
cmake -B build && cmake --build build
```

### 运行示例

```bash
./build/main
```

### Benchmark 测试

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
./build/benchmark
```

## 使用示例

```cpp
#include "SkipList.hpp"

// 默认 int/int 键值对
SkipList<int, std::string> list(16);

// 插入
list.insert(1, "hello");
list.insert(2, "world");

// 查询
auto value = list.search(1);  // 返回 optional

// 删除
list.erase(1);

// 范围查询
auto range = list.range_query(10, 50);

// 持久化
list.dump("dumpfile");
list.load("dumpfile");

// 自定义比较器（可选）
struct CaseInsensitiveLess {
    bool operator()(const std::string& a, const std::string& b) const {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    }
};
SkipList<std::string, std::string, CaseInsensitiveLess> dict(16);
```

## 构造函数

```cpp
SkipList(int max_level, double prob = 0.5, Compare comp = Compare());
```

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `max_level` | 跳表最大层级 | - |
| `prob` | 每层晋升概率 | 0.5 |
| `comp` | 键比较器 | `std::less<K>` |

## 接口

| 方法 | 说明 |
|------|------|
| `insert(key, value)` | 插入/更新键值对 |
| `erase(key)` | 删除键值对 |
| `search(key)` | 查找值（返回 optional） |
| `contain(key)` | 检查键是否存在 |
| `range_query(start, end)` | 范围查询 |
| `dump(path)` | 持久化到文件 |
| `load(path)` | 从文件加载 |
| `display()` | 打印跳表结构（调试） |
| `size()` | 返回元素数量 |

## 编译宏

| 宏 | 说明 |
|-----|------|
| `SKIP_DEBUG_OUTSTREAM` | Debug 模式下启用调试输出（insert/search/erase 等操作日志） |

```bash
# Debug 模式（启用调试输出 + sanitizers）
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Release 模式（优化构建，无调试输出）
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

## 项目结构

```
.
├── SkipList.hpp    # 跳表实现（header-only）
├── main.cc         # 示例程序
├── benchmark.cc    # QPS 基准测试
└── CMakeLists.txt  # 构建配置
```

## 性能

单线程基准测试（Intel i5 12600 kf 10核 16线程，Release 模式，100000 操作）：

| 操作 | QPS |
|------|-----|
| Insert | 6896316 |
| Erase | 17834849 |
| Mixed | 5011024 |
