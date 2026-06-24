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
        const std::string GET_SENTENCE_BY_ID_QUERY = "SELECT ID "
                                                     "FROM MCC2LM_SENTENCE "
                                                     "WHERE ID = ?;";
        const std::string INSERT_SENTENCE_QUERY = "INSERT INTO MCC2LM_SENTENCE "
                               "VALUES (?, ?);";

        // MCC2LM_WORD_SENTENCE_MAP
        const std::string GET_WORD_SENTENCE_MAP_BY_ID_QUERY = "SELECT ID "
                                                              "FROM MCC2LM_WORD_SENTENCE_MAP "
                                                              "WHERE SENTENCE_ID = ? "
                                                              "AND WORD_ID = ?;";
        const std::string INSERT_WORD_SENTENCE_MAP_QUERY =
            "INSERT INTO MCC2LM_WORD_SENTENCE_MAP (SENTENCE_ID, WORD_ID)"
            "VALUES (?, ?);";

        // ---

        int hash;
        std::string value;
        std::vector<Word> words;

        int hash_from_unordered_words() const
        {
            std::vector<std::string> sorted_values;
            sorted_values.reserve(words.size());

            for (const Word &word : words)
            {
                sorted_values.push_back(word.get_value());
            }

            std::sort(sorted_values.begin(), sorted_values.end());

            std::string canonical;
            for (const std::string &word_value : sorted_values)
            {
                canonical.push_back('\x1f');
                canonical += word_value;
            }

            const std::size_t raw_hash = std::hash<std::string>{}(canonical);
            return static_cast<int>(raw_hash % INT_MAX);
        }

    public:
        Sentence(std::string value, std::vector<Word> words) : value(value),
                                                               words(words)
        {
            Logger logger("Sentence('" + value + "')");

            hash = hash_from_unordered_words();

            logger.Info("`Sentence` initialized");
        }

        int get_hash() const
        {
            return hash;
        }

        void Save(sqlite3 *db)
        {
            Logger logger("Sentence('" + value + "')::Save");

            logger.Debug("Saving");

            if (words.empty())
            {
                return;
            }

            const bool already_exists = row_exists(
                db,
                GET_SENTENCE_BY_ID_QUERY,
                [this, db](sqlite3_stmt *stmt)
                {
                    bind_int(db, stmt, 1, hash, "[Sentence::Save] Failed to bind sentence ID");
                },
                "[Sentence::Save] Failed to select sentence");

            if (!already_exists)
            {
                execute_non_query(
                    db,
                    INSERT_SENTENCE_QUERY,
                    [this, db](sqlite3_stmt *stmt)
                    {
                        bind_int(db, stmt, 1, hash, "[Sentence::Save] Failed to bind sentence insert ID");
                        bind_text(db, stmt, 2, value, "[Sentence::Save] Failed to bind sentence value");
                    },
                    "[Sentence::Save] Failed to insert sentence");
            }

            for (Word &word : words)
            {
                word.Save(db);

                ensure_mapping_row(
                    db,
                    GET_WORD_SENTENCE_MAP_BY_ID_QUERY,
                    INSERT_WORD_SENTENCE_MAP_QUERY,
                    [this, &word, db](sqlite3_stmt *stmt)
                    {
                        bind_int(db, stmt, 1, hash, "[Sentence::Save] Failed to bind sentence-word map sentence ID");
                        bind_int(db, stmt, 2, word.get_hash(), "[Sentence::Save] Failed to bind sentence-word map word ID");
                    },
                    "[Sentence::Save] sentence-word map mutation");
            }

            logger.Info("Saved");
        }
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
            int8_t file_idx, int node_set_idx) : file_idx(file_idx),
                                                 node_set_idx(node_set_idx)
        {

            if (file_idx < 0 || file_idx >= static_cast<int8_t>(LCMC_FILENAMES.size()))
            {
                return;
            }

            Logger logger("SentenceIterator(" + LCMC_FILENAMES.at(file_idx) + ")");

            logger.Debug("Initializing `SentenceIterator`");

            parser = std::make_shared<xmlpp::DomParser>();
            parser->parse_file(LCMC_BASEDIR + "/" + LCMC_FILENAMES.at(file_idx));
            if (!parser->get_document() || !parser->get_document()->get_root_node())
            {
                throw ParserException("[SentenceIterator] Failed to parse sentence XML");
                return;
            }

            curr_node_set = parser->get_document()->get_root_node()->find("//s");

            logger.Info("`SentenceIterator` initialized");
        }

        int8_t get_file_idx() const
        {
            return file_idx;
        }

        int get_curr_node_set_idx() const
        {
            return node_set_idx;
        }

        SentenceIterator begin() const
        {
            SentenceIterator iterator(file_idx, 0);
            if (!iterator.curr_node_set.empty())
            {
                return iterator;
            }

            return ++iterator;
        }

        SentenceIterator end() const
        {
            return SentenceIterator(file_idx, curr_node_set.size());
        }

        SentenceIterator &operator++()
        {
            node_set_idx++;
            return *this;
        }

        Sentence operator*() const
        {
            Logger logger("SentenceIterator(" + LCMC_FILENAMES.at(file_idx) + ")::operator*");

            logger.Debug("Instantiating `Sequence` for index {}", node_set_idx);

            xmlpp::Node *sentence_node = curr_node_set.at(node_set_idx);
            WordIterator iterator(sentence_node->find(".//w"), 0);
            std::vector<Word> word_seq;
            std::string sentence_value;

            for (Word word_node : iterator)
            {
                if (!sentence_value.empty())
                {
                    sentence_value += " ";
                }

                sentence_value += word_node.get_value();
                word_seq.push_back(word_node);
            }

            if (!word_seq.empty())
            {
                return Sentence(sentence_value, word_seq);
            }

            throw ParserException("[SentenceIterator::operator*] Sentence without words");
            return Sentence("", {});
        }

        bool operator!=(const SentenceIterator &rhs) const
        {
            return get_file_idx() != rhs.get_file_idx() ||
                   get_curr_node_set_idx() != rhs.get_curr_node_set_idx();
        }
    };
}

#endif
