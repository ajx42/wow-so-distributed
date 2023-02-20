#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include "WowLogger.H"

#include <mutex>
#include <thread>

class WowCacheManager
{
public:
  bool validate( std::string path, int32_t fh ) { return true; /*@TODO*/ }
  bool isDirty( std::string path, int32_t fh ) { return true; /*@TODO*/ }
  bool readFile( int32_t fd, std::string& buf ); 
  bool saveToCache( std::string path, const std::string& buf ); 

  void registerFileClose( int32_t fd );
  void registerFileOpen( int32_t fd, std::string filepath );
  bool isFileOpen( std::string filepath );
  std::string getFilePathFromFd( int32_t fd );
  void deleteFromCache( std::string path );
  void rename( std::string oldPath, std::string newPath );
private:
  std::mutex mapMutex_;
  // The need for this mapping: we need to map fd <> path to perform file
  // reads before closing. A file may be open in modes that don't allow
  // reads, so we can't just use the fd at hand while closing. The idea is
  // to figure out the file path and create a read fd for it. The strategy
  // is further illustrated in `readFile` method.
  // Note the below maps only include entries corresponding to currently
  // open files to avoid potential disasters of reading some other file
  // and sync-ing that with the server.
  // @TODO: this can be optimised with TRIEs
  std::map<int32_t, std::string> fdToFilepath_;
  std::map<std::string, int32_t> filepathToFreq_;
};

inline void WowCacheManager::registerFileClose( int32_t fd )
{
  const std::lock_guard<std::mutex> lock( mapMutex_ );
  if ( fdToFilepath_.find( fd ) == fdToFilepath_.end() ) {
    LogWarn( "CacheManager can't register close, unknown fd=" + std::to_string( fd ) );
    return;
  }
  auto path = fdToFilepath_[ fd ];
  fdToFilepath_.erase( fd );
  auto newFreq = --filepathToFreq_[ path ];
  if ( ! newFreq ) {
    filepathToFreq_.erase( path );
  }
  LogInfo( "CacheManager registered file close for fd=" + std::to_string( fd ) + " path=" + path );
}

inline void WowCacheManager::registerFileOpen( int32_t fd, std::string filepath )
{
  const std::lock_guard<std::mutex> lock( mapMutex_ );
  if ( filepathToFreq_.find( filepath ) == filepathToFreq_.end() ) {
    filepathToFreq_[ filepath ] = 0;
  }
  fdToFilepath_[ fd ] = filepath;
  filepathToFreq_[ filepath ]++;
  LogInfo( "CacheManager registered file open for fd=" + std::to_string( fd ) + " path=" + filepath );
}

inline bool WowCacheManager::isFileOpen( std::string filepath )
{
  const std::lock_guard<std::mutex> lock( mapMutex_ );
  return filepathToFreq_.find( filepath ) != filepathToFreq_.end();
}

inline std::string WowCacheManager::getFilePathFromFd( int32_t fd ) {
  const std::lock_guard<std::mutex> lock( mapMutex_ );
  if ( fdToFilepath_.find( fd ) == fdToFilepath_.end() ) {
    LogWarn( "CacheManager can't find path for unknown fd=" + std::to_string( fd ) );
    return "";
  }
  LogInfo( "CacheManager read mapping fd=" + std::to_string( fd ) + " <> file=" + fdToFilepath_[ fd ] );
  return fdToFilepath_[ fd ];
}

inline bool WowCacheManager::saveToCache( std::string path, const std::string& buf )
{
  if ( path.empty() || path[0] != '/' ) {
    return false; // something is very wrong, we only expect absolute paths
  }
  // we are assuming path is a file path, so should not end in '/'
  std::string dirTree;
  for ( unsigned int i = 0; i < path.size(); ++i ) {
    if ( path[i] == '/' ) {
      dirTree = path.substr(0, i);
    }
  }
  std::filesystem::create_directories(dirTree);

  FILE* fp = fopen( path.c_str(), "wb" );
  if ( fp != NULL ) {
    fwrite( buf.data(), 1, buf.size(), fp );
    fclose(fp);
    LogInfo( "CacheManager cached bytes=" + std::to_string( buf.size() ) + " for file=" + path );
    return true;
  }
  LogWarn( "CacheManager failed to cache bytes=" + std::to_string( buf.size() ) + ". File=" + path + " won't be opened." );
  return false;
}

inline bool WowCacheManager::readFile( int32_t fd, std::string& buf ) {
  buf.clear();
  fsync(fd);

  // a file may be open in write mode, so we can't read its contents
  // directly without a read fd 
  int readFd = 0;
  auto path = getFilePathFromFd( fd );
  if ( path.empty() ) {
    return false;
    readFd = fd; // we will try falling back to what we have
    // this may or may not work, if this fails, we will return an empty buffer
    // and will not perform a writeback to server
  } else {
    readFd = open( path.c_str(), O_RDONLY );
  }

  lseek( readFd, 0, SEEK_SET );
  int size = lseek( readFd, 0, SEEK_END );
  buf.resize(size);
  lseek( readFd, 0, SEEK_SET );
  int res = pread( readFd, buf.data(), size, 0 );
  close( readFd );
  if ( res < 0 ) {
    buf = "";
    LogWarn( "CacheManager couldn't read file for fd=" + std::to_string( fd ) + " writeback to server will be disabled" );
    return false;
  }
  return true;
}

inline void WowCacheManager::deleteFromCache( std::string path ) {
  if(std::filesystem::remove( path )) {
      LogInfo( "CacheManager deleted file=" + path );
  } else {
      LogWarn( "CacheManager failed to delete file=" + path );
  }
}

inline void WowCacheManager::rename( std::string oldPath, std::string newPath ) {
  std::filesystem::rename( oldPath, newPath );
}
