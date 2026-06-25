#ifndef MCC2LM_RADICAL_HPP

#define MCC2LM_RADICAL_HPP

#include <string>
#include <vector>
#include <mcc2lm/sql.hpp>
#include <cstdint>
#include <mcc2lm/logger.hpp>

namespace mcc2lm
{
    struct CharacterMetadata
    {
        std::string pinyin;
        std::string meaning;
    };

    std::vector<std::string> get_radicals_from_logogram(std::string logogram_value);
    CharacterMetadata get_unihan_metadata(const std::string &logogram_value);

    class Radical
    {
        // MCC2LM_RADICAL
        const std::string INSERT_RADICAL_QUERY =
            "INSERT INTO MCC2LM_RADICAL "
            "(ID, PINYIN, MEANING) VALUES (?, ?, ?) "
            "ON CONFLICT(ID) DO UPDATE SET "
            "PINYIN = CASE "
            "WHEN (MCC2LM_RADICAL.PINYIN IS NULL OR MCC2LM_RADICAL.PINYIN = '') "
            "AND excluded.PINYIN IS NOT NULL AND excluded.PINYIN <> '' "
            "THEN excluded.PINYIN ELSE MCC2LM_RADICAL.PINYIN END, "
            "MEANING = CASE "
            "WHEN (MCC2LM_RADICAL.MEANING IS NULL OR MCC2LM_RADICAL.MEANING = '') "
            "AND excluded.MEANING IS NOT NULL AND excluded.MEANING <> '' "
            "THEN excluded.MEANING ELSE MCC2LM_RADICAL.MEANING END;";
        // ---

        std::string value;
        std::string pinyin;
        std::string meaning;

    public:
        Radical(std::string value,
                std::string pinyin,
                std::string meaning) : value(value),
                                       pinyin(pinyin),
                                       meaning(meaning)
        {
            if (value.length() > 0 && pinyin.length() > 0 && meaning.length() > 0)
            {
                return;
            }

            throw ParserException(
                "Not enough information to build `Radical`: value=" //
                + value                                             //
                + " pinyin="                                        //
                + pinyin                                            //
                + " meaning="                                       //
                + meaning);
        }

        std::string get_value() const
        {
            return value;
        }

        void Save(sqlite3 *db)
        {
            Logger logger("Radical('" + value + "')::Save");

            logger.Debug("Saving");

            if (value.empty())
            {
                return;
            }

            execute_non_query(
                db,
                INSERT_RADICAL_QUERY,
                [this, db](sqlite3_stmt *stmt)
                {
                    bind_text(
                        db,
                        stmt,
                        1,
                        value,
                        "[Radical::Save] Failed to bind radical value");
                    bind_text(
                        db,
                        stmt,
                        2,
                        pinyin,
                        "[Radical::Save] Failed to bind radical pinyin");
                    bind_text(
                        db,
                        stmt,
                        3,
                        meaning,
                        "[Radical::Save] Failed to bind radical meaning");
                },
                "[Radical::Save] Failed to insert radical");

            logger.Debug("Saved");
        }
    };

    class RadicalIterator
    {
        int curr_idx;
        std::string logogram_value;
        std::vector<std::string> raw_radicals;

    public:
        RadicalIterator(
            std::string logogram_value,
            int curr_idx) : logogram_value(logogram_value),
                            curr_idx(curr_idx)
        {
            raw_radicals = get_radicals_from_logogram(logogram_value);
        }

        int get_curr_idx() const
        {
            return curr_idx;
        }

        RadicalIterator begin() const
        {
            return RadicalIterator(logogram_value, 0);
        }

        RadicalIterator end() const
        {
            return RadicalIterator(logogram_value, raw_radicals.size());
        }

        RadicalIterator &operator++()
        {
            curr_idx++;
            return *this;
        }

        Radical operator*() const
        {
            const std::string radical_value = raw_radicals.at(curr_idx);
            const CharacterMetadata metadata = get_unihan_metadata(radical_value);
            return Radical(radical_value, metadata.pinyin, metadata.meaning);
        }

        bool operator!=(const RadicalIterator &rhs) const
        {
            return get_curr_idx() != rhs.get_curr_idx();
        }
    };
}

#endif
