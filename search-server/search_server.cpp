#include "search_server.h"

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(string_view stop_words_view)
    : SearchServer(SplitIntoWords(stop_words_view))
{
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    if (!IsValidWord(document)) {
        throw invalid_argument("invalid document"s);
    }
    storage.emplace_back(document.data());
    const auto words = SplitIntoWordsNoStop(storage.back());

    const double inv_word_count = 1.0 / words.size();
    for (const string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        freqs_in_docs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}


vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}


const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const map<string_view, double> empty_map_;
    if (count(document_ids_.begin(), document_ids_.end(), document_id) == 0) {
        return empty_map_;
    }
    return freqs_in_docs_.at(document_id);
}

MatchReturn SearchServer::MatchDocument(string_view raw_query, int document_id) const {
    if (!IsIdCorrect(document_id)) {
        throw out_of_range("неверный id"s);
    } 
    if (!IsValidWord(raw_query)) {
        throw invalid_argument("некорректный запрос"s);
    }
    auto query = ParseQuery(raw_query, false);
    
    vector<string_view> matched_words;
    for (const string_view word : query.minus_words) {
        if (freqs_in_docs_.at(document_id).count(word)) {
            return { matched_words, documents_.at(document_id).status };
        }
    }
    for (const string_view word : query.plus_words) {
        string w {word};
        if (freqs_in_docs_.at(document_id).count(w)) {
            matched_words.push_back(word);
        }
    }
    return { matched_words, documents_.at(document_id).status };
}


void SearchServer::RemoveDocument(int document_id) {
    if (count(document_ids_.begin(), document_ids_.end(), document_id) == 0) {
        return;
    }
    document_ids_.erase(find(document_ids_.begin(), document_ids_.end(), document_id));
    documents_.erase(documents_.find(document_id));
    freqs_in_docs_.erase(freqs_in_docs_.find(document_id));
    for (auto& [word, info] : word_to_document_freqs_) {
        auto it = info.find(document_id);
        if (it != info.end()) {
            info.erase(it);
        }
    }
}



bool SearchServer::IsStopWord(const string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string_view word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

bool SearchServer::IsValidWords(const vector<string_view>& words) const {
    for (const string_view word : words) {
        if (!IsValidWord(word)) {
            return false;
        }
    }
    return true;
}

bool SearchServer::IsIdCorrect(int id) const {
    return documents_.count(id) != 0;
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    vector<string_view> words;
    for (const string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}


SearchServer::QueryWord SearchServer::ParseQueryWord(const string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + text.data() + " is invalid"s);
    }

    return { word, is_minus, IsStopWord(word) };
}


SearchServer::Query SearchServer::ParseQuery(string_view text, bool flag) const {
    Query result;
    for (const string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) { result.minus_words.push_back(query_word.data);
            }
            else {
               result.plus_words.push_back(query_word.data);
            }
        }
    }

    if (!flag) {
        std::sort(result.plus_words.begin(), result.plus_words.end());
        auto last = std::unique(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(last, result.plus_words.end());

        std::sort(result.minus_words.begin(), result.minus_words.end());
        last = std::unique(result.minus_words.begin(), result.minus_words.end());
        result.minus_words.erase(last, result.minus_words.end());
    }

    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void SearchServer::RemoveDocument(execution::sequenced_policy, int document_id) {
    if (count(document_ids_.begin(), document_ids_.end(), document_id) == 0) {
        return;
    }
    document_ids_.erase(find(document_ids_.begin(), document_ids_.end(), document_id));
    documents_.erase(documents_.find(document_id));
    freqs_in_docs_.erase(freqs_in_docs_.find(document_id));
    for (auto& [word, info] : word_to_document_freqs_) {
        auto it = info.find(document_id);
        if (it != info.end()) {
            info.erase(it);
        }
    }
}


void SearchServer::RemoveDocument(execution::parallel_policy, int document_id) {
    vector<string_view> words_to_remove(freqs_in_docs_[document_id].size());

    transform(execution::par,
        freqs_in_docs_[document_id].begin(),
        freqs_in_docs_[document_id].end(),
        words_to_remove.begin(),
        [](const auto& word_freq) { return word_freq.first; });

    for_each(execution::par,
        words_to_remove.begin(),
        words_to_remove.end(),
        [&](const string_view word) { word_to_document_freqs_[word].erase(document_id); });

    document_ids_.erase(find(document_ids_.begin(), document_ids_.end(), document_id));
    documents_.erase(document_id);
    freqs_in_docs_.erase(document_id);
}
