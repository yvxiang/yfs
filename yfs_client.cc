// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include "lock_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
  /*
  put(0x00000001, "");
  printf("success create root\n");
  */
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string
yfs_client::filename(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool
yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the file lock

  printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  printf("getfile %016llx -> sz %llu\n", inum, fin.size);

 release:

  return r;
}

int
yfs_client::setfile(inum inum, fileinfo file_info)
{
    int r = OK;

  printf("setfile %016llx\n", inum);
  printf("set size of %llu to %u\n", inum, file_info.size);
  extent_protocol::attr a;
  a.size = file_info.size;
  a.atime = file_info.atime;
  a.ctime = file_info.ctime;
  a.mtime = file_info.mtime;
  if(ec->setattr(inum, a) != extent_protocol::OK) {
      r = IOERR;
      goto release;
  }
  return OK;

 release:

  return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the directory lock

  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  return r;
}

int 
yfs_client::get(inum file_num, std::string &file_con)
{
    //printf("want to get file in yfs_client\n");
    int ret = ec->get(file_num, file_con);
    //printf("yfs_client get file returns %d\n");
//    return ec->get(file_num, file_con);
    return ret;
}

int
yfs_client::put(inum file_num, std::string file_con)
{
    return ec->put(file_num, file_con);
}

int 
yfs_client::create(inum parent_num, std::string new_file_name,
                           inum &new_file_inum, bool is_dir)
{
    if(!isdir(parent_num)) return IOERR;
    std::string dir_con;
    
    char mod = 'a';
    FILE *fp = fopen("/share/yc/log", &mod);
  
    int ret = get(parent_num, dir_con);
    if(ret != OK)  return ret;
    /*
    fprintf(fp, "old dic con\n");
    fprintf(fp, "------------\n");
    const char *old = dir_con.c_str();
    fprintf(fp, "%s\n", old);
    fprintf(fp, "------------\n");
    */
    std::string::iterator end = dir_con.begin();
    std::string cur_file_name;
    std::string cur_file_con;
    while(end != dir_con.end()) {
        while(end != dir_con.end() && *end != ' ') {
            cur_file_name += *end;
            end++;
        }
        end++;
        if(cur_file_name == new_file_name) {
            printf("file already exists!!!\n");
            return EXIST;
        }
        while(end != dir_con.end() && *end != ' ')
            end++;
        if(end == dir_con.end())
            break;
        end++;
    }
    //const char *n = new_file_name.c_str();
    //printf("has check the dir, we are allowed to create file %s\n", n); 

    // now pick up a inum for the new file(dic)
//    srandom(getpid());
   // int tmp_num;
    if(is_dir) {
        new_file_inum = rand() & 0x7FFFFFFF;
    } else {
        new_file_inum = (rand() & 0xFFFFFFFF) | (1 << 31);
    }
    //fprintf(fp, "new file num:!!!!!!! %llu\n", new_file_inum);
    if(dir_con.size() != 0) {
        new_file_name.insert(0, 1, ' ');
    }
    new_file_name += " " + filename(new_file_inum);
    dir_con += new_file_name; 
    ret = put(parent_num, dir_con);
   // fprintf(fp, "we have update the dic %lld\n", parent_num);
    /*
    const char *c_dic = dir_con.c_str();
    fprintf(fp, "\n\n");
    fprintf(fp, "new dic\n");
    fprintf(fp, "--------\n");
    fprintf(fp, "%s\n", c_dic);
    fprintf(fp, "--------\n");
    */
    if(ret != OK)   return ret;

    ret = put(new_file_inum, cur_file_con);
    //fprintf(fp, "we have touch the new file %s, %d\n", n, new_file_inum);

    fclose(fp);
    return OK;

}

bool
yfs_client::lookup(inum parent_num, std::string file_name, inum &file_num)
{
    if(!isdir(parent_num)) return false;

    std::string dir_con;
    int ret = get(parent_num, dir_con);
    if(ret != yfs_client::OK)   return ret;

    std::string::iterator end = dir_con.begin();
    std::string cur_file_name = "";
    //printf("look up file begin\n");
    while(end != dir_con.end()) {
        cur_file_name = "";
        while(end != dir_con.end() && *end != ' ') {
            cur_file_name += *end;
            end++;
        }
        if(end == dir_con.end())
            return false;
        end++;
        inum cur_file_num = 0;
        while(end != dir_con.end() && *end != ' ') {
            cur_file_num = cur_file_num * 10 + *end - '0';
            end++;
        }
        /*
        const char *cn = cur_file_name.c_str();
        printf("%s\n", cn);
        */
        if(file_name == cur_file_name) {
            file_num = cur_file_num;
            return true;
        }
        if(end == dir_con.end()) {
            //printf("couldn't find\n");
            return false;
        }
        end++;
    }
    return false;
}
    
int
yfs_client::read(inum inum, std::string &buf, size_t &size,
                                                off_t off)
{
    int ret = get(inum, buf);
    if(ret != OK)   return ret;
    if(off >= buf.size()) {
        buf = "";
    } else {
        buf = buf.substr(off, size);
    }
    size = buf.size();
    return OK;
}

int
yfs_client::write(inum inum, std::string str, off_t off,
                                        size_t &size)
{
    if(!isfile(inum))   return IOERR;
    std::string file_con; 
    int ret = get(inum, file_con);
    if(ret != OK)   return ret;
    if(off >= file_con.size())
        file_con.resize(off);
    //printf("in yfs_client::write old %s\n", file_con.c_str());
    file_con.replace(off, str.size(), str);
    //printf("in yfs_client::write new %s\n", file_con.c_str());
    size = str.size();
    ret = put(inum, file_con);
    return ret;
  return ret;   
}





