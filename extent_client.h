// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <map>
#include <string>
#include "extent_protocol.h"
#include "rpc.h"

#include "rsm_client.h"

class extent_client {
 public:
  struct file {
      bool dirty;
      std::string content;
//      extent_protocol::attr file_attr;
      file(std::string &buf) : dirty(false), content(buf) {}
  };
  struct _attr {
      bool dirty;
      extent_protocol::attr file_attr;
  };
 private:
  rpcc *cl;
  rsm_client *rsmc;
  std::map<extent_protocol::extentid_t, file> file_cache;
  std::map<extent_protocol::extentid_t, _attr> file_attr_cache;

 public:
  extent_client(std::string dst);

  extent_protocol::status get(extent_protocol::extentid_t eid, 
			      std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				  extent_protocol::attr &a);
  extent_protocol::status setattr(extent_protocol::extentid_t eid,
                  extent_protocol::attr a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
  void flush(extent_protocol::extentid_t eid);
};

#endif 

