#include <mcc2lm/radical.hpp>

#include <cstdint>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <mcc2lm/constants.hpp>
#include <mcc2lm/exceptions.hpp>

namespace mcc2lm
{
    namespace
    {
        using RadicalIndex = std::unordered_map<std::string, std::vector<std::string>>;
        using MetadataAccumulator = std::unordered_map<std::string, std::vector<std::string>>;
        using MetadataIndex = std::unordered_map<std::string, CharacterMetadata>;

        bool starts_with(const std::string &value, const std::string &prefix)
        {
            return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
        }

        bool is_ascii_digit(char c)
        {
            return c >= '0' && c <= '9';
        }

        std::vector<std::string> split_tab_fields(const std::string &line)
        {
            std::vector<std::string> fields;
            std::size_t start = 0;

            while (start <= line.size())
            {
                const std::size_t tab_pos = line.find('\t', start);
                if (tab_pos == std::string::npos)
                {
                    fields.push_back(line.substr(start));
                    break;
                }

                fields.push_back(line.substr(start, tab_pos - start));
                start = tab_pos + 1;
            }

            return fields;
        }

        std::string codepoint_to_utf8(std::uint32_t codepoint)
        {
            std::string out;

            if (codepoint <= 0x7FU)
            {
                out.push_back(static_cast<char>(codepoint));
                return out;
            }

            if (codepoint <= 0x7FFU)
            {
                out.push_back(static_cast<char>(0xC0U | (codepoint >> 6)));
                out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
                return out;
            }

            if (codepoint <= 0xFFFFU)
            {
                out.push_back(static_cast<char>(0xE0U | (codepoint >> 12)));
                out.push_back(static_cast<char>(0x80U | ((codepoint >> 6) & 0x3FU)));
                out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
                return out;
            }

            if (codepoint <= 0x10FFFFU)
            {
                out.push_back(static_cast<char>(0xF0U | (codepoint >> 18)));
                out.push_back(static_cast<char>(0x80U | ((codepoint >> 12) & 0x3FU)));
                out.push_back(static_cast<char>(0x80U | ((codepoint >> 6) & 0x3FU)));
                out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
                return out;
            }

            throw ParserException("[get_radicals_from_logogram] Invalid Unicode codepoint value");
        }

        bool parse_unihan_codepoint(const std::string &raw, std::uint32_t &result)
        {
            if (!starts_with(raw, "U+"))
            {
                return false;
            }

            const std::string hex = raw.substr(2);
            if (hex.empty())
            {
                return false;
            }

            std::size_t parsed = 0;
            const unsigned long value = std::stoul(hex, &parsed, 16);
            if (parsed != hex.size())
            {
                return false;
            }

            result = static_cast<std::uint32_t>(value);
            return true;
        }

        bool parse_radical_number(std::string_view token, int &radical_number)
        {
            const std::size_t plus_pos = token.rfind('+');
            if (plus_pos == std::string_view::npos || plus_pos + 1 >= token.size())
            {
                return false;
            }

            const std::string_view rs_part = token.substr(plus_pos + 1);
            std::size_t idx = 0;
            while (idx < rs_part.size() && is_ascii_digit(static_cast<char>(rs_part[idx])))
            {
                ++idx;
            }

            if (idx == 0 || idx == rs_part.size() || rs_part[idx] != '.')
            {
                return false;
            }

            const std::string raw_number(rs_part.substr(0, idx));
            radical_number = std::stoi(raw_number);
            return true;
        }

        bool append_unique(std::vector<std::string> &target, const std::string &value)
        {
            for (const std::string &existing : target)
            {
                if (existing == value)
                {
                    return false;
                }
            }

            target.push_back(value);
            return true;
        }

        std::string join_values(const std::vector<std::string> &values)
        {
            if (values.empty())
            {
                return "";
            }

            std::string joined = values.front();
            for (std::size_t idx = 1; idx < values.size(); ++idx)
            {
                joined += "; ";
                joined += values[idx];
            }

            return joined;
        }

        std::string kangxi_radical_from_number(int radical_number)
        {
            if (radical_number < 1 || radical_number > 214)
            {
                return "";
            }

            const std::uint32_t kangxi_codepoint = 0x2F00U + static_cast<std::uint32_t>(radical_number - 1);
            return codepoint_to_utf8(kangxi_codepoint);
        }

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

            for (const auto &entry : pinyin_values)
            {
                CharacterMetadata &metadata = index[entry.first];
                metadata.pinyin = join_values(entry.second);
                if (!metadata.pinyin.empty())
                {
                    ++with_pinyin;
                }
            }

            for (const auto &entry : meaning_values)
            {
                CharacterMetadata &metadata = index[entry.first];
                metadata.meaning = join_values(entry.second);
                if (!metadata.meaning.empty())
                {
                    ++with_meaning;
                }
            }

            logger.Info(
                "Loaded Unihan metadata index (rows scanned: {}, pinyin entries: {}, meaning entries: {})",
                row_count,
                with_pinyin,
                with_meaning);
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
    }

    std::vector<std::string> get_radicals_from_logogram(std::string logogram_value)
    {
        Logger logger("get_radicals_from_logogram('" + logogram_value + "')");

        if (logogram_value.empty())
        {
            return {};
        }

        const RadicalIndex &index = get_radical_index();
        const RadicalIndex::const_iterator it = index.find(logogram_value);

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

        const MetadataIndex &index = get_metadata_index();
        const MetadataIndex::const_iterator it = index.find(logogram_value);

        if (it == index.end())
        {
            return CharacterMetadata{"", ""};
        }

        return it->second;
    }
}
