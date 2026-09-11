#pragma once

#ifndef _H_ANY_SERIALIZER_REGISTRY_
#define _H_ANY_SERIALIZER_REGISTRY_

#include "json_elements.h"
#include "json_serialization.h"

#include <any>
#include <functional>
#include <list>
#include <map>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace strata::json::serialization
{
	class AnySerializerRegistry
	{
	public:
		using Serializer = std::function<strata::json::value(const std::any&)>;

		template <typename T> static void RegisterType()
		{
			_serializers[std::type_index(typeid(T))] = [](const std::any& a) {
				return strata::json::serialization::Serialize<T>(std::any_cast<const T&>(a));
				};
		}

		static value Serialize(const std::any& source)
		{
			auto it = _serializers.find(std::type_index(source.type()));
			if (it == _serializers.end())
			{
				throw std::runtime_error(std::string("No serializer registered for ") + source.type().name());
			}

			return it->second(source);
		}

	private:
		inline static std::unordered_map<std::type_index, Serializer> _serializers;
	};

	template <> struct JSerializer<std::any>
	{
		static strata::json::value Serialize(const std::any& source)
		{
			return AnySerializerRegistry::Serialize(source);
		}

		static std::any Deserialize(const value& element)
		{
			if (element.is_object())
			{
				return strata::json::serialization::Deserialize<std::map<std::string, std::any>>(element);
			}
			else if (element.is_array())
			{
				return strata::json::serialization::Deserialize<std::vector<std::any>>(element);
			}
			else
			{
				return element.serialize();
			}
		}
	};
} // namespace strata::json::serialization

#endif // !_H_ANY_SERIALIZER_REGISTRY_
