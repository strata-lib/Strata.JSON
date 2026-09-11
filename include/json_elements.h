#pragma once

#ifndef _H_JSON_ELEMENTS_
#define _H_JSON_ELEMENTS_

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace strata
{
	namespace json
	{
		class json_exception : public std::runtime_error
		{
		public:
			json_exception() = default;
			json_exception(const std::string& message) : std::runtime_error(message)
			{
			}
		};

		class value;

		using value_ptr = std::unique_ptr<value>;
		using value_list = std::vector<value_ptr>;
		using value_map = std::unordered_map<std::string, value_ptr>;

		using number = double;
		using integer = int64_t;
		using boolean = bool;
		using string = std::string;

		class array
		{
		public:
			class const_iterator;
			class iterator;

			array() = default;
			array(array&&) noexcept;
			array(const array&);

			array& push_back(const value& value);
			array& push_back(value&& value);
			array& push_back();
			value& emplace_back();
			array& pop_back();
			value& back();
			const value& back() const;

			void clear();
			size_t size() const;
			bool empty() const;

			value& at(size_t index);
			const value& at(size_t index) const;

			array& operator=(const array& other);
			value& operator[](size_t index);
			const value& operator[](size_t index) const;

			bool operator==(const array& other) const;
			bool operator!=(const array& other) const
			{
				return !(*this == other);
			}

			string serialize(bool pretty_print = false, int indentation = 1) const;

			const_iterator begin() const;
			const_iterator end() const;

			const_iterator cbegin() const;
			const_iterator cend() const;

			class const_iterator
			{
			public:
				using result = const value&;

			public:
				const_iterator(value_list::const_iterator it);
				~const_iterator() = default;

				const_iterator& operator++();
				bool operator==(const const_iterator& other) const;
				bool operator!=(const const_iterator& other) const;

				result operator*() const;

			protected:
				value_list::const_iterator m_it;
			};

			class iterator : public const_iterator
			{
			public:
				using result = value&;

			public:
				using const_iterator::const_iterator;
				~iterator() = default;

				iterator& operator++();
				result operator*();
			};

		private:
			value_list m_elements;
		};

		class object
		{
		public:
			class const_iterator;
			class iterator;

			object() = default;
			object(object&&) noexcept;
			object(const object&);

			void insert(const std::string& key, value&& element);
			void insert(const std::string& key, const value& element);
			bool contains(const std::string& key) const;
			bool contains_object(const std::string& key) const;
			bool contains_array(const std::string& key) const;

			size_t size() const;

			value& operator[](const std::string& key);
			value& operator[](const char* key);
			const value& operator[](const std::string& key) const;
			const value& operator[](const char* key) const;
			object& operator=(const object& other);

			bool operator==(const object& other) const;
			bool operator!=(const object& other) const
			{
				return !(*this == other);
			}

			value& at(const std::string& key);
			value& at(const char* key);
			const value& at(const std::string& key) const;
			const value& at(const char* key) const;

			string serialize(bool pretty_print = false, int indentation = 1) const;

			bool empty() const;

			iterator begin();
			iterator end();

			const_iterator begin() const;
			const_iterator end() const;
			const_iterator cbegin() const;
			const_iterator cend() const;

		public:
			class const_iterator
			{
			public:
				using result = std::pair<std::string_view, const value&>;

			public:
				const_iterator(value_map::const_iterator it);
				~const_iterator() = default;

				const_iterator& operator++();
				bool operator==(const const_iterator& other) const;
				bool operator!=(const const_iterator& other) const;

				result operator*() const;

			protected:
				value_map::const_iterator m_it;
			};

			class iterator : public const_iterator
			{
			public:
				using result = std::pair<std::string_view, value&>;

			public:
				using const_iterator::const_iterator;
				~iterator() = default;

				iterator& operator++();
				result operator*();
			};

		private:
			value_map m_elements;
		};

		using json_variant = std::variant<std::nullptr_t, number, boolean, string, array, object>;

		class value
		{
		public:
			static const value null;

			value(integer value);
			value(int value);
			value(size_t value);
			value(float value);
			value(number value);
			value(boolean value);
			value(const char* value);
			value(const std::string& value);
			value(std::string&& value);
			value(const std::string_view value);
			value(array&& value);
			value(object&& value);
			value();
			value(value&& other) noexcept;
			value(const value& other);

			boolean is_string() const;
			const string& as_string() const;

			boolean is_number() const;
			number as_number() const;
			int64_t as_integer() const;

			boolean is_boolean() const;
			boolean as_boolean() const;

			boolean is_null() const;

			boolean is_array() const;
			array& as_array();
			const array& as_array() const;

			boolean is_object() const;
			object& as_object();
			const object& as_object() const;

			boolean contains(const string& key) const;
			boolean contains_object(const string& key) const;
			boolean contains_array(const string& key) const;

			value& operator=(const char*);
			value& operator=(const string&);
			value& operator=(string&&) noexcept;
			value& operator=(std::string_view);
			value& operator=(boolean);
			value& operator=(integer);
			value& operator=(number);
			value& operator=(const value&);
			value& operator=(value&&) noexcept;
			value& operator=(const array&);
			value& operator=(array&&) noexcept;
			value& operator=(const object&);
			value& operator=(object&&) noexcept;

			bool operator==(boolean v) const;
			bool operator!=(boolean v) const
			{
				return !(*this == v);
			}
			bool operator==(number v) const;
			bool operator!=(number v) const
			{
				return !(*this == v);
			}
			bool operator==(integer v) const;
			bool operator!=(integer v) const
			{
				return !(*this == v);
			}
			bool operator==(const char* v) const;
			bool operator!=(const char* v) const
			{
				return !(*this == v);
			}
			bool operator==(const string& v) const;
			bool operator!=(const string& v) const
			{
				return !(*this == v);
			}
			bool operator==(const value& v) const;
			bool operator!=(const value& v) const
			{
				return !(*this == v);
			}

			value& operator[](size_t index);
			value& operator[](int index)
			{
				return this->operator[](static_cast<size_t>(index));
			}
			const value& operator[](size_t index) const;
			const value& operator[](int index) const
			{
				return this->operator[](static_cast<size_t>(index));
			}

			value& operator[](const std::string& key);
			value& operator[](const char* key);
			const value& operator[](const std::string& key) const;
			const value& operator[](const char* key) const;

			operator boolean() const;
			operator number() const;
			operator int() const;
			operator size_t() const;
			operator int64_t() const;
			operator const string& () const;
			operator const object& () const;
			operator const array& () const;

			string serialize(bool pretty_print = false, int indentation = 1) const;

		private:
			json_variant m_value;
		};
	} // namespace json
} // namespace strata

#endif // !_H_value_
