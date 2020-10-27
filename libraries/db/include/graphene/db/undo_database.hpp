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
#include <fc/crypto/ripemd160.hpp>
#include <graphene/db/object.hpp>
#include <deque>
#include <fc/exception/exception.hpp>
#include <fstream>
#include <map>
#include <set>
#include <graphene/db/serializable_undo_state.hpp>
#include <leveldb/db.h>
#define GRAPHENE_UNDO_BUFF_MAX_SIZE 20

namespace graphene {
	namespace db {

		using namespace boost::multi_index;
		using std::map;
		using fc::flat_set;
		class object_database;
		struct serializable_undo_state;
		struct undo_state
		{
			undo_state() {}
			map<object_id_type, unique_ptr<object> > old_values;
			map<object_id_type, object_id_type>      old_index_next_ids;
			std::set<object_id_type>                 new_ids;
			map<object_id_type, unique_ptr<object> > removed;
			serializable_undo_state get_serializable_undo_state() const;
			undo_state(const serializable_undo_state& sta);
			undo_state& operator=(const serializable_undo_state& sta);
			undo_state& operator=(const undo_state& sta);
			void reset();
		};
		class undo_storage
		{
		public:
			void open(const fc::path& dbdir);
			bool is_open()const;
			void flush();
			void close();
			~undo_storage() { close(); };
			bool store(const undo_state_id_type & _id, const serializable_undo_state& b);
			undo_state_id_type store_undo_state(const undo_state& b);
			bool remove(const undo_state_id_type& id);
			bool get_state(const undo_state_id_type& id,undo_state& state) const ;
			bool                   contains(const undo_state_id_type& id)const;
			block_id_type          fetch_block_id(uint32_t block_num)const;
			optional<serializable_undo_state> fetch_optional(const undo_state_id_type& id)const;
			optional<serializable_undo_state> fetch_by_number(uint32_t block_num)const;
			optional<serializable_undo_state> last()const;
			optional<undo_state_id_type> last_id()const;
		private:
			leveldb::DB* db = NULL;;
			leveldb::Status open_status;
		};

		/**
		 * @class undo_database
		 * @brief tracks changes to the state and allows changes to be undone
		 *
		 */
		class undo_database
		{
		public:
			undo_database(object_database& db);

			class session
			{
			public:
				session(session&& mv)
					:_db(mv._db), _apply_undo(mv._apply_undo)
				{
					mv._apply_undo = false;
				}
				~session() {
					try {
						if (_apply_undo) _db.undo();
					}
					catch (const fc::exception& e)
					{
						elog("${e}", ("e", e.to_detail_string()));
						throw; // maybe crash..
					}
					if (_disable_on_exit) _db.disable();
				}
				void commit() { _apply_undo = false; _db.commit(); }
				void undo() { if (_apply_undo) _db.undo(); _apply_undo = false; }
				void merge() { if (_apply_undo) _db.merge(); _apply_undo = false; }

				session& operator = (session&& mv)
				{
					try {
						if (this == &mv) return *this;
						if (_apply_undo) _db.undo();
						_apply_undo = mv._apply_undo;
						mv._apply_undo = false;
						return *this;
					} FC_CAPTURE_AND_RETHROW()
				}

			private:
				friend class undo_database;
				session(undo_database& db, bool disable_on_exit = false) : _db(db), _disable_on_exit(disable_on_exit) {}
				undo_database& _db;

				bool _apply_undo = true;
				bool _disable_on_exit = false;
			};

			void    disable();
			void    discard();
			void    enable();
			bool    enabled()const { return !_disabled; }
			int     get_active_session() { return _active_sessions; }
			session start_undo_session(bool force_enable = false);
			/**
			 * This should be called just after an object is created
			 */
			void on_create(const object& obj);
			/**
			 * This should be called just before an object is modified
			 *
			 * If it's a new object as of this undo state, its pre-modification value is not stored, because prior to this
			 * undo state, it did not exist. Any modifications in this undo state are irrelevant, as the object will simply
			 * be removed if we undo.
			 */
			void on_modify(const object& obj);
			/**
			 * This should be called just before an object is removed.
			 *
			 * If it's a new object as of this undo state, its pre-removal value is not stored, because prior to this undo
			 * state, it did not exist. Now that it's been removed, it doesn't exist again, so nothing has happened.
			 * Instead, remove it from the list of newly created objects (which must be deleted if we undo), as we don't
			 * want to re-delete it if this state is undone.
			 */
			void on_remove(const object& obj);

			/**
			 *  Removes the last committed session,
			 *  note... this is dangerous if there are
			 *  active sessions... thus active sessions should
			 *  track
			 */
			void pop_commit();

			std::size_t size()const { return _stack.size()+back.size(); }
			void set_max_size(size_t new_max_size);
			size_t max_size()const;

			const undo_state& head()const;

			void save_to_file(const fc::string& path);
			void from_file(const fc::string& path);
			void reset();
			void remove_storage();
		private:
			void undo();
			void merge();
			void commit();

			uint32_t                _active_sessions = 0;
			bool                    _disabled = true;
			std::deque<undo_state_id_type>  _stack;
			object_database&        _db;
			std::unique_ptr<undo_storage>           state_storage;
			string					storage_path;
			size_t                  _max_size = 1440;
			//back保存当前活跃的state,不活跃的将id存入stack,数据存入db;
			//在保存时需要将back现存入stack和db
			//在初始化时，从文件恢复stack和storage，根据stack的尾部的id，从storage中找到对应state，反序列化赋值给back
			std::deque<undo_state>              back;
			int max_back_size = GRAPHENE_UNDO_BUFF_MAX_SIZE;
		};

	}
} // graphene::db
