#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>

#include "fs.grpc.pb.h"
#include "WowRPCClient.H"

int main( int argc, char* argv[] )
{
  std::cout << "hello world" << std::endl;
  std::string target_str = "localhost:50051";
  WowRPCClient client(
    grpc::CreateChannel( target_str, grpc::InsecureChannelCredentials() ) );
  auto reply = client.Ping(132);
  std::cout << "The reply is: " << reply << std::endl;

  auto sz = std::string( argv[1] );
  std::cout << sz << std::endl;  

  client.PerformSpeedTest( 12, std::stoi(sz) );

  return 0;
}
