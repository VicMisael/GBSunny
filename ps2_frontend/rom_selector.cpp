#include "rom_selector.h"

#include <SDL.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <dirent.h>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>

namespace ps2_frontend {
namespace {

constexpr int ScreenWidth = 640;
constexpr int ScreenHeight = 448;
constexpr int FontScale = 2;
constexpr int GlyphWidth = 6 * FontScale;
constexpr int LineHeight = 18;
constexpr int ListTop = 91;
constexpr int VisibleRows = 17;

struct Entry {
    std::string name;
    std::string path;
    bool directory;
};

using Glyph = std::array<std::uint8_t, 5>;

constexpr Glyph glyph_for(char character)
{
    const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
    switch (c) {
    case 'A': return {0x7E, 0x11, 0x11, 0x11, 0x7E};
    case 'B': return {0x7F, 0x49, 0x49, 0x49, 0x36};
    case 'C': return {0x3E, 0x41, 0x41, 0x41, 0x22};
    case 'D': return {0x7F, 0x41, 0x41, 0x22, 0x1C};
    case 'E': return {0x7F, 0x49, 0x49, 0x49, 0x41};
    case 'F': return {0x7F, 0x09, 0x09, 0x09, 0x01};
    case 'G': return {0x3E, 0x41, 0x49, 0x49, 0x7A};
    case 'H': return {0x7F, 0x08, 0x08, 0x08, 0x7F};
    case 'I': return {0x00, 0x41, 0x7F, 0x41, 0x00};
    case 'J': return {0x20, 0x40, 0x41, 0x3F, 0x01};
    case 'K': return {0x7F, 0x08, 0x14, 0x22, 0x41};
    case 'L': return {0x7F, 0x40, 0x40, 0x40, 0x40};
    case 'M': return {0x7F, 0x02, 0x0C, 0x02, 0x7F};
    case 'N': return {0x7F, 0x04, 0x08, 0x10, 0x7F};
    case 'O': return {0x3E, 0x41, 0x41, 0x41, 0x3E};
    case 'P': return {0x7F, 0x09, 0x09, 0x09, 0x06};
    case 'Q': return {0x3E, 0x41, 0x51, 0x21, 0x5E};
    case 'R': return {0x7F, 0x09, 0x19, 0x29, 0x46};
    case 'S': return {0x46, 0x49, 0x49, 0x49, 0x31};
    case 'T': return {0x01, 0x01, 0x7F, 0x01, 0x01};
    case 'U': return {0x3F, 0x40, 0x40, 0x40, 0x3F};
    case 'V': return {0x1F, 0x20, 0x40, 0x20, 0x1F};
    case 'W': return {0x3F, 0x40, 0x38, 0x40, 0x3F};
    case 'X': return {0x63, 0x14, 0x08, 0x14, 0x63};
    case 'Y': return {0x07, 0x08, 0x70, 0x08, 0x07};
    case 'Z': return {0x61, 0x51, 0x49, 0x45, 0x43};
    case '0': return {0x3E, 0x51, 0x49, 0x45, 0x3E};
    case '1': return {0x00, 0x42, 0x7F, 0x40, 0x00};
    case '2': return {0x62, 0x51, 0x49, 0x49, 0x46};
    case '3': return {0x22, 0x41, 0x49, 0x49, 0x36};
    case '4': return {0x18, 0x14, 0x12, 0x7F, 0x10};
    case '5': return {0x2F, 0x49, 0x49, 0x49, 0x31};
    case '6': return {0x3E, 0x49, 0x49, 0x49, 0x32};
    case '7': return {0x01, 0x71, 0x09, 0x05, 0x03};
    case '8': return {0x36, 0x49, 0x49, 0x49, 0x36};
    case '9': return {0x26, 0x49, 0x49, 0x49, 0x3E};
    case '.': return {0x00, 0x60, 0x60, 0x00, 0x00};
    case ':': return {0x00, 0x36, 0x36, 0x00, 0x00};
    case '/': return {0x20, 0x10, 0x08, 0x04, 0x02};
    case '\\': return {0x02, 0x04, 0x08, 0x10, 0x20};
    case '-': return {0x08, 0x08, 0x08, 0x08, 0x08};
    case '_': return {0x40, 0x40, 0x40, 0x40, 0x40};
    case '[': return {0x00, 0x7F, 0x41, 0x41, 0x00};
    case ']': return {0x00, 0x41, 0x41, 0x7F, 0x00};
    case '(': return {0x00, 0x1C, 0x22, 0x41, 0x00};
    case ')': return {0x00, 0x41, 0x22, 0x1C, 0x00};
    case '+': return {0x08, 0x08, 0x3E, 0x08, 0x08};
    case '?': return {0x02, 0x01, 0x51, 0x09, 0x06};
    case ' ': return {0, 0, 0, 0, 0};
    default: return {0x02, 0x01, 0x51, 0x09, 0x06};
    }
}

void draw_text(SDL_Renderer* renderer, int x, int y, std::string_view text, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (const char character : text) {
        const Glyph glyph = glyph_for(character);
        for (int column = 0; column < static_cast<int>(glyph.size()); ++column) {
            for (int row = 0; row < 7; ++row) {
                if ((glyph[column] & (1U << row)) == 0) {
                    continue;
                }
                const SDL_Rect pixel{
                    x + column * FontScale,
                    y + row * FontScale,
                    FontScale,
                    FontScale};
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
        x += GlyphWidth;
    }
}

std::string lowercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool is_rom(const std::string& name)
{
    const std::string lower = lowercase(name);
    return lower.ends_with(".gb") || lower.ends_with(".gbc");
}

std::string join_path(const std::string& directory, const std::string& name)
{
    return !directory.empty() && directory.back() == '/'
        ? directory + name
        : directory + "/" + name;
}

bool is_device_root(const std::string& path)
{
    const std::size_t colon = path.find(':');
    return colon != std::string::npos && path.size() == colon + 2 && path.back() == '/';
}

std::string parent_path(std::string path)
{
    if (is_device_root(path)) {
        return {};
    }
    while (!path.empty() && path.back() == '/') {
        path.pop_back();
    }
    const std::size_t separator = path.find_last_of('/');
    if (separator == std::string::npos) {
        return {};
    }
    path.resize(separator + 1);
    return path;
}

bool path_is_directory(const std::string& path, const dirent& item)
{
    if (item.d_type == DT_DIR) {
        return true;
    }
    if (item.d_type != DT_UNKNOWN) {
        return false;
    }
    struct stat status {};
    return stat(path.c_str(), &status) == 0 && S_ISDIR(status.st_mode);
}

std::vector<Entry> device_entries()
{
    return {
        {"USB (MASS0:/)", "mass0:/", true},
        {"HOST (HOST:/)", "host:/", true},
        {"MEMORY CARD 1 (MC0:/)", "mc0:/", true},
        {"MEMORY CARD 2 (MC1:/)", "mc1:/", true},
        {"CD/DVD (CDFS:/)", "cdfs:/", true},
    };
}

bool read_directory(const std::string& path, std::vector<Entry>& entries)
{
    DIR* directory = opendir(path.c_str());
    if (directory == nullptr) {
        return false;
    }

    entries.clear();
    while (dirent* item = readdir(directory)) {
        const std::string name = item->d_name;
        if (name.empty() || name == "." || name == "..") {
            continue;
        }

        const std::string full_path = join_path(path, name);
        const bool directory_entry = path_is_directory(full_path, *item);
        if (directory_entry || is_rom(name)) {
            entries.push_back({name, full_path, directory_entry});
        }
    }
    closedir(directory);

    std::sort(entries.begin(), entries.end(), [](const Entry& left, const Entry& right) {
        if (left.directory != right.directory) {
            return left.directory > right.directory;
        }
        return lowercase(left.name) < lowercase(right.name);
    });
    return true;
}

std::string clipped(std::string text, std::size_t maximum)
{
    if (text.size() <= maximum) {
        return text;
    }
    text.resize(maximum - 3);
    return text + "...";
}

void render_menu(SDL_Renderer* renderer,
                 const std::string& path,
                 const std::vector<Entry>& entries,
                 std::size_t selected,
                 std::string_view status)
{
    SDL_SetRenderDrawColor(renderer, 8, 14, 28, 255);
    SDL_RenderClear(renderer);

    const SDL_Rect header{0, 0, ScreenWidth, 68};
    SDL_SetRenderDrawColor(renderer, 25, 72, 132, 255);
    SDL_RenderFillRect(renderer, &header);
    draw_text(renderer, 24, 16, "GBSUNNY", {255, 224, 80, 255});
    draw_text(renderer, 24, 43,
              path.empty() ? "SELECT A DEVICE" : clipped(path, 48),
              {220, 232, 255, 255});

    const std::size_t first = selected >= VisibleRows ? selected - VisibleRows + 1 : 0;
    const std::size_t last = std::min(entries.size(), first + VisibleRows);
    for (std::size_t index = first; index < last; ++index) {
        const int y = ListTop + static_cast<int>(index - first) * LineHeight;
        if (index == selected) {
            const SDL_Rect highlight{16, y - 3, ScreenWidth - 32, LineHeight};
            SDL_SetRenderDrawColor(renderer, 44, 108, 180, 255);
            SDL_RenderFillRect(renderer, &highlight);
        }

        const std::string prefix = entries[index].directory ? "[DIR] " : "      ";
        const SDL_Color color = entries[index].directory
            ? SDL_Color{128, 210, 255, 255}
            : SDL_Color{245, 245, 245, 255};
        draw_text(renderer, 24, y, clipped(prefix + entries[index].name, 48), color);
    }

    if (entries.empty()) {
        draw_text(renderer, 24, ListTop, "NO ROMS OR DIRECTORIES", {180, 190, 205, 255});
    }

    const SDL_Rect footer{0, ScreenHeight - 38, ScreenWidth, 38};
    SDL_SetRenderDrawColor(renderer, 15, 30, 52, 255);
    SDL_RenderFillRect(renderer, &footer);
    draw_text(renderer, 18, ScreenHeight - 27, status, {255, 190, 90, 255});
    draw_text(renderer, 294, ScreenHeight - 27, "X SELECT   O BACK", {210, 220, 235, 255});
    SDL_RenderPresent(renderer);
}

enum class Action { None, Up, Down, PageUp, PageDown, Select, Back, Quit };

Action controller_action(const SDL_ControllerButtonEvent& event)
{
    switch (event.button) {
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return Action::Up;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return Action::Down;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return Action::PageUp;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return Action::PageDown;
    case SDL_CONTROLLER_BUTTON_A: return Action::Select;
    case SDL_CONTROLLER_BUTTON_B: return Action::Back;
    default: return Action::None;
    }
}

Action keyboard_action(const SDL_KeyboardEvent& event)
{
    switch (event.keysym.sym) {
    case SDLK_UP: return Action::Up;
    case SDLK_DOWN: return Action::Down;
    case SDLK_LEFT: return Action::PageUp;
    case SDLK_RIGHT: return Action::PageDown;
    case SDLK_RETURN: return Action::Select;
    case SDLK_ESCAPE: return Action::Back;
    default: return Action::None;
    }
}

Action wait_for_action()
{
    SDL_Event event {};
    while (SDL_WaitEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            return Action::Quit;
        }
        if (event.type == SDL_CONTROLLERBUTTONDOWN) {
            return controller_action(event.cbutton);
        }
        if (event.type == SDL_KEYDOWN) {
            return keyboard_action(event.key);
        }
    }
    return Action::Quit;
}

void close_controller(SDL_GameController* controller)
{
    if (controller != nullptr) {
        SDL_GameControllerClose(controller);
    }
}

} // namespace

std::optional<std::string> select_rom(SDL_Renderer* renderer)
{
    SDL_GameController* controller = nullptr;
    for (int index = 0; index < SDL_NumJoysticks(); ++index) {
        if (SDL_IsGameController(index) == SDL_TRUE) {
            controller = SDL_GameControllerOpen(index);
            break;
        }
    }

    std::string current_path;
    std::string status = "D-PAD MOVE";
    std::vector<Entry> entries = device_entries();
    std::size_t selected = 0;

    for (;;) {
        selected = entries.empty() ? 0 : std::min(selected, entries.size() - 1);
        render_menu(renderer, current_path, entries, selected, status);

        switch (wait_for_action()) {
        case Action::Up:
            if (!entries.empty()) {
                selected = selected == 0 ? entries.size() - 1 : selected - 1;
            }
            break;
        case Action::Down:
            if (!entries.empty()) {
                selected = (selected + 1) % entries.size();
            }
            break;
        case Action::PageUp:
            selected = selected > VisibleRows ? selected - VisibleRows : 0;
            break;
        case Action::PageDown:
            if (!entries.empty()) {
                selected = std::min(selected + VisibleRows, entries.size() - 1);
            }
            break;
        case Action::Select:
            if (entries.empty()) {
                break;
            }
            if (!entries[selected].directory) {
                const std::string result = entries[selected].path;
                close_controller(controller);
                return result;
            }
            {
                std::vector<Entry> next_entries;
                if (read_directory(entries[selected].path, next_entries)) {
                    current_path = entries[selected].path;
                    entries = std::move(next_entries);
                    selected = 0;
                    status = "X SELECT   O BACK";
                } else {
                    status = "DEVICE OR DIRECTORY UNAVAILABLE";
                }
            }
            break;
        case Action::Back:
            if (current_path.empty()) {
                close_controller(controller);
                return std::nullopt;
            }
            {
                const std::string parent = parent_path(current_path);
                if (parent.empty()) {
                    current_path.clear();
                    entries = device_entries();
                } else {
                    std::vector<Entry> parent_entries;
                    if (read_directory(parent, parent_entries)) {
                        current_path = parent;
                        entries = std::move(parent_entries);
                    }
                }
                selected = 0;
                status = "X SELECT   O BACK";
            }
            break;
        case Action::Quit:
            close_controller(controller);
            return std::nullopt;
        case Action::None:
            break;
        }
    }
}

} // namespace ps2_frontend
