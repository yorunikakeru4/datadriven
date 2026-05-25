#pragma once

#include <filesystem>
#include <system_error>

#if defined(_WIN32)
#include <process.h>
#define DATADRIVEN_PROCESS_ID() _getpid()
#else
#include <unistd.h>
#define DATADRIVEN_PROCESS_ID() getpid()
#endif

namespace datadriven::internal {

void ReplaceFile(const std::filesystem::path &from,
                 const std::filesystem::path &to, std::error_code &ec);

} // namespace datadriven::internal
