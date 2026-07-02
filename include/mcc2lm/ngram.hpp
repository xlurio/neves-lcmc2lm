#ifndef MCC2LM_NGRAM_HPP

#define MCC2LM_NGRAM_HPP

#include <string>
#include <vector>
#include <mcc2lm/word.hpp>
#include <mcc2lm/sql.hpp>
#include <mcc2lm/logger.hpp>
#include <climits>

namespace mcc2lm
{
    class Ngram
    {
        // MCC2LM_NGRAM
        const std::string UPSERT_NGRAM_QUERY =
            "INSERT INTO MCC2LM_NGRAM (ID, VALUE, N_WORDS, OCCURRENCIES) "
            "VALUES (?, ?, ?, 1) "
            "ON CONFLICT(ID) DO UPDATE SET "
            "OCCURRENCIES = MCC2LM_NGRAM.OCCURRENCIES + 1 "
            "WHERE MCC2LM_NGRAM.VALUE = excluded.VALUE;";

        const std::string GET_NGRAM_VALUE_QUERY =
            "SELECT VALUE FROM MCC2LM_NGRAM WHERE ID = ?;";

        const std::string INSERT_WORD_NGRAM_MAP_QUERY =
            "INSERT OR IGNORE INTO MCC2LM_WORD_NGRAM_MAP (NGRAM_ID, WORD_ID) "
            "VALUES (?, ?);";

        // ---

        int hash;
        std::string value;
        int n_words;
        std::vector<int> component_word_hashes;

        int hash_from_value() const;
        int next_hash_candidate(int current_hash) const;

    public:
        // Static query for accessing from Sentence::Save()
        static constexpr const char *WORD_NGRAM_MAP_INSERT_QUERY =
            "INSERT OR IGNORE INTO MCC2LM_WORD_NGRAM_MAP (NGRAM_ID, WORD_ID) VALUES (?, ?);";

        Ngram(const std::vector<Word> &words, int n_words_count);

        int get_hash() const;
        std::string get_value() const;
        int get_n_words() const;
        bool ShouldPersist() const;
        void Save(sqlite3 *db);
    };

#include <mcc2lm/impl/ngram_impl.inc>
}

#endif
