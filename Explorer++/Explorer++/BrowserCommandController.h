// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "NavigationHelper.h"
#include "ShellBrowser/SortModes.h"
#include "../Helper/ShellHelper.h"
#include <boost/core/noncopyable.hpp>

class BrowserWindow;
class ClipboardStore;
struct Config;
class ResourceLoader;
class ShellBrowser;

class BrowserCommandController : private boost::noncopyable
{
public:
	BrowserCommandController(BrowserWindow *browser, Config *config, ClipboardStore *clipboardStore,
		const ResourceLoader *resourceLoader);

	bool IsCommandEnabled(int command) const;
	void ExecuteCommand(int command,
		OpenFolderDisposition disposition = OpenFolderDisposition::CurrentTab);

private:
	static constexpr wchar_t DOCUMENTATION_URL[] =
		L"https://explorerplusplus.readthedocs.io/en/latest/";

	bool IsCommandContextSensitive(int command) const;

	bool CanStartCommandPrompt() const;

	void OnSortBy(SortMode sortMode);
	void StartCommandPrompt(LaunchProcessFlags flags = LaunchProcessFlags::None);
	void CopyFolderPath() const;
	void OnChangeDisplayColors();
	void GoBack(OpenFolderDisposition disposition);
	void GoForward(OpenFolderDisposition disposition);
	void GoUp(OpenFolderDisposition disposition);
	void GoToPath(const std::wstring &path, OpenFolderDisposition disposition);
	void GoToKnownFolder(REFKNOWNFOLDERID knownFolderId, OpenFolderDisposition disposition);
	void OnOpenOnlineDocumentation();
	void OnCheckForUpdates();
	void OnAbout();

	ShellBrowser *GetActiveShellBrowser();
	const ShellBrowser *GetActiveShellBrowser() const;

	BrowserWindow *const m_browser;
	Config *const m_config;
	ClipboardStore *const m_clipboardStore;
	const ResourceLoader *const m_resourceLoader;
};
