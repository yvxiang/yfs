// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"

    
class lock_server {
private:
    typedef enum {
        FREE,
        BUSY
    }lock_stat;

    typedef struct {
        lock_stat cur_stat;
        long long holder_id;
    }lock_info;

protected:
  int nacquire;

 public:
  lock_server();
  ~lock_server() {};
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
 private:
  std::map<lock_protocol::lockid_t, lock_info> lock_dic;
  pthread_mutex_t operation_lock;

};

#endif 







