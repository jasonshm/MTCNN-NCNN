#pragma once
/**
 * @internal
 * @file       video_input.hpp
 * @copyright  VCA Technology
 * @~english
 * @brief      Declaration of the @c VideoInput class.
 */

#include <cstddef>
#include <cstdint>
#include <utility>

#include <vca/core_sdk/video/buffers.hpp>
#include <vca/core_sdk/video/formats.hpp>
#include <vca/media/frame_rate.hpp>

class VideoInput {
 public:
  static constexpr const unsigned int BufferTypeCount = 3;

  static constexpr const vca::media::FrameRate ScaledFrameRate = {15, 1}; // fps
  #if 0
  static constexpr const unsigned int ScaledWidth = 360;
  static constexpr const unsigned int ScaledHeight = 240;
  #elif 0
  static constexpr const unsigned int ScaledWidth = 720;
  static constexpr const unsigned int ScaledHeight = 480;
  #elif 1
  static constexpr const unsigned int ScaledWidth = 960;
  static constexpr const unsigned int ScaledHeight = 540;
  #elif 0
  static constexpr const unsigned int ScaledWidth = 1280;
  static constexpr const unsigned int ScaledHeight = 720;
  #else
  static constexpr const unsigned int ScaledWidth = 1920;
  static constexpr const unsigned int ScaledHeight = 1080;
  #endif

 public:
  struct Frame {
    vca::core::video::Buffers input_buffers, output_buffers;
    uint64_t timestamp;
  };

 protected:
  VideoInput();

 private:
  VideoInput(VideoInput &&src);

  VideoInput(const VideoInput&) = delete;

 public:
  virtual ~VideoInput();

  virtual vca::core::video::Formats GetInputFormats() const = 0;

  virtual vca::core::video::Formats GetOutputFormats() const = 0;

  virtual bool ReadFrame(Frame &frame) = 0;

 private:
  VideoInput& operator=(const VideoInput&) = delete;
  VideoInput& operator=(VideoInput&&) = delete;
};
