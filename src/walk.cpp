#include <algorithm>
#include <datadriven/datadriven.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace datadriven {
namespace {

struct WalkEntry {
  std::filesystem::directory_entry entry;
  std::string name;
};

bool IsTempFile(std::string_view name) {
  if (name.empty()) {
    return false;
  }
  if (name.front() == '.') {
    return true;
  }
  if (name.back() == '~') {
    return true;
  }
  return name.size() >= 2 && name.front() == '#' && name.back() == '#';
}

} // namespace

void Walk(std::string_view path_view,
          const std::function<void(std::string_view)> &fn) {
  std::filesystem::path root(path_view);
  if (!std::filesystem::exists(root)) {
    throw std::runtime_error("path does not exist: " + root.string());
  }
  if (!std::filesystem::is_directory(root)) {
    fn(root.string());
    return;
  }

  // Iterative DFS; reverse-insert sorted entries so the first lexical entry is
  // popped first from the LIFO stack.
  std::vector<std::filesystem::path> work = {root};
  while (!work.empty()) {
    std::filesystem::path current = std::move(work.back());
    work.pop_back();

    if (!std::filesystem::is_directory(current)) {
      fn(current.string());
      continue;
    }

    std::vector<WalkEntry> entries;
    try {
      for (const auto &entry : std::filesystem::directory_iterator(current)) {
        auto name = entry.path().filename().string();
        if (!IsTempFile(name)) {
          entries.push_back({entry, std::move(name)});
        }
      }
    } catch (const std::filesystem::filesystem_error &e) {
      throw std::runtime_error("failed to read directory " + current.string() +
                               ": " + e.what());
    }

    std::sort(entries.begin(), entries.end(),
              [](const auto &a, const auto &b) { return a.name < b.name; });

    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
      work.push_back(it->entry.path());
    }
  }
}

} // namespace datadriven
