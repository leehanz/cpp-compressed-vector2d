#include "vector2d.h"

using namespace std;

int main()
{
    // test default constructor.
    vector2d<double> v0;
    v0.push_back({ 21, 21, 21 });
    v0.push_back({ 22, 22, 22 });
    v0.push_back({ 23, 23, 23 });
    v0.push_back({ 24, 24, 24 });
    v0.push_back({ 25, 25, 25 });

    v0[0].push_back(-1);
    v0[1].push_back(-1);
    v0[2].push_back(-1);
    v0[3].resize(6);
    v0.print();

    // test nrow constructor.
    vector2d<double> v(5);

    // test vecto2d::row_type::push_back
    v[0].push_back(1);
    v.push_back({7, 8, 9});
    v[2].push_back(4);
    std::vector<double> tmp = {10, 11, 12};
    v.push_back(tmp);
    v[2].push_back(5);
    v[2].push_back(6);
    v[0].push_back(2);
    v.push_back({13, 14});
    v[0].push_back(3);

    // test vecto2d::row_type::reserve
    v[0].reserve(4);
    v[0].push_back(4);
    v[2].reserve(4);

    // test vecto2d::row_type::insert
    auto a = v[2].insert(v[2].begin() + 2, 99);
    auto b = v[2].insert(v[2].begin() + 2, 98);
    auto c = v[0].insert(v[0].begin() + 2, 99);

    v[2].reserve(8);
    std::vector<double> t = {95, 96, 97};
    auto d = v[2].insert(v[2].begin() + 2, t.begin(), t.end());

    std::vector<double> t2 = {93, 94};
    auto e = v[1].insert(v[1].end(), 93);

    std::vector<double> t3 = {97, 98};
    auto f = v[0].insert(v[0].end(), t3.begin(), t3.end());
    v[0].insert(v[0].end(), 99);

    // test vecto2d::row_type::erase
    auto g = v[0].erase(v[0].begin()+2);
    auto h = v[2].erase(v[2].begin(), v[2].begin()+2);

    // test vecto2d::row_type::clear
    v[6].clear();

    // test vector2d::insert
    v.insert(v.begin() + 1, v0[1]);
    v.insert(v.begin() + 2, v0.begin() + 2, v0.begin() + 4);

    // test vector2d::pop_back
    v.pop_back();

    // test vector2d::resize
    v.resize(12, v0[1]);

    // test vector2d::erase
    v.erase(v.begin() + 1, v.begin() + 4);
    v.erase(v.end() - 5);

    // test vector2d::compact
    v.compact();
     
    v.print();

    return 0;
}