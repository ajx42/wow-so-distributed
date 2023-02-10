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

