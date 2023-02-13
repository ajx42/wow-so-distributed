#pragma once

#include <iostream>
#include <fstream>

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
