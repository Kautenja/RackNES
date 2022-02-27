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

#include <unistd.h>
#include <jansson.h>
#include <fstream>
#include <cstdio>
#include <string>
#include <unordered_map>
#include "rack.hpp"

// @brief The supported themes.
static const std::vector<std::string> THEMES{
    "Light",
    "Dark"
};

/// @brief Get the theme for the plugin.
///
/// @param value The value to store the output theme in.
/// @returns 0 If the operation succeeded, a positive error code otherwise.
///
inline int get_theme(std::string* value) {
    std::string path = rack::asset::user("RackNES.json");
    if (access(path.c_str(), R_OK) != 0) {  // File does not exist.
        return 1;
    }
    // Attempt to load the JSON file.
    json_error_t error;
    json_t* root = json_load_file(path.c_str(), 0, &error);
    if (!root) {  // Failed to load JSON.
        return 2;
    }
    // TODO: handle `error` object.
    json_t* theme = json_object_get(root, "theme");
    if (!theme) {  // No theme object.
        return 3;
    }
    *value = json_string_value(theme);
    return 0;
}

/// @brief Set the theme for the plugin.
///
/// @param value The new theme to store and set as default.
/// @returns 0 If the operation succeeded, a positive error code otherwise.
///
inline int set_theme(const std::string& value) {
    std::string path = rack::asset::user("RackNES.json");
    json_t* root;
    if (access(path.c_str(), R_OK) != 0) {  // File does not exist.
        root = json_object();
    } else {  // File exists.
        json_error_t error;
        root = json_load_file(path.c_str(), 0, &error);
    }
    json_object_set_new(root, "theme", json_string(value.c_str()));
    return json_dump_file(root, path.c_str(), 0);
}

/// A menu item for selecting themes.
struct ThemeMenuItem : rack::MenuItem {
    /// The widget associated with the menu item.
    rack::ModuleWidget* widget = nullptr;
    /// The basename for the panel files for this menu item.
    std::string basename;
    /// The theme for this menu item.
    std::string theme;

    /// Respond to an action on the menu item.
    void onAction(const rack::event::Action &e) override {
        set_theme(theme);
        const auto path = basename + "-" + theme + ".svg";
        widget->setPanel(APP->window->loadSvg(asset::plugin(plugin_instance, path)));
    }
};

/// @brief A widget base class for supporting plugin theming.
/// @tparam BASENAME The basename of the module's panel paths for the plugins.
/// Should be in the "res/ModuleName" format.
template<const char* BASENAME>
struct ThemedWidget : rack::ModuleWidget {
    /// Create a new themed widget.
    ThemedWidget() {
        std::string theme = THEMES[0];
        get_theme(&theme);
        theme = "-" + theme + ".svg";
        setPanel(APP->window->loadSvg(asset::plugin(plugin_instance, BASENAME + theme)));
    }

    /// Add context items for the module to a menu on the UI.
    ///
    /// @param menu the menu to add the context items to
    ///
    inline void appendContextMenu(ui::Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Plugin Theme"));
        std::string current_theme = THEMES[0];
        get_theme(&current_theme);
        for (int i = 0; i < 2; i++) {
            auto item = createMenuItem<ThemeMenuItem>(THEMES[i], CHECKMARK(current_theme == THEMES[i]));
            item->widget = this;
            item->basename = BASENAME;
            item->theme = THEMES[i];
            menu->addChild(item);
        }
    }
};

#endif  // THEME_HPP_
