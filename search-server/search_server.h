#pragma once
#include <algorithm>
#include <numeric>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "string_processing.h"
#include "document.h"
#include <execution>
#include <string_view>
#include <deque>
#include <exception>
#include <iterator>
#include "concurrent_map.h"


const int MAX_RESULT_DOCUMENT_COUNT = 5;

const double EPSILON = 1e-6;

using MatchReturn = tuple<vector<string_view>, DocumentStatus>;

class SearchServer {
public:

    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const string& stop_words_text);

    explicit SearchServer(string_view stop_words_view);

    void AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings);

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(string_view raw_query, DocumentPredicate document_predicate) const;

    vector<Document> FindTopDocuments(string_view raw_query, DocumentStatus status) const;

    vector<Document> FindTopDocuments(string_view raw_query) const;
    
    template <typename ExecutionPolicy, typename DocumentPredicate>
    vector<Document> FindTopDocuments(ExecutionPolicy &policy, string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    vector<Document> FindTopDocuments(ExecutionPolicy &policy, string_view raw_query, DocumentStatus status) const;

    template <typename ExecutionPolicy>
    vector<Document> FindTopDocuments(ExecutionPolicy &policy, string_view raw_query) const;

    int GetDocumentCount() const;

    set<int>::const_iterator begin() const;

    set<int>::const_iterator end() const;

    MatchReturn MatchDocument(string_view raw_query, int document_id) const;
    
    template <typename ExecutionPolicy>
    MatchReturn MatchDocument(ExecutionPolicy &policy, string_view raw_query, int document_id) const;

    const map<string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(execution::sequenced_policy, int document_id);
    void RemoveDocument(execution::parallel_policy, int document_id);


private:
    struct QueryWord {
        string_view data;
        bool is_minus;
        bool is_stop;
    };

    const set<string, less<>> stop_words_;
    map<string_view, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    set<int> document_ids_;
    map<int, map<string_view, double>> freqs_in_docs_;
    deque<string> storage;

    bool IsStopWord(const string_view word) const;

    static bool IsValidWord(const string_view word);

    bool IsValidWords(const vector<string_view>& words) const;

    vector<string_view> SplitIntoWordsNoStop(string_view text) const;

    static int ComputeAverageRating(const vector<int>& ratings);

    QueryWord ParseQueryWord(const string_view text) const;

    struct Query {
        vector<string_view> plus_words;
        vector<string_view> minus_words;
    };

    bool IsIdCorrect(int id) const;

    Query ParseQuery(string_view text, bool flag) const;

    double ComputeWordInverseDocumentFreq(const string_view word) const;

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    
    template <typename ExecutionPolicy, typename DocumentPredicate>
    vector<Document> FindAllDocuments(ExecutionPolicy &policy, const Query& query, DocumentPredicate document_predicate) const;

};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw invalid_argument("Some of stop words are invalid"s);
    }
}


template <typename DocumentPredicate>
vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    map<int, double> document_to_relevance;
    for (const string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename ExecutionPolicy, typename DocumentPredicate>
    vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy &policy, const Query& query, DocumentPredicate document_predicate) const {
    if (is_same_v<decay_t<ExecutionPolicy>, execution::sequenced_policy>) {
        return FindAllDocuments(query, document_predicate);
    } else {
        ConcurrentMap<int, double> document_to_relevance(15);
        for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](string_view word) {
            if (word_to_document_freqs_.count(word) == 0) {
                return;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for_each(policy, word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(), [&](auto data) {
            const auto& document_data = documents_.at(data.first);
            if (document_predicate(data.first, document_data.status, document_data.rating)) {
                document_to_relevance[data.first].ref_to_value += data.second * inverse_document_freq;
            }});});

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap())
        {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
}

template <typename ExecutionPolicy>
MatchReturn SearchServer::MatchDocument(ExecutionPolicy &policy, string_view raw_query, int document_id) const {
    if (is_same_v<decay_t<ExecutionPolicy>, execution::sequenced_policy>) {
        return MatchDocument(raw_query, document_id);
    } else {
        if (!IsIdCorrect(document_id)) {
        throw out_of_range("неверный id"s);
        } 
        if (!IsValidWord(raw_query)) {
            throw invalid_argument("некорректный запрос"s);
        }
        auto query = ParseQuery(raw_query, true);

        vector<string_view> matched_words;
        matched_words.reserve(query.plus_words.size());

        auto is_word_present = [&](string_view word) {
            return freqs_in_docs_.at(document_id).count(word) > 0;
        };

        if (any_of(query.minus_words.begin(), query.minus_words.end(), is_word_present)) {
            return { matched_words, documents_.at(document_id).status };
        }

        auto new_end = copy_if(query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), is_word_present);
        matched_words.erase(new_end, matched_words.end());

        sort(matched_words.begin(), matched_words.end());
        auto last = unique(matched_words.begin(), matched_words.end());
        matched_words.erase(last, matched_words.end());

        return { matched_words, documents_.at(document_id).status };
        }
}

    
template <typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query, false);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename ExecutionPolicy, typename DocumentPredicate>
vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy &policy, string_view raw_query, DocumentPredicate document_predicate) const {
    if (is_same_v<decay_t<ExecutionPolicy>, execution::sequenced_policy>) {
        return FindTopDocuments(raw_query, document_predicate);
    } else {
        const auto query = ParseQuery(raw_query, false);

        auto matched_documents = FindAllDocuments(policy, query, document_predicate);

        sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
                });
            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
            return matched_documents;
        }
    }

template <typename ExecutionPolicy>
vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy &policy, string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;});
}

template <typename ExecutionPolicy>
vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy &policy, string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}
