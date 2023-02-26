#pragma once

#include <algorithm>
#include <iostream>
#include <vector>
#include <list>
#include <thread>

#include "unreliablefs.h"
#include "unreliablefs_ops.h"

struct CloseOp
{
  CloseOp( std::string path, struct fuse_file_info ff )
    : filePath( path ), fuseFileInfo( ff ) {}

  std::string filePath;
  struct fuse_file_info fuseFileInfo;

  void apply() {
    unreliable_release_impl( filePath.c_str(), &fuseFileInfo );
  }

  int getFd() { return fuseFileInfo.fh; }
};

class WowCloseQueue
{
public:
  static WowCloseQueue& Instance() {
    static WowCloseQueue obj;
    return obj;
  }

  void addNewClose( std::string path, struct fuse_file_info fi );
  
  // any open / rename / delete / flush should first call this
  // and ensure there are no pending closes for the file at hand
  void onFileOp( std::string path );
  
  virtual ~WowCloseQueue();

private:
  WowCloseQueue();
  void popOne();

  bool keepRunning_ = true;
  std::thread watchDog_;
  std::mutex listMutex_;
  std::list<CloseOp> pendingOps_;
};

WowCloseQueue::WowCloseQueue()
{
  watchDog_ = std::thread([&](){    
      while ( keepRunning_ ) {
        std::this_thread::sleep_for( std::chrono::microseconds( 1000 ) );
        popOne();
      }
  });
}

// make sure all pending ops are applied on close
WowCloseQueue::~WowCloseQueue()
{
  // stop the watchdog
  keepRunning_ = false;
  watchDog_.join();

  // clear any remaining ops
  const std::lock_guard<std::mutex> lock( listMutex_ );
  std::for_each(
    pendingOps_.begin(), pendingOps_.end(), 
    [&]( auto&& op ) { 
      LogInfo( "WowCloseQueue: triggering close on destruction for path=" + op.filePath + " fd=" + std::to_string( op.getFd() ) );
      op.apply(); 
    }
  );
}

void WowCloseQueue::popOne()
{
  const std::lock_guard<std::mutex> lock( listMutex_ );
  if ( pendingOps_.empty() ) {
    return;
  }
  auto op = pendingOps_.front();
  pendingOps_.pop_front();
  LogInfo("WowCloseQueue: WatchDog triggered close op on path=" + op.filePath + " fd=" + std::to_string( op.getFd() ));
  op.apply();
}

// we always add to the back to help maintain partial order of ops
// on same file
void WowCloseQueue::addNewClose( std::string path, struct fuse_file_info fi )
{
  const std::lock_guard<std::mutex> lock( listMutex_ );
  LogInfo( "WowCloseQueue: adding new close for path=" + path + " fd=" + std::to_string(fi.fh));
  
  bool found = false;
  pendingOps_.remove_if([&]( auto&& op ){
    auto ret = op.filePath == path && op.getFd() == fi.fh;
    if ( ret ) {
      LogInfo( "WowCloseQueue: (already) triggering close op for path=" + op.filePath + " fd=" + std::to_string( op.getFd() ) );
      op.apply();
    }
    found |= ret;
    return ret;
  });

  if ( found ) {
    return;
  }
  
  pendingOps_.push_back( CloseOp{ path, fi } );
}

void WowCloseQueue::onFileOp( std::string path )
{
  const std::lock_guard<std::mutex> lock( listMutex_ );  
  pendingOps_.remove_if([&]( auto&& op ){
    auto ret = op.filePath == path;
    if ( ret ) {
      LogInfo( "WowCloseQueue: triggering close op for path=" + op.filePath + " fd=" + std::to_string( op.getFd() ) );
      op.apply();
    }
    return ret;
  });
}


