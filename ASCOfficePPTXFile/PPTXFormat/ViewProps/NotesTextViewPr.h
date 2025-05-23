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
#pragma once

#include "./../WrapperWritingElement.h"
#include "CViewPr.h"

namespace PPTX
{
	namespace nsViewProps
	{
		class NotesTextViewPr : public WrapperWritingElement
		{
		public:
			PPTX_LOGIC_BASE(NotesTextViewPr)

			virtual void fromXML(XmlUtils::CXmlNode& node)
			{
				CViewPr = node.ReadNode(L"p:cViewPr");

				FillParentPointersForChilds();
			}
			virtual std::wstring toXML() const
			{
				XmlUtils::CNodeValue oValue;
				oValue.Write(CViewPr);

				return XmlUtils::CreateNode(L"p:notesTextViewPr", oValue);
			}
			virtual void toPPTY(NSBinPptxRW::CBinaryFileWriter* pWriter) const
			{
				pWriter->WriteRecord1(0, CViewPr);
			}
			virtual void fromPPTY(NSBinPptxRW::CBinaryFileReader* pReader)
			{
				LONG _end_rec = pReader->GetPos() + pReader->GetRecordSize() + 4;

				while (pReader->GetPos() < _end_rec)
				{
					BYTE _rec = pReader->GetUChar();

					switch (_rec)
					{
						case 0:
						{
							CViewPr.fromPPTY(pReader);
						}break;
						default:
						{
							pReader->SkipRecord();
						}break;
					}
				}
				pReader->Seek(_end_rec);
			}
			virtual void toXmlWriter(NSBinPptxRW::CXmlWriter* pWriter) const
			{
				pWriter->StartNode(L"p:notesTextViewPr");
				pWriter->EndAttributes();

				CViewPr.toXmlWriter(pWriter);

				pWriter->EndNode(L"p:notesTextViewPr");
			}

			nsViewProps::CViewPr CViewPr;
		protected:
			virtual void FillParentPointersForChilds()
			{
				CViewPr.SetParentPointer(this);
			}
		};
	} // namespace nsViewProps
} // namespace PPTX