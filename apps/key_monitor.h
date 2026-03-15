#include <array>
#include <bitset>
#include <concepts>
#include <filesystem>
#include <libevdev/libevdev.h>
#include <map>
#include <set>
#include <utility>

enum key_state_t { KEY_RELEASED, KEY_PRESSED, KEY_HOLD };
using key_code_t = decltype(input_event::code);
using key_event_t = std::pair<key_code_t, key_state_t>;
using name_file_map_t = std::map<std::string, std::filesystem::path>;

// All tracked keyboard leds
constexpr std::array<key_code_t, 3> keyboardLedValues = {LED_NUML, LED_CAPSL,
                                                         LED_SCROLLL};
// When this key is held, the alternative function for a key is used
constexpr key_code_t fnKey = 74;
// This button sends stop signal
constexpr key_code_t stopKey = 78;
// Keys that represent each sound
const std::map<key_code_t, int> soundKeys = {{69, 1}, {98, 2}, {55, 3},
                                             {71, 4}, {72, 5}, {73, 6}};
// Keys that represent the different sound banks
const std::map<key_code_t, int> bankKeys = {{69, 1}, {98, 2}, {71, 3}, {72, 4}};
// Volume keys and steps
const std::map<key_code_t, int> volumeKeys = {{55, +5}, {73, -5}};

class EventDevice {
  public:
    EventDevice(std::filesystem::path device_path);
    ~EventDevice();

    // Delete copy constructor and copy assignment
    EventDevice(const EventDevice &) = delete;
    EventDevice &operator=(const EventDevice &) = delete;

    // Move constructor and move assignment
    EventDevice(EventDevice &&other);
    EventDevice &operator=(EventDevice &&other);

    void cycleLeds();
    std::string getName() { return libevdev_get_name(dev); }
    key_event_t getNextKey();
    bool keyIsActive(key_code_t code);
    void setLed(size_t ledno, bool state);

  private:
    int fd = -1;
    struct libevdev *dev = NULL;
    unsigned int readFlag =
        LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING;
    std::bitset<keyboardLedValues.size()> ledState;
    std::set<key_code_t> activeKeys;
    key_event_t lastKey{0, KEY_RELEASED};

    void closeDevice();
    void handleLedEvent(key_code_t code, bool value);
    void resetReferences() {
        fd = -1;
        dev = NULL;
    }
};

// Functions to check the state of a key event
inline bool isReleased(key_state_t state) { return state == KEY_RELEASED; };
inline bool isPressed(key_state_t state) { return state == KEY_PRESSED; };
inline bool isHeld(key_state_t state) { return state == KEY_HOLD; };
inline bool isActive(key_state_t state) {
    return isPressed(state) || isHeld(state);
};

key_state_t getKeyState(std::integral auto value);
int changeVolume(key_code_t code);
void process_key(EventDevice &device, key_event_t keyEvent);
name_file_map_t get_input_files_by_names();
