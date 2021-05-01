/**
 * @internal
 * @file       test_input.cpp
 * @copyright  VCA Technology
 * @~english
 * @brief      Implementation of the @c TestInput class.
 */

#define _USE_MATH_DEFINES

#include <cmath>
#include <cstdlib>
#include <cstring>

#include <vca/media/four_cc.hpp>

#include "test_input.hpp"

struct TestObject {
  int x, y, size;
};

static void GenerateFrameData(uint8_t *buffer, unsigned int width, unsigned int height, std::ptrdiff_t stride,
    TestObject object) {
  // Fill the background with grey noise
  std::srand(123);
  for (unsigned int y = 0; y != height; y++)
    for (unsigned int x = 0; x != width; x++)
      buffer[y * stride + x] = std::rand() % 160;
  std::memset(buffer + height * stride, 127, stride * height / 2);

  // Draw a black object on top
  const unsigned int s = static_cast<unsigned int>(std::max(0, object.size));
  const unsigned int t = static_cast<unsigned int>(std::max(0, object.y));
  const unsigned int b = static_cast<unsigned int>(std::min(static_cast<int>(height), object.y + object.size));
  const unsigned int l = static_cast<unsigned int>(std::max(0, object.x));
  const unsigned int w = std::min(s, width - l);
  if (object.size > 0)
    for(unsigned int y = t; y < b; y++)
      std::memset(buffer + y * stride + l, 0, w);
}

TestInput::TestInput(unsigned int width, unsigned int height) :
  frame_num_(0),
  width_(width),
  height_(height) {
  if (width_ == 0 || height_ == 0) {
    width_ = ScaledWidth;
    height_ = ScaledHeight;
  }
}

TestInput::~TestInput() {}

vca::core::video::Formats TestInput::GetInputFormats() const {
  return vca::core::video::Formats{
      vca::core::video::Format{vca::media::four_cc::FourCcs::NV12,
        ScaledFrameRate, width_, height_}
    };
}

vca::core::video::Formats TestInput::GetOutputFormats() const {
  return vca::core::video::Formats{};
}

bool TestInput::ReadFrame(Frame &frame) {
  frame.timestamp = static_cast<uint64_t>(frame_num_) * (1000000000UL * ScaledFrameRate.den) / ScaledFrameRate.num;

  const unsigned int buffer_size = (width_ * height_ * 3) / 2;

  // Create an image with object moving in a looping pattern in the foreground
  uint8_t *const data = new uint8_t[buffer_size];
  const int object_size = 64;
  GenerateFrameData(data, width_, height_, width_, {
      static_cast<int>(cosf(M_PI * frame_num_ / 200.0f) * width_ * 0.25f + width_ / 2 - object_size / 2),
      static_cast<int>(sinf(M_PI * frame_num_ / 100.0f) * height_ * 0.25f + height_ / 2 - object_size / 2),
      object_size});

  frame.input_buffers = vca::core::video::Buffers{vca::core::video::Buffer{buffer_size, data, 1,
      {vca::core::video::Buffer::Plane{buffer_size, 0, static_cast<ptrdiff_t>(width_)}},
      [data]() { delete[] data; }}};
  frame.output_buffers = vca::core::video::Buffers{};

  frame_num_++;

  return true;
}
