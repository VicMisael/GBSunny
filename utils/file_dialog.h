#pragma once

#include <string>
#include <vector>

// Wrapper for portable-file-dialogs to keep Windows headers out of main.cpp
std::vector<std::string> open_file_dialog(const std::string& title,
										  const std::string& default_path,
										  const std::vector<std::string>& filters);
