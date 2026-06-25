#include "radical_helpers.hpp"

#include <cstdint>
#include <string>

#include <mcc2lm/exceptions.hpp>

namespace mcc2lm
{
    namespace detail
    {
        bool starts_with(const std::string &value, const std::string &prefix)
        {
            return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
        }

        bool is_ascii_digit(char c)
        {
            return c >= '0' && c <= '9';
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

        bool decode_utf8_single_codepoint(const std::string &value, std::uint32_t &result)
        {
            if (value.empty())
            {
                return false;
            }

            const unsigned char first_byte = static_cast<unsigned char>(value[0]);

            if ((first_byte & 0x80U) == 0)
            {
                if (value.size() != 1)
                {
                    return false;
                }

                result = static_cast<std::uint32_t>(first_byte);
                return true;
            }

            int expected_size = 0;
            std::uint32_t codepoint = 0;

            if ((first_byte & 0xE0U) == 0xC0U)
            {
                expected_size = 2;
                codepoint = static_cast<std::uint32_t>(first_byte & 0x1FU);
            }
            else if ((first_byte & 0xF0U) == 0xE0U)
            {
                expected_size = 3;
                codepoint = static_cast<std::uint32_t>(first_byte & 0x0FU);
            }
            else if ((first_byte & 0xF8U) == 0xF0U)
            {
                expected_size = 4;
                codepoint = static_cast<std::uint32_t>(first_byte & 0x07U);
            }
            else
            {
                return false;
            }

            if (value.size() != static_cast<std::size_t>(expected_size))
            {
                return false;
            }

            for (int idx = 1; idx < expected_size; ++idx)
            {
                const unsigned char continuation = static_cast<unsigned char>(value[idx]);
                if ((continuation & 0xC0U) != 0x80U)
                {
                    return false;
                }

                codepoint = (codepoint << 6U) | static_cast<std::uint32_t>(continuation & 0x3FU);
            }

            result = codepoint;
            return true;
        }

        bool is_kangxi_radical_codepoint(std::uint32_t codepoint)
        {
            return codepoint >= 0x2F00U && codepoint <= 0x2FD5U;
        }
    }
}
