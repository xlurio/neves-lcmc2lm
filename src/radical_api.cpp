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
            return CharacterMetadata{"", ""};
        }

        const detail::MetadataIndex &index = detail::get_metadata_index();
        const detail::MetadataIndex::const_iterator it = index.find(logogram_value);

        CharacterMetadata metadata = CharacterMetadata{"", ""};

        if (it != index.end())
        {
            metadata = it->second;
        }

        std::uint32_t codepoint = 0;
        if (!detail::decode_utf8_single_codepoint(logogram_value, codepoint) ||
            !detail::is_kangxi_radical_codepoint(codepoint) ||
            (!metadata.pinyin.empty() && !metadata.meaning.empty()))
        {
            return metadata;
        }

        const auto merge_from_base_character = [&index, &metadata](const std::string &base_character)
        {
            const detail::MetadataIndex::const_iterator base_metadata_it = index.find(base_character);
            if (base_metadata_it == index.end())
            {
                return;
            }

            if (metadata.pinyin.empty())
            {
                metadata.pinyin = base_metadata_it->second.pinyin;
            }

            if (metadata.meaning.empty())
            {
                metadata.meaning = base_metadata_it->second.meaning;
            }
        };

        const detail::RadicalFallbackIndex &fallback_index = detail::get_kangxi_fallback_index();
        const detail::RadicalFallbackIndex::const_iterator fallback_it = fallback_index.find(logogram_value);
        if (fallback_it != fallback_index.end())
        {
            merge_from_base_character(fallback_it->second);
        }

        if (!metadata.pinyin.empty() && !metadata.meaning.empty())
        {
            return metadata;
        }

        const detail::RadicalFallbackIndex &metadata_fallback_index = detail::get_radical_metadata_fallback_index();
        const detail::RadicalFallbackIndex::const_iterator metadata_fallback_it = metadata_fallback_index.find(logogram_value);
        if (metadata_fallback_it != metadata_fallback_index.end())
        {
            merge_from_base_character(metadata_fallback_it->second);
        }

        return metadata;
    }
}
