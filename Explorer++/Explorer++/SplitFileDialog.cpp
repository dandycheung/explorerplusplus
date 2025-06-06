// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SplitFileDialog.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/FileOperations.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/StringHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/XMLSettings.h"
#include <wil/resource.h>
#include <comdef.h>
#include <unordered_map>

namespace NSplitFileDialog
{
const int WM_APP_SETTOTALSPLITCOUNT = WM_APP + 1;
const int WM_APP_SETCURRENTSPLITCOUNT = WM_APP + 2;
const int WM_APP_SPLITFINISHED = WM_APP + 3;
const int WM_APP_INPUTFILEINVALID = WM_APP + 4;

const TCHAR COUNTER_PATTERN[] = _T("/N");

DWORD WINAPI SplitFileThreadProcStub(LPVOID pParam);
}

const TCHAR SplitFileDialogPersistentSettings::SETTINGS_KEY[] = _T("SplitFile");

const TCHAR SplitFileDialogPersistentSettings::SETTING_SIZE[] = _T("Size");
const TCHAR SplitFileDialogPersistentSettings::SETTING_SIZE_GROUP[] = _T("SizeGroup");

SplitFileDialog::SplitFileDialog(const ResourceLoader *resourceLoader, HWND hParent,
	const std::wstring &strFullFilename) :
	BaseDialog(resourceLoader, IDD_SPLITFILE, hParent, DialogSizingType::None),
	m_strFullFilename(strFullFilename),
	m_bSplittingFile(false),
	m_bStopSplitting(false),
	m_pSplitFile(nullptr)
{
	m_persistentSettings = &SplitFileDialogPersistentSettings::GetInstance();
}

SplitFileDialog::~SplitFileDialog()
{
	if (m_pSplitFile != nullptr)
	{
		m_pSplitFile->StopSplitting();
		m_pSplitFile->Release();
	}
}

INT_PTR SplitFileDialog::OnInitDialog()
{
	SHFILEINFO shfi;
	DWORD_PTR dwRes = SHGetFileInfo(m_strFullFilename.c_str(), 0, &shfi, sizeof(shfi), SHGFI_ICON);

	if (dwRes != 0)
	{
		ICONINFO ii;
		GetIconInfo(shfi.hIcon, &ii);
		SendDlgItemMessage(m_hDlg, IDC_SPLIT_STATIC_ICON, STM_SETIMAGE, IMAGE_BITMAP,
			reinterpret_cast<LPARAM>(ii.hbmColor));

		DeleteObject(ii.hbmColor);
		DeleteObject(ii.hbmMask);
		DestroyIcon(shfi.hIcon);
	}

	SetDlgItemText(m_hDlg, IDC_SPLIT_EDIT_FILENAME, m_strFullFilename.c_str());

	HANDLE hFile = CreateFile(m_strFullFilename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, 0, nullptr);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER lFileSize;
		GetFileSizeEx(hFile, &lFileSize);

		auto fileSizeText = FormatSizeString(lFileSize.QuadPart);
		SetDlgItemText(m_hDlg, IDC_SPLIT_EDIT_FILESIZE, fileSizeText.c_str());

		CloseHandle(hFile);
	}

	TCHAR szOutputDirectory[MAX_PATH];
	StringCchCopy(szOutputDirectory, std::size(szOutputDirectory), m_strFullFilename.c_str());
	PathRemoveFileSpec(szOutputDirectory);
	SetDlgItemText(m_hDlg, IDC_SPLIT_EDIT_OUTPUT, szOutputDirectory);

	HWND hComboBox = GetDlgItem(m_hDlg, IDC_SPLIT_COMBOBOX_SIZES);
	int iPos;

	auto text = m_resourceLoader->LoadString(IDS_SPLIT_FILE_SIZE_BYTES);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(text.c_str())));
	m_SizeMap.insert(std::unordered_map<int, SizeType>::value_type(iPos, SizeType::Bytes));
	text = m_resourceLoader->LoadString(IDS_SPLIT_FILE_SIZE_KB);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(text.c_str())));
	m_SizeMap.insert(std::unordered_map<int, SizeType>::value_type(iPos, SizeType::KB));
	text = m_resourceLoader->LoadString(IDS_SPLIT_FILE_SIZE_MB);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(text.c_str())));
	m_SizeMap.insert(std::unordered_map<int, SizeType>::value_type(iPos, SizeType::MB));
	text = m_resourceLoader->LoadString(IDS_SPLIT_FILE_SIZE_GB);
	iPos = static_cast<int>(SendMessage(hComboBox, CB_INSERTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(text.c_str())));
	m_SizeMap.insert(std::unordered_map<int, SizeType>::value_type(iPos, SizeType::GB));

	SendMessage(hComboBox, CB_SELECTSTRING, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(m_persistentSettings->m_strSplitGroup.c_str()));

	HWND hEditSize = GetDlgItem(m_hDlg, IDC_SPLIT_EDIT_SIZE);
	SetWindowText(hEditSize, m_persistentSettings->m_strSplitSize.c_str());
	SendMessage(hEditSize, EM_SETSEL, 0, -1);
	SetFocus(hEditSize);

	TCHAR szOutputFilename[MAX_PATH];
	StringCchCopy(szOutputFilename, std::size(szOutputFilename), m_strFullFilename.c_str());
	PathStripPath(szOutputFilename);
	StringCchPrintf(szOutputFilename, std::size(szOutputFilename), _T("%s.part%s"),
		szOutputFilename, NSplitFileDialog::COUNTER_PATTERN);
	SetDlgItemText(m_hDlg, IDC_SPLIT_EDIT_OUTPUTFILENAME, szOutputFilename);

	auto hCurentFont = reinterpret_cast<HFONT>(
		SendDlgItemMessage(m_hDlg, IDC_SPLIT_STATIC_FILENAMEHELPER, WM_GETFONT, 0, 0));

	LOGFONT lf;
	GetObject(hCurentFont, sizeof(lf), reinterpret_cast<LPVOID>(&lf));

	lf.lfItalic = TRUE;
	m_hHelperTextFont = CreateFontIndirect(&lf);

	SendDlgItemMessage(m_hDlg, IDC_SPLIT_STATIC_FILENAMEHELPER, WM_SETFONT,
		reinterpret_cast<WPARAM>(m_hHelperTextFont), MAKEWORD(TRUE, 0));

	SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_ELAPSEDTIME, _T("00:00:00"));

	m_persistentSettings->RestoreDialogPosition(m_hDlg, false);

	return 0;
}

wil::unique_hicon SplitFileDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_resourceLoader->LoadIconFromPNGAndScale(Icon::SplitFiles, iconWidth, iconHeight);
}

INT_PTR SplitFileDialog::OnTimer(int iTimerID)
{
	if (iTimerID == ELPASED_TIMER_ID)
	{
		m_uElapsedTime++;

		/* Update the elapsed time display (form is hh:mm:ss). */
		TCHAR szElapsedTime[9];
		StringCchPrintf(szElapsedTime, std::size(szElapsedTime), _T("%02d:%02d:%02d"),
			m_uElapsedTime / 3600, (m_uElapsedTime / 60) % 60, m_uElapsedTime % 60);
		SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_ELAPSEDTIME, szElapsedTime);
	}

	return 0;
}

INT_PTR SplitFileDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (lParam != 0)
	{
		switch (LOWORD(wParam))
		{
		case IDC_SPLIT_BUTTON_OUTPUT:
			OnChangeOutputDirectory();
			break;

		case IDOK:
			OnOk();
			break;

		case IDCANCEL:
			OnCancel();
			break;
		}
	}

	return 0;
}

INT_PTR SplitFileDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

INT_PTR SplitFileDialog::OnDestroy()
{
	DeleteObject(m_hHelperTextFont);

	return 0;
}

void SplitFileDialog::SaveState()
{
	m_persistentSettings->SaveDialogPosition(m_hDlg);

	m_persistentSettings->m_strSplitSize = GetWindowString(GetDlgItem(m_hDlg, IDC_SPLIT_EDIT_SIZE));
	m_persistentSettings->m_strSplitGroup =
		GetWindowString(GetDlgItem(m_hDlg, IDC_SPLIT_COMBOBOX_SIZES));

	m_persistentSettings->m_bStateSaved = TRUE;
}

INT_PTR SplitFileDialog::OnPrivateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (uMsg)
	{
	case NSplitFileDialog::WM_APP_SETTOTALSPLITCOUNT:
		SendDlgItemMessage(m_hDlg, IDC_SPLIT_PROGRESS, PBM_SETRANGE32, 0, wParam);
		break;

	case NSplitFileDialog::WM_APP_SETCURRENTSPLITCOUNT:
		SendDlgItemMessage(m_hDlg, IDC_SPLIT_PROGRESS, PBM_SETPOS, wParam, 0);
		break;

	case NSplitFileDialog::WM_APP_SPLITFINISHED:
		OnSplitFinished();
		break;

	case NSplitFileDialog::WM_APP_INPUTFILEINVALID:
	{
		auto errorMessage = m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_INPUTFILEINVALID);
		SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_MESSAGE, errorMessage.c_str());

		assert(m_pSplitFile != nullptr);

		m_pSplitFile->Release();
		m_pSplitFile = nullptr;

		m_bSplittingFile = false;
		m_bStopSplitting = false;

		KillTimer(m_hDlg, ELPASED_TIMER_ID);

		SetDlgItemText(m_hDlg, IDOK, m_szOk);
	}
	break;
	}

	return 0;
}

void SplitFileDialog::OnOk()
{
	if (!m_bSplittingFile)
	{
		HWND hOutputFilename = GetDlgItem(m_hDlg, IDC_SPLIT_EDIT_OUTPUTFILENAME);

		if (GetWindowTextLength(hOutputFilename) == 0)
		{
			auto errorMessage =
				m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_OUTPUTFILENAMEERROR);
			SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_MESSAGE, errorMessage.c_str());

			SetFocus(hOutputFilename);
			return;
		}

		std::wstring strOutputFilename = GetWindowString(hOutputFilename);

		/* Now, check that the filename has a variable component. Without the
		variable component, the filenames of all the split files would be exactly
		the same. */
		if (strOutputFilename.find(NSplitFileDialog::COUNTER_PATTERN) == std::wstring::npos)
		{
			auto errorMessage =
				m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_OUTPUTFILENAMECONSTANTERROR);
			SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_MESSAGE, errorMessage.c_str());

			SetFocus(hOutputFilename);
			return;
		}

		HWND hEditOutputDirectory = GetDlgItem(m_hDlg, IDC_SPLIT_EDIT_OUTPUT);

		if (GetWindowTextLength(hEditOutputDirectory) == 0)
		{
			auto errorMessage =
				m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_OUTPUTDIRECTORYERROR);
			SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_MESSAGE, errorMessage.c_str());

			SetFocus(hEditOutputDirectory);
			return;
		}

		std::wstring strOutputDirectory = GetWindowString(hEditOutputDirectory);

		BOOL bTranslated;
		UINT uSplitSize = GetDlgItemInt(m_hDlg, IDC_SPLIT_EDIT_SIZE, &bTranslated, FALSE);

		if (!bTranslated || uSplitSize == 0)
		{
			auto errorMessage = m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_SIZEERROR);
			SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_MESSAGE, errorMessage.c_str());

			SetFocus(GetDlgItem(m_hDlg, IDC_SPLIT_EDIT_SIZE));
			return;
		}

		HWND hComboBox = GetDlgItem(m_hDlg, IDC_SPLIT_COMBOBOX_SIZES);
		int iCurSel = static_cast<int>(SendMessage(hComboBox, CB_GETCURSEL, 0, 0));

		auto itr = m_SizeMap.find(iCurSel);

		if (itr != m_SizeMap.end())
		{
			switch (itr->second)
			{
			case SizeType::Bytes:
				/* Nothing needs to be done, as the selection
				is in bytes. */
				break;

			case SizeType::KB:
				uSplitSize *= KB;
				break;

			case SizeType::MB:
				uSplitSize *= MB;
				break;

			case SizeType::GB:
				uSplitSize *= GB;
				break;
			}
		}

		m_pSplitFile = new SplitFile(m_hDlg, m_strFullFilename, strOutputFilename,
			strOutputDirectory, uSplitSize);

		GetDlgItemText(m_hDlg, IDOK, m_szOk, static_cast<int>(std::size(m_szOk)));

		auto stopText = m_resourceLoader->LoadString(IDS_STOP);
		SetDlgItemText(m_hDlg, IDOK, stopText.c_str());

		m_bSplittingFile = true;

		m_uElapsedTime = 0;
		SetTimer(m_hDlg, ELPASED_TIMER_ID, ELPASED_TIMER_TIMEOUT, nullptr);

		auto splittingText = m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_SPLITTING);
		SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_MESSAGE, splittingText.c_str());

		HANDLE hThread = CreateThread(nullptr, 0, NSplitFileDialog::SplitFileThreadProcStub,
			reinterpret_cast<LPVOID>(m_pSplitFile), 0, nullptr);
		SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);
		CloseHandle(hThread);
	}
	else
	{
		m_bStopSplitting = true;

		if (m_pSplitFile != nullptr)
		{
			m_pSplitFile->StopSplitting();
		}
	}
}

void SplitFileDialog::OnCancel()
{
	if (m_bSplittingFile)
	{
		m_bStopSplitting = true;
	}
	else
	{
		EndDialog(m_hDlg, 0);
	}
}

void SplitFileDialog::OnChangeOutputDirectory()
{
	auto title = m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_DIRECTORYTITLE);

	unique_pidl_absolute pidl;
	BOOL bSucceeded = FileOperations::CreateBrowseDialog(m_hDlg, title, wil::out_param(pidl));

	if (!bSucceeded)
	{
		return;
	}

	std::wstring parsingName;
	HRESULT hr = GetDisplayName(pidl.get(), SHGDN_FORPARSING, parsingName);

	if (FAILED(hr))
	{
		return;
	}

	SetDlgItemText(m_hDlg, IDC_SPLIT_EDIT_OUTPUT, parsingName.c_str());
}

void SplitFileDialog::OnSplitFinished()
{
	std::wstring message;

	if (!m_bStopSplitting)
	{
		message = m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_FINISHED);
	}
	else
	{
		message = m_resourceLoader->LoadString(IDS_SPLITFILEDIALOG_CANCELLED);
	}

	SetDlgItemText(m_hDlg, IDC_SPLIT_STATIC_MESSAGE, message.c_str());

	assert(m_pSplitFile != nullptr);

	m_pSplitFile->Release();
	m_pSplitFile = nullptr;

	m_bSplittingFile = false;
	m_bStopSplitting = false;

	KillTimer(m_hDlg, ELPASED_TIMER_ID);

	int iHighLimit =
		static_cast<int>(SendDlgItemMessage(m_hDlg, IDC_SPLIT_PROGRESS, PBM_GETRANGE, FALSE, 0));
	SendDlgItemMessage(m_hDlg, IDC_SPLIT_PROGRESS, PBM_SETPOS, iHighLimit, 0);

	SetDlgItemText(m_hDlg, IDOK, m_szOk);
}

DWORD WINAPI NSplitFileDialog::SplitFileThreadProcStub(LPVOID pParam)
{
	assert(pParam != nullptr);

	auto *pSplitFile = reinterpret_cast<SplitFile *>(pParam);
	pSplitFile->Split();

	return 0;
}

SplitFile::SplitFile(HWND hDlg, const std::wstring &strFullFilename,
	const std::wstring &strOutputFilename, const std::wstring &strOutputDirectory, UINT uSplitSize)
{
	m_hDlg = hDlg;
	m_strFullFilename = strFullFilename;
	m_strOutputFilename = strOutputFilename;
	m_strOutputDirectory = strOutputDirectory;
	m_uSplitSize = uSplitSize;

	m_bStopSplitting = false;

	InitializeCriticalSection(&m_csStop);
}

SplitFile::~SplitFile()
{
	DeleteCriticalSection(&m_csStop);
}

void SplitFile::Split()
{
	HANDLE hInputFile = CreateFile(m_strFullFilename.c_str(), GENERIC_READ, FILE_SHARE_READ,
		nullptr, OPEN_EXISTING, 0, nullptr);

	if (hInputFile == INVALID_HANDLE_VALUE)
	{
		PostMessage(m_hDlg, NSplitFileDialog::WM_APP_INPUTFILEINVALID, 0, 0);
		return;
	}

	LARGE_INTEGER lFileSize;
	GetFileSizeEx(hInputFile, &lFileSize);

	LONGLONG nSplits = lFileSize.QuadPart / m_uSplitSize;

	if ((lFileSize.QuadPart % m_uSplitSize) != 0)
	{
		nSplits++;
	}

	PostMessage(m_hDlg, NSplitFileDialog::WM_APP_SETTOTALSPLITCOUNT, static_cast<WPARAM>(nSplits),
		0);

	SplitInternal(hInputFile, lFileSize);

	CloseHandle(hInputFile);

	SendMessage(m_hDlg, NSplitFileDialog::WM_APP_SPLITFINISHED, 0, 0);
}

void SplitFile::SplitInternal(HANDLE hInputFile, const LARGE_INTEGER &lFileSize)
{
	LARGE_INTEGER lRunningSplitSize = {};

	char *pBuffer = new char[m_uSplitSize];
	bool bStop = false;
	int nSplitsMade = 1;

	while (lRunningSplitSize.QuadPart < lFileSize.QuadPart && !bStop)
	{
		DWORD dwNumberOfBytesRead;
		ReadFile(hInputFile, reinterpret_cast<LPVOID>(pBuffer), m_uSplitSize, &dwNumberOfBytesRead,
			nullptr);

		std::wstring strOutputFullFilename;
		ProcessFilename(nSplitsMade, strOutputFullFilename);

		HANDLE hOutputFile = CreateFile(strOutputFullFilename.c_str(), GENERIC_WRITE, 0, nullptr,
			CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (hOutputFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwNumberOfBytesWritten;
			WriteFile(hOutputFile, reinterpret_cast<LPCVOID>(pBuffer), dwNumberOfBytesRead,
				&dwNumberOfBytesWritten, nullptr);

			CloseHandle(hOutputFile);
		}

		/* TODO: Wait for a set period of time before sending message
		(so as not to block the GUI). */
		PostMessage(m_hDlg, NSplitFileDialog::WM_APP_SETCURRENTSPLITCOUNT, nSplitsMade, 0);

		lRunningSplitSize.QuadPart += dwNumberOfBytesRead;
		nSplitsMade++;

		EnterCriticalSection(&m_csStop);
		if (m_bStopSplitting)
		{
			bStop = true;
		}
		LeaveCriticalSection(&m_csStop);
	}

	delete[] pBuffer;
}

void SplitFile::ProcessFilename(int nSplitsMade, std::wstring &strOutputFullFilename)
{
	std::wstring strOutputFilename = m_strOutputFilename;

	std::wstringstream ss;
	ss << nSplitsMade;
	strOutputFilename.replace(strOutputFilename.find(NSplitFileDialog::COUNTER_PATTERN), 2,
		ss.str());

	strOutputFullFilename = m_strOutputDirectory + _T("\\") + strOutputFilename;
}

void SplitFile::StopSplitting()
{
	EnterCriticalSection(&m_csStop);
	m_bStopSplitting = true;
	LeaveCriticalSection(&m_csStop);
}

SplitFileDialogPersistentSettings::SplitFileDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY)
{
	m_strSplitSize = _T("10");
	m_strSplitGroup = _T("KB");
}

SplitFileDialogPersistentSettings &SplitFileDialogPersistentSettings::GetInstance()
{
	static SplitFileDialogPersistentSettings sfadps;
	return sfadps;
}

void SplitFileDialogPersistentSettings::SaveExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::SaveString(hKey, SETTING_SIZE, m_strSplitSize);
	RegistrySettings::SaveString(hKey, SETTING_SIZE_GROUP, m_strSplitGroup);
}

void SplitFileDialogPersistentSettings::LoadExtraRegistrySettings(HKEY hKey)
{
	RegistrySettings::ReadString(hKey, SETTING_SIZE, m_strSplitSize);
	RegistrySettings::ReadString(hKey, SETTING_SIZE_GROUP, m_strSplitGroup);
}

void SplitFileDialogPersistentSettings::SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom,
	IXMLDOMElement *pParentNode)
{
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SIZE, m_strSplitSize.c_str());
	XMLSettings::AddAttributeToNode(pXMLDom, pParentNode, SETTING_SIZE_GROUP,
		m_strSplitGroup.c_str());
}

void SplitFileDialogPersistentSettings::LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue)
{
	if (lstrcmpi(bstrName, SETTING_SIZE) == 0)
	{
		m_strSplitSize = _bstr_t(bstrValue);
	}
	else if (lstrcmpi(bstrName, SETTING_SIZE_GROUP) == 0)
	{
		m_strSplitGroup = _bstr_t(bstrValue);
	}
}
