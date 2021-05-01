#pragma once
/**
 * @internal
 * @file       fake_input.hpp
 * @copyright  VCA Technology
 * @~english
 * @brief      Declaration of the @c TestInput class.
 */

#include <memory>

#include "video_input.hpp"

class TestInput final : public VideoInput {
 private:
  static constexpr const unsigned int ScaledBufferSize = (ScaledWidth * ScaledHeight * 3) / 2;
  static constexpr const vca::media::FrameRate ScaledFrameRate = {15, 1};
  // static constexpr const vca::media::FrameRate ScaledFrameRate = {30, 1};

 public:
  TestInput(unsigned width, unsigned height);

  ~TestInput();

  vca::core::video::Formats GetInputFormats() const;

  vca::core::video::Formats GetOutputFormats() const;

  bool ReadFrame(Frame &frame);

 private:
  unsigned int frame_num_;
  unsigned int width_, height_;
};
