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
RPCResponse WowRPCClient::DownloadStat(const std::string& file_name, struct stat* buf)
{
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
      return RPCResponse(-1, -1);
  }
  
  //Copy and return.
  memcpy(buf, response.data().data(), sizeof(struct stat));

  return RPCResponse(response.ret(), response.server_errno());
}

RPCResponse WowRPCClient::Mkdir(const std::string& dir_name, mode_t mode) {
  wowfs::MkdirRequest request; 
  wowfs::MkdirResponse response;
  grpc::ClientContext context;

  // Prepare request
  request.set_dir_name(dir_name);
  request.set_mode(mode);

  // Dispatch
  auto status = stub_->Mkdir(&context, request, &response);

  // Check response
  if (!status.ok()) {
    std::cerr << "Mkdir rpc failed\n";
    return RPCResponse(-1, -1);
  }

  return RPCResponse(response.ret(), response.server_errno());
}