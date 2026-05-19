#include <datadriven/datadriven.hpp>
#include <exception>
#include <iostream>

int main(int argc, char **argv) {
  if (argc == 1) {
    std::cerr << "Usage: " << argv[0] << " <test-file>...\n";
    return 1;
  }
  try {
    for (int i = 1; i < argc; ++i) {
      datadriven::ClearResults(argv[i]);
      std::cout << "Cleared " << argv[i] << ".\n";
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  return 0;
}
