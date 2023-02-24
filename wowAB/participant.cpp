#include <iostream>

#include "WowSocket.H"
#include "WowConcert.H"
#include "argparse/argparse.hpp"

int main( int argc, char** argv ) {
  argparse::ArgumentParser program( "client" );
 
  program.add_argument( "--type" )
    .required()
    .help( "which client a or b" );

  program.add_argument( "--port" )
    .required()
    .help( "run client service on this port" );

  program.add_argument( "--orchestrator-host" )
    .required()
    .help( "ip for the orchestrator" );
 
  program.add_argument( "--orchestrator-port" )
    .required()
    .help( "port for the orchestrator" );

  program.add_argument( "--file" )
    .required()
    .help( "target file to work with" );

  try {
    program.parse_args(argc, argv);
  }
  catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

  auto fName = program.get<std::string>( "--file" );
  auto oPort = (uint32_t)std::stoi( program.get<std::string>( "--orchestrator-port" ) );
  auto oHost = program.get<std::string>( "--orchestrator-host" );
  auto pPort = (uint32_t)std::stoi( program.get<std::string>( "--port" ) );
  auto pType = program.get<std::string>( "--type" );
  auto pId   = ParticipantId::LAST_PT; 

  if ( pType == "node_a" ) pId = ParticipantId::NODE_A;
  if ( pType == "node_b" ) pId = ParticipantId::NODE_B;
  if ( pType == "sink"   ) pId = ParticipantId::SINK;

  if ( pId == ParticipantId::LAST_PT ) {
    std::cerr << "Unknown service type" << std::endl;
    std::exit( 1 );
  } 

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  // Filling server information
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons( oPort );
  servaddr.sin_addr.s_addr = inet_addr( oHost.c_str() );

  auto participant = WowParticipant(
    pPort, *reinterpret_cast<struct sockaddr*>( &servaddr ), pId, fName
  );

  participant.play();
}

