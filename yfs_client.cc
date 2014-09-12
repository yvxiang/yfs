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


yfs_client::yfs_client(std::string extent_dst, std::string lock_dst):lc(lock_dst)
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
  lc.acquire(inum);
  //printf("yfs_client.cc getattr get lock %u\n", inum);
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  //printf("getfile %016llx -> sz %llu\n", inum, fin.size);

 release:
  //printf("yfs_client.cc getattr release lock %u\n", inum);
  lc.release(inum);
  return r;
}

int
yfs_client::setfile(inum inum, fileinfo file_info)
{
    int r = OK;
  lc.acquire(inum);
  printf("yfs_client setfile get lock %u\n", inum);
  printf("setfile %016llx\n", inum);
  //printf("set size of %llu to %u\n", inum, file_info.size);
  extent_protocol::attr a;
  a.size = file_info.size;
  a.atime = file_info.atime;
  a.ctime = file_info.ctime;
  a.mtime = file_info.mtime;
  if(ec->setattr(inum, a) != extent_protocol::OK) {
      r = IOERR;
      goto release;
  }
  lc.release(inum);
  //printf("yfs_client setfile release lock %u\n", inum);
  return OK;
  
 release:
  //printf("yfs_client setfile release lock %u\n", inum);
  lc.release(inum);
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
  lc.acquire(inum);
  //printf("yfs_client getdir get lock %u\n", inum);
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  //printf("yfs_client getdir release lock %u\n", inum);
  lc.release(inum);
  return r;
}

int 
yfs_client::get(inum file_num, std::string &file_con)
{
    //printf("want to get file in yfs_client\n");
    lc.acquire(file_num);
    //printf("yfs_client.cc get get lock %u\n", file_num);
    int ret = ec->get(file_num, file_con);
    //printf("yfs_client get file returns %d\n");
//    return ec->get(file_num, file_con);
    return ret;
}

int
yfs_client::put(inum file_num, std::string file_con)
{
  //  lc.acquire(file_num);
    int ret =  ec->put(file_num, file_con);
   // lc.release(file_num);
    return ret;

}

int 
yfs_client::create(inum parent_num, std::string new_file_name,
                           inum &new_file_inum, bool is_dir)
{
    if(!isdir(parent_num)) return IOERR;
    std::string dir_con;
    
    //char mod = 'a';
    //FILE *fp = fopen("/share/yc/log", &mod);
//    printf("%u want to create %s, want to get lock\n", getpid(), new_file_name.c_str());
    int ret = get(parent_num, dir_con);
 //   printf("%u want to create %s, has get lock\n", getpid(),  new_file_name.c_str());
    if(ret != OK) {
        //printf("yfs_client.cc create release lock %u\n");
        lc.release(parent_num);
        return ret;
    }
    /*
    fprintf(fp, "old dic con\n");
    fprintf(fp, "------------\n");
    const char *old = dir_con.c_str();
    fprintf(fp, "%s\n", old);
    fprintf(fp, "------------\n");
    */
//    printf("want to create %s\n", new_file_name.c_str());
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
        //    printf("file already exists!!!\n");
        //printf("yfs_client.cc create release lock %u\n");
            lc.release(parent_num);
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
    //srand(getpid());
    //srand(time(NULL));
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
    if(ret != OK) {
        //printf("yfs_client.cc create release lock %u\n");
        lc.release(parent_num);
        return ret;
    }
     
    ret = put(new_file_inum, cur_file_con);
   // lc.release(new_file_inum);
   // printf("%u want to create %s, has create, next to release lock\n", getpid(), new_file_name.c_str());
        //printf("yfs_client.cc create release lock %u\n");
    lc.release(parent_num);
   // printf("%u want to create %s, has release lock\n", getpid(), new_file_name.c_str());
    //fprintf(fp, "we have touch the new file %s, %d\n", n, new_file_inum);

    //fclose(fp);
    return OK;

}

bool
yfs_client::lookup(inum parent_num, std::string file_name, inum &file_num)
{
    if(!isdir(parent_num)) return false;

    std::string dir_con;
    int ret = get(parent_num, dir_con);
    if(ret != yfs_client::OK) {
        //printf("yfs_client lookup release lock %u\n", parent_num);
        lc.release(parent_num);
        return ret;
    }

    std::string::iterator end = dir_con.begin();
    std::string cur_file_name = "";
    //printf("look up file begin\n");
    while(end != dir_con.end()) {
        cur_file_name = "";
        while(end != dir_con.end() && *end != ' ') {
            cur_file_name += *end;
            end++;
        }
        if(end == dir_con.end()) {
            lc.release(parent_num);
            return false;
        }
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
        //printf("yfs_client lookup release lock %u\n", parent_num);
            lc.release(parent_num);
            return true;
        }
        if(end == dir_con.end()) {
            //printf("couldn't find\n");
        //printf("yfs_client lookup release lock %u\n", parent_num);
            lc.release(parent_num);
            return false;
        }
        end++;
    }
        //printf("yfs_client lookup release lock %u\n", parent_num);
    lc.release(parent_num);
    return false;
}
    
int
yfs_client::read(inum inum, std::string &buf, size_t &size,
                                                off_t off)
{
    int ret = get(inum, buf);
    //printf("yfs_client.cc read release lock %u\n", inum);
    lc.release(inum);
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

    if(ret != OK) { 
        //printf("yfs_client.cc write release lock %u\n");
        lc.release(inum);
        return ret;
    }
    printf("want to write %u, old size is %d\n", inum, size);
    if(off >= file_con.size())
        file_con.resize(off);
    //printf("in yfs_client::write old %s\n", file_con.c_str());
    file_con.replace(off, str.size(), str);
    //printf("in yfs_client::write new %s\n", file_con.c_str());
    size = str.size();
    ret = put(inum, file_con);
    printf("write to %u, new size = %d\n", inum, size);
        //printf("yfs_client.cc write release lock %u\n");
    lc.release(inum);
    return ret;
  return ret;   
}

int
yfs_client::mkdir(inum parent_inum, std::string file_name,
                                        inum &new_file_inum)
{
    int ret = create(parent_inum, file_name, new_file_inum, true);
    return ret;
}

int
yfs_client::remove(inum inum)
{
    lc.acquire(inum);
    //printf("yfs_client.cc remove acquire lock %u\n", inum);
    int ret = ec->remove(inum);
    lc.release(inum);
    //printf("yfs_client.cc remove release lock %u\n", inum);
    return ret;
}

int
yfs_client::unlink(inum parent_inum, std::string file_name)
{
    std::string dir_con;
    int ret = get(parent_inum, dir_con);
    if(ret != yfs_client::OK) {
        //printf("yfs_client.cc unlink release lock %u\n", parent_inum);
        lc.release(parent_inum);
        return ret;
    }

    //printf("dir con is :\n");
    //printf("%s\n", dir_con.c_str());
    //printf("to delete %s\n", file_name.c_str());
    size_t pos = dir_con.find(file_name, 0);

    if(pos == std::string::npos)  {
     //   printf("could not find the file!!!\n");
        lc.release(parent_inum);
        return yfs_client::NOENT;
    }

    size_t file_num_pos = pos + file_name.size() + 1;
    inum file_inum = 0;
    while(file_num_pos < dir_con.size() &&
                            dir_con[file_num_pos] != ' ') {
        file_inum = file_inum * 10 + 
                        dir_con[file_num_pos] - '0';
    //    printf("cur_file_inum %u\n", file_inum);
        file_num_pos++;
    }

    // judge the pos because maybe we are unlinking the first
    //or the middle or the last file int the dic, be carefully!!
    if(file_num_pos < dir_con.size()) {
        file_num_pos++;
    } else if(file_num_pos == dir_con.size()) {
        if(pos > 0)
            pos--;
    }
    
    dir_con.erase(pos, file_num_pos - pos);
    //printf("new dir is :\n");
    //printf("%s %d\n", dir_con.c_str(), dir_con.size());
    put(parent_inum, dir_con);
    //printf("yfs_client.cc unlink release lock %u\n", parent_inum);
    lc.release(parent_inum);
    //now begin to remove the actural file content
    ret = remove(file_inum);
    //printf("yfs_client::unlink remove %u returns %d\n", file_inum, ret);
    return ret;

}

void
yfs_client::release_lk(inum dir_inum)
{
    lc.release(dir_inum);
    //printf("yfs_client.cc release_lk release lock %u\n", dir_inum);
}


