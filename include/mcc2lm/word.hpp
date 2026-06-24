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
            "INSERT INTO MCC2LM_WORD (ID, VALUE, OCCURRENCIES) "
            "VALUES (?, ?, 1) "
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

        int next_hash_candidate(int current_hash) const
        {
            if (current_hash >= INT_MAX - 1)
            {
                return 1;
            }

            return current_hash + 1;
        }

    public:
        Word(std::vector<mcc2lm::Logogram> logograms,
             std::string value,
             std::string raw_pos_tag) : logograms(logograms),
                                        value(value)
        {
            Logger logger("Word(" + value + ")");

            logger.Debug("Initializing `Word`");

            pos_tag = PosTagFromRaw(raw_pos_tag);
            hash = hash_from_value();

            logger.Info("`Word` initialized");
        }

        int get_hash() const
        {
            return hash;
        }

        bool ShouldPersist() const
        {
            return !value.empty() && pos_tag != WordPosTag::IGNORE;
        }

        void Save(sqlite3 *db)
        {
            Logger logger("Word('" + value + "')::Save");

            logger.Debug("Saving");

            if (!ShouldPersist())
            {
                return;
            }

            run_in_immediate_transaction(
                db,
                "[Word::Save] immediate upsert",
                [this, db]()
                {
                    int persisted_hash = hash;
                    constexpr int max_probe_attempts = 2048;

                    for (int attempt = 0; attempt < max_probe_attempts; ++attempt)
                    {
                        const int changes = execute_non_query_with_changes(
                            db,
                            UPSERT_WORD_QUERY,
                            [this, persisted_hash, db](sqlite3_stmt *stmt)
                            {
                                bind_int(db, stmt, 1, persisted_hash, "[Word::Save] Failed to bind word mutation ID");
                                bind_text(db, stmt, 2, value, "[Word::Save] Failed to bind word value");
                            },
                            "[Word::Save] Failed to persist word");

                        if (changes > 0)
                        {
                            hash = persisted_hash;
                            return;
                        }

                        const std::pair<bool, std::string> existing_word = optional_text_value(
                            db,
                            GET_WORD_BY_ID_QUERY,
                            [persisted_hash, db](sqlite3_stmt *stmt)
                            {
                                bind_int(db, stmt, 1, persisted_hash, "[Word::Save] Failed to bind word ID");
                            },
                            0,
                            "[Word::Save] Failed to select word");

                        if (!existing_word.first)
                        {
                            throw DatabaseException("[Word::Save] Word upsert did not affect rows and no existing row was found");
                        }

                        if (existing_word.second == value)
                        {
                            hash = persisted_hash;
                            return;
                        }

                        persisted_hash = next_hash_candidate(persisted_hash);
                    }

                    throw DatabaseException("[Word::Save] Exhausted hash collision probe attempts for value '" + value + "'");
                });

            for (mcc2lm::Logogram &logogram : logograms)
            {
                logogram.Save(db);

                ensure_mapping_row(
                    db,
                    INSERT_LOGOGRAM_WORD_MAP_QUERY,
                    [this, &logogram, db](sqlite3_stmt *stmt)
                    {
                        bind_int(db, stmt, 1, hash, "[Word::Save] Failed to bind logogram-word map word ID");
                        bind_text(db, stmt, 2, logogram.get_id(), "[Word::Save] Failed to bind logogram-word map logogram ID");
                    },
                    "[Word::Save] logogram-word map mutation");
            }

            logger.Info("Saving");
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

        WordIterator begin() const
        {
            return WordIterator(word_node_set, 0);
        }

        WordIterator end() const
        {
            return WordIterator(word_node_set, word_node_set.size());
        }

        WordIterator &operator++()
        {
            curr_idx++;
            return *this;
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

            throw ParserException("[WordIterator::operator*] Failed to parse word node");
            return Word::MakeFromRawStrAndPosTag("", "x");
        }

        bool operator!=(const WordIterator &rhs) const
        {
            return get_curr_idx() != rhs.get_curr_idx();
        }
    };
}

#endif
