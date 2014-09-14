#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>

#include <set>
#include <map>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"


class lock_server_cache {
 struct lock_stat {
     bool holded;
     bool revoke;
     std::string holder;
     std::set<std::string> waiter;
     lock_stat() : holded(false) {}
 };
 private:
  int nacquire;
  std::map<lock_protocol::lockid_t, lock_stat> lock_stat_map;
  pthread_mutex_t lock_stat_map_lock;
 public:
  lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(lock_protocol::lockid_t, std::string id, int &);
  int release(lock_protocol::lockid_t, std::string id, int &);
};

#endif
