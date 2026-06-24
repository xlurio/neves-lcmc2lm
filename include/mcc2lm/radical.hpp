#ifndef MCC2LM_RADICAL_HPP

#define MCC2LM_RADICAL_HPP

#include <string>
#include <vector>
#include <mcc2lm/sql.hpp>
#include <cstdint>
#include <mcc2lm/logger.hpp>

namespace mcc2lm
{
    std::vector<std::string> get_radicals_from_logogram(std::string logogram_value);

    class Radical
    {
        // MCC2LM_RADICAL
        const std::string INSERT_RADICAL_QUERY =
            "INSERT OR IGNORE INTO MCC2LM_RADICAL "
            "(ID) VALUES (?);";
        // ---

        std::string value;

    public:
        Radical(std::string value) : value(value) {}

        std::string get_value() const {
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
            return Radical(raw_radicals.at(curr_idx));
        }

        bool operator!=(const RadicalIterator &rhs) const
        {
            return get_curr_idx() != rhs.get_curr_idx();
        }
    };
}

#endif
