#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>

#include "fs.grpc.pb.h"
#include "WowRPCClient.H"

int main()
{
  std::cout << "hello world" << std::endl;
  std::string target_str = "localhost:50051";
  WowRPCClient client(
    grpc::CreateChannel( target_str, grpc::InsecureChannelCredentials() ) );
  auto reply = client.Ping(132);
  std::cout << "The reply is: " << reply << std::endl;
  return 0;
}
