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
      printf("cache read %u size %d\n", eid, it->second.content.size());
      buf = it->second.content;
//      it->second.file_attr.atime = time(NULL);
      printf("get from cache %u %s\n", eid, buf.c_str());
  } else {
      ret = cl->call(extent_protocol::get, eid, buf);
      file_cache.insert(std::make_pair(eid, file(buf)));
      it = file_cache.find(eid);
 //     it->second.file_attr.atime = time(NULL);
      //printf("get from  %u %s\n", eid, buf.c_str());
  }
  
  
 /* 
  ret = cl->call(extent_protocol::get, eid, buf);
  */
  
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
  
  std::map<extent_protocol::extentid_t, _attr>::iterator it;
  it = file_attr_cache.find(eid);
  if(it != file_attr_cache.end()) {
      attr.size = it->second.file_attr.size;
      attr.atime = it->second.file_attr.atime;
      attr.ctime = it->second.file_attr.ctime;
      attr.mtime = it->second.file_attr.mtime;
  } else {
      ret = cl->call(extent_protocol::getattr, eid, attr);
      _attr new_attr;
      new_attr.file_attr.size = attr.size;
      new_attr.file_attr.atime = attr.atime;
      new_attr.file_attr.ctime = attr.ctime;
      new_attr.file_attr.mtime = attr.mtime;
      file_attr_cache.insert(std::make_pair(eid, new_attr));
      it = file_attr_cache.find(eid);
      it->second.dirty = false;
  }
  
  return ret;
}

extent_protocol::status
extent_client::setattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr attr)

{
    extent_protocol::status ret = extent_protocol::OK;
    printf("extent_client::setattr %llu %u\n", eid, attr.size);
    
    /*
    ret = cl->call(extent_protocol::setattr, eid, attr);
    */
    

    std::map<extent_protocol::extentid_t, _attr>::iterator it;
    it = file_attr_cache.find(eid);
    if(it != file_attr_cache.end()) {
        it->second.file_attr.size = attr.size;
        it->second.file_attr.atime = attr.atime;
        it->second.file_attr.ctime = attr.ctime;
        it->second.file_attr.mtime = attr.mtime;
        it->second.dirty = true;
    }
    
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
  if(it == file_cache.end()) {
      file_cache.insert(std::make_pair(eid, file(buf)));
      it = file_cache.find(eid);
  }
  it->second.dirty = true;
  it->second.content = buf;
//  it->second.file_attr.mtime = it->second.file_attr.atime
                        //= it->second.file_attr.ctime = time(NULL);
//  it->second.file_attr.size = it->second.content.size();
  file_attr_cache[eid].file_attr.atime
                        = file_attr_cache[eid].file_attr.mtime
                        = file_attr_cache[eid].file_attr.ctime
                        = time(NULL);
  file_attr_cache[eid].file_attr.size = it->second.content.size();
  file_attr_cache[eid].dirty = true;
  printf("write to cache %u %s\n", eid, buf.c_str());
  
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

  std::map<extent_protocol::extentid_t, _attr>::iterator ait;
  ait = file_attr_cache.find(eid);
  if(ait != file_attr_cache.end())
      file_attr_cache.erase(ait);

  return ret;
}

void
extent_client::flush(extent_protocol::extentid_t eid)
{
    std::map<extent_protocol::extentid_t, file>::iterator it;
    it = file_cache.find(eid);
    if(it != file_cache.end()) {
        int r;
        if(it->second.dirty)
            cl->call(extent_protocol::put, eid, it->second.content, r);
        printf("flush out:\n");
        printf("%llu\n", eid);
        printf("%s\n", it->second.content.c_str());
        file_cache.erase(it);
        file_attr_cache.erase(eid);
    } else {
        printf("ERROR try to flush a cache that doesn't exist\n");
    }
}
