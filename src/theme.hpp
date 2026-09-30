// Theme support.
// Copyright 2020 Christian Kauten
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

#ifndef THEME_HPP_
#define THEME_HPP_

#include <string>
#include "rack.hpp"

/// @brief A widget base class following Rack's global light/dark preference.
/// @tparam BASENAME The panel path prefix in the "res/ModuleName" format.
/// @details Rack's themed panel switches SVGs on the UI thread, including
/// browser previews. No plugin-specific preference or patch state is needed.
template<const char* BASENAME>
struct ThemedWidget : rack::ModuleWidget {
    /// Create a panel that follows View > Use dark panels if available.
    ThemedWidget() {
        const std::string basename = BASENAME;
        setPanel(rack::createPanel(
            rack::asset::plugin(plugin_instance, basename + "-Light.svg"),
            rack::asset::plugin(plugin_instance, basename + "-Dark.svg")
        ));
    }
};

#endif  // THEME_HPP_
