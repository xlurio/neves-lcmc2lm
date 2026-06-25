#include "radical_helpers.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include <mcc2lm/constants.hpp>
#include <mcc2lm/exceptions.hpp>
#include <mcc2lm/logger.hpp>

namespace mcc2lm
{
    namespace detail
    {
        void load_unihan_radicals(RadicalIndex &index)
        {
            Logger logger("get_radicals_from_logogram::load_unihan_radicals");

            std::ifstream infile(UNIHAN_RADICAL_STROKE_COUNTS_PATH);
            if (!infile.is_open())
            {
                throw ParserException(
                    "[get_radicals_from_logogram] Failed to open Unihan file: " +
                    UNIHAN_RADICAL_STROKE_COUNTS_PATH);
            }

            std::size_t row_count = 0;
            std::size_t mapped_count = 0;

            std::string line;
            while (std::getline(infile, line))
            {
                if (line.empty() || starts_with(line, "#"))
                {
                    continue;
                }

                const std::vector<std::string> fields = split_tab_fields(line);
                if (fields.size() < 3)
                {
                    continue;
                }

                std::uint32_t codepoint = 0;
                if (!parse_unihan_codepoint(fields[0], codepoint))
                {
                    continue;
                }

                const std::string logogram = codepoint_to_utf8(codepoint);
                std::istringstream data_tokens(fields[2]);
                std::string token;
                bool mapped_this_row = false;

                while (data_tokens >> token)
                {
                    int radical_number = 0;
                    if (!parse_radical_number(token, radical_number))
                    {
                        continue;
                    }

                    const std::string radical = kangxi_radical_from_number(radical_number);
                    if (radical.empty())
                    {
                        continue;
                    }

                    if (append_unique(index[logogram], radical))
                    {
                        mapped_this_row = true;
                    }
                }

                ++row_count;
                if (mapped_this_row)
                {
                    ++mapped_count;
                }
            }

            logger.Info("Loaded Unihan radical index with {} mapped logograms ({} rows scanned)", mapped_count, row_count);
        }

        void load_unihan_metadata(MetadataIndex &index)
        {
            Logger logger("get_unihan_metadata::load_unihan_metadata");

            std::ifstream infile(UNIHAN_READINGS_PATH);
            if (!infile.is_open())
            {
                throw ParserException(
                    "[get_unihan_metadata] Failed to open Unihan file: " +
                    UNIHAN_READINGS_PATH);
            }

            MetadataAccumulator pinyin_values;
            MetadataAccumulator meaning_values;
            std::size_t row_count = 0;

            std::string line;
            while (std::getline(infile, line))
            {
                if (line.empty() || starts_with(line, "#"))
                {
                    continue;
                }

                const std::vector<std::string> fields = split_tab_fields(line);
                if (fields.size() < 3)
                {
                    continue;
                }

                std::uint32_t codepoint = 0;
                if (!parse_unihan_codepoint(fields[0], codepoint))
                {
                    continue;
                }

                const std::string character = codepoint_to_utf8(codepoint);
                const std::string &property_name = fields[1];
                const std::string &property_value = fields[2];

                if (property_value.empty())
                {
                    continue;
                }

                if (property_name == "kMandarin")
                {
                    append_unique(pinyin_values[character], property_value);
                }
                else if (property_name == "kDefinition")
                {
                    append_unique(meaning_values[character], property_value);
                }

                ++row_count;
            }

            std::size_t with_pinyin = 0;
            std::size_t with_meaning = 0;
            std::size_t complete_entries = 0;

            for (const auto &entry : pinyin_values)
            {
                const std::string pinyin = join_values(entry.second);
                if (!pinyin.empty())
                {
                    ++with_pinyin;
                }
                else
                {
                    continue;
                }

                const MetadataAccumulator::const_iterator meaning_it = meaning_values.find(entry.first);
                if (meaning_it == meaning_values.end())
                {
                    continue;
                }

                const std::string meaning = join_values(meaning_it->second);
                if (meaning.empty())
                {
                    continue;
                }

                index.emplace(entry.first, CharacterMetadata(pinyin, meaning));
                ++complete_entries;
            }

            for (const auto &entry : meaning_values)
            {
                const std::string meaning = join_values(entry.second);
                if (!meaning.empty())
                {
                    ++with_meaning;
                }
            }

            logger.Info(
                "Loaded Unihan metadata index (rows scanned: {}, pinyin entries: {}, meaning entries: {}, complete entries: {})",
                row_count,
                with_pinyin,
                with_meaning,
                complete_entries);
        }
    }
}
