#ifndef lock_server_cache_rsm_h
#define lock_server_cache_rsm_h

#include <string>

#include "lock_protocol.h"
#include "rpc.h"
#include "rsm_state_transfer.h"
#include "rsm.h"

#include <set>
#include <map>
#include "rpc/fifo.h"

class lock_server_cache_rsm : public rsm_state_transfer {
 typedef std::map<std::string, lock_protocol::xid_t> client_xid_map;
 typedef std::map<std::string, int> client_reply_map;
 struct lock_stat {
     bool holded;
     bool revoke;
     std::string holder;
     std::set<std::string> waiter;
     client_xid_map highest_xid_from_client;
     client_reply_map highest_xid_acquire_reply;
     client_reply_map highest_xid_release_reply;

     lock_stat() : holded(false), revoke(false) {}
 };

 struct revoke_entry {
     std::string client_id;
     lock_protocol::lockid_t lid;
     lock_protocol::xid_t xid;
     revoke_entry(std::string c_id = "", lock_protocol::lockid_t l_id = 0,
                                            lock_protocol::xid_t x_id = 0)
                    :client_id(c_id), lid(l_id), xid(x_id) {}
 };

 struct retry_entry {
     std::string client_id;
     lock_protocol::lockid_t lid;
     lock_protocol::xid_t xid;
     retry_entry(std::string c_id = "", lock_protocol::lockid_t l_id = 0,
                                            lock_protocol::xid_t x_id = 0)
                    : client_id(c_id), lid(l_id), xid(x_id) {}
 };

 private:
  int nacquire;
  class rsm *rsm;
  std::map<lock_protocol::lockid_t, lock_stat> lock_stat_map;
  fifo<revoke_entry> revoke_queue;
  fifo<retry_entry> retry_queue;
  pthread_mutex_t lock_stat_map_lock;
 public:
  lock_server_cache_rsm(class rsm *rsm = 0);
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  void revoker();
  void retryer();
  std::string marshal_state();
  void unmarshal_state(std::string state);
  int acquire(lock_protocol::lockid_t, std::string id, 
	      lock_protocol::xid_t, int &);
  int release(lock_protocol::lockid_t, std::string id, lock_protocol::xid_t,
	      int &);
};

#endif
