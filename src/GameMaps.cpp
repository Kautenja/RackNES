// Memory map data from https://datacrystal.romhacking.net

#include "plugin.hpp"

enum GameIds {
    PLUMBER,
    TUNIC,
    NUM_GAMES
};

struct GameElement {
    uint16_t address = 0x0000;
    uint8_t minVal = 0x00;
    uint8_t maxVal = 0xFF;
    std::string name = "";
    bool toggle = false;

    GameElement(uint16_t addr, uint8_t min, uint8_t max, std::string s, bool tog = false) {
        address = addr;
        maxVal = max;
        name = s;
        toggle = tog;
    }
};

struct GameMap {
    int gameId = -1;
    std::vector<GameElement> elements;

    GameMap() {};

    uint16_t getAddress(int elementId) {
        uint16_t address = elements[elementId].address;
        return address;
    }

    uint8_t getMinValue(int elementId) {
        uint8_t minVal = elements[elementId].minVal;
        return minVal;
    }

    uint8_t getMaxValue(int elementId) {
        uint8_t maxVal = elements[elementId].maxVal;
        return maxVal;
    }

    bool isToggle(int elementId) {
        return elements[elementId].toggle;
    }

    std::string getName(int elementId) {
        std::string name = elements[elementId].name;
        return name;
    }

    std::vector<GameElement> getMap() {
        return elements;
    }

    std::string getGameName(int id) {
        switch (id) {
            case PLUMBER:
                return "Plumber";
                break;
            case TUNIC:
                return "Tunic";
                break;
            default:
                return "I AM ERROR";
                break;
        }
    }

    void setGame(int id) {
        gameId = id;
        elements.clear();
        switch (gameId) {
            case PLUMBER:
                elements.push_back(GameElement(0x000E, 0x00, 0x0C, "Player State"));
                elements.push_back(GameElement(0x001D, 0x00, 0x03, "Player Float State"));
                elements.push_back(GameElement(0x0033, 0x00, 0x02, "Player Facing Direction"));
                elements.push_back(GameElement(0x0045, 0x00, 0x02, "Player Moving Direction"));
                elements.push_back(GameElement(0x0700, 0x00, 0x28, "Player Horizontal Speed"));
                elements.push_back(GameElement(0x006D, 0x00, 0xFF, "Player Horizontal Level Position"));
                elements.push_back(GameElement(0x0086, 0x00, 0xFF, "Player Horizontal Screen Position"));
                elements.push_back(GameElement(0x00CE, 0x00, 0xFF, "Player Vertical Screen Position"));
                elements.push_back(GameElement(0x03C4, 0x00, 0xFF, "Player Palette"));
                elements.push_back(GameElement(0x06D5, 0x00, 0xFF, "Player Sprite State"));
                elements.push_back(GameElement(0x0704, 0x00, 0x01, "Player Swimming", true));
                elements.push_back(GameElement(0x0756, 0x00, 0x02, "Player Powerup State"));
                elements.push_back(GameElement(0x075A, 0x00, 0x62, "Player Lives"));
                elements.push_back(GameElement(0x075E, 0x00, 0x62, "Player Coins"));
                elements.push_back(GameElement(0x001B, 0x00, 0x2E, "Spawn Powerup", true));
                elements.push_back(GameElement(0x0039, 0x00, 0x03, "Spawned Powerup Type"));
                elements.push_back(GameElement(0x004B, 0x00, 0x02, "Mushroom Heading"));
                elements.push_back(GameElement(0x008C, 0x00, 0xFF, "Powerup Horizontal Screen Position"));
                elements.push_back(GameElement(0x00D4, 0x00, 0xFF, "Powerup Vertical Screen Position"));
                elements.push_back(GameElement(0x008D, 0x00, 0xFF, "Fireball Horizontal Screen Position"));
                elements.push_back(GameElement(0x00D5, 0x00, 0xFF, "Fireball Vertical Screen Position"));
                elements.push_back(GameElement(0x00A6, 0x00, 0xFF, "Fireball Vertical Speed"));
                elements.push_back(GameElement(0x0016, 0x00, 0x3C, "Enemy 1 Type"));
                elements.push_back(GameElement(0x0017, 0x00, 0x3C, "Enemy 2 Type"));
                elements.push_back(GameElement(0x0018, 0x00, 0x3C, "Enemy 3 Type"));
                elements.push_back(GameElement(0x0019, 0x00, 0x3C, "Enemy 4 Type"));
                elements.push_back(GameElement(0x001A, 0x00, 0x3C, "Enemy 5 Type"));
                elements.push_back(GameElement(0x001A, 0x00, 0x02, "Enemy 1 Heading"));
                elements.push_back(GameElement(0x001A, 0x00, 0x02, "Enemy 2 Heading"));
                elements.push_back(GameElement(0x001A, 0x00, 0x02, "Enemy 3 Heading"));
                elements.push_back(GameElement(0x001A, 0x00, 0x02, "Enemy 4 Heading"));
                elements.push_back(GameElement(0x001A, 0x00, 0x02, "Enemy 5 Heading"));
                elements.push_back(GameElement(0x006E, 0x00, 0xFF, "Enemy 1 Horizontal Level Position"));
                elements.push_back(GameElement(0x006F, 0x00, 0xFF, "Enemy 2 Horizontal Level Position"));
                elements.push_back(GameElement(0x0070, 0x00, 0xFF, "Enemy 3 Horizontal Level Position"));
                elements.push_back(GameElement(0x0071, 0x00, 0xFF, "Enemy 4 Horizontal Level Position"));
                elements.push_back(GameElement(0x0072, 0x00, 0xFF, "Enemy 5 Horizontal Level Position"));
                elements.push_back(GameElement(0x0087, 0x00, 0xFF, "Enemy 1 Horizontal Screen Position"));
                elements.push_back(GameElement(0x0088, 0x00, 0xFF, "Enemy 2 Horizontal Screen Position"));
                elements.push_back(GameElement(0x0089, 0x00, 0xFF, "Enemy 3 Horizontal Screen Position"));
                elements.push_back(GameElement(0x008A, 0x00, 0xFF, "Enemy 4 Horizontal Screen Position"));
                elements.push_back(GameElement(0x008B, 0x00, 0xFF, "Enemy 5 Horizontal Screen Position"));
                elements.push_back(GameElement(0x00CF, 0x00, 0xFF, "Enemy 1 Vertical Screen Position"));
                elements.push_back(GameElement(0x00D0, 0x00, 0xFF, "Enemy 2 Vertical Screen Position"));
                elements.push_back(GameElement(0x00D1, 0x00, 0xFF, "Enemy 3 Vertical Screen Position"));
                elements.push_back(GameElement(0x00D2, 0x00, 0xFF, "Enemy 4 Vertical Screen Position"));
                elements.push_back(GameElement(0x00D3, 0x00, 0xFF, "Enemy 5 Vertical Screen Position"));
                elements.push_back(GameElement(0x00E7, 0x00, 0xFF, "Level Layout Address"));
                elements.push_back(GameElement(0x00E9, 0x00, 0xFF, "Enemy Layout Address"));
                elements.push_back(GameElement(0x075F, 0x00, 0x0A, "World Number"));
                elements.push_back(GameElement(0x0760, 0x00, 0x04, "Level Number"));
                elements.push_back(GameElement(0x0772, 0x00, 0x03, "Level State"));
                elements.push_back(GameElement(0x0773, 0x00, 0x04, "Level Palette"));
                break;
            case TUNIC:
                elements.push_back(GameElement(0x0513, 0x00, 0x01, "Candle On/Off", true));
                elements.push_back(GameElement(0x0523, 0x00, 0xFF, "Randomizer"));
                elements.push_back(GameElement(0x0526, 0x00, 0xFF, "Cave Return Screen"));
                elements.push_back(GameElement(0x052E, 0x00, 0x01, "Sword Disable", true));
                elements.push_back(GameElement(0x05F0, 0x00, 0xFF, "Frequency (Triangle)"));
                elements.push_back(GameElement(0x05F1, 0x00, 0x01, "Reverb (Triangle)", true));
                elements.push_back(GameElement(0x05F4, 0x00, 0xFF, "Rhythm"));
                elements.push_back(GameElement(0x0600, 0x00, 0xFF, "Song"));
                elements.push_back(GameElement(0x0607, 0x00, 0x80, "Jingle (Pulse 2)"));
                elements.push_back(GameElement(0x0604, 0x00, 0x80, "Jingle (Pulse 1)"));
                elements.push_back(GameElement(0x0606, 0x00, 0xFF, "Sound Effect"));
                elements.push_back(GameElement(0x060A, 0x00, 0xFF, "Song Position (Pulse 2)"));
                elements.push_back(GameElement(0x060B, 0x00, 0xFF, "Song Position (Pulse 1)"));
                elements.push_back(GameElement(0x060C, 0x00, 0xFF, "Song Position (Triangle)"));
                elements.push_back(GameElement(0x060D, 0x00, 0xFF, "Song Position (Noise)"));
                elements.push_back(GameElement(0x060F, 0x00, 0xFF, "Note Duration (Pulse 1)"));
                elements.push_back(GameElement(0x0610, 0x00, 0xFF, "Note Duration (Pulse 2)"));
                elements.push_back(GameElement(0x0611, 0x00, 0xFF, "Rhythm (Pulse 2)"));
                elements.push_back(GameElement(0x0612, 0x00, 0xFF, "Volume (Pulse 2)"));
                elements.push_back(GameElement(0x0613, 0x00, 0xFF, "Rhythm (Pulse 1)"));
                elements.push_back(GameElement(0x0614, 0x00, 0xFF, "Volume (Pulse 1)"));
                elements.push_back(GameElement(0x0615, 0x00, 0xFF, "Note Duration (Triangle)"));
                elements.push_back(GameElement(0x0616, 0x00, 0x01, "Note Delay"));
                elements.push_back(GameElement(0x0619, 0x80, 0x01, "Reverb (Pulse)", true));
                elements.push_back(GameElement(0x061E, 0x00, 0x01, "Triangle Freeze"));
                elements.push_back(GameElement(0x0656, 0x00, 0xFF, "B Item"));
                elements.push_back(GameElement(0x0657, 0x00, 0x03, "Sword Type"));
                elements.push_back(GameElement(0x0658, 0x00, 0xFF, "Bomb Count"));
                elements.push_back(GameElement(0x0659, 0x00, 0x02, "Arrow Type"));
                elements.push_back(GameElement(0x065A, 0x00, 0x01, "Bow Obtained", true));
                elements.push_back(GameElement(0x065B, 0x00, 0x02, "Candle Type"));
                elements.push_back(GameElement(0x065C, 0x00, 0x01, "Whistle Obtained", true));
                elements.push_back(GameElement(0x065D, 0x00, 0x01, "Food Obtained", true));
                elements.push_back(GameElement(0x065E, 0x00, 0x02, "Potion Type", true));
                elements.push_back(GameElement(0x065F, 0x00, 0x01, "Magical Rod Obtained", true));
                elements.push_back(GameElement(0x0660, 0x00, 0x01, "Raft Obtained", true));
                elements.push_back(GameElement(0x0661, 0x00, 0x01, "Magic Book Obtained", true));
                elements.push_back(GameElement(0x0662, 0x00, 0x02, "Ring Type"));
                elements.push_back(GameElement(0x0663, 0x00, 0x01, "Step Ladder Obtained", true));
                elements.push_back(GameElement(0x0664, 0x00, 0x01, "Magical Key Obtained", true));
                elements.push_back(GameElement(0x0665, 0x00, 0x01, "Power Bracelet Unlocked", true));
                elements.push_back(GameElement(0x0666, 0x00, 0x01, "Letter Obtained", true));
                elements.push_back(GameElement(0x0667, 0x00, 0xFF, "Compasses Obtained"));
                elements.push_back(GameElement(0x0668, 0x00, 0xFF, "Maps Obtained"));
                elements.push_back(GameElement(0x0669, 0x00, 0x01, "Lvl 9 Compass Obtained", true));
                elements.push_back(GameElement(0x066A, 0x00, 0x01, "Lvl 9 Map Obtained", true));
                elements.push_back(GameElement(0x066C, 0x00, 0x01, "Clock Obtained", true));
                elements.push_back(GameElement(0x066D, 0x00, 0xFF, "Rupee Count"));
                elements.push_back(GameElement(0x066E, 0x00, 0xFF, "Key Count"));
                elements.push_back(GameElement(0x066F, 0x00, 0xFF, "Health"));
                elements.push_back(GameElement(0x0671, 0x00, 0xFF, "Triforce Pieces"));
                elements.push_back(GameElement(0x0674, 0x00, 0x01, "Boomerang Obtained", true));
                elements.push_back(GameElement(0x0675, 0x00, 0x01, "Magic Boomerang Obtained", true));
                elements.push_back(GameElement(0x0676, 0x00, 0x01, "Magic Shield Obtained", true));
                elements.push_back(GameElement(0x67C, 0x08, 0xFF, "Max Bomb Count"));
                break;
        }
    }
};