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
#ifndef PPTX_LOGIC_BLUR_INCLUDE_H_
#define PPTX_LOGIC_BLUR_INCLUDE_H_

#include "./../../WrapperWritingElement.h"

namespace PPTX
{
	namespace Logic
	{
		class Blur : public WrapperWritingElement
		{
		public:
			
			WritingElement_AdditionConstructors(Blur)
			PPTX_LOGIC_BASE2(Blur)

			Blur& operator=(const Blur& oSrc)
			{
				parentFile		= oSrc.parentFile;
				parentElement	= oSrc.parentElement;

				rad  = oSrc.rad;
				grow = oSrc.grow;
				return *this;
			}

			virtual OOX::EElementType getType() const
			{
				return OOX::et_a_blur;
			}	
			void fromXML(XmlUtils::CXmlLiteReader& oReader)
			{
				ReadAttributes( oReader );
			}
			void ReadAttributes(XmlUtils::CXmlLiteReader& oReader)
			{
				WritingElement_ReadAttributes_Start_No_NS( oReader )
					WritingElement_ReadAttributes_Read_if     ( oReader, _T("rad"), rad)
					WritingElement_ReadAttributes_Read_else_if( oReader, _T("grow"), grow)
				WritingElement_ReadAttributes_End_No_NS( oReader )
			}
			virtual void fromXML(XmlUtils::CXmlNode& node)
			{
				XmlMacroReadAttributeBase(node, L"rad", rad);
				XmlMacroReadAttributeBase(node, L"grow", grow);
			}

			virtual std::wstring toXML() const
			{
				XmlUtils::CAttribute oAttr;
				oAttr.Write(_T("rad"), rad);
				oAttr.Write(_T("grow"), grow);

				return XmlUtils::CreateNode(_T("a:blur"), oAttr);
			}
			virtual void toXmlWriter(NSBinPptxRW::CXmlWriter* pWriter) const
			{
				pWriter->StartNode(L"a:blur");
				pWriter->StartAttributes();
				pWriter->WriteAttribute(L"rad", rad);
				pWriter->WriteAttribute(L"grow", grow);
				pWriter->EndAttributes();
				pWriter->EndNode(L"a:blur");
			}
			virtual void toPPTY(NSBinPptxRW::CBinaryFileWriter* pWriter) const
			{
				pWriter->StartRecord(EFFECT_TYPE_BLUR);

				pWriter->WriteBYTE(NSBinPptxRW::g_nodeAttributeStart);
				pWriter->WriteSize_t2(0, rad);
				pWriter->WriteBool2(1, grow);
				pWriter->WriteBYTE(NSBinPptxRW::g_nodeAttributeEnd);

				pWriter->EndRecord();
			}
			virtual void fromPPTY(NSBinPptxRW::CBinaryFileReader* pReader)
			{
				pReader->Skip(4); // len
				BYTE _type = pReader->GetUChar(); 
				LONG _e = pReader->GetPos() + pReader->GetRecordSize() + 4;

				pReader->Skip(1);

				while (true)
				{
					BYTE _at = pReader->GetUChar_TypeNode();
					if (_at == NSBinPptxRW::g_nodeAttributeEnd)
						break;

					switch (_at)
					{
						case 0:	rad = (size_t)pReader->GetULong(); break;
						case 1:	grow = pReader->GetBool(); break;
					}
				}
				pReader->Seek(_e);
			}
		public:
			nullable_sizet	rad;
			nullable_bool	grow;
		protected:
			virtual void FillParentPointersForChilds(){};
		};
	} // namespace Logic
} // namespace PPTX

#endif // PPTX_LOGIC_BLUR_INCLUDE_H_
