/**
 * @internal
 * @file       ffmpeg_common.hpp
 * @copyright  VCA Technology
 * @~english
 * @brief      Declaration of common ffmpeg helper functions
 */

#include <string>

void FfmpegInit();

std::string AvErrorAsString(int err);
