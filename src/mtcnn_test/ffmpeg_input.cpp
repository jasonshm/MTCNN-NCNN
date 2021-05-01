/**
 * @internal
 * @file       ffmpeg_input.cpp
 * @copyright  VCA Technology
 * @~english
 * @brief      Implementation of the @c FfmpegInput class.
 */

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <limits>

extern "C" {
#include <libavutil/imgutils.h>
}

#include "ffmpeg_common.hpp"
#include "ffmpeg_input.hpp"

FfmpegInput::FfmpegInput(const std::string &path, unsigned scaled_width, unsigned scaled_height) :
  format_context_(nullptr),
  have_frame_(false),
  prev_scaled_frame_time_offset_(0),
  scaled_width_(scaled_width),
  scaled_height_(scaled_height),
  input_formats_(),
  output_formats_(),
  timestamp_(0) {
  int ret;

#if 0
  av_log_set_level(AV_LOG_TRACE);
#endif

  AVDictionary *options = NULL;
  if ((ret = av_dict_set(&options, "buffer_size", "262144", 0)) < 0)
    throw std::runtime_error("Failed to set options");

  ret = avformat_open_input(&format_context_, path.c_str(), NULL, &options);
  if (ret < 0) {
    std::ostringstream ss;
    ss << "Could not open '" << path << "'" << std::endl << AvErrorAsString(ret) << std::endl;
    throw std::runtime_error(ss.str());
  }

  av_dict_free(&options);

  if (avformat_find_stream_info(format_context_, NULL) < 0)
    throw std::runtime_error("Could not find stream information");

  stream_index_ = av_find_best_stream(format_context_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  if (stream_index_ < 0) {
    std::ostringstream ss;
    ss << "Could not find video stream in input file '" << path << "'" << std::endl;
    throw std::runtime_error(ss.str());
  }

  AVStream *const stream = format_context_->streams[stream_index_];

  codec_ = avcodec_find_decoder(stream->codecpar->codec_id);
  if (!codec_)
    throw std::runtime_error("Codec not found");

  codec_context_ = avcodec_alloc_context3(codec_);
  if (!codec_)
    throw std::runtime_error("Failed to allocate video codec context");

  if (avcodec_parameters_to_context(codec_context_, stream->codecpar) < 0)
    throw std::runtime_error("Failed to copy video codec parameters to decoder context");
  const uint32_t codec_tag = codec_context_->codec_tag ? codec_context_->codec_tag : MKTAG('A', 'V', 'C', '1');

  if (avcodec_open2(codec_context_, codec_, NULL) < 0)
    throw std::runtime_error("Failed to open codec");

  const vca::media::FrameRate framerate = FrameRateFromStream(stream);

  frame_ = av_frame_alloc();
  if (!frame_)
    throw std::runtime_error("Failed to allocate video frame");

  av_init_packet(&packet_);
  packet_.data = nullptr;
  packet_.size = 0;

  // Read the first frame
  if (!FindNextFrame())
    throw std::runtime_error("Failed to read first frame");

  // Set up the scaler
  if (scaled_width_ == 0 || scaled_height_ == 0) {
    const unsigned int scale_width = (codec_context_->width * 1024) / ScaledWidth;
    const unsigned int scale_height = (codec_context_->height * 1024) / ScaledHeight;
    if (scale_width > scale_height) {
      scaled_width_ = ScaledWidth;
      scaled_height_ = ((codec_context_->height * 1024) / scale_width) & ~0x01;
    } else {
      scaled_width_ = ((codec_context_->width * 1024) / scale_height) & ~0x01;
      scaled_height_ = ScaledHeight;
    }
  } else {
    if ((scaled_width_ & 0x1) || (scaled_height_ & 0x1)) {
      scaled_width_ &= ~0x01;
      scaled_height_ &= ~0x01;
      std::cerr << "Note: Adjusted image size to " << scaled_width_ << 'x' << scaled_height_ << std::endl;
    }
  }

  if (scaled_width_ == 0 || scaled_height_ == 0) {
    std::ostringstream ss;
    ss << "Invalid scaled image size " << scaled_width_ << "x" << scaled_height_ << std::endl;
    throw std::runtime_error(ss.str());
  }

  scale_context_ = sws_getContext(codec_context_->width, codec_context_->height, codec_context_->pix_fmt,
      scaled_width_, scaled_height_, AV_PIX_FMT_NV12, SWS_BILINEAR, NULL, NULL, NULL);

  input_formats_ = vca::core::video::Formats{
      vca::core::video::Format{
          PixelFormatToFourCc(codec_context_->pix_fmt), framerate, static_cast<unsigned int>(codec_context_->width),
          static_cast<unsigned int>(codec_context_->height)},
      vca::core::video::Format{vca::media::four_cc::NV12, ScaledFrameRate, scaled_width_, scaled_height_}
    };
  output_formats_ = vca::core::video::Formats{
      vca::core::video::Format{
          codec_tag, framerate, static_cast<unsigned int>(codec_context_->width),
          static_cast<unsigned int>(codec_context_->height)},
    };
}

FfmpegInput::~FfmpegInput() {
  av_frame_free(&frame_);
  avcodec_free_context(&codec_context_);
  avformat_close_input(&format_context_);
  if (packet_.data)
    av_packet_unref(&packet_);
  sws_freeContext(scale_context_);
}

vca::core::video::Formats FfmpegInput::GetInputFormats() const {
  return input_formats_;
}

vca::core::video::Formats FfmpegInput::GetOutputFormats() const {
  return output_formats_;
}

bool FfmpegInput::ReadFrame(Frame &frame) {
  if (!have_frame_ && !FindNextFrame())
    return false;

  const AVStream *const stream = format_context_->streams[stream_index_];
  vca::media::FrameRate framerate = FrameRateFromStream(stream);

  // fix frame rate if the video has too high rate
  if ((framerate.num / framerate.den) > 100.) {
    framerate.num = 30;
    framerate.den = 1;
  }

  // Calculate the timestamp
  if (frame_->pts != std::numeric_limits<decltype(frame_->pts)>::min()) {
    const AVRational nano_second_time_base = {1, 1000000000};
    timestamp_ = av_rescale_q(frame_->pts, stream->time_base, nano_second_time_base);
  }
  frame.timestamp = timestamp_;

  // Calculate the relative time of the frame within the 15-FPS scaled frame
  const bool drop_scaled_frames = framerate.den == 0 ? false :
      (((65536 * framerate.num * ScaledFrameRate.den) / framerate.den) > (ScaledFrameRate.num * 65536));
  const AVRational scaled_time_base = {ScaledFrameRate.den, ScaledFrameRate.num * 65536};
  const unsigned int scaled_frame_time_offset =
      av_rescale_q(frame_->pts, stream->time_base, scaled_time_base) % 65536;

  // Make the coded buffer
  AVPacket *const packet = av_packet_clone(&packet_);
  vca::core::video::Buffer coded_buffer{
      static_cast<size_t>(packet->size),
      packet->data,
      1,
      { vca::core::video::Buffer::Plane{static_cast<size_t>(packet->size), 0, 0} },
      [packet]() {
        AVPacket *ptr = packet;
        av_packet_free(&ptr);
      }};

  // Make the decoded buffer
  AVFrame *const frame_ref = av_frame_clone(frame_);

  vca::core::video::Buffer::Planes planes = {};
  size_t plane_count = 0;
  uint8_t *base = reinterpret_cast<uint8_t*>(~0ULL), *end = 0;

  for (int i = 0; frame_ref->buf[i] && i != AV_NUM_DATA_POINTERS; i++) {
    const auto size = frame_ref->buf[i]->size;
    const auto data = frame_ref->data[i];
    if (size && data) {
      base = std::min(base, data);
      end = std::max(end, data + size);
    }
  }

  for (int i = 0; frame_ref->buf[i] && i != AV_NUM_DATA_POINTERS; i++) {
    const auto size = frame_ref->buf[i]->size;
    const auto data = frame_ref->data[i];
    if (size && data)
      planes[plane_count++] = {static_cast<size_t>(size), static_cast<size_t>(data - base), frame_ref->linesize[i]};
  }

  vca::core::video::Buffer decoded_buffer{static_cast<size_t>(end - base), base, plane_count, std::move(planes),
      [frame_ref]() {
        AVFrame *ptr = frame_ref;
        av_frame_free(&ptr);
      }};

  // Make the scaled image
  vca::core::video::Buffer scaled_buffer{0, nullptr, 0, {}, {}};
  if (!drop_scaled_frames || scaled_frame_time_offset < prev_scaled_frame_time_offset_) {
    const size_t scaled_y_plane_size = scaled_width_ * scaled_height_;
    const size_t scaled_image_size = (scaled_y_plane_size * 3) / 2;
    uint8_t *const scaled_data = new uint8_t[scaled_image_size];
    uint8_t *const dst_planes[] = {scaled_data, scaled_data + scaled_y_plane_size};
    const int dst_strides[] = {static_cast<int>(scaled_width_), static_cast<int>(scaled_width_)};
    sws_scale(scale_context_, frame_->data, frame_->linesize, 0, frame_->height, dst_planes, dst_strides);

    scaled_buffer = vca::core::video::Buffer{scaled_image_size, scaled_data, 2, {
        vca::core::video::Buffer::Plane{scaled_y_plane_size, 0, static_cast<std::ptrdiff_t>(scaled_width_)},
        vca::core::video::Buffer::Plane{
          scaled_y_plane_size / 2, scaled_y_plane_size, static_cast<std::ptrdiff_t>(scaled_width_)}
      },
      [scaled_data]() { delete[] scaled_data; }};
  }

  prev_scaled_frame_time_offset_ = scaled_frame_time_offset;

  have_frame_ = false;

  frame.input_buffers = vca::core::video::Buffers{std::move(decoded_buffer), std::move(scaled_buffer)};
  frame.output_buffers = vca::core::video::Buffers{std::move(coded_buffer)};

  return true;
}

bool FfmpegInput::FindNextFrame() {
  int ret;

  for (;;) {
    if (packet_.data)
      av_packet_unref(&packet_);

    if (av_read_frame(format_context_, &packet_) < 0)
      return false;

    if (packet_.stream_index == stream_index_) {
      ret = avcodec_send_packet(codec_context_, &packet_);
      if (ret >= 0) {
        ret = avcodec_receive_frame(codec_context_, frame_);
        if (ret >= 0) {
          have_frame_ = true;
          return true;
        } else if (ret != AVERROR(EAGAIN)) {
          return false;
        }
      } if (ret != AVERROR(EAGAIN)) {
        return false;
      }
    }
  }
}

size_t FfmpegInput::GetImageSize() const {
  size_t image_size = 0;
  assert(have_frame_);
  for (int i = 0; frame_->buf[i] && i != AV_NUM_DATA_POINTERS; i++)
    image_size += frame_->buf[i]->size;
  return image_size;
}

vca::media::FourCc FfmpegInput::PixelFormatToFourCc(AVPixelFormat pixel_format) {
  switch (pixel_format) {
   case AV_PIX_FMT_YUV420P:
   case AV_PIX_FMT_YUVJ420P:
    return MKTAG('Y','V','1','6');
   default: {
      std::ostringstream ss;
      ss << "No known FourCC for " << av_get_pix_fmt_name(pixel_format) << " pixel format";
      throw std::runtime_error(ss.str());
      return 0;
    }
  }
}

vca::media::FrameRate FfmpegInput::FrameRateFromStream(const AVStream *stream) {
  return vca::media::FrameRate{
    static_cast<unsigned int>(stream->avg_frame_rate.num),
    static_cast<unsigned int>(stream->avg_frame_rate.den)
  };
}
