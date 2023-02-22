#pragma once

#include <iostream>
#include <memory>
#include <sys/stat.h>
#ifdef HAVE_XATTR
#include <sys/xattr.h>
#endif
#include <errno.h>
#include <sys/file.h>

#include <dirent.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "fs.grpc.pb.h"

class WowFSServiceImpl final : public wowfs::WowFS::Service
{
private:
  std::string basedir_;
public:
  explicit WowFSServiceImpl(const std::string& basedir) {
    // if basedir does not end with a /, add one
    if (basedir.back() != '/') {
      this->basedir_ = basedir + "/";
    } else {
      this->basedir_ = basedir;
    }
  }
  grpc::Status TestCall( grpc::ServerContext*, const wowfs::Cmd*, wowfs::Ack* );
  
  //Will implement for downloading file info
  //grpc::Status DownloadFile(grpc::ServerContext*, const wowfs::DownloadRequest*, 
          //grpc::ServerWriter<wowfs::DownloadResponse>*);
  grpc::Status DownloadStat(grpc::ServerContext*, const wowfs::DownloadRequest*, 
          grpc::ServerWriter<wowfs::DownloadResponse>*);
  
#ifdef HAVE_XATTR
  grpc::Status GetXAttr(grpc::ServerContext*, const wowfs::GetXAttrRequest*,
          grpc::ServerWriter<wowfs::DownloadResponse>*);
#endif

  grpc::Status Access(grpc::ServerContext*, const wowfs::AccessRequest*, wowfs::AccessResponse*);

  grpc::Status Mkdir(grpc::ServerContext*, const wowfs::MkdirRequest*, 
                    wowfs::MkdirResponse*);
  grpc::Status Rmdir(grpc::ServerContext*, const wowfs::RmdirRequest*, 
                    wowfs::RmdirResponse*);
  grpc::Status Open(grpc::ServerContext*, const wowfs::OpenRequest*, 
                    wowfs::OpenResponse*);
  grpc::Status Create(grpc::ServerContext*, const wowfs::CreateRequest*, 
                    wowfs::CreateResponse*);
  
  grpc::Status Writeback(grpc::ServerContext*, grpc::ServerReader<wowfs::StreamWriteRequest>*,
    wowfs::StreamWriteResponse*);

  grpc::Status ReadFile(grpc::ServerContext*, const wowfs::StreamReadRequest*,
      grpc::ServerWriter<wowfs::StreamReadResponse>*);

  grpc::Status ReadDir(grpc::ServerContext*, const wowfs::StreamReadRequest*,
      grpc::ServerWriter<wowfs::StreamReadResponse>*);
    
  grpc::Status Unlink(grpc::ServerContext*, const wowfs::UnlinkRequest*, 
                      wowfs::UnlinkResponse*);
  grpc::Status Rename(grpc::ServerContext*, const wowfs::RenameRequest*, 
                      wowfs::RenameResponse*);
  grpc::Status Link(grpc::ServerContext*, const wowfs::LinkRequest*, 
                    wowfs::LinkResponse*);
  grpc::Status Symlink(grpc::ServerContext*, const wowfs::SymlinkRequest*, 
                       wowfs::SymlinkResponse*);
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
    int ret = lstat((this->basedir_ + request->file_name()).c_str(), &buf);
    
    //Send struct
    wowfs::DownloadResponse response;
    response.set_data(reinterpret_cast<char*>(&buf), sizeof(buf));
    response.set_ret(ret);
    response.set_server_errno(errno);
    writer->Write(response);

    return grpc::Status::OK;
}

grpc::Status WowFSServiceImpl::ReadFile(
    grpc::ServerContext* context, const wowfs::StreamReadRequest* request,
    grpc::ServerWriter<wowfs::StreamReadResponse>* writer )
{
  wowfs::StreamReadResponse response;
  // open file for reading
  auto fd = open( (this->basedir_ + request->path()).c_str(), O_RDONLY );
  if ( fd == -1 ) {
    // we couldn't open the file, return error
    response.set_ret( -1 );
    response.set_server_errno( errno );
    writer->Write( response );
    return grpc::Status::OK;
  }

  int bytesToRead = request->size();

  std::string buf;
  buf.resize( bytesToRead + 100 ); // 
  auto ret = pread( fd, buf.data(), bytesToRead, 0 ); // read from beginning
  close( fd );
  if ( ret < 0 ) {
    // we couldn't read
    response.set_ret( -1 );
    response.set_server_errno( errno );
    writer->Write( response );
    return grpc::Status::OK;
  }

  std::cerr << "Read bytes: " << ret << " and expected " << bytesToRead << std::endl;
  int remainingSize = ret;
  int chunkSize = 1 << 20;
  int pos = 0;
  while ( remainingSize ) {
    int epochSize = std::min( remainingSize, chunkSize );
    response.set_buf( buf.substr( pos, epochSize ) );
    response.set_ret( epochSize );
    response.set_server_errno( 0 );
    pos += epochSize;
    remainingSize -= epochSize;
    writer->Write( response );
  }
  return grpc::Status::OK;
}

#ifdef HAVE_XATTR
grpc::Status WowFSServiceImpl::GetXAttr(grpc::ServerContext* context, const wowfs::GetXAttrRequest* request, 
          grpc::ServerWriter<wowfs::DownloadResponse>* writer)
{
    int res;
    char * buffer = (char*)malloc(request->size());

#ifdef __APPLE__
    res = getxattr(
        (this->basedir_ + request->file_path()).c_str(), request->name().c_str(), buffer, request->size(), 0, XATTR_NOFOLLOW);
#else
    res = getxattr((this->basedir_ + request->file_path()).c_str(), request->name().c_str(), buffer, request->size());
#endif /* __APPLE__ */

    wowfs::DownloadResponse response;
    response.set_data(buffer, request->size());
    response.set_ret(res);
    response.set_server_errno(errno);
    writer->Write(response);

    free(buffer);

    return grpc::Status::OK; 
}
#endif

grpc::Status WowFSServiceImpl::Access(grpc::ServerContext* context,
         const wowfs::AccessRequest* request, wowfs::AccessResponse* response)
{
    //Run access operation
    mode_t mode = request->mode();
    int res = access((this->basedir_ + request->file_path()).c_str(), mode);
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
    int ret = mkdir((this->basedir_ + request->dir_name()).c_str(), 0777);
    response->set_ret(ret);
    response->set_server_errno(errno);
    
    return grpc::Status::OK;
}

grpc::Status WowFSServiceImpl::Rmdir(
    grpc::ServerContext* context, const wowfs::RmdirRequest* request,
    wowfs::RmdirResponse* response)
{
    //Run rmdir operation
    int ret = rmdir((this->basedir_ + request->dir_name()).c_str());
    response->set_ret(ret);
    response->set_server_errno(errno);
    
    return grpc::Status::OK;
}

grpc::Status WowFSServiceImpl::Open(
    grpc::ServerContext* context, const wowfs::OpenRequest* request,
    wowfs::OpenResponse* response)
{
    //Run open operation
    int ret = open((this->basedir_ + request->file_name()).c_str(), request->flags() | O_CREAT, 0777);
    response->set_ret(ret);
    response->set_server_errno(errno);
    close(ret); 
    return grpc::Status::OK;
}

grpc::Status WowFSServiceImpl::Create(
    grpc::ServerContext* context, const wowfs::CreateRequest* request,
    wowfs::CreateResponse* response)
{
    //Run open operation
    int ret = open((this->basedir_ + request->file_name()).c_str(), request->flags(), 0777);
    response->set_ret(ret);
    response->set_server_errno(errno);
    close(ret);
    
    return grpc::Status::OK;
}

grpc::Status WowFSServiceImpl::Writeback(
    grpc::ServerContext* context, grpc::ServerReader<wowfs::StreamWriteRequest>* reader,
    wowfs::StreamWriteResponse* response)
{
  int64_t numBytesWritten = 0;
  int outputFd = -1;
  wowfs::StreamWriteRequest req;
  while ( reader->Read( &req ) ) {
    if ( outputFd == -1 ) {
      // there may be a case when a file is deleted on the server, while another user
      // writes back to it later, therefore we should create a file if it doesn't exist
      outputFd = open( (this->basedir_ + req.path()).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777 );
      if ( outputFd == -1 ) {
        // something went wrong
        response->set_ret( -1 );
        response->set_server_errno( errno );
        return grpc::Status::OK;
      }
      std::cerr << "file opened for writing: " << this->basedir_ + req.path() << std::endl;
    }
    auto res = pwrite( outputFd, req.buf().data(), req.size(), req.offset() );
    if ( res == -1 ) {
      response->set_ret( -1 );
      response->set_server_errno( errno );
      return grpc::Status::OK;
    }
    numBytesWritten += res;
  }
  if ( outputFd > 0 ) {
    // if we have an output file, make sure the data persists on the disk
    fsync( outputFd );
    close( outputFd );
  }
  std::cerr << "writes completed, # bytes written: " << numBytesWritten << std::endl;
  response->set_ret( numBytesWritten );
  response->set_server_errno( 0 );
  return grpc::Status::OK;
}

grpc::Status WowFSServiceImpl::ReadDir(
    grpc::ServerContext* context, const wowfs::StreamReadRequest* request,
    grpc::ServerWriter<wowfs::StreamReadResponse>* writer )
{
  wowfs::StreamReadResponse response;
  DIR *dir = opendir( (this->basedir_ + request->path()).c_str() );
  if ( dir == NULL ) {
    response.set_ret( -1 );
    response.set_server_errno( errno );
    writer->Write( response );
    return grpc::Status::OK;
  }
  struct dirent* de;

  std::vector<dirent> entries;

  while ( ( de = readdir( dir ) ) != NULL ) {
    entries.push_back( *de );
  }

  closedir( dir );

  std::string buf(
      reinterpret_cast<const char*>( entries.data() ),
      entries.size() * sizeof( dirent ) );
  int remainingSize = buf.size();
  int chunkSize = 1<<20;
  int pos = 0;
  while ( remainingSize ) {
    int epochSize = std::min( remainingSize, chunkSize );
    response.set_buf( buf.substr( pos, epochSize ) );
    response.set_ret( epochSize );
    response.set_server_errno( 0 );
    pos += epochSize;
    remainingSize -= epochSize;
    writer->Write( response );
  }
  return grpc::Status::OK;
}
grpc::Status WowFSServiceImpl::Unlink(grpc::ServerContext* context, const wowfs::UnlinkRequest* request, 
                      wowfs::UnlinkResponse* response) 
{
    int ret = unlink((this->basedir_ + request->path()).c_str());
    response->set_ret(ret);
    response->set_server_errno(errno);

    return grpc::Status::OK;
}

grpc::Status WowFSServiceImpl::Rename(grpc::ServerContext* context, const wowfs::RenameRequest* request, 
                      wowfs::RenameResponse* response) 
{
    int ret = rename(
        (this->basedir_ + request->old_path()).c_str(), 
        (this->basedir_ + request->new_path()).c_str());
    response->set_ret(ret);
    response->set_server_errno(errno);

    return grpc::Status::OK;
}

grpc::Status WowFSServiceImpl::Link(grpc::ServerContext* context, const wowfs::LinkRequest* request,
                      wowfs::LinkResponse* response)
{
    int ret = link(
        (this->basedir_ + request->old_path()).c_str(),
        (this->basedir_ + request->new_path()).c_str());
    response->set_ret(ret);
    response->set_server_errno(errno);

    return grpc::Status::OK;
}

grpc::Status WowFSServiceImpl::Symlink(grpc::ServerContext* context, const wowfs::SymlinkRequest* request,
                      wowfs::SymlinkResponse* response)
{
    int ret = symlink(
        request->target().c_str(),
        (this->basedir_ + request->link_path()).c_str());
    response->set_ret(ret);
    response->set_server_errno(errno);

    return grpc::Status::OK;
}
