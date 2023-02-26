#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <signal.h>

int main( int argc, char** argv )
{
  int fd;
  std::string data;
  int ret;

  fd = open( "/tmp/wowfs/config.txt", O_WRONLY | O_TRUNC | O_CREAT );
  data = "very_imp_data";
  ret = write( fd, data.data(), data.size() );
  close(fd);

  fd = open( "/tmp/wowfs/checkpoint.txt", O_WRONLY | O_APPEND | O_CREAT );
  data = "very_imp_data has been generated";
  ret = write( fd, data.data(), data.size() );
  close( fd );


  fd = open( "/tmp/wowfs/checkpoint.txt", O_WRONLY | O_APPEND );
  data = "very_imp_computation has been done";
  ret = write( fd, data.data(), data.size() );
  close( fd );

  kill( std::stoi(argv[1]), SIGSEGV );  

}
