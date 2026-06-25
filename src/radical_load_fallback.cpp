#include "radical_helpers.hpp"

#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

#include <mcc2lm/constants.hpp>
#include <mcc2lm/exceptions.hpp>
#include <mcc2lm/logger.hpp>

namespace mcc2lm
{
    namespace detail
    {
        void load_kangxi_fallback_index(RadicalFallbackIndex &index)
        {
            Logger logger("get_unihan_metadata::load_kangxi_fallback_index");

            std::ifstream infile(UNIHAN_DICTIONARY_LIKE_DATA_PATH);
            if (!infile.is_open())
            {
                throw ParserException(
                    "[get_unihan_metadata] Failed to open Unihan file: " +
                    UNIHAN_DICTIONARY_LIKE_DATA_PATH);
            }

            std::size_t row_count = 0;
            std::size_t mapping_count = 0;

            std::string line;
            while (std::getline(infile, line))
            {
                if (line.empty() || starts_with(line, "#"))
                {
                    continue;
                }

                const std::vector<std::string> fields = split_tab_fields(line);
                if (fields.size() < 3 || fields[1] != "kHDZRadBreak")
                {
                    continue;
                }

                std::uint32_t base_codepoint = 0;
                if (!parse_unihan_codepoint(fields[0], base_codepoint))
                {
                    continue;
                }

                const std::string base_character = codepoint_to_utf8(base_codepoint);
                std::istringstream data_tokens(fields[2]);
                std::string token;

                while (data_tokens >> token)
                {
                    std::uint32_t radical_codepoint = 0;
                    if (!extract_bracket_codepoint(token, radical_codepoint) ||
                        !is_kangxi_radical_codepoint(radical_codepoint))
                    {
                        continue;
                    }

                    const std::string radical_character = codepoint_to_utf8(radical_codepoint);
                    if (index.find(radical_character) == index.end())
                    {
                        index.emplace(radical_character, base_character);
                        ++mapping_count;
                    }
                }

                ++row_count;
            }

            logger.Info(
                "Loaded Kangxi fallback index (rows scanned: {}, mappings: {})",
                row_count,
                mapping_count);
        }

        void load_radical_metadata_fallback_index(RadicalFallbackIndex &index)
        {
            Logger logger("get_unihan_metadata::load_radical_metadata_fallback_index");

            const RadicalIndex &radical_index = get_radical_index();
            const MetadataIndex &metadata_index = get_metadata_index();

            std::size_t mapping_count = 0;

            for (const auto &entry : radical_index)
            {
                const MetadataIndex::const_iterator metadata_it = metadata_index.find(entry.first);
                if (metadata_it == metadata_index.end() ||
                    metadata_it->second.pinyin.empty() ||
                    metadata_it->second.meaning.empty())
                {
                    continue;
                }

                for (const std::string &radical_character : entry.second)
                {
                    std::uint32_t radical_codepoint = 0;
                    if (!decode_utf8_single_codepoint(radical_character, radical_codepoint) ||
                        !is_kangxi_radical_codepoint(radical_codepoint) ||
                        index.find(radical_character) != index.end())
                    {
                        continue;
                    }

                    index.emplace(radical_character, entry.first);
                    ++mapping_count;
                }
            }

            logger.Info(
                "Loaded radical metadata fallback index (mappings: {})",
                mapping_count);
        }

        const RadicalIndex &get_radical_index()
        {
            static RadicalIndex index;
            static std::once_flag loaded;

            std::call_once(loaded, []()
                           { load_unihan_radicals(index); });

            return index;
        }

        const MetadataIndex &get_metadata_index()
        {
            static MetadataIndex index;
            static std::once_flag loaded;

            std::call_once(loaded, []()
                           { load_unihan_metadata(index); });

            return index;
        }

        const RadicalFallbackIndex &get_kangxi_fallback_index()
        {
            static RadicalFallbackIndex index;
            static std::once_flag loaded;

            std::call_once(loaded, []()
                           { load_kangxi_fallback_index(index); });

            return index;
        }

        const RadicalFallbackIndex &get_radical_metadata_fallback_index()
        {
            static RadicalFallbackIndex index;
            static std::once_flag loaded;

            std::call_once(loaded, []()
                           { load_radical_metadata_fallback_index(index); });

            return index;
        }
    }
}
