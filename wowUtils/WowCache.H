#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include "WowLogger.H"

#include <mutex>
#include <thread>
#include <optional>

class WowCacheManager
{
public:
   struct FileMeta {
    std::string path;
    bool isDirty = false;
  };

  bool validate( std::string path, int32_t fh ) { return true; /*@TODO*/ }
  bool readFile( int32_t fd, std::string& buf ); 
  bool saveToCache( std::string path, const std::string& buf ); 

  void registerFileClose( int32_t fd );
  void registerFileOpen( int32_t fd, std::string filepath );
  void registerFileDirty( int32_t fd );
  
  bool isFileOpen( std::string filepath );
  bool isFileDirty( int32_t fd );
  std::string getFilePathFromFd( int32_t fd );
  std::optional<FileMeta> getFileMetaFromFd( int32_t fd );

  void deleteFromCache( std::string path );
  void rename( std::string oldPath, std::string newPath );
  bool constructDirPath( std::string path );
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

  std::map<int32_t, FileMeta> fdToMeta_;
  std::map<std::string, int32_t> filepathToFreq_;
};

inline void WowCacheManager::registerFileDirty( int32_t fd )
{
  const std::lock_guard<std::mutex> lock( mapMutex_ );
  if ( fdToMeta_.find( fd ) == fdToMeta_.end() ) {
    LogError( "CacheManager can't register dirty, unknown fd=" + std::to_string( fd )
              + ". Note this looks like a serious issue!" );
  } else {
    if ( ! fdToMeta_[ fd ].isDirty ) {
      LogInfo( "CacheManager registered file dirty for fd=" + std::to_string( fd ) );
    }
    fdToMeta_[ fd ].isDirty = true;
  }
}

inline void WowCacheManager::registerFileClose( int32_t fd )
{
  const std::lock_guard<std::mutex> lock( mapMutex_ );
  if ( fdToMeta_.find( fd ) == fdToMeta_.end() ) {
    LogWarn( "CacheManager can't register close, unknown fd=" + std::to_string( fd ) );
    return;
  }
  auto path = fdToMeta_[ fd ].path;
  fdToMeta_.erase( fd );
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
  fdToMeta_[ fd ] = FileMeta { .path = filepath, .isDirty = false } ;
  filepathToFreq_[ filepath ]++;
  LogInfo( "CacheManager registered file open for fd=" + std::to_string( fd ) + " path=" + filepath );
}

inline bool WowCacheManager::isFileOpen( std::string filepath )
{
  const std::lock_guard<std::mutex> lock( mapMutex_ );
  return filepathToFreq_.find( filepath ) != filepathToFreq_.end();
}

inline std::optional<WowCacheManager::FileMeta>
WowCacheManager::getFileMetaFromFd( int32_t fd )
{
  const std::lock_guard<std::mutex> lock( mapMutex_ );
  if ( fdToMeta_.find( fd ) == fdToMeta_.end() ) {
    LogWarn( "CacheManager can't find meta for unknown fd=" + std::to_string( fd ) );
    return {};
  }
  LogInfo( "CacheManager read mapping fd=" + std::to_string( fd ) + " <> file="
           + fdToMeta_[ fd ].path + " isDirty=" + std::to_string( (int)fdToMeta_[fd].isDirty ) );
  return { fdToMeta_[ fd ] };
}

inline std::string WowCacheManager::getFilePathFromFd( int32_t fd ) {
  auto meta = getFileMetaFromFd( fd );
  return meta.has_value() ? meta.value().path : "";
}

inline bool WowCacheManager::isFileDirty( int32_t fd )
{
  auto meta = getFileMetaFromFd( fd );
  return meta.has_value() ? meta.value().isDirty : false;
}  

inline bool WowCacheManager::constructDirPath( std::string path )
{
  if ( path.empty() || path[0] != '/' ) {
    return false;
  }
  // we are assuming path is a file path, so should not end in '/'
  std::string dirTree;
  for ( unsigned int i = 0; i < path.size(); ++i ) {
    if ( path[i] == '/' ) {
      dirTree = path.substr(0, i);
    }
  }
  std::filesystem::create_directories(dirTree);
  return true;
}

inline bool WowCacheManager::saveToCache( std::string path, const std::string& buf )
{
  if ( ! constructDirPath( path ) ) {
    return false;
  }

  FILE* fp = fopen( path.c_str(), "wb" );
  if ( fp != NULL ) {
    fwrite( buf.data(), 1, buf.size(), fp );
    fclose(fp);
    LogInfo( "CacheManager cached bytes=" + std::to_string( buf.size() ) + " for file=" + path );
    return true;
  }

  LogWarn( "CacheManager failed to cache bytes=" + std::to_string( buf.size() ) 
            + ". File=" + path + " won't be opened." );
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
    // If path is empty, this means that the file descriptor we are dealing with is
    // already closed.
    LogWarn( "CacheManager couldn't recognise unknown file descriptor fd=" + std::to_string(fd)
             + ". Is the fd already closed?" );
    return false; // disable writeback
  } else {
    readFd = open( path.c_str(), O_RDONLY );
    if ( readFd <= 0 ) {
      // If we can't open the file for reading at this point, then it might mean
      // the file has been deleted by another thread/process!
      LogWarn( "CacheManager couldn't open file for reading path=" + path );
      return false; // disable writeback
    }
  }

  lseek( readFd, 0, SEEK_SET );
#ifdef __APPLE__
  // Note: OSX does't have an lseek64, so we will need to stay on lseek.
  // This is somewhat dangerous as sizes exceeding 32-bit integers can
  // overflow. But I don't want to fix it for now.
  auto size = lseek( readFd, 0, SEEK_END );
#else
  off64_t size = lseek64( readFd, 0, SEEK_END );
#endif

  if ( size == -1 ) {
    // lseek has failed for some reason
    return false; // disable writeback
  }

  buf.resize(size);
  lseek( readFd, 0, SEEK_SET );
  int res = pread( readFd, buf.data(), size, 0 );
  close( readFd );
  if ( res < 0 ) {
    buf = "";
    LogWarn( "CacheManager couldn't read file for fd=" + std::to_string( fd )
             + " writeback to server will be disabled" );
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
