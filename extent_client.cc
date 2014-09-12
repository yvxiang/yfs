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
      it->second.file_attr.atime = time(NULL);
  } else {
      ret = cl->call(extent_protocol::get, eid, buf);
      file_cache.insert(std::make_pair(eid, file(buf)));
      it = file_cache.find(eid);
      it->second.file_attr.atime = time(NULL);
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
  
  
  std::map<extent_protocol::extentid_t, file>::iterator it;
  it = file_cache.find(eid);
  if(it != file_cache.end()) {
      attr.size = it->second.file_attr.size;
      attr.atime = it->second.file_attr.atime;
      attr.ctime = it->second.file_attr.ctime;
      attr.mtime = it->second.file_attr.mtime;
  } else {
      printf("fuck it is not in cache %u\n, eid");
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
    

    std::map<extent_protocol::extentid_t, file>::iterator it;
    it = file_cache.find(eid);
    printf("call setattr\n");
    if(it != file_cache.end()) {
        it->second.file_attr.size = attr.size;
        it->second.file_attr.atime = attr.atime;
        it->second.file_attr.ctime = attr.ctime;
        it->second.file_attr.mtime = attr.mtime;
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
  /*
  if(it != file_cache.end()) {
      //printf("put to cache %u\n", eid);
      //printf("cache %u old size %d\n", eid, it->second.content.size());
      it->second.content = buf;
      //printf("cache %u new size %d\n", eid, it->second.content.size());
      it->second.dirty = true;
      it->second.file_attr.mtime = it->second.file_attr.ctime
                                    = time(NULL);
      it->second.file_attr.size = it->second.content.size();
  } else {
      //printf("create a new file or dic %u\n", eid);
      file_cache.insert(std::make_pair(eid, file(buf)));
      it = file_cache.find(eid);
      it->second.dirty = true;
      it->second.content = buf;
      it->second.file_attr.mtime = it->second.file_attr.atime
                        = it->second.file_attr.ctime = time(NULL);
      it->second.file_attr.size = it->second.content.size();
  }
  */
  if(it == file_cache.end()) {
      file_cache.insert(std::make_pair(eid, file(buf)));
      it = file_cache.find(eid);
  }
  it->second.dirty = true;
  it->second.content = buf;
  it->second.file_attr.mtime = it->second.file_attr.atime
                        = it->second.file_attr.ctime = time(NULL);
  it->second.file_attr.size = it->second.content.size();
  
 
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


