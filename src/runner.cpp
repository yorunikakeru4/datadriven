#include <datadriven/datadriven.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <unistd.h>

#include "test_data_reader.hpp"
#include <datadriven/internal/string_util.hpp>

namespace datadriven {

std::string CmdArg::ToString() const {
  if (vals.empty()) {
    return key;
  }
  if (vals.size() == 1) {
    return key + "=" + vals[0];
  }
  std::ostringstream out;
  out << key << "=(";
  for (std::size_t i = 0; i < vals.size(); ++i) {
    if (i != 0) {
      out << ", ";
    }
    out << vals[i];
  }
  out << ")";
  return out.str();
}

void CmdArg::ExpectNumVals(std::size_t n) const {
  if (vals.size() != n) {
    throw std::runtime_error("argument " + key + " requires " +
                             std::to_string(n) + " values, has " +
                             std::to_string(vals.size()));
  }
}

void CmdArg::ExpectNumValsGE(std::size_t n) const {
  if (vals.size() < n) {
    throw std::runtime_error("argument " + key + " requires at least " +
                             std::to_string(n) + " values, has " +
                             std::to_string(vals.size()));
  }
}

std::string CmdArg::FirstVal() const {
  ExpectNumValsGE(1);
  return vals[0];
}

std::string CmdArg::SingleVal() const {
  ExpectNumVals(1);
  return vals[0];
}

std::pair<std::string, std::string> CmdArg::TwoVals() const {
  ExpectNumVals(2);
  return {vals[0], vals[1]};
}

std::string TestData::FullCmd() const {
  std::ostringstream out;
  out << cmd;
  for (const auto &arg : cmd_args) {
    out << " " << arg.ToString();
  }
  return out.str();
}

std::string TestData::ToString() const {
  if (input.empty()) {
    return FullCmd() + "\n";
  }
  return FullCmd() + "\n" + input + "\n";
}

const CmdArg *TestData::Arg(std::string_view key) const {
  for (const auto &arg : cmd_args) {
    if (arg.key == key) {
      return &arg;
    }
  }
  return nullptr;
}

bool TestData::HasArg(std::string_view key) const {
  return Arg(key) != nullptr;
}

namespace {

std::string UnifiedFailure(const TestData &d, std::string_view actual) {
  std::ostringstream out;
  out << "\n" << d.pos << ":\n";
  out << internal::IndentLines(d.ToString()) << "\n";
  out << "expected:\n" << internal::IndentLines(d.expected) << "\n";
  out << "found:\n" << internal::IndentLines(actual);
  return out.str();
}

void EmitLines(internal::TestDataReader &reader, std::string_view text) {
  std::istringstream lines{std::string(text)};
  std::string line;
  while (std::getline(lines, line)) {
    reader.Emit(line);
  }
}

void EmitActual(internal::TestDataReader &reader, std::string_view actual) {
  reader.Emit("----");
  if (internal::HasWhitespaceOnlyLine(actual)) {
    reader.Emit("----");
    EmitLines(reader, actual);
    reader.Emit("----");
    reader.Emit("----");
    reader.Emit("");
    return;
  }
  EmitLines(reader, actual);
  reader.Emit("");
}

std::string RunTestInternal(std::string_view source_name, std::istream &input,
                            const Handler &handler, Options options) {
  internal::TestDataReader reader(std::string(source_name), input,
                                  options.rewrite);
  std::vector<std::string> subtests;
  while (auto td = reader.Next()) {
    if (td->cmd == "subtest") {
      if (td->cmd_args.empty()) {
        throw std::runtime_error(td->pos + ": invalid syntax for subtest");
      }
      const std::string name = td->cmd_args[0].key;
      if (name == "end") {
        if (subtests.empty()) {
          throw std::runtime_error(td->pos +
                                   ": subtest end without corresponding start");
        }
        if (td->cmd_args.size() == 2 &&
            td->cmd_args[1].key != subtests.back()) {
          throw std::runtime_error(td->pos +
                                   ": mismatched subtest end directive");
        }
        subtests.pop_back();
        continue;
      }
      if (!subtests.empty()) {
        const std::string prefix = subtests.back() + "/";
        if (name.rfind(prefix, 0) != 0) {
          throw std::runtime_error(
              td->pos + ": name of nested subtest must begin with " + prefix);
        }
      }
      subtests.push_back(name);
      continue;
    }

    std::string actual = handler(*td);
    if (!actual.empty() && actual.back() != '\n') {
      actual.push_back('\n');
    }
    if (options.rewrite) {
      EmitActual(reader, actual);
      continue;
    }
    if (td->expected != actual) {
      throw std::runtime_error(UnifiedFailure(*td, actual));
    }
  }
  if (!subtests.empty()) {
    throw std::runtime_error("EOF encountered without subtest end directive");
  }

  std::string rewrite = reader.RewriteOutput();
  if (rewrite.size() > 2 && rewrite[rewrite.size() - 1] == '\n' &&
      rewrite[rewrite.size() - 2] == '\n') {
    rewrite.pop_back();
  }
  return rewrite;
}

std::filesystem::path RewriteTempPath(const std::filesystem::path &path) {
  const std::filesystem::path parent =
      path.parent_path().empty() ? "." : path.parent_path();
  return parent / (path.filename().string() + ".rewrite." +
                   std::to_string(static_cast<long>(::getpid())) + ".tmp");
}

void WriteFileAtomically(std::string_view path_text,
                         std::string_view contents) {
  const std::filesystem::path path{std::string(path_text)};
  std::error_code ec;
  const auto original_status = std::filesystem::status(path, ec);
  if (ec) {
    throw std::runtime_error("failed to stat " + path.string() + ": " +
                             ec.message());
  }

  const auto temp = RewriteTempPath(path);

  try {
    std::ofstream out(temp, std::ios::trunc);
    out.exceptions(std::ios::badbit | std::ios::failbit);
    out << contents;
    out.close();
  } catch (const std::ios_base::failure &e) {
    std::filesystem::remove(temp, ec);
    throw std::runtime_error("failed to write " + temp.string() + ": " +
                             e.what());
  }

  if (std::filesystem::is_regular_file(original_status)) {
    std::filesystem::permissions(temp, original_status.permissions(), ec);
    if (ec) {
      std::filesystem::remove(temp, ec);
      throw std::runtime_error("failed to set permissions on " + temp.string() +
                               ": " + ec.message());
    }
  }

  std::filesystem::rename(temp, path, ec);
  if (ec) {
    std::filesystem::remove(temp, ec);
    throw std::runtime_error("failed to replace " + path.string() + ": " +
                             ec.message());
  }
}

} // namespace

void RunTestFromString(std::string_view input, Handler handler,
                       Options options) {
  if (options.rewrite) {
    throw std::runtime_error("RunTestFromString does not support rewrite mode");
  }
  std::istringstream stream{std::string(input)};
  RunTestInternal("<string>", stream, std::move(handler), options);
}

void RunTest(std::string_view path, Handler handler, Options options) {
  if (std::filesystem::is_directory(path)) {
    throw std::runtime_error(
        std::string(path) +
        " is a directory, not a file; consider using datadriven::Walk");
  }
  std::ifstream file{std::string(path)};
  if (!file) {
    throw std::runtime_error("failed to open " + std::string(path));
  }
  std::string rewrite =
      RunTestInternal(path, file, std::move(handler), options);
  if (options.rewrite) {
    WriteFileAtomically(path, rewrite);
  }
}

void ClearResults(std::string_view path) {
  RunTest(
      path, [](const TestData &) { return std::string{}; },
      Options{.rewrite = true});
}

} // namespace datadriven
