#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    set<int> ids_to_delete;
    for (const int doc_id1 : search_server) {
        const auto word_freq1 = search_server.GetWordFrequencies(doc_id1);
        if (ids_to_delete.count(doc_id1) > 0) {
            continue;
        }
        for (const int doc_id2 : search_server) {
            if (ids_to_delete.count(doc_id2) > 0 || doc_id1 == doc_id2) {
                continue;
            }
            const auto word_freq2 = search_server.GetWordFrequencies(doc_id2);
            if (word_freq1.size() != word_freq2.size()) {
                continue;
            } else {
                bool flag = true;
                for (int i = 0; i < word_freq1.size(); ++i) {
                    if (next(word_freq1.begin(), i)->first != (next(word_freq2.begin(), i)->first)) {
                        flag = false;
                    }
                }
                if (flag) {
                    cout << "Found duplicate document id "s << doc_id2 << endl;
                    ids_to_delete.insert(doc_id2);
                }
            }
        }
    }
}
