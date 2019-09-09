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
#include <graphene/db/object_database.hpp>

#include <fc/io/raw.hpp>
#include <fc/container/flat.hpp>
#include <fc/uint128.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
namespace graphene { namespace db {

object_database::object_database()
:_undo_db(*this)
{
   _index.resize(255);
   _undo_db.enable();
}

object_database::~object_database(){
}

void object_database::initialize_leveldb()
{
	leveldb::Options options;
	options.create_if_missing = true;
	open_status = leveldb::DB::Open(options, (get_data_dir() / "transactions").string(), &db);
	if (!open_status.ok())
	{
		db = nullptr;
		elog("database open failed : ${msg}", ("msg", open_status.ToString().c_str()));
		FC_ASSERT(false, "database open error");
	}
}


void object_database::destruct_leveldb()
{
	if (db != nullptr)
		delete db;
	db = nullptr;
}

void object_database::reinitialize_leveldb()
{
	destruct_leveldb();
	fc::remove_all(get_data_dir() / "transactions");
	initialize_leveldb();
}


void object_database::close()
{
}

const object* object_database::find_object( object_id_type id )const
{
   return get_index(id.space(),id.type()).find( id );
}
const object& object_database::get_object( object_id_type id )const
{
   return get_index(id.space(),id.type()).get( id );
}

const index& object_database::get_index(uint8_t space_id, uint8_t type_id)const
{
   FC_ASSERT( _index.size() > space_id, "", ("space_id",space_id)("type_id",type_id)("index.size",_index.size()) );
   FC_ASSERT( _index[space_id].size() > type_id, "", ("space_id",space_id)("type_id",type_id)("index[space_id].size",_index[space_id].size()) );
   const auto& tmp = _index[space_id][type_id];
   FC_ASSERT( tmp );
   return *tmp;
}
index& object_database::get_mutable_index(uint8_t space_id, uint8_t type_id)
{
   FC_ASSERT( _index.size() > space_id, "", ("space_id",space_id)("type_id",type_id)("index.size",_index.size()) );
   FC_ASSERT( _index[space_id].size() > type_id , "", ("space_id",space_id)("type_id",type_id)("index[space_id].size",_index[space_id].size()) );
   const auto& idx = _index[space_id][type_id];
   FC_ASSERT( idx, "", ("space",space_id)("type",type_id) );
   return *idx;
}

void object_database::flush()
{
//   ilog("Save object_database in ${d}", ("d", _data_dir));
	if ("" == _data_dir){
		return;
	}
   for( uint32_t space = 0; space < _index.size(); ++space )
   {
      fc::create_directories( _data_dir / "object_database" / fc::to_string(space) );
      const auto types = _index[space].size();
      for( uint32_t type = 0; type  <  types; ++type )
         if( _index[space][type] )
            _index[space][type]->save( _data_dir / "object_database" / fc::to_string(space)/fc::to_string(type) );
   }
}

void object_database::wipe(const fc::path& data_dir)
{
   close();
   ilog("Wiping object database...");
   fc::remove_all(data_dir / "object_database");
   ilog("Done wiping object databse.");
}

void object_database::open(const fc::path& data_dir)
{ try {
   ilog("Opening object database from ${d} ...", ("d", data_dir));
   _data_dir = data_dir;
   for( uint32_t space = 0; space < _index.size(); ++space )
      for( uint32_t type = 0; type  < _index[space].size(); ++type )
         if( _index[space][type] )
            _index[space][type]->open( _data_dir / "object_database" / fc::to_string(space)/fc::to_string(type) );
   ilog( "Done opening object database." );

} FC_CAPTURE_AND_RETHROW( (data_dir) ) }


void object_database::pop_undo()
{ try {
   _undo_db.pop_commit();
} FC_CAPTURE_AND_RETHROW() }

void object_database::save_undo( const object& obj )
{
   _undo_db.on_modify( obj );
}

void object_database::save_undo_add( const object& obj )
{
   _undo_db.on_create( obj );
}

void object_database::save_undo_remove(const object& obj)
{
   _undo_db.on_remove( obj );
}

//void Cached_levelDb::Close(const leveldb::DB* l_db) {
//	leveldb::WriteOptions wop;
//	
//	if (l_db != nullptr) {
//		Flush(wop, l_db);
//	}
//}
//leveldb::Status Cached_levelDb::Open(const leveldb::Options& options, const std::string& dbname, const leveldb::DB* l_db) {
//	if (l_db != nullptr) {
//		delete l_db;
//	}
//	auto open_status = leveldb::DB::Open(options, dbname, &l_db);
//	if (!open_status.ok())
//	{
//		l_db = nullptr;
//	}
//	return open_status;
//};
leveldb::Status Cached_levelDb::Put(leveldb::Slice key, leveldb::Slice value) {
	auto exist_iter = cache_delete.find(key.ToString());
	if (exist_iter != cache_delete.end()) {
		cache_delete.erase(exist_iter);
	}
	cache_store[key.ToString()] = value.ToString();
	return leveldb::Status::OK();
};
leveldb::Status Cached_levelDb::Get(const leveldb::ReadOptions& read_op, leveldb::Slice key, std::string* value,leveldb::DB* l_db) {
	auto exist = cache_store.count(key.ToString());
	if (exist == 0) {
		string temp;
		auto ret =l_db->Get(read_op,key,&temp);
		if (!ret.ok())
		{
			return ret;
		}
		auto exist_iter = cache_delete.find(key.ToString());
		if (exist_iter != cache_delete.end())
		{
			return leveldb::Status::NotFound("Not found");
		}
		*value = temp;
		return leveldb::Status::OK();
	}
	else {
		auto temp = cache_store[key.ToString()];
		*value = temp;
		return leveldb::Status::OK();
	}
};
leveldb::Status Cached_levelDb::Delete( leveldb::Slice key) {
	auto exist_iter = cache_store.find(key.ToString());
	if (exist_iter != cache_store.end()) {
		cache_store.erase(exist_iter);
	}
	cache_delete.insert(key.ToString());
	return leveldb::Status::OK();
};
leveldb::Status Cached_levelDb::Flush(leveldb::WriteOptions w_op,leveldb::DB* l_db) {
	leveldb::WriteBatch wb;
	for (auto iter : cache_store)
	{
		wb.Put(iter.first, iter.second);
	}
	for (auto iter_d : cache_delete)
	{
		wb.Delete(iter_d);
	}
	if (l_db == nullptr)
	{
		return leveldb::Status::IOError("level db ptr is NULL");
	}
	//w_op.sync = true;
	auto ret = l_db->Write(w_op, &wb);
	if (ret.ok())
	{
		cache_store.erase(cache_store.begin(), cache_store.end());
		cache_delete.erase(cache_delete.begin(), cache_delete.end());
	}
	return ret;
};
vector<string> Cached_levelDb::GetToDelete(const leveldb::ReadOptions& read_op, std::string key, leveldb::DB* l_db, bool getKey) {
	vector<string> ret;
	for (auto iter_d : cache_store)
	{
		auto store_key = iter_d.first;
		auto pos = store_key.find(key);
		if (pos != store_key.npos && store_key != key)
		{
			if (getKey)
			{
				ret.push_back(store_key);
			}
			else {
				ret.push_back(iter_d.second);
			}
			
		}
	}
	leveldb::Iterator* it = l_db->NewIterator(read_op);
	for (it->Seek(key); it->Valid(); it->Next()) {
		if (!it->key().starts_with(key))
			break;
		if (it->key().ToString() == key)
			continue;
		std::cout << "invoke_result " << it->key().ToString() << ":" << it->value().ToString() << std::endl;
		auto exist_iter = cache_delete.find(it->key().ToString());
		if (exist_iter != cache_delete.end())
		{
			continue;
		}
		if (getKey)
		{
			ret.push_back(it->key().ToString());
		}
		else {
			ret.push_back(it->value().ToString());
		}
		
	}
	delete it;
	return ret;
};

} } // namespace graphene::db
