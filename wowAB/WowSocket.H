#pragma once

#include <iostream>
#include <array>
#include <exception>
#include <vector>

#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "WowLogger.H"

volatile bool keepRunning = true;

class WowUDPSocket
{
public:
  static const int RECV_BUF_SIZE = 1024;
  using Buf_t = std::array<uint8_t, 1024>;
  WowUDPSocket( int32_t port );

  int waitReceive();

  bool sendTo( struct sockaddr clientAddr, std::vector<uint8_t>& dataVec );
  const Buf_t& getReadBuffer() const { return sockBuf_; }
  const struct sockaddr_in& getLastClient() const { return readAddr_; }


private:
  int sockFd_ = 0;
  int sockPort_ = 0;
  int sockOptVal_ = 1;
  Buf_t sockBuf_;

  struct sockaddr_in sockAddr_, readAddr_;
};

inline WowUDPSocket::WowUDPSocket( int32_t port )
  : sockPort_( port )
{
  sockFd_ = socket( AF_INET, SOCK_DGRAM, 0 );
  if ( sockFd_ < 0 ) {
    LogError( "Error opening socket" );
    std::terminate();
  }

  setsockopt( sockFd_, SOL_SOCKET, SO_REUSEADDR, (const void*) &sockOptVal_,
             sizeof( int ) );
 
  bzero( (char *) &sockAddr_, sizeof( sockAddr_ ) );
  sockAddr_.sin_family = AF_INET;
  sockAddr_.sin_addr.s_addr = htonl( INADDR_ANY );
  sockAddr_.sin_port = htons( sockPort_ );

  if ( bind( sockFd_, (struct sockaddr*) &sockAddr_, sizeof(sockAddr_) ) < 0 ) {
    LogError( "Error binding socket" );
    std::terminate();
  }
}

inline int WowUDPSocket::waitReceive()
{
  bool shouldRepeat = false;
  socklen_t clientLen = sizeof(readAddr_);
  int ret = 0;
  do {
    ret = recvfrom( sockFd_, sockBuf_.data(), RECV_BUF_SIZE, MSG_DONTWAIT,
                         (struct sockaddr*) &readAddr_, &clientLen );
    // if there is any other error, we should break
    // if ret is not -1, we are done and should break
    shouldRepeat = ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK);
  } while ( keepRunning && shouldRepeat );
  return ret ;
}

inline bool WowUDPSocket::sendTo( struct sockaddr clientAddr, std::vector<uint8_t>& dataVec )
{
  int clientLen = sizeof(clientAddr);
  auto ret = sendto( sockFd_, dataVec.data(), dataVec.size(), MSG_NOSIGNAL,
                     &clientAddr, clientLen );
  return ret != -1;
}


