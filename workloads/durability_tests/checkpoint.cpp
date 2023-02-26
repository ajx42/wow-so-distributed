#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <signal.h>

// this is the file we are trying to put in a state inconsistent with checkpoints
std::string WOWDB_FILE = "/tmp/wowfs/wow_db.txt";

// checkpoints file used by this sample application
std::string CHECKPOINTS_FILE = "/tmp/wowfs/wow_checkpoint.txt";
std::string CHECKPOINT_1_KEY = "checkpoint_1";

// this is the very important data that is being written to wow_db and which is
// checkpointed given how critical it is
std::string VERY_IMP_DATA = "very_imp_data=42";


bool checkpointOnePresent()
{
  if ( ! std::filesystem::exists(CHECKPOINTS_FILE) ) {
    return false;
  }
  std::string line;
  std::ifstream infile( CHECKPOINTS_FILE );
  while ( std::getline( infile, line ) ) {
    std::cout << line << std::endl;
    if ( line.find( "checkpoint_1" ) != std::string::npos ) return true;
  }
  return false;
}

int main( int argc, char** argv )
{
  int fd;
  std::string data;
  int ret;
  std::string readBuf;
  int very_imp_number;

  if ( checkpointOnePresent() ) {
    readBuf.resize(1024);
    fd = open( WOWDB_FILE.c_str(), O_RDONLY );
    ret = pread(fd, readBuf.data(), readBuf.size(), 0);
    readBuf[ret] = '\0';
    very_imp_number = std::stoi(readBuf.substr(readBuf.find('=') + 1));
  } else {
    fd = open( WOWDB_FILE.c_str(), O_WRONLY | O_TRUNC | O_CREAT );
    data = VERY_IMP_DATA;
    ret = write( fd, data.data(), data.size() );
    close(fd);

    fd = open( CHECKPOINTS_FILE.c_str(), O_WRONLY | O_APPEND | O_CREAT );
    data = CHECKPOINT_1_KEY + ": very important data has been written\n";
    ret = write( fd, data.data(), data.size() );
    close( fd ); 
    
    very_imp_number = 42;
  }

  int fib_now = 1, fib_prev = 0;
  for ( int i = 2; i <= very_imp_number; ++i ) {
    auto temp = fib_now;
    fib_now = fib_now + fib_prev;
    fib_prev = temp;
  }

  fd = open( CHECKPOINTS_FILE.c_str(), O_WRONLY | O_APPEND );
  data = "checkpoint 2: very_imp_computation has been done\n";
  ret = write( fd, data.data(), data.size() );
  close( fd );

  std::cout << "result of very important computation on very important number is: " << fib_now << std::endl;
  
  if ( argc > 1 )
    kill( std::stoi(argv[1]), SIGSEGV );  

}
