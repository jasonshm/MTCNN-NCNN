#pragma once
/**
 * @internal
 * @file       stream_output.hpp
 * @copyright  VCA Technology
 * @~english
 * @brief      Declaration of the @c StreamOutput class.
 */

#include <ostream>

#include "video_output.hpp"

class StreamOutput final : public VideoOutput {
 public:
  StreamOutput(unsigned int width, unsigned int height, std::ostream &stream);

  void PushFrame(const uint8_t *buffer_data);

 private:
  std::ostream &stream_;
};
