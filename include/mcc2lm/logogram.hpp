#ifndef MCC2LM_LEXOGRAM_HPP

#define MCC2LM_LEXOGRAM_HPP

#include <string>
#include <mcc2lm/sql.hpp>
#include <cstdint>
#include <mcc2lm/logger.hpp>
#include <mcc2lm/radical.hpp>

namespace mcc2lm
{
    class Logogram
    {
        // MCC2LM_LOGOGRAM
        const std::string UPSERT_LOGOGRAM_QUERY =
            "INSERT INTO MCC2LM_LOGOGRAM (ID, OCCURRENCIES) "
            "VALUES (?, 1) "
            "ON CONFLICT(ID) DO UPDATE SET OCCURRENCIES = MCC2LM_LOGOGRAM.OCCURRENCIES + 1;";
        const std::string INSERT_RADICAL_LOGOGRAM_MAP_QUERY =
            "INSERT OR IGNORE INTO MCC2LM_RADICAL_LOGOGRAM_MAP "
            "(LOGOGRAM_ID, RADICAL_ID) VALUES (?, ?);";

        // ---

        std::string value;

    public:
        Logogram(std::string value) : value(value)
        {
            Logger logger("Logogram(" + value + ")");
            logger.Debug("`Logogram` initialized");
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

            execute_non_query(
                db,
                UPSERT_LOGOGRAM_QUERY,
                [this, db](sqlite3_stmt *stmt)
                {
                    bind_text(
                        db,
                        stmt,
                        1,
                        value,
                        "[Logogram::Save] Failed to bind value mutation ID");
                },
                "[Logogram::Save] Failed to persist value");

            RadicalIterator radicals(value, 0);

            for (Radical radical : radicals)
            {
                radical.Save(db);

                ensure_mapping_row(
                    db,
                    INSERT_RADICAL_LOGOGRAM_MAP_QUERY,
                    [this, &radical, db](sqlite3_stmt *stmt)
                    {
                        bind_text(
                            db,
                            stmt,
                            1,
                            value,
                            "[Word::Save] Failed to bind radical-logogram map logogram ID");
                        bind_text(
                            db,
                            stmt,
                            2,
                            radical.get_value(),
                            "[Word::Save] Failed to bind radical-logogram map radical ID");
                    },
                    "[Word::Save] radical-logogram map mutation");
            }

            logger.Debug("Saved");
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
