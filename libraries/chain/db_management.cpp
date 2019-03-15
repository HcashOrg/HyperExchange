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

#include <graphene/chain/database.hpp>

#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>

#include <fc/io/fstream.hpp>

#include <fstream>
#include <functional>
#include <iostream>

#include "boost/filesystem/operations.hpp"
namespace graphene { namespace chain {

database::database()
{
   initialize_indexes();
   initialize_evaluators();
}

database::~database()
{
   clear_pending();
}
bool copyDir(
	boost::filesystem::path const & source,
	boost::filesystem::path const & destination
)
{
	namespace fs = boost::filesystem;
	try
	{
		// Check whether the function call is valid
		if (
			!fs::exists(source) ||
			!fs::is_directory(source)
			)
		{
			std::cerr << "Source directory " << source.string()
				<< " does not exist or is not a directory." << '\n'
				;
			return false;
		}
		if (fs::exists(destination))
		{
			std::cerr << "Destination directory " << destination.string()
				<< " already exists." << '\n'
				;
			return false;
		}
		// Create the destination directory
		if (!fs::create_directory(destination))
		{
			std::cerr << "Unable to create destination directory"
				<< destination.string() << '\n'
				;
			return false;
		}
	}
	catch (fs::filesystem_error const & e)
	{
		std::cerr << e.what() << '\n';
		return false;
	}
	// Iterate through the source directory
	for (
		fs::directory_iterator file(source);
		file != fs::directory_iterator(); ++file
		)
	{
		try
		{
			fs::path current(file->path());
			if (fs::is_directory(current))
			{
				// Found directory: Recursion
				if (
					!copyDir(
						current,
						destination / current.filename()
					)
					)
				{
					return false;
				}
			}
			else
			{
				// Found file: Copy
				fs::copy_file(
					current,
					destination / current.filename()
				);
			}
		}
		catch (fs::filesystem_error const & e)
		{
			std::cerr << e.what() << '\n';
		}
	}
	return true;
}
void database::read_backup_info(const fc::path& dir)
{
	ifstream backup_info((dir / "backup_info").string());
	if (!backup_info)
	{
		backup_block_num = 0;
		backup_path = dir;
		return;
	}

	backup_path = dir;
	int32_t num;
	string bid;
	try
	{
		backup_info >> backup_block_num >> bid;
	    //check  whether id matched num
	}
	catch (const std::exception&)
	{
		backup_block_num = 0;
	}
	if(backup_info)
		backup_info.close();
}
void graphene::chain::database::restore_from_backup(const fc::path& dir)
{
	std::cout << backup_path.string() << std::endl;
	FC_ASSERT(boost::filesystem::exists(backup_path));
	copyDir(backup_path / "object_database", dir / "object_database");
	copyDir(backup_path / "undo_dbstorage", dir / "undo_dbstorage");
	boost::filesystem::copy_file(backup_path / "fork_db", dir / "fork_db");
	boost::filesystem::copy_file(backup_path / "undo_dbstack", dir / "undo_dbstack");
}

void database::remove_backup()
{
	try {
		backup_block_num = 0;
		if (boost::filesystem::exists(backup_path))
			boost::filesystem::remove_all(backup_path);
	}catch(...)
	{}
}

void database::backup()
{
	auto num = head_block_num();
	auto bid = head_block_id();
	close();
	try {
		if (boost::filesystem::exists(backup_path))
			boost::filesystem::remove_all(backup_path);
		if (!boost::filesystem::exists(backup_path))
			boost::filesystem::create_directory(backup_path);
		copyDir(data_dir_using / "object_database", backup_path/ "object_database");
		copyDir(data_dir_using / "undo_dbstorage", backup_path/ "undo_dbstorage");
		boost::filesystem::copy_file(data_dir_using / "fork_db", backup_path/ "fork_db");
		boost::filesystem::copy_file(data_dir_using / "undo_dbstack", backup_path/ "undo_dbstack");
		ofstream backup_info;
		backup_info.open((backup_path /"backup_info").string(), std::ios::trunc);
		backup_info << num <<" "<< bid.str();
		backup_info.close();
		backup_block_num = num;
	}
	catch (std::exception& e)
	{
		backup_block_num = 0;
		cout << e.what() << std::endl;
	}
	open(data_dir_using, genesis_loader_using);
}
void database::reindex(fc::path data_dir, const genesis_state_type& initial_allocation,bool completely)
{ try {
   ilog( "reindexing blockchain" );
   wipe(data_dir, false);   
   uint64_t start_block = 1;
   if (!completely)
   {
	   try {
		   restore_from_backup(data_dir);
		   start_block = backup_block_num;
	   }
	   catch (...)
	   {

	   }
   }
   bool restore_failed = false;
   try {
		   open(data_dir, [&initial_allocation] {return initial_allocation; });
		   start_block = head_block_num() + 1;
	   }
	   catch (...)
	   {
		   restore_failed = true;
	}
   if (restore_failed)
   {
	   start_block = 1;
	   ilog("restore from backup failed");
	   wipe(data_dir, false);
	   try {
		   open(data_dir, [&initial_allocation] {return initial_allocation; });
	   }
	   catch (const deserialize_fork_database_failed& e)
	   {
	   }
	   catch (const deserialize_undo_database_failed& e)
	   {
	   }
   }
   auto start = fc::time_point::now();
   auto last_block = _block_id_to_block.last();
   if( !last_block ) {
      elog( "!no last block" );
      edump((last_block));
      return;
   }

   const auto last_block_num = last_block->block_num();

   ilog( "Replaying blocks..." );
   //_undo_db.reset();
   _undo_db.set_max_size(1440);
   _undo_db.enable();
   _fork_db.set_max_size(1500);
   _fork_db.reset();
   _undo_db.discard();
   _undo_db.enable();
   _undo_db.set_max_size(GRAPHENE_UNDO_BUFF_MAX_SIZE);
   uint32_t undo_enable_num = last_block_num - 1440;
   for( uint32_t i = start_block; i <= last_block_num; ++i )
   {
      if( i % 10000 == 0 ) std::cerr << "   " << double(i*100)/last_block_num << "%   "<<i << " of " <<last_block_num<<"   \n";
      fc::optional< signed_block > block = _block_id_to_block.fetch_by_number(i);
      if( !block.valid() )
      {
         wlog( "Reindexing terminated due to gap:  Block ${i} does not exist!", ("i", i) );
         uint32_t dropped_count = 0;
         while( true )
         {
            fc::optional< block_id_type > last_id = _block_id_to_block.last_id();
            // this can trigger if we attempt to e.g. read a file that has block #2 but no block #1
            if( !last_id.valid() )
               break;
            // we've caught up to the gap
            if( block_header::num_from_id( *last_id ) <= i )
               break;
            _block_id_to_block.remove( *last_id );
            dropped_count++;
         }
         wlog( "Dropped ${n} blocks from after the gap", ("n", dropped_count) );
         break;
      }
      _fork_db.push_block(*block);
	  if (i >= undo_enable_num)
		  _undo_db.set_max_size(1440);
	  auto session=_undo_db.start_undo_session();
      apply_block(*block, skip_miner_signature |
                          skip_transaction_signatures |
                          skip_transaction_dupe_check |
                          skip_tapos_check |
                          skip_witness_schedule_check |
                          skip_authority_check);
	  session.commit();
   }
   auto end = fc::time_point::now();
   ilog( "Done reindexing, elapsed time: ${t} sec", ("t",double((end-start).count())/1000000.0 ) );
} FC_CAPTURE_AND_RETHROW( (data_dir) ) }

void database::wipe(const fc::path& data_dir, bool include_blocks)
{
   ilog("Wiping database", ("include_blocks", include_blocks));
   close();
   object_database::wipe(data_dir);
   if( include_blocks )
      fc::remove_all( data_dir / "database" );
}

void database::open(
   const fc::path& data_dir,
   std::function<genesis_state_type()> genesis_loader)
{
   try
   {

      object_database::open(data_dir);

      _block_id_to_block.open(data_dir / "database" / "block_num_to_block");
	  data_dir_using = data_dir;
	  genesis_loader_using = genesis_loader;
      if( !find(global_property_id_type()) )
         init_genesis(genesis_loader());

      fc::optional<signed_block> last_block = _block_id_to_block.last();
      if( last_block.valid() )
      {
         _fork_db.start_block( *last_block );
         idump((last_block->id())(last_block->block_num()));
         idump((head_block_id())(head_block_num()));
         if( last_block->id() != head_block_id() )
         {
              FC_ASSERT( head_block_num() == 0||get_block_id_for_num(head_block_num())== head_block_id(), "last block ID does not match current chain state",
                         ("last_block->id", last_block->id())("head_block_num",head_block_num()) );
         }
      }

		  fc::path data_dir = get_data_dir() / "undo_db";
		  try {
			  _undo_db.from_file(data_dir.string());
		  }
		  catch (...)
		  {
			  FC_CAPTURE_AND_THROW(deserialize_fork_database_failed, (data_dir));
		  }
		  fc::path fork_data_dir = get_data_dir() / "fork_db";
		  _fork_db.from_file(fork_data_dir.string());



   }
   FC_CAPTURE_LOG_AND_RETHROW( (data_dir) )
}

void database::close()
{

   // TODO:  Save pending tx's on close()
   clear_pending();
   // pop all of the blocks that we can given our undo history, this should
   // throw when there is no more undo history to pop
   if( rewind_on_close)
   {
      try
      {
         uint32_t cutoff = head_block_num()-_undo_db.size();
		 auto i = head_block_num();

         while( head_block_num() > cutoff )
         {
			 std::cout << "cutof=" << cutoff << "\t" << "pop:" << head_block_num() << "\t" << "to cutoff" << head_block_num() - cutoff << std::endl;
         //   elog("pop");
            block_id_type popped_block_id = head_block_id();
            pop_block();
            _fork_db.remove(popped_block_id); // doesn't throw on missing
            try
            {
               _block_id_to_block.remove(popped_block_id);
            }
            catch (const fc::key_not_found_exception&)
            {
            }
         }
      }
      catch ( const fc::exception& e )
      {
         wlog( "Database close unexpected exception: ${e}", ("e", e) );
      }
   }

   // Since pop_block() will move tx's in the popped blocks into pending,
   // we have to clear_pending() after we're done popping to get a clean
   // DB state (issue #336).
   clear_pending();

   object_database::flush();
   object_database::close();
   fc::path undo_data_dir = get_data_dir() / "undo_db";
   fc::path fork_data_dir = get_data_dir() / "fork_db";
   if (!rewind_on_close)
   {
	   _undo_db.save_to_file(undo_data_dir.string());
	   _fork_db.save_to_file(fork_data_dir.string());
   }
   else
   {
	   boost::filesystem::remove(undo_data_dir);
	   boost::filesystem::remove(fork_data_dir);
	   _undo_db.discard();
   }


   if( _block_id_to_block.is_open() )
      _block_id_to_block.close();

   _fork_db.reset();
}

} }
