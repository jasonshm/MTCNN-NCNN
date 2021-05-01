/**
 * @internal
 * @file       subprocess_output.cpp
 * @copyright  VCA Technology
 * @~english
 * @brief      Implementation of the @c SubprocessOutput class.
 */

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "subprocess_output.hpp"

static SubprocessOutput::Process Spawn(const std::string app, const std::vector<std::string> &args) {
#ifdef _WIN32
  throw std::logic_error("Subprocess-based outputs are not yet supported in Windows");
#else
  int in_pipe[2];

  if (pipe(in_pipe) < 0)
    throw std::runtime_error("Pipe error");

  const int pid = fork();
  if (pid < 0) {
    throw std::runtime_error("Fork error");
  } else if (pid == 0) {
    close(in_pipe[1]);

    try {
      if (in_pipe[0] != STDIN_FILENO) {
        if (dup2(in_pipe[0], STDIN_FILENO) != STDIN_FILENO)
          throw std::runtime_error("dup2 error");
        close(in_pipe[0]);
      }

      const int dev_null = open("/dev/null", 0);
      if (dev_null == -1)
        throw std::runtime_error("Failed to open /dev/null");
      if (dev_null != STDOUT_FILENO && dup2(dev_null, STDOUT_FILENO) != STDOUT_FILENO)
        throw std::runtime_error("dup2 error");
      if (dev_null != STDERR_FILENO && dup2(dev_null, STDERR_FILENO) != STDERR_FILENO)
        throw std::runtime_error("dup2 error");

      std::vector<char*> argv(args.size() + 2);
      *(argv.begin()) = strdup(app.c_str());
      std::transform(args.cbegin(), args.cend(), argv.begin() + 1,
          [](const std::string &s) { return strdup(s.c_str()); });
      *(argv.end() - 1) = nullptr;

      if (execvp(app.c_str(), argv.data()) < 0) {
        std::ostringstream ss;
        ss << "Failed to start " << app;
        throw std::runtime_error(ss.str());
      }
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  close(in_pipe[0]);
  return {pid, in_pipe[1]};
#endif
}

SubprocessOutput::SubprocessOutput(unsigned int width, unsigned int height,
    const std::string &app, const std::vector<std::string> &args) :
  VideoOutput(width, height),
  process_(Spawn(app, args)) {}

SubprocessOutput::~SubprocessOutput() {
#ifndef _WIN32
  close(process_.in_fd);

  int status;
  waitpid(process_.pid, &status, 0);
#endif
}

void SubprocessOutput::PushFrame(const uint8_t *buffer_data) {
#ifndef _WIN32
  if (write(process_.in_fd, buffer_data, buffer_size_) != static_cast<int>(buffer_size_))
    throw std::runtime_error("Failed to push output frame");
#endif
}
