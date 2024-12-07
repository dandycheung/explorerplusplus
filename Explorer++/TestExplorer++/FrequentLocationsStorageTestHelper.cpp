// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsStorageTestHelper.h"
#include "FrequentLocationsModel.h"
#include "ShellTestHelper.h"

bool operator==(const FrequentLocationsModel &first, const FrequentLocationsModel &second)
{
	return first.GetVisits() == second.GetVisits();
}

namespace FrequentLocationsStorageTestHelper
{

void BuildReferenceModel(FrequentLocationsModel *frequentLocationsModel)
{
	using namespace std::chrono_literals;

	frequentLocationsModel->SetLocationVisits(
		{ BuildFrequentLocation(L"c:\\fake1", 20,
			  LocationVisitInfo::Clock::time_point(1733488834758531us)),
			BuildFrequentLocation(L"c:\\fake2", 3,
				LocationVisitInfo::Clock::time_point(1732110119269690us)),
			BuildFrequentLocation(L"c:\\fake3", 65,
				LocationVisitInfo::Clock::time_point(1717940540499464us)) });
}

LocationVisitInfo BuildFrequentLocation(const std::wstring &location, int numVisits,
	const LocationVisitInfo::Clock::time_point &lastVisitTime)
{
	return { CreateSimplePidlForTest(location, nullptr, ShellItemType::Folder), numVisits,
		lastVisitTime };
}

}