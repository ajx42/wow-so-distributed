#include <iostream>
#include "WowRPCServer.H"
#include "argparse/argparse.hpp"

//Remove partially written files
void CleanTmpFiles(const char* path, const char* extension) {
    std::cout << "Cleaning up leftover tmp files...\n";
    DIR* dir = opendir(path);
    if (dir == nullptr) {
        std::cerr << "Error opening directory: " << path << std::endl;
        return;
    }
    
    //Recursively iterate through FS and remove tmp files.
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        //Directory
        if (entry->d_type == DT_DIR) {
            if (std::strcmp(entry->d_name, ".") != 0 && std::strcmp(entry->d_name, "..") != 0) {
                std::string new_path = std::string(path) + "/" + entry->d_name;
                CleanTmpFiles(new_path.c_str(), extension);
            }
        }
        //"Regular" file
        else {
            std::string filename = entry->d_name;
            if (filename.size() >= strlen(extension) && filename.substr(filename.size() - strlen(extension)) == extension) {
                std::string full_path = std::string(path) + "/" + filename;
                std::cout << "Deleting file: " << full_path << std::endl;
                if (std::remove(full_path.c_str()) != 0) {
                    std::cerr << "Error deleting file: " << full_path << std::endl;
                }
            }
        }
    }

    closedir(dir);
}

int main( int argc, char** argv )
{
  //Parse command line arguments
  argparse::ArgumentParser program("server");
  program.add_argument("--withCrashOnWrite")
    .help("Crash the server when a write is requested.")
    .default_value(false)
    .implicit_value(true);

  try {
    program.parse_args( argc, argv );
  }
  catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

  auto withCrashOnWrite = program.get<bool>("--withCrashOnWrite");
  if (withCrashOnWrite) {
    std::cout << "Server will crash on write.\n";
  }
  

  //If we crashed, clean up any partially written files
  //This may report an error with our current testing script
  //since we start the server before directories are created.
  CleanTmpFiles("/tmp/wowfs_remote", ".wow");

  std::string server_addr("0.0.0.0:50051");
  WowFSServiceImpl service("/tmp/wowfs_remote", withCrashOnWrite);

  grpc::EnableDefaultHealthCheckService( true );
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  grpc::ServerBuilder builder;

  builder.AddListeningPort( server_addr, grpc::InsecureServerCredentials() );
  builder.RegisterService( &service );

  std::unique_ptr<grpc::Server> server( builder.BuildAndStart() );
  std::cout << "Server listening on " << server_addr << std::endl;

  server->Wait();

  return 0;
}
