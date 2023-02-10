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
struct stat * WowRPCClient::DownloadStat(const std::string& file_name)
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
      std::cerr << "Failed to download file stat : " << status.error_message() << "\n";
      return nullptr;
  }
  
  //Copy and return.
  struct stat * s = (struct stat *)malloc(sizeof(struct stat));
  memcpy(s, response.data().data(), response.data().size());

  return s;
}
