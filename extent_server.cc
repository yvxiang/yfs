// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rsm.h"
#include "rsm_state_transfer.h"

extent_server::extent_server(rsm *_rsms) : rsms(_rsms)
{ pthread_mutex_init(&operation_lock, NULL);
  file root_dic;
  root_dic.content = "";
  root_dic.file_attr.atime = root_dic.file_attr.ctime = 
      root_dic.file_attr.mtime = time(NULL);
  file_map.insert(std::make_pair<extent_protocol::extentid_t, file> (
                    0x00000001, root_dic));
  rsms->set_state_transfer(this);
}
extent_server::~extent_server()
{

}


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  // You fill this in for Lab 2.
  pthread_mutex_lock(&operation_lock);
  std::map<extent_protocol::extentid_t, file>::iterator file_it;

  if((file_it = file_map.find(id)) != file_map.end()) {
      //printf("in extent_server::put old %s\n", file_it->second.content.c_str());
      //printf("in extent_server::put new %s\n", buf.c_str());
      file_it->second.content = buf;
      file_it->second.file_attr.ctime = file_it->second.file_attr.mtime
                                        = time(NULL);
                        
      file_it->second.file_attr.size = buf.size();
      pthread_mutex_unlock(&operation_lock);
      return extent_protocol::OK;
  }
  file new_file;
  new_file.file_attr.atime = new_file.file_attr.ctime = 
                            new_file.file_attr.mtime = time(NULL);
  new_file.file_attr.size = buf.size();
  new_file.content = buf;
  file_map.insert(std::make_pair<extent_protocol::extentid_t, file>(id, new_file));
  pthread_mutex_unlock(&operation_lock);
  return extent_protocol::OK;
//  return extent_protocol::IOERR;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  // You fill this in for Lab 2.
  pthread_mutex_lock(&operation_lock);

  std::map<extent_protocol::extentid_t, file>::iterator file_it;
  if((file_it = file_map.find(id)) != file_map.end()) {
      buf = file_it->second.content;
      pthread_mutex_unlock(&operation_lock);
      return extent_protocol::OK;
  } else {
      pthread_mutex_unlock(&operation_lock);
      return extent_protocol::NOENT;
  }

  return extent_protocol::IOERR;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  // You fill this in for Lab 2.
  // You replace this with a real implementation. We send a phony response
  // for now because it's difficult to get FUSE to do anything (including
  // unmount) if getattr fails.
  /*
  a.size = 0;
  a.atime = 0;
  a.mtime = 0;
  a.ctime = 0;
  */
    pthread_mutex_lock(&operation_lock);
    std::map<extent_protocol::extentid_t, file>::iterator file_it;

    if((file_it = file_map.find(id)) != file_map.end()) {
        a.size = file_it->second.file_attr.size;
        a.atime = file_it->second.file_attr.atime;
        a.mtime = file_it->second.file_attr.mtime;
        a.ctime = file_it->second.file_attr.ctime;
        pthread_mutex_unlock(&operation_lock);
        return extent_protocol::OK;
    } else {
        pthread_mutex_unlock(&operation_lock);
        return extent_protocol::NOENT;
    }

}

int extent_server::setattr(extent_protocol::extentid_t id, 
                                       extent_protocol::attr &a)
{
    printf("extent_server::setattr set %llu size to %u\n", id, a.size);
    
    pthread_mutex_lock(&operation_lock); 
    std::map<extent_protocol::extentid_t, file>::iterator file_it;

    if((file_it = file_map.find(id)) != file_map.end()) {

        file_it->second.file_attr.atime = a.atime;
        file_it->second.file_attr.ctime = a.ctime;
        file_it->second.file_attr.mtime = a.mtime;
        if(a.size < file_it->second.file_attr.size) {
            //printf("set file %llu size from %u to %u\n", id, file_it->second.file_attr.size, a.size); 
            file_it->second.file_attr.size = a.size;
            file_it->second.content.resize(a.size);
        } else if(a.size > file_it->second.file_attr.size) {
            //printf("file is too small, old size is %u\n", file_it->second.content.size());
            file_it->second.file_attr.size = a.size;
            file_it->second.content.resize(a.size, '\0');
            //printf("file is larger, new size is %u\n", a.size);
            //printf("set file %llu size from %u to %u\n", id, file_it->second.file_attr.size, a.size); 
        }

        pthread_mutex_unlock(&operation_lock);
        return extent_protocol::OK;

    } else {

        pthread_mutex_unlock(&operation_lock);
        return extent_protocol::NOENT;

    }

}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  // You fill this in for Lab 2.
  pthread_mutex_lock(&operation_lock);
  std::map<extent_protocol::extentid_t, file>::iterator file_it;

  if((file_it = file_map.find(id)) != file_map.end()) {
      file_map.erase(file_it);
      //printf("success remove file %llu\n", id);
      pthread_mutex_unlock(&operation_lock);
      return extent_protocol::OK;
  } else {
      //printf("couldn't find the file %llu to remove\n", id);
      pthread_mutex_unlock(&operation_lock);
      return extent_protocol::NOENT;
  }

  return extent_protocol::IOERR;
}
std::string extent_server::marshal_state()
{
}
void extent_server::unmarshal_state(std::string state)
{
}
