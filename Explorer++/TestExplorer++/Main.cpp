// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include <gtest/gtest.h>
#include <gdiplus.h>

using namespace testing;

class ComEnvironment : public Environment
{
public:
	void SetUp() override
	{
		// Technically, an STA thread requires that messages be pumped, which isn't something that's
		// done here. That should be ok, as it shouldn't be needed for the few specific COM-related
		// methods that are called (e.g. SHCreateShellItemArrayFromIDLists).
		// It might be a problem if a test were to create a COM object on the main thread, marshal
		// that object to another thread, then invoke a method on it from that second thread. But
		// that's not something that's explicitly done and it's unlikely that any of the COM-related
		// methods that are being called are doing that either.
		// Ultimately, COM needs to be initialized for some of the methods called within the tests
		// to succeed.
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		ASSERT_HRESULT_SUCCEEDED(hr);
	}

	void TearDown() override
	{
		CoUninitialize();
	}
};

// A few tests rely on being able to use GDI+.
class GdiplusEnvironment : public Environment
{
public:
	void SetUp() override
	{
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::Status status =
			Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
		ASSERT_EQ(status, Gdiplus::Ok);
	}

	void TearDown() override
	{
		Gdiplus::GdiplusShutdown(m_gdiplusToken);
	}

private:
	ULONG_PTR m_gdiplusToken;
};

int wmain(int argc, wchar_t *argv[])
{
	AddGlobalTestEnvironment(new ComEnvironment);
	AddGlobalTestEnvironment(new GdiplusEnvironment);
	InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
