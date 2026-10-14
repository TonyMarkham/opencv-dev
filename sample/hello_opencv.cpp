// Headless OpenCV validation: create a deterministic image, write it to a
// PNG file, read the file back, and assert the pixels survive the round
// trip, then run an imgproc check whose result is exactly predictable.
//
// Expected output (exit code 0):
//   Image: 320x240 BGR
//   PNG written: hello_opencv.png
//   PNG read back: identical
//   Threshold check: exact

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>

int main()
{
  // Every pixel is a fixed function of (x, y) — no randomness, no clock.
  cv::Mat src(240, 320, CV_8UC3);
  for (int y = 0; y < src.rows; ++y)
  {
    for (int x = 0; x < src.cols; ++x)
    {
      cv::Vec3b& px = src.at<cv::Vec3b>(y, x);
      px[0] = static_cast<uchar>((x * 3) % 256); // B
      px[1] = static_cast<uchar>((y * 5) % 256); // G
      px[2] = static_cast<uchar>((x + y) % 256); // R
    }
  }
  std::cout << "Image: " << src.cols << "x" << src.rows << " BGR" << std::endl;

  // PNG round trip through the codec stack compiled into
  // libopencv_imgcodecs (bundled libpng + zlib).
  if (!cv::imwrite("hello_opencv.png", src))
  {
    std::cerr << "hello_opencv: FAILED: PNG write" << std::endl;
    return 1;
  }
  std::cout << "PNG written: hello_opencv.png" << std::endl;

  const cv::Mat back = cv::imread("hello_opencv.png", cv::IMREAD_COLOR);
  if (back.empty() || back.cols != src.cols || back.rows != src.rows ||
      back.type() != src.type())
  {
    std::cerr << "hello_opencv: FAILED: PNG read back geometry" << std::endl;
    return 1;
  }
  // PNG is lossless: every pixel must come back exactly.
  if (cv::norm(src, back, cv::NORM_INF) != 0.0)
  {
    std::cerr << "hello_opencv: FAILED: PNG round trip not pixel-exact"
              << std::endl;
    return 1;
  }
  std::cout << "PNG read back: identical" << std::endl;

  // imgproc with an exactly predictable result: convert to single-channel
  // grayscale (threshold's contract is unambiguous there), threshold at 100,
  // and compare against the expected value recomputed here for every pixel.
  cv::Mat gray;
  cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
  cv::Mat th;
  cv::threshold(gray, th, 100, 255, cv::THRESH_BINARY);
  if (th.empty() || th.size() != gray.size() || th.type() != gray.type())
  {
    std::cerr << "hello_opencv: FAILED: threshold geometry" << std::endl;
    return 1;
  }
  for (int y = 0; y < gray.rows; ++y)
  {
    for (int x = 0; x < gray.cols; ++x)
    {
      const uchar got = th.at<uchar>(y, x);
      const uchar want = gray.at<uchar>(y, x) > 100 ? 255 : 0;
      if (got != want)
      {
        std::cerr << "hello_opencv: FAILED: threshold mismatch at (" << x
                  << "," << y << ")" << std::endl;
        return 1;
      }
    }
  }
  std::cout << "Threshold check: exact" << std::endl;

  return 0;
}
