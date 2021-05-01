/**
 * @internal
 * @file       stream_output.cpp
 * @copyright  VCA Technology
 * @~english
 * @brief      Implementation of the @c StreamOutput class.
 */

#include "stream_output.hpp"

StreamOutput::StreamOutput(unsigned int width, unsigned int height, std::ostream &stream) :
  VideoOutput(width, height),
  stream_(stream) {}

void StreamOutput::PushFrame(const uint8_t *buffer) {
  stream_.write(reinterpret_cast<const char*>(buffer), buffer_size_);
}
