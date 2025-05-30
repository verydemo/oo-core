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
#ifndef OOX_RUN_FILE_INCLUDE_H_
#define OOX_RUN_FILE_INCLUDE_H_

#include "../CommonInclude.h"

#include "Text.h"
#include "../Styles/rPr.h"

namespace OOX
{
	namespace Spreadsheet
	{
		//необработано:
		class CRun : public OOX::Spreadsheet::WritingElementWithChilds<CText>
		{
		public:
			WritingElement_AdditionConstructors(CRun)
			CRun()
			{
			}
			virtual ~CRun()
			{
			}
			virtual void fromXML(XmlUtils::CXmlNode& node)
			{
			}
            virtual std::wstring toXML() const
			{
				return _T("");
			}
			virtual void toXML(NSStringUtils::CStringBuilder& writer) const
			{
				writer.WriteString(_T("<r>"));
				if(m_oRPr.IsInit())
					m_oRPr->toXML(writer);
				
                for ( size_t i = 0; i < m_arrItems.size(); ++i)
                {
                    if (  m_arrItems[i] )
                    {
                        m_arrItems[i]->toXML(writer);
                    }
                }

				writer.WriteString(_T("</r>"));
			}
			virtual void fromXML(XmlUtils::CXmlLiteReader& oReader)
			{
				ReadAttributes( oReader );

				if ( oReader.IsEmptyNode() )
					return;

				int nCurDepth = oReader.GetDepth();
				while( oReader.ReadNextSiblingNode( nCurDepth ) )
				{
					std::wstring sName = XmlUtils::GetNameNoNS(oReader.GetName());

					if ( _T("rPr") == sName )
						m_oRPr = oReader;
					else if ( _T("t") == sName )
						m_arrItems.push_back( new CText( oReader ));
				}
			}

            void fromBin(std::wstring& str, unsigned short fontindex)
            {
                auto ptr = new CText();
                ptr->fromBin(str);
                m_arrItems.push_back(ptr);

                m_oRPr.Init();
                m_oRPr->m_nFontIndex.Init();
                m_oRPr->m_nFontIndex = fontindex;
            }

			virtual EElementType getType () const
			{
				return et_x_r;
			}

		private:
			void ReadAttributes(XmlUtils::CXmlLiteReader& oReader)
			{
			}
		public:
			nullable<CRPr>	m_oRPr;
		};
	} //Spreadsheet
} // namespace OOX

#endif // OOX_RUN_FILE_INCLUDE_H_
