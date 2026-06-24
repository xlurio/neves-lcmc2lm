#ifndef MCC2LM_LEXOGRAM_HPP

#define MCC2LM_LEXOGRAM_HPP

#include <string>
#include <mcc2lm/sql.hpp>
#include <cstdint>
#include <mcc2lm/logger.hpp>

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

        std::string value;

    public:
        Logogram(std::string value) : value(value) {
            Logger logger("Logogram(" + value + ")");
            logger.Info("`Logogram` initialized");
        }

        const std::string &get_id() const
        {
            return value;
        }

        void Save(sqlite3 *db)
        {
            Logger logger("Logogram('" + value + "')::Save");

            logger.Debug("Saving");

            if (value.empty())
            {
                return;
            }

            const bool already_exists = row_exists(
                db,
                GET_LOGOGRAM_BY_ID_QUERY,
                [this, db](sqlite3_stmt *stmt)
                {
                    bind_text(db, stmt, 1, value, "[Logogram::Save] Failed to bind value ID");
                },
                "[Logogram::Save] Failed to select value");

            sqlite3_stmt *mutation_stmt = nullptr;
            const std::string &query = already_exists ? UPDATE_LOGOGRAM_QUERY : INSERT_LOGOGRAM_QUERY;
            (void)mutation_stmt;

            execute_non_query(
                db,
                query,
                [this, db](sqlite3_stmt *stmt)
                {
                    bind_text(db, stmt, 1, value, "[Logogram::Save] Failed to bind value mutation ID");
                },
                "[Logogram::Save] Failed to persist value");

            logger.Info("Saved");
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

        LogogramIterator begin() const
        {
            return LogogramIterator(raw_str, 0);
        }

        LogogramIterator end() const
        {
            return LogogramIterator(raw_str, raw_str.size());
        }

        int get_char_size() const
        {
            if (curr_idx < 0 || curr_idx >= static_cast<int>(raw_str.size()))
            {
                return 0;
            }

            const unsigned char first_char = static_cast<unsigned char>(raw_str.at(curr_idx));

            if ((first_char & 0x80U) == 0)
            {
                return 1;
            }

            if ((first_char & 0xE0U) == 0xC0U)
            {
                return 2;
            }

            if ((first_char & 0xF0U) == 0xE0U)
            {
                return 3;
            }

            if ((first_char & 0xF8U) == 0xF0U)
            {
                return 4;
            }

            // Invalid leading byte. Consume one byte to avoid livelock.
            return 1;
        }

        LogogramIterator &operator++()
        {
            const int char_size = get_char_size();
            if (char_size <= 0)
            {
                curr_idx = static_cast<int>(raw_str.size());
                return *this;
            }

            const int capped_next_idx = curr_idx + char_size;
            const int max_idx = static_cast<int>(raw_str.size());
            curr_idx = capped_next_idx > max_idx ? max_idx : capped_next_idx;
            return *this;
        }

        Logogram operator*() const
        {
            const int char_size = get_char_size();
            if (char_size <= 0)
            {
                return Logogram("");
            }

            int safe_char_size = char_size;
            const int remaining_size = static_cast<int>(raw_str.size()) - curr_idx;
            if (safe_char_size > remaining_size)
            {
                safe_char_size = remaining_size;
            }

            return Logogram(raw_str.substr(curr_idx, safe_char_size));
        }

        bool operator!=(const LogogramIterator &rhs) const
        {
            return get_curr_idx() != rhs.get_curr_idx();
        }
    };
}

#endif
