#include <iostream>

#include "WowSocket.H"
#include "WowConcert.H"

int main() {
    struct sockaddr_in     servaddr;
   
    memset(&servaddr, 0, sizeof(servaddr));
       
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(61617);
    servaddr.sin_addr.s_addr = INADDR_ANY;
  
  auto participant = WowParticipant( 71711, *reinterpret_cast<struct sockaddr*>( &servaddr ),
                                    ParticipantId::NODE_B );

  participant.notifyOrchestrator( MessageType::REGISTRATION );
  participant.waitFor( MessageType::ACK );
  participant.notifyOrchestrator( MessageType::READY );
  participant.waitFor( MessageType::TRIGGER );
  std::cout << "Triggered" << std::endl;
}
