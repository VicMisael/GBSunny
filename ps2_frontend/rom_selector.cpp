#include "rom_selector.h"
#include "ps2_font.h"

#include <SDL.h>

#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>

namespace ps2_frontend {
namespace {

constexpr int ScreenWidth = 640;
constexpr int ScreenHeight = 448;
constexpr int LineHeight = 18;
constexpr int ListTop = 91;
constexpr int VisibleRows = 17;

struct Entry {
    std::string name;
    std::string path;
    bool directory;
};

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
    font::draw_text(renderer, 24, 16, "GBSUNNY", {255, 224, 80, 255});
    font::draw_text(renderer, 24, 43,
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
        font::draw_text(renderer, 24, y, clipped(prefix + entries[index].name, 48), color);
    }

    if (entries.empty()) {
        font::draw_text(renderer, 24, ListTop, "NO ROMS OR DIRECTORIES", {180, 190, 205, 255});
    }

    const SDL_Rect footer{0, ScreenHeight - 38, ScreenWidth, 38};
    SDL_SetRenderDrawColor(renderer, 15, 30, 52, 255);
    SDL_RenderFillRect(renderer, &footer);
    font::draw_text(renderer, 18, ScreenHeight - 27, status, {255, 190, 90, 255});
    font::draw_text(renderer, 294, ScreenHeight - 27, "X SELECT   O BACK", {210, 220, 235, 255});
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
