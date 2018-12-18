#pragma once
#include <fc/crypto/ripemd160.hpp>
#include <unordered_map>
#include <fc/variant_object.hpp>
#include <graphene/db/object.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <fstream>
#include <leveldb/db.h>
#include <map>
#include <set>
using namespace graphene::chain;
namespace graphene {
	namespace db {
		struct serializable_obj
		{
			int s;
			int t;
			fc::variant obj;
			serializable_obj() {}
			serializable_obj(const object& obj);
			unique_ptr<object>  to_object() const;
		};
		typedef fc::ripemd160  undo_state_id_type;
		struct serializable_undo_state//:public abstract_object<serializable_undo_state>
		{
		public:
			 //static const uint8_t space_id = chain::implementation_ids;
			 //static const uint8_t type_id = chain::impl_undo_state_ob_type;

			serializable_undo_state() {}
			serializable_undo_state(const serializable_undo_state& sta);
			std::map<object_id_type, serializable_obj > old_values;
			std::map<object_id_type, object_id_type>      old_index_next_ids;
			std::set<object_id_type>                 new_ids;
			std::map<object_id_type, serializable_obj> removed;
			undo_state_id_type undo_id()const;
		};


		//struct by_undo {};
		//struct by_object_id {};
		//typedef multi_index_container<
		//    serializable_undo_state,
		//    indexed_by<
		//    ordered_unique< tag<by_object_id>, member< object, object_id_type, &object::id > >,
		//    ordered_unique< tag<by_undo>, const_mem_fun< serializable_undo_state, undo_state_id_type, &serializable_undo_state::undo_id >>
		//    >
		//> undo_state_index_type;
		////
		//typedef chain::generic_index<serializable_undo_state, undo_state_index_type> undo_index;

	}
}
FC_REFLECT(graphene::db::serializable_obj, (s)(t)(obj))
FC_REFLECT(graphene::db::serializable_undo_state, (old_values)(old_index_next_ids)(new_ids)(removed))
