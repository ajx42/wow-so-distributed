#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>

#include "fs.grpc.pb.h"
#include "WowRPCClient.H"

int main( int argc, char* argv[] )
{
  std::string target_str = "c220g5-110522.wisc.cloudlab.us:50051";
  WowRPCClient client(
    grpc::CreateChannel( target_str, grpc::InsecureChannelCredentials() ) );

  auto sz = std::string( argv[1] );

  client.PerformSpeedTest( 12 /*random id to debug on server*/, std::stoll(sz) );

  return 0;
}
