#pragma once

#include <algorithm>
#include <iostream>
#include <vector>
#include <list>
#include <thread>

#include "unreliablefs.h"
#include "unreliablefs_ops.h"

class WritebackOp
{
public:
  enum Kind { CLOSE, FLUSH };

  WritebackOp( std::string path, struct fuse_file_info ff, Kind kd )
    : filePath_( path ), fuseFileInfo_( ff ), kind_( kd )  {}

  void apply();
 
  int getFd()               { return fuseFileInfo_.fh; }
  std::string getFilePath() { return filePath_; }

  std::string getRepr();

private:
  std::string filePath_;
  struct fuse_file_info fuseFileInfo_;
  Kind kind_;
};

void WritebackOp::apply()
{
  if ( kind_ == WritebackOp::CLOSE ) {
    unreliable_release_impl( filePath_.c_str(), &fuseFileInfo_ );    
  } else if ( kind_ == WritebackOp::FLUSH ) {
    WowManager::Instance().writebackToServer( filePath_, fuseFileInfo_.fh );
  }
}

std::string WritebackOp::getRepr()
{
  return std::string("WritebackOp=[")
    + ( kind_ == WritebackOp::CLOSE ? "CLOSE" : "FLUSH") + ", " 
    + filePath_ + ", " + std::to_string(fuseFileInfo_.fh) + "]";
}

class WowWritebackReorderManager
{
public:
  static WowWritebackReorderManager& Instance() {
    static WowWritebackReorderManager obj;
    return obj;
  }

  void addNewWriteback( std::string path, struct fuse_file_info fi, bool isClose );
  
  // any open / rename / delete / flush should first call this
  // and ensure there are no pending closes for the file at hand
  void onFileOp( std::string path );
  
  virtual ~WowWritebackReorderManager();

private:
  WowWritebackReorderManager();
  void popOne();

  bool keepRunning_ = true;
  std::thread watchDog_;
  std::mutex listMutex_;
  std::list<WritebackOp> pendingOps_;
};

WowWritebackReorderManager::WowWritebackReorderManager()
{
  watchDog_ = std::thread([&](){    
      while ( keepRunning_ ) {
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        popOne();
      }
  });
}

// make sure all pending ops are applied on close
WowWritebackReorderManager::~WowWritebackReorderManager()
{
  // stop the watchdog
  keepRunning_ = false;
  watchDog_.join();

  // clear any remaining ops
  const std::lock_guard<std::mutex> lock( listMutex_ );
  std::for_each(
    pendingOps_.begin(), pendingOps_.end(), 
    [&]( auto&& op ) { 
      LogInfo( "WowWritebackReorderManager: triggering on destruction " + op.getRepr() ); 
      op.apply(); 
    }
  );
}

void WowWritebackReorderManager::popOne()
{
  const std::lock_guard<std::mutex> lock( listMutex_ );
  if ( pendingOps_.empty() ) {
    return;
  }
  auto op = pendingOps_.front();
  pendingOps_.pop_front();
  LogInfo("WowWritebackReorderManager: WatchDog triggered " + op.getRepr() );
  op.apply();
}

// we always add to the back to help maintain partial order of ops
// on same file
void WowWritebackReorderManager::addNewWriteback( 
  std::string path, struct fuse_file_info fi, bool isClose )
{
  const std::lock_guard<std::mutex> lock( listMutex_ );

  auto op = WritebackOp{
    path, fi, isClose ? WritebackOp::CLOSE : WritebackOp::FLUSH };
  LogInfo( "WowWritebackReorderManager: adding new " + op.getRepr() ); 
  
  pendingOps_.push_back( op );
}

void WowWritebackReorderManager::onFileOp( std::string path )
{
  const std::lock_guard<std::mutex> lock( listMutex_ );  
  
  // perform all the writebacks, in order for the file
  // optimisation: we can only do one writeback and simple close the rest
  // this can be one motivation for why someone might do such a scheme
  // but for simplicity, we will simply do multiple writebacks
  std::for_each( pendingOps_.begin(), pendingOps_.end(), [&]( auto&& op ) {
    if ( op.getFilePath() == path ) {
      LogInfo( "WowWritebackReorderManager: triggering onFileOp " + op.getRepr() );
      op.apply();
    }
  });
  
  // remove_if does not guarantee order of removal, so we iterate above and apply
  // in order and then get rid of the applied ops
  pendingOps_.remove_if([&]( auto&& op ){
    return op.getFilePath() == path;
  });
}


