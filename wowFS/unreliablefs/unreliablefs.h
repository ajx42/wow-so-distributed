#ifndef UNRELIABLEFS_HH
#define UNRELIABLEFS_HH

#include <limits.h> /* PATH_MAX */
#include <pthread.h>

#include <grpcpp/grpcpp.h>
#include "fs.grpc.pb.h"
#include "WowRPCClient.H"
#include "WowCache.H"

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

  enum StatSelect {
    NONE, LOCAL, REMOTE
  };

  struct StatInfo {
    struct stat statData;
    bool remotePresent = false;
    StatSelect source = NONE;
    int32_t errorCode = 0;
  };
  
  WowRPCClient client;
  WowCacheManager cmgr;
  
  std::string removeMountPrefix(std::string file_path) ;
  StatInfo shouldFetch( std::string path );
  bool writebackToServer( std::string path, int fd );
private:
  // singleton
  WowManager() : client(grpc::CreateChannel( 
    "localhost:50051", grpc::InsecureChannelCredentials() )) {}

};

inline bool WowManager::writebackToServer( std::string path, int fd )
{
  if ( ! cmgr.isFileDirty( fd ) ) {
    return true; // file is not dirty, so nothing to do.
  }

  auto serverPath = removeMountPrefix( path );
  std::string readBuf;
  auto readStatus = cmgr.readFile( fd, readBuf );

  if ( readStatus ) {
    auto res = client.Writeback( serverPath, readBuf );
    if ( res.ret_ == -1 ) {
      LogWarn("write back failed to server, local writes will be lost errno="
              + std::to_string(res.server_errno_));
      return false;
    }
  } else {
    LogInfo( "write back disabled for path=" + std::string( path ) );
  }
  return true;
}

inline std::string WowManager::removeMountPrefix(std::string file_path)
{
    const std::string prefix = "/tmp/wowfs_local/";
    if(file_path.find(prefix) == std::string::npos)
        return file_path;
    return file_path.substr(prefix.length());
}

inline WowManager::StatInfo WowManager::shouldFetch( std::string path )
{
  struct stat remoteStat, localStat;
  auto fsPath = removeMountPrefix( path );

  // fetch stat from server (remote), and store any errors that we see
  auto remoteResponse = client.DownloadStat( fsPath, &remoteStat );
  auto remoteSuccess = remoteResponse.ret_ != -1;
  auto remoteError = remoteResponse.server_errno_;

  // get the local stat and record any errors
  auto localSuccess = stat( path.c_str(), &localStat ) >= 0;
  auto localError = localSuccess ? 0 : errno;

  if ( ! localSuccess && ! remoteSuccess ) {
    // this looks e a new file, no need to fetch, we should indicate that
    // remote is not present, so that we can create file on the server on
    // the receiving end
    return StatInfo { .statData = {}, .remotePresent = false, .source = NONE,
      .errorCode = remoteError }; 
  } else if ( localSuccess && ! remoteSuccess ) {
    return StatInfo { .statData = localStat,  .remotePresent = false,
      .source = LOCAL, .errorCode = localError };
  } else if ( ! localSuccess && remoteSuccess ) {
    // file is not present locally, but is on remote
    return StatInfo { .statData = remoteStat, .remotePresent = true,
      .source = REMOTE, .errorCode = remoteError };
  }

  // at this point, we know that both stats are available, so we need to compare
#ifdef __APPLE__
  auto localModTime  = (int64_t)localStat.st_mtimespec.tv_sec * 1e9
                     + localStat.st_mtimespec.tv_nsec;
  auto remoteModTime = (int64_t)remoteStat.st_mtimespec.tv_sec * 1e9
                     + remoteStat.st_mtimespec.tv_nsec;
#else
  auto localModTime  = (int64_t)localStat.st_mtim.tv_sec * 1e9
                     + localStat.st_mtim.tv_nsec;
  auto remoteModTime = (int64_t)remoteStat.st_mtim.tv_sec * 1e9
                     + remoteStat.st_mtim.tv_nsec;
#endif

  auto localWins = localModTime >= remoteModTime;

  return StatInfo {
    .statData = (localWins ? localStat : remoteStat),
    .remotePresent = true,
    .source = (localWins ? LOCAL : REMOTE),
    .errorCode = (localWins ? localError : remoteError)
  };
}
     
#endif /* UNRELIABLEFS_HH */

