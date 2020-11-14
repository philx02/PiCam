#include <iostream>
#include <cstdlib>
#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;
namespace bp = boost::process;
namespace bpo = boost::program_options;

int main(int argc, char *argv[])
{
  bpo::options_description wAllOptions("All options");
  wAllOptions.add_options()
    ("help", "Produce help message.")
    ("MainProgram,p", bpo::value< std::string >()->required(), "Path to the main program")
    ("ConnectionMonitor,m", bpo::value< std::string >()->required(), "Path to the connection monitor")
    ("RepairConnection,r", bpo::value< std::string >()->required(), "Path to the repair connection script")
    ;

  bpo::variables_map wVariablesMap;
  bpo::store(bpo::parse_command_line(argc, argv, wAllOptions), wVariablesMap);

  if (wVariablesMap.count("help"))
  {
    std::cout << wAllOptions << std::endl;
    return 0;
  }

  auto &&wMainProgram = wVariablesMap["MainProgram"].as< bfs::path >().c_str();
  auto &&wConnectionMonitor = wVariablesMap["ConnectionMonitor"].as< bfs::path >().c_str();
  auto &&wRepairConnection = wVariablesMap["RepairConnection"].as< bfs::path >().c_str();

  if (!bfs::exists(wMainProgram))
  {
    std::cerr << "MainProgram not found." << std::endl;
    exit(1);
  }
  if (!bfs::exists(wConnectionMonitor))
  {
    std::cerr << "ConnectionMonitor not found." << std::endl;
    exit(1);
  }
  if (!bfs::exists(wRepairConnection))
  {
    std::cerr << "RepairConnection not found." << std::endl;
    exit(1);
  }

  while (true)
  {
    auto wChild = bp::child(wMainProgram);
    //if (bp::system("..\\bin\\Debug\\ConnectionMonitor.exe", argv[1]) != 0)
    if (bp::system(wConnectionMonitor) != 0)
    {
      std::cout << "Disconnected, restarting network..." << std::endl;
      bp::system(wRepairConnection);
    }
  }
  return 0;
}
