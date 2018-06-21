#include <iostream>
#include <vector>
#include <algorithm>
#include <map>

using namespace std;

using vitr = vector<int>::iterator;

vector<int> v = { 1, 2, 3, -1, 4, 5, 6, 7, 8, -1, -1, -1, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, -1, -1, -1, -1, -1, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, -1, -1, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, -1, -1 };
map <vitr, int> m;
int total_gaps =0 ;
void make_map()
{
    vitr start = v.begin();
    vitr end = v.end();
    vitr curr = start;
    auto is_minus_one = [](int i) { return (i == -1); };
    while (curr != end)
    {
        vitr t = find_if(curr, end, is_minus_one);    
        if (t == end) break;
        vitr t1 = find_if_not(t, end, is_minus_one);
        int dist = distance(t, t1);
        m[t] = dist;
        curr = t1;
        total_gaps += dist;
    }    
}
bool get_value(int idx, int &value)
{
    vitr idx_iter = v.begin();
    advance(idx_iter, idx);
    for (auto &elem : m)
    {
        if (idx_iter >= elem.first)
        {
            advance(idx_iter, elem.second);
        }
        else
        {
            if (idx_iter == v.end()) return false;
            value = *idx_iter;
            return true;
        }
    }
    return false;
}

int main()
{
    make_map();
    int val;
    for (int i = 0; i < (v.size()-total_gaps); i++)
    {
        if (get_value(i, val))
        {
            cout << val << endl;
        }
    }
    return 0;
} 