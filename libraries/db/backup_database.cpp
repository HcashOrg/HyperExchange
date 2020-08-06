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
#include <graphene/db/backup_database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/filesystem.hpp>
#include <iostream>
#include <leveldb/write_batch.h>
namespace graphene { namespace chain {
	void backup_database::store_into_db(const signed_block& blk, leveldb::DB * l_db)
	{
		try {
			FC_ASSERT(l_db != nullptr, "leveldb is not valid.");
			backup_data dt;
			dt.blk_data = blk;
			dt.blk_id = blk.id();
			const auto& vec = fc::raw::pack(dt);
			leveldb::Status sta = l_db->Put(leveldb::WriteOptions(), fc::to_string(blk.block_num()), leveldb::Slice(vec.data(), vec.size()));
			if (!sta.ok())
			{
				elog("Put error: ${error}", ("error", (fc::to_string(blk.block_num()) + ":" + sta.ToString()).c_str()));
				FC_ASSERT(false, "Put Data to block backup failed");
				return;
			}
		}FC_CAPTURE_AND_RETHROW((blk))
	}

	map<uint32_t,signed_block> backup_database::get_from_db(leveldb::DB* l_db)
	{
		try {
			FC_ASSERT(l_db != nullptr, "the backup leveldb is invalid.");
			map<uint32_t,signed_block> d_blk;
			leveldb::Iterator *iterator = l_db->NewIterator(leveldb::ReadOptions());
			FC_ASSERT(iterator, "cannot create new iterator.");
			iterator->SeekToFirst();
			while (iterator->Valid())
			{
				leveldb::Slice sKey = iterator->key();
				leveldb::Slice sVal = iterator->value();
				string out = sVal.ToString();
				vector<char> vec(out.begin(), out.end());
				signed_block blk = fc::raw::unpack<backup_data>(vec).blk_data;
				uint32_t num = fc::variant(sKey.ToString()).as<uint32_t>();
				d_blk.insert(std::make_pair(num,blk));
				iterator->Next();
			}
			delete iterator;
			iterator = nullptr;
			return d_blk;
		}FC_CAPTURE_AND_RETHROW()
	}
} } // graphene::chain
