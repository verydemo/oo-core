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

#include "office_elements.h"
#include "office_elements_create.h"

namespace cpdoccore { 
namespace odf_reader {

namespace text {

// text:note-citation
//////////////////////////////////////////////////////////////////////////////////////////////////
class note_citation : public office_element_impl<note_citation>
{
public:
    static const wchar_t * ns;
    static const wchar_t * name;
    static const xml::NodeType xml_type = xml::typeElement;
    static const ElementType type = typeTextNoteCitation;
    CPDOCCORE_DEFINE_VISITABLE();
    
	virtual void docx_convert(oox::docx_conversion_context & Context) ;
    virtual std::wostream & text_to_stream(std::wostream & _Wostream, bool bXmlEncode = true) const;

    note_citation() {} 

private:
    virtual void add_attributes( const xml::attributes_wc_ptr & Attributes );
    virtual void add_child_element( xml::sax * Reader, const std::wstring & Ns, const std::wstring & Name);
    virtual void add_text(const std::wstring & Text);
    virtual void add_space(const std::wstring & Text);

    std::wstring				text_label_;
    office_element_ptr_array	content_;

};

CP_REGISTER_OFFICE_ELEMENT2(note_citation);

// text:note-body
//////////////////////////////////////////////////////////////////////////////////////////////////
class note_body : public office_element_impl<note_body>
{
public:
    static const wchar_t * ns;
    static const wchar_t * name;
    static const xml::NodeType xml_type = xml::typeElement;
    static const ElementType type = typeTextNoteBody;
    CPDOCCORE_DEFINE_VISITABLE();
   
	virtual void docx_convert(oox::docx_conversion_context & Context) ;
    virtual std::wostream & text_to_stream(std::wostream & _Wostream, bool bXmlEncode = true) const;

	note_body() {} 

private:
    virtual void add_attributes( const xml::attributes_wc_ptr & Attributes );
    virtual void add_child_element( xml::sax * Reader, const std::wstring & Ns, const std::wstring & Name);
    virtual void add_text(const std::wstring & Text);
    virtual void add_space(const std::wstring & Text);

    office_element_ptr_array    content_;

};

CP_REGISTER_OFFICE_ELEMENT2(note_body);


}
}
}

