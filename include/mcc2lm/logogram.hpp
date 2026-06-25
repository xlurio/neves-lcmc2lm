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
            "INSERT INTO MCC2LM_LOGOGRAM (ID, OCCURRENCIES, PINYIN, MEANING) "
            "VALUES (?, 1, ?, ?) "
            "ON CONFLICT(ID) DO UPDATE SET "
            "OCCURRENCIES = MCC2LM_LOGOGRAM.OCCURRENCIES + 1, "
            "PINYIN = CASE "
            "WHEN (MCC2LM_LOGOGRAM.PINYIN IS NULL OR MCC2LM_LOGOGRAM.PINYIN = '') "
            "AND excluded.PINYIN IS NOT NULL AND excluded.PINYIN <> '' "
            "THEN excluded.PINYIN ELSE MCC2LM_LOGOGRAM.PINYIN END, "
            "MEANING = CASE "
            "WHEN (MCC2LM_LOGOGRAM.MEANING IS NULL OR MCC2LM_LOGOGRAM.MEANING = '') "
            "AND excluded.MEANING IS NOT NULL AND excluded.MEANING <> '' "
            "THEN excluded.MEANING ELSE MCC2LM_LOGOGRAM.MEANING END;";
        const std::string INSERT_RADICAL_LOGOGRAM_MAP_QUERY =
            "INSERT OR IGNORE INTO MCC2LM_RADICAL_LOGOGRAM_MAP "
            "(LOGOGRAM_ID, RADICAL_ID) VALUES (?, ?);";

        // ---

        std::string value;
        std::string pinyin;
        std::string meaning;

    public:
        Logogram(std::string value);
        const std::string &get_id() const;
        void Save(sqlite3 *db);
    };

    class LogogramIterator
    {
        int curr_idx;
        std::string raw_str;

    public:
        LogogramIterator(std::string raw_str, int curr_idx);
        int get_curr_idx() const;
        LogogramIterator begin() const;
        LogogramIterator end() const;
        int get_char_size() const;
        LogogramIterator &operator++();
        Logogram operator*() const;
        bool operator!=(const LogogramIterator &rhs) const;
    };

#include <mcc2lm/impl/logogram_value_impl.inc>
#include <mcc2lm/impl/logogram_iterator_impl.inc>
}

#endif
