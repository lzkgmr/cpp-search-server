#include "request_queue.h"


RequestQueue::RequestQueue(const SearchServer& search_server) 
    : server_(search_server)         
{
}

template <typename DocumentPredicate>
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
    vector<Document> result = server_.FindTopDocuments(raw_query, document_predicate);
    requests_.push_back({result.empty(), result});
    if (requests_.size() > min_in_day_) {
        requests_.pop_front();
    }
    return result;
}
    
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}
    
vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}
    
int RequestQueue::GetNoResultRequests() const {
    return count_if(requests_.begin(), requests_.end(), [](const QueryResult& request) {
        return request.isEmpty;
    });
}
