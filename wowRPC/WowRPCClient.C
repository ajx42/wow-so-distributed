#include "WowRPCClient.H"
#include <iostream>
#include <sys/stat.h>
#include <dirent.h>

#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>

// Note this method is only used for Testing
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

// Note this method is only used for Testing
void WowRPCClient::PerformSpeedTest( int32_t identifier, int64_t msgSize )
{
  grpc::ClientContext context;
  wowfs::SpeedTestRequest req;
  wowfs:: Ack reply;

  req.set_identifier( identifier );
  std::string data(msgSize, '0');
  req.set_data( data );
  
  auto t_start = std::chrono::high_resolution_clock::now();
  auto status = stub_->SpeedTest( &context, req, &reply );
  auto t_end = std::chrono::high_resolution_clock::now();

  std::cout   << "Size=" << msgSize <<"|"
              << "Status=" << status.ok() << "|"
              << std::fixed << std::setprecision(10)
              << "WallClockTime(us)="
              << std::chrono::duration<double, std::micro>(t_end-t_start).count()
              << std::endl;
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

RPCResponse WowRPCClient::Writeback( const std::string& path, const std::string& buf )
{
  wowfs::StreamWriteRequest request;
  wowfs::StreamWriteResponse response;
  grpc::ClientContext context;
  std::unique_ptr<grpc::ClientWriter<wowfs::StreamWriteRequest>> writer(
      stub_->Writeback(&context, &response) );
  int64_t remainingData = buf.size();
  int64_t chunkSize = 1 << 20; // 1 MB chunks
  int64_t pos = 0;
  // data being written back can be empty in case the file was truncated to
  // 0 bytes.
  do {
    request.set_path(path);
    // @TODO: this can be optimised
    auto epochSize = std::min( chunkSize, remainingData );
    request.set_path( path );
    request.set_buf( buf.substr( pos, epochSize ) );
    request.set_size( epochSize );
    request.set_offset( pos );
    pos += epochSize;
    remainingData -= epochSize;
    if ( ! writer->Write( request ) ) {
      std::cerr << "broken pipe! writes failing" << std::endl;
      break;
    }
  } while ( remainingData );
  writer->WritesDone();
  auto status = writer->Finish();
  // if things went well return number of bytes written
  if ( status.ok() ) {
    return RPCResponse(response.ret(), response.server_errno());
  } else {
    return RPCResponse(-1, -1);
  }
}

RPCResponse WowRPCClient::DownloadFile(
    const std::string& path, std::string& buf, size_t fileSize )
{
  buf.clear();
  buf.reserve(fileSize + 100); // i am not superstitious, just a lill
  buf = "";
  wowfs::StreamReadRequest request;
  wowfs::StreamReadResponse response;
  grpc::ClientContext context;
  size_t bytesRead = 0;
  request.set_path( path );
  request.set_size( fileSize );
  std::unique_ptr<grpc::ClientReader<wowfs::StreamReadResponse>> reader(
      stub_->ReadFile( &context, request ) );
  while ( reader->Read( &response ) ) {
    if ( response.ret() < 0 ) {
      // we are probably done
      break;
    }
    // server returns the # bytes read in ret if it's not negative
    bytesRead += response.ret();
    buf += response.buf().substr(0, response.ret());
  }
  auto status = reader->Finish();
  if ( status.ok() ) {
    // not sure if this is the right way for error handling
    return RPCResponse( response.ret() >= 0 ? bytesRead : response.ret(), response.server_errno() );
  } else {
    return RPCResponse(-1, -1);
  }
}

RPCResponse WowRPCClient::DownloadDir(
    const std::string& path, std::string& buf )
{
  buf.clear();
  buf = "";
  // ideally we should reserve some space

  wowfs::StreamReadRequest request;
  wowfs::StreamReadResponse response;
  grpc::ClientContext context;

  size_t bytesRead = 0;
  request.set_path( path );
  request.set_size( -1 ); // we don't know this yet
  std::unique_ptr<grpc::ClientReader<wowfs::StreamReadResponse>> reader(
      stub_->ReadDir( &context, request ) );
  while ( reader->Read( &response ) ) {
    if ( response.ret() < 0 ) {
      break;
    }
    bytesRead += response.ret();
    buf += response.buf().substr(0, response.ret());
  }
  auto status = reader->Finish();
  if ( status.ok() ) {
    return RPCResponse( response.ret() >= 0 ? bytesRead : response.ret(), response.server_errno() );
  } else {
    return RPCResponse( -1, -1 );
  }
}

RPCResponse WowRPCClient::Unlink( const std::string& path ){
  wowfs::UnlinkRequest request;
  wowfs::UnlinkResponse response;
  grpc::ClientContext context;

  request.set_path( path );
  
  auto status = stub_->Unlink( &context, request, &response );

  if ( !status.ok() ) {
    return RPCResponse(-1, -1);
  }
  return RPCResponse( response.ret(), response.server_errno() );
}

RPCResponse WowRPCClient::Rename( const std::string& oldPath, const std::string& newPath ){
  wowfs::RenameRequest request;
  wowfs::RenameResponse response;
  grpc::ClientContext context;

  request.set_old_path( oldPath );
  request.set_new_path( newPath );
  
  auto status = stub_->Rename( &context, request, &response );

  if ( !status.ok() ) {
    return RPCResponse(-1, -1);
  }
  return RPCResponse( response.ret(), response.server_errno() );
}

RPCResponse WowRPCClient::Link( const std::string& oldPath, const std::string& newPath ){
  wowfs::LinkRequest request;
  wowfs::LinkResponse response;
  grpc::ClientContext context;

  request.set_old_path( oldPath );
  request.set_new_path( newPath );
  
  auto status = stub_->Link( &context, request, &response );

  if ( !status.ok() ) {
    return RPCResponse(-1, -1);
  }
  return RPCResponse( response.ret(), response.server_errno() );
}

RPCResponse WowRPCClient::Symlink( const std::string& target, const std::string& linkPath ){
  wowfs::SymlinkRequest request;
  wowfs::SymlinkResponse response;
  grpc::ClientContext context;

  request.set_target( target );
  request.set_link_path( linkPath );
  
  auto status = stub_->Symlink( &context, request, &response );

  if ( !status.ok() ) {
    return RPCResponse(-1, -1);
  }
  return RPCResponse( response.ret(), response.server_errno() );
}
