#ifndef MCC2LM_WORD_HPP

#define MCC2LM_WORD_HPP

#include <string>
#include <vector>
#include <mcc2lm/logogram.hpp>
#include <mcc2lm/exceptions.hpp>
#include <mcc2lm/logger.hpp>
#include <libxml++/libxml++.h>
#include <climits>

namespace mcc2lm
{
    enum WordPosTag
    {
        ADJECTIVE,      // a, ad, ag, an, b, bg
        ADVERB,         // d, dg
        CONJUCTION,     // c, cg
        INTERJECTION,   // e
        MODAL_PARTICLE, // y, yg, z, zg
        NOUN,           // *
        NUMERAL,        // m, mg
        ONOMATOPOEIA,   // o
        PREPOSITION,    // p, pg
        PRONOUN,        // r, rg
        VERB,           // u, v, vd, vg, vn
        UNCLASSIFIED,   // x
        IGNORE          // ew, h, k, w
    };

    class Word
    {
        // MCC2LM_WORD
        const std::string GET_WORD_BY_ID_QUERY = "SELECT VALUE "
                                                 "FROM MCC2LM_WORD "
                                                 "WHERE ID = ?;";
        const std::string UPSERT_WORD_QUERY =
            "INSERT INTO MCC2LM_WORD "
            "(ID, VALUE, POS_TAG, OCCURRENCIES) VALUES (?, ?, ?, 1) "
            "ON CONFLICT(ID) DO UPDATE SET OCCURRENCIES = MCC2LM_WORD.OCCURRENCIES + 1 "
            "WHERE MCC2LM_WORD.VALUE = excluded.VALUE;";

        // MCC2LM_LOGOGRAM_WORD_MAP
        const std::string INSERT_LOGOGRAM_WORD_MAP_QUERY =
            "INSERT OR IGNORE INTO MCC2LM_LOGOGRAM_WORD_MAP (WORD_ID, LOGOGRAM_ID)"
            "VALUES (?, ?);";

        // ---

        int hash;
        std::vector<mcc2lm::Logogram> logograms;
        std::string value;
        WordPosTag pos_tag;

        WordPosTag PosTagFromRaw(std::string raw_pos_tag);
        int hash_from_value() const;
        int next_hash_candidate(int current_hash) const;

    public:
        Word(std::vector<mcc2lm::Logogram> logograms,
             std::string value,
             std::string raw_pos_tag);

        int get_hash() const;
        std::string get_verbose_pos_tag() const;
        bool ShouldPersist() const;
        void Save(sqlite3 *db);
        const std::string &get_value() const;

        static Word MakeFromRawStrAndPosTag(std::string raw_str, std::string raw_pos_tag);
    };

    class WordIterator
    {
        int curr_idx;
        xmlpp::Node::NodeSet word_node_set;

    public:
        WordIterator(
            xmlpp::Node::NodeSet word_node_set,
            int curr_idx);

        int get_curr_idx() const;
        WordIterator begin() const;
        WordIterator end() const;
        WordIterator &operator++();
        Word operator*() const;
        bool operator!=(const WordIterator &rhs) const;
    };

#include <mcc2lm/impl/word_word_impl.inc>
#include <mcc2lm/impl/word_save_impl.inc>
#include <mcc2lm/impl/word_iterator_impl.inc>
}

#endif
