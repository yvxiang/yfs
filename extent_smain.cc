#include <unistd.h>
#include <sys/types.h>
#include "rpc.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include "extent_server.h"

// Main loop of extent server

int
main(int argc, char *argv[])
{
  int count = 0;

  if(argc != 3){
    fprintf(stderr, "Usage: %s port\n", argv[0]);
    exit(1);
  }

  setvbuf(stdout, NULL, _IONBF, 0);

  char *count_env = getenv("RPC_COUNT");
  if(count_env != NULL){
    count = atoi(count_env);
  }

  rsm rsms(argv[1], argv[2]);
  extent_server ls(&rsms);

  //rsms.set_state_transfer((rsm_state_transfer)&ls);
  rsms.set_state_transfer(&ls);
  rsms.reg(extent_protocol::get, &ls, &extent_server::get);
  rsms.reg(extent_protocol::getattr, &ls, &extent_server::getattr);
  rsms.reg(extent_protocol::put, &ls, &extent_server::put);
  rsms.reg(extent_protocol::remove, &ls, &extent_server::remove);
  rsms.reg(extent_protocol::setattr, &ls, &extent_server::setattr);

  while(1)
    sleep(1000);
}
