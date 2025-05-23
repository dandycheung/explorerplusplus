// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/NavigateParams.h"
#include "ShellBrowser/NavigationManager.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/PidlHelper.h"
#include <concurrencpp/concurrencpp.h>
#include <memory>

class NavigationEvents;
class PreservedHistoryEntry;
class ShellEnumeratorFake;
class ShellNavigationController;
class TabNavigationInterface;

class ShellBrowserFake : public ShellBrowser
{
public:
	ShellBrowserFake(NavigationEvents *navigationEvents, TabNavigationInterface *tabNavigation,
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &preservedEntries,
		int currentEntry, std::shared_ptr<concurrencpp::executor> enumerationExecutor = nullptr,
		std::shared_ptr<concurrencpp::executor> originalExecutor = nullptr);
	ShellBrowserFake(NavigationEvents *navigationEvents, TabNavigationInterface *tabNavigation,
		std::shared_ptr<concurrencpp::executor> enumerationExecutor = nullptr,
		std::shared_ptr<concurrencpp::executor> originalExecutor = nullptr);
	~ShellBrowserFake();

	void NavigateToPath(const std::wstring &path,
		HistoryEntryType addHistoryType = HistoryEntryType::AddEntry,
		PidlAbsolute *outputPidl = nullptr);

	// ShellBrowser
	FolderSettings GetFolderSettings() const override;
	ShellNavigationController *GetNavigationController() const override;
	ViewMode GetViewMode() const override;
	void SetViewMode(ViewMode viewMode) override;
	bool CanCreateNewFolder() const override;
	void CreateNewFolder() override;
	bool CanSplitFile() const override;
	void SplitFile() override;
	bool CanMergeFiles() const override;
	void MergeFiles() override;
	void SelectAllItems() override;
	void InvertSelection() override;
	void ClearSelection() override;

protected:
	NavigationManager *GetNavigationManager() override;
	const NavigationManager *GetNavigationManager() const override;

private:
	const std::shared_ptr<ShellEnumeratorFake> m_shellEnumerator;
	const std::shared_ptr<concurrencpp::inline_executor> m_inlineExecutor;
	NavigationManager m_navigationManager;
	std::unique_ptr<ShellNavigationController> m_navigationController;
	ViewMode m_viewMode = ViewMode::Icons;
};
