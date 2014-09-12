// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// The calls assume that the caller holds a lock on the extent

extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  extent_protocol::status ret = extent_protocol::OK;

  std::map<extent_protocol::extentid_t, file>::iterator it;
  it = file_cache.find(eid);

  if(it != file_cache.end()) {
      buf = it->second.content;
  } else {
      ret = cl->call(extent_protocol::get, eid, buf);
      file_cache.insert(std::make_pair(eid, file(buf)));
  }
  return ret;
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;
  /*
  ret = cl->call(extent_protocol::getattr, eid, attr);
  */
  std::map<extent_protocol::extentid_t, file>::iterator it;
  it = file_cache.find(eid);
  if(it != file_cache.end()) {
      attr.size = it->second.file_attr.size;
      attr.atime = it->second.file_attr.atime;
      attr.ctime = it->second.file_attr.ctime;
      attr.mtime = it->second.file_attr.mtime;
  }
  return ret;
}

extent_protocol::status
extent_client::setattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr attr)

{
    extent_protocol::status ret = extent_protocol::OK;
    printf("extent_client::setattr %llu %u\n", eid, attr.size);
    ret = cl->call(extent_protocol::setattr, eid, attr);
    return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  /*
  int r;
  ret = cl->call(extent_protocol::put, eid, buf, r);
  printf("extent_client::put\n%s\n", buf.c_str());
  */

  std::map<extent_protocol::extentid_t, file>::iterator it;
  it = file_cache.find(eid);
  if(it != file_cache.end()) {
      it->second.content = buf;
      it->second.dirty = true;
      it->second.file_attr.mtime = it->second.file_attr.atime
                                    = time(NULL);
  }

  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;
  /*
  int r;
  ret = cl->call(extent_protocol::remove, eid, r);
  */
  std::map<extent_protocol::extentid_t, file>::iterator it;
  it = file_cache.find(eid);
  if(it != file_cache.end()) {
      file_cache.erase(it);
  }
  return ret;
}


