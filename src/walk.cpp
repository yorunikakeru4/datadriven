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
  std::filesystem::path path(path_view);
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("path does not exist: " + path.string());
  }
  if (!std::filesystem::is_directory(path)) {
    const auto s = path.string();
    fn(s);
    return;
  }

  std::vector<WalkEntry> entries;
  for (const auto &entry : std::filesystem::directory_iterator(path)) {
    auto name = entry.path().filename().string();
    if (!IsTempFile(name)) {
      entries.push_back({entry, std::move(name)});
    }
  }
  std::sort(entries.begin(), entries.end(),
            [](const auto &a, const auto &b) { return a.name < b.name; });

  for (const auto &entry : entries) {
    const auto child = entry.entry.path().string();
    Walk(child, fn);
  }
}

} // namespace datadriven
