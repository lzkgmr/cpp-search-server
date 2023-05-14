#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <iomanip>

using namespace std;
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

void TestAddDocument() {

    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    
    {
        SearchServer server;
        const auto found_docs = server.FindTopDocuments("city"s);
        ASSERT(found_docs.empty());
    }
}

void TestExcludeMinusWordsFromFinalList() {
    const int doc_id_1 = 2;
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = {1, 2, 3};

    const int doc_id_2 = 6;
    const string content_2 = "     "s;
    const vector<int> ratings_2 = {3, -6, 5};

    const int doc_id_3 = 98;
    const string content_3 = "dog"s;
    const vector<int> ratings_3 = {0, 5, 1};

    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        const auto found_docs_1 = server.FindTopDocuments("city dog -cat"s);
        ASSERT_EQUAL(found_docs_1.size(), 1);
        const Document& doc0_1 = found_docs_1[0];
        ASSERT_EQUAL(doc0_1.id, doc_id_3);
    }

}

void TestMatchingOfDocuments() {
    const int doc_id_1 = 2;
    const string content_1 = "cat with in the city"s;
    const vector<int> ratings_1 = {1, 2, 3};

    const int doc_id_2 = 6;
    const string content_2 = "     "s;
    const vector<int> ratings_2 = {3, -6, 5};

    const int doc_id_3 = 98;
    const string content_3 = "dog fish"s;
    const vector<int> ratings_3 = {0, 5, 1};

    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);

        tuple<vector<string>, DocumentStatus> t = server.MatchDocument("cat with tail"s, doc_id_1);
        vector<string> words = get<0>(t);
        ASSERT(words[0] == "cat"s && words[1] == "with"s);

        words.clear();
        t = server.MatchDocument("cat with tail"s, doc_id_2);
        words = get<0>(t);
        ASSERT(words.empty());

        words.clear();
        t = server.MatchDocument("cat -city"s, doc_id_1);
        words = get<0>(t);
        ASSERT(words.empty());

    }
}

void TestSortingWithRelevance() {
    const int doc_id_1 = 2;
    const string content_1 = "белый кот и модный ошейник"s;
    const vector<int> ratings_1 = {1, 2, 3};

    const int doc_id_2 = 6;
    const string content_2 = "     "s;
    const vector<int> ratings_2 = {3, -6, 5};

    const int doc_id_3 = 98;
    const string content_3 = "пушистый кот пушистый хвост"s;
    const vector<int> ratings_3 = {0, 5, 1};

    const int doc_id_4 = 1;
    const string content_4 = "ухоженный пёс выразительные глаза"s;
    const vector<int> ratings_4 = {6, 2, -8};

    const int doc_id_5 = 1;
    const string content_5 = "Разместите код остальных тестов здесь"s;
    const vector<int> ratings_5 = {6, 2, -8};

    SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        server.AddDocument(doc_id_4, content_4, DocumentStatus::ACTUAL, ratings_4);
        server.AddDocument(doc_id_5, content_5, DocumentStatus::ACTUAL, ratings_5);

        vector<Document> docs = server.FindTopDocuments("пушистый ухоженный кот"s);
        ASSERT_EQUAL(docs.size(), 3);
        ASSERT(docs[0].id == doc_id_3 && docs[1].id == doc_id_4 && docs[2].id == doc_id_1);

}

void TestCalculatingRating() {
    const int doc_id_1 = 2;
    const string content_1 = "белый кот и модный ошейник"s;
    const vector<int> ratings_1 = {2, 8, -3};

    const int doc_id_2 = 6;
    const string content_2 = "     "s;
    const vector<int> ratings_2 = {3, 7, 2, 7};

    const int doc_id_3 = 98;
    const string content_3 = "пушистый кот пушистый хвост"s;
    const vector<int> ratings_3 = {3, 7, 2, 7};

    const int doc_id_4 = 1;
    const string content_4 = "ухоженный пёс выразительные глаза"s;
    const vector<int> ratings_4 = {4, 5, -12, 2, 1};

    SearchServer server;
    {
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        server.AddDocument(doc_id_4, content_4, DocumentStatus::ACTUAL, ratings_4);

        vector<Document> docs = server.FindTopDocuments("пушистый ухоженный кот "s);
        ASSERT_EQUAL(docs.size(), 3);
        ASSERT(docs[0].rating == 4 && docs[1].rating == 0 && docs[2].rating == 2);
    }
}
void TestCalculatingRelevance() {
    const int doc_id_1 = 2;
    const string content_1 = "белый кот и модный ошейник"s;
    const vector<int> ratings_1 = {2, 8, -3};

    const int doc_id_2 = 6;
    const string content_2 = "     "s;
    const vector<int> ratings_2 = {3, 7, 2, 7};

    const int doc_id_3 = 98;
    const string content_3 = "пушистый кот пушистый хвост"s;
    const vector<int> ratings_3 = {3, 7, 2, 7};

    const int doc_id_4 = 1;
    const string content_4 = "ухоженный пёс выразительные глаза"s;
    const vector<int> ratings_4 = {4, 5, -12, 2, 1};

    SearchServer server;
    {
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        server.AddDocument(doc_id_4, content_4, DocumentStatus::ACTUAL, ratings_4);

        vector<Document> docs = server.FindTopDocuments("пушистый ухоженный кот "s);
        ASSERT_EQUAL(docs.size(), 3);
        ASSERT((abs(docs[0].relevance - 0.866434) < 1e-6) && (abs(docs[1].relevance - 0.346574) < 1e-6)  && (abs(docs[2].relevance - 0.138629) < 1e-6));
    }
}

void TestFilterDocsWithPredicate() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    
    SearchServer server;
    {
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(3, "dog in the city"s, DocumentStatus::BANNED, {3, 0, -4});
        vector<Document> t = server.FindTopDocuments(content, [](int document_id, [[maybe_unused]] DocumentStatus status, [[maybe_unused]] int rating) { return document_id % 2 == 0; });
        ASSERT(!t.empty());
        ASSERT_EQUAL(t[0].id % 2,0);
    }
}

void TestSearchWithStatus() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    
    SearchServer server;
    {
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(3, "dog in the city"s, DocumentStatus::BANNED, {3, 0, -4});
        vector<Document> t = server.FindTopDocuments(content, [](int document_id, [[maybe_unused]] DocumentStatus status, [[maybe_unused]] int rating) { return status == DocumentStatus::ACTUAL; });
        ASSERT(!t.empty());
        ASSERT_EQUAL(t[0].id, doc_id);
    }
}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWordsFromFinalList);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMatchingOfDocuments);
    RUN_TEST(TestSortingWithRelevance);
    RUN_TEST(TestCalculatingRating);
    RUN_TEST(TestCalculatingRelevance);
    RUN_TEST(TestFilterDocsWithPredicate);
    RUN_TEST(TestSearchWithStatus);
    // Не забудьте вызывать остальные тесты здесь
}
