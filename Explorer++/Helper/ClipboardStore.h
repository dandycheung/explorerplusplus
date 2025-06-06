// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <wil/resource.h>

// Represents an underlying clipboard backing store. A derived class might simply forward these
// calls to the system clipboard, but it could also implement the methods itself, to provide a
// simulated clipboard.
class ClipboardStore
{
public:
	virtual ~ClipboardStore() = default;

	virtual bool Open() = 0;
	virtual bool Close() = 0;

	virtual bool IsDataAvailable(UINT format) const = 0;
	virtual wil::unique_hglobal GetData(UINT format) const = 0;
	virtual bool SetData(UINT format, wil::unique_hglobal global) = 0;

	virtual wil::com_ptr_nothrow<IDataObject> GetDataObject() const = 0;
	virtual bool SetDataObject(IDataObject *dataObject) = 0;
	virtual bool IsDataObjectCurrent(IDataObject *dataObject) const = 0;
	virtual bool FlushDataObject() = 0;

	virtual bool Clear() = 0;
};
