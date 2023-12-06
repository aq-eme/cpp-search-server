#pragma once
#include "document.h"
#include "read_input_functions.h"

#include <set>

std::vector<std::string> SplitIntoWords(std::basic_string_view<char> text);

std::vector<std::string_view> SplitIntoWordsView(std::string_view str);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (std::basic_string_view<char> str : strings) {
        if (!str.empty()) {
            non_empty_strings.emplace(str);
        }
    }
    return non_empty_strings;
}






