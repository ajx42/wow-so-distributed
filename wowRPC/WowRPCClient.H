#pragma once

#include <iostream>
#include <memory>

#include <grpcpp/channel.h>
#include "fs.grpc.pb.h"
#include <sys/stat.h>

class WowRPCClient
{
public:
  WowRPCClient( std::shared_ptr<grpc::Channel> channel )
    : stub_( wowfs::WowFS::NewStub(channel) ) {}

  int32_t Ping( int32_t cmd );

  int32_t DownloadStat(const std::string& file_name, struct stat* buf, int* errno_);
//#ifdef HAVE_XATTR
  int32_t GetXAttr(const std::string& file_path, const std::string& name, char * value, const size_t size, int* errno_);
//#endif
  int32_t Access(const std::string& file_path, mode_t mode, int* errno_);
  int32_t Mkdir(const std::string& dir_name, mode_t mode, int* errno_);

private:
  std::unique_ptr<wowfs::WowFS::Stub> stub_;
};

