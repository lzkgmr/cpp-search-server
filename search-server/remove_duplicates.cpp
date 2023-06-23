#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    set<int> ids_to_delete;
    map<int, set<string>> docs_words;
    for (const int doc_id : search_server) {
        const auto& words = search_server.GetWordFrequencies(doc_id);
        for (const auto& [word, freq] : words) {
            docs_words[doc_id].insert(word);
        }
    }
    int i = 0; //счетчик позиции пары в docs_words
    for (const auto& [id, words] : docs_words) {
        auto it = find_if(next(docs_words.begin(), i), docs_words.end(), [id, words](const pair<int, set<string>>& x) {
            return id != x.first && words == x.second;
        });
        if (it != docs_words.end() && ids_to_delete.count(it->first) == 0) {
            cout << "Found duplicate document id "s << it->first << endl;
            ids_to_delete.insert(it->first);
        }
        ++i;
    }
    for (int id : ids_to_delete) {
        search_server.RemoveDocument(id);
    }
}
