#include "json_elements.h"
#include <cstdint>
#include <string>

using namespace strata::json;

static std::string calculate_indentation(bool pretty_print, int indentation)
{
	if (!pretty_print)
	{
		return std::move(std::string(""));
	}

	return std::move(std::string((size_t)4 * (size_t)indentation, ' '));
}

// ARRAY DEFINITIONS

array::array(const array& other)
{
	m_elements.clear();
	m_elements.resize(other.m_elements.size());
	for (size_t i = 0; i < other.m_elements.size(); ++i)
	{
		m_elements[i] = std::make_unique<value>(*other.m_elements[i]);
	}
}

array::array(array&& other) noexcept : m_elements{ std::move(other.m_elements) }
{
}

array& array::push_back(const value& value)
{
	m_elements.push_back(std::make_unique<json::value>(value));
	return *this;
}

array& array::push_back(value&& value)
{
	m_elements.push_back(std::make_unique<json::value>(std::move(value)));
	return *this;
}

array& array::push_back()
{
	emplace_back();
	return *this;
}

value& array::emplace_back()
{
	return *m_elements.emplace_back(std::make_unique<value>());
}

array& array::pop_back()
{
	m_elements.pop_back();
	return *this;
}

value& array::back()
{
	return *m_elements.back();
}

const value& array::back() const
{
	return *m_elements.back();
}

void array::clear()
{
	return m_elements.clear();
}

size_t array::size() const
{
	return m_elements.size();
}

bool array::empty() const
{
	return m_elements.empty();
}

value& array::at(size_t index)
{
	return *m_elements.at(index).get();
}

const value& array::at(size_t index) const
{
	return *m_elements.at(index).get();
}

array& array::operator=(const array& other)
{
	m_elements.clear();
	m_elements.resize(other.m_elements.size());
	for (size_t i = 0; i < other.m_elements.size(); ++i)
	{
		m_elements[i] = std::make_unique<value>(*other.m_elements[i]);
	}

	return *this;
}

value& array::operator[](size_t index)
{
	while (index >= m_elements.size())
	{
		emplace_back();
	}

	return *m_elements[index];
}

const value& array::operator[](size_t index) const
{
	if (index >= m_elements.size())
	{
		return value::null;
	}

	return *m_elements[0];
}

bool array::operator==(const array& other) const
{
	if (m_elements.size() != other.size())
	{
		return false;
	}

	return serialize() == other.serialize();
}

string array::serialize(bool pretty_print, int indentation) const
{
	if (indentation < 1)
	{
		indentation = 1;
	}

	std::string indent = calculate_indentation(pretty_print, indentation);
	std::string result = pretty_print ? "[\n" : "[";
	for (size_t i{ 0 }; i < m_elements.size(); ++i)
	{
		result += indent + m_elements[i]->serialize(pretty_print, indentation + 1);
		if (i + 1 < m_elements.size())
		{
			result += ", ";
		}

		if (pretty_print)
		{
			result += "\n";
		}
	}

	return result + calculate_indentation(pretty_print, indentation - 1) + "]";
}

array::const_iterator array::begin() const
{
	return m_elements.begin();
}

array::const_iterator array::end() const
{
	return m_elements.end();
}

array::const_iterator array::cbegin() const
{
	return m_elements.cbegin();
}

array::const_iterator array::cend() const
{
	return m_elements.cend();
}

array::const_iterator::const_iterator(value_list::const_iterator it) 
	: m_it(it)
{
}

array::const_iterator& array::const_iterator::operator++()
{
	++m_it;
	return *this;
}

bool array::const_iterator::operator==(const const_iterator& other) const
{
	return m_it == other.m_it;
}

bool array::const_iterator::operator!=(const const_iterator& other) const
{
	return !(*this == other);
}

array::const_iterator::result array::const_iterator::operator*() const
{
	return **m_it;
}

array::iterator& array::iterator::operator++()
{
	const_iterator::operator++();
	return *this;
}

array::iterator::result array::iterator::operator*()
{
	return **m_it;
}

// OBJECT IMPLEMENTATION

object::object(object&& other) noexcept : m_elements{ std::move(other.m_elements) }
{
}

object::object(const object& other)
{
	*this = other;
}

void object::insert(const std::string& key, value&& element)
{
	m_elements.insert(std::make_pair(key, std::make_unique<value>(std::move(element))));
}

void object::insert(const std::string& key, const value& element)
{
	m_elements.insert(std::make_pair(key, std::make_unique<value>(element)));
}

bool object::contains(const std::string& key) const
{
	return m_elements.find(key) != m_elements.end();
}

bool object::contains_object(const std::string& key) const
{
	auto it = m_elements.find(key);
	if (it == m_elements.end())
	{
		return false;
	}

	return it->second->is_object();
}

bool object::contains_array(const std::string& key) const
{
	auto it = m_elements.find(key);
	if (it == m_elements.end())
	{
		return false;
	}

	return it->second->is_array();
}

size_t object::size() const
{
	return m_elements.size();
}

value& object::operator[](const std::string& key)
{
	auto it = m_elements.find(key);
	if (it == m_elements.end())
	{
		it = m_elements.insert(it, std::make_pair(key, std::make_unique<value>()));
	}

	return *it->second;
}

value& object::operator[](const char* key)
{
	return this->operator[](std::string(key));
}

const value& object::operator[](const std::string& key) const
{
	auto it = m_elements.find(key);
	if (it == m_elements.end())
	{
		return value::null;
	}

	return *it->second;
}

const value& object::operator[](const char* key) const
{
	auto it = m_elements.find(key);
	if (it == m_elements.end())
	{
		return value::null;
	}

	return *it->second;
}

object& object::operator=(const object& other)
{
	m_elements.clear();
	m_elements.reserve(other.m_elements.bucket_count());

	for (const auto& bucket : other.m_elements)
	{
		m_elements.emplace(bucket.first, std::make_unique<value>(*bucket.second));
	}

	return *this;
}

bool object::operator==(const object& other) const
{
	if (m_elements.size() != other.m_elements.size())
	{
		return false;
	}

	return serialize() == other.serialize();
}

value& object::at(const std::string& key)
{
	return *m_elements.at(key);
}

value& object::at(const char* key)
{
	return *m_elements.at(key);
}

const value& object::at(const std::string& key) const
{
	return *m_elements.at(key);
}

const value& object::at(const char* key) const
{
	return *m_elements.at(key);
}

string object::serialize(bool pretty_print, int indentation) const
{
	if (indentation < 1)
	{
		indentation = 1;
	}

	std::string indent = calculate_indentation(pretty_print, indentation);
	std::string objString = pretty_print ? "{\n" : "{";
	for (auto i = m_elements.begin(); i != m_elements.end(); ++i)
	{
		objString += indent + "\"" + i->first + "\": " + i->second->serialize(pretty_print, indentation + 1);
		if (std::next(i) != m_elements.end())
		{
			objString += ",";
		}

		if (pretty_print)
		{
			objString += "\n";
		}
	}

	return objString + calculate_indentation(pretty_print, indentation - 1) + "}";
}

bool object::empty() const
{
	return m_elements.empty();
}

object::iterator object::begin()
{
	return m_elements.begin();
}

object::iterator object::end()
{
	return m_elements.end();
}

object::const_iterator object::begin() const
{
	return m_elements.begin();
}

object::const_iterator object::end() const
{
	return m_elements.end();
}

object::const_iterator object::cbegin() const
{
	return m_elements.cbegin();
}

object::const_iterator object::cend() const
{
	return m_elements.cend();
}

object::const_iterator::const_iterator(value_map::const_iterator it)
	: m_it(it)
{
}

object::const_iterator& object::const_iterator::operator++()
{
	++m_it;
	return *this;
}

bool object::const_iterator::operator==(const const_iterator& other) const
{
	return m_it == other.m_it;
}

bool object::const_iterator::operator!=(const const_iterator& other) const
{
	return !(*this == other);
}

object::const_iterator::result object::const_iterator::operator*() const
{
	const auto& inner_pair = *m_it;
	return { inner_pair.first, *inner_pair.second };
}

object::iterator& object::iterator::operator++()
{
	const_iterator::operator++();
	return *this;
}

object::iterator::result object::iterator::operator*()
{
	const auto& inner_pair = *m_it;
	return { inner_pair.first, *inner_pair.second };
}

const value value::null = {};

value::value(integer value) : value((number)value)
{
}

value::value(int value) : value((integer)value)
{
}

value::value(size_t value) : value((integer)value)
{
}

value::value(float value) : value((number)value)
{
}

value::value(number value) : m_value{ value }
{
}

value::value(boolean value) : m_value{ value }
{
}

value::value(const char* value) : value(std::string(value))
{
}

value::value(const std::string& value) : value(std::string(value))
{
}

value::value(std::string&& value) : m_value{ value }
{
}

value::value(const std::string_view value) : value(std::string(value))
{
}

value::value(array&& value) : m_value{ std::move(value) }
{
}

value::value(object&& value) : m_value{ std::move(value) }
{
}

value::value()
{
	*this = null;
}

value::value(value&& other) noexcept : m_value{ std::move(other.m_value) }
{
	other.m_value = nullptr;
}

value::value(const value& other)
{
	// Explicit copy
	m_value = json_variant(other.m_value);
}

boolean value::is_string() const
{
	return std::holds_alternative<string>(m_value);
}

const string& value::as_string() const
{
	if (auto b = std::get_if<string>(&m_value))
	{
		return *b;
	}

	static string empty = "";
	return empty;
}

boolean value::is_number() const
{
	return std::holds_alternative<number>(m_value);
}

number value::as_number() const
{
	if (auto b = std::get_if<number>(&m_value))
	{
		return *b;
	}

	return 0;
}

int64_t value::as_integer() const
{
	return (int64_t)as_number();
}

boolean value::is_boolean() const
{
	return std::holds_alternative<boolean>(m_value);
}

boolean value::as_boolean() const
{
	if (auto b = std::get_if<boolean>(&m_value))
	{
		return *b;
	}

	return false;
}

boolean value::is_null() const
{
	return std::holds_alternative<std::nullptr_t>(m_value);
}

boolean value::is_array() const
{
	return std::holds_alternative<array>(m_value);
}

array& value::as_array()
{
	return std::get<array>(m_value);
}

const array& value::as_array() const
{
	return std::get<array>(m_value);
}

boolean value::is_object() const
{
	return std::holds_alternative<object>(m_value);
}

object& value::as_object()
{
	return std::get<object>(m_value);
}

const object& value::as_object() const
{
	return std::get<object>(m_value);
}

boolean value::contains(const string& key) const
{
	if (!is_object())
	{
		return false;
	}

	return as_object().contains(key);
}

boolean value::contains_array(const string& key) const
{
	if (!is_object())
	{
		return false;
	}

	return as_object().contains_array(key);
}

boolean value::contains_object(const string& key) const
{
	if (!is_object())
	{
		return false;
	}

	return as_object().contains_object(key);
}

value& value::operator=(const value& other)
{
	// Explicit copy
	m_value = json_variant(other.m_value);
	return *this;
}

value& value::operator=(value&& other) noexcept
{
	m_value = std::move(other.m_value);
	other.m_value = nullptr;
	return *this;
}

value& value::operator=(const array& value)
{
	m_value = value;
	return *this;
}

value& value::operator=(array&& value) noexcept
{
	m_value = { std::move(value) };
	return *this;
}

value& value::operator=(const object& value)
{
	m_value = value;
	return *this;
}

value& value::operator=(object&& value) noexcept
{
	m_value = { std::move(value) };
	return *this;
}

bool value::operator==(boolean v) const
{
	if (!is_boolean())
	{
		return false;
	}

	return as_boolean() == v;
}

bool value::operator==(number v) const
{
	if (!is_number())
	{
		return false;
	}

	return as_number() == v;
}

bool value::operator==(integer v) const
{
	if (!is_number())
	{
		return false;
	}

	return as_integer() == v;
}

bool value::operator==(const char* v) const
{
	if (!is_string())
	{
		return false;
	}

	return as_string() == v;
}

bool value::operator==(const string& v) const
{
	if (!is_string())
	{
		return false;
	}

	return as_string() == v;
}

bool value::operator==(const value& v) const
{
	if (m_value.index() != v.m_value.index())
	{
		return false;
	}

	return m_value == v.m_value;
}

value& value::operator[](size_t index)
{
	if (!is_array())
	{
		*this = array();
	}

	return as_array()[index];
}

const value& value::operator[](size_t index) const
{
	if (!is_array())
	{
		return null;
	}

	return as_array()[index];
}

value& value::operator=(const char* value)
{
	m_value = std::string{ value };
	return *this;
}

value& value::operator=(const string& value)
{
	m_value = value;
	return *this;
}

value& value::operator=(string&& value) noexcept
{
	m_value = { std::move(value) };
	return *this;
}

value& value::operator=(std::string_view value)
{
	m_value = std::string{ value };
	return *this;
}

value& value::operator=(boolean value)
{
	m_value = value;
	return *this;
}

value& value::operator=(integer value)
{
	m_value = (number)value;
	return *this;
}

value& value::operator=(number value)
{
	m_value = value;
	return *this;
}

string value::serialize(bool pretty_print, int indentation) const
{
	if (auto b = std::get_if<std::nullptr_t>(&m_value))
	{
		return "null";
	}

	if (auto b = std::get_if<number>(&m_value))
	{
		return std::to_string(*b);
	}

	if (auto b = std::get_if<boolean>(&m_value))
	{
		return *b ? "true" : "false";
	}

	if (auto b = std::get_if<string>(&m_value))
	{
		std::string output;
		const std::string& val = as_string();
		for (int i{ 0 }; i < val.size(); i++)
		{
			switch (val[i])
			{
			case '\b':
				output += "\\b";
				break;
			case '\f':
				output += "\\f";
				break;
			case '\n':
				output += "\\n";
				break;
			case '\r':
				output += "\\r";
				break;
			case '\t':
				output += "\\t";
				break;
			case '\\':
				output += "\\\\";
				break;
			case '/':
				output += "\\/";
				break;
			case '\"':
				output += "\\\"";
				break;
			default:
				output += val[i];
			}
		}

		return "\"" + output + "\"";
	}

	if (auto b = std::get_if<array>(&m_value))
	{
		return b->serialize(pretty_print, indentation);
	}

	if (auto b = std::get_if<object>(&m_value))
	{
		return b->serialize(pretty_print, indentation);
	}

	throw json_exception("Unkown value type");
}

value& value::operator[](const std::string& key)
{
	if (!is_object())
	{
		*this = object{};
	}

	return as_object()[key];
}

value& value::operator[](const char* key)
{
	if (!is_object())
	{
		*this = object{};
	}

	return as_object()[key];
}

const value& value::operator[](const std::string& key) const
{
	if (!is_object())
	{
		return null;
	}

	return as_object()[key];
}

const value& value::operator[](const char* key) const
{
	if (!is_object())
	{
		return null;
	}

	return as_object()[key];
}

value::operator boolean() const
{
	return as_boolean();
}

value::operator number() const
{
	return as_number();
}

value::operator int() const
{
	return (int)as_number();
}

value::operator size_t() const
{
	return (size_t)as_number();
}

value::operator int64_t() const
{
	return (int64_t)as_number();
}

value::operator const string& () const
{
	return as_string();
}

value::operator const object& () const
{
	return as_object();
}

value::operator const array& () const
{
	return as_array();
}
