#ifndef MCC2LM_LEXOGRAM_HPP

#define MCC2LM_LEXOGRAM_HPP

#include <string>
#include <mcc2lm/sql.hpp>
#include <cstdint>

namespace mcc2lm
{
    class Logogram
    {
        // MCC2LM_LOGOGRAM
        const std::string GET_LOGOGRAM_BY_ID_QUERY = "SELECT ID "
                                                     "FROM MCC2LM_LOGOGRAM "
                                                     "WHERE ID = ?;";
        const std::string INSERT_LOGOGRAM_QUERY = "INSERT INTO MCC2LM_LOGOGRAM (ID, OCCURRENCIES) "
                                                  "VALUES (?, 1);";
        const std::string UPDATE_LOGOGRAM_QUERY = "UPDATE MCC2LM_LOGOGRAM "
                                                  "SET OCCURRENCIES = OCCURRENCIES + 1 "
                                                  "WHERE ID = ?;";

        // ---

        std::string logogram;

    public:
        Logogram(std::string logogram) : logogram(logogram) {}

        const std::string &get_id() const
        {
            return logogram;
        }

        void save(sqlite3 *db)
        {
            if (logogram.empty())
            {
                return;
            }

            const bool already_exists = row_exists(
                db,
                GET_LOGOGRAM_BY_ID_QUERY,
                [this, db](sqlite3_stmt *stmt)
                {
                    bind_text(db, stmt, 1, logogram, "[Logogram::save] Failed to bind logogram ID");
                },
                "[Logogram::save] Failed to select logogram");

            sqlite3_stmt *mutation_stmt = nullptr;
            const std::string &query = already_exists ? UPDATE_LOGOGRAM_QUERY : INSERT_LOGOGRAM_QUERY;
            (void)mutation_stmt;

            execute_non_query(
                db,
                query,
                [this, db](sqlite3_stmt *stmt)
                {
                    bind_text(db, stmt, 1, logogram, "[Logogram::save] Failed to bind logogram mutation ID");
                },
                "[Logogram::save] Failed to persist logogram");
        }
    };

    class LogogramIterator
    {
        int curr_idx;
        std::string raw_str;

    public:
        LogogramIterator(std::string raw_str, int curr_idx) : raw_str(raw_str),
                                                                  curr_idx(curr_idx) {}

        int get_curr_idx() const
        {
            return curr_idx;
        }

        LogogramIterator begin()
        {
            return LogogramIterator(raw_str, 0);
        }

        LogogramIterator end()
        {
            return LogogramIterator(raw_str, raw_str.size());
        }

        int get_char_size() const
        {
            char first_char = raw_str.at(curr_idx);

            int8_t curr_bit = 0;
            int char_size = 0;
            uint8_t shift_offset = 7;

            do
            {
                curr_bit = first_char >> shift_offset;
                char_size++;
                shift_offset--;
            } while (curr_bit != 0);

            return char_size;
        }

        LogogramIterator operator++() const
        {
            return LogogramIterator(raw_str, curr_idx + get_char_size());
        }

        Logogram operator*() const
        {
            return Logogram(raw_str.substr(curr_idx, get_char_size()));
        }

        bool operator!=(const LogogramIterator &rhs) const
        {
            return get_curr_idx() != rhs.get_curr_idx();
        }
    };
}

#endif
