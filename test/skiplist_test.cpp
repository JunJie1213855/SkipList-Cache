#include <gtest/gtest.h>
#include <vector>
#include "SkipList.hpp"

// =============================================================================
// Test Fixtures
// =============================================================================

class SkipListTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

    SkipList<int, std::string> list{16};
};

// =============================================================================
// Insert Tests
// =============================================================================

TEST_F(SkipListTest, InsertSingleElement) {
    EXPECT_TRUE(list.insert(1, "one"));
    EXPECT_TRUE(list.contain(1));
    EXPECT_EQ(list.search(1).value(), "one");
}

TEST_F(SkipListTest, InsertMultipleElements) {
    EXPECT_TRUE(list.insert(1, "one"));
    EXPECT_TRUE(list.insert(2, "two"));
    EXPECT_TRUE(list.insert(3, "three"));

    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(list.search(1).value(), "one");
    EXPECT_EQ(list.search(2).value(), "two");
    EXPECT_EQ(list.search(3).value(), "three");
}

TEST_F(SkipListTest, InsertDuplicateKey) {
    EXPECT_TRUE(list.insert(1, "one"));
    EXPECT_FALSE(list.insert(1, "ONE"));  // 更新，返回 false
    EXPECT_EQ(list.search(1).value(), "ONE");  // 值已更新
    EXPECT_EQ(list.size(), 1);  // 大小不变
}

TEST_F(SkipListTest, InsertDifferentKeyTypes) {
    SkipList<std::string, std::string> str_list{16};
    EXPECT_TRUE(str_list.insert("key1", "value1"));
    EXPECT_TRUE(str_list.insert("key2", "value2"));
    EXPECT_EQ(str_list.size(), 2);
    EXPECT_EQ(str_list.search("key1").value(), "value1");
}

// =============================================================================
// Search Tests
// =============================================================================

TEST_F(SkipListTest, SearchExistingKey) {
    list.insert(1, "one");
    list.insert(2, "two");

    auto result = list.search(1);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "one");
}

TEST_F(SkipListTest, SearchNonExistingKey) {
    list.insert(1, "one");

    auto result = list.search(999);
    EXPECT_FALSE(result.has_value());
}

TEST_F(SkipListTest, SearchInEmptyList) {
    auto result = list.search(1);
    EXPECT_FALSE(result.has_value());
}

// =============================================================================
// Erase Tests
// =============================================================================

TEST_F(SkipListTest, EraseExistingKey) {
    list.insert(1, "one");
    EXPECT_TRUE(list.contain(1));

    list.erase(1);
    EXPECT_FALSE(list.contain(1));
}

TEST_F(SkipListTest, EraseNonExistingKey) {
    list.insert(1, "one");
    list.erase(999);  // 不应崩溃

    EXPECT_EQ(list.size(), 1);  // 大小不变
}

TEST_F(SkipListTest, EraseAndReinsert) {
    list.insert(1, "one");
    list.erase(1);
    EXPECT_FALSE(list.contain(1));

    list.insert(1, "ONE");
    EXPECT_TRUE(list.contain(1));
    EXPECT_EQ(list.search(1).value(), "ONE");
}

// =============================================================================
// Size Tests
// =============================================================================

TEST_F(SkipListTest, SizeEmptyList) {
    EXPECT_EQ(list.size(), 0);
}

TEST_F(SkipListTest, SizeAfterInsert) {
    list.insert(1, "one");
    list.insert(2, "two");
    EXPECT_EQ(list.size(), 2);
}

TEST_F(SkipListTest, SizeAfterErase) {
    list.insert(1, "one");
    list.insert(2, "two");
    list.erase(1);
    EXPECT_EQ(list.size(), 1);
}

// =============================================================================
// Contain Tests
// =============================================================================

TEST_F(SkipListTest, ContainExistingKey) {
    list.insert(1, "one");
    EXPECT_TRUE(list.contain(1));
}

TEST_F(SkipListTest, ContainNonExistingKey) {
    list.insert(1, "one");
    EXPECT_FALSE(list.contain(999));
}

TEST_F(SkipListTest, ContainInEmptyList) {
    EXPECT_FALSE(list.contain(1));
}

// =============================================================================
// Range Query Tests
// =============================================================================

TEST_F(SkipListTest, RangeQueryNormal) {
    for (int i = 1; i <= 10; ++i) {
        list.insert(i, std::to_string(i));
    }

    auto results = list.range_query(3, 7);
    EXPECT_EQ(results.size(), 4);  // 3, 4, 5, 6

    std::vector<int> keys;
    for (const auto& [k, v] : results) {
        keys.push_back(k);
    }
    EXPECT_EQ(keys, (std::vector<int>{3, 4, 5, 6}));
}

TEST_F(SkipListTest, RangeQueryEmpty) {
    list.insert(5, "five");

    auto results = list.range_query(10, 20);
    EXPECT_EQ(results.size(), 0);
}

TEST_F(SkipListTest, RangeQueryAll) {
    list.insert(1, "one");
    list.insert(2, "two");
    list.insert(3, "three");

    auto results = list.range_query(0, 100);
    EXPECT_EQ(results.size(), 3);
}

TEST_F(SkipListTest, RangeQuerySingleElement) {
    list.insert(5, "five");

    auto results = list.range_query(5, 6);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].first, 5);
}

// =============================================================================
// String Key Tests
// =============================================================================

TEST_F(SkipListTest, StringKeyInsert) {
    SkipList<std::string, std::string> str_list{16};
    EXPECT_TRUE(str_list.insert("user1", "john"));
    EXPECT_TRUE(str_list.insert("user2", "jane"));

    EXPECT_EQ(str_list.size(), 2);
    EXPECT_EQ(str_list.search("user1").value(), "john");
}

TEST_F(SkipListTest, StringKeySearch) {
    SkipList<std::string, std::string> str_list{16};
    str_list.insert("name:alice", "Alice");
    str_list.insert("name:bob", "Bob");

    EXPECT_TRUE(str_list.search("name:alice").has_value());
    EXPECT_FALSE(str_list.search("name:charlie").has_value());
}

// =============================================================================
// Custom Comparator Tests
// =============================================================================

TEST_F(SkipListTest, CustomComparatorReverse) {
    SkipList<int, std::string, std::greater<int>> rev_list{16};
    rev_list.insert(1, "one");
    rev_list.insert(3, "three");
    rev_list.insert(2, "two");

    // 使用 greater<int>，键按降序排列
    EXPECT_TRUE(rev_list.contain(3));
    EXPECT_TRUE(rev_list.contain(2));
    EXPECT_TRUE(rev_list.contain(1));
}

// =============================================================================
// Dump/Load Tests
// =============================================================================

TEST_F(SkipListTest, DumpAndLoad) {
    // 插入数据
    list.insert(1, "one");
    list.insert(2, "two");
    list.insert(3, "three");

    // Dump 到文件
    const char* filename = "/tmp/skiplist_test_dump.txt";
    list.dump(filename);

    // 创建新 list 并 load
    SkipList<int, std::string> new_list{16};
    new_list.load(filename);

    // 验证数据一致
    EXPECT_EQ(new_list.size(), 3);
    EXPECT_EQ(new_list.search(1).value(), "one");
    EXPECT_EQ(new_list.search(2).value(), "two");
    EXPECT_EQ(new_list.search(3).value(), "three");
}

TEST_F(SkipListTest, DumpAndLoadEmpty) {
    const char* filename = "/tmp/skiplist_test_empty.txt";

    // 空表 dump
    list.dump(filename);

    // Load 空文件
    SkipList<int, std::string> new_list{16};
    new_list.load(filename);

    EXPECT_EQ(new_list.size(), 0);
}

TEST_F(SkipListTest, DumpAndLoadStringKey) {
    SkipList<std::string, std::string> str_list{16};
    str_list.insert("user1", "john");
    str_list.insert("user2", "jane");

    const char* filename = "/tmp/skiplist_test_strkey.txt";
    str_list.dump(filename);

    SkipList<std::string, std::string> new_list{16};
    new_list.load(filename);

    EXPECT_EQ(new_list.size(), 2);
    EXPECT_EQ(new_list.search("user1").value(), "john");
    EXPECT_EQ(new_list.search("user2").value(), "jane");
}
