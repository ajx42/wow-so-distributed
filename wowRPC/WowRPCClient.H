#pragma once

#include <iostream>
#include <memory>

#include <grpcpp/channel.h>
#include "fs.grpc.pb.h"

class WowRPCClient
{
public:
  WowRPCClient( std::shared_ptr<grpc::Channel> channel )
    : stub_( wowfs::WowFS::NewStub(channel) ) {}

  int32_t Ping( int32_t cmd );

private:
  std::unique_ptr<wowfs::WowFS::Stub> stub_;
};

inline int32_t WowRPCClient::Ping( int32_t cmd )
{
  wowfs::Cmd msg; msg.set_sup( cmd );
  wowfs::Ack reply;

  grpc::ClientContext context;

  auto status = stub_->TestCall( &context, msg, &reply );
  if ( status.ok() ) {
    std::cerr << "Received OK" << std::endl;
    return reply.ok();
  } else {
    std::cerr << "RPC Failed" << std::endl;
    return -1;
  }
}
