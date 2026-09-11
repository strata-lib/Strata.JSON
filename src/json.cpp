#include "json.h"
#include "json_elements.h"
#include "span.h"

#include <cassert>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cuchar>
#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace strata::json;

namespace strata
{
	namespace json
	{
		namespace utility
		{
			template <typename... Args> inline std::string string_format(const std::string& format, Args... args)
			{
				// Extra space for '\0'
				int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
				if (size_s <= 0)
				{
					throw std::runtime_error("Error during formatting.");
				}

				auto size = static_cast<size_t>(size_s);
				std::unique_ptr<char[]> buf(new char[size]);
				std::snprintf(buf.get(), size, format.c_str(), args...);
				return std::string(buf.get(),
					buf.get() + size - 1); // We don't want the '\0' inside
			}

			inline bool isdigit(const char c)
			{
				if (c < 0 || c > 127)
				{
					return false;
				}

				return ::isdigit(c);
			}

			inline bool str2int(int& i, char const* s)
			{
				std::stringstream ss(s);
				ss >> i;
				if (ss.fail())
				{
					// not an integer
					return false;
				}
				return true;
			}
		}
	}
}

size_t json::MAX_PARSING_DEPTH{ 500 };

void json::data_iterator::skip_wspace()
{
	while (is_valid())
	{
		char c = peek();
		if (c == '\r' ||
			c == '\n' ||
			c == ' ' ||
			c == '\t')
		{
			offset++;
			continue;
		}

		return;
	}
}

value json::from_text(const std::string& text, parsing_errors* errors)
{
	span<const uint8_t> bytes{ reinterpret_cast<const uint8_t*>(text.data()), text.size() };
	data_iterator it{ bytes };
	return from_iterator(it, errors);
}

value json::from_text(const std::string& text)
{
	return from_text(text, nullptr);
}

value json::from_file(const std::filesystem::path& path,
	parsing_errors* errors)
{
	std::ifstream file;
	file.open(path);
	value json = from_stream(file, errors);
	file.close();
	return json;
}

value json::from_file(const std::filesystem::path& path)
{
	return from_file(path, nullptr);
}

value json::from_stream(std::istream& stream, parsing_errors* errors)
{
	stream.ignore(std::numeric_limits<std::streamsize>::max());
	const size_t byte_count = static_cast<size_t>(stream.gcount());
	stream.clear(); //  Since ignore will have set eof.
	stream.seekg(0, std::ios_base::beg);

	if (byte_count == 0)
	{
		if (errors)
		{
			errors->push_back(get_err_template(parsing_error_type::expected_data));
		}

		return value{ };
	}

	std::vector<uint8_t> data;
	data.resize(byte_count);
	char* firstPtr = (char*)(&data[0]);
	stream.read(firstPtr, data.size());

	span<const uint8_t> bytes{ reinterpret_cast<const uint8_t*>(data.data()), data.size() };
	data_iterator it{ bytes };
	return from_iterator(it, errors);
}

value json::from_stream(std::istream& stream)
{
	return from_stream(stream, nullptr);
}

value json::from_memory(std::vector<uint8_t>& stream,
	parsing_errors* errors)
{
	data_iterator it{ span<const uint8_t>{ &stream[0], stream.size() } };
	return from_iterator(it, errors);
}

value json::from_memory(std::vector<uint8_t>& stream)
{
	return from_memory(stream, nullptr);
}

parsing_errors& json::get_last_errors()
{
	return m_errors;
}

void json::clear_errors()
{
	m_errors.clear();
}

value json::from_iterator(data_iterator& it, parsing_errors* errors)
{
	json p;
	value element = p.parse_value(it);
	it.skip_wspace();
	if (it.offset != it.data.size() && p.m_errors.size() == 0)
	{
		p.build_error(parsing_error_type::leftover_characters_in_data, it);
	}

	if (p.m_errors.size() > 0)
	{
		if (errors)
		{
			*errors = p.m_errors;
		}

		element = value{ };
	}

#ifdef _DEBUG
	if (p.m_errors.size() != 0)
	{
		assert(element.is_null());
	}
#endif // DEBUG

	return element;
}

std::string json::get_err_template(parsing_error_type type)
{
	switch (type)
	{
	default:
	case parsing_error_type::unknown:
		return "Unexpected error at '%d':\n";
	case parsing_error_type::expected_data:
		return "Expected data to parse but found nothing.\n";
	case parsing_error_type::expected_null:
		return "Expected 'null' keyword at '%d':\n";
	case parsing_error_type::expected_bool:
		return "Expected 'true' or 'false' keyword while parsing boolean at '%d':\n";
	case parsing_error_type::unexpected_character_in_file:
		return "Unexpected character at '%d':\n";
	case parsing_error_type::unexpected_eof:
		return "Unexpected EOF at '%d':\n";
	case parsing_error_type::unexpected_char_terminator:
		return "Unexpected 0 Terminator at '%d':\n";
	case parsing_error_type::unexpected_newline:
		return "Unexpected unexcaped newline at '%d':\n";
	case parsing_error_type::unexpected_tab_character:
		return "Unexpected unexcaped TAB character at '%d':\n";
	case parsing_error_type::unexpected_comma:
		return "Unexpected trailing comma (',') at '%d' (Did you forget a value?):\n";
	case parsing_error_type::expected_start_of_array:
		return "Unexpected start of JArray at '%d':\n";
	case parsing_error_type::expected_start_of_object:
		return "Unexpected start of JObject at '%d':\n";
	case parsing_error_type::expected_end_of_array:
		return "Expected end of JArray at '%d', did you forget a comma?:\n";
	case parsing_error_type::expected_end_of_object:
		return "Unexpected end of JObject at '%d':\n";
	case parsing_error_type::expected_colon_marker:
		return "Expected colon (':') at '%d':\n";
	case parsing_error_type::expected_quotes:
		return "Expected double quotes ('\"') at '%d':\n";
	case parsing_error_type::leftover_characters_in_data:
		return "Leftover characters at '%d' after parsing json:\n";
	case parsing_error_type::not_a_number:
		return "Value is not a valid number at '%d':\n";
	case parsing_error_type::number_cannot_start_with_zero:
		return "Number at '%d' starts with 0, leading 0s are not allowed:\n";
	case parsing_error_type::number_cannot_end_with_sign:
		return "Number cannot end with '-' or '+', error at '%d':\n";
	case parsing_error_type::number_cannot_end_with_exponential_character:
		return "Number cannot end with 'e', error at '%d':\n";
	case parsing_error_type::number_cannot_end_with_decimal_separator:
		return "Number ends with decimal separator '.' at '%d':\n";
	case parsing_error_type::expected_utf_character:
		return "Character at '%d' is not a HEX digit!\n";
	case parsing_error_type::max_json_depth_reached:
		return "Max json depth of '%d' reached at '%d'";
	case parsing_error_type::unexpected_escape_character:
		return "Unexpected escape character at '%d', it is not a valid escape character!\n";
	case parsing_error_type::unpaired_surrogate:
		return "Unpaired UTF-16 surrogate in \\u escape at '%d':\n";
	}
}

void json::build_error(parsing_error_type type, data_iterator& data)
{
	std::string templ = get_err_template(type);
	size_t charIndex = data.offset;

	std::string preview{ "" };
	if (charIndex < data.data.size())
	{
		size_t startIndex = charIndex;
		size_t endIndex = charIndex;
		size_t backThreshold = (charIndex >= 15) ? (charIndex - 15) : 0;

		while (startIndex > 0 && data.data[startIndex] != '\n' && data.data[startIndex] != '\r' &&
			startIndex > backThreshold)
		{
			startIndex--;
		}

		while (endIndex < data.data.size() - 1 && data.data[endIndex] != '\n' && data.data[endIndex] != '\r' &&
			endIndex < charIndex + 15)
		{
			endIndex++;
		}

		auto cString = data.data;
		preview = std::string(&cString[startIndex], &cString[endIndex]);
		std::string errMarker = '\n' + std::string(charIndex - startIndex, ' ') + '^';
		preview += errMarker;
	}

	switch (type)
	{
	case parsing_error_type::max_json_depth_reached:
		templ = utility::string_format(templ, json::MAX_PARSING_DEPTH, charIndex);
		break;
	default:
		templ = utility::string_format(templ, charIndex);
		break;
	}

	m_errors.push_back(templ + preview);
}

value json::parse_value(data_iterator& it)
{
	while (it.is_valid())
	{
		it.skip_wspace();
		switch (it.peek())
		{
		case '[': {
			// Parse array
			auto result = parse_array(it);
			m_depth--;
			return result;
		}
		case '{': {
			// Parse object
			auto result = parse_object(it);
			m_depth--;
			return result;
		}
		case '"': {
			// Parse string
			std::string s;
			int result = parse_string(it, s);
			if (result != 0)
			{
				return value{};
			}

			return value{ s };
		}
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '-': {
			// Parse number
			return parse_number(it);
		}
		case 't':
		case 'f': {
			return parse_boolean(it);
		}
		case 'n': {
			const char* ptr = reinterpret_cast<const char*>(&it.data[it.offset]);
			if (strncmp(ptr, "null", 4) == 0)
			{
				it.offset += 4;
				return value{};
			}

			build_error(parsing_error_type::expected_null, it);
			it.offset = it.data.size();
			break;
		}
		default:
			build_error(parsing_error_type::unexpected_character_in_file, it);
			it.offset = it.data.size();
			break;
		}
	}

	if (it.offset == it.data.size() && m_errors.size() == 0)
	{
		build_error(parsing_error_type::unexpected_eof, it);
	}

	return value{};
}

array json::parse_array(data_iterator& it)
{
	if (m_depth >= json::MAX_PARSING_DEPTH)
	{
		build_error(parsing_error_type::max_json_depth_reached, it);
		return array{};
	}

	m_depth++;
	if (it.peek() != '[')
	{
		build_error(parsing_error_type::expected_start_of_array, it);
		return array{};
	}

	it.read1();
	it.skip_wspace();

	array result{};
	while (it.is_valid() && it.peek() != ']')
	{
		it.skip_wspace();
		auto val = parse_value(it);
		if (!m_errors.empty())
		{
			return array{};
		}

		result.push_back(std::move(val));

		it.skip_wspace();
		if (it.peek() != ',')
		{
			break;
		}

		it.read1();
		it.skip_wspace();

		if (it.peek() == ']')
		{
			build_error(parsing_error_type::unexpected_comma, it);
			return array{};
		}
	}

	it.skip_wspace();
	if (it.peek() != ']')
	{
		build_error(parsing_error_type::expected_end_of_array, it);
		return array{};
	}

	it.read1();
	return result;
}

object json::parse_object(data_iterator& it)
{
	if (m_depth >= json::MAX_PARSING_DEPTH)
	{
		build_error(parsing_error_type::max_json_depth_reached, it);
		return object{};
	}

	m_depth++;

	if (it.peek() != '{')
	{
		build_error(parsing_error_type::expected_start_of_object, it);
		return object{};
	}

	it.read1();
	it.skip_wspace();

	object result = {};
	bool has_read_comma = false;
	size_t last_comma_index = -1;
	while (it.is_valid() && it.peek() != '}')
	{
		it.skip_wspace();
		auto [memberName, value] = parse_member(it);
		if (!m_errors.empty())
		{
			return object{};
		}

		has_read_comma = false;
		result.insert(memberName, std::move(value));
		it.skip_wspace();
		if (it.peek() != ',')
		{
			break;
		}

		last_comma_index = it.offset;
		has_read_comma = true;
		it.read1();
	}

	if (has_read_comma)
	{
		it.offset = last_comma_index;
		build_error(parsing_error_type::unexpected_comma, it);
		return object{};
	}

	it.skip_wspace();
	if (it.peek() != '}')
	{
		build_error(parsing_error_type::expected_end_of_object, it);
		return object{};
	}

	it.read1();
	return result;
}

value json::parse_boolean(data_iterator& it)
{
	std::string boolValue;

	it.skip_wspace();
	if (it.peek() == 't')
	{
		const char* ptr = reinterpret_cast<const char*>(&it.data[it.offset]);
		if (strncmp(ptr, "true", 4) == 0)
		{
			it.offset += 4;
			return true;
		}
	}

	if (it.peek() == 'f')
	{
		const char* ptr = reinterpret_cast<const char*>(&it.data[it.offset]);
		if (strncmp(ptr, "false", 5) == 0)
		{
			it.offset += 5;
			return false;
		}
	}

	build_error(parsing_error_type::expected_bool, it);
	return value{};
}

value json::parse_number(data_iterator& it)
{
	it.skip_wspace();

	std::string number = "";
	bool isFractional = false;

	char firstChar = it.peek();
	if (!utility::isdigit(firstChar))
	{
		if (firstChar != '-')
		{
			build_error(parsing_error_type::not_a_number, it);
			return value{};
		}

		number += it.read1();

		if (!utility::isdigit(it.peek()))
		{
			build_error(parsing_error_type::not_a_number, it);
			return value{};
		}
	}

	while (utility::isdigit(it.peek()))
	{
		number += it.read1();
	}

	// Number has fractionalpart
	if (it.peek() == '.')
	{
		isFractional = true;
		number += it.read1();
		if (!utility::isdigit(it.peek()))
		{
			build_error(parsing_error_type::number_cannot_end_with_decimal_separator, it);
			return value{};
		}

		while (utility::isdigit(it.peek()))
		{
			number += it.read1();
		}
	}

	// Number has exponent part
	if (it.peek() == 'e' || it.peek() == 'E')
	{
		number += it.read1();
		firstChar = it.peek();
		if (!utility::isdigit(firstChar))
		{
			if (firstChar != '+' && firstChar != '-')
			{
				build_error(parsing_error_type::number_cannot_end_with_exponential_character, it);
				return value{};
			}

			number += it.read1();

			if (!utility::isdigit(it.peek()))
			{
				build_error(parsing_error_type::number_cannot_end_with_sign, it);
				return value{};
			}
		}

		while (utility::isdigit(it.peek()))
		{
			number += it.read1();
		}
	}

	auto error = is_valid_number(number);
	if (error != parsing_error_type::no_error)
	{
		build_error(error, it);
		return value{};
	}

	try
	{
		if (isFractional)
		{
			return std::stod(number);
		}

		return (integer)std::strtoll(number.data(), 0, 10);
	}
	catch (std::out_of_range&)
	{
		return 0;
	}
}

parsing_error_type json::is_valid_number(const std::string& value)
{
	size_t strLen = value.size();
	if (strLen == 0)
	{
		return parsing_error_type::not_a_number;
	}

	if (strLen == 1)
	{
		// Number can only contain these characters but not start with these
		if (value[0] == '.' || value[0] == '-' || value[0] == 'e')
		{
			return parsing_error_type::not_a_number;
		}
	}

	// Value cannot start with a zero
	int offset = value[0] == '-' ? 1 : 0;

	// If value is zero and there are more characters in the number
	// then it is an error
	if (value[offset] == '0')
	{
		if (offset + 1 < value.size())
		{
			if (value[offset + 1] != '.' && value[offset + 1] != 'e')
				return parsing_error_type::number_cannot_start_with_zero;
		}
	}

	return parsing_error_type::no_error;
}

int json::parse_string(data_iterator& it, std::string& result)
{
	if (it.peek() != '"')
	{
		build_error(parsing_error_type::expected_quotes, it);
		return -1;
	}
	it.read1();

	while (it.peek() != '"')
	{
		if (!it.is_valid())
		{
			build_error(parsing_error_type::unexpected_eof, it);
			return -1;
		}

		if (it.peek() == '\\')
		{
			std::string escapedCharacters;
			if (!get_escaped_character(it, escapedCharacters))
			{
				return -1;
			}
			result += escapedCharacters;
			continue;
		}

		if (it.peek() == 0)
		{
			build_error(parsing_error_type::unexpected_char_terminator, it);
			return -1;
		}

		if (it.peek() == '\n' || it.peek() == '\r')
		{
			build_error(parsing_error_type::unexpected_newline, it);
			return -1;
		}

		if (it.peek() == '\t')
		{
			build_error(parsing_error_type::unexpected_tab_character, it);
			return -1;
		}

		result += it.read1();
	}

	if (it.peek() != '"')
	{
		build_error(parsing_error_type::expected_quotes, it);
		return -1;
	}

	it.read1();
	return 0;
}

std::tuple<std::string, value> json::parse_member(data_iterator& it)
{
	std::string memberName;
	int result = parse_string(it, memberName);
	if (result != 0)
	{
		return std::make_tuple("", value{});
	}

	it.skip_wspace();
	if (it.peek() != ':')
	{
		build_error(parsing_error_type::expected_colon_marker, it);
		return std::make_tuple(memberName, value{});
	}

	it.read1();
	return std::make_tuple(memberName, parse_value(it));
}

bool json::get_escaped_character(data_iterator& it, std::string& escapedCharacters)
{
	escapedCharacters = "";
	it.read1(); // Consume escape char
	switch (it.peek())
	{
	case '\\':
		it.read1();
		escapedCharacters = "\\";
		return true;
	case '/':
		it.read1();
		escapedCharacters = "/";
		return true;
	case 'b':
		it.read1();
		escapedCharacters = "\b";
		return true;
	case 'f':
		it.read1();
		escapedCharacters = "\f";
		return true;
	case 'n':
		it.read1();
		escapedCharacters = "\n";
		return true;
	case 'r':
		it.read1();
		escapedCharacters = "\r";
		return true;
	case 't':
		it.read1();
		escapedCharacters = "\t";
		return true;
	case '"':
		it.read1();
		escapedCharacters = "\"";
		return true;
	case 'u': {
		it.offset++;
		char32_t codepoint;
		if (!parse_unicode_hex_quad(it, codepoint))
		{
			return false;
		}

		if (codepoint >= 0xD800 && codepoint <= 0xDBFF)
		{
			// High surrogate — must be immediately followed by a low
			// surrogate escape so the pair can form one codepoint.
			if (it.peek() != '\\' || it.offset + 1 >= it.data.size() || it.data[it.offset + 1] != 'u')
			{
				build_error(parsing_error_type::unpaired_surrogate, it);
				return false;
			}

			it.offset += 2; // consume the '\u' of the low surrogate

			char32_t low;
			if (!parse_unicode_hex_quad(it, low))
			{
				return false;
			}

			if (low < 0xDC00 || low > 0xDFFF)
			{
				build_error(parsing_error_type::unpaired_surrogate, it);
				return false;
			}

			codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
		}
		else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF)
		{
			// Lone low surrogate with no preceding high surrogate.
			build_error(parsing_error_type::unpaired_surrogate, it);
			return false;
		}

		escapedCharacters = codepoint_to_utf8(codepoint);
		return true;
	}
	default:
		build_error(parsing_error_type::unexpected_escape_character, it);
	}
	return false;
}

bool json::add_if_hex(char* arr, int index, char c)
{
	if (c < 0)
	{
		// Invalid data
		return false;
	}

	if (c != 'A' && c != 'a' && c != 'B' && c != 'b' && c != 'C' && c != 'c' && c != 'D' && c != 'd' && c != 'E' &&
		c != 'e' && c != 'F' && c != 'f' && !std::isdigit(c))
	{
		return false;
	}

	arr[index] = c;
	return true;
}

bool json::parse_unicode_hex_quad(data_iterator& it, char32_t& codepointOut)
{
	int i{ 0 };
	char hex[5]{ 0 };

	for (int j{ 0 }; j < 4; j++)
	{
		if (!add_if_hex(hex, i++, it.peek()))
		{
			build_error(parsing_error_type::expected_utf_character, it);
			return false;
		}

		it.offset++;
	}

	codepointOut = static_cast<char32_t>(std::stoul(hex, nullptr, 16));
	return true;
}

std::string json::codepoint_to_utf8(char32_t c)
{
	std::mbstate_t state{};
	char mbBuf[MB_LEN_MAX]{ 0 };
	auto len = std::c32rtomb(mbBuf, c, &state);
	if (len == std::size_t(-1))
	{
		len = std::c32rtomb(mbBuf, U'\uFFFD', &state);
		if (len == std::size_t(-1))
		{
			throw std::runtime_error("UTF ERROR");
		}
	}

	mbBuf[len] = 0;
	return std::string(mbBuf);
}
