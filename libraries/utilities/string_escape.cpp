/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/utilities/string_escape.hpp>
#include <sstream>
#include <assert.h>
#include <fc/string.hpp>
namespace graphene { namespace utilities {

  std::string escape_string_for_c_source_code(const std::string& input)
  {
    std::ostringstream escaped_string;
    escaped_string << "\"";
    for (unsigned i = 0; i < input.size(); ++i)
    {
      switch (input[i])
      {
      case '\a': 
        escaped_string << "\\a";
        break;
      case '\b': 
        escaped_string << "\\b";
        break;
      case '\t': 
        escaped_string << "\\t";
        break;
      case '\n': 
        escaped_string << "\\n";
        break;
      case '\v': 
        escaped_string << "\\v";
        break;
      case '\f': 
        escaped_string << "\\f";
        break;
      case '\r': 
        escaped_string << "\\r";
        break;
      case '\\': 
        escaped_string << "\\\\";
        break;
      case '\"': 
        escaped_string << "\\\"";
        break;
      default:
        escaped_string << input[i];
      }
    }
    escaped_string << "\"";
    return escaped_string.str();
  }
  
  std::string remove_zero_for_str_amount(const std::string& input)
  {
	  auto pos = input.find(".");
	  if (std::string::npos == pos)
		  return input;
	  auto temp = input;
	  auto end = input.size()-1;
	  for (; end > pos; end--)
	  {
		  if (input[end] == '0')
		  {
			  continue;
		  }
		  break;
	  }
	  if (end == pos)
		  return temp.substr(0, end);
	  
	  return temp.substr(0, end + 1);
  }
  std::string amount_to_string(uint64_t amount, int precision)
  {
	  uint64_t scaled_precision = 1;
	  for (uint8_t i = 0; i < precision; ++i)
		  scaled_precision *= 10;
	  assert(scaled_precision > 0);

	  std::string result = fc::to_string(amount / scaled_precision);
	  auto decimals = amount % scaled_precision;
	  if (decimals)
		  result += "." + fc::to_string(scaled_precision + decimals).erase(0, 1);
	  return result;
  }

} } // end namespace graphene::utilities

