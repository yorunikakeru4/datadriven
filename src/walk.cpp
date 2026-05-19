#include <datadriven/datadriven.hpp>

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace datadriven {
namespace {

bool IsTempFile(const std::filesystem::path& path) {
  const auto name = path.filename().string();
  if (name.empty()) return false;
  if (name.front() == '.') return true;
  if (name.back() == '~') return true;
  return name.size() >= 2 && name.front() == '#' && name.back() == '#';
}

}  // namespace

void Walk(std::string_view path_view, const std::function<void(std::string_view)>& fn) {
  std::filesystem::path path(path_view);
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("path does not exist: " + path.string());
  }
  if (!std::filesystem::is_directory(path)) {
    const auto s = path.string();
    fn(s);
    return;
  }

  std::vector<std::filesystem::directory_entry> entries;
  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    if (!IsTempFile(entry.path())) entries.push_back(entry);
  }
  std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
    return a.path().filename().string() < b.path().filename().string();
  });

  for (const auto& entry : entries) {
    const auto child = entry.path().string();
    Walk(child, fn);
  }
}

}  // namespace datadriven
