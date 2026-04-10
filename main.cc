#include "SkipList.hpp"

int main()
{
    SkipList<int, int> list(16);
    for (int i = 0; i < 100; i++)
        list.insert(i, i * i);
    for(int i = 0; i < 100; i += 2)
        list.erase(i);
    list.display();
    list.dump();
    return 0;
}