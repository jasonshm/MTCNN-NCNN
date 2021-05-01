/**
 * @internal
 * @file       video_output.cpp
 * @copyright  VCA Technology
 * @~english
 * @brief      Implementation of the @c VideoOutput class.
 */

#include "video_output.hpp"

VideoOutput::VideoOutput(unsigned int width, unsigned int height) :
  buffer_size_((3 * width * height) / 2) {}

VideoOutput::~VideoOutput() {}
