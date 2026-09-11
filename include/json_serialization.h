#pragma once

#ifndef _H_JSERIALIZER_
#define _H_JSERIALIZER_

#include "json.h"
#include "json_elements.h"

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace strata::json::serialization
{
	// ------------------------------------------------------------------
	// JSerializer<T> is the customization point for the whole system.
	//
	// A specialization must provide:
	//   static json::value Serialize(const T& value);
	//   static T Deserialize(const json::json_proxy& element);
	//
	template <typename T> struct JSerializer; // deliberately undefined: using an unspecialized T for a clear compile error

	template <typename T> inline value Serialize(const T& value)
	{
		return JSerializer<T>::Serialize(value);
	}

	template <typename T> inline T Deserialize(const value& element)
	{
		return JSerializer<T>::Deserialize(element);
	}

	// BUILT IN TYPES
	template <> struct JSerializer<int>
	{
		static value Serialize(int source)
		{
			return source;
		}
		static int Deserialize(const value& element)
		{
			return (int)element.as_integer();
		}
	};

	template <> struct JSerializer<int64_t>
	{
		static value Serialize(int64_t source)
		{
			return source;
		}
		static int64_t Deserialize(const value& element)
		{
			return (int64_t)element.as_integer();
		}
	};

	template <> struct JSerializer<bool>
	{
		static value Serialize(bool source)
		{
			return source;
		}
		static bool Deserialize(const value& element)
		{
			return element.as_boolean();
		}
	};

	template <> struct JSerializer<double>
	{
		static value Serialize(double source)
		{
			return source;
		}
		static double Deserialize(const value& element)
		{
			return element.as_number();
		}
	};

	template <> struct JSerializer<float>
	{
		static value Serialize(float source)
		{
			return source;
		}
		static float Deserialize(const value& element)
		{
			return (float)element.as_number();
		}
	};

	template <> struct JSerializer<std::string>
	{
		static value Serialize(const std::string& source)
		{
			return source;
		}
		static std::string Deserialize(const value& element)
		{
			return element.as_string();
		}
	};

	template <typename T> struct JSerializer<std::vector<T>>
	{
		static value Serialize(const std::vector<T>& source)
		{
			auto array = strata::json::array{};
			for (const auto& element : source)
			{
				array.push_back(std::move(strata::json::serialization::Serialize(element)));
			}

			return array;
		}

		static std::vector<T> Deserialize(const value& element)
		{
			if (!element.is_array())
			{
				throw std::runtime_error("Deserialize<vector<T>>: element is not an array");
			}

			const array& array = element;
			std::vector<T> result;
			result.reserve(array.size());
			for (const auto& item : array)
			{
				result.push_back(std::move(strata::json::serialization::Deserialize<T>(item)));
			}

			return result;
		}
	};

	template <typename T> struct JSerializer<std::unordered_set<T>>
	{
		static value Serialize(const std::unordered_set<T>& source)
		{
			auto array = strata::json::array{};
			for (const auto& element : source)
			{
				array.push_back(std::move(strata::json::serialization::Serialize(element)));
			}

			return array;
		}

		static std::unordered_set<T> Deserialize(const value& element)
		{
			if (!element.is_array())
			{
				throw std::runtime_error("Deserialize<vector<T>>: element is not an array");
			}

			const array& array = element;
			std::unordered_set<T> result;
			for (const auto& item : array)
			{
				result.insert(std::move(strata::json::serialization::Deserialize<T>(item)));
			}

			return result;
		}
	};

	template <typename T> struct JSerializer<std::map<std::string, T>>
	{
		static value Serialize(const std::map<std::string, T>& source)
		{
			auto object = strata::json::object{};
			for (const auto& [key, value] : source)
			{
				object.insert(key, std::move(strata::json::serialization::Serialize(value)));
			}

			return object;
		}

		static std::map<std::string, T> Deserialize(const value& element)
		{
			if (!element.is_object())
			{
				throw std::runtime_error("Deserialize<map<string,T>>: element is not an object");
			}

			const object& object = element;
			std::map<std::string, T> result;
			for (const auto& [key, value] : object)
			{
				result.emplace(key, std::move(strata::json::serialization::Deserialize<T>(value)));
			}

			return result;
		}
	};

	template <typename T> struct JSerializer<std::unordered_map<std::string, T>>
	{
		static value Serialize(const std::unordered_map<std::string, T>& source)
		{
			auto object = strata::json::object{};
			for (const auto& [key, value] : source)
			{
				object.insert(key, std::move(strata::json::serialization::Serialize(value)));
			}

			return object;
		}

		static std::unordered_map<std::string, T> Deserialize(const value& element)
		{
			if (!element.is_object())
			{
				throw std::runtime_error("Deserialize<map<string,T>>: element is not an object");
			}

			const object& object = element;
			std::unordered_map<std::string, T> result;
			for (const auto& [key, value] : object)
			{
				result.emplace(key, std::move(strata::json::serialization::Deserialize<T>(value)));
			}

			return result;
		}
	};
} // namespace strata::json::serialization

#endif // !_H_JSERIALIZER_
