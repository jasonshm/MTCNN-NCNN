#pragma once
/**
 * @internal
 * @file       video_output.hpp
 * @copyright  VCA Technology
 * @~english
 * @brief      Declaration of the @c VideoOutput class.
 */

#include <cstddef>
#include <cstdint>

class VideoOutput {
 public:
  VideoOutput(unsigned int width, unsigned int height);

  virtual ~VideoOutput();

  virtual void PushFrame(const uint8_t *buffer_data) = 0;

 protected:
  size_t buffer_size_;
};
