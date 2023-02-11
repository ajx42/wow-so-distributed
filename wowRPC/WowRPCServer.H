#pragma once

#include <iostream>
#include <memory>
#include <sys/stat.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "fs.grpc.pb.h"

class WowFSServiceImpl final : public wowfs::WowFS::Service
{
  grpc::Status TestCall( grpc::ServerContext*, const wowfs::Cmd*, wowfs::Ack* );
  
  //Will implement for downloading file info
  //grpc::Status DownloadFile(grpc::ServerContext*, const wowfs::DownloadRequest*, 
          //grpc::ServerWriter<wowfs::DownloadResponse>*);
  grpc::Status DownloadStat(grpc::ServerContext*, const wowfs::DownloadRequest*, 
          grpc::ServerWriter<wowfs::DownloadResponse>*);
};

grpc::Status WowFSServiceImpl::TestCall( 
    grpc::ServerContext*, const wowfs::Cmd* cmd, wowfs::Ack* ack )
{
  std::cout << "hello ... " << std::endl;
  ack->set_ok(cmd->sup());
  return grpc::Status::OK;
}

grpc::Status WowFSServiceImpl::DownloadStat(grpc::ServerContext* context, const wowfs::DownloadRequest* request, 
          grpc::ServerWriter<wowfs::DownloadResponse>* writer)
{
    //Run lstat operation
    struct stat buf;
    if (lstat(request->file_name().c_str(), &buf) == -1) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "lstat: File not found.");
    }
    
    //Serialize struct
    std::cerr << "Sending stat, inode : " << buf.st_ino << "\n";
    char * buffer = (char*)malloc(sizeof(buf));
    memcpy(buffer, &buf, sizeof(buf));
    
    //Send struct
    wowfs::DownloadResponse response;
    response.set_data(buffer, sizeof(buf));
    writer->Write(response);
    
    free(buffer);

    return grpc::Status::OK;
}
