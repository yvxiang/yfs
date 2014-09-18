// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <set>
#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"
#include "extent_client.h"

// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 5.
class lock_release_user {
 public:
  virtual void dorelease(lock_protocol::lockid_t) = 0;
  virtual ~lock_release_user() {};
};

class lock_release_handler : public lock_release_user {
  public:
    lock_release_handler(extent_client *p) : release_ec_pointer(p) {};
    void dorelease(lock_protocol::lockid_t);
  private:
    extent_client *release_ec_pointer;
};

class lock_client_cache : public lock_client {

 typedef enum {
     NONE,
     FREE,
     LOCKED,
     ACQUIRING,
     RELEASING
 }LOCK_STAT;

 struct lock_stat {
     LOCK_STAT ls;
     bool revoke;
     bool retry;
     lock_stat() : ls(NONE), revoke(false), retry(false) {}
 };

 private:
  lock_release_handler *lu;
  int rlock_port;
  std::string hostname;
  std::string id;
  std::map<lock_protocol::lockid_t, lock_stat> lock_stat_map;
  pthread_mutex_t lock_stat_map_lock;
  pthread_cond_t acquire_wait_cond;
 public:
  lock_client_cache(std::string xdst, lock_release_handler *l = 0);
  void set_lu_pointer(lock_release_handler *l);
  virtual ~lock_client_cache() {};
  lock_protocol::status acquire(lock_protocol::lockid_t);
  lock_protocol::status release(lock_protocol::lockid_t);
  rlock_protocol::status revoke_handler(lock_protocol::lockid_t, 
                                        int &);
  rlock_protocol::status retry_handler(lock_protocol::lockid_t, 
                                       int &);
};


#endif
