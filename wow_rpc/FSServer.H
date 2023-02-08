#pragma once

#include <iostream>
#include <memory>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "build/fs.grpc.pb.h"

class WowFSServiceImpl final : public wowfs::WowFS::Service
{
  grpc::Status TestCall( grpc::ServerContext*, const wowfs::Cmd*, wowfs::Ack* );
};

grpc::Status WowFSServiceImpl::TestCall( 
    grpc::ServerContext*, const wowfs::Cmd* cmd, wowfs::Ack* ack )
{
  std::cout << "hello ... " << std::endl;
  ack->set_ok(cmd->sup());
  return grpc::Status::OK;
}
