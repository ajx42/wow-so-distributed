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

class FHManager
{
public:
  static FHManager& Instance()
  {
    static FHManager fhmanager;
    return fhmanager;
  }
  // Insert mapping, return error if already exists.
  int32_t insertFHMapping(int32_t fh, std::string path)
  {
    auto it = _lookupMapping(fh);

    if (it == fh_mapping.end())
    {
      fh_mapping.emplace_back(fh, path);
      return 0;
    }
    else
    {
      return -1;
    }
  }
  // Remove mapping, return error if fh not present.
  int32_t removeFHMapping(int32_t fh)
  {
    auto it = _lookupMapping(fh);

    if (it != fh_mapping.end())
    {
      fh_mapping.erase(it);
      return 0;
    }
    else
    {
      return -1;
    }
  }

  // Update existing mapping, return error if fh not present.
  int32_t updateFHMapping(int32_t fh, std::string path)
  {
    auto it = _lookupMapping(fh);

    if (it != fh_mapping.end())
    {
      it->second = path;
      return 0;
    }
    else
    {
      return -1;
    }
  }

  std::string lookupPath(int32_t fh)
  {
    return _lookupMapping(fh)->second;
  }

private:
  std::vector<std::pair<int32_t, std::string>> fh_mapping;
  std::vector<std::pair<int32_t, std::string>>::iterator _lookupMapping(int32_t fh)
  {
    return std::find_if(fh_mapping.begin(), fh_mapping.end(),
                        [fh](const std::pair<int32_t, std::string> &p)
                        {
                          return p.first == fh;
                        });
  }
};

#endif /* UNRELIABLEFS_HH */
