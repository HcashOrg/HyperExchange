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

#include <graphene/backup/backup_plugin.hpp>

#include <graphene/app/impacted.hpp>
#include <graphene/chain/database.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>
#include <iostream>
namespace graphene { namespace backup {

	namespace detail
	{


		class backup_plugin_impl
		{
		public:
			backup_plugin_impl(backup_plugin& _plugin);
			virtual ~backup_plugin_impl();


			/** this method is called as a callback after a block is applied
			 * and will process/index all operations that were applied in the block.
			 */

			graphene::chain::database& database()
			{
				return _self.database();
			}
			/** add one history record, then check and remove the earliest history record */
			void toBackup(const signed_block& block);
			void removeBack(const uint32_t& block_num);
			void executeBackUp();
		private:
			backup_plugin& _self;
			map<uint32_t, string> added_blocks;
			Cached_levelDb  c_ldb;
		};

		backup_plugin_impl::backup_plugin_impl(backup_plugin& plugin) :_self(plugin) {
		}
		backup_plugin_impl::~backup_plugin_impl() {}

		void backup_plugin_impl::executeBackUp()
		{
			const auto& db = database();
			auto last_num = db.get_dynamic_global_properties().last_irreversible_block_num;
			try {
				vector<uint32_t> erased_added;
				leveldb::WriteBatch wb;
				for (const auto& block_key : added_blocks)
				{
					if (block_key.first > last_num)
						break;
					wb.Put(fc::to_string(block_key.first), leveldb::Slice(block_key.second));
					erased_added.push_back(block_key.first);
				}
				wb.Put("blocknum", fc::to_string(last_num) + "|" + db.get_block_id_for_num(last_num).operator fc::string());
				db.get_backup_db()->Write(leveldb::WriteOptions(), &wb);
				for (const auto& k : erased_added)
				{
					added_blocks.erase(k);
				}
			}FC_CAPTURE_AND_LOG((last_num))
		}

		void backup_plugin_impl::removeBack(const uint32_t& block_num)
		{
			try {
				if (added_blocks.count(block_num))
					added_blocks.erase(block_num);
			}FC_CAPTURE_AND_LOG((block_num))
		}


		void backup_plugin_impl::toBackup(const signed_block& block)
		{
			auto& db = database();
			if (db.get_backup_db() == nullptr)
				return;
			db.backup_db.store_into_db(block, db.get_backup_db());
		}
	}
backup_plugin::backup_plugin():my( new detail::backup_plugin_impl(*this) )
{
}

backup_plugin::~backup_plugin()
{
}

std::string backup_plugin::plugin_name()const
{
   return "backup_plugin";
}

void backup_plugin::plugin_set_program_options(
	boost::program_options::options_description& cli,
	boost::program_options::options_description& cfg
)
{
}

void backup_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
	database().applied_backup.connect([&](const signed_block& b) { my->toBackup(b); });
}

void backup_plugin::plugin_startup()
{
}




} }
