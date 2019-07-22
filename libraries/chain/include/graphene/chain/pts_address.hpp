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
#pragma once

#include <fc/array.hpp>
#include <string>
#include <iostream>
const std::string bch_prefix = "bitcoincash";
namespace fc { namespace ecc { class public_key; } }

namespace graphene { namespace chain {

   /**
    *  Implements address stringification and validation from PTS
    */
   struct pts_address
   {
       pts_address(); ///< constructs empty / null address
       pts_address( const std::string& base58str );   ///< converts to binary, validates checksum
       pts_address( const fc::ecc::public_key& pub, bool compressed = true, uint8_t version=56 ); ///< converts to binary

       uint8_t version()const { return addr.at(0); }
       bool is_valid()const;

       operator std::string()const; ///< converts to base58 + checksum

       fc::array<char,25> addr; ///< binary representation of address
   };

   inline bool operator == ( const pts_address& a, const pts_address& b ) { return a.addr == b.addr; }
   inline bool operator != ( const pts_address& a, const pts_address& b ) { return a.addr != b.addr; }
   inline bool operator <  ( const pts_address& a, const pts_address& b ) { return a.addr <  b.addr; }
   struct pts_address_extra {
	   pts_address_extra(); ///< constructs empty / null address
	   pts_address_extra(const std::string& base58str);   ///< converts to binary, validates checksum
	   pts_address_extra(const fc::ecc::public_key& pub, bool compressed = true, uint16_t version = 56); ///< converts to binary
	   uint16_t version()const { 
		   uint16_t version;
		   version = addr.at(0);
		   version = (version << 8) + addr.at(1);
		   std::cout << "pts address extra "<<version << std::endl;
		   return version;
	   }
	   bool is_valid()const;

	   operator std::string()const; ///< converts to base58 + checksum

	   fc::array<char, 26> addr; ///< binary representation of address
   };
   inline bool operator == (const pts_address_extra& a, const pts_address_extra& b) { return a.addr == b.addr; }
   inline bool operator != (const pts_address_extra& a, const pts_address_extra& b) { return a.addr != b.addr; }
   inline bool operator <  (const pts_address_extra& a, const pts_address_extra& b) { return a.addr < b.addr; }
   struct pts_address_bch
   {
	   pts_address_bch(); ///< constructs empty / null address
	   pts_address_bch(const std::string& bitcoincashstr);   ///< converts to binary, validates checksum
	   pts_address_bch(const fc::ecc::public_key& pub, bool compressed = true, uint8_t version = 56); ///< converts to binary
	   pts_address_bch(const pts_address & pts_addr, uint8_t kh, uint8_t sh);

	   uint8_t version()const { return addr.at(0); }
	   bool is_valid()const;

	   operator std::string()const; ///< converts to base58 + checksum

	   fc::array<char, 42> addr; ///< binary representation of address
	   
   };

   inline bool operator == (const pts_address_bch& a, const pts_address_bch& b) { return a.addr == b.addr; }
   inline bool operator != (const pts_address_bch& a, const pts_address_bch& b) { return a.addr != b.addr; }
   inline bool operator <  (const pts_address_bch& a, const pts_address_bch& b) { return a.addr < b.addr; }
} } // namespace graphene::chain

namespace std
{
   template<>
   struct hash<graphene::chain::pts_address> 
   {
       public:
         size_t operator()(const graphene::chain::pts_address &a) const 
         {
            size_t s;
            memcpy( (char*)&s, &a.addr.data[sizeof(a)-sizeof(s)], sizeof(s) );
            return s;
         }
   };
   template<>
   struct hash<graphene::chain::pts_address_extra>
   {
   public:
	   size_t operator()(const graphene::chain::pts_address_extra &a) const
	   {
		   size_t s;
		   memcpy((char*)&s, &a.addr.data[sizeof(a) - sizeof(s)], sizeof(s));
		   return s;
	   }
   };
   template<>
   struct hash<graphene::chain::pts_address_bch>
   {
   public:
	   size_t operator()(const graphene::chain::pts_address_bch &a) const
	   {
		   size_t s;
		   memcpy((char*)&s, &a.addr.data[sizeof(a) - sizeof(s)], sizeof(s));
		   return s;
	   }
   };
}

#include <fc/reflect/reflect.hpp>
FC_REFLECT( graphene::chain::pts_address, (addr) )
FC_REFLECT(graphene::chain::pts_address_extra, (addr))
FC_REFLECT(graphene::chain::pts_address_bch, (addr))
namespace fc 
{ 
   void to_variant( const graphene::chain::pts_address& var,  fc::variant& vo );
   void from_variant( const fc::variant& var,  graphene::chain::pts_address& vo );
   void to_variant(const graphene::chain::pts_address_extra& var, fc::variant& vo);
   void from_variant(const fc::variant& var, graphene::chain::pts_address_extra& vo);
   void to_variant(const graphene::chain::pts_address_bch& var, fc::variant& vo);
   void from_variant(const fc::variant& var, graphene::chain::pts_address_bch& vo);
}
