#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>

#include "lock_protocol.h"
#include "lock_client.h"

class yfs_client {
  extent_client *ec;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

  struct file_list {
      std::string name;
      yfs_client::inum inum;
  };


 private:
  static std::string filename(inum);
  static inum n2i(std::string);
 public:

  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int setfile(inum, fileinfo);
  int getdir(inum, dirinfo &);


  int get(inum, std::string &);
  int put(inum, std::string );
  int create(inum, std::string, inum &, bool is_dir = false);
  bool lookup(inum, std::string, inum&);
  int read(inum, std::string&, size_t&, off_t);
  int write(inum, std::string, off_t, size_t&);
 
};

#endif 
