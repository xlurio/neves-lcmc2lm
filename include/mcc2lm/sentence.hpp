#ifndef MCC2LM_SENTENCE_HPP

#define MCC2LM_SENTENCE_HPP

#include <string>
#include <vector>
#include <mcc2lm/word.hpp>
#include <mcc2lm/constants.hpp>
#include <algorithm>
#include <mcc2lm/logger.hpp>

namespace mcc2lm
{
    class Sentence
    {
        // MCC2LM_SENTENCE
        const std::string INSERT_SENTENCE_QUERY =
            "INSERT OR IGNORE INTO MCC2LM_SENTENCE "
            "VALUES (?, ?);";

        // MCC2LM_WORD_SENTENCE_MAP
        const std::string INSERT_WORD_SENTENCE_MAP_QUERY =
            "INSERT OR IGNORE INTO MCC2LM_WORD_SENTENCE_MAP (SENTENCE_ID, WORD_ID)"
            "VALUES (?, ?);";

        // ---

        int hash;
        std::string value;
        std::vector<Word> words;

        int hash_from_unordered_words() const;

    public:
        Sentence(std::string value, std::vector<Word> words);
        int get_hash() const;
        bool ShouldPersist() const;
        void Save(sqlite3 *db);
    };

    class SentenceIterator
    {
        using difference_type = int;
        using value_type = Sentence;
        std::shared_ptr<xmlpp::DomParser> parser;
        xmlpp::Node::NodeSet curr_node_set;
        int8_t file_idx;
        int node_set_idx;

    public:
        SentenceIterator(
            int8_t file_idx, int node_set_idx);

        int8_t get_file_idx() const;
        int get_curr_node_set_idx() const;
        SentenceIterator begin() const;
        SentenceIterator end() const;
        SentenceIterator &operator++();
        Sentence operator*() const;
        bool operator!=(const SentenceIterator &rhs) const;
    };

#include <mcc2lm/sentence_value_impl.inc>
#include <mcc2lm/sentence_iterator_impl.inc>
}

#endif
