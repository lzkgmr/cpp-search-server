#pragma once
#include "search_server.h"
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate);

    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status);
    
    vector<Document> AddFindRequest(const string& raw_query);
    
    int GetNoResultRequests() const;
    
private:
    struct QueryResult {
        bool isEmpty;
        vector<Document> documents;
    };
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& server_;
};

template <typename DocumentPredicate>
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
    vector<Document> result = server_.FindTopDocuments(raw_query, document_predicate);
    requests_.push_back({result.empty(), result});
    if (requests_.size() > min_in_day_) {
        requests_.pop_front();
    }
    return result;
}
