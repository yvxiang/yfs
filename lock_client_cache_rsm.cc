// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache_rsm.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"

#include "rsm_client.h"

static void *
releasethread(void *x)
{
  lock_client_cache_rsm *cc = (lock_client_cache_rsm *) x;
  cc->releaser();
  return 0;
}

int lock_client_cache_rsm::last_port = 0;

lock_client_cache_rsm::lock_client_cache_rsm(std::string xdst, 
				     class lock_release_handler *_lu)
  : lock_client(xdst), lu(_lu)
{
  srand(time(NULL)^last_port);
  rlock_port = ((rand()%32000) | (0x1 << 10));
  const char *hname;
  // VERIFY(gethostname(hname, 100) == 0);
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlock_port;
  id = host.str();
  last_port = rlock_port;
  rpcs *rlsrpc = new rpcs(rlock_port);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache_rsm::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache_rsm::retry_handler);
  xid = 0;
  // You fill this in Step Two, Lab 7
  // - Create rsmc, and use the object to do RPC 
  //   calls instead of the rpcc object of lock_client
  pthread_t th;
  int r = pthread_create(&th, NULL, &releasethread, (void *) this);
  VERIFY (r == 0);

  pthread_cond_init(&acquire_wait_cond, NULL);
  pthread_mutex_init(&lock_stat_map_lock, NULL);

  rsmc = new rsm_client(xdst);
}


void
lock_client_cache_rsm::releaser()
{

  // This method should be a continuous loop, waiting to be notified of
  // freed locks that have been revoked by the server, so that it can
  // send a release RPC.
    while(1) {
        release_entry lock_to_release;
        release_queue.deq(&lock_to_release);
        //tprintf("releaser: release lock %llu\n", lock_to_release.lid);

        if(lu)
            lu->dorelease(lock_to_release.lid);
        int r;
        rsmc->call(lock_protocol::release, lock_to_release.lid, id,
            xid, r);

        pthread_mutex_lock(&lock_stat_map_lock);

        std::map<lock_protocol::lockid_t, lock_stat>::iterator it;
        it = lock_stat_map.find(lock_to_release.lid);
        it->second.ls = NONE;

        pthread_cond_broadcast(&acquire_wait_cond);
        pthread_mutex_unlock(&lock_stat_map_lock);
        //tprintf("releaser: release lock %llu returns\n", lock_to_release.lid);
    }
}


lock_protocol::status
lock_client_cache_rsm::acquire(lock_protocol::lockid_t lid)
{
  int ret = lock_protocol::OK;
  bool have_lock = false, err = false;
  std::map<lock_protocol::lockid_t, lock_stat>::iterator it;

  //tprintf("acquire:%s:%u try to get lock %llu\n", id.c_str(), pthread_self(), lid);
  pthread_mutex_lock(&lock_stat_map_lock);

  it = lock_stat_map.find(lid);
  if(it == lock_stat_map.end()) {
      it = lock_stat_map.insert(std::make_pair(lid, lock_stat())).first;
  }

  do {
      if(it->second.ls == NONE || (it->second.ls == ACQUIRING &&
                                    it->second.retry == true)) {
          it->second.ls = ACQUIRING;
          it->second.retry = false;
          it->second.xid = xid;
          lock_protocol::xid_t cur_xid = xid;
          xid++;

          /*
          if(it->second.ls == NONE) {
              cur_xid = xid;
              xid++;
          }
          */
        //  it->second.xid = cur_xid;;

          pthread_mutex_unlock(&lock_stat_map_lock);
          int r;
          lock_protocol::status  rpcret = rsmc->call(lock_protocol::acquire,
                                            lid, id, cur_xid, r); 

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
      } else if(it->second.ls == LOCKED ||
                                 it->second.ls == RELEASING) {
          pthread_cond_wait(&acquire_wait_cond, &lock_stat_map_lock);
          
      } else if(it->second.ls == ACQUIRING && it->second.retry == false) {
          timespec outtime;
          clock_gettime(CLOCK_REALTIME, &outtime);
          outtime.tv_sec += 3;
          int t = pthread_cond_timedwait(&acquire_wait_cond,
                                &lock_stat_map_lock, &outtime);
          if(t == ETIMEDOUT) {
              tprintf("y:sleep 3 seconds\n");
              it->second.retry = true;
          }
      }
  } while(!have_lock && !err);

  pthread_mutex_unlock(&lock_stat_map_lock);
  return ret;
}

lock_protocol::status
lock_client_cache_rsm::release(lock_protocol::lockid_t lid)
{
  int ret = lock_protocol::OK;
  std::map<lock_protocol::lockid_t, lock_stat>::iterator it;

  pthread_mutex_lock(&lock_stat_map_lock);

  it = lock_stat_map.find(lid);
  if(it == lock_stat_map.end()) {
      //tprintf("ERROR %s:%u try to release lock %llu that doesn't hold\n",
       //                                 id.c_str(), pthread_self(), lid);
      ret = lock_protocol::NOENT;
  } else {
      if(it->second.revoke == true) {
          it->second.ls = RELEASING;
          it->second.revoke = false;

          pthread_mutex_unlock(&lock_stat_map_lock);

          if(lu)
              lu->dorelease(lid);
          //tprintf("%s:%u try to release %llu\n", id.c_str(), pthread_self(), lid);
          int r;
          rsmc->call(lock_protocol::release, lid, id, xid, r);

          pthread_mutex_lock(&lock_stat_map_lock);
          it->second.ls = NONE;
          pthread_cond_broadcast(&acquire_wait_cond);
      } else {
          it->second.ls = FREE;
          pthread_cond_broadcast(&acquire_wait_cond);
      }
  }

  pthread_mutex_unlock(&lock_stat_map_lock);
  return ret;

}


rlock_protocol::status
lock_client_cache_rsm::revoke_handler(lock_protocol::lockid_t lid, 
			          lock_protocol::xid_t _xid, int &)
{
  tprintf("y:receive revoke client = %s, lid = %llu, xid = %llu\n",
                                id.c_str(), lid, _xid);
  int ret = rlock_protocol::OK;
  std::map<lock_protocol::lockid_t, lock_stat>::iterator it;

  pthread_mutex_lock(&lock_stat_map_lock);

  //tprintf("%s receive revoke %llu\n", id.c_str(), lid);
  it = lock_stat_map.find(lid);
  if(it != lock_stat_map.end()) {
      if(it->second.xid == _xid) {
          if(it->second.ls == FREE) {
              it->second.ls = RELEASING;
              it->second.revoke = false;
              release_queue.enq(release_entry(lid, _xid));
          } else {
              if(it->second.revoke)
                  tprintf("ERR %s try revoke %llu twice\n", id.c_str(), lid);
              it->second.revoke = true;
          }
      } else {
          tprintf("ERROR try to revoke a lock that hasn't acquired\n");
      }
  } else {
      tprintf("ERROR try to revoke with a wrong xid(%d, %d)\n", it->second.xid, _xid);
  }

  pthread_mutex_unlock(&lock_stat_map_lock);

  return ret;
}

rlock_protocol::status
lock_client_cache_rsm::retry_handler(lock_protocol::lockid_t lid, 
			         lock_protocol::xid_t xid, int &)
{
  int ret = rlock_protocol::OK;
  std::map<lock_protocol::lockid_t, lock_stat>::iterator it;

  pthread_mutex_lock(&lock_stat_map_lock);

  //tprintf("%s receive retry lock %llu from server\n", id.c_str(), lid);
  tprintf("y:receive retry lid = %llu\n", lid);
  it = lock_stat_map.find(lid);
  if(it == lock_stat_map.end()) {
      tprintf("ERROR try to retry a lock that hasn't acquire");
  } else {
      if(it->second.xid == xid) {
          it->second.retry = true;
          pthread_cond_broadcast(&acquire_wait_cond);
      } else {
          tprintf("ERROR try to retry with a wrong xid\n");
      }
  }

  pthread_mutex_unlock(&lock_stat_map_lock);
  return ret;
}
void lock_release_handler::dorelease(lock_protocol::lockid_t lid)
{
  release_ec_pointer->flush(lid);
}

void lock_client_cache_rsm::set_lu_pointer(lock_release_handler *p)
{
        lu = p;
}
