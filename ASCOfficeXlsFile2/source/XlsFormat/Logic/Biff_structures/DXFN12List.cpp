﻿/*
 * (c) Copyright Ascensio System SIA 2010-2019
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * You can contact Ascensio System SIA at 20A-12 Ernesta Birznieka-Upisha
 * street, Riga, Latvia, EU, LV-1050.
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * Pursuant to Section 7(b) of the License you must retain the original Product
 * logo when distributing the program. Pursuant to Section 7(e) we decline to
 * grant you any rights under trademark law for use of our trademarks.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */

#include "DXFN12List.h"

namespace XLS
{


BiffStructurePtr DXFN12List::clone()
{
	return BiffStructurePtr(new DXFN12List(*this));
}

DXFN12List::DXFN12List()
{		
	bExist	= false;
	size	= 0xffffffff;
	dxfId_	= -1;
}


DXFN12List::~DXFN12List()
{
}

void DXFN12List::load(CFRecord& record)
{
	GlobalWorkbookInfoPtr global_info = record.getGlobalWorkbookInfo();

	size_t pos_record = record.getRdPtr();
	
	if (size == 0xffffffff) size = record.getDataSize() - pos_record;

	if (size > 0)
	{
		bExist = true;
		record >> dxfn;
		size -= (record.getRdPtr() - pos_record);
		pos_record = record.getRdPtr();
	}
	if (size > 0)
	{
		dxfn.xfext = XFExtNoFRTPtr(new XFExtNoFRT);
		record >> *dxfn.xfext;

		size -= (record.getRdPtr() - pos_record);
	}

	if (bExist)
	{
		if (dxfn.ibitAtrNum && !dxfn.ifmtNinch && !dxfn.fIfmtUser)
		{
			dxfn.ibitAtrNum = false;
		}

		std::wstringstream strm;
		dxfn.serialize(strm);
			
		dxfId_ = global_info->RegistrDxfn(strm.str());

	}
}


} // namespace XLS

