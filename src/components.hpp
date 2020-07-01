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

#ifndef RACKNES_COMPONENTS_HPP
#define RACKNES_COMPONENTS_HPP

#include "plugin.hpp"

/// @brief A knob in the style of the NES turbo knob, but the shape of Rogan 1P.
struct Rogan1PSNES : rack::Rogan {
    Rogan1PSNES() {
        static constexpr auto path = "res/ComponentLibrary/Rogan1PSNES.svg";
        setSvg(APP->window->loadSvg(rack::asset::plugin(plugin_instance, path)));
    }
};

/// @brief A knob in the style of the NES turbo knob, but the shape of Rogan 2P.
struct Rogan2PSNES : rack::Rogan {
    Rogan2PSNES() {
        static constexpr auto path = "res/ComponentLibrary/Rogan2PSNES.svg";
        setSvg(APP->window->loadSvg(rack::asset::plugin(plugin_instance, path)));
    }
};

/// @brief A knob in the style of the NES turbo knob, but the shape of Rogan 3P.
struct Rogan3PSNES : rack::Rogan {
    Rogan3PSNES() {
        static constexpr auto path = "res/ComponentLibrary/Rogan3PSNES.svg";
        setSvg(APP->window->loadSvg(rack::asset::plugin(plugin_instance, path)));
    }
};

/// @brief A switch resembling the "Start"/"Select" switch from the NES
/// controller in the vertical position.
struct NESSwitchVertical : app::SvgSwitch {
    NESSwitchVertical() {
        momentary = true;
        static constexpr auto path_0 = "res/ComponentLibrary/NESSwitchVertical_0.svg";
        addFrame(APP->window->loadSvg(asset::plugin(plugin_instance, path_0)));
        static constexpr auto path_1 = "res/ComponentLibrary/NESSwitchVertical_1.svg";
        addFrame(APP->window->loadSvg(asset::plugin(plugin_instance, path_1)));
    }
};

/// @brief A switch resembling the "A"/"B" switch from the NES controller.
struct CKD6_NES_Red : app::SvgSwitch {
    CKD6_NES_Red() {
        momentary = true;
        static constexpr auto path_0 = "res/ComponentLibrary/CKD6_NES_Red_0.svg";
        addFrame(APP->window->loadSvg(asset::plugin(plugin_instance, path_0)));
        static constexpr auto path_1 = "res/ComponentLibrary/CKD6_NES_Red_1.svg";
        addFrame(APP->window->loadSvg(asset::plugin(plugin_instance, path_1)));
    }
};

#endif  // RACKNES_COMPONENTS_HPP
