// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabContainerBackgroundContextMenu.h"
#include "Bookmarks/BookmarkHelper.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"
#include "TabContainerImpl.h"
#include "TabRestorer.h"

TabContainerBackgroundContextMenu::TabContainerBackgroundContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, TabContainerImpl *tabContainerImpl,
	TabRestorer *tabRestorer, BookmarkTree *bookmarkTree, BrowserWindow *browser,
	CoreInterface *coreInterface, const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_tabContainerImpl(tabContainerImpl),
	m_tabRestorer(tabRestorer),
	m_bookmarkTree(bookmarkTree),
	m_browser(browser),
	m_coreInterface(coreInterface),
	m_resourceLoader(resourceLoader)
{
	BuildMenu();

	m_connections.push_back(m_menuView->AddItemSelectedObserver(std::bind(
		&TabContainerBackgroundContextMenu::OnMenuItemSelected, this, std::placeholders::_1)));
}

void TabContainerBackgroundContextMenu::BuildMenu()
{
	m_menuView->AppendItem(IDM_TAB_CONTAINER_NEW_TAB,
		m_resourceLoader->LoadString(IDS_TAB_CONTAINER_MENU_NEW_TAB), {}, L"",
		GetAcceleratorTextForId(IDM_FILE_NEWTAB));
	m_menuView->AppendItem(IDM_TAB_CONTAINER_REOPEN_CLOSED_TAB,
		m_resourceLoader->LoadString(IDS_TAB_CONTAINER_MENU_REOPEN_CLOSED_TAB), {}, L"",
		GetAcceleratorTextForId(IDA_RESTORE_LAST_TAB));
	m_menuView->AppendItem(IDM_TAB_CONTAINER_BOOKMARK_ALL_TABS,
		m_resourceLoader->LoadString(IDS_TAB_CONTAINER_MENU_BOOKMARK_ALL_TABS), {}, L"",
		GetAcceleratorTextForId(IDM_BOOKMARKS_BOOKMARK_ALL_TABS));

	if (m_tabRestorer->IsEmpty())
	{
		m_menuView->EnableItem(IDM_TAB_CONTAINER_REOPEN_CLOSED_TAB, false);
	}
}

void TabContainerBackgroundContextMenu::OnMenuItemSelected(UINT menuItemId)
{
	switch (menuItemId)
	{
	case IDM_TAB_CONTAINER_NEW_TAB:
		m_tabContainerImpl->CreateNewTabInDefaultDirectory(TabSettings(_selected = true));
		break;

	case IDM_TAB_CONTAINER_REOPEN_CLOSED_TAB:
		m_tabRestorer->RestoreLastTab();
		break;

	case IDM_TAB_CONTAINER_BOOKMARK_ALL_TABS:
		BookmarkHelper::BookmarkAllTabs(m_bookmarkTree, m_resourceLoader,
			m_coreInterface->GetMainWindow(), m_browser, m_coreInterface, m_acceleratorManager);
		break;

	default:
		DCHECK(false);
		break;
	}
}
