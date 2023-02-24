#include <iostream>
#include <map>
#include <set>
#include <array>
#include <thread>
#include <chrono>

#include "WowSocket.H"
#include "WowLogger.H"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

enum ParticipantId : int32_t
{
  SINK          = 1 << 0,
  NODE_A        = 1 << 1,
  NODE_B        = 1 << 2,
  ORCHESTRATOR  = 1 << 3,
  LAST_PT       = 1 << 4,
};

enum class MessageType : int32_t
{
  RESET = 0,
  REGISTRATION = 1,
  TRIGGER = 2,
  READY = 3,
  ACK = 4,
  GO = 5,
  DONE = 6,
  LAST_MSG = 7,
};

constexpr const int32_t getNumParticipants()
{
  return __builtin_popcount( ParticipantId::LAST_PT - 1 ) - 1; 
  // substract 1 since we are not counting the orchestrator
}

struct WowPigeon
{
  ParticipantId origin = ParticipantId::LAST_PT;
  MessageType kind = MessageType::LAST_MSG;
} __attribute__((packed)) ;

// Main class used by orchestrator for the "performance"
class WowOrchestra
{
public:
  WowOrchestra( int32_t port, int32_t secondaryPort ) 
    : socket_( port ), secondarySocket_( secondaryPort ) {} 
  void perform( int32_t rounds );

private:
   // wait for everyone to join
  void waitRegistrations();
  void waitMsgSubset( MessageType, int32_t pmask );
  bool notifyOne( ParticipantId, MessageType, bool useSecondary = false );
  bool notifyAll( MessageType );

  WowUDPSocket socket_;
  WowUDPSocket secondarySocket_; // used to send triggers to NODE_B
  std::map<ParticipantId, struct sockaddr> contactDetails_;
};

inline void WowOrchestra::waitRegistrations()
{
  auto pendingRegistrations = getNumParticipants();
  while ( pendingRegistrations ) {
    auto ret = socket_.waitReceive();
    if ( ret >= sizeof(WowPigeon) ) {
      auto& buf = socket_.getReadBuffer();
      auto& msg = *reinterpret_cast<const WowPigeon*>( buf.data() );
      if ( contactDetails_.find( msg.origin ) == contactDetails_.end() ) {
        pendingRegistrations--;
        std::cout << "Registered: " << msg.origin << std::endl;
      }
      std::cout << msg.origin << " " << static_cast<int32_t>( msg.kind ) << std::endl;
      auto lastClientCopy = socket_.getLastClient();
      contactDetails_[msg.origin] = *reinterpret_cast<struct sockaddr*>(
        &lastClientCopy );
    }
  }
}

inline bool WowOrchestra::notifyOne( ParticipantId id, MessageType mtype, bool useSecondary /*false*/ )
{
  auto& sendSocket = useSecondary ? secondarySocket_ : socket_;
  std::vector<uint8_t> writeBuf( sizeof(WowPigeon) );
  *reinterpret_cast<WowPigeon*>( writeBuf.data() ) = WowPigeon {
    .origin = ParticipantId::ORCHESTRATOR,
    .kind = mtype
  };
  return sendSocket.sendTo( contactDetails_[id], writeBuf );
}

inline bool WowOrchestra::notifyAll( MessageType mtype )
{
  bool outcome = true;

  for ( auto [id, _] : contactDetails_ ) { 
    outcome &= notifyOne( id , mtype );
  }
  return outcome;
}

inline void WowOrchestra::waitMsgSubset( MessageType mtype, int32_t pmask )
{
  int32_t cmask = 0;
  while ( cmask != pmask ) {
    auto ret = socket_.waitReceive();
    if ( ret >= sizeof( WowPigeon ) ) {
      auto& buf = socket_.getReadBuffer();
      auto& msg = *reinterpret_cast<const WowPigeon*>( buf.data() );
      if ( msg.kind == mtype && ( pmask & msg.origin ) != 0 ) {
        cmask |= msg.origin;
      }
    }
  }
}

inline void WowOrchestra::perform( int32_t rounds )
{
  waitRegistrations();
  notifyAll( MessageType::ACK );
  
  waitMsgSubset(
    MessageType::READY, ParticipantId::NODE_A | ParticipantId::NODE_B | ParticipantId::SINK );
 
  // my poor attempt at trying to send triggers in parallel as far as possible 
  // the writes are however so small that this hardly is going to make a difference
  auto notifyABParallel = [&]( auto&& mtype ) {
    auto workerA = std::thread([&](){
      notifyOne( ParticipantId::NODE_A, mtype );    
    });

    auto workerB = std::thread([&](){
      notifyOne( ParticipantId::NODE_B, mtype, true );
    });

    workerA.join();
    workerB.join();
  };

  while ( rounds-- ) {
    // trigger writes
    notifyABParallel( MessageType::TRIGGER );

    // wait for clients to be done with the writes
    waitMsgSubset(
      MessageType::DONE, ParticipantId::NODE_A | ParticipantId::NODE_B );
    
    // notify sink the observe file and record results
    notifyOne( ParticipantId::SINK, MessageType::GO );

    // wait for sink to be done and be ready for next round
    waitMsgSubset( MessageType::READY, ParticipantId::SINK );

    // reset and prepare for next round
    notifyABParallel( MessageType::RESET );
    
    // wait for clients to be ready for writes again
    waitMsgSubset(
      MessageType::READY, ParticipantId::NODE_A | ParticipantId::NODE_B );
  }
}

// File System Nodes i.e. A, B, and Server (Sink) are the participants.
class WowParticipant
{
public:
  static constexpr size_t READ_BUF_SIZE = 256;

  WowParticipant(
      int32_t port, struct sockaddr orchestratorAddr,
      ParticipantId id, std::string filename )
    : socket_( port ), orchestratorAddr_( orchestratorAddr ),
      id_( id ), filename_( filename ) {}

  void play();

private:
  bool notifyOrchestrator( MessageType );
  void waitFor( MessageType );
  
  void playSink();
  void playClient();

  WowUDPSocket socket_;
  struct sockaddr orchestratorAddr_;
  ParticipantId id_;
  std::string filename_;

  std::array<uint8_t, READ_BUF_SIZE> readBuf_;

};

inline void WowParticipant::play() {
  if ( id_ == ParticipantId::SINK ) playSink();
  else playClient();
}

inline void WowParticipant::playSink()
{
  notifyOrchestrator( MessageType::REGISTRATION );
  waitFor( MessageType::ACK );

  int iter = 0;

  while ( true ) {
    notifyOrchestrator( MessageType::READY );
    waitFor( MessageType::GO );
    // a FUGLY hack, since writes from clients return even before writing
    // back, their signal does not imply that the write itself is done
    // so we need to make sure, we actually read something! We sleep here
    // and hope things work out.
    std::this_thread::sleep_for(
        std::chrono::milliseconds( 5 ) );
    
    auto fd = open( filename_.c_str(), O_RDONLY );
    auto ret = read( fd, readBuf_.data(), READ_BUF_SIZE );

    if ( ret != -1 ) {
      std::cout << iter++ << " : " << std::string( (const char*)readBuf_.data(), ret ) << std::endl;
    }
    
    close( fd );
  }
}

inline void WowParticipant::playClient()
{
  std::string nodeName = id_ == ParticipantId::NODE_A ? "node_a" : "node_b";
  notifyOrchestrator( MessageType::REGISTRATION );
  waitFor( MessageType::ACK );
  
  while( true ) {
    //prepare
    auto fd = open( filename_.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0777 );
    auto ret = write( fd, nodeName.data(), nodeName.size() );
     if ( ret == -1 ) {
      std::cout << "write failed: " << errno << std::endl;
    }
    std::cout << nodeName << " prepared." << std::endl;


    notifyOrchestrator( MessageType::READY );
    waitFor( MessageType::TRIGGER );
    
    close( fd );
    std::cout << nodeName << " triggered." << std::endl;
    
    notifyOrchestrator( MessageType::DONE );
    waitFor( MessageType::RESET );
  }
}

inline bool WowParticipant::notifyOrchestrator( MessageType mtype )
{
  std::vector<uint8_t> writeBuf( sizeof(WowPigeon) );
  *reinterpret_cast<WowPigeon*>( writeBuf.data() ) = WowPigeon {
    .origin = id_,
    .kind = mtype
  };

  return socket_.sendTo( orchestratorAddr_, writeBuf );
}

inline void WowParticipant::waitFor( MessageType mtype )
{
  while( true ) {
    auto ret = socket_.waitReceive();
    if ( ret >= sizeof(WowPigeon) ) {
      auto& buf = socket_.getReadBuffer();
      auto& msg = *reinterpret_cast<const WowPigeon*>( buf.data() );
      if ( msg.origin == ParticipantId::ORCHESTRATOR &&
           msg.kind == mtype ) {
        std::cout << "Received mtype=" << static_cast<int32_t>(mtype) << std::endl;
        break;
      }
    }
  }
}

