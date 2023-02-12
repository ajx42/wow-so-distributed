#pragma once

#include <iostream>
#include <memory>
#include <sys/stat.h>
//#ifdef HAVE_XATTR
#include <sys/xattr.h>
//#endif
#include <errno.h>

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
  
//#ifdef HAVE_XATTR
  grpc::Status GetXAttr(grpc::ServerContext*, const wowfs::GetXAttrRequest*,
          grpc::ServerWriter<wowfs::DownloadResponse>*);
//#endif

  grpc::Status Access(grpc::ServerContext*, const wowfs::AccessRequest*, wowfs::AccessResponse*);

  grpc::Status Mkdir(grpc::ServerContext*, const wowfs::MkdirRequest*, wowfs::MkdirResponse*);
  
};

grpc::Status WowFSServiceImpl::TestCall( 
    grpc::ServerContext*, const wowfs::Cmd* cmd, wowfs::Ack* ack )
{
  std::cout << "hello ... " << std::endl;
  ack->set_ok(cmd->sup());
  return grpc::Status::OK;
}

grpc::Status WowFSServiceImpl::DownloadStat(
    grpc::ServerContext* context, const wowfs::DownloadRequest* request, 
    grpc::ServerWriter<wowfs::DownloadResponse>* writer)
{
    //Run lstat operation
    struct stat buf;
    int ret = lstat(request->file_name().c_str(), &buf);
    
    //Send struct
    wowfs::DownloadResponse response;
    response.set_data(reinterpret_cast<char*>(&buf), sizeof(buf));
    response.set_ret(ret);
    response.set_server_errno(errno);
    writer->Write(response);

    return grpc::Status::OK;
}

//#ifdef HAVE_XATTR
grpc::Status WowFSServiceImpl::GetXAttr(grpc::ServerContext* context, const wowfs::GetXAttrRequest* request, 
          grpc::ServerWriter<wowfs::DownloadResponse>* writer)
{
    int res;
    char * buffer = (char*)malloc(request->size());

#ifdef __APPLE__
    res = getxattr(request->file_path().c_str(), request->name().c_str(), buffer, request->size(), 0, XATTR_NOFOLLOW);
#else
    res = getxattr(request->file_path().c_str(), request->name().c_str(), buffer, request->size());
#endif /* __APPLE__ */

    wowfs::DownloadResponse response;
    response.set_data(buffer, request->size());
    response.set_ret(res);
    response.set_server_errno(errno);
    writer->Write(response);

    free(buffer);

    return grpc::Status::OK; 
}
//#endif

grpc::Status WowFSServiceImpl::Access(grpc::ServerContext* context,
         const wowfs::AccessRequest* request, wowfs::AccessResponse* response)
{
    //Run access operation
    mode_t mode = request->mode();
    int res = access(request->file_path().c_str(), mode);
    response->set_ret(res);
    response->set_server_errno(errno);
    
    return grpc::Status::OK;
}

grpc::Status WowFSServiceImpl::Mkdir(
    grpc::ServerContext* context, const wowfs::MkdirRequest* request,
    wowfs::MkdirResponse* response)
{
    //Run mkdir operation
    mode_t mode = request->mode();
    int ret = mkdir(request->dir_name().c_str(), mode);
    response->set_ret(ret);
    response->set_server_errno(errno);
    
    return grpc::Status::OK;
}