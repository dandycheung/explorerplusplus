// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserWindow.h"
#include <gmock/gmock.h>

class BrowserWindowMock : public BrowserWindow
{
public:
	// BrowserWindow
	MOCK_METHOD(BrowserCommandController *, GetCommandController, (), (override));
	MOCK_METHOD(BrowserPane *, GetActivePane, (), (const, override));
	MOCK_METHOD(void, FocusActiveTab, (), (override));
	MOCK_METHOD(ShellBrowser *, GetActiveShellBrowser, (), (override));
	MOCK_METHOD(HWND, GetHWND, (), (const, override));

	// Navigator
	MOCK_METHOD(void, OpenItem,
		(const std::wstring &itemPath, OpenFolderDisposition openFolderDisposition), (override));
	MOCK_METHOD(void, OpenItem,
		(PCIDLIST_ABSOLUTE pidlItem, OpenFolderDisposition openFolderDisposition), (override));
};
