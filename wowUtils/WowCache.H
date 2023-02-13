#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>

class WowCacheManager
{
public:
  bool validate( std::string path, int32_t fh ) { return true; /*@TODO*/ }
  bool isDirty( std::string path, int32_t fh ) { return true; /*@TODO*/ }
  std::string& readFile( int32_t fd ); 
  bool saveToCache( std::string path, const std::string& buf ); 
private:
  std::string buf;
};

inline bool WowCacheManager::saveToCache( std::string path, const std::string& buf )
{
  if ( path.empty() || path[0] != '/' ) {
    return false; // something is very wrong, we only expect absolute paths
  }
  // we are assuming path is a file path, so should not end in '/'
  std::string dirTree;
  for ( int i = 0; i < path.size(); ++i ) {
    if ( path[i] == '/' ) {
      dirTree = path.substr(0, i);
    }
  }
  std::filesystem::create_directories(dirTree);

  FILE* fp = fopen( path.c_str(), "wb" );
  if ( fp != NULL ) {
    fwrite( buf.data(), 1, buf.size(), fp );
    fclose(fp);
    return true;
  }  
  return false;
}

inline std::string& WowCacheManager::readFile( int32_t fd ) {
  lseek( fd, 0, SEEK_SET );
  int size = lseek( fd, 0, SEEK_END );
  buf.resize(size);
  lseek( fd, 0, SEEK_SET );
  int res = pread( fd, buf.data(), size, 0 );
  if ( res < 0 ) buf = ""; // failed read
  return buf;
}
