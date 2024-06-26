// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsService.h"
#include "ShellTestHelper.h"
#include "../Helper/StringHelper.h"
#include <gtest/gtest.h>

using namespace testing;

bool operator==(const LocationVisitInfo &locationInfo1, const LocationVisitInfo &locationInfo2)
{
	// The last visit time is ignored here.
	return locationInfo1.GetLocation() == locationInfo2.GetLocation()
		&& locationInfo1.GetNumVisits() == locationInfo2.GetNumVisits();
}

HRESULT PrintPath(const LocationVisitInfo &locationInfo, std::ostream *os)
{
	std::wstring parsingPath;
	HRESULT hr = GetDisplayName(locationInfo.GetLocation().Raw(), SHGDN_FORPARSING, parsingPath);

	if (FAILED(hr))
	{
		return hr;
	}

	auto narrowPath = WstrToStr(parsingPath);

	if (!narrowPath)
	{
		return E_FAIL;
	}

	*os << *narrowPath;

	return S_OK;
}

void PrintTo(const LocationVisitInfo &locationInfo, std::ostream *os)
{
	*os << "LocationVisitInfo(";

	HRESULT hr = PrintPath(locationInfo, os);

	if (FAILED(hr))
	{
		*os << "(Unknown path)";
	}

	*os << ", " << locationInfo.GetNumVisits();
	*os << ")";
}

TEST(FrequentLocationsServiceTest, DifferentLocations)
{
	FrequentLocationsService frequentLocationService;

	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	frequentLocationService.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	frequentLocationService.RegisterLocationVisit(fake2);

	PidlAbsolute fake3 = CreateSimplePidlForTest(L"C:\\Fake3");
	frequentLocationService.RegisterLocationVisit(fake3);

	const auto &visits = frequentLocationService.GetVisits();

	// Items with the same visit count are sorted by their last visit time, so more recent items
	// should appear first.
	std::vector<LocationVisitInfo> expectedVisits = { { fake3, 1 }, { fake2, 1 }, { fake1, 1 } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));
}

TEST(FrequentLocationsServiceTest, RepeatedVisits)
{
	FrequentLocationsService frequentLocationService;

	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	frequentLocationService.RegisterLocationVisit(fake1);
	frequentLocationService.RegisterLocationVisit(fake1);
	frequentLocationService.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	frequentLocationService.RegisterLocationVisit(fake2);

	const auto &visits = frequentLocationService.GetVisits();

	std::vector<LocationVisitInfo> expectedVisits = { { fake1, 3 }, { fake2, 1 } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));
}

TEST(FrequentLocationsServiceTest, VisitCountOrderChanges)
{
	FrequentLocationsService frequentLocationService;

	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	frequentLocationService.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	frequentLocationService.RegisterLocationVisit(fake2);
	frequentLocationService.RegisterLocationVisit(fake2);

	const auto &visits = frequentLocationService.GetVisits();

	std::vector<LocationVisitInfo> expectedVisits = { { fake2, 2 }, { fake1, 1 } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));

	frequentLocationService.RegisterLocationVisit(fake1);
	frequentLocationService.RegisterLocationVisit(fake1);

	expectedVisits = { { fake1, 3 }, { fake2, 2 } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));

	frequentLocationService.RegisterLocationVisit(fake2);
	frequentLocationService.RegisterLocationVisit(fake2);

	expectedVisits = { { fake2, 4 }, { fake1, 3 } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));
}

TEST(FrequentLocationsServiceTest, VisitTimeOrderChanges)
{
	FrequentLocationsService frequentLocationService;

	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	frequentLocationService.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	frequentLocationService.RegisterLocationVisit(fake2);

	frequentLocationService.RegisterLocationVisit(fake1);
	frequentLocationService.RegisterLocationVisit(fake2);

	const auto &visits = frequentLocationService.GetVisits();

	// fake2 is the most recently visited item, so it should appear first.
	std::vector<LocationVisitInfo> expectedVisits = { { fake2, 2 }, { fake1, 2 } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));

	frequentLocationService.RegisterLocationVisit(fake2);
	frequentLocationService.RegisterLocationVisit(fake1);

	// fake1 is now the most recently visited item.
	expectedVisits = { { fake1, 3 }, { fake2, 3 } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));
}

TEST(FrequentLocationsServiceTest, LocationsChangedEvent)
{
	FrequentLocationsService frequentLocationService;

	MockFunction<void()> callback;
	frequentLocationService.AddLocationsChangedObserver(callback.AsStdFunction());
	EXPECT_CALL(callback, Call()).Times(4);

	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	frequentLocationService.RegisterLocationVisit(fake1);
	frequentLocationService.RegisterLocationVisit(fake1);
	frequentLocationService.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	frequentLocationService.RegisterLocationVisit(fake2);
}
