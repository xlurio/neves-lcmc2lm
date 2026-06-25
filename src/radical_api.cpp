#include "radical_helpers.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mcc2lm
{
    std::vector<std::string> get_radicals_from_logogram(std::string logogram_value)
    {
        Logger logger("get_radicals_from_logogram('" + logogram_value + "')");

        if (logogram_value.empty())
        {
            return {};
        }

        const detail::RadicalIndex &index = detail::get_radical_index();
        const detail::RadicalIndex::const_iterator it = index.find(logogram_value);

        if (it == index.end())
        {
            logger.Debug("No Unihan radical mapping found");
            return {};
        }

        return it->second;
    }

    CharacterMetadata get_unihan_metadata(const std::string &logogram_value)
    {
        if (logogram_value.empty())
        {
            throw ParserException("[get_unihan_metadata] Empty logogram value");
        }

        const detail::MetadataIndex &index = detail::get_metadata_index();
        const detail::MetadataIndex::const_iterator it = index.find(logogram_value);

        if (it != index.end())
        {
            return it->second;
        }

        std::string pinyin;
        std::string meaning;

        std::uint32_t codepoint = 0;
        if (!detail::decode_utf8_single_codepoint(logogram_value, codepoint) ||
            !detail::is_kangxi_radical_codepoint(codepoint))
        {
            throw ParserException("[get_unihan_metadata] Metadata not found for logogram: " + logogram_value);
        }

        const auto merge_from_base_character = [&index, &pinyin, &meaning](const std::string &base_character)
        {
            const detail::MetadataIndex::const_iterator base_metadata_it = index.find(base_character);
            if (base_metadata_it == index.end())
            {
                return;
            }

            if (pinyin.empty())
            {
                pinyin = base_metadata_it->second.get_pinyin();
            }

            if (meaning.empty())
            {
                meaning = base_metadata_it->second.get_meaning();
            }
        };

        const detail::RadicalFallbackIndex &fallback_index = detail::get_kangxi_fallback_index();
        const detail::RadicalFallbackIndex::const_iterator fallback_it = fallback_index.find(logogram_value);
        if (fallback_it != fallback_index.end())
        {
            merge_from_base_character(fallback_it->second);
        }

        if (!pinyin.empty() && !meaning.empty())
        {
            return CharacterMetadata(pinyin, meaning);
        }

        const detail::RadicalFallbackIndex &metadata_fallback_index = detail::get_radical_metadata_fallback_index();
        const detail::RadicalFallbackIndex::const_iterator metadata_fallback_it = metadata_fallback_index.find(logogram_value);
        if (metadata_fallback_it != metadata_fallback_index.end())
        {
            merge_from_base_character(metadata_fallback_it->second);
        }

        if (!pinyin.empty() && !meaning.empty())
        {
            return CharacterMetadata(pinyin, meaning);
        }

        throw ParserException("[get_unihan_metadata] Metadata not found for logogram: " + logogram_value);
    }
}
