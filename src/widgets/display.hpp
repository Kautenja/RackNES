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
struct Display : rack::TransparentWidget {
 private:
    /// the size of the internal pixel buffer to render
    const rack::Vec image_size;
    /// a pointer to the pixels to render
    const uint8_t* pixels;
    /// a pointer to the image to draw the display to
    int screen = -1;

 public:
    /// whether the screen is turned on
    bool is_on = false;

    /// @brief Initialize a new display widget.
    ///
    /// @param position the position of the screen on the module
    /// @param pixels_ the pixels on the display to render
    /// @param image_size the size of the input image
    /// @param render_size the output size of the display to render
    ///
    explicit Display(
        rack::Vec position,
        const uint8_t* pixels_,
        rack::Vec image_size_,
        rack::Vec render_size
    ) :
        TransparentWidget(), image_size(image_size_), pixels(pixels_) {
        setPosition(position);
        setSize(render_size);
    }

    /// @brief Draw the display on the main context.
    ///
    /// @param args the arguments for the draw context for this widget
    ///
    void draw(const DrawArgs& args) override {
        // the x position of the screen
        static constexpr int x = 0;
        // the y position of the screen
        static constexpr int y = 0;
        // the alpha value of the SVG image
        static constexpr float alpha = 1.f;
        // don't do anything if the screen is not on
        if (!is_on) return;
        // return if the pixels aren't on the screen yet
        if (pixels == nullptr) return;
        // draw the screen
        if (screen == -1)  // check if the screen has been initialized yet
            screen = nvgCreateImageRGBA(args.vg, image_size.x, image_size.y, 0, pixels);
        else  // update the screen with the pixel data
            nvgUpdateImage(args.vg, screen, pixels);
        // get the screen as a fill paint (for a rectangle)
        auto imgPaint = nvgImagePattern(args.vg, x, y, box.size.x, box.size.y, 0, screen, alpha);
        // create a path for the rectangle to show the screen
        nvgBeginPath(args.vg);
        // create a rectangle to draw the screen
        nvgRect(args.vg, x, y, box.size.x, box.size.y);
        // paint the rectangle's fill from the screen
        nvgFillPaint(args.vg, imgPaint);
        nvgFill(args.vg);
    }
};

#endif  // RACKNES_WIDGETS_DISPLAY_HPP_
