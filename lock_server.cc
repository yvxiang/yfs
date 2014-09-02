// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
    pthread_mutex_init(&operation_lock, NULL);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %lld\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
    /*
    char mode = 'a';
    FILE *fp = fopen("/share/yc/log", &mode); 
    std::map<lock_protocol::lockid_t, lock_info>::iterator it;
    fprintf(fp, "\n");
    fprintf(fp, "--------------------------------\n");
    fprintf(fp, "%d try to acquire lock %lld\n", clt, lid);
    fprintf(fp, "now all the locks's stat is:\n");
    fprintf(fp, "LOCK ID\tSTAT\tHOLDER\n");
    for(it = lock_dic.begin(); it != lock_dic.end(); it++) {
        fprintf(fp, "%lld %d %d\n", it->first, (it->second).cur_stat,
                (it->second).holder_id);
    }
    fprintf(fp, "--------------------------------\n");
    fprintf(fp, "\n");
    fclose(fp);
    */
    std::map<lock_protocol::lockid_t, lock_info>::iterator it;
__retry:
    pthread_mutex_lock(&operation_lock);
    if((it = lock_dic.find(lid)) == lock_dic.end()) { // build a new lock
        lock_info new_lock;
        new_lock.cur_stat = BUSY;
        new_lock.holder_id = clt;
        lock_dic.insert(std::make_pair<lock_protocol::lockid_t,
                lock_info>(lid, new_lock));
        r = 0;
  //     printf("%d acquire a new lock, lock id is %lld\n", clt, lid);
        pthread_mutex_unlock(&operation_lock);
        return lock_protocol::OK;
    } else {
        if((it->second).cur_stat == BUSY) {
            pthread_mutex_unlock(&operation_lock);
            goto __retry;
            r = -1;
   //         printf("%d try to acquire a granted lock, lock id is %lld\n",
   //                        clt, lid);
            pthread_mutex_unlock(&operation_lock);
            return lock_protocol::RETRY;
        } else {
            // how to handle the currence
            (it->second).cur_stat = BUSY;
            (it->second).holder_id = clt;
  //          printf("%d acquire a lock, lock id is %lld\n", clt, lid);
            pthread_mutex_unlock(&operation_lock);
            r = 0;
            return lock_protocol::OK;
        }
    }
}
lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
    std::map<lock_protocol::lockid_t, lock_info>:: iterator it;
    pthread_mutex_lock(&operation_lock);
    if((it = lock_dic.find(lid)) == lock_dic.end()) {
        r = -1;
//        printf("%d try to release a lock %lld that doesn't exist\n", clt, lid);
        pthread_mutex_unlock(&operation_lock);
        return lock_protocol::NOENT;
    } else {
        (it->second).cur_stat = FREE;
        (it->second).holder_id = -1;
        r = 0;
 //       printf("%d release a lock, lock id is %lld\n", clt, lid);
        pthread_mutex_unlock(&operation_lock);
        return lock_protocol::OK;
    }
}






