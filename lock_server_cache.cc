// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"


lock_server_cache::lock_server_cache()
{
    pthread_mutex_init(&lock_stat_map_lock, NULL);
}


int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, 
                               int &)
{
  lock_protocol::status ret = lock_protocol::OK;
  std::map<lock_protocol::lockid_t, lock_stat>::iterator it;
  std::string client_to_revoke;
  pthread_mutex_lock(&lock_stat_map_lock);
//  printf("server: %s want to get lock %u\n", id.c_str() ,lid);


  it = lock_stat_map.find(lid);
  if(it == lock_stat_map.end()) {
      it = lock_stat_map.insert(std::make_pair(lid, lock_stat())).first;
  }
  if(it->second.holded) {
      ret = lock_protocol::RETRY;
      it->second.waiter.insert(id);
      if(!it->second.revoke) {
          it->second.revoke = true;
          client_to_revoke = it->second.holder;
      }
  } else {
  //printf("server: %s get lock %u\n", id.c_str() ,lid);
      ret = lock_protocol::OK;
      it->second.waiter.erase(id);
      it->second.holded = true;
      it->second.holder = id;
      it->second.revoke = false;
  }

  pthread_mutex_unlock(&lock_stat_map_lock);

  if(!client_to_revoke.empty()) {
      int r;
      handle(client_to_revoke).safebind()->call(rlock_protocol::revoke,
                                            lid, r);
  }
  return ret;                                                  

}

int 
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, 
         int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  pthread_mutex_lock(&lock_stat_map_lock);

  std::map<lock_protocol::lockid_t, lock_stat>::iterator it;
  std::string client_to_wake_up;
  bool more_to_wake_up = false;
  if((it = lock_stat_map.find(lid)) == lock_stat_map.end()) {
      printf("ERROR try to release a lock that doesn't exist\n");
      ret = lock_protocol::NOENT;
  } else {
      it->second.holded = false;
      it->second.holder = "";
      if(!it->second.waiter.empty()) {
          client_to_wake_up = *it->second.waiter.begin();
          if(it->second.waiter.size() > 1) {
              more_to_wake_up = true;
          }
      }
  }


  pthread_mutex_unlock(&lock_stat_map_lock);

  if(!client_to_wake_up.empty()) {
      handle(client_to_wake_up).safebind()->call(rlock_protocol::retry, 
                                                    lid, r);
      if(more_to_wake_up) 
          handle(client_to_wake_up).safebind()->call(rlock_protocol::revoke, 
                                                    lid, r);
  }

  return ret;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}

