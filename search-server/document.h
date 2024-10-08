#pragma once
#include <iostream>
#include <vector>

using namespace std;

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

struct DocumentData {
    int rating;
    DocumentStatus status;
};

ostream& operator<<(ostream& out, const Document& document);

void PrintDocument(const Document& document);

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status);