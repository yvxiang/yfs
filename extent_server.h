// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"
#include "yfs_client.h"

class extent_server {

 public:
  extent_server();
  struct file {
      std::string content;
      extent_protocol::attr file_attr;
  };
  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
 private:
  pthread_mutex_t operation_lock;
  std::map<extent_protocol::extentid_t, file> file_map;



};

#endif 







