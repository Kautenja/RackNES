// SVG Components for VCV Rack (buttons, knobs, etc.).
// Copyright 2020 Christian Kauten
//
// Author: Christian Kauten (kautenja@auburn.edu)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef RACKNES_WIDGETS_DISPLAY_HPP_
#define RACKNES_WIDGETS_DISPLAY_HPP_

#include "rack.hpp"

/// A widget that displays a 32-bit RGBA pixel buffer.
struct Display : rack::LightWidget {
 private:
    /// the size of the internal pixel buffer to render
    const rack::Vec image_size;
    /// a pointer to the pixels to render. A pixel is represented as 4 bytes
    /// in RGBA order
    const uint8_t* pixels;
    /// a pointer to the image to draw the display to
    int screen = -1;

 public:
    /// whether the screen is turned on
    bool is_on = false;

    /// @brief Initialize a new display widget.
    ///
    /// @param position the position of the screen on the module
    /// @param pixels_ the pixels on the display to render. A pixel is
    /// represented as 4 bytes in RGBA order
    /// @param image_size_ the size of the input image
    /// @param render_size the output size of the display to render. NanoSVG
    /// will provide interpolation logic between the image_size_ and the
    /// render_size.
    ///
    explicit Display(
        rack::Vec position,
        const uint8_t* pixels_,
        rack::Vec image_size_,
        rack::Vec render_size
    ) :
        LightWidget(), image_size(image_size_), pixels(pixels_) {
        setPosition(position);
        setSize(render_size);
    }

    /// @brief Draw the display on the main context.
    ///
    /// @param args the arguments for the draw context for this widget
    ///
    void draw(const DrawArgs& args) override {
        // the image flags for creating the screen
        static constexpr int imageFlags = 0;
        // the x position of the screen (relative, not absolute)
        static constexpr int x = 0;
        // the y position of the screen (relative, not absolute)
        static constexpr int y = 0;
        // the angle to draw the screen at
        static constexpr float angle = 0;
        // the alpha value of the SVG image
        static constexpr float alpha = 1.f;
        // don't do anything if the screen is not on
        if (!is_on) return;
        // return if the pixels aren't set for the screen yet
        if (pixels == nullptr) return;
        // -------------------------------------------------------------------
        // create / update the image container
        // -------------------------------------------------------------------
        if (screen == -1)  // check if the screen has been initialized yet
            screen = nvgCreateImageRGBA(args.vg, image_size.x, image_size.y, imageFlags, pixels);
        else  // update the screen with the pixel data
            nvgUpdateImage(args.vg, screen, pixels);
        // -------------------------------------------------------------------
        // draw the screen
        // -------------------------------------------------------------------
        nvgBeginPath(args.vg);
        nvgRect(args.vg, x, y, box.size.x, box.size.y);
        nvgFillPaint(args.vg, nvgImagePattern(args.vg, x, y, box.size.x, box.size.y, angle, screen, alpha));
        nvgFill(args.vg);
        nvgClosePath(args.vg);
    }
};

#endif  // RACKNES_WIDGETS_DISPLAY_HPP_
