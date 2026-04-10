#include "SkipList.hpp"

int main()
{
    SkipList<int, int> list(16);
    for (int i = 0; i < 100; i++)
        list.insert_element(i, i * i);

    list.display_list();
    list.dump_file();
    return 0;
}