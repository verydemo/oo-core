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

#include "../Source/XlsxFormat/Chart/ChartSerialize.cpp"
#include "../Source/XlsxFormat/Chart/ChartSerializeEx.cpp"
#include "../Source/XlsxFormat/Common.cpp"
#include "../Source/XlsxFormat/FileFactory_Spreadsheet.cpp"
#include "../Source/XlsxFormat/Xlsx.cpp"
#include "../Source/XlsxFormat/XlsxFlat.cpp"
#include "../Source/XlsxFormat/Worksheets/SheetData.cpp"
#include "../Source/XlsxFormat/Worksheets/Worksheet.cpp"
#include "../Source/XlsxFormat/Worksheets/ConditionalFormatting.cpp"
#include "../Source/XlsxFormat/Worksheets/DataValidation.cpp"
#include "../Source/XlsxFormat/Table/Tables.cpp"
#include "../Source/XlsxFormat/Controls/Controls.cpp"
#include "../Source/XlsxFormat/Styles/rPr.cpp"
#include "../Source/XlsxFormat/SharedStrings/Si.cpp"
#include "../Source/XlsxFormat/SharedStrings/Text.cpp"
#include "../Source/XlsxFormat/Pivot/Pivots.cpp"
#include "../Source/XlsxFormat/Slicer/SlicerCache.cpp"
#include "../Source/XlsxFormat/Slicer/SlicerCacheExt.cpp"
#include "../Source/XlsxFormat/Slicer/Slicer.cpp"
#include "../Source/XlsxFormat/NamedSheetViews/NamedSheetViews.cpp"
#include "../Source/XlsxFormat/Pivot/PivotCacheDefinitionExt.cpp"
#include "../Source/XlsxFormat/Workbook/Workbook.cpp"
