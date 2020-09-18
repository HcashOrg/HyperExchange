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
#include <graphene/chain/block_database.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <fc/io/raw.hpp>
#include <fc/smart_ref_impl.hpp>
#include <leveldb/write_batch.h>
namespace graphene { namespace chain {

	struct index_entry
	{
		uint64_t      block_pos = 0;
		uint32_t      block_size = 0;
		block_id_type block_id;
	};
}
}
FC_REFLECT(graphene::chain::index_entry, (block_pos)(block_size)(block_id));

namespace graphene {
	namespace chain {

bool block_database::is_opendb(const fc::path& dbdir)
{
	return fc::exists(dbdir / "index");
}
void block_database::open_db( const fc::path& dbdir )
{ try {
   //fc::create_directories(dbdir);
   _block_num_to_pos.exceptions(std::ios_base::failbit | std::ios_base::badbit);
   _blocks.exceptions(std::ios_base::failbit | std::ios_base::badbit);
   {
     _block_num_to_pos.open( (dbdir/"index").generic_string().c_str(), std::fstream::binary | std::fstream::in | std::fstream::out );
     _blocks.open( (dbdir/"blocks").generic_string().c_str(), std::fstream::binary | std::fstream::in | std::fstream::out );
   }
} FC_CAPTURE_AND_RETHROW( (dbdir) ) }
void block_database::migrate(const fc::path & dbdir) {
	try {
		FC_ASSERT(is_opendb(dbdir));
		open_db(dbdir);
		open(dbdir);
		auto bk_op = last_db();
		while (bk_op.valid())
		{
			remove_db(bk_op->id());
			store(bk_op->id(), *bk_op);
			bk_op = last_db();
		}
		remove_all(dbdir/"index");
		remove_all(dbdir / "blocks");

	}FC_CAPTURE_AND_RETHROW()
}
optional<signed_block> block_database::last_db()const
{
	try
	{
		index_entry e;
		_block_num_to_pos.seekg(0, _block_num_to_pos.end);

		if (_block_num_to_pos.tellp() < sizeof(index_entry))
			return optional<signed_block>();

		_block_num_to_pos.seekg(-sizeof(index_entry), _block_num_to_pos.end);
		_block_num_to_pos.read((char*)&e, sizeof(e));
		uint64_t pos = _block_num_to_pos.tellg();
		while (e.block_size == 0 && pos > 0)
		{
			pos -= sizeof(index_entry);
			_block_num_to_pos.seekg(pos);
			_block_num_to_pos.read((char*)&e, sizeof(e));
		}

		if (e.block_size == 0)
			return optional<signed_block>();

		vector<char> data(e.block_size);
		_blocks.seekg(e.block_pos);
		_blocks.read(data.data(), e.block_size);
		auto result = fc::raw::unpack<signed_block>(data);
		return result;
	}
	catch (const fc::exception&)
	{
	}
	catch (const std::exception&)
	{
	}
	return optional<signed_block>();
}


//bool block_database::is_open()const
//{
//  return _blocks.is_open();
//}

//void block_database::close()
//{
//	if (_blocks.is_open())
//		_blocks.close();
//	if (_block_num_to_pos.is_open())
//		_block_num_to_pos.close();
//}

//void block_database::flush()
//{
//  _blocks.flush();
//  _block_num_to_pos.flush();
//}

//void block_database::store( const block_id_type& _id, const signed_block& b )
//{
//   block_id_type id = _id;
//   if( id == block_id_type() )
//   {
//      id = b.id();
//      elog( "id argument of block_database::store() was not initialized for block ${id}", ("id", id) );
//   }
//   auto num = block_header::num_from_id(id);
//   _block_num_to_pos.seekp( sizeof( index_entry ) * num );
//   index_entry e;
//   _blocks.seekp( 0, _blocks.end );
//   auto vec = fc::raw::pack( b );
//   e.block_pos  = _blocks.tellp();
//   e.block_size = vec.size();
//   e.block_id   = id;
//   _blocks.write( vec.data(), vec.size() );
//   _block_num_to_pos.write( (char*)&e, sizeof(e) );
//}

void block_database::remove_db(const block_id_type& id)
{
	try {
		index_entry e;
		auto index_pos = sizeof(e)*block_header::num_from_id(id);
		_block_num_to_pos.seekg(0, _block_num_to_pos.end);
		if (_block_num_to_pos.tellg() <= index_pos)
			FC_THROW_EXCEPTION(fc::key_not_found_exception, "Block ${id} not contained in block database", ("id", id));

		_block_num_to_pos.seekg(index_pos);
		_block_num_to_pos.read((char*)&e, sizeof(e));

		if (e.block_id == id)
		{
			e.block_size = 0;
			_block_num_to_pos.seekp(sizeof(e)*block_header::num_from_id(id));
			_block_num_to_pos.write((char*)&e, sizeof(e));
		}
	} FC_CAPTURE_AND_RETHROW((id))
}

//bool block_database::contains( const block_id_type& id )const
//{
//   if( id == block_id_type() )
//      return false;

//   index_entry e;
//   auto index_pos = sizeof(e)*block_header::num_from_id(id);
//   _block_num_to_pos.seekg( 0, _block_num_to_pos.end );
//   if ( _block_num_to_pos.tellg() <= index_pos )
//      return false;
//   _block_num_to_pos.seekg( index_pos );
//   _block_num_to_pos.read( (char*)&e, sizeof(e) );

//   return e.block_id == id && e.block_size > 0;
//}

//block_id_type block_database::fetch_block_id( uint32_t block_num )const
//{
//   assert( block_num != 0 );
//   index_entry e;
//   auto index_pos = sizeof(e)*block_num;
//   _block_num_to_pos.seekg( 0, _block_num_to_pos.end );
//   if ( _block_num_to_pos.tellg() <= int64_t(index_pos) )
//      FC_THROW_EXCEPTION(fc::key_not_found_exception, "Block number ${block_num} not contained in block database", ("block_num", block_num));

//   _block_num_to_pos.seekg( index_pos );
//   _block_num_to_pos.read( (char*)&e, sizeof(e) );

//   FC_ASSERT( e.block_id != block_id_type(), "Empty block_id in block_database (maybe corrupt on disk?)" );
//   return e.block_id;
//}

//optional<signed_block> block_database::fetch_optional( const block_id_type& id )const
//{
//   try
//   {
//      index_entry e;
//      auto index_pos = sizeof(e)*block_header::num_from_id(id);
//      _block_num_to_pos.seekg( 0, _block_num_to_pos.end );
//      if ( _block_num_to_pos.tellg() <= index_pos )
//         return {};

//      _block_num_to_pos.seekg( index_pos );
//      _block_num_to_pos.read( (char*)&e, sizeof(e) );

//      if( e.block_id != id ) return optional<signed_block>();

//      vector<char> data( e.block_size );
//      _blocks.seekg( e.block_pos );
//      if (e.block_size)
//      _blocks.read( data.data(), e.block_size );
//      auto result = fc::raw::unpack<signed_block>(data);
//      FC_ASSERT( result.id() == e.block_id );
//      return result;
//}
//   catch (const fc::exception&)
bool is_block_num(const string& str)
   {
	for (const auto& s : str)
   {
		if (!std::isdigit(s))
			return false;
   }
	return true;
}

void block_database::open(const fc::path& dbdir)
{
	try {
		if (_blocks_db != nullptr)
			return;
		leveldb::Options options;
		options.create_if_missing = true;
		if (!fc::exists(dbdir))
		{
			fc::create_directories(dbdir);
		}
		auto open_status = leveldb::DB::Open(options, dbdir.generic_string(), &_blocks_db);
		FC_ASSERT(open_status.ok(),"block database open error.");
	}FC_CAPTURE_AND_RETHROW((dbdir))
}
bool block_database::is_open() const
   {
	return _blocks_db != nullptr;

   }

void block_database::flush()
   {
   }

void block_database::close()
   {
	auto b = last();
	std::cout << "closed, the block num is  " << b->block_num() << std::endl;
	if (is_open())
		delete _blocks_db;
	_blocks_db = nullptr;
   }

void block_database::store(const block_id_type& id, const signed_block& b)
{
	try {
		FC_ASSERT(is_open());
		auto num = b.block_num();
		leveldb::WriteBatch wb;
		vector<char> vec_id = fc::raw::pack(b.id());
		wb.Put(fc::to_string(num),fc::string(vec_id.begin(),vec_id.end()));
		vector<char> vec_bk = fc::raw::pack(b);
		wb.Put(id.str(), fc::string(vec_bk.begin(), vec_bk.end()));
		vector<char> vec_num = fc::raw::pack(b.block_num());
		wb.Put("last", fc::string(vec_num.begin(), vec_num.end()));
		_blocks_db->Write(leveldb::WriteOptions(),&wb);
	}FC_CAPTURE_AND_RETHROW((id)(b))
}

void block_database::remove(const block_id_type& id)
{
	try{
		FC_ASSERT(is_open());
		string value;
		auto b_id_op = last_id();
		auto status = _blocks_db->Get(leveldb::ReadOptions(), id.str(), &value);
		if (status.ok())
   {
			vector<char> vec(value.begin(),value.end());
			auto result = fc::raw::unpack<signed_block>(vec);

			auto num = result.block_num();

			leveldb::WriteBatch wb;
			wb.Delete(fc::to_string(num));
			wb.Delete(id.str());
			if (*b_id_op == id)
      {
				auto newlast = num - 1;
				auto vec_new = fc::raw::pack(newlast);
				wb.Put("last", fc::string(vec_new.begin(),vec_new.end()));
      }
			_blocks_db->Write(leveldb::WriteOptions(),&wb);
		}
	}FC_CAPTURE_AND_RETHROW((id))
   }

bool block_database::contains(const block_id_type& id) const
   {
	try {
		FC_ASSERT(is_open());
		string value;
		auto status = _blocks_db->Get(leveldb::ReadOptions(), id.str(), &value);
		return status.ok();
	}FC_CAPTURE_AND_RETHROW((id))
   }

block_id_type block_database::fetch_block_id(uint32_t block_num)const
   {
	try {
		FC_ASSERT(is_open());
		string value;
		auto status = _blocks_db->Get(leveldb::ReadOptions(), fc::to_string(block_num), &value);
		if (status.ok())
		{
			vector<char> vec(value.begin(), value.end());
			auto result = fc::raw::unpack<block_id_type>(vec);
			return result;
   }
		return block_id_type();
	}FC_CAPTURE_AND_RETHROW((block_num))
}

optional<signed_block > block_database::fetch_optional(const block_id_type& id) const
{
	try {
		FC_ASSERT(is_open());
		string value;
		auto status = _blocks_db->Get(leveldb::ReadOptions(), id.str(), &value);
		if (status.ok())
   {
			vector<char> vec(value.begin(), value.end());
			auto result = fc::raw::unpack<signed_block>(vec);
			return result;
      }

		return optional<signed_block>();
	}FC_CAPTURE_AND_RETHROW((id))
   }

optional<signed_block> block_database::fetch_by_number(uint32_t block_num) const
   {
	try {
		auto id = fetch_block_id(block_num);
		return fetch_optional(id);
	}FC_CAPTURE_AND_RETHROW((block_num))
   }

optional<signed_block > block_database::last() const
   {
	try {
		FC_ASSERT(is_open());
		string value;
		auto status = _blocks_db->Get(leveldb::ReadOptions(), fc::string("last"), &value);
		if (status.ok())
		{
			vector<char> vec(value.begin(), value.end());
			auto num = fc::raw::unpack<uint32_t>(vec);
			return fetch_by_number(num);
   }
		return optional<signed_block>();
	}FC_CAPTURE_AND_RETHROW()
}
optional<block_id_type> block_database::last_id() const
{
	try {
		optional<signed_block> blk_op = last();
		if (blk_op.valid())
			return blk_op->id();
   return optional<block_id_type>();
	}FC_CAPTURE_AND_RETHROW()
}


} }
