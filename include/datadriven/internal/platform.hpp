#pragma once

#include <filesystem>
#include <system_error>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <process.h>
#include <windows.h>
#define DATADRIVEN_PROCESS_ID() _getpid()
#else
#include <unistd.h>
#define DATADRIVEN_PROCESS_ID() getpid()
#endif

namespace datadriven::internal {

inline void ReplaceFile(const std::filesystem::path &from,
                        const std::filesystem::path &to, std::error_code &ec) {
#if defined(_WIN32)
  if (MoveFileExW(from.c_str(), to.c_str(),
                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0) {
    ec.clear();
    return;
  }
  ec =
      std::error_code(static_cast<int>(GetLastError()), std::system_category());
#else
  std::filesystem::rename(from, to, ec);
#endif
}

} // namespace datadriven::internal
