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

RPCResponse WowRPCClient::GetXAttr(const std::string& file_path, 
  const std::string& name, char * value, const size_t size)
 {

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
      return RPCResponse(-1, -1);
  }

  //Copy and return
  memcpy(value, response.data().data(), response.data().size());

  return RPCResponse(response.ret(), response.server_errno());
}

RPCResponse WowRPCClient::Access(const std::string& file_path, mode_t mode)
{
  wowfs::AccessRequest request;
  wowfs::AccessResponse response;
  grpc::ClientContext context;

  // Prepare request
  request.set_file_path(file_path);
  request.set_mode(mode);

  // Dispatch
  auto status = stub_->Access(&context, request, &response);

  // Check response
  if (!status.ok()) {
    std::cerr << "Access rpc failed\n";
    return RPCResponse(-1, -1);
  }

  return RPCResponse(response.ret(), response.server_errno());
}

RPCResponse WowRPCClient::Mkdir(const std::string& dir_name, mode_t mode)
{
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

RPCResponse WowRPCClient::Rmdir(const std::string& dir_name) {
  wowfs::RmdirRequest request;
  wowfs::RmdirResponse response;
  grpc::ClientContext context;

  // Prepare request
  request.set_dir_name(dir_name);

  // Dispatch
  auto status = stub_->Rmdir(&context, request, &response);

  // Check response
  if (!status.ok()) {
    std::cerr << "Rmdir rpc failed\n";
    return RPCResponse(-1, -1);
  }

  return RPCResponse(response.ret(), response.server_errno());
}

RPCResponse WowRPCClient::Open(const std::string& file_name, int flags) {
  wowfs::OpenRequest request;
  wowfs::OpenResponse response;
  grpc::ClientContext context;

  // Prepare request
  request.set_file_name(file_name);
  request.set_flags(flags);

  // Dispatch
  auto status = stub_->Open(&context, request, &response);

  // Check response
  if (!status.ok()) {
    std::cerr << "Open rpc failed\n";
    return RPCResponse(-1, -1);
  }

  return RPCResponse(response.ret(), response.server_errno());
}

RPCResponse WowRPCClient::Create(const std::string& file_name, mode_t mode, int flags) {
  wowfs::CreateRequest request;
  wowfs::CreateResponse response;
  grpc::ClientContext context;

  // Prepare request
  request.set_file_name(file_name);
  request.set_mode(mode);
  request.set_flags(flags);

  // Dispatch
  auto status = stub_->Create(&context, request, &response);

  // Check response
  if (!status.ok()) {
    std::cerr << "Create rpc failed\n";
    return RPCResponse(-1, -1);
  }

  return RPCResponse(response.ret(), response.server_errno());
}

RPCResponse WowRPCClient::Utimens(const std::string& file_name, const struct timespec ts[2]) {
  wowfs::UtimensRequest request;
  wowfs::UtimensResponse response;
  grpc::ClientContext context;

  std::unique_ptr<grpc::ClientWriter<wowfs::UtimensRequest>> writer(stub_->Utimens(&context, &response));
  
  // Prepare request
  request.set_file_name(file_name);
  request.set_data(reinterpret_cast<const char*>(ts), sizeof(ts));
  writer->Write(request);
  writer->WritesDone();

  auto status = writer->Finish();
  if(!status.ok())
  {
      std::cerr << "Utimens rpc failed\n";
      return RPCResponse(-1, -1);
  }

  return RPCResponse(response.ret(), response.server_errno());
}