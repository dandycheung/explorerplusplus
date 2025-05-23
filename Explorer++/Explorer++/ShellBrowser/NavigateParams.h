// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "HistoryEntry.h"
#include "../Helper/PidlHelper.h"
#include <optional>

class NavigationRequest;

enum class NavigationType
{
	// Standard navigation to a folder.
	Normal,

	// Navigation to a history item (either backwards or forwards).
	History,

	// Navigation that goes up one level.
	Up
};

enum class HistoryEntryType
{
	// No history entry will be added.
	None,

	// A new history entry will be added.
	AddEntry,

	// The current history entry will be replaced. Useful both in situations where the current
	// history entry may be out of date (e.g. a history entry for a past folder may include an out
	// of date display name), as well as situations where the entry needs to be replaced wholesale
	// (e.g. because the current directory has been renamed and the entry needs to be replaced by
	// one that refers to the new location).
	ReplaceCurrentEntry
};

struct NavigateParams
{
public:
	// Navigating to a shell link object (i.e. an object that supports the IShellLink interface),
	// will result in a navigation to the target instead. Therefore, the final pidl may not always
	// match the request pidl.
	PidlAbsolute requestPidl;
	PidlAbsolute pidl;

	std::optional<int> historyEntryId;
	HistoryEntryType historyEntryType = HistoryEntryType::AddEntry;
	NavigationType navigationType = NavigationType::Normal;

	// if true, the navigation will proceed in the current tab, regardless of the navigation target
	// mode in effect. Useful, for example, in situations where the current directory has been
	// renamed or deleted and a navigation to another directory needs to occur within the tab,
	// regardless of the navigation target mode.
	bool overrideNavigationTargetMode = false;

	// When navigating up, this will store the pidl of the previous item.
	PidlAbsolute originalPidl;

	static NavigateParams Normal(PCIDLIST_ABSOLUTE pidl,
		HistoryEntryType historyEntryType = HistoryEntryType::AddEntry)
	{
		NavigateParams params;
		params.requestPidl = pidl;
		params.pidl = pidl;
		params.historyEntryType = historyEntryType;
		params.navigationType = NavigationType::Normal;
		return params;
	}

	static NavigateParams History(const HistoryEntry *historyEntry)
	{
		NavigateParams params;
		params.requestPidl = historyEntry->GetPidl();
		params.pidl = historyEntry->GetPidl();
		params.historyEntryId = historyEntry->GetId();
		params.historyEntryType = HistoryEntryType::ReplaceCurrentEntry;
		params.navigationType = NavigationType::History;
		return params;
	}

	static NavigateParams Up(PCIDLIST_ABSOLUTE pidl, PCIDLIST_ABSOLUTE originalPidl)
	{
		NavigateParams params;
		params.requestPidl = pidl;
		params.pidl = pidl;
		params.originalPidl = originalPidl;
		params.historyEntryType = HistoryEntryType::AddEntry;
		params.navigationType = NavigationType::Up;
		return params;
	}

	// This is only used in tests.
	bool operator==(const NavigateParams &) const = default;

private:
	NavigateParams() = default;
};
