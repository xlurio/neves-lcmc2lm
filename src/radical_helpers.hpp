#ifndef MCC2LM_RADICAL_HELPERS_HPP

#define MCC2LM_RADICAL_HELPERS_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <mcc2lm/radical.hpp>

namespace mcc2lm
{
    namespace detail
    {
        using RadicalIndex = std::unordered_map<std::string, std::vector<std::string>>;
        using MetadataAccumulator = std::unordered_map<std::string, std::vector<std::string>>;
        using MetadataIndex = std::unordered_map<std::string, CharacterMetadata>;
        using RadicalFallbackIndex = std::unordered_map<std::string, std::string>;

        bool starts_with(const std::string &value, const std::string &prefix);
        bool is_ascii_digit(char c);
        std::vector<std::string> split_tab_fields(const std::string &line);
        std::string codepoint_to_utf8(std::uint32_t codepoint);
        bool parse_unihan_codepoint(const std::string &raw, std::uint32_t &result);
        bool decode_utf8_single_codepoint(const std::string &value, std::uint32_t &result);
        bool is_kangxi_radical_codepoint(std::uint32_t codepoint);
        bool extract_bracket_codepoint(const std::string &token, std::uint32_t &result);
        bool parse_radical_number(std::string_view token, int &radical_number);
        bool append_unique(std::vector<std::string> &target, const std::string &value);
        std::string join_values(const std::vector<std::string> &values);
        std::string kangxi_radical_from_number(int radical_number);

        void load_unihan_radicals(RadicalIndex &index);
        void load_unihan_metadata(MetadataIndex &index);
        void load_kangxi_fallback_index(RadicalFallbackIndex &index);
        void load_radical_metadata_fallback_index(RadicalFallbackIndex &index);

        const RadicalIndex &get_radical_index();
        const MetadataIndex &get_metadata_index();
        const RadicalFallbackIndex &get_kangxi_fallback_index();
        const RadicalFallbackIndex &get_radical_metadata_fallback_index();
    }
}

#endif
