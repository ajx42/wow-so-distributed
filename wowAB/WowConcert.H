#include <iostream>
#include <map>
#include <set>

#include "WowSocket.H"
#include "WowLogger.H"

enum class ParticipantId : int32_t
{
  SINK = 0,
  NODE_A = 1,
  NODE_B = 2,
  ORCHESTRATOR = 3,
  LAST_VAL = 4,

};

enum class MessageType : int32_t
{
  RESET = 0,
  REGISTRATION = 1,
  TRIGGER = 2,
  READY = 3,
  ACK = 4,
  LAST_VAL = 5
};

constexpr const int32_t getNumParticipants()
{
  return static_cast<int32_t>( ParticipantId::LAST_VAL ) - 1;
}


struct WowPigeon
{
  ParticipantId origin = ParticipantId::LAST_VAL;
  MessageType kind = MessageType::LAST_VAL;
} __attribute__((packed)) ;

// Main class used by orchestrator for the "performance"
class WowOrchestra
{
public:
  WowOrchestra( int32_t port ) : socket_( port ) {} 
  
  void perform( int32_t rounds );
private:
   // wait for everyone to join
  void waitRegistrations();
  void waitReady();
  bool sendMessage( ParticipantId, MessageType );
  bool notifyAll( MessageType );

  WowUDPSocket socket_;
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
        std::cout << "Registered: " << static_cast<int32_t>( msg.origin ) << std::endl;
      }
      std::cout << static_cast<int32_t>( msg.origin ) << " " << static_cast<int32_t>( msg.kind ) << std::endl;
      auto lastClientCopy = socket_.getLastClient();
      contactDetails_[msg.origin] = *reinterpret_cast<struct sockaddr*>(
        &lastClientCopy );
    }
  }
}

inline void WowOrchestra::waitReady()
{
  std::set<ParticipantId> seen;
  auto pendingMessages = getNumParticipants();
  while ( pendingMessages ) {
    auto ret = socket_.waitReceive();
    if ( ret >= sizeof(WowPigeon) ) {
      auto& buf = socket_.getReadBuffer();
      auto& msg = *reinterpret_cast<const WowPigeon*>( buf.data() );
      if ( msg.kind == MessageType::READY && seen.find( msg.origin ) == seen.end() ) {
        pendingMessages--;
        seen.insert( msg.origin );
        std::cout << "IsReady: " << static_cast<int32_t>( msg.origin ) << std::endl;
      }
    }
  }
}

inline bool WowOrchestra::sendMessage( ParticipantId id, MessageType mtype )
{
  std::vector<uint8_t> writeBuf( sizeof(WowPigeon) );
  *reinterpret_cast<WowPigeon*>( writeBuf.data() ) = WowPigeon {
    .origin = ParticipantId::ORCHESTRATOR,
    .kind = mtype
  };
  return socket_.sendTo( contactDetails_[id], writeBuf );
}

inline bool WowOrchestra::notifyAll( MessageType mtype )
{
  bool outcome = true;
  for ( int i = 0; i < getNumParticipants(); ++i ) {
    outcome &= sendMessage( static_cast<ParticipantId>( i ), mtype );
  }
  return outcome;
}

inline void WowOrchestra::perform( int32_t rounds )
{
  waitRegistrations();
  notifyAll( MessageType::ACK );
  while ( rounds-- ) {
    waitReady();

    sendMessage( ParticipantId::NODE_A, MessageType::TRIGGER );
    sendMessage( ParticipantId::NODE_B, MessageType::TRIGGER );
  }
}

class WowParticipant
{
public:
  WowParticipant( int32_t port, struct sockaddr orchestratorAddr, ParticipantId id )
    : socket_( port ), orchestratorAddr_( orchestratorAddr ), id_( id ) {}
  
  bool notifyOrchestrator( MessageType );
  void waitFor( MessageType );
private:

  WowUDPSocket socket_;
  struct sockaddr orchestratorAddr_;
  ParticipantId id_;

};

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

