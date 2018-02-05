#pragma once
#include <string>
#include <fc/io/json.hpp>

namespace graphene {
	namespace utilities {
		std::string json_ordered_dumps(const fc::variant& value);
	}
}