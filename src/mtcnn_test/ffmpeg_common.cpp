/**
 * @internal
 * @file       ffmpeg_common.cpp
 * @copyright  VCA Technology
 * @~english
 * @brief      Implementation of common ffmpeg helper functions
 */

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "ffmpeg_common.hpp"

void FfmpegInit() {
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
  av_register_all();
  avcodec_register_all();
#endif
  avformat_network_init();
}

std::string AvErrorAsString(int err) {
  char err_buf[256];
  av_strerror(err, err_buf, sizeof(err_buf));
  return err_buf;
}
