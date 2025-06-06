// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <vector>

struct RebarBandStorageInfo;

namespace MainRebarRegistryStorage
{

std::vector<RebarBandStorageInfo> Load(HKEY mainRebarKey);
void Save(HKEY mainRebarKey, const std::vector<RebarBandStorageInfo> &rebarStorageInfo);

}
