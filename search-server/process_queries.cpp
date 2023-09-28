#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) 
{
    std::vector<std::vector<Document>> documents_lists(queries.size());
    transform(execution::par, 
              queries.begin(), queries.end(),
             documents_lists.begin(), [&search_server](const std::string_view query) {
                 return search_server.FindTopDocuments(query);
             });
    return documents_lists;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::list<Document> documents;
    std::vector<std::vector<Document>> docs_lists = ProcessQueries(search_server, queries);
    for (const std::vector<Document>& docs : docs_lists) {
        documents.insert(documents.end(), docs.begin(), docs.end());
    }
    return documents;
}

