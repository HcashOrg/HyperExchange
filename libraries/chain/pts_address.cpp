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
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/pts_address.hpp>
#include <graphene/utilities/hash.hpp>

#include <fc/crypto/base58.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <algorithm>
#include <graphene/chain/cashaddr.h>

namespace graphene { namespace chain {

   pts_address::pts_address()
   {
      memset( addr.data, 0, sizeof(addr.data) );
   }

   pts_address::pts_address( const std::string& base58str )
   {
      std::vector<char> v = fc::from_base58( fc::string(base58str) );
      if( v.size() )
         memcpy( addr.data, v.data(), std::min<size_t>( v.size(), sizeof(addr) ) );

      if( !is_valid() )
      {
         FC_THROW_EXCEPTION( invalid_pts_address, "invalid pts_address ${a}", ("a", base58str) );
      }
   }

   pts_address::pts_address( const fc::ecc::public_key& pub, bool compressed, uint8_t version )
   {
       fc::sha256 sha2;
       if( compressed )
       {
           auto dat = pub.serialize();
           sha2     = fc::sha256::hash(dat.data, sizeof(dat) );
       }
       else
       {
           auto dat = pub.serialize_ecc_point();
           sha2     = fc::sha256::hash(dat.data, sizeof(dat) );
       }
       auto rep      = fc::ripemd160::hash((char*)&sha2,sizeof(sha2));
       addr.data[0]  = version;
       memcpy( addr.data+1, (char*)&rep, sizeof(rep) );
       auto check    = fc::sha256::hash( addr.data, sizeof(rep)+1 );
       check = fc::sha256::hash(check);
       memcpy( addr.data+1+sizeof(rep), (char*)&check, 4 );
   }

   /**
    *  Checks the address to verify it has a
    *  valid checksum
    */
   bool pts_address::is_valid()const
   {
       auto check    = fc::sha256::hash( addr.data, sizeof(fc::ripemd160)+1 );
       check = fc::sha256::hash(check);
       return memcmp( addr.data+1+sizeof(fc::ripemd160), (char*)&check, 4 ) == 0;
   }

   pts_address::operator std::string()const
   {
        return fc::to_base58( addr.data, sizeof(addr) );
   }

   pts_address_extra::pts_address_extra()
   {
	   memset(addr.data, 0, sizeof(addr.data));
   }
   pts_address_extra::pts_address_extra(const std::string& base58str)
   {
	   std::vector<char> v = fc::from_base58(fc::string(base58str));
	   if (v.size())
		   memcpy(addr.data, v.data(), std::min<size_t>(v.size(), sizeof(addr)));

	   if (!is_valid())
	   {
		   FC_THROW_EXCEPTION(invalid_pts_address, "invalid pts_address_extra ${a}", ("a", base58str));
	   }
   }

   pts_address_extra::pts_address_extra(const fc::ecc::public_key& pub, bool compressed, uint16_t version)
   {
	   uint256 digest;
	   if (compressed)
	   {
		   auto dat = pub.serialize();

		   digest = HashR14(dat.data, dat.data+sizeof(dat));
	   }
	   else
	   {
		   auto dat = pub.serialize_ecc_point();
		   digest = HashR14(dat.data, dat.data + sizeof(dat));
	   }
	   auto rep = fc::ripemd160::hash((char*)&digest, sizeof(digest));
	   addr.data[0] = char(version >> 8);
	   addr.data[1] = (char)version;
	   memcpy(addr.data + 2, (char*)&rep, sizeof(rep));
	   auto check = HashR14(addr.data, addr.data + sizeof(rep) + 2);
	   check = HashR14((char*)&check, (char*)&check + sizeof(check));
	   memcpy(addr.data + 2 + sizeof(rep), (char*)&check, 4);
   }

   /**
   *  Checks the address to verify it has a
   *  valid checksum
   */
   bool pts_address_extra::is_valid()const
   {
	   auto check = HashR14(addr.data, addr.data + sizeof(fc::ripemd160) + 2);
	   check = HashR14((char*)&check, (char*)&check + sizeof(check));
	   return memcmp(addr.data + 2 + sizeof(fc::ripemd160), (char*)&check, 4) == 0;
   }

   pts_address_extra::operator std::string()const
   {
	   return fc::to_base58(addr.data, sizeof(addr));
   }
   pts_address_bch::pts_address_bch()
   {
	   memset(addr.data, 0, sizeof(addr.data));
   }
   
   template <int frombits, int tobits, bool pad, typename O, typename I>
   bool ConvertBits(O &out, I it, I end) {
	   size_t acc = 0;
	   size_t bits = 0;
	   constexpr size_t maxv = (1 << tobits) - 1;
	   constexpr size_t max_acc = (1 << (frombits + tobits - 1)) - 1;
	   while (it != end) {
		   acc = ((acc << frombits) | *it) & max_acc;
		   bits += frombits;
		   while (bits >= tobits) {
			   bits -= tobits;
			   out.push_back((acc >> bits) & maxv);
		   }
		   ++it;
	   }

	   // We have remaining bits to encode but do not pad.
	   if (!pad && bits) {
		   return false;
	   }

	   // We have remaining bits to encode so we do pad.
	   if (pad && bits) {
		   out.push_back((acc << (tobits - bits)) & maxv);
	   }

	   return true;
   }
   template <class T>
   std::vector<uint8_t> PackAddrData(const T &id, uint8_t type) {
	   uint8_t version_byte(type << 3);
	   size_t size = id.size();
	   uint8_t encoded_size = 0;
	   switch (size * 8) {
	   case 160:
		   encoded_size = 0;
		   break;
	   case 192:
		   encoded_size = 1;
		   break;
	   case 224:
		   encoded_size = 2;
		   break;
	   case 256:
		   encoded_size = 3;
		   break;
	   case 320:
		   encoded_size = 4;
		   break;
	   case 384:
		   encoded_size = 5;
		   break;
	   case 448:
		   encoded_size = 6;
		   break;
	   case 512:
		   encoded_size = 7;
		   break;
	   default:
		   throw std::runtime_error(
			   "Error packing cashaddr: invalid address length");
	   }
	   version_byte |= encoded_size;
	   std::vector<uint8_t> data = { version_byte };
	   data.insert(data.end(), std::begin(id), std::end(id));

	   std::vector<uint8_t> converted;
	   // Reserve the number of bytes required for a 5-bit packed version of a
	   // hash, with version byte.  Add half a byte(4) so integer math provides
	   // the next multiple-of-5 that would fit all the data.
	   converted.reserve(((size + 1) * 8 + 4) / 5);
	   ConvertBits<8, 5, true>(converted, std::begin(data), std::end(data));

	   return converted;
   }
   pts_address_bch::pts_address_bch(const std::string& bitcoincashstr)
   {
	   auto decode_addr = cashaddr::Decode(bitcoincashstr, bch_prefix);
	   if (decode_addr.first == "" || decode_addr.first != bch_prefix){
		   FC_THROW_EXCEPTION(invalid_pts_address, "invalid pts_address ${a}", ("a", bitcoincashstr));
	   }
	   cashaddr::data combined =  decode_addr.second;
	   string addWithCheck(combined.begin(), combined.end());
	   memcpy(addr.data, addWithCheck.data(), 42);
	   if (!is_valid())
	   {
		   FC_THROW_EXCEPTION(invalid_pts_address, "invalid pts_address ${a}", ("a", bitcoincashstr));
	   }
   }
   pts_address_bch::pts_address_bch(const fc::ecc::public_key& pub, bool compressed, uint8_t version)
   {
	   fc::sha256 sha2;
	   if (compressed)
	   {
		   auto dat = pub.serialize();
		   sha2 = fc::sha256::hash(dat.data, sizeof(dat));
	   }
	   else
	   {
		   auto dat = pub.serialize_ecc_point();
		   sha2 = fc::sha256::hash(dat.data, sizeof(dat));
	   }
	   auto rep = fc::ripemd160::hash((char*)&sha2, sizeof(sha2));
	   char temp[20];
	   memcpy(temp, (char *)&rep, sizeof(rep));
	   std::string addr_str_temp(temp, 20);
	   auto packed_addr = PackAddrData(addr_str_temp, 0);
	   auto prefix = cashaddr::ExpandPrefix(bch_prefix);
	   //auto prefix_addr = cashaddr::Cat(prefix, addrdata);
	   cashaddr::data checksum = cashaddr::CreateChecksum(bch_prefix, packed_addr);
	   cashaddr::data combined = cashaddr::Cat(packed_addr, checksum);
	   string addWithCheck(combined.begin(),combined.end());
	   memcpy(addr.data, addWithCheck.data(),42);
// 	   addr.data[0] = version;
// 	   memcpy(addr.data + 1, (char*)&rep, sizeof(rep));
// 	   auto check = fc::sha256::hash(addr.data, sizeof(rep) + 1);
// 	   check = fc::sha256::hash(check);
// 	   memcpy(addr.data + 1 + sizeof(rep), (char*)&check, 4);
   }
   pts_address_bch::pts_address_bch(const pts_address & pts_addr, uint8_t kh, uint8_t sh) {
	   if (!pts_addr.is_valid()) {
		   FC_THROW_EXCEPTION(invalid_pts_address, "invalid pts_address ${a}", ("a", pts_addr));
	   }
	   auto real_addr = pts_addr.addr;
	   std::string addr_str_temp(real_addr.begin()+1, real_addr.begin() + 21);
	   uint8_t addr_type = 0;
	   if (pts_addr.version() == kh){
		   addr_type = 0;
	   }
	   else if(pts_addr.version() == sh){
		   addr_type = 1;
	   }
	   auto packed_addr = PackAddrData(addr_str_temp, addr_type);
	   auto prefix = cashaddr::ExpandPrefix(bch_prefix);
	   //auto prefix_addr = cashaddr::Cat(prefix, addrdata);
	   cashaddr::data checksum = cashaddr::CreateChecksum(bch_prefix, packed_addr);
	   cashaddr::data combined = cashaddr::Cat(packed_addr, checksum);
	   string addWithCheck(combined.begin(), combined.end());
	   memcpy(addr.data, addWithCheck.data(), 42);
   }
   /**
	*  Checks the address to verify it has a
	*  valid checksum
	*/
   bool pts_address_bch::is_valid()const
   {
	   cashaddr::data values(addr.begin(), addr.end());
	   std::string prefix(bch_prefix);
	   return cashaddr::VerifyChecksum(prefix, values);
	   /*
	   auto check = fc::sha256::hash(addr.data, sizeof(fc::ripemd160) + 1);
	   check = fc::sha256::hash(check);
	   return memcmp(addr.data + 1 + sizeof(fc::ripemd160), (char*)&check, 4) == 0;*/
   }

   pts_address_bch::operator std::string()const
   {
	   std::string prefix = bch_prefix;
	   std::string ret = prefix + ':';
	   std::vector<unsigned char> combined(addr.begin(), addr.end());
	   ret.reserve(ret.size() + combined.size());
	   for (uint8_t c : combined) {
		   ret += cashaddr::GET_CHARSET(c);
	   }
	   return ret;
	   //return fc::to_base58(addr.data, sizeof(addr));
   }
} } // namespace graphene

namespace fc
{
   void to_variant( const graphene::chain::pts_address& var,  variant& vo )
   {
        vo = std::string(var);
   }
   void from_variant( const variant& var,  graphene::chain::pts_address& vo )
   {
        vo = graphene::chain::pts_address( var.as_string() );
   }
   void to_variant(const graphene::chain::pts_address_extra& var, variant& vo)
   {
	   vo = std::string(var);
   }
   void from_variant(const variant& var, graphene::chain::pts_address_extra& vo)
   {
	   vo = graphene::chain::pts_address_extra(var.as_string());
   }
   void to_variant(const graphene::chain::pts_address_bch& var, variant& vo)
   {
	   vo = std::string(var);
   }
   void from_variant(const variant& var, graphene::chain::pts_address_bch& vo)
   {
	   vo = graphene::chain::pts_address_bch(var.as_string());
   }
   
}
