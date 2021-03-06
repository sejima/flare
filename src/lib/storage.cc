/**
 *	storage.cc
 *
 *	implementation of gree::flare::storage
 *
 *	@author	Masaki Fujimoto <fujimoto@php.net>
 *
 *	$Id$
 */
#include "app.h"
#include "storage.h"

namespace gree {
namespace flare {

// {{{ ctor/dtor
/**
 *	ctor for storage
 */
storage::storage(string data_dir, int mutex_slot_size, int header_cache_size):
		_open(false),
		_data_dir(data_dir),
		_mutex_slot_size(mutex_slot_size),
		_mutex_slot(NULL),
		_header_cache_size(header_cache_size),
		_header_cache_map(NULL) {
	this->_mutex_slot = _new_ pthread_rwlock_t[mutex_slot_size];
	int i;
	for (i = 0; i < this->_mutex_slot_size; i++) {
		pthread_rwlock_init(&this->_mutex_slot[i], NULL);
	}
	pthread_rwlock_init(&this->_mutex_header_cache_map, NULL);

	this->_header_cache_map = tcmapnew();
}

/**
 *	dtor for storage
 */
storage::~storage() {
	_delete_(this->_mutex_slot);

	if (this->_header_cache_map) {
		tcmapdel(this->_header_cache_map);
	}
}
// }}}

// {{{ operator overloads
// }}}

// {{{ public methods
// }}}

// {{{ protected methods
int storage::_serialize_header(entry& e, uint8_t* data) {
	int offset = 0;

	// flag
	memcpy(data+offset, &e.flag, sizeof(e.flag));
	offset += sizeof(e.flag);

	// expire
	memcpy(data+offset, &e.expire, sizeof(e.expire));
	offset += sizeof(e.expire);

	// size
	memcpy(data+offset, &e.size, sizeof(e.size));
	offset += sizeof(e.size);
	
	// version
	memcpy(data+offset, &e.version, sizeof(e.version));
	offset += sizeof(e.version);

	return 0;
}

int storage::_unserialize_header(const uint8_t* data, int data_len, entry& e) {
	int offset = 0;

	// flag
	if (static_cast<int>(offset + sizeof(e.flag)) > data_len) {
		log_warning("data is smaller than expected header size (member=flag, data_len=%d, offset=%d, size=%d)", data_len, offset, sizeof(e.flag));
		return -1;
	}
	memcpy(&e.flag, data+offset, sizeof(e.flag));
	offset += sizeof(e.flag);

	// expire
	if (static_cast<int>(offset + sizeof(e.expire)) > data_len) {
		log_warning("data is smaller than expected header size (member=expire, data_len=%d, offset=%d, size=%d)", data_len, offset, sizeof(e.expire));
		return -1;
	}
	memcpy(&e.expire, data+offset, sizeof(e.expire));
	offset += sizeof(e.expire);
	
	// size
	if (static_cast<int>(offset + sizeof(e.size)) > data_len) {
		log_warning("data is smaller than expected header size (member=size, data_len=%d, offset=%d, size=%d)", data_len, offset, sizeof(e.size));
		return -1;
	}
	memcpy(&e.size, data+offset, sizeof(e.size));
	offset += sizeof(e.size);

	// version
	if (static_cast<int>(offset + sizeof(e.version)) > data_len) {
		log_warning("data is smaller than expected header size (member=version, data_len=%d, offset=%d, size=%d)", data_len, offset, sizeof(e.version));
		return -1;
	}
	memcpy(&e.version, data+offset, sizeof(e.version));
	offset += sizeof(e.version);
	
	log_debug("unserialized (flag=%d, expire=%d, size=%llu, version=%llu)", e.flag, e.expire, e.size, e.version);

	return offset;
}

int storage::_set_header_cache(string key, entry& e) {
	log_debug("set header into cache (key=%s, expire=%ld, version=%llu)", key.c_str(), e.expire, e.version);
	uint8_t tmp[sizeof(time_t) + sizeof(uint64_t)];
	time_t* p;
	uint64_t* q;

	p = reinterpret_cast<time_t*>(tmp);
	*p = e.expire == 0 ? stats_object->get_timestamp() : e.expire;
	q = reinterpret_cast<uint64_t*>(tmp+sizeof(time_t));
	*q = e.version;

	pthread_rwlock_wrlock(&this->_mutex_header_cache_map);
	tcmapput(this->_header_cache_map, key.c_str(), key.size(), tmp, sizeof(tmp));

	// cut front here
	int n = tcmaprnum(this->_header_cache_map) - this->_header_cache_size;
	if (n > 0) {
		log_debug("cutting front cache (n=%d, current=%d, size=%d)", n, tcmaprnum(this->_header_cache_map), this->_header_cache_size);
		tcmapcutfront(this->_header_cache_map, n);
	}
	pthread_rwlock_unlock(&this->_mutex_header_cache_map);

	return 0;
};
// }}}

// {{{ private methods
// }}}

}	// namespace flare
}	// namespace gree

// vim: foldmethod=marker tabstop=2 shiftwidth=2 autoindent
