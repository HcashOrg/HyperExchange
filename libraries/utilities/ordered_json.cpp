#include <graphene/utilities/ordered_json.hpp>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>
#include <sstream>
#include <list>

namespace graphene {
	namespace utilities {
		std::string json_ordered_dumps(const fc::variant& value)
		{
			if (value.is_object())
			{
				std::stringstream ss;
				ss << "{";
				const auto& json_obj = value.as<fc::mutable_variant_object>();
				std::list<std::string> keys;
				for (auto it = json_obj.begin(); it != json_obj.end(); it++)
				{
					keys.push_back(it->key());
				}
				keys.sort();
				bool is_first = true;
				for (const auto& key : keys)
				{
					if (!is_first)
						ss << ",";
					is_first = false;
					ss << fc::json::to_string(key) << ":" << json_ordered_dumps(json_obj[key]);
				}
				ss << "}";
				return ss.str();
			}
			else
			{
				return fc::json::to_string(value);
			}
		}
	}
}
