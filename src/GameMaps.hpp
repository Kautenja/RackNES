// Memory map data from https://datacrystal.romhacking.net
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

#ifndef GAME_MAPS_HPP_
#define GAME_MAPS_HPP_

#include <string>
#include <cstdint>

/// A memory location within a particular game.
struct GameParameter {
    /// the RAM address for the cheat code
    uint16_t address = 0x0000;
    /// the minimal value for the cheat parameter
    uint8_t minimum = 0x00;
    /// the maximal value for the cheat parameter
    uint8_t maximum = 0xFF;
    /// the string name of the cheat
    std::string name = "";
    /// sets whether this location has only two valid values. when true,
    /// CV Genie expects a trigger to flip the value from one to the other
    bool toggle = false;

    /// @brief Initialize a new cheat parameter.
    ///
    /// @param address_ the RAM address for the cheat code
    /// @param minimum_ the minimal value for the cheat parameter
    /// @param maximum_ the maximal value for the cheat parameter
    /// @param name_ the string name of the cheat
    /// @param toggle_ whether this cheat code has only two values
    ///
    GameParameter(
        uint16_t address_,
        uint8_t minimum_,
        uint8_t maximum_,
        const std::string& name_,
        bool toggle_ = false
    ) :
        address(address_),
        minimum(minimum_),
        maximum(maximum_),
        name(name_),
        toggle(toggle_) { }
};

/// IDs for the presently supported games
enum GameIds {
    PLUMBER,
    TUNIC,
    NUM_GAMES
};

/// the number of parameters for each supported game
static constexpr unsigned PARAMETER_COUNTS[NUM_GAMES] = {
    53,
    55
};

static const char* NAMES[NUM_GAMES] = {
    "Super Mario Bros.",
    "The Legend of Zelda"
};

/// Parameters for the game "Super Mario Bros."
static const GameParameter PLUMBER_PARAMETERS[PARAMETER_COUNTS[PLUMBER]] = {
    GameParameter(0x000E, 0x00, 0x0C, "Player State"),
    GameParameter(0x001D, 0x00, 0x03, "Player Float State"),
    GameParameter(0x0033, 0x00, 0x02, "Player Facing Direction"),
    GameParameter(0x0045, 0x00, 0x02, "Player Moving Direction"),
    GameParameter(0x0700, 0x00, 0x28, "Player Horizontal Speed"),
    GameParameter(0x006D, 0x00, 0xFF, "Player Horizontal Level Position"),
    GameParameter(0x0086, 0x00, 0xFF, "Player Horizontal Screen Position"),
    GameParameter(0x00CE, 0x00, 0xFF, "Player Vertical Screen Position"),
    GameParameter(0x03C4, 0x00, 0xFF, "Player Palette"),
    GameParameter(0x06D5, 0x00, 0xFF, "Player Sprite State"),
    GameParameter(0x0704, 0x00, 0x01, "Player Swimming", true),
    GameParameter(0x0756, 0x00, 0x02, "Player Powerup State"),
    GameParameter(0x075A, 0x00, 0x62, "Player Lives"),
    GameParameter(0x075E, 0x00, 0x62, "Player Coins"),
    GameParameter(0x001B, 0x00, 0x2E, "Spawn Powerup", true),
    GameParameter(0x0039, 0x00, 0x03, "Spawned Powerup Type"),
    GameParameter(0x004B, 0x00, 0x02, "Mushroom Heading"),
    GameParameter(0x008C, 0x00, 0xFF, "Powerup Horizontal Screen Position"),
    GameParameter(0x00D4, 0x00, 0xFF, "Powerup Vertical Screen Position"),
    GameParameter(0x008D, 0x00, 0xFF, "Fireball Horizontal Screen Position"),
    GameParameter(0x00D5, 0x00, 0xFF, "Fireball Vertical Screen Position"),
    GameParameter(0x00A6, 0x00, 0xFF, "Fireball Vertical Speed"),
    GameParameter(0x0016, 0x00, 0x3C, "Enemy 1 Type"),
    GameParameter(0x0017, 0x00, 0x3C, "Enemy 2 Type"),
    GameParameter(0x0018, 0x00, 0x3C, "Enemy 3 Type"),
    GameParameter(0x0019, 0x00, 0x3C, "Enemy 4 Type"),
    GameParameter(0x001A, 0x00, 0x3C, "Enemy 5 Type"),
    GameParameter(0x001A, 0x00, 0x02, "Enemy 1 Heading"),
    GameParameter(0x001A, 0x00, 0x02, "Enemy 2 Heading"),
    GameParameter(0x001A, 0x00, 0x02, "Enemy 3 Heading"),
    GameParameter(0x001A, 0x00, 0x02, "Enemy 4 Heading"),
    GameParameter(0x001A, 0x00, 0x02, "Enemy 5 Heading"),
    GameParameter(0x006E, 0x00, 0xFF, "Enemy 1 Horizontal Level Position"),
    GameParameter(0x006F, 0x00, 0xFF, "Enemy 2 Horizontal Level Position"),
    GameParameter(0x0070, 0x00, 0xFF, "Enemy 3 Horizontal Level Position"),
    GameParameter(0x0071, 0x00, 0xFF, "Enemy 4 Horizontal Level Position"),
    GameParameter(0x0072, 0x00, 0xFF, "Enemy 5 Horizontal Level Position"),
    GameParameter(0x0087, 0x00, 0xFF, "Enemy 1 Horizontal Screen Position"),
    GameParameter(0x0088, 0x00, 0xFF, "Enemy 2 Horizontal Screen Position"),
    GameParameter(0x0089, 0x00, 0xFF, "Enemy 3 Horizontal Screen Position"),
    GameParameter(0x008A, 0x00, 0xFF, "Enemy 4 Horizontal Screen Position"),
    GameParameter(0x008B, 0x00, 0xFF, "Enemy 5 Horizontal Screen Position"),
    GameParameter(0x00CF, 0x00, 0xFF, "Enemy 1 Vertical Screen Position"),
    GameParameter(0x00D0, 0x00, 0xFF, "Enemy 2 Vertical Screen Position"),
    GameParameter(0x00D1, 0x00, 0xFF, "Enemy 3 Vertical Screen Position"),
    GameParameter(0x00D2, 0x00, 0xFF, "Enemy 4 Vertical Screen Position"),
    GameParameter(0x00D3, 0x00, 0xFF, "Enemy 5 Vertical Screen Position"),
    GameParameter(0x00E7, 0x00, 0xFF, "Level Layout Address"),
    GameParameter(0x00E9, 0x00, 0xFF, "Enemy Layout Address"),
    GameParameter(0x075F, 0x00, 0x0A, "World Number"),
    GameParameter(0x0760, 0x00, 0x04, "Level Number"),
    GameParameter(0x0772, 0x00, 0x03, "Level State"),
    GameParameter(0x0773, 0x00, 0x04, "Level Palette")
};

/// Parameters for the game "The Legend of Zelda."
static const GameParameter TUNIC_PARAMETERS[PARAMETER_COUNTS[TUNIC]] = {
    GameParameter(0x0513, 0x00, 0x01, "Candle On/Off", true),
    GameParameter(0x0523, 0x00, 0xFF, "Randomizer"),
    GameParameter(0x0526, 0x00, 0xFF, "Cave Return Screen"),
    GameParameter(0x052E, 0x00, 0x01, "Sword Disable", true),
    GameParameter(0x05F0, 0x00, 0xFF, "Frequency (Triangle)"),
    GameParameter(0x05F1, 0x00, 0x01, "Reverb (Triangle)", true),
    GameParameter(0x05F4, 0x00, 0xFF, "Rhythm"),
    GameParameter(0x0600, 0x00, 0xFF, "Song"),
    GameParameter(0x0607, 0x00, 0x80, "Jingle (Pulse 2)"),
    GameParameter(0x0604, 0x00, 0x80, "Jingle (Pulse 1)"),
    GameParameter(0x0606, 0x00, 0xFF, "Sound Effect"),
    GameParameter(0x060A, 0x00, 0xFF, "Song Position (Pulse 2)"),
    GameParameter(0x060B, 0x00, 0xFF, "Song Position (Pulse 1)"),
    GameParameter(0x060C, 0x00, 0xFF, "Song Position (Triangle)"),
    GameParameter(0x060D, 0x00, 0xFF, "Song Position (Noise)"),
    GameParameter(0x060F, 0x00, 0xFF, "Note Duration (Pulse 1)"),
    GameParameter(0x0610, 0x00, 0xFF, "Note Duration (Pulse 2)"),
    GameParameter(0x0611, 0x00, 0xFF, "Rhythm (Pulse 2)"),
    GameParameter(0x0612, 0x00, 0xFF, "Volume (Pulse 2)"),
    GameParameter(0x0613, 0x00, 0xFF, "Rhythm (Pulse 1)"),
    GameParameter(0x0614, 0x00, 0xFF, "Volume (Pulse 1)"),
    GameParameter(0x0615, 0x00, 0xFF, "Note Duration (Triangle)"),
    GameParameter(0x0616, 0x00, 0x01, "Note Delay"),
    GameParameter(0x0619, 0x80, 0x01, "Reverb (Pulse)", true),
    GameParameter(0x061E, 0x00, 0x01, "Triangle Freeze"),
    GameParameter(0x0656, 0x00, 0xFF, "B Item"),
    GameParameter(0x0657, 0x00, 0x03, "Sword Type"),
    GameParameter(0x0658, 0x00, 0xFF, "Bomb Count"),
    GameParameter(0x0659, 0x00, 0x02, "Arrow Type"),
    GameParameter(0x065A, 0x00, 0x01, "Bow Obtained", true),
    GameParameter(0x065B, 0x00, 0x02, "Candle Type"),
    GameParameter(0x065C, 0x00, 0x01, "Whistle Obtained", true),
    GameParameter(0x065D, 0x00, 0x01, "Food Obtained", true),
    GameParameter(0x065E, 0x00, 0x02, "Potion Type", true),
    GameParameter(0x065F, 0x00, 0x01, "Magical Rod Obtained", true),
    GameParameter(0x0660, 0x00, 0x01, "Raft Obtained", true),
    GameParameter(0x0661, 0x00, 0x01, "Magic Book Obtained", true),
    GameParameter(0x0662, 0x00, 0x02, "Ring Type"),
    GameParameter(0x0663, 0x00, 0x01, "Step Ladder Obtained", true),
    GameParameter(0x0664, 0x00, 0x01, "Magical Key Obtained", true),
    GameParameter(0x0665, 0x00, 0x01, "Power Bracelet Unlocked", true),
    GameParameter(0x0666, 0x00, 0x01, "Letter Obtained", true),
    GameParameter(0x0667, 0x00, 0xFF, "Compasses Obtained"),
    GameParameter(0x0668, 0x00, 0xFF, "Maps Obtained"),
    GameParameter(0x0669, 0x00, 0x01, "Lvl 9 Compass Obtained", true),
    GameParameter(0x066A, 0x00, 0x01, "Lvl 9 Map Obtained", true),
    GameParameter(0x066C, 0x00, 0x01, "Clock Obtained", true),
    GameParameter(0x066D, 0x00, 0xFF, "Rupee Count"),
    GameParameter(0x066E, 0x00, 0xFF, "Key Count"),
    GameParameter(0x066F, 0x00, 0xFF, "Health"),
    GameParameter(0x0671, 0x00, 0xFF, "Triforce Pieces"),
    GameParameter(0x0674, 0x00, 0x01, "Boomerang Obtained", true),
    GameParameter(0x0675, 0x00, 0x01, "Magic Boomerang Obtained", true),
    GameParameter(0x0676, 0x00, 0x01, "Magic Shield Obtained", true),
    GameParameter(0x67C, 0x08, 0xFF, "Max Bomb Count")
};

static const GameParameter* games[2] = {
    PLUMBER_PARAMETERS,
    TUNIC_PARAMETERS
};

/// @brief A container for memory maps of NES games
struct GameMap {
    /// the current ID of the loaded game
    int gameId = -1;

    /// @brief Initialize a new game map.
    GameMap() { }

    /// @brief Return the memory address for a given ID.
    inline uint16_t getAddress(int elementId) const {
        return games[gameId][elementId].address;
    }

    /// @brief Return the minimum value for the memory address with a given ID.
    inline uint8_t getMinValue(int elementId) const {
        return games[gameId][elementId].minimum;
    }

    /// @brief Return the maximum value for the memory address with a given ID.
    inline uint8_t getMaxValue(int elementId) const {
        return games[gameId][elementId].maximum;
    }

    /// @brief Return whether the memory address with a given ID is a toggle.
    inline bool isToggle(int elementId) const {
        return games[gameId][elementId].toggle;
    }

    /// @brief Return the string name for the memory address with a given ID.
    inline std::string getName(int elementId) const {
        return games[gameId][elementId].name;
    }

    inline unsigned getNumCheats() const {
        return PARAMETER_COUNTS[gameId];
    }

    /// @brief Return the name for the game associated with a given ID.
    inline std::string getGameName(int id) const {
        return NAMES[id];
    }

    /// @brief Set the game to a new game ID.
    inline void setGame(GameIds id) { gameId = id; }
};

#endif  // GAME_MAPS_HPP_
