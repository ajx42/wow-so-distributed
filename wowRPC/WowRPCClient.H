#pragma once

#include <iostream>
#include <memory>

#include <grpcpp/channel.h>
#include "fs.grpc.pb.h"
#include <sys/stat.h>

struct RPCResponse {
  int32_t ret_;
  int32_t server_errno_;
  RPCResponse(int32_t ret, int32_t server_errno) 
    : ret_(ret), server_errno_(server_errno) {}
};

class WowRPCClient
{
public:
  WowRPCClient( std::shared_ptr<grpc::Channel> channel )
    : stub_( wowfs::WowFS::NewStub(channel) ) {}

  int32_t Ping( int32_t cmd );

  RPCResponse GetXAttr(const std::string& file_path, 
    const std::string& name, char * value, const size_t size);
  RPCResponse Access(const std::string& file_path, mode_t mode);
  RPCResponse DownloadStat(const std::string& file_name, struct stat* buf);
  RPCResponse Mkdir(const std::string& dir_name, mode_t mode);
  RPCResponse Rmdir(const std::string& dir_name);
  RPCResponse Open(const std::string& file_name, int flags);
  RPCResponse Create(const std::string& file_name, mode_t mode, int flags);
  RPCResponse Writeback( const std::string& path, const std::string& buf );
  RPCResponse DownloadFile( const std::string& path, std::string& buf, size_t size ); 
  RPCResponse DownloadDir( const std::string& path, std::string& buf );

  RPCResponse Unlink( const std::string& path );
private:
  std::unique_ptr<wowfs::WowFS::Stub> stub_;
};

