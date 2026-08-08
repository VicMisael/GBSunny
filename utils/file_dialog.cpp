#include "file_dialog.h"

#if defined(__has_include)
#  if __has_include("portable-file-dialogs.h")
#    include "portable-file-dialogs.h"
#    define HAVE_PFD 1
#  endif
#endif

#if defined(_WIN32) && !defined(HAVE_PFD)
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <memory>
#endif

#include <string>
#include <vector>

std::vector<std::string> open_file_dialog(const std::string& title,
										  const std::string& default_path,
										  const std::vector<std::string>& filters)
{
#if defined(HAVE_PFD)
	try {
		auto result = pfd::open_file(title, default_path, filters, pfd::opt::none).result();
		return result;
	}
	catch (...) {
		return {};
	}
#elif defined(_WIN32)
	// Simple Win32 OPENFILENAME-based dialog for single selection
	OPENFILENAMEA ofn;
	CHAR szFile[MAX_PATH] = {0};

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = default_path.empty() ? nullptr : default_path.c_str();
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	std::vector<std::string> result;
	if (GetOpenFileNameA(&ofn)) {
		result.emplace_back(std::string(ofn.lpstrFile));
	}
	return result;
#else
	// Not implemented on this platform
	(void)title; (void)default_path; (void)filters;
	return {};
#endif
}
