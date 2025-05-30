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
#include "../CommonInclude.h"

#include "rPr.h"
#include "../../XlsbFormat/Biff12_records/Fmt.h"
#include "../../XlsbFormat/Biff12_unions/ACFMT.h"

namespace OOX
{
	namespace Spreadsheet
	{
		class CNumFmt : public WritingElement
		{
		public:
			WritingElement_AdditionConstructors(CNumFmt)
            WritingElement_XlsbConstructors(CNumFmt)
			CNumFmt()
			{
			}
			virtual ~CNumFmt()
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
                toXML2(writer, _T("numFmt"));
			}
			void toXML2(NSStringUtils::CStringBuilder& writer, const wchar_t* sHeader) const
			{
				toXMLWithNS(writer, L"", sHeader, L"");
			}
			void toXMLWithNS(NSStringUtils::CStringBuilder& writer, const std::wstring &node_ns, const std::wstring &node_name, const std::wstring &child_ns) const
			{
				writer.StartNodeWithNS(node_ns, node_name);
				writer.StartAttributes();
				WritingStringNullableAttrInt(L"numFmtId", m_oNumFmtId, m_oNumFmtId->GetValue());
				WritingStringNullableAttrEncodeXmlString(L"formatCode", m_oFormatCode, *m_oFormatCode);
				WritingStringNullableAttrEncodeXmlString(L"x16r2:formatCode16", m_oFormatCode16, *m_oFormatCode16);
				WritingStringNullableAttrBool(L"sourceLinked", m_oSourceLinked);
				writer.EndAttributesAndNode();
			}
			virtual void fromXML(XmlUtils::CXmlLiteReader& oReader)
			{
				ReadAttributes( oReader );

				if ( !oReader.IsEmptyNode() )
					oReader.ReadTillEnd();
			}

            void fromBin(XLS::BaseObjectPtr& obj)
            {
                ReadAttributes(obj);
            }

			virtual EElementType getType () const
			{
				return et_x_NumFmt;
			}

		private:
			void ReadAttributes(XmlUtils::CXmlLiteReader& oReader)
			{
				WritingElement_ReadAttributes_Start( oReader )
					WritingElement_ReadAttributes_Read_if     ( oReader, _T("formatCode"),		m_oFormatCode )
					WritingElement_ReadAttributes_Read_if     ( oReader, _T("x16r2:formatCode16"),	m_oFormatCode16 )
					WritingElement_ReadAttributes_Read_if     ( oReader, _T("numFmtId"),		m_oNumFmtId )
					WritingElement_ReadAttributes_Read_if     ( oReader, _T("sourceLinked"),	m_oSourceLinked )

					WritingElement_ReadAttributes_Read_if     ( oReader, _T("ss:Format"),		m_oFormatCode )
				WritingElement_ReadAttributes_End( oReader )
			}

            void ReadAttributes(XLS::BaseObjectPtr& obj)
            {
                auto ptr = static_cast<XLSB::Fmt*>(obj.get());
                if(ptr != nullptr)
                {
                    m_oFormatCode = ptr->stFmtCode.value();
                    m_oNumFmtId   = ptr->ifmt;
                }
            }

		public:
			nullable_string									m_oFormatCode;
			nullable_string									m_oFormatCode16;
			nullable<SimpleTypes::CUnsignedDecimalNumber<>>	m_oNumFmtId;
			nullable<SimpleTypes::COnOff<>>					m_oSourceLinked;
		};
		class CNumFmts : public WritingElementWithChilds<CNumFmt>
		{
		public:
			WritingElement_AdditionConstructors(CNumFmts)
            WritingElement_XlsbVectorConstructors(CNumFmts)
			CNumFmts()
			{
			}
			virtual ~CNumFmts()
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
				if(m_arrItems.empty()) return;

				writer.WriteString(_T("<numFmts"));
				WritingStringNullableAttrInt(L"count", m_oCount, m_oCount->GetValue());
				writer.WriteString(_T(">"));
                for ( size_t i = 0; i < m_arrItems.size(); ++i)
                {
                    if (  m_arrItems[i] )
                    {
                        m_arrItems[i]->toXML(writer);
                    }
                }
				writer.WriteString(_T("</numFmts>"));
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

					if (L"numFmt" == sName)
					{
						m_arrItems.push_back(new CNumFmt(oReader));

						if (m_arrItems.back()->m_oNumFmtId.IsInit())
						{
							m_mapNumFmtIndex.insert(std::make_pair(m_arrItems.back()->m_oNumFmtId->GetValue(), m_arrItems.size() - 1));
						}
					}
				}
			}

            void fromBin(std::vector<XLS::BaseObjectPtr>& obj)
            {
                ReadAttributes(obj);

                for(auto &fmt : obj)
                {
                    XLS::BaseObjectPtr ptr = nullptr;
                    if(fmt->get_type() == XLS::typeACFMT)
                    {
                        ptr = boost::shared_ptr<XLS::BaseObject>(static_cast<XLSB::ACFMT*>(fmt.get())->m_BrtFmt);
                    }
                    else if(fmt->get_type() == XLS::typeFmt)
                    {
                        ptr = boost::shared_ptr<XLS::BaseObject>(fmt);
                    }

                    m_arrItems.push_back(new CNumFmt(ptr));

                    if (m_arrItems.back()->m_oNumFmtId.IsInit())
                    {
                        m_mapNumFmtIndex.insert(std::make_pair(m_arrItems.back()->m_oNumFmtId->GetValue(), m_arrItems.size() - 1));
                    }
                }

            }

			virtual EElementType getType () const
			{
				return et_x_NumFmts;
			}

		private:
			void ReadAttributes(XmlUtils::CXmlLiteReader& oReader)
			{
				WritingElement_ReadAttributes_Start( oReader )
					WritingElement_ReadAttributes_Read_if ( oReader, _T("count"), m_oCount )
				WritingElement_ReadAttributes_End( oReader )
			}
            void ReadAttributes(std::vector<XLS::BaseObjectPtr>& obj)
            {
                m_oCount = (_INT32)obj.size();
            }
		public:
			nullable<SimpleTypes::CUnsignedDecimalNumber<>> m_oCount;

			std::map<unsigned int, size_t> m_mapNumFmtIndex;
			std::map<std::wstring, int> m_mapFormatCode;
		};
	} //Spreadsheet
} // namespace OOX
