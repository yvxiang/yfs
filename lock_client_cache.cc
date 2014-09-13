// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"


lock_client_cache::lock_client_cache(std::string xdst, 
				     lock_release_handler *_lu)
  : lock_client(xdst), lu(_lu)
{
  rpcs *rlsrpc = new rpcs(0);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);

  const char *hname;
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlsrpc->port();
  id = host.str();

  pthread_mutex_init(&lock_stat_map_lock, NULL);
  pthread_cond_init(&acquire_wait_cond, NULL);
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
  int ret = lock_protocol::OK;
  bool have_lock = false, err = false;
  std::map<lock_protocol::lockid_t, lock_stat>::iterator it;
  pthread_mutex_lock(&lock_stat_map_lock);

  it = lock_stat_map.find(lid);
  if(it == lock_stat_map.end()) {
      it = lock_stat_map.insert(std::make_pair(lid, lock_stat())).first;
  }

  do {
      if(it->second.ls == NONE || (it->second.ls == ACQUIRING &&
                                    it->second.retry == true)
                                || it->second.ls == RELEASING) {
          it->second.ls = ACQUIRING;
          it->second.retry = false;

          pthread_mutex_unlock(&lock_stat_map_lock);

          int r;
          lock_protocol::status  rpcret = cl->call(lock_protocol::acquire,
                                            lid, id, r); 

          pthread_mutex_lock(&lock_stat_map_lock);

          if(rpcret == lock_protocol::OK) {
              it->second.ls = LOCKED;
              have_lock = true;
          } else if(rpcret == lock_protocol::RETRY) {
          } else {
              err = true;
              ret = rpcret;
          }
      } else if(it->second.ls == FREE) {
          it->second.ls = LOCKED;
          have_lock = true;
      } else if(it->second.ls == LOCKED || (it->second.ls ==
                    ACQUIRING && it->second.retry == false)) {
          pthread_cond_wait(&acquire_wait_cond, &lock_stat_map_lock);
      }
  } while(!have_lock && !err);

  pthread_mutex_unlock(&lock_stat_map_lock);
  return ret;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
  int ret = lock_protocol::OK;
  std::map<lock_protocol::lockid_t, lock_stat>::iterator it;
  pthread_mutex_lock(&lock_stat_map_lock);

  it = lock_stat_map.find(lid);
  if(it == lock_stat_map.end()) {
      printf("ERROR try to release a lock that doesn't exist\n");
      ret = lock_protocol::NOENT;
  } else {
      if(it->second.revoke == true) {
          it->second.revoke = false;
          it->second.ls = RELEASING;

         // firsh flush it out 
          lu->dorelease(lid);

          pthread_mutex_unlock(&lock_stat_map_lock);

          int r;
          ret = cl->call(lock_protocol::release, lid, id, r);

          pthread_mutex_lock(&lock_stat_map_lock);
      } else {
          it->second.ls = FREE;
          pthread_cond_broadcast(&acquire_wait_cond);
      }
  }

  pthread_mutex_unlock(&lock_stat_map_lock);
  return ret;
}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, 
                                  int &)
{
  int ret = rlock_protocol::OK;
  std::map<lock_protocol::lockid_t, lock_stat>::iterator it;
  pthread_mutex_lock(&lock_stat_map_lock);

  it = lock_stat_map.find(lid);
  if(it == lock_stat_map.end()) {
      printf("ERROR try to revoke a lock that doesn't exist\n");
  } else {
      if(it->second.ls == FREE) {
          it->second.ls = RELEASING;
          it->second.revoke = false;

          lu->dorelease(lid);

          pthread_mutex_unlock(&lock_stat_map_lock);

          int r;
          ret = cl->call(lock_protocol::release, lid, id, r);

          pthread_mutex_lock(&lock_stat_map_lock);
      } else {
          it->second.revoke = true;
      }
  }
  pthread_mutex_unlock(&lock_stat_map_lock);

  return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, 
                                 int &)
{
  int ret = rlock_protocol::OK;
  std::map<lock_protocol::lockid_t, lock_stat>::iterator it;

  pthread_mutex_lock(&lock_stat_map_lock);

  it = lock_stat_map.find(lid);
  if(it == lock_stat_map.end()) {
      printf("ERROR try to retry a lock that doesn't exist\n");
  } else {
      it->second.retry = true;
      pthread_cond_broadcast(&acquire_wait_cond);
  }

  pthread_mutex_unlock(&lock_stat_map_lock);
  return ret;
}

void
lock_release_handler::dorelease(lock_protocol::lockid_t lid)
{

    release_ec_pointer->flush(lid);
}
void
lock_client_cache::set_lu_pointer(lock_release_handler *p)
{
    lu = p;
}
