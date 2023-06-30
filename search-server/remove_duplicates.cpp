#include "remove_duplicates.h"
#include "search_server.h"
#include <iostream>



void RemoveDuplicates(SearchServer& search_server) {
    using namespace std::literals;
    std::set<int> duplicate_document_id;
    std::set<std::set<std::string>> document_words;

    for (auto document_id : search_server)
    {
        const auto& document_to_word_freqs_ = search_server.GetWordFrequencies(document_id);
        std::set<std::string> words;

        for (const auto& [word, freq] : document_to_word_freqs_) {
            words.insert(word);
        }

        if (document_words.count(words)) {
            duplicate_document_id.insert(document_id);
        }
        else {
            document_words.insert(words);
        }
    }

    for (const auto document_id : duplicate_document_id) {
        std::cout << "Found duplicate document id " << document_id << std::endl;

        search_server.RemoveDocument(document_id);
    }
}