#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    set<int> ids_to_delete;
    map<int, set<string>> docs_words;
    for (const int doc_id : search_server) {
        const auto& words_freqs = search_server.GetWordFrequencies(doc_id); 
        set<string> words; 
        for (const auto& [word, freq] : words_freqs) {
            words.insert(word);
        }
        if (count_if(docs_words.begin(), docs_words.end(), [words](const auto& x) {
            return x.second == words;
        })) {
            cout << "Found duplicate document id "s << doc_id << endl;
            ids_to_delete.insert(doc_id);
        } else {
            docs_words[doc_id] = words;
        }
        
    }
    for (int id : ids_to_delete) {
        search_server.RemoveDocument(id);
    }
}
