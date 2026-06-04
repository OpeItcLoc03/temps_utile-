// Copyright (c) 2016 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef DRIVERS_DISPLAY_H_
#define DRIVERS_DISPLAY_H_

#include "framebuffer.h"
#include "page_display_driver.h"
#include "SH1106_128x64_driver.h"
#include "weegfx.h"

namespace display {

extern FrameBuffer<SH1106_128x64_Driver::kFrameSize, 2> frame_buffer;
extern PagedDisplayDriver<SH1106_128x64_Driver> driver;

void Init();
void AdjustOffset(uint8_t offset);

static inline void Flush() __attribute__((always_inline));
static inline void Flush() {
	if (driver.Flush())
		frame_buffer.read();
}

static inline void Update() __attribute__((always_inline));
static inline void Update() {
  if (driver.frame_valid()) {
    driver.Update();
  } else {
    if (frame_buffer.readable())
      driver.Begin(frame_buffer.readable_frame());
  }
}

#if defined(__IMXRT1062__)
// T4.0 has no display DMA: SendPage() blocks (bit-banged SPI, ~150us/page). The
// K20 build hides that latency by paging the transfer through the 60us CORE ISR
// via Flush()/Update(); doing the same with a blocking bit-bang overruns the ISR
// and hangs the system. So on T4 the panel is flushed synchronously here, from
// whatever context just finished drawing the frame (see GRAPHICS_END_FRAME), and
// the ISR does not touch the display at all.
static inline void Render() {
  if (!frame_buffer.readable())
    return;
  driver.Begin(frame_buffer.readable_frame());
  while (!driver.Flush())
    driver.Update();
  frame_buffer.read();
}
#endif

};

extern weegfx::Graphics graphics;

#define GRAPHICS_BEGIN_FRAME(wait) \
do { \
  DEBUG_PIN_SCOPE(DEBUG_PIN_1); \
  uint8_t *frame = NULL; \
  do { \
    if (display::frame_buffer.writeable()) \
      frame = display::frame_buffer.writeable_frame(); \
  } while (!frame && wait); \
  if (frame) { \
    graphics.Begin(frame, true); \
    do {} while(0)

#if defined(__IMXRT1062__)
// T4.0: push the just-finished frame to the panel synchronously (no ISR/DMA).
#define DISPLAY_RENDER_FRAME() display::Render()
#else
#define DISPLAY_RENDER_FRAME() do {} while (0)
#endif

#define GRAPHICS_END_FRAME() \
    graphics.End(); \
    display::frame_buffer.written(); \
    DISPLAY_RENDER_FRAME(); \
  } \
} while (0)


#endif // DRIVERS_DISPLAY_H_
