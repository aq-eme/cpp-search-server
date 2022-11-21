#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
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
    void AddDocument(int document_id, const string& document) {
        ++document_count_;
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string &word: words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
    }
    // Возвращает топ-5 самых релевантных документов в виде пар: {id, релевантность}
    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);
        // Сортируем документы по возрастанию релевантности и id
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        // Оставляем MAX_RESULT_DOCUMENT_COUNT самых релевантных документов
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
// Содержимое раздела private: доступно только внутри методов самого класса
private:
    // обьявляем поле для хранения количества документов
    int document_count_ = 0;
    // объявляем множество для хранения "стоп-слов"
    set<string> stop_words_;
    // объявляем словарь пар строк и множеств чисел для хранения
    map<string, map<int, double>> word_to_document_freqs_;

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };


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
        return log(document_count_ * 1.0 / word_to_document_freqs_.at(word).size());
    }

    // Для каждого документа возвращает пару {релевантность, id}
    vector<Document> FindAllDocuments(const Query& query) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
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
            matched_documents.push_back({document_id, relevance});
        }
        return matched_documents;
    }

// Возвращает true, если среди слов документа (document_words) встречаются слова поискового запроса query
    /*static int MatchDocument(const DocumentContent& content, const Query& query) {
        if (query.plus_words.empty()) {
            return 0;
        }
        set<string> matched_words;
        for (const string& word : content.words) {
            if (query.minus_words.count(word) != 0) {
                return 0;
            }
            if (matched_words.count(word) != 0) {
                continue;
            }
            if (query.plus_words.count(word) != 0) {
                matched_words.insert(word);
            }
        }
        return static_cast<int>(matched_words.size());
    }
     */
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
// Считываем документы
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();
// Ищем документы по поисковому запросу
    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}