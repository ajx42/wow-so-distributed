#ifndef UNRELIABLEFS_HH
#define UNRELIABLEFS_HH

#include <limits.h> /* PATH_MAX */
#include <pthread.h>

#include <grpcpp/grpcpp.h>
#include "fs.grpc.pb.h"
#include "WowRPCClient.H"

#define DEFAULT_CONF_NAME "unreliablefs.conf"

typedef struct unreliablefs_config {
     struct err_inj_q *errors;
     char             *basedir;
     char             *config_path;
     unsigned int      seed;
     unsigned int      debug;
     pthread_mutex_t   mutex;
} unreliablefs_config;

class WowManager
{
public:
  static WowManager& Instance()
  {
    static WowManager mgr;
    return mgr;
  }
  
  WowRPCClient client;

private:
  // singleton
  WowManager() : client(grpc::CreateChannel( 
    "localhost:50051", grpc::InsecureChannelCredentials() )) {}
};

#endif /* UNRELIABLEFS_HH */
