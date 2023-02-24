#include <iostream>

#include "WowSocket.H"
#include "WowConcert.H"
#include "argparse/argparse.hpp"

int main( int argc, char** argv ) {
  argparse::ArgumentParser program("orchestrator");
  program.add_argument( "--port" )
    .required()
    .help( "run orchestrator service on this port" );

  program.add_argument( "--iters" )
    .required()
    .help( "number of test iterations" );

  try {
    program.parse_args(argc, argv);
  }
  catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

  // fetch these as strings, oddly enough the library can't cast
  // these to int directly
  auto port = program.get<std::string>( "--port" );
  auto numIters = program.get<std::string>( "--iters" );
  auto fName = program.get<std::string>( "--file" );

  WowOrchestra abtesting( std::stoi( port ), fName );
  abtesting.perform( std::stoi( numIters ) );
}
