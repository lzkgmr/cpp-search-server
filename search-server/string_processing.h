#pragma once
#include <vector>
#include <string>
#include <set>
#include <string_view>
#include <iostream>

using namespace std;

vector<string_view> SplitIntoWords(string_view text);

template <typename StringContainer>
set<string, less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string, less<>> non_empty_strings;
    for (string_view str : strings) {
        if (!str.empty()) {
            string s{str};
            non_empty_strings.insert(s);
        }
    }
    return non_empty_strings;
}
