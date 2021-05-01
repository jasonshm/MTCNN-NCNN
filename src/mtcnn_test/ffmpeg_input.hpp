#pragma once
/**
 * @internal
 * @file       ffmpeg_input.hpp
 * @copyright  VCA Technology
 * @~english
 * @brief      Declaration of the @c FfmpegInput class.
 */

#include <cstdint>
#include <string>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <vca/media/four_cc.hpp>

#include "video_input.hpp"

class FfmpegInput final : public VideoInput {
 public:
  FfmpegInput(const std::string &path, unsigned scaled_width, unsigned scaled_height);

  ~FfmpegInput();

  vca::core::video::Formats GetInputFormats() const;

  vca::core::video::Formats GetOutputFormats() const;

  bool ReadFrame(Frame &frame);

 private:
  bool FindNextFrame();

  size_t GetImageSize() const;

 private:
  static vca::media::FourCc PixelFormatToFourCc(AVPixelFormat pixel_format);

  static vca::media::FrameRate FrameRateFromStream(const AVStream *stream);

 private:
  AVFormatContext *format_context_;
  int stream_index_;
  const AVCodec *codec_;
  AVCodecContext *codec_context_;
  AVFrame *frame_;
  AVPacket packet_;
  bool have_frame_;

  unsigned int prev_scaled_frame_time_offset_;
  unsigned int scaled_width_, scaled_height_;
  SwsContext *scale_context_;

  vca::core::video::Formats input_formats_, output_formats_;

  uint64_t timestamp_;
};
