#pragma once

#ifndef _H_JSON_PARSER_
#define _H_JSON_PARSER_

#include "json_elements.h"
#include "span.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <istream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace strata::json
{
	using parsing_errors = std::vector<std::string>;
	enum class parsing_error_type
	{
		unknown = 0,
		expected_null,
		expected_bool,

		unexpected_character_in_file,

		unexpected_eof,
		unexpected_comma,
		unexpected_char_terminator,
		unexpected_newline,
		unexpected_tab_character,
		unpaired_surrogate,

		expected_start_of_array,
		expected_start_of_object,
		expected_end_of_array,
		expected_end_of_object,
		expected_colon_marker,
		expected_utf_character,
		unexpected_escape_character,
		expected_data,

		expected_quotes,
		leftover_characters_in_data,

		// Number errors
		not_a_number,
		number_cannot_start_with_zero,
		number_cannot_end_with_decimal_separator,
		number_cannot_end_with_exponential_character,
		number_cannot_end_with_sign,

		max_json_depth_reached,

		no_error,
	};

	class json
	{
	public:
		static size_t MAX_PARSING_DEPTH;

		static value from_text(const std::string& text, parsing_errors* errors);
		static value from_text(const std::string& text);
		static value from_file(const std::filesystem::path& path, parsing_errors* errors);
		static value from_file(const std::filesystem::path& path);
		static value from_stream(std::istream& stream, parsing_errors* errors);
		static value from_stream(std::istream& stream);
		static value from_memory(std::vector<uint8_t>& stream, parsing_errors* errors);
		static value from_memory(std::vector<uint8_t>& stream);

		parsing_errors& get_last_errors();
		void clear_errors();

	private:
		json() = default;

		parsing_errors m_errors;
		size_t m_depth{ 0 };

	private:
		struct data_iterator
		{
			explicit data_iterator(span<const uint8_t> d) : data(d), offset{ 0 }
			{
			}

			char read1()
			{
				return static_cast<char>(data[offset++]);
			}

			bool is_valid() const
			{
				return offset < data.size();
			}

			size_t remaining() const
			{
				return data.size() - offset;
			}

			void skip_wspace();

			char peek(size_t lookahead = 0) const
			{
				if (offset + lookahead >= data.size())
					return EOF;

				return static_cast<char>(data[offset + lookahead]);
			}

			size_t offset;
			span<const uint8_t> data;
		};

	private:
		static value from_iterator(data_iterator& it, parsing_errors* errors);
		static std::string get_err_template(parsing_error_type type);
		void build_error(parsing_error_type type, data_iterator& data);

		value parse_value(data_iterator& it);
		value parse_boolean(data_iterator& it);
		value parse_number(data_iterator& it);
		int parse_string(data_iterator& it, std::string& result);

		array parse_array(data_iterator& it);
		object parse_object(data_iterator& it);

		parsing_error_type is_valid_number(const std::string& value);

		/// returns a tuple with the json element name and json element pointer
		std::tuple<std::string, value> parse_member(data_iterator& it);

		bool get_escaped_character(data_iterator& it, std::string& escapedCharacters);
		bool add_if_hex(char*, int, char);
		bool parse_unicode_hex_quad(data_iterator& it, char32_t& codepointOut);
		std::string codepoint_to_utf8(char32_t c);
	};
} // namespace strata::json

#endif // !_H_JSON_PARSER_

#ifdef STRATA_JSOIN_IMPL

#include "..\src\json.cpp"
#include "..\src\json_elements.cpp"

#endif // STRATA_JSOIN_IMPL
