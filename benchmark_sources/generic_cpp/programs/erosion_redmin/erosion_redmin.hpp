#ifndef EROSION_REDMIN_HPP
#define EROSION_REDMIN_HPP
#include <cstdint>

#include "vicuna_crt.hpp"
#include "uart.hpp"

// Grayscale erosion with a flat k x k structuring element (k odd, centred, k <= 16).
// Same semantics as OpenCV cv::erode with a rectangular kernel and the default border
// (pixels outside the image never win), i.e. the minimum over the window clipped to the image.
// Returns 0 on success, -1 if k is even or larger than 16.
int erosion_redmin_u8(const uint8_t *src, uint8_t *dst, int32_t height, int32_t width, int32_t k);

#endif
