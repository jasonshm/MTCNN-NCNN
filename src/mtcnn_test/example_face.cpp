/**
 * @file      example_face.cpp
 * @copyright UDP Technology Ltd.
 * @~english
 * @brief     Example program for Face detection
 */

#define _USE_MATH_DEFINES

#include <cmath>
#include <cstring>
#include <future>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <string>
#include <string_view>

#include <sys/time.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#ifdef WITH_FFMPEG
#include "ffmpeg_common.hpp"
#include "ffmpeg_input.hpp"
#endif

#include "mtcnn.h"
#include "stream_output.hpp"
#include "subprocess_output.hpp"
#include "test_input.hpp"

static const constexpr char TestInputUri[] = "test://";
static const constexpr int max_frames_to_play = 180*30;

static unsigned int width = 0, height = 0;
static bool output_json = false;
static bool output_xml = false;
static bool print_events = false;
static unsigned int frame_no = 0;

static double get_current_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static void Usage(std::ostream &o, const char *argv0) {
  o << "Usage: " << argv0 << " [OPTIONS...] [FILE]\n"
    "\n"
    "Test C++ test for VCA Core Embedded\n"
    "\n"
    "  FILE               The path to a file to load, or test:// to generate a\n"
    "                     test video stream\n"
    "\n"
    "Options:\n"
    "  -s,--size WxH      Specifies the size of the input image\n"
    "  --json             Outputs VCA meta-data in VCA JSON format\n"
    "  --xml              Outputs VCA meta-data in VCA XML format\n"
    "  --print-events     Prints events\n"
    "  --video-output {ffplay,mplayer,stdout}\n"
    "                     Show video using specified method.\n"
    "\n"
    "\n";
}

static std::unique_ptr<VideoInput> CreateVideoInput(const std::string &path) {
  if (path == TestInputUri)
    return std::unique_ptr<VideoInput>(new TestInput(width, height));
#ifdef WITH_FFMPEG
  return std::unique_ptr<VideoInput>(new FfmpegInput(path, width, height));
#else
  throw std::runtime_error("Unsupported input");
#endif
}

static std::unique_ptr<VideoOutput> CreateVideoOutput(
    const std::string &method, const vca::core::video::Format &format) {
  if (method == "ffplay") {
    std::ostringstream size_ss;
    size_ss << format.width << 'x' << format.height;
    return std::unique_ptr<VideoOutput>(new SubprocessOutput(format.width, format.height, "ffplay",
        {"-f", "rawvideo", "-pixel_format", "nv12", "-video_size", size_ss.str(), "-"}));
  } else if (method == "mplayer") {
    std::ostringstream rawvideo_arg_ss;
    rawvideo_arg_ss << "w=" << format.width << ":h=" << format.height << ":format=nv12";
    return std::unique_ptr<VideoOutput>(new SubprocessOutput(format.width, format.height, "mplayer",
        {"-demuxer", "rawvideo", "-rawvideo", rawvideo_arg_ss.str(), "-"}));
  } else if (method == "stdout") {
    return std::unique_ptr<VideoOutput>(new StreamOutput(format.width, format.height, std::cout));
  } else if (method == "none") {
    return std::unique_ptr<VideoOutput>();
  }

  std::ostringstream ss;
  ss << "Unsupported video output method: " << method;
  throw std::runtime_error(ss.str());
}

static void PrintFormats(const std::string &heading, const vca::core::video::Formats &formats) {
  std::cerr << heading << ':' << std::endl;
  for (const auto &f : formats) {
    std::cerr << "    " << vca::media::four_cc::ToString(f.four_cc) << ' ' <<
        f.frame_rate.num;
    if (f.frame_rate.den != 1)
      std::cerr << '/'<< f.frame_rate.den;
    std::cerr << "-fps";
    if (f.width || f.height)
      std::cerr << ' ' << f.width << 'x' << f.height;
    std::cerr << std::endl;
  }

  std::cerr << std::endl;
}

static void UnreferenceFrame(VideoInput::Frame& frame) {
  for (int i = 0; i != static_cast<int>(frame.input_buffers.size()); i++) {
    if (frame.input_buffers[i].unreference)
      frame.input_buffers[i].unreference();
  }
  for (int i = 0; i != static_cast<int>(frame.output_buffers.size()); i++) {
    if (frame.output_buffers[i].unreference)
      frame.output_buffers[i].unreference();
  }
}

static void swapYUV_I420toNV12(unsigned char* i420bytes, unsigned char* nv12bytes, int width, int height) {
  int nLenY = width * height;
  int nLenU = nLenY / 4;

  memcpy(nv12bytes, i420bytes, width * height);

  for (int i = 0; i < nLenU; i++) {
    nv12bytes[nLenY + 2 * i] = i420bytes[nLenY + i];             // U
    nv12bytes[nLenY + 2 * i + 1] = i420bytes[nLenY + nLenU + i]; // V
  }
}

static void BGR2YUV_nv12(cv::Mat src, cv::Mat &dst) {
  int w_img = src.cols;
  int h_img = src.rows;
  dst = cv::Mat(h_img * 1.5, w_img, CV_8UC1, cv::Scalar(0));
  cv::Mat src_YUV_I420(h_img * 1.5, w_img, CV_8UC1, cv::Scalar(0)); //YUV_I420
  cv::cvtColor(src, src_YUV_I420, CV_BGR2YUV_I420);
  swapYUV_I420toNV12(src_YUV_I420.data, dst.data, w_img, h_img);
}

int main(int argc, const char *const *const argv) {
  static constexpr const vca::media::FrameRate frame_rate = {15, 1};  // FPS

  static constexpr const auto frame_duration = std::chrono::nanoseconds(std::chrono::seconds(1) * frame_rate.den) /
      frame_rate.num;

  int arg;
  std::unique_ptr<uint8_t[]> bia_buffer;
  std::string path = TestInputUri;
  std::string output_video_method = "none";

#ifdef WITH_FFMPEG
  // Init ffmpeg
  FfmpegInit();
#endif

  // Parse the arguments
  if (argc == 2 && (std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0)) {
    Usage(std::cout, argv[0]);
    return EXIT_SUCCESS;
  }

  for (arg = 1; arg != argc; arg++) {
    if (std::strcmp(argv[arg], "-s") == 0 || std::strcmp(argv[arg], "--size") == 0) {
      if (arg + 1 == argc) {
        std::cerr << "No image size specified";
        return EXIT_FAILURE;
      } else {
        unsigned int w, h;
        const char *size = argv[++arg];
        if (std::sscanf(size, "%u x %u", &w, &h) == 2) {
          width = w;
          height = h;
        } else {
          std::cerr << "Failed to parse image size argument: " << size << std::endl;
          return EXIT_FAILURE;
        }
      }
    } else if (std::strcmp(argv[arg], "--json") == 0) {
      output_json = true;
    } else if (std::strcmp(argv[arg], "--xml") == 0) {
      output_xml = true;
    } else if (std::strcmp(argv[arg], "--video-output") == 0) {
      if (arg + 1 == argc) {
        std::cerr << "No video output method specified";
        return EXIT_FAILURE;
      } else {
        output_video_method = argv[++arg];
      }
    } else if (std::strcmp(argv[arg], "--print-events") == 0) {
          print_events = true;
    } else {
      break;
    }
  }

  if (arg < argc)
    path = argv[arg++];

  if (arg < argc) {
    std::cerr << "Unexpected option: " << argv[arg] << std::endl;
    Usage(std::cerr, argv[0]);
    return EXIT_FAILURE;
  }

  // Load the video
  std::shared_ptr<VideoInput> video_input;
  try {
    video_input = CreateVideoInput(path);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  const auto input_formats = video_input->GetInputFormats();
  const auto output_formats = video_input->GetOutputFormats();

  // Set up the video output
  const auto &analytics_format = input_formats.back();
  std::unique_ptr<VideoOutput> video_output;

  try {
    video_output = CreateVideoOutput(output_video_method, analytics_format);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  if (video_output)
    bia_buffer.reset(new uint8_t[(3 * analytics_format.width * analytics_format.height) / 2]);

  VideoInput::Frame frame;

  PrintFormats("Input formats", input_formats);
  PrintFormats("Output formats", output_formats);

  const char *model_path = "/mnt/shares/face/MTCNN-NCNN/models";
  MTCNN mtcnn(model_path);
  std::vector<Bbox> finalBbox;

  frame_no = 0;
  while (true) {
    VideoInput::Frame frame;
    if (video_input->ReadFrame(frame)) {
      // Keep a copy of the video for BIA
      if (video_output) {
        const auto &cif_buffer = frame.input_buffers.back();
        std::memcpy(bia_buffer.get(), cif_buffer.data + cif_buffer.planes[0].offset, cif_buffer.planes[0].size);
        std::memcpy(bia_buffer.get() + cif_buffer.planes[0].size, cif_buffer.data + cif_buffer.planes[1].offset, cif_buffer.planes[1].size);
      }

      // Output the video
      if (video_output) {
        const auto &analytics_format = input_formats.back();
        // convert NV12 to BGR
        cv::Mat picYV12 = cv::Mat(analytics_format.height * 3/2, analytics_format.width, CV_8UC1, bia_buffer.get());
        cv::Mat picBGR;
        cv::cvtColor(picYV12, picBGR, CV_YUV2BGR_NV12);

        ncnn::Mat ncnn_img = ncnn::Mat::from_pixels(picBGR.data, ncnn::Mat::PIXEL_BGR2RGB, picBGR.cols, picBGR.rows);

        double begin = get_current_time();
        #if(MAXFACEOPEN==1)
        mtcnn.detectMaxFace(ncnn_img, finalBbox);
        #else
        mtcnn.detect(ncnn_img, finalBbox);
        #endif
        double end = get_current_time();

        std::cerr << " " << picBGR.cols << "x" << picBGR.rows << " took: " << end - begin << "ms" << std::endl;

        const int num_box = finalBbox.size();
        std::vector<cv::Rect> bbox;
        bbox.resize(num_box);
        for (int i = 0; i < num_box; i++) {
          bbox[i] = cv::Rect(finalBbox[i].x1, finalBbox[i].y1, finalBbox[i].x2 - finalBbox[i].x1 + 1, finalBbox[i].y2 - finalBbox[i].y1 + 1);

          for (int j = 0; j<5; j = j + 1) {
            //cv::circle(image, cvPoint(finalBbox[i].ppoint[j], finalBbox[i].ppoint[j + 5]), 2, CV_RGB(0, 255, 0), CV_FILLED);
            cv::circle(picBGR, cvPoint(finalBbox[i].landmark.x[j], finalBbox[i].landmark.y[j]), 2, CV_RGB(0, 255, 0), CV_FILLED);
          }
        }

        for (vector<cv::Rect>::iterator it = bbox.begin(); it != bbox.end(); it++) {
          cv::rectangle(picBGR, (*it), cv::Scalar(0, 0, 255), 2, 8, 0);
        }

        // convert BGR to NV12
        cv::Mat nv12;
        BGR2YUV_nv12(picBGR, nv12);
        video_output->PushFrame(nv12.data);
      }
    } else {
      break;
    }
    frame_no++;

    if (frame_no > max_frames_to_play) {
      std::cerr << "Stop it reached max_frames_to_play:" << max_frames_to_play << std::endl;
      break;
    }
  }

  return EXIT_SUCCESS;
}