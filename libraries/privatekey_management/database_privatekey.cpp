/**
* Author: wengqiang (email: wens.wq@gmail.com  site: qiangweng.site)
*
* Copyright © 2015--2018 . All rights reserved.
*
* File: database_privatekey.cpp
* Date: 2018-01-11
*/

#include "graphene/privatekey_management/database_privatekey.hpp"
#include "fc/crypto/aes.hpp"

namespace graphene {
	namespace privatekey_management {

		struct index_entry
		{
			uint64_t         key_id = 0;
			uint64_t       key_pos  = 0;
			uint64_t   keydata_size = 0;


		};	

		void database_privatekey::open(const fc::path& dbdir)
		{
			try
			{
				fc::create_directories(dbdir);
				_keys.exceptions(std::ios_base::failbit | std::ios_base::badbit);


				if (!fc::exists(dbdir / "index"))
				{
					_key_index.open((dbdir / "index").generic_string().c_str(), std::fstream::binary | std::fstream::in | std::fstream::out | std::fstream::trunc);
					_keys.open((dbdir / "keys").generic_string().c_str(), std::fstream::binary | std::fstream::in | std::fstream::out | std::fstream::trunc);
				}
				else
				{
					_key_index.open((dbdir / "index").generic_string().c_str(), std::fstream::binary | std::fstream::in | std::fstream::out);
					_keys.open((dbdir / "keys").generic_string().c_str(), std::fstream::binary | std::fstream::in | std::fstream::out);
				}


			}FC_CAPTURE_AND_RETHROW( (dbdir) )
		}

		bool database_privatekey::is_open()const
		{
			return _keys.is_open();
		}


		void database_privatekey::close()
		{
			_keys.close();
			_key_index.close();
		}

		void database_privatekey::flush()
		{
			_key_index.flush();
			_keys.flush();
		}

		void database_privatekey::store(const crosschain_privatekey_data& key_data, fc::sha512 checksum)
		{
			//set key store data
			crosschain_privatekey_store  data;
			data.addr = key_data.addr;
			auto plain_key = fc::raw::pack(key_data.wif_key);
			data.cipher_keys = fc::aes_encrypt(checksum, plain_key);
			data.id = key_data.id;

			auto pack_key_data = fc::raw::pack(data);

			//write index data
			index_entry e;
			_keys.seekp(0, _keys.end);
			e.key_id = key_data.id;
			e.key_pos = _keys.tellp();
			e.keydata_size = pack_key_data.size();
			_key_index.seekp(sizeof(index_entry) * (e.key_id - 1));
			_key_index.write((char *)&e, sizeof(e));
			
			//write key data			
			_keys.write(pack_key_data.data(), pack_key_data.size());

		}

		void database_privatekey::remove(const crosschain_privatekey_data& key_data)
		{

			try
			{
				index_entry e;
				auto index_pos = sizeof(e) * (key_data.id - 1);

				_key_index.seekg(0, _key_index.end);
				if (_key_index.tellg() <= int64_t(index_pos))
					FC_THROW_EXCEPTION(fc::key_not_found_exception, "key ${id} not contained in privatekey database", ("id", key_data.id));

				_key_index.seekg(index_pos);
				_key_index.read((char *)&e, sizeof(e));

				if (e.key_id == key_data.id)
				{
					e.keydata_size = 0;

					_key_index.seekp(sizeof(e)*(key_data.id -  1));
					_key_index.write((char *)&e, sizeof(e));
				}
			}FC_CAPTURE_AND_RETHROW( (key_data.id) )
			


		}

		bool database_privatekey::contains(const uint64_t key_id)const
		{
			if (key_id <= 0)
			{
				return false;
			}


			index_entry e;
			auto index_pos = sizeof(e) * (key_id - 1);
			_key_index.seekg(0, _key_index.end);

			if (_key_index.tellg() <= int64_t(index_pos))
				return false;

			_key_index.seekg(index_pos);
			_key_index.read((char *)&e, sizeof(e));

			return e.key_id == key_id && e.keydata_size != 0;


		}

		fc::optional<crosschain_privatekey_data> database_privatekey::fetch_by_id(const uint64_t key_id, fc::sha512  checksum)const
		{
			assert(key_id > 0);

			index_entry e;
			auto index_pos = sizeof(e) * (key_id - 1);

			_key_index.seekg(0, _key_index.end);
			if ( _key_index.tellg() <= int64_t(index_pos))
				FC_THROW_EXCEPTION(fc::key_not_found_exception, "privatekey id ${key_id} not contained in private_key database", ("id", key_id));

			_key_index.seekg(index_pos);
			_key_index.read((char *)&e, sizeof(e));

			FC_ASSERT(e.key_id > 0, "Empty key_id in privatekey database (maybe corrupt on disk?)");
			if (e.key_id != key_id)
				return fc::optional<crosschain_privatekey_data>();


			std::vector<char> data(e.keydata_size);
			_keys.seekg(e.key_pos);
			if (e.keydata_size == 0)
				return fc::optional<crosschain_privatekey_data>();
			else
			{
				_keys.read(data.data(), e.keydata_size);
				auto result = fc::raw::unpack<crosschain_privatekey_store>(data);
				FC_ASSERT(result.id == e.key_id);

				crosschain_privatekey_data  priv_key;
				priv_key.id = result.id;
				priv_key.addr = result.addr;

				auto wif_key_data = fc::aes_decrypt(checksum, result.cipher_keys);
				priv_key.wif_key = fc::raw::unpack<std::string>(wif_key_data);

				return priv_key;

			}


		}




	}
} // end namespace graphene::privatekey_management


FC_REFLECT(graphene::privatekey_management::index_entry, (key_pos)(key_size)(addr));