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

#include "../FileTypes.h"
#include "../File.h"

namespace OOX
{
	class CDocument;

	class Media : public File
	{
	public:
		Media(OOX::Document *pMain, bool bDocument = true);
		Media(OOX::Document *pMain, const CPath& filename, bool bExternal = false);
		virtual ~Media()
		{
		}
		virtual const FileType type() const
		{
			return FileTypes::Media;
		}
		virtual void read(const CPath& filename);
		virtual void write(const CPath& filename, const CPath& directory, CContentTypes& content) const;

		void set_filename(const std::wstring & file_path, bool bExternal);
		void set_filename(CPath & file_path, bool bExternal, bool bDefault = false);

		bool IsExist()
		{
			return m_bExist;
		}
		bool IsExternal()
		{
			return m_bExternal;
		}
		CPath filename()
		{
			return m_filename;
		}
		virtual void copy_to(const CPath& path) const;
		virtual const CPath DefaultDirectory() const
		{
			if (m_bDocument) return type().DefaultDirectory();
			else	return L"../" + type().DefaultDirectory();
		}
		virtual const CPath DefaultFileName() const
		{
			return m_filename.GetFilename();
		}
		std::vector<BYTE> m_Data;
	protected:
		CPath	m_filename;
		bool	m_bExist;
		bool	m_bExternal;
		bool	m_bDocument; //for upper/lower level rels (defaultDirectory)
	};
} // namespace OOX

