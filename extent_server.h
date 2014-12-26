// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"
#include "yfs_client.h"

#include "rsm.h"
#include "rsm_state_transfer.h"

class extent_server : public rsm_state_transfer {

 public:
  extent_server(class rsm *_rsms = 0);
  struct file {
      std::string content;
      extent_protocol::attr file_attr;
  };
  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int setattr(extent_protocol::extentid_t id, extent_protocol::attr&);
  int remove(extent_protocol::extentid_t id, int &);
  std::string marshal_state();
  void unmarshal_state(std::string state);
 private:
  pthread_mutex_t operation_lock;
  std::map<extent_protocol::extentid_t, file> file_map;
  class rsm *rsms;



};

#endif 







