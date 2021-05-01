#pragma once
/**
 * @internal
 * @file       subprocess_output.hpp
 * @copyright  VCA Technology
 * @~english
 * @brief      Declaration of the @c SubprocessOutput class.
 */

#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "video_output.hpp"

class SubprocessOutput final : public VideoOutput {
 public:
  struct Process {
#ifdef _WIN32
    PROCESS_INFORMATION process_information;
#else
    int pid;
    int in_fd;
#endif
  };

 public:
  SubprocessOutput(unsigned int width, unsigned int height,
      const std::string &app, const std::vector<std::string> &args);

  virtual ~SubprocessOutput();

  void PushFrame(const uint8_t *buffer_data);

 private:
  Process process_;
};
