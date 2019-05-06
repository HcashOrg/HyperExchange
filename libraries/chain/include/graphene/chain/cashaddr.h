// Copyright (c) 2017 Pieter Wuille
// Copyright (c) 2017 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Cashaddr is an address format inspired by bech32.

#include <cstdint>
#include <string>
#include <vector>
namespace cashaddr {
	typedef std::vector<uint8_t> data;
	const int8_t GET_CHARSET_REV(uint8_t i);
	const char GET_CHARSET(uint8_t i);
	/**
	 * Concatenate two byte arrays.
	 */
	data Cat(data x, const data &y);

	/**
	 * This function will compute what 8 5-bit values to XOR into the last 8 input
	 * values, in order to make the checksum 0. These 8 values are packed together
	 * in a single 40-bit integer. The higher bits correspond to earlier values.
	 */
	uint64_t PolyMod(const data &v);

	/**
	 * Convert to lower case.
	 *
	 * Assume the input is a character.
	 */
	inline uint8_t LowerCase(uint8_t c);

	/**
	 * Expand the address prefix for the checksum computation.
	 */
	data ExpandPrefix(const std::string &prefix);

	/**
	 * Verify a checksum.
	 */
	bool VerifyChecksum(const std::string &prefix, const data &payload);

	/**
	 * Create a checksum.
	 */
	data CreateChecksum(const std::string &prefix, const data &payload);
/**
 * Encode a cashaddr string. Returns the empty string in case of failure.
 */
std::string Encode(const std::string &prefix,
                   const std::vector<uint8_t> &values);

/**
 * Decode a cashaddr string. Returns (prefix, data). Empty prefix means failure.
 */
std::pair<std::string, std::vector<uint8_t>>
Decode(const std::string &str, const std::string &default_prefix);
} // namespace cashaddr
