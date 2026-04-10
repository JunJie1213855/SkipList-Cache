#include "SkipList.hpp"

int main()
{
    SkipList<int, int> list(16);
    for (int i = 0; i < 10; i++)
        list.insert(i, i * i);
    // for (int i = 0; i < 100; i += 2)
    //     list.erase(i);
    auto res = list.range_query(10, 50);
    for (auto &pair : res)
    {
        std::cout << "key : " << pair.first << ", value : " << pair.second << std::endl;
    }
    list.display();
    list.dump("dumpfile");
    return 0;
}