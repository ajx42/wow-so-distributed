#include "WowRPCClient.H"
#include <iostream>
#include <sys/stat.h>

int32_t WowRPCClient::Ping( int32_t cmd )
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

//Download struct stat from server for given filepath.
int32_t WowRPCClient::DownloadStat(const std::string& file_name, struct stat* buf, int* errno_){
  wowfs::DownloadRequest request;
  wowfs::DownloadResponse response;
  grpc::ClientContext context;
    
  //Prepare request
  request.set_file_name(file_name);
    
  //Dispatch
  auto writer = stub_->DownloadStat(&context, request);
  
  //Check response
  writer->Read(&response);

  grpc::Status status = writer->Finish();

  if(!status.ok())
  {
      std::cerr << "DownloadStat rpc failed\n";
      return -1;
  }
  
  //Copy and return.
  memcpy(buf, response.data().data(), sizeof(struct stat));
  *errno_ = response.errno_();

  return response.res();
}

//#ifdef HAVE_XATTR
int32_t WowRPCClient::GetXAttr(const std::string& file_path, const std::string& name, char * value, const size_t size, int* errno_){
  wowfs::GetXAttrRequest request;
  wowfs::DownloadResponse response;
  grpc::ClientContext context;

  //Prepare request
  request.set_file_path(file_path);
  request.set_name(name);
  request.set_size(size);

  //Dispatch
  auto writer = stub_->GetXAttr(&context, request);

  //Check Response
  writer->Read(&response);

  grpc::Status status = writer->Finish();

  if(!status.ok())
  {
      std::cerr << "DownloadStat rpc failed\n";
      return -1;
  }

  //Copy and return
  memcpy(value, response.data().data(), response.data().size());
  *errno_ = response.errno_();

  return response.res();
}
//#endif

int32_t WowRPCClient::Access(const std::string& file_path, mode_t mode, int* errno_)
{
  wowfs::AccessRequest request;
  wowfs::AccessResponse response;
  grpc::ClientContext context;

  // Prepare request
  request.set_file_path(file_path);
  request.set_mode(mode);

  // Dispatch
  auto status = stub_->Access(&context, request, &response);
  *errno_ = response.errno_();

  // Check response
  if (!status.ok()) {
    std::cerr << "Access rpc failed\n";
    return -1;
  }

  return response.res();
}

int32_t WowRPCClient::Mkdir(const std::string& dir_name, mode_t mode, int* errno_) {
  wowfs::MkdirRequest request; 
  wowfs::MkdirResponse response;
  grpc::ClientContext context;

  // Prepare request
  request.set_dir_name(dir_name);
  request.set_mode(mode);

  // Dispatch
  auto status = stub_->Mkdir(&context, request, &response);
  *errno_ = response.errno_();

  // Check response
  if (!status.ok()) {
    std::cerr << "Mkdir rpc failed\n";
    return -1;
  }

  return response.res();
}