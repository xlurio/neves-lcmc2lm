#include "radical_helpers.hpp"

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace mcc2lm
{
    namespace detail
    {
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

        bool extract_bracket_codepoint(const std::string &token, std::uint32_t &result)
        {
            const std::size_t bracket_start = token.find("[U+");
            if (bracket_start == std::string::npos)
            {
                return false;
            }

            const std::size_t hex_start = bracket_start + 3;
            const std::size_t bracket_end = token.find(']', hex_start);
            if (bracket_end == std::string::npos || bracket_end <= hex_start)
            {
                return false;
            }

            const std::string raw_hex = token.substr(hex_start, bracket_end - hex_start);
            if (raw_hex.empty())
            {
                return false;
            }

            std::size_t parsed = 0;
            const unsigned long codepoint = std::stoul(raw_hex, &parsed, 16);
            if (parsed != raw_hex.size())
            {
                return false;
            }

            result = static_cast<std::uint32_t>(codepoint);
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
    }
}
