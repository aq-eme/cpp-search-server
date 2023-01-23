#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <map>
#include <cmath>


using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}
// SplitIntoWords принимает на вход строку и возвращает вектор слов.
vector<string> SplitIntoWords(const string& text) {
    vector<string> words; // объявляем вектор строк для хранения слов
    string word; // объявляем строку для хранения слова
    // начинаем цикл от 0 до размера строки (проходим посимвольно по всей строке)
    for (const char c : text) {
        // условие на выполнение блока: символ это пробел
        if (c == ' ') {
            if (!word.empty()) {
                // добавляем слово в конец вектора слов
                words.push_back(word);
                // присваиваем переменной пустую строку, т.е. начинаем новое слово
                word.clear();
            }
            // условие на выполнение блока (когда все условия выше - не верны): символ это не пробел (обратное условие)
        } else {
            // добавляем символ к строке. Этим блоком мы собираем слово в переменной word
            word += c;
        }
    }
    // добавляем последнее слово word в конец вектора words
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

// Объявляем перечислимый тип
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

// определяем класс поискового сервера
class SearchServer {
// Содержимое раздела public: доступно для вызова из кода вне класса
public:


    // определяем метод для разбиения строки на множество "стоп-слов" принимает константную ссылку на строку text
    void SetStopWords(const string& text) {
        // проходим по всем словам вектора возвращённого функцией SplitIntoWords
        for (const string& word : SplitIntoWords(text)) {
            // добавляем слово во множество слов
            stop_words_.insert(word);
        }
    }
    // добавление документа в базу данных сервера
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        ++document_count_;
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string &word: words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status // сохраняем статум для документа по его document_id
        });
    }
    // Возвращает топ-5 самых релевантных документов в виде пар: {id, релевантность}
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);


        // Сортируем документы по возрастанию релевантности и id
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        // Оставляем MAX_RESULT_DOCUMENT_COUNT самых релевантных документов
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

// Содержимое раздела private: доступно только внутри методов самого класса
private:
    // Структура для хранения внутренних данных о документе(рейтинг и статус)
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    // обьявляем поле для хранения количества документов
    int document_count_ = 0;
    // объявляем множество для хранения "стоп-слов"
    set<string> stop_words_;
    // объявляем словарь пар строк и множеств чисел для хранения
    map<string, map<int, double>> word_to_document_freqs_;
    // обьявляем словарь id документа и значением — его рейтинг.
    map<int, DocumentData> documents_;

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        // static_cast позволяет привести значение к типу int без использования дополнительной переменной
        return rating_sum / static_cast<int>(ratings.size());
    }

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        // проходим по всем словам из текста и проверяем, что их нет в списке стоп-слов
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        // вернём результат без стоп-слов
        return words;
    }
    // Сравнение первого символа с '-' и убрать этот минус вызовом .substr(1), а затем проверить результат по словарю стоп-слов.
    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Слово не должно быть пустым
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(documents_.size() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    // Для каждого документа возвращает пару {релевантность, id}
    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
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

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
// Считываем документы
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        const string document = ReadLine();

        int status_raw;
        cin >> status_raw;

        int ratings_size;
        cin >> ratings_size;

        // создали вектор размера ratings_size из нулей
        vector<int> ratings(ratings_size, 0);

        // считали каждый элемент с помощью ссылки
        for (int& rating : ratings) {
            cin >> rating;
        }
        search_server.AddDocument(document_id, document, static_cast<DocumentStatus>(status_raw), ratings);
        ReadLine();
    }

    return search_server;
}
void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}