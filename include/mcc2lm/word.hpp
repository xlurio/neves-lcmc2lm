#ifndef MCC2LM_WORD_HPP

#define MCC2LM_WORD_HPP

#include <string>
#include <vector>
#include <mcc2lm/logogram.hpp>
#include <mcc2lm/exceptions.hpp>
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
        const std::string INSERT_WORD_QUERY = "INSERT INTO MCC2LM_WORD (ID, VALUE, OCCURRENCIES) "
                                              "VALUES (?, ?, 1);";
        const std::string UPDATE_WORD_QUERY = "UPDATE MCC2LM_WORD "
                                              "SET OCCURRENCIES = OCCURRENCIES + 1 "
                                              "WHERE ID = ?;";

        // MCC2LM_LOGOGRAM_WORD_MAP
        const std::string GET_LOGOGRAM_WORD_MAP_BY_ID_QUERY = "SELECT ID "
                                                              "FROM MCC2LM_LOGOGRAM_WORD_MAP "
                                                              "WHERE WORD_ID = ? "
                                                              "AND LOGOGRAM_ID = ?;";
        const std::string INSERT_LOGOGRAM_WORD_MAP_QUERY =
            "INSERT INTO MCC2LM_LOGOGRAM_WORD_MAP (WORD_ID, LOGOGRAM_ID)"
            "VALUES (?, ?);";

        // ---

        int hash;
        std::vector<mcc2lm::Logogram> logograms;
        std::string value;
        WordPosTag pos_tag;

        WordPosTag PosTagFromRaw(std::string raw_pos_tag)
        {
            if (raw_pos_tag == "a" || raw_pos_tag == "ad" || raw_pos_tag == "ag" ||
                raw_pos_tag == "an" || raw_pos_tag == "b" || raw_pos_tag == "bg")
            {
                return WordPosTag::ADJECTIVE;
            }
            else if (raw_pos_tag == "d" || raw_pos_tag == "dg")
            {
                return WordPosTag::ADVERB;
            }
            else if (raw_pos_tag == "c" || raw_pos_tag == "cg")
            {
                return WordPosTag::CONJUCTION;
            }
            else if (raw_pos_tag == "e")
            {
                return WordPosTag::INTERJECTION;
            }
            else if (raw_pos_tag == "y" || raw_pos_tag == "yg" || raw_pos_tag == "z" ||
                     raw_pos_tag == "zg")
            {
                return WordPosTag::MODAL_PARTICLE;
            }
            else if (raw_pos_tag == "m" || raw_pos_tag == "mg")
            {
                return WordPosTag::NUMERAL;
            }
            else if (raw_pos_tag == "o")
            {
                return WordPosTag::ONOMATOPOEIA;
            }
            else if (raw_pos_tag == "p" || raw_pos_tag == "pg")
            {
                return WordPosTag::PREPOSITION;
            }
            else if (raw_pos_tag == "r" || raw_pos_tag == "rg")
            {
                return WordPosTag::PRONOUN;
            }
            else if (raw_pos_tag == "u" || raw_pos_tag == "v" || raw_pos_tag == "vd" ||
                     raw_pos_tag == "vg" || raw_pos_tag == "vn")
            {
                return WordPosTag::VERB;
            }
            else if (raw_pos_tag == "x")
            {
                return WordPosTag::UNCLASSIFIED;
            }
            else if (raw_pos_tag == "ew" || raw_pos_tag == "h" || raw_pos_tag == "k" ||
                     raw_pos_tag == "w")
            {
                return WordPosTag::IGNORE;
            }

            return WordPosTag::NOUN;
        }

        int hash_from_value() const
        {
            const std::size_t raw_hash = std::hash<std::string>{}(value);
            return static_cast<int>(raw_hash % INT_MAX);
        }

    public:
        Word(std::vector<mcc2lm::Logogram> logograms,
             std::string value,
             std::string raw_pos_tag) : logograms(logograms),
                                        value(value)
        {
            pos_tag = PosTagFromRaw(raw_pos_tag);
            hash = hash_from_value();
        }

        int get_hash() const
        {
            return hash;
        }

        void save(sqlite3 *db)
        {
            if (value.empty() || pos_tag == WordPosTag::IGNORE)
            {
                return;
            }

            const std::pair<bool, std::string> existing_word = optional_text_value(
                db,
                GET_WORD_BY_ID_QUERY,
                [this, db](sqlite3_stmt *stmt)
                {
                    bind_int(db, stmt, 1, hash, "[Word::save] Failed to bind word ID");
                },
                0,
                "[Word::save] Failed to select word");

            const bool already_exists = existing_word.first;
            const std::string &existing_value = existing_word.second;
            if (already_exists && existing_value != value)
            {
                throw DatabaseException("[Word::save] Hash collision for word ID " + std::to_string(hash));
            }

            if (already_exists)
            {
                execute_non_query(
                    db,
                    UPDATE_WORD_QUERY,
                    [this, db](sqlite3_stmt *stmt)
                    {
                        bind_int(db, stmt, 1, hash, "[Word::save] Failed to bind word mutation ID");
                    },
                    "[Word::save] Failed to persist word");
            }
            else
            {
                execute_non_query(
                    db,
                    INSERT_WORD_QUERY,
                    [this, db](sqlite3_stmt *stmt)
                    {
                        bind_int(db, stmt, 1, hash, "[Word::save] Failed to bind word mutation ID");
                        bind_text(db, stmt, 2, value, "[Word::save] Failed to bind word value");
                    },
                    "[Word::save] Failed to persist word");
            }

            for (mcc2lm::Logogram &logogram : logograms)
            {
                logogram.save(db);

                ensure_mapping_row(
                    db,
                    GET_LOGOGRAM_WORD_MAP_BY_ID_QUERY,
                    INSERT_LOGOGRAM_WORD_MAP_QUERY,
                    [this, &logogram, db](sqlite3_stmt *stmt)
                    {
                        bind_int(db, stmt, 1, hash, "[Word::save] Failed to bind logogram-word map word ID");
                        bind_text(db, stmt, 2, logogram.get_id(), "[Word::save] Failed to bind logogram-word map logogram ID");
                    },
                    "[Word::save] logogram-word map mutation");
            }
        }

        const std::string &get_value() const
        {
            return value;
        }

        static Word MakeFromRawStrAndPosTag(
            std::string raw_str, std::string raw_pos_tag)
        {
            std::vector<mcc2lm::Logogram> logogram_seq;
            mcc2lm::LogogramIterator iterator(raw_str, 0);

            for (mcc2lm::Logogram logograph : iterator)
            {
                logogram_seq.push_back(logograph);
            }

            return Word(
                logogram_seq,
                raw_str,
                raw_pos_tag);
        }
    };

    class WordIterator
    {
        int curr_idx;
        xmlpp::Node::NodeSet word_node_set;

    public:
        WordIterator(
            xmlpp::Node::NodeSet word_node_set,
            int curr_idx) : word_node_set(word_node_set),
                            curr_idx(curr_idx) {}

        int get_curr_idx() const
        {
            return curr_idx;
        }

        WordIterator begin()
        {
            return WordIterator(word_node_set, 0);
        }

        WordIterator end()
        {
            return WordIterator(word_node_set, word_node_set.size());
        }

        WordIterator operator++() const
        {
            return WordIterator(word_node_set, curr_idx + 1);
        }

        Word operator*() const
        {
            xmlpp::Node *word_node = word_node_set.at(curr_idx);

            if (xmlpp::Element *element = dynamic_cast<xmlpp::Element *>(word_node))
            {
                if (element->has_child_text())
                {
                    if (xmlpp::TextNode *text_node = element->get_first_child_text())
                    {
                        return Word::MakeFromRawStrAndPosTag(
                            text_node->get_content(),
                            element->get_attribute_value("POS"));
                    }
                }
            }

            throw DatabaseException("[WordIterator::operator*] Failed to parse word node");
            return Word::MakeFromRawStrAndPosTag("", "x");
        }

        bool operator!=(const WordIterator &rhs) const
        {
            return get_curr_idx() != rhs.get_curr_idx();
        }
    };
}

#endif
