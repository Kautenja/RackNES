// The RackNES VCVRack plugin.
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

#include "plugin.hpp"

/// the global instance of the RackNES plug-in
Plugin* plugin_instance;

/// Initialize an instance of the RackNES plug-in.
///
/// @param instance a fresh instance of the RackNES plug-in to initialize
///
void init(Plugin* instance) {
    plugin_instance = instance;
    instance->addModel(modelRackNES);
}
