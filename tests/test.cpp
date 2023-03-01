#include <algorithm>
#include <catch.hpp>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <vector>

#include "hash_map.hpp"

struct StrangeInt {
    int x;
    static int counter;

    StrangeInt() {
        ++counter;
    }

    StrangeInt(int x) : x(x) {  // NOLINT
        ++counter;
    }

    StrangeInt(const StrangeInt& rs) : x(rs.x) {
        ++counter;
    }

    bool operator==(const StrangeInt& rs) const {
        return x == rs.x;
    }

    static void init() {
        counter = 0;
    }

    ~StrangeInt() {
        --counter;
    }

    friend std::ostream& operator<<(std::ostream& out, const StrangeInt& x) {
        out << x.x;
        return out;
    }
};

int StrangeInt::counter;

namespace std {
template <>
struct hash<StrangeInt> {
    size_t operator()(const StrangeInt& x) const {
        return x.x;
    }
};
}  // namespace std

TEST_CASE("Interface check") {
    const HashMap<int, int> map{{1, 5}, {3, 4}, {2, 1}};

    REQUIRE(map.empty() == false);
    REQUIRE(std::is_same<HashMap<int, int>::const_iterator, decltype(map.begin())>::value == true);

    auto hash_f = map.hash_function();
    HashMap<int, int>::const_iterator it = map.find(3);
    REQUIRE(it->second == 4);
    it = map.find(7);
    REQUIRE(it == map.end());
    REQUIRE(std::is_same<const int, std::remove_reference<decltype(map.at(1))>::type>::value);
}

TEST_CASE("Exception check") {
    const HashMap<int, int> map{{2, 3}, {-7, -13}, {0, 8}};
    try {
        auto cur = map.at(8);
    } catch (const std::out_of_range& e) {
        return;
    } catch (...) {
        FAIL("'at' doesn't throw std::out_of_range");
    }
    FAIL("'at' doesn't throw anything");
}

TEST_CASE("Check destructor") {
    StrangeInt::init();
    {
        HashMap<StrangeInt, int> s{{5, 4}, {3, 2}, {1, 0}};
        REQUIRE(s.size() == 3);
    }
    REQUIRE(StrangeInt::counter == 0);
    {
        HashMap<StrangeInt, int> s{{-3, 3}, {-2, 2}, {-1, 1}};
        HashMap<StrangeInt, int> s1(s);
        s1.insert(std::make_pair(0, 0));
        HashMap<StrangeInt, int> s2(s1);
        REQUIRE(s1.find(0) != s1.end());
    }
    REQUIRE(StrangeInt::counter == 0);
}

TEST_CASE("Reference check") {
    HashMap<int, int> map{{3, 4}, {3, 5}, {4, 7}, {-1, -3}};
    map[3] = 7;
    REQUIRE(map[3] == 7);
    REQUIRE(map[0] == 0);
    auto it = map.find(4);
    REQUIRE(it != map.end());
    it->second = 3;
    auto cur = map.find(4);
    REQUIRE(cur->second == 3);
}

size_t stupid_hash(int /*x*/) {
    return 0;
}

TEST_CASE("Hash check") {
    struct Hasher {
        std::hash<std::string> hasher;
        size_t operator()(const std::string& s) const {
            return hasher(s);
        }
    };
    HashMap<std::string, std::string, Hasher> map{{"aba", "caba"}, {"simple", "case"}, {"test", "test"}};
    auto simple_hash = [](unsigned long long x) -> size_t { return x % 17239; };
    HashMap<int, std::string, decltype(simple_hash)> second_map(simple_hash);
    auto it = second_map.insert(std::make_pair(0, "a"));
    it = second_map.insert(std::make_pair(0, "b"));
    second_map[17239] = "check";
    it = second_map.find(0);
    auto second_hash_fn = second_map.hash_function();
    REQUIRE(second_hash_fn(17239) == 0);
    it = second_map.find(0);
    REQUIRE(second_map[0] == "a");
    REQUIRE(second_map[17239] == "check");
    HashMap<int, int, std::function<size_t(int)>> stupid_map(stupid_hash);
    auto stupid_hash_fn = stupid_map.hash_function();
    for (int i = 0; i < 1000; ++i) {
        stupid_map[i] = i + 1;
        if (stupid_hash_fn(i)) {
            FAIL("Wrong hash function in class");
        }
    }
    REQUIRE(stupid_map.size() == 1000);
}

TEST_CASE("Check copy") {
    HashMap<int, int> first;
    HashMap<int, int> second(first);
    second.insert(std::make_pair(1, 1));
    HashMap<int, int> third(second.begin(), second.end());
    third[0] = 5;
    REQUIRE(third.size() == 2);
    first = third;
    second = second = first;
    REQUIRE(first.find(0)->second == 5);
    REQUIRE(second[0] == 5);
}

TEST_CASE("Check iterators") {
    {
        HashMap<int, int> first{{0, 0}};
        HashMap<int, int>::iterator just_iterator;
        HashMap<int, int>::iterator it = first.begin();
        REQUIRE(std::is_same<const int, std::remove_reference<decltype(it->first)>::type>::value);
        REQUIRE(it++ == first.begin());
        REQUIRE(it == first.end());
        REQUIRE(++first.begin() == first.end());
        first.erase(0);
        REQUIRE(first.begin() == first.end());
        just_iterator = first.begin();
    }
    {
        const HashMap<int, int> first{{1, 1}};
        HashMap<int, int>::const_iterator just_iterator;
        HashMap<int, int>::const_iterator it = first.begin();
        REQUIRE(it++ == first.begin());
        REQUIRE(it == first.end());
        REQUIRE(++first.begin() == first.end());
        just_iterator = it;
    }
}

TEST_CASE("Check workability") {
    {
        HashMap<int, int> mp;
        mp.insert(std::make_pair(3, 5));
        mp.insert(std::make_pair(2, 1));
        mp.insert(std::make_pair(0, 7));
        auto it = mp.find(0);
        REQUIRE(it->second == 7);
        mp.erase(0);
        it = mp.find(0);
        REQUIRE(it == mp.end());
        REQUIRE(mp[2] == 1);
        mp.insert(std::make_pair(8, -4));
        std::vector<std::pair<int, int>> v;
        it = mp.begin();
        for (; it != mp.end(); ++it) {
            v.push_back({it->first, it->second});
        }
        sort(v.begin(), v.end());
        REQUIRE(v.size() == 3);
        REQUIRE(v[0] == std::pair<int, int>(2, 1));
        REQUIRE(v[1] == std::pair<int, int>(3, 5));
        REQUIRE(v[2] == std::pair<int, int>(8, -4));
        mp.clear();
        it = mp.find(3);
        REQUIRE(it == mp.end());
        mp.insert(std::make_pair(3, 3));
        it = mp.find(3);
        REQUIRE(it->second == 3);
        REQUIRE(mp.size() == 1);
    }
    {
        HashMap<int, int> mp;
        mp[0] = 1;
        auto it = mp.find(0);
        REQUIRE(it->second == 1);
        mp[0] = 2;
        it = mp.find(0);
        REQUIRE(it->second == 2);
        REQUIRE(mp[0] == 2);
        REQUIRE(mp.at(0) == 2);
        mp.erase(0);
        mp.erase(1);
        REQUIRE(mp.size() == 0);
        REQUIRE(mp.begin() == mp.end());
    }
    {
        HashMap<int, int, std::function<size_t(int)>> mp(stupid_hash);
        for (int i = 0; i < 1000; ++i) {
            mp[i] = i;
        }
        for (int i = 0; i < 1000; ++i) {
            auto it = mp.find(i);
            REQUIRE(it->second == i);
            REQUIRE(mp[i] == i);
            REQUIRE(mp.at(i) == i);
        }
        HashMap<int, int, std::function<size_t(int)>> mp2 = mp;
        for (int i = 0; i < 1000; ++i) {
            auto it = mp2.find(i);
            REQUIRE(it->second == i);
            REQUIRE(mp2[i] == i);
            REQUIRE(mp2.at(i) == i);
        }
    }
    {
        HashMap<int, int> mp;
        std::map<int, int> norm_mp;
        std::vector<int> v(1000000);
        static std::random_device rd;
        static std::mt19937 rnd{rd()};
        for (int i = 0; i < 1000000; ++i) {
            v[i] = rnd();
        }
        for (int i = 0; i < 1000000; ++i) {
            mp[v[i]] = i;
            auto it = mp.find(v[i]);
            REQUIRE(it != mp.end());
            norm_mp[v[i]] = i;
        }
        for (int i = 0; i < 1000000; ++i) {
            auto it = mp.find(v[i]);
            auto it2 = norm_mp.find(v[i]);
            if (it2 == norm_mp.end()) {
                REQUIRE(it == mp.end());
                continue;
            }
            REQUIRE(it != mp.end());
            REQUIRE(it->second == it2->second);
            REQUIRE(mp[v[i]] == it2->second);
            REQUIRE(mp.at(v[i]) == it2->second);
        }
    }
    {
        HashMap<std::string, int> mp;
        std::map<std::string, int> norm_mp;
        std::vector<std::string> v(1000);
        static std::random_device rd;
        static std::mt19937 rnd{rd()};
        for (int i = 0; i < 1000; ++i) {
            size_t sz = 1 + rnd() % 100;
            for (int j = 0; j < sz; ++j) {
                v[i] += char('a' + rnd() % 26);
            }
        }
        for (int i = 0; i < 1000; ++i) {
            mp[v[i]] = i;
            norm_mp[v[i]] = i;
        }
        for (int i = 0; i < 1000; ++i) {
            auto it = mp.find(v[i]);
            auto it2 = norm_mp.find(v[i]);
            if (it2 == norm_mp.end()) {
                REQUIRE(it == mp.end());
                continue;
            }
            REQUIRE(it != mp.end());
            REQUIRE(it->second == it2->second);
            REQUIRE(mp[v[i]] == it2->second);
            REQUIRE(mp.at(v[i]) == it2->second);
        }
        HashMap<std::string, int> mp2 = mp;
        for (int i = 0; i < 1000; ++i) {
            auto it = mp2.find(v[i]);
            auto it2 = norm_mp.find(v[i]);
            if (it2 == norm_mp.end()) {
                REQUIRE(it == mp.end());
                continue;
            }
            REQUIRE(it != mp.end());
            REQUIRE(it->second == it2->second);
            REQUIRE(mp2[v[i]] == it2->second);
            REQUIRE(mp2.at(v[i]) == it2->second);
        }
    }
}