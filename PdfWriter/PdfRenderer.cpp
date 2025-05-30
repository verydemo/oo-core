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
#include "../DesktopEditor/common/File.h"
#include "../DesktopEditor/common/Directory.h"

#include "PdfRenderer.h"

#include "Src/Document.h"
#include "Src/Pages.h"
#include "Src/Image.h"
#include "Src/Font.h"
#include "Src/FontCidTT.h"
#include "Src/FontTT.h"
#include "Src/Annotation.h"
#include "Src/Destination.h"
#include "Src/Field.h"

#include "../DesktopEditor/graphics/Image.h"
#include "../DesktopEditor/graphics/structures.h"
#include "../DesktopEditor/raster/BgraFrame.h"
#include "../DesktopEditor/raster/ImageFileFormatChecker.h"
#include "../DesktopEditor/graphics/pro/Fonts.h"
#include "../DesktopEditor/graphics/pro/Image.h"

#include "../UnicodeConverter/UnicodeConverter.h"
#include "../Common/Network/FileTransporter/include/FileTransporter.h"

#include "OnlineOfficeBinToPdf.h"

#include <iostream>

#if defined(GetTempPath)
#undef GetTempPath
#endif

#if defined(CreateFile)
#undef CreateFile
#endif

#if defined(CreateDirectory)
#undef CreateDirectory
#endif

#define MM_2_PT(X) ((X) * 72.0 / 25.4)
#define PT_2_MM(X) ((X) * 25.4 / 72.0)

#define LONG_2_BOOL(X) ((X) ? true : false)

#ifdef DrawText
#undef DrawText
#endif

#ifdef LoadImage
#undef LoadImage
#endif

using namespace PdfWriter;

#define HI_SURROGATE_START  0xD800
#define HI_SURROGATE_END    0xDBFF
#define LO_SURROGATE_START  0xDC00
#define LO_SURROGATE_END    0xDFFF


static unsigned int* WStringToUtf32(const std::wstring& wsUnicodeText, unsigned int& unLen)
{
	if (wsUnicodeText.size() <= 0)
		return NULL;

	unsigned int* pUnicodes = new unsigned int[wsUnicodeText.size()];
	if (!pUnicodes)
		return NULL;

	unsigned int* pOutput = pUnicodes;
	unLen = 0;
	if (2 == sizeof(wchar_t))
	{
		const wchar_t* wsEnd = wsUnicodeText.c_str() + wsUnicodeText.size();
		wchar_t* wsInput = (wchar_t*)wsUnicodeText.c_str();

		wchar_t wLeading, wTrailing;
		unsigned int unCode;
		while (wsInput < wsEnd)
		{
			wLeading = *wsInput++;
			if (wLeading < 0xD800 || wLeading > 0xDFFF)
			{
				pUnicodes[unLen++] = (unsigned int)wLeading;
			}
			else if (wLeading >= 0xDC00)
			{
				// Такого не должно быть
				continue;
			}
			else
			{
				unCode = (wLeading & 0x3FF) << 10;
				wTrailing = *wsInput++;
				if (wTrailing < 0xDC00 || wTrailing > 0xDFFF)
				{
					// Такого не должно быть
					continue;
				}
				else
				{
                    pUnicodes[unLen++] = ((unCode | (wTrailing & 0x3FF)) + 0x10000);
				}
			}
		}
	}
	else
	{
		unLen = wsUnicodeText.size();
		for (unsigned int unIndex = 0; unIndex < unLen; unIndex++)
		{
			pUnicodes[unIndex] = (unsigned int)wsUnicodeText.at(unIndex);
		}
	}

	return pUnicodes;
}

// Этих типов браша нет в рендерере, мы их используем, когда конвертим из веба
static const long c_BrushTypeLinearGradient = 8001;
static const long c_BrushTypeRadialGradient = 8002;

enum ERendererCommandType
{
	renderercommandtype_Text  = 0x01,
	renderercommandtype_Image = 0x02,
	renderercommandtype_Path  = 0x03,
	renderercommandtype_Clip  = 0x04
};
//----------------------------------------------------------------------------------------
//
// CRendererCommandBase
//
//----------------------------------------------------------------------------------------
class CRendererCommandBase
{
public:
	virtual ~CRendererCommandBase(){};
	virtual ERendererCommandType GetType() = 0;
};
//----------------------------------------------------------------------------------------
//
// CRendererTextCommand
//
//----------------------------------------------------------------------------------------
class CRendererTextCommand : public CRendererCommandBase
{
public:
	CRendererTextCommand(unsigned char* pCodes, unsigned int nLen, const double& dX, const double& dY)
	{
		m_pCodes      = pCodes;
		m_nLen        = nLen;
		m_dX          = dX;
		m_dY          = dY;
		m_pFont       = NULL;
		m_dSize       = -1;
		m_lColor      = 0;
		m_nAlpha       = 255;
		m_dCharSpace   = 0;
		m_dHorScaling  = 100;
		m_nMode        = (int)textrenderingmode_Fill;

		m_bNeedDoItalic = false;
		m_bNeedDoBold   = false;
	}
	~CRendererTextCommand()
	{
		if (m_pCodes)
			delete[] m_pCodes;
	}
	ERendererCommandType GetType()
	{
		return renderercommandtype_Text;
	}

	inline double         GetX() const
	{
		return m_dX;
	}
	inline double         GetY() const
	{
		return m_dY;
	}
	inline unsigned char* GetCodes() const
	{
		return m_pCodes;
	}
	inline unsigned int   GetCodesLen() const
	{
		return m_nLen;
	}
	inline void           SetFont(CFontDict* pFont)
	{
		m_pFont = pFont;
	}
	inline void           SetSize(const double& dSize)
	{
		m_dSize = dSize;
	}
	inline void           SetColor(const LONG& lColor)
	{
		m_lColor = lColor;
	}
	inline void           SetAlpha(const BYTE& nAlpha)
	{
		m_nAlpha = nAlpha;
	}
	inline void           SetCharSpace(const double& dCharSpace)
	{
		m_dCharSpace = dCharSpace;
	}
	inline void           SetHorScaling(const double& dKoef)
	{
		m_dHorScaling = dKoef;
	}
	inline void           SetMode(const int& nMode)
	{
		m_nMode = nMode;
	}
	inline void           SetNeedDoItalic(const bool& bItalic)
	{



		m_bNeedDoItalic = bItalic;
	}
	inline void           SetNeedDoBold(const bool& bBold)
	{
		m_bNeedDoBold = bBold;
	}
	inline CFontDict*     GetFont() const
	{
		return m_pFont;
	}
	inline double         GetSize() const
	{
		return m_dSize;
	}
	inline LONG           GetColor() const
	{
		return m_lColor;
	}
	inline BYTE           GetAlpha() const
	{
		return m_nAlpha;
	}
	inline double         GetSpace() const
	{
		return m_dCharSpace;
	}
	inline double         GetHorScaling() const
	{
		return m_dHorScaling;
	}
	inline int            GetMode() const
	{
		return m_nMode;
	}
	inline bool           IsNeedDoItalic() const
	{
		return m_bNeedDoItalic;
	}
	inline bool           IsNeedDoBold() const
	{
		return m_bNeedDoBold;
	}

private:

	unsigned char* m_pCodes;
	unsigned int   m_nLen;
	double         m_dX;
	double         m_dY;

	CFontDict*     m_pFont;
	bool           m_bNeedDoItalic;
	bool           m_bNeedDoBold;
	double         m_dSize;
	LONG           m_lColor;
	BYTE           m_nAlpha;
	double         m_dCharSpace;
	int            m_nMode;
	double         m_dHorScaling;
};
//----------------------------------------------------------------------------------------
//
// CCommandManager
//
//----------------------------------------------------------------------------------------
CPdfRenderer::CCommandManager::CCommandManager(CPdfRenderer* pRenderer) : m_pRenderer(pRenderer)
{
}
CPdfRenderer::CCommandManager::~CCommandManager()
{
	Clear();
}
CRendererTextCommand* CPdfRenderer::CCommandManager::AddText(unsigned char* pCodes, unsigned int nLen, const double& dX, const double& dY)
{
	CRendererCommandBase* pCommand = new CRendererTextCommand(pCodes, nLen, dX, dY);
	Add(pCommand);
	return (CRendererTextCommand*)pCommand;
}
void CPdfRenderer::CCommandManager::Add(CRendererCommandBase* pCommand)
{
	if (pCommand)
	{
		if (m_vCommands.size() > 0 && pCommand->GetType() != m_vCommands.at(0)->GetType())
			Flush();

		m_vCommands.push_back(pCommand);
	}
}
void CPdfRenderer::CCommandManager::Flush()
{
	int nCommandsCount = m_vCommands.size();
	if (nCommandsCount > 0)
	{
		CPage* pPage = m_pRenderer->m_pPage;
		pPage->GrSave();

		pPage->SetTransform(m_oTransform.m11, m_oTransform.m12, m_oTransform.m21, m_oTransform.m22, m_oTransform.dx, m_oTransform.dy);

		ERendererCommandType eType = m_vCommands.at(0)->GetType();
		if (renderercommandtype_Text == eType)
		{
			pPage->BeginText();
			CRendererTextCommand* pText = NULL;

			CFontDict*     pTextFont = NULL;
			double         dTextSize = -1;
			LONG          lTextColor = 0;
			BYTE          nTextAlpha = 255;
			double        dTextSpace = 0;			
			double       dHorScaling = 100;
			ETextRenderingMode eMode = textrenderingmode_Fill;
			bool        isNeedDoBold = false;
			bool      isNeedDoItalic = false;
			double        dLineWidth = -1;

			double dPrevX = -1000;
			double dPrevY = -1000;
			unsigned short ushPrevCode = 0;

			CTextLine oTextLine;
			for (int nIndex = 0; nIndex < nCommandsCount; nIndex++)
			{
				pText = (CRendererTextCommand*)m_vCommands.at(nIndex);
				if (!pText)
					continue;

				if (pTextFont != pText->GetFont() || fabs(dTextSize - pText->GetSize()) > 0.001)
				{
					oTextLine.Flush(pPage);
					pTextFont = pText->GetFont();
					dTextSize = pText->GetSize();
					pPage->SetFontAndSize(pTextFont, dTextSize);					
				}

				if (lTextColor != pText->GetColor())
				{
					oTextLine.Flush(pPage);
					lTextColor = pText->GetColor();
					TColor oColor = lTextColor;
					pPage->SetFillColor(oColor.r, oColor.g, oColor.b);
					pPage->SetStrokeColor(oColor.r, oColor.g, oColor.b);
				}

				if (nTextAlpha != pText->GetAlpha())
				{
					oTextLine.Flush(pPage);
					nTextAlpha = pText->GetAlpha();
					pPage->SetFillAlpha(nTextAlpha);
					pPage->SetStrokeAlpha(nTextAlpha);
				}

				if (fabs(dTextSpace - pText->GetSpace()) > 0.001)
				{
					oTextLine.Flush(pPage);
					dTextSpace = pText->GetSpace();
					pPage->SetCharSpace(dTextSpace);					
				}

				if ((int)eMode != pText->GetMode() || isNeedDoBold != pText->IsNeedDoBold())
				{
					oTextLine.Flush(pPage);
					eMode = (ETextRenderingMode)pText->GetMode();
					isNeedDoBold = pText->IsNeedDoBold();

					if (isNeedDoBold && eMode == textrenderingmode_Fill)
					{
						double dNewLineWidth = dTextSize / 12 * 0.343;
						if (fabs(dLineWidth - dNewLineWidth) > 0.001)
						{
							dLineWidth = dNewLineWidth;
							pPage->SetLineWidth(dLineWidth);
						}

						pPage->SetTextRenderingMode(textrenderingmode_FillThenStroke);
					}
					else
					{
						pPage->SetTextRenderingMode(eMode);
					}
				}

				if (fabs(dHorScaling - pText->GetHorScaling()) > 0.001)
				{
					oTextLine.Flush(pPage);
					dHorScaling = pText->GetHorScaling();
					pPage->SetHorizontalScalling(dHorScaling);
				}

				if (isNeedDoItalic != pText->IsNeedDoItalic())
				{
					oTextLine.Flush(pPage);

					if (pText->IsNeedDoItalic())
						pPage->SetTextMatrix(1, 0, 0.26, 1, 0, 0);
					else
						pPage->SetTextMatrix(1, 0, 0, 1, 0, 0);

					isNeedDoItalic = pText->IsNeedDoItalic();
				}

				unsigned char* pCodes    = pText->GetCodes();
				unsigned short ushCode   = (pCodes[0] << 8) + pCodes[1];
				unsigned int   unLen     = pText->GetCodesLen();
				double         dX        = pText->GetX();
				double         dY        = pText->GetY();
				double         dTextSize = pText->GetSize();
				double         dWidth    = ((CFontCidTrueType*)pText->GetFont())->GetWidth(ushCode) / 1000.0 * dTextSize;
				
				if (!oTextLine.Add(pCodes, unLen, dX, dY, dWidth, dTextSize))
				{
					oTextLine.Flush(pPage);
					if (!oTextLine.Add(pCodes, unLen, dX, dY, dWidth, dTextSize))
					{
						pPage->DrawText(dX, dY, pCodes, unLen);
					}
				}
			}

			oTextLine.Flush(pPage);
			pPage->EndText();
		}

		pPage->GrRestore();
	}

	Clear();
}
void CPdfRenderer::CCommandManager::Clear()
{
	for (int nIndex = 0, nCount = m_vCommands.size(); nIndex < nCount; nIndex++)
	{
		CRendererCommandBase* pCommand = m_vCommands.at(nIndex);
		delete pCommand;
	}
	m_vCommands.clear();
}
void CPdfRenderer::CCommandManager::SetTransform(const CTransform& oTransform)
{
	m_oTransform = oTransform;
}
void CPdfRenderer::CCommandManager::SetTransform(const double& m11, const double& m12, const double& m21, const double& m22, const double& dx, const double& dy)
{
	m_oTransform.Set(m11, m12, m21, m22, dx, dy);
}
//----------------------------------------------------------------------------------------
//
// CPdfRenderer
//
//----------------------------------------------------------------------------------------
CPdfRenderer::CPdfRenderer(NSFonts::IApplicationFonts* pAppFonts, bool isPDFA) : m_oCommandManager(this)
{
	m_pAppFonts = pAppFonts;

	// Создаем менеджер шрифтов с собственным кэшем
	m_pFontManager = pAppFonts->GenerateFontManager();
    NSFonts::IFontsCache* pMeasurerCache = NSFonts::NSFontCache::Create();
	pMeasurerCache->SetStreams(pAppFonts->GetStreams());
	m_pFontManager->SetOwnerCache(pMeasurerCache);

	m_pDocument = new CDocument();

	if (isPDFA)
		m_pDocument->SetPDFAConformanceMode(true);

	if (!m_pDocument || !m_pDocument->CreateNew())
	{
		SetError();
		return;
	}

	m_pDocument->SetCompressionMode(COMP_ALL);

	m_bValid      = true;
	m_bEdit       = false;
	m_bEditPage   = false;
	m_dPageHeight = 297;
	m_dPageWidth  = 210;
	m_pPage       = NULL;
	m_pFont       = NULL;

	m_bNeedUpdateTextFont      = true;
	m_bNeedUpdateTextColor     = true;
	m_bNeedUpdateTextAlpha     = true;
	m_bNeedUpdateTextCharSpace = true;
	m_bNeedUpdateTextSize      = true;

	m_wsTempFolder = L"";
	SetTempFolder(NSFile::CFileBinary::GetTempPath());
}
CPdfRenderer::~CPdfRenderer()
{
	RELEASEOBJECT(m_pDocument);
	RELEASEINTERFACE(m_pFontManager);

	if (L"" != m_wsTempFolder)
		NSDirectory::DeleteDirectory(m_wsTempFolder);
}
void CPdfRenderer::SetPassword(const std::wstring& wsPassword)
{
    if (!IsValid())
        return;

    m_pDocument->SetPasswords(wsPassword, wsPassword);
}

void CPdfRenderer::SetDocumentID(const std::wstring& wsDocumentID)
{
    if (!IsValid())
        return;

    m_pDocument->SetDocumentID(wsDocumentID);
}
int CPdfRenderer::SaveToFile(const std::wstring& wsPath)
{
	// TODO: Переделать на код ошибки
	if (!IsValid())
		return 1;

	m_oCommandManager.Flush();

	if (!m_pDocument->SaveToFile(wsPath))
		return 1;

	return 0;
}
void CPdfRenderer::SetTempFolder(const std::wstring& wsPath)
{
	if (L"" != m_wsTempFolder)
		NSDirectory::DeleteDirectory(m_wsTempFolder);
	
	int nCounter = 0;
    m_wsTempFolder = wsPath;
    int nPathLen = (int)m_wsTempFolder.length();
    if (nPathLen > 0)
    {
        const wchar_t* pData = m_wsTempFolder.c_str();
        if ((pData[nPathLen - 1] != '/') && (pData[nPathLen - 1] != '\\'))
            m_wsTempFolder += L"/";

        m_wsTempFolder += L"PDF";
    }

    std::wstring sTest = m_wsTempFolder;
	while (NSDirectory::Exists(m_wsTempFolder))
	{
        m_wsTempFolder = sTest + L"/PDF_" + std::to_wstring(nCounter);
		nCounter++;
	}
	NSDirectory::CreateDirectory(m_wsTempFolder);
}
std::wstring CPdfRenderer::GetTempFile()
{
	return NSFile::CFileBinary::CreateTempFileWithUniqueName(m_wsTempFolder, L"PDF");
}
std::wstring CPdfRenderer::GetTempDirectory()
{
	return m_wsTempFolder;
}
//----------------------------------------------------------------------------------------
// Тип рендерера
//----------------------------------------------------------------------------------------
HRESULT CPdfRenderer::get_Type(LONG* lType)
{
	*lType = c_nPDFWriter;
	return S_OK;
}
//----------------------------------------------------------------------------------------
// Функции для работы со страницей
//----------------------------------------------------------------------------------------
HRESULT CPdfRenderer::NewPage()
{
	m_oCommandManager.Flush();

	if (!IsValid() || m_bEdit)
		return S_FALSE;

	m_pPage = m_pDocument->AddPage();

	if (!m_pPage)
	{
		SetError();
		return S_FALSE;
	}

	m_pPage->SetWidth(MM_2_PT(m_dPageWidth));
	m_pPage->SetHeight(MM_2_PT(m_dPageHeight));

	Reset();

	return S_OK;
}
HRESULT CPdfRenderer::get_Height(double* dHeight)
{
	*dHeight = m_dPageHeight;
	return S_OK;
}
HRESULT CPdfRenderer::put_Height(const double& dHeight)
{
	if (!IsValid() || !m_pPage)
		return S_FALSE;

	if (m_bEdit && m_bEditPage)
	{
		return S_OK;
	}

	m_dPageHeight = dHeight;
	m_pPage->SetHeight(MM_2_PT(dHeight));
	return S_OK;
}
HRESULT CPdfRenderer::get_Width(double* dWidth)
{
	*dWidth = m_dPageWidth;
	return S_OK;
}
HRESULT CPdfRenderer::put_Width(const double& dWidth)
{
	if (!IsValid() || !m_pPage)
		return S_FALSE;

	if (m_bEdit && m_bEditPage)
	{
		return S_OK;
	}

	m_dPageWidth = dWidth;
	m_pPage->SetWidth(MM_2_PT(dWidth));
	return S_OK;
}
HRESULT CPdfRenderer::get_DpiX(double* dDpiX)
{
	*dDpiX = 72;
	return S_OK;
}
HRESULT CPdfRenderer::get_DpiY(double* dDpiY)
{
	*dDpiY = 72;
	return S_OK;
}
//----------------------------------------------------------------------------------------
// Функции для работы с Pen
//----------------------------------------------------------------------------------------
HRESULT CPdfRenderer::get_PenColor(LONG* lColor)
{
	*lColor = m_oPen.GetColor();
	return S_OK;
}
HRESULT CPdfRenderer::put_PenColor(const LONG& lColor)
{
	m_oPen.SetColor(lColor);
	return S_OK;
}
HRESULT CPdfRenderer::get_PenAlpha(LONG* lAlpha)
{
	*lAlpha = m_oPen.GetAlpha();
	return S_OK;
}
HRESULT CPdfRenderer::put_PenAlpha(const LONG& lAlpha)
{
	m_oPen.SetAlpha(lAlpha);
	return S_OK;
}
HRESULT CPdfRenderer::get_PenSize(double* dSize)
{
	*dSize = m_oPen.GetSize();
	return S_OK;
}
HRESULT CPdfRenderer::put_PenSize(const double& dSize)
{
	m_oPen.SetSize(dSize);
	return S_OK;
}
HRESULT CPdfRenderer::get_PenDashStyle(BYTE* nDashStyle)
{
	*nDashStyle = m_oPen.GetDashStyle();
	return S_OK;
}
HRESULT CPdfRenderer::put_PenDashStyle(const BYTE& nDashStyle)
{
	m_oPen.SetDashStyle(nDashStyle);
	return S_OK;
}
HRESULT CPdfRenderer::get_PenLineStartCap(BYTE* nCapStyle)
{
	*nCapStyle = m_oPen.GetStartCapStyle();
	return S_OK;
}
HRESULT CPdfRenderer::put_PenLineStartCap(const BYTE& nCapStyle)
{
	m_oPen.SetStartCapStyle(nCapStyle);
	return S_OK;
}
HRESULT CPdfRenderer::get_PenLineEndCap(BYTE* nCapStyle)
{
	*nCapStyle = m_oPen.GetEndCapStyle();
	return S_OK;
}
HRESULT CPdfRenderer::put_PenLineEndCap(const BYTE& nCapStyle)
{
	m_oPen.SetEndCapStyle(nCapStyle);
	return S_OK;
}
HRESULT CPdfRenderer::get_PenLineJoin(BYTE* nJoinStyle)
{
	*nJoinStyle = m_oPen.GetJoinStyle();
	return S_OK;
}
HRESULT CPdfRenderer::put_PenLineJoin(const BYTE& nJoinStyle)
{
	m_oPen.SetJoinStyle(nJoinStyle);
	return S_OK;
}
HRESULT CPdfRenderer::get_PenDashOffset(double* dOffset)
{
	*dOffset = m_oPen.GetDashOffset();
	return S_OK;
}
HRESULT CPdfRenderer::put_PenDashOffset(const double& dOffset)
{
	m_oPen.SetDashOffset(dOffset);
	return S_OK;
}
HRESULT CPdfRenderer::get_PenAlign(LONG* lAlign)
{
	*lAlign = m_oPen.GetAlign();
	return S_OK;
}
HRESULT CPdfRenderer::put_PenAlign(const LONG& lAlign)
{
	m_oPen.SetAlign(lAlign);
	return S_OK;
}
HRESULT CPdfRenderer::get_PenMiterLimit(double* dMiter)
{
	*dMiter = m_oPen.GetMiter();
	return S_OK;
}
HRESULT CPdfRenderer::put_PenMiterLimit(const double& dMiter)
{
	m_oPen.SetMiter(dMiter);
	return S_OK;
}
HRESULT CPdfRenderer::PenDashPattern(double* pPattern, LONG lCount)
{
	m_oPen.SetDashPattern(pPattern, lCount);
	return S_OK;
}
//----------------------------------------------------------------------------------------
// Функции для работы с Brush
//----------------------------------------------------------------------------------------
HRESULT CPdfRenderer::get_BrushType(LONG* lType)
{
	*lType = m_oBrush.GetType();
	return S_OK;
}
HRESULT CPdfRenderer::put_BrushType(const LONG& lType)
{
	m_oBrush.SetType(lType);
	return S_OK;
}
HRESULT CPdfRenderer::get_BrushColor1(LONG* lColor)
{
	*lColor = m_oBrush.GetColor1();
	return S_OK;
}
HRESULT CPdfRenderer::put_BrushColor1(const LONG& lColor)
{
	if (lColor != m_oBrush.GetColor1())
	{
		m_oBrush.SetColor1(lColor);
		m_bNeedUpdateTextColor = true;
	}
	return S_OK;
}
HRESULT CPdfRenderer::get_BrushAlpha1(LONG* lAlpha)
{
	*lAlpha = m_oBrush.GetAlpha1();
	return S_OK;
}
HRESULT CPdfRenderer::put_BrushAlpha1(const LONG& lAlpha)
{
	if (lAlpha != m_oBrush.GetAlpha1())
	{
		m_oBrush.SetAlpha1(lAlpha);
		m_bNeedUpdateTextAlpha = true;
	}
	return S_OK;
}
HRESULT CPdfRenderer::get_BrushColor2(LONG* lColor)
{
	*lColor = m_oBrush.GetColor2();
	return S_OK;
}
HRESULT CPdfRenderer::put_BrushColor2(const LONG& lColor)
{
	m_oBrush.SetColor2(lColor);
	return S_OK;
}
HRESULT CPdfRenderer::get_BrushAlpha2(LONG* lAlpha)
{
	*lAlpha = m_oBrush.GetAlpha2();
	return S_OK;
}
HRESULT CPdfRenderer::put_BrushAlpha2(const LONG& lAlpha)
{
	m_oBrush.SetAlpha2(lAlpha);
	return S_OK;
}
HRESULT CPdfRenderer::get_BrushTexturePath(std::wstring* wsPath)
{
	*wsPath = m_oBrush.GetTexturePath();
	return S_OK;
}
HRESULT CPdfRenderer::put_BrushTexturePath(const std::wstring& wsPath)
{
	m_oBrush.SetTexturePath(wsPath);
	return S_OK;
}
HRESULT CPdfRenderer::get_BrushTextureMode(LONG* lMode)
{
	*lMode = m_oBrush.GetTextureMode();
	return S_OK;
}
HRESULT CPdfRenderer::put_BrushTextureMode(const LONG& lMode)
{
	m_oBrush.SetTextureMode(lMode);
	return S_OK;
}
HRESULT CPdfRenderer::get_BrushTextureAlpha(LONG* lAlpha)
{
	*lAlpha = m_oBrush.GetTextureAlpha();
	return S_OK;
}
HRESULT CPdfRenderer::put_BrushTextureAlpha(const LONG& lAlpha)
{
	m_oBrush.SetTextureAlpha(lAlpha);
	return S_OK;
}
HRESULT CPdfRenderer::get_BrushLinearAngle(double* dAngle)
{
	*dAngle = m_oBrush.GetLinearAngle();
	return S_OK;
}
HRESULT CPdfRenderer::put_BrushLinearAngle(const double& dAngle)
{
	m_oBrush.SetLinearAngle(dAngle);
	return S_OK;
}
HRESULT CPdfRenderer::BrushRect(const INT& nVal, const double& dLeft, const double& dTop, const double& dWidth, const double& dHeight)
{
	// Данными параметрами пользуемся, только если пришла команда EnableBrushRect, если команда не пришла, тогда
	// ориентируемся на границы пата.
	m_oBrush.SetBrushRect(nVal, dLeft, dTop, dWidth, dHeight);
    m_oBrush.EnableBrushRect(1 == nVal ? true : false);
	return S_OK;
}
HRESULT CPdfRenderer::BrushBounds(const double& dLeft, const double& dTop, const double& dWidth, const double& dHeight)
{
	// TODO: Пока определяется все по границам пата
	return S_OK;
}
HRESULT CPdfRenderer::put_BrushGradientColors(LONG* lColors, double* pPositions, LONG lCount)
{
	m_oBrush.SetGradientColors(lColors, pPositions, lCount);
	return S_OK;
}
//----------------------------------------------------------------------------------------
// Функции для работы со шрифтами
//----------------------------------------------------------------------------------------
HRESULT CPdfRenderer::get_FontName(std::wstring* wsName)
{
	*wsName = m_oFont.GetName();
	return S_OK;
}
HRESULT CPdfRenderer::put_FontName(const std::wstring& wsName)
{
	if (wsName != m_oFont.GetName())
	{
		m_oFont.SetName(wsName);
		m_bNeedUpdateTextFont = true;
	}

	return S_OK;
}
HRESULT CPdfRenderer::get_FontPath(std::wstring* wsPath)
{
	*wsPath = m_oFont.GetPath();
	return S_OK;
}
HRESULT CPdfRenderer::put_FontPath(const std::wstring& wsPath)
{
	if (wsPath != m_oFont.GetPath())
	{
		m_oFont.SetPath(wsPath);
		m_bNeedUpdateTextFont = true;
	}
	return S_OK;
}
HRESULT CPdfRenderer::get_FontSize(double* dSize)
{
	*dSize = m_oFont.GetSize();
	return S_OK;
}
HRESULT CPdfRenderer::put_FontSize(const double& dSize)
{
	if (fabs(dSize - m_oFont.GetSize()) > 0.001)
	{
		m_oFont.SetSize(dSize);
		m_bNeedUpdateTextSize = true;
	}
	return S_OK;
}
HRESULT CPdfRenderer::get_FontStyle(LONG* lStyle)
{
	*lStyle = m_oFont.GetStyle();
	return S_OK;
}
HRESULT CPdfRenderer::put_FontStyle(const LONG& lStyle)
{
	if (lStyle != m_oFont.GetStyle())
	{
		m_oFont.SetStyle(lStyle);
		m_bNeedUpdateTextFont = true;
	}
	return S_OK;
}
HRESULT CPdfRenderer::get_FontStringGID(INT* bGid)
{
	*bGid = m_oFont.GetGid() ? 1 : 0;
	return S_OK;
}
HRESULT CPdfRenderer::put_FontStringGID(const INT& bGid)
{
	m_oFont.SetGid(bGid ? true : false);
	return S_OK;
}
HRESULT CPdfRenderer::get_FontCharSpace(double* dSpace)
{
	*dSpace = m_oFont.GetCharSpace();
	return S_OK;
}
HRESULT CPdfRenderer::put_FontCharSpace(const double& dSpace)
{
	if (fabs(dSpace - m_oFont.GetCharSpace()) > 0.001)
	{
		m_oFont.SetCharSpace(dSpace);
		m_bNeedUpdateTextCharSpace = true;
	}
	return S_OK;
}
HRESULT CPdfRenderer::get_FontFaceIndex(int* nFaceIndex)
{
	*nFaceIndex = (int)m_oFont.GetFaceIndex();
	return S_OK;
}
HRESULT CPdfRenderer::put_FontFaceIndex(const int& nFaceIndex)
{
	if (nFaceIndex != m_oFont.GetFaceIndex())
	{
		m_oFont.SetFaceIndex(nFaceIndex);
		m_bNeedUpdateTextFont = true;
	}

	return S_OK;
}
//----------------------------------------------------------------------------------------
// Функции для вывода текста
//----------------------------------------------------------------------------------------
HRESULT CPdfRenderer::CommandDrawTextCHAR(const LONG& lUnicode, const double& dX, const double& dY, const double& dW, const double& dH)
{
	if (!IsPageValid())
		return S_FALSE;

	unsigned int unUnicode = lUnicode;
	unsigned char* pCodes = EncodeString(&unUnicode, 1);
	return DrawText(pCodes, 2, dX, dY) ? S_OK : S_FALSE;
}
HRESULT CPdfRenderer::CommandDrawText(const std::wstring& wsUnicodeText, const double& dX, const double& dY, const double& dW, const double& dH)
{
	if (!IsPageValid() || !wsUnicodeText.size())
		return S_FALSE;

	unsigned int unLen;
	unsigned int* pUnicodes = WStringToUtf32(wsUnicodeText, unLen);
	if (!pUnicodes)
		return S_FALSE;

	unsigned char* pCodes = EncodeString(pUnicodes, unLen);
	delete[] pUnicodes;

	if (!pCodes)
		return S_FALSE;

	// Специальный случай для текста из Djvu, нам не нужно, чтобы он рисовался
	if (L"" == m_oFont.GetPath() && L"DjvuEmptyFont" == m_oFont.GetName())
	{
		if (m_bNeedUpdateTextFont)
		{
			m_oFont.SetName(L"Arial");
			UpdateFont();
			m_oFont.SetName(L"DjvuEmptyFont");
			if (!m_pFont)
				return S_FALSE;
		}

		double dFontSize = MM_2_PT(dH);
		double dStringWidth = 0;
		for (unsigned int unIndex = 0; unIndex < unLen; unIndex++)
		{
			unsigned short ushCode = (pCodes[2 * unIndex] << 8) + pCodes[2 * unIndex + 1];
			dStringWidth += m_pFont->GetWidth(ushCode) * dFontSize / 1000.0;
		}

		double dResultWidth = MM_2_PT(dW);

		CTransform& t = m_oTransform;
		m_oCommandManager.SetTransform(t.m11, -t.m12, -t.m21, t.m22, MM_2_PT(t.dx + t.m21 * m_dPageHeight), MM_2_PT(m_dPageHeight - m_dPageHeight * t.m22 - t.dy));

		CRendererTextCommand* pText = m_oCommandManager.AddText(pCodes, unLen * 2, MM_2_PT(dX), MM_2_PT(m_dPageHeight - dY - dH));
		pText->SetFont(m_pFont);
		pText->SetSize(dFontSize);
		pText->SetMode(textrenderingmode_Invisible);
		if (fabs(dStringWidth) > 0.001)
			pText->SetHorScaling(dResultWidth / dStringWidth * 100);

		return S_OK;
	}

	return DrawText(pCodes, unLen * 2, dX, dY) ? S_OK : S_FALSE;
}
HRESULT CPdfRenderer::CommandDrawTextExCHAR(const LONG& lUnicode, const LONG& lGid, const double& dX, const double& dY, const double& dW, const double& dH)
{
	if (!IsPageValid())
		return S_FALSE;

	unsigned int unUnicode = lUnicode;
	unsigned int unGID     = lGid;
	unsigned char* pCodes = EncodeGID(unGID, &unUnicode, 1);
	return DrawText(pCodes, 2, dX, dY) ? S_OK : S_FALSE;
}
HRESULT CPdfRenderer::CommandDrawTextEx(const std::wstring& wsUnicodeText, const unsigned int* pGids, const unsigned int unGidsCount, const double& dX, const double& dY, const double& dW, const double& dH)
{
	if (!IsPageValid() || (!wsUnicodeText.size() && (!pGids || !unGidsCount)))
		return S_FALSE;

	unsigned int unLen = 0;
	unsigned int* pUnicodes = NULL;
	if (pGids && unGidsCount)
	{
		unLen = unGidsCount;
		if (wsUnicodeText.size())
		{
			unsigned int unUnicodeLen;
			pUnicodes = WStringToUtf32(wsUnicodeText, unUnicodeLen);
			if (!pUnicodes || unUnicodeLen != unLen)
				RELEASEARRAYOBJECTS(pUnicodes);
		}

		if (!pUnicodes)
		{
			pUnicodes = new unsigned int[unLen];
			if (!pUnicodes)
				return S_FALSE;

			for (unsigned int unIndex = 0; unIndex < unLen; unIndex++)
				pUnicodes[unIndex] = pGids[unIndex];
		}
	}
	else
	{
		pUnicodes = WStringToUtf32(wsUnicodeText, unLen);
		if (!pUnicodes)
			return S_FALSE;
	}

	unsigned char* pCodes = EncodeString(pUnicodes,  unLen, pGids);
	RELEASEARRAYOBJECTS(pUnicodes);
	return DrawText(pCodes, unLen * 2, dX, dY) ? S_OK : S_FALSE;
}
HRESULT CPdfRenderer::CommandDrawTextCHAR2(unsigned int* pUnicodes, const unsigned int& unUnicodeCount, const unsigned int& unGid, const double& dX, const double& dY, const double& dW, const double& dH)
{
	if (!IsPageValid())
		return S_FALSE;

	unsigned char* pCodes = EncodeGID(unGid, pUnicodes, unUnicodeCount);
	return DrawText(pCodes, 2, dX, dY) ? S_OK : S_FALSE;
}
//----------------------------------------------------------------------------------------
// Маркеры команд
//----------------------------------------------------------------------------------------
HRESULT CPdfRenderer::BeginCommand(const DWORD& dwType)
{
	// Здесь мы ничего не делаем
	return S_OK;
}
HRESULT CPdfRenderer::EndCommand(const DWORD& dwType)
{
	if (!IsPageValid())
		return S_FALSE;

	// Здесь мы различаем лишь 2 команды: присоединить текущий пат к клипу и отменить клип
	if (c_nClipType == dwType)
	{
		m_oCommandManager.Flush();
		m_pPage->GrSave();
		m_lClipDepth++;
		UpdateTransform();

		if (c_nClipRegionTypeEvenOdd & m_lClipMode)
			m_oPath.Clip(m_pPage, true);
		else
			m_oPath.Clip(m_pPage, false);
	}
	else if (c_nResetClipType == dwType)
	{
		m_oCommandManager.Flush();
		while (m_lClipDepth)
		{
			m_pPage->GrRestore();
			m_lClipDepth--;
		}
	}
	else if (c_nPageType == dwType)
	{
		for (int nIndex = 0, nCount = m_vDestinations.size(); nIndex < nCount; ++nIndex)
		{
			TDestinationInfo& oInfo = m_vDestinations.at(nIndex);
			if (m_pDocument->GetPagesCount() > oInfo.unDestPage)
			{
				AddLink(oInfo.pPage, oInfo.dX, oInfo.dY, oInfo.dW, oInfo.dH, oInfo.dDestX, oInfo.dDestY, oInfo.unDestPage);
				m_vDestinations.erase(m_vDestinations.begin() + nIndex);
				nIndex--;
				nCount--;
			}
		}
	}

	return S_OK;
}
//----------------------------------------------------------------------------------------
// Функции для работы с патом
//----------------------------------------------------------------------------------------
HRESULT CPdfRenderer::PathCommandStart()
{
	m_oPath.Clear();
	return S_OK;
}
HRESULT CPdfRenderer::PathCommandEnd()
{
	m_oPath.Clear();
	return S_OK;
}
HRESULT CPdfRenderer::DrawPath(const LONG& lType)
{
	m_oCommandManager.Flush();

	if (!IsPageValid())
		return S_FALSE;

	bool bStroke = LONG_2_BOOL(lType & c_nStroke);
	bool bFill   = LONG_2_BOOL(lType & c_nWindingFillMode);
	bool bEoFill = LONG_2_BOOL(lType & c_nEvenOddFillMode);

	m_pPage->GrSave();
	UpdateTransform();

	if (bStroke)
		UpdatePen();

	std::wstring sTextureOldPath = L"";
	std::wstring sTextureTmpPath = L"";

	if (bFill || bEoFill)
	{
		if (c_BrushTypeTexture == m_oBrush.GetType())
		{
			sTextureOldPath = m_oBrush.GetTexturePath();
			sTextureTmpPath = GetDownloadFile(sTextureOldPath);

			if (!sTextureTmpPath.empty())
				m_oBrush.SetTexturePath(sTextureTmpPath);
		}

		UpdateBrush();
	}

	if (!m_pShading)
	{
		m_oPath.Draw(m_pPage, bStroke, bFill, bEoFill);
	}
	else
	{
		if (bFill || bEoFill)
		{
			m_pPage->GrSave();
			m_oPath.Clip(m_pPage, bEoFill);

			if (NULL != m_pShadingExtGrState)
				m_pPage->SetExtGrState(m_pShadingExtGrState);

			m_pPage->DrawShading(m_pShading);
			m_pPage->GrRestore();
		}

		if (bStroke)
			m_oPath.Draw(m_pPage, bStroke, false, false);
	}

	m_pPage->GrRestore();

	if (!sTextureTmpPath.empty())
	{
		m_oBrush.SetTexturePath(sTextureOldPath);

		if (NSFile::CFileBinary::Exists(sTextureTmpPath))
			NSFile::CFileBinary::Remove(sTextureTmpPath);
	}

	return S_OK;
}
HRESULT CPdfRenderer::PathCommandMoveTo(const double& dX, const double& dY)
{
	m_oPath.MoveTo(MM_2_PT(dX), MM_2_PT(m_dPageHeight - dY));
	return S_OK;
}
HRESULT CPdfRenderer::PathCommandLineTo(const double& dX, const double& dY)
{
	m_oPath.LineTo(MM_2_PT(dX), MM_2_PT(m_dPageHeight - dY));
	return S_OK;
}
HRESULT CPdfRenderer::PathCommandLinesTo(double* pPoints, const int& nCount)
{
	if (nCount < 4 || !pPoints)
		return S_OK;

	if (!m_oPath.IsMoveTo())
		m_oPath.MoveTo(MM_2_PT(pPoints[0]), MM_2_PT(m_dPageHeight - pPoints[1]));

	int nPointsCount = (nCount / 2) - 1;
	for (int nIndex = 1; nIndex <= nPointsCount; ++nIndex)
	{
		m_oPath.LineTo(MM_2_PT(pPoints[nIndex * 2]), MM_2_PT(m_dPageHeight - pPoints[nIndex * 2 + 1]));
	}

	return S_OK;
}
HRESULT CPdfRenderer::PathCommandCurveTo(const double& dX1, const double& dY1, const double& dX2, const double& dY2, const double& dXe, const double& dYe)
{
	m_oPath.CurveTo(MM_2_PT(dX1), MM_2_PT(m_dPageHeight - dY1), MM_2_PT(dX2), MM_2_PT(m_dPageHeight - dY2), MM_2_PT(dXe), MM_2_PT(m_dPageHeight - dYe));
	return S_OK;
}
HRESULT CPdfRenderer::PathCommandCurvesTo(double* pPoints, const int& nCount)
{
	if (nCount < 8 || !pPoints)
		return S_OK;

	if (!m_oPath.IsMoveTo())
		m_oPath.MoveTo(MM_2_PT(pPoints[0]), MM_2_PT(m_dPageHeight - pPoints[1]));

	int nPointsCount = (nCount - 2) / 6;
	double* pCur = pPoints + 2;
	for (int nIndex = 0; nIndex <= nPointsCount; ++nIndex, pCur += 6)
	{
		m_oPath.CurveTo(MM_2_PT(pCur[0]), MM_2_PT(m_dPageHeight - pCur[1]), MM_2_PT(pCur[2]), MM_2_PT(m_dPageHeight - pCur[3]), MM_2_PT(pCur[4]), MM_2_PT(m_dPageHeight - pCur[5]));
	}

	return S_OK;
}
HRESULT CPdfRenderer::PathCommandArcTo(const double& dX, const double& dY, const double& dW, const double& dH, const double& dStartAngle, const double& dSweepAngle)
{
	m_oPath.ArcTo(MM_2_PT(dX), MM_2_PT(m_dPageHeight - dY - dH), MM_2_PT(dW), MM_2_PT(dH), dStartAngle, dSweepAngle);
	return S_OK;
}
HRESULT CPdfRenderer::PathCommandClose()
{
	m_oPath.Close();
	return S_OK;
}
HRESULT CPdfRenderer::PathCommandGetCurrentPoint(double* dX, double* dY)
{
	m_oPath.GetLastPoint(*dX, *dY);
	*dX = PT_2_MM(*dX);
	*dY = PT_2_MM(*dY);
	return S_OK;
}
HRESULT CPdfRenderer::PathCommandTextCHAR(const LONG& lUnicode, const double& dX, const double& dY, const double& dW, const double& dH)
{
	unsigned int unUnicode = lUnicode;
	bool bRes = PathCommandDrawText(&unUnicode, 1, dX, dY, NULL);
	return bRes ? S_OK : S_FALSE;
}
HRESULT CPdfRenderer::PathCommandText(const std::wstring& wsUnicodeText, const double& dX, const double& dY, const double& dW, const double& dH)
{
	unsigned int unLen;
	unsigned int* pUnicodes = WStringToUtf32(wsUnicodeText, unLen);
	if (!pUnicodes)
		return S_FALSE;

	PathCommandDrawText(pUnicodes, unLen, dX, dY, NULL);
	delete[] pUnicodes;

	return S_OK;
}
HRESULT CPdfRenderer::PathCommandTextExCHAR(const LONG& lUnicode, const LONG& lGid, const double& dX, const double& dY, const double& dW, const double& dH)
{
	unsigned int unUnicode = lUnicode;
	unsigned int unGid     = lGid;
	bool bRes = PathCommandDrawText(&unUnicode, 1, dX, dY, &unGid);
	return bRes ? S_OK : S_FALSE;
}
HRESULT CPdfRenderer::PathCommandTextEx(const std::wstring& wsUnicodeText, const unsigned int* pGids, const unsigned int unGidsCount, const double& dX, const double& dY, const double& dW, const double& dH)
{
	if (!IsPageValid() || (!wsUnicodeText.size() && (!pGids || !unGidsCount)))
		return S_FALSE;

	unsigned int unLen = 0;
	unsigned int* pUnicodes = NULL;
	if (pGids && unGidsCount)
	{
		unLen = unGidsCount;
		if (wsUnicodeText.size())
		{
			unsigned int unUnicodeLen;
			pUnicodes = WStringToUtf32(wsUnicodeText, unUnicodeLen);
			if (!pUnicodes || unUnicodeLen != unLen)
				RELEASEARRAYOBJECTS(pUnicodes);
		}

		if (!pUnicodes)
		{
			pUnicodes = new unsigned int[unLen];
			if (!pUnicodes)
				return S_FALSE;

			for (unsigned int unIndex = 0; unIndex < unLen; unIndex++)
				pUnicodes[unIndex] = pGids[unIndex];
		}
	}
	else
	{
		pUnicodes = WStringToUtf32(wsUnicodeText, unLen);
		if (!pUnicodes)
			return S_FALSE;
	}

	bool bRes = PathCommandDrawText(pUnicodes, unLen, dX, dY, pGids);
	RELEASEARRAYOBJECTS(pUnicodes);

	return bRes ? S_OK : S_FALSE;
}
//----------------------------------------------------------------------------------------
// Функции для вывода изображений
//----------------------------------------------------------------------------------------
HRESULT CPdfRenderer::DrawImage(IGrObject* pImage, const double& dX, const double& dY, const double& dW, const double& dH)
{
	m_oCommandManager.Flush();

	if (!IsPageValid() || !pImage)
		return S_OK;

	if (!DrawImage((Aggplus::CImage*)pImage, dX, dY, dW, dH, 255))
		return S_FALSE;

	return S_OK;
}
HRESULT CPdfRenderer::DrawImageFromFile(const std::wstring& wsImagePathSrc, const double& dX, const double& dY, const double& dW, const double& dH, const BYTE& nAlpha)
{
	m_oCommandManager.Flush();

	if (!IsPageValid())
		return S_OK;

	std::wstring sTempImagePath = GetDownloadFile(wsImagePathSrc);
	std::wstring wsImagePath = sTempImagePath.empty() ? wsImagePathSrc : sTempImagePath;

	Aggplus::CImage* pAggImage = NULL;

	CImageFileFormatChecker oImageFormat(wsImagePath);
    if (_CXIMAGE_FORMAT_WMF == oImageFormat.eFileType ||
            _CXIMAGE_FORMAT_EMF == oImageFormat.eFileType ||
            _CXIMAGE_FORMAT_SVM == oImageFormat.eFileType ||
            _CXIMAGE_FORMAT_SVG == oImageFormat.eFileType)
	{
		// TODO: Реализовать отрисовку метафайлов по-нормальному
        MetaFile::IMetaFile* pMeta = MetaFile::Create(m_pAppFonts);
        pMeta->LoadFromFile(wsImagePath.c_str());

		double dNewW = std::max(10.0, dW) / 25.4 * 300;
		std::wstring wsTempFile = GetTempFile();
        pMeta->ConvertToRaster(wsTempFile.c_str(), _CXIMAGE_FORMAT_PNG, dNewW);

        RELEASEOBJECT(pMeta);

		pAggImage = new Aggplus::CImage(wsTempFile);
	}
	else
	{
		pAggImage = new Aggplus::CImage(wsImagePath);
	}

	HRESULT hRes = S_OK;
	if (!pAggImage || !DrawImage(pAggImage, dX, dY, dW, dH, nAlpha))
		hRes = S_FALSE;

	if (NSFile::CFileBinary::Exists(sTempImagePath))
		NSFile::CFileBinary::Remove(sTempImagePath);

	if (pAggImage)
		delete pAggImage;

	return hRes;
}
//----------------------------------------------------------------------------------------
// Функции для выставления преобразования
//----------------------------------------------------------------------------------------
HRESULT CPdfRenderer::SetTransform(const double& dM11, const double& dM12, const double& dM21, const double& dM22, const double& dX, const double& dY)
{
	m_oCommandManager.Flush();
	m_oTransform.Set(dM11, dM12, dM21, dM22, dX, dY);
	return S_OK;
}
HRESULT CPdfRenderer::GetTransform(double* dM11, double* dM12, double* dM21, double* dM22, double* dX, double* dY)
{
	*dM11 = m_oTransform.m11;
	*dM12 = m_oTransform.m12;
	*dM21 = m_oTransform.m21;
	*dM22 = m_oTransform.m22;
	*dX   = m_oTransform.dx;
	*dY   = m_oTransform.dy;
	return S_OK;
}
HRESULT CPdfRenderer::ResetTransform()
{
	m_oCommandManager.Flush();
	m_oTransform.Reset();
	return S_OK;
}
//----------------------------------------------------------------------------------------
// Тип клипа
//----------------------------------------------------------------------------------------
HRESULT CPdfRenderer::get_ClipMode(LONG* lMode)
{
	*lMode = m_lClipMode;
	return S_OK;
}
HRESULT CPdfRenderer::put_ClipMode(const LONG& lMode)
{
	m_lClipMode = lMode;
	return S_OK;
}
//----------------------------------------------------------------------------------------
// Дополнительные функции
//----------------------------------------------------------------------------------------
HRESULT CPdfRenderer::CommandLong(const LONG& lType, const LONG& lCommand)
{
	return S_OK;
}
HRESULT CPdfRenderer::CommandDouble(const LONG& lType, const double& dCommand)
{
	return S_OK;
}
HRESULT CPdfRenderer::CommandString(const LONG& lType, const std::wstring& sCommand)
{
	return S_OK;
}
HRESULT CPdfRenderer::AddHyperlink(const double& dX, const double& dY, const double& dW, const double& dH, const std::wstring& wsUrl, const std::wstring& wsTooltip)
{
	NSUnicodeConverter::CUnicodeConverter conv;
	CAnnotation* pAnnot = m_pDocument->CreateUriLinkAnnot(m_pPage, TRect(MM_2_PT(dX), m_pPage->GetHeight() - MM_2_PT(dY), MM_2_PT(dX + dW), m_pPage->GetHeight() - MM_2_PT(dY + dH)), conv.SASLprepToUtf8(wsUrl).c_str());
	pAnnot->SetBorderStyle(EBorderSubtype::border_subtype_Solid, 0);
	return S_OK;
}
HRESULT CPdfRenderer::AddLink(const double& dX, const double& dY, const double& dW, const double& dH, const double& dDestX, const double& dDestY, const int& nPage)
{
	unsigned int unPagesCount = m_pDocument->GetPagesCount();
	if (unPagesCount == 0)
		return S_OK;

	CPage* pPage = m_pDocument->GetPage(nPage);
	if (!pPage)
	{
		m_vDestinations.push_back(TDestinationInfo(m_pPage, dX, dY, dW, dH, dDestX, dDestY, nPage));
	}
	else
	{
		AddLink(m_pPage, dX, dY, dW, dH, dDestX, dDestY, nPage);
	}

	return S_OK;
}
HRESULT CPdfRenderer::AddFormField(const CFormFieldInfo &oInfo)
{
	unsigned int  unPagesCount = m_pDocument->GetPagesCount();
	if (!m_pDocument || 0 == unPagesCount)
		return S_OK;

	if (m_bNeedUpdateTextFont)
		UpdateFont();

	if (!m_pFont)
		return S_OK;

	PdfWriter::CFontTrueType* pFontTT = m_pDocument->CreateTrueTypeFont(m_pFont);
	if (!pFontTT)
		return S_OK;

	double dX, dY, dW, dH;
	oInfo.GetBounds(dX, dY, dW, dH);

	CFieldBase* pFieldBase = NULL;

	bool bRadioButton = false;

	if (oInfo.IsTextField())
	{
		CTextField* pField = m_pDocument->CreateTextField();
		pFieldBase = static_cast<CFieldBase*>(pField);
	}
	else if (oInfo.IsDropDownList())
	{
		CChoiceField* pField = m_pDocument->CreateChoiceField();
		pFieldBase = static_cast<CFieldBase*>(pField);
	}
	else if (oInfo.IsCheckBox())
	{
		const CFormFieldInfo::CCheckBoxFormPr* pPr = oInfo.GetCheckBoxPr();

		CCheckBoxField* pField = NULL;
		std::wstring wsGroupName = pPr->GetGroupKey();
		if (L"" != wsGroupName)
		{
			bRadioButton = true;
			CRadioGroupField* pRadioGroup = m_pDocument->GetRadioGroupField(wsGroupName);
			if (pRadioGroup)
				pField = pRadioGroup->CreateKid();
		}
		else
		{
			pField = m_pDocument->CreateCheckBoxField();
		}

		pFieldBase = static_cast<CFieldBase*>(pField);
	}
	else if (oInfo.IsPicture())
	{
		CPictureField* pField = m_pDocument->CreatePictureField();
		pFieldBase = static_cast<CFieldBase*>(pField);
	}
	else if (oInfo.IsSignature())
	{
		CSignatureField* pField = m_pDocument->CreateSignatureField();
		pFieldBase = static_cast<CFieldBase*>(pField);
	}

	if (!pFieldBase)
		return S_FALSE;

	// 0 - Right
	// 1 - Left
	// 2 - Center
	// 3 - Justify
	// 4 - Distributed
	unsigned int unAlign = oInfo.GetJc();
	if (0 == unAlign)
		pFieldBase->SetAlign(CFieldBase::EFieldAlignType::Right);
	else if (2 == unAlign)
		pFieldBase->SetAlign(CFieldBase::EFieldAlignType::Center);

	if (oInfo.HaveBorder())
	{
		unsigned char unR, unG, unB, unA;
		oInfo.GetBorderColor(unR, unG, unB, unA);

		pFieldBase->SetFieldBorder(EBorderSubtype::border_subtype_Solid, TRgb(unR, unG, unB), MM_2_PT(oInfo.GetBorderSize()), 0, 0, 0);
	}

	if (oInfo.HaveShd())
	{
		unsigned char unR, unG, unB, unA;
		oInfo.GetShdColor(unR, unG, unB, unA);
		pFieldBase->SetShd(TRgb(unR, unG, unB));
	}

	pFieldBase->SetRequiredFlag(oInfo.IsRequired());
	pFieldBase->SetFieldHint(oInfo.GetHelpText());

	bool isBold   = m_oFont.IsBold();
	bool isItalic = m_oFont.IsItalic();


	if (oInfo.IsTextField())
	{
		const CFormFieldInfo::CTextFormPr* pPr = oInfo.GetTextPr();
		std::wstring wsValue = pPr->GetTextValue();

		unsigned int unLen;
		unsigned int* pUnicodes = WStringToUtf32(wsValue, unLen);
		if (!pUnicodes)
			return S_FALSE;

		unsigned short* pCodes = new unsigned short[unLen];
		if (!pCodes)
		{
			RELEASEARRAYOBJECTS(pUnicodes);
			return S_FALSE;
		}

		CFontCidTrueType** ppFonts = new CFontCidTrueType*[unLen];
		if (!ppFonts)
		{
			RELEASEARRAYOBJECTS(pUnicodes);
			RELEASEARRAYOBJECTS(pCodes);
			return S_FALSE;
		}

		for (unsigned int unIndex = 0; unIndex < unLen; ++unIndex)
		{
			unsigned int unUnicode = pUnicodes[unIndex];

			if (!m_pFont->HaveChar(unUnicode))
			{
				std::wstring wsFontFamily   = m_pAppFonts->GetFontBySymbol(unUnicode);
				CFontCidTrueType* pTempFont = GetFont(wsFontFamily, isBold, isItalic);
				if (pTempFont)
				{
					pCodes[unIndex]  = pTempFont->EncodeUnicode(unUnicode);
					ppFonts[unIndex] = pTempFont;
					continue;
				}
			}

			pCodes[unIndex]  = m_pFont->EncodeUnicode(unUnicode);
			ppFonts[unIndex] = m_pFont;
		}

		CTextField* pField = dynamic_cast<CTextField*>(pFieldBase);
		if (!pField)
		{
			RELEASEARRAYOBJECTS(pUnicodes);
			RELEASEARRAYOBJECTS(pCodes);
			RELEASEARRAYOBJECTS(ppFonts);
			return S_FALSE;
		}

		double _dY = m_pPage->GetHeight() - MM_2_PT(dY);
		double _dB = m_pPage->GetHeight() - MM_2_PT(dY + dH);

		double dMargin   = 2; // такой отступ используется в AdobeReader
		double dBaseLine = MM_2_PT(dH - oInfo.GetBaseLineOffset());
		double dShiftX   = dMargin;

		pFieldBase->AddPageRect(m_pPage, TRect(MM_2_PT(dX) - dMargin, _dY, MM_2_PT(dX + dW) + dMargin, _dB));

		pField->SetMaxLen(pPr->GetMaxCharacters());
		pField->SetCombFlag(pPr->IsComb());
		pField->SetAutoFit(pPr->IsAutoFit());
		pField->SetMultilineFlag(pPr->IsMultiLine());

		bool isComb = pPr->IsComb();
		unsigned int unAlign = oInfo.GetJc();
		double dFontSize = m_oFont.GetSize();

		TColor oColor      = m_oBrush.GetTColor1();
		bool isPlaceHolder = oInfo.IsPlaceHolder();

		TRgb oNormalColor(oColor.r, oColor.g, oColor.b);
		TRgb oPlaceHolderColor;
		oPlaceHolderColor.r = oNormalColor.r + (1.0 - oNormalColor.r) / 2.0;
		oPlaceHolderColor.g = oNormalColor.g + (1.0 - oNormalColor.g) / 2.0;
		oPlaceHolderColor.b = oNormalColor.b + (1.0 - oNormalColor.b) / 2.0;

		if (!isPlaceHolder)
			pField->SetTextValue(wsValue);

		if (!isComb && pPr->IsMultiLine())
		{
			unsigned short* pCodes2 = new unsigned short[unLen];
			unsigned int* pWidths   = new unsigned int[unLen];

			unsigned short ushSpaceCode = 0xFFFF;
			for (unsigned int unIndex = 0; unIndex < unLen; ++unIndex)
			{
				pCodes2[unIndex] = (0x0020 == pUnicodes[unIndex] ? ushSpaceCode : 0);
				pWidths[unIndex] = ppFonts[unIndex]->GetWidth(pCodes[unIndex]);
			}

			m_oLinesManager.Init(pCodes2, pWidths, unLen, ushSpaceCode, pFontTT->GetLineHeight(), pFontTT->GetAscent());

			// TODO: Разобраться более детально по какой именно высоте идет в Adobe расчет
			//       пока временно оставим (H - 3 * margin)
			if (pPr->IsAutoFit())
				dFontSize = m_oLinesManager.ProcessAutoFit(MM_2_PT(dW), (MM_2_PT(dH) - 3 * dMargin));

			double dLineHeight = pFontTT->GetLineHeight() * dFontSize / 1000.0;

			m_oLinesManager.CalculateLines(dFontSize, MM_2_PT(dW));

			pField->StartTextAppearance(m_pFont, dFontSize, isPlaceHolder ? oPlaceHolderColor : oNormalColor, 1.0);

			unsigned int unLinesCount = m_oLinesManager.GetLinesCount();
			double dLineShiftY = MM_2_PT(dH) - pFontTT->GetLineHeight() * dFontSize / 1000.0 - dMargin;
			for (unsigned int unIndex = 0; unIndex < unLinesCount; ++unIndex)
			{
				unsigned int unLineStart = m_oLinesManager.GetLineStartPos(unIndex);
				double dLineShiftX = dShiftX;
				double dLineWidth = m_oLinesManager.GetLineWidth(unIndex, dFontSize);
				if (0 == unAlign)
					dLineShiftX += MM_2_PT(dW) - dLineWidth;
				else if (2 == unAlign)
					dLineShiftX += (MM_2_PT(dW) - dLineWidth) / 2;

				int nInLineCount = m_oLinesManager.GetLineEndPos(unIndex) - m_oLinesManager.GetLineStartPos(unIndex);
				if (nInLineCount > 0)
					pField->AddLineToTextAppearance(dLineShiftX, dLineShiftY, pCodes + unLineStart, nInLineCount, ppFonts + unLineStart, NULL);

				dLineShiftY -= dLineHeight;
			}

			pField->EndTextAppearance();

			m_oLinesManager.Clear();

			delete[] pCodes2;
			delete[] pWidths;
		}
		else
		{
			double* pShifts = NULL;
			unsigned int unShiftsCount = 0;

			if (isComb)
			{
				pField->SetDoNotScrollFlag(true);
				pField->SetDoNotSpellCheckFlag(true);
				pField->SetMultilineFlag(false);

				unShiftsCount = unLen;
				pShifts = new double[unShiftsCount];
				if (pShifts && unShiftsCount)
				{
					// Сдвиг нулевой для comb форм и не забываем, что мы к ширине добавили 2 * dMargin
					dShiftX = 0;
					unsigned int unCellsCount = std::max(unShiftsCount, pPr->GetMaxCharacters());
					double dPrevW = 0;
					double dCellW = (MM_2_PT(dW) + 2 * dMargin) / unCellsCount;

					if (0 == unAlign && unShiftsCount)
						dPrevW = (unCellsCount - unShiftsCount) * dCellW;

					for (unsigned int unIndex = 0; unIndex < unShiftsCount; ++unIndex)
					{
						unsigned short ushCode = pCodes[unIndex];
						double dGlyphWidth = ppFonts[unIndex]->GetGlyphWidth(ushCode) / 1000.0 * dFontSize;
						double dTempShift = (dCellW - dGlyphWidth) / 2;
						pShifts[unIndex] = dPrevW + dTempShift;
						dPrevW = dCellW - dTempShift;

					}
				}
				else
				{
					unShiftsCount = 0;
				}
			}
			else if (0 == unAlign || 2 == unAlign)
			{
				double dSumWidth = 0;
				for (unsigned int unIndex = 0; unIndex < unLen; ++unIndex)
				{
					unsigned short ushCode = pCodes[unIndex];
					double dLetterWidth    = ppFonts[unIndex]->GetWidth(ushCode) / 1000.0 * dFontSize;
					dSumWidth += dLetterWidth;
				}

				if (0 == unAlign && MM_2_PT(dW) - dSumWidth > 0)
					dShiftX += MM_2_PT(dW) - dSumWidth;
				else if (2 == unAlign && (MM_2_PT(dW) - dSumWidth) / 2 > 0)
					dShiftX += (MM_2_PT(dW) - dSumWidth) / 2;
			}

			pField->SetTextAppearance(wsValue, pCodes, unLen, m_pFont, isPlaceHolder ? oPlaceHolderColor : oNormalColor, 1.0, m_oFont.GetSize(), dShiftX, dBaseLine, ppFonts, pShifts);
			RELEASEARRAYOBJECTS(pShifts);
		}

		RELEASEARRAYOBJECTS(pUnicodes);
		RELEASEARRAYOBJECTS(pCodes);
		RELEASEARRAYOBJECTS(ppFonts);

		pField->SetDefaultAppearance(pFontTT, m_oFont.GetSize(), TRgb(oColor.r, oColor.g, oColor.b));

		std::wstring wsPlaceHolder = pPr->GetPlaceHolder();
		if (!wsPlaceHolder.empty())
		{
			unsigned int unMaxLen = pPr->GetMaxCharacters();
			if (unMaxLen && wsPlaceHolder.length() > unMaxLen)
				wsPlaceHolder = wsPlaceHolder.substr(0, unMaxLen);

			pField->SetPlaceHolderText(wsPlaceHolder, oNormalColor, oPlaceHolderColor);
		}

		pField->SetFormat(pPr->GetFormatPr());
	}
	else if (oInfo.IsDropDownList())
	{
		const CFormFieldInfo::CDropDownFormPr* pPr = oInfo.GetDropDownPr();
		std::wstring wsValue = pPr->GetTextValue();

		unsigned int unLen;
		unsigned int* pUnicodes = WStringToUtf32(wsValue, unLen);
		if (!pUnicodes)
			return S_FALSE;

		unsigned short* pCodes = new unsigned short[unLen];
		if (!pCodes)
		{
			RELEASEARRAYOBJECTS(pUnicodes);
			return S_FALSE;
		}

		CFontCidTrueType** ppFonts = new CFontCidTrueType*[unLen];
		if (!ppFonts)
		{
			RELEASEARRAYOBJECTS(pUnicodes);
			RELEASEARRAYOBJECTS(pCodes);
			return S_FALSE;
		}

		for (unsigned int unIndex = 0; unIndex < unLen; ++unIndex)
		{
			unsigned int unUnicode = pUnicodes[unIndex];

			if (!m_pFont->HaveChar(unUnicode))
			{
				std::wstring wsFontFamily   = m_pAppFonts->GetFontBySymbol(unUnicode);
				CFontCidTrueType* pTempFont = GetFont(wsFontFamily, isBold, isItalic);
				if (pTempFont)
				{
					pCodes[unIndex]  = pTempFont->EncodeUnicode(unUnicode);
					ppFonts[unIndex] = pTempFont;
					continue;
				}
			}
			pCodes[unIndex]  = m_pFont->EncodeUnicode(unUnicode);
			ppFonts[unIndex] = m_pFont;
		}

		CChoiceField* pField = dynamic_cast<CChoiceField*>(pFieldBase);
		if (!pField)
		{
			RELEASEARRAYOBJECTS(pUnicodes);
			RELEASEARRAYOBJECTS(pCodes);
			RELEASEARRAYOBJECTS(ppFonts);
			return S_FALSE;
		}

		pFieldBase->AddPageRect(m_pPage, TRect(MM_2_PT(dX), m_pPage->GetHeight() - MM_2_PT(dY), MM_2_PT(dX + dW), m_pPage->GetHeight() - MM_2_PT(dY + dH)));

		TColor oColor = m_oBrush.GetTColor1();

		TRgb oNormalColor(oColor.r, oColor.g, oColor.b);
		TRgb oPlaceHolderColor;
		oPlaceHolderColor.r = oNormalColor.r + (1.0 - oNormalColor.r) / 2.0;
		oPlaceHolderColor.g = oNormalColor.g + (1.0 - oNormalColor.g) / 2.0;
		oPlaceHolderColor.b = oNormalColor.b + (1.0 - oNormalColor.b) / 2.0;

		pField->SetTextValue(wsValue);
		pField->SetTextAppearance(wsValue, pCodes, unLen, m_pFont, oInfo.IsPlaceHolder() ? oPlaceHolderColor : oNormalColor, 1, m_oFont.GetSize(), 0, MM_2_PT(dH - oInfo.GetBaseLineOffset()), ppFonts);

		RELEASEARRAYOBJECTS(pUnicodes);
		RELEASEARRAYOBJECTS(pCodes);
		RELEASEARRAYOBJECTS(ppFonts);

		unsigned int unSelectedIndex = 0xFFFF;
		for (unsigned int unIndex = 0, unItemsCount = pPr->GetComboBoxItemsCount(); unIndex < unItemsCount; ++unIndex)
		{
			std::wstring wsItem = pPr->GetComboBoxItem(unIndex);
			pField->AddOption(wsItem);
			if (wsItem == wsValue)
				unSelectedIndex = unIndex;
		}

		pField->SetComboFlag(true);
		pField->SetEditFlag(pPr->IsEditComboBox());

		pField->SetDefaultAppearance(pFontTT, m_oFont.GetSize(), oInfo.IsPlaceHolder() ? oPlaceHolderColor : oNormalColor);

		if (!pPr->GetPlaceHolder().empty())
		{
			pField->SetPlaceHolderText(pPr->GetPlaceHolder(), oNormalColor, oPlaceHolderColor);

			if (!pPr->IsEditComboBox())
			{
				// Для drop-down list в 0 позиции мы добавили плейсхолдер
				if (oInfo.IsPlaceHolder())
					unSelectedIndex = 0;
				else if (0xFFFF != unSelectedIndex)
					unSelectedIndex++;
			}
		}

		if (!pPr->IsEditComboBox() && 0xFFFF != unSelectedIndex)
			pField->SetSelectedIndex(unSelectedIndex);
	}
	else if (oInfo.IsCheckBox())
	{
		const CFormFieldInfo::CCheckBoxFormPr* pPr = oInfo.GetCheckBoxPr();

		CCheckBoxField* pField = dynamic_cast<CCheckBoxField*>(pFieldBase);
		if (!pField)
			return S_FALSE;

		pFieldBase->AddPageRect(m_pPage, TRect(MM_2_PT(dX), m_pPage->GetHeight() - MM_2_PT(dY), MM_2_PT(dX + dW), m_pPage->GetHeight() - MM_2_PT(dY + dH)));
		pField->SetValue(pPr->IsChecked());

		CFontCidTrueType* pCheckedFont   = GetFont(pPr->GetCheckedFontName(), false, false);
		CFontCidTrueType* pUncheckedFont = GetFont(pPr->GetUncheckedFontName(), false, false);
		if (!pCheckedFont)
			pCheckedFont = m_pFont;

		if (!pUncheckedFont)
			pUncheckedFont = m_pFont;

		unsigned int unCheckedSymbol   = pPr->GetCheckedSymbol();
		unsigned int unUncheckedSymbol = pPr->GetUncheckedSymbol();

		unsigned short ushCheckedCode   = pCheckedFont->EncodeUnicode(unCheckedSymbol);
		unsigned short ushUncheckedCode = pUncheckedFont->EncodeUnicode(unUncheckedSymbol);

		TColor oColor = m_oBrush.GetTColor1();
		pField->SetAppearance(L"", &ushCheckedCode, 1, pCheckedFont, L"", &ushUncheckedCode, 1, pUncheckedFont, TRgb(oColor.r, oColor.g, oColor.b), 1, m_oFont.GetSize(), 0, MM_2_PT(dH - oInfo.GetBaseLineOffset()));
	}
	else if (oInfo.IsPicture())
	{
		const CFormFieldInfo::CPictureFormPr* pPr = oInfo.GetPicturePr();

		CPictureField* pField = dynamic_cast<CPictureField*>(pFieldBase);
		pFieldBase->AddPageRect(m_pPage, TRect(MM_2_PT(dX), m_pPage->GetHeight() - MM_2_PT(dY), MM_2_PT(dX + dW), m_pPage->GetHeight() - MM_2_PT(dY + dH)));		
		pField->SetConstantProportions(pPr->IsConstantProportions());
		pField->SetRespectBorders(pPr->IsRespectBorders());
		pField->SetScaleType(static_cast<CPictureField::EScaleType>(pPr->GetScaleType()));
		pField->SetShift(pPr->GetShiftX() / 1000.0, (1000 - pPr->GetShiftY()) / 1000.0);

		std::wstring wsPath = pPr->GetPicturePath();
		CImageDict* pImage = NULL;
		if (wsPath.length())
		{
			Aggplus::CImage oImage(wsPath);
			pImage = LoadImage(&oImage, 255);
		}

		pField->SetAppearance(pImage);
	}
	else if (oInfo.IsSignature())
	{
		const CFormFieldInfo::CSignatureFormPr* pPr = oInfo.GetSignaturePr();

		CSignatureField* pField = dynamic_cast<CSignatureField*>(pFieldBase);
		if (!pField)
			return S_FALSE;

		pFieldBase->AddPageRect(m_pPage, TRect(MM_2_PT(dX), m_pPage->GetHeight() - MM_2_PT(dY), MM_2_PT(dX + dW), m_pPage->GetHeight() - MM_2_PT(dY + dH)));
		pField->SetName(pPr->GetName());
		pField->SetReason(pPr->GetReason());
		pField->SetContact(pPr->GetContact());
		pField->SetDate(pPr->GetDate());

		std::wstring wsPath = pPr->GetPicturePath();
		CImageDict* pImage = NULL;
		if (!wsPath.empty())
		{
			Aggplus::CImage oImage(wsPath);
			pImage = LoadImage(&oImage, 255);
		}

		pField->SetAppearance(pImage);

		// TODO Реализовать, когда появится поддержка CSignatureField
		pField->SetCert();
	}


	// Выставляем имя в конце, потому что там возможно копирование настроек поля в новое родительское поле, поэтому к текущему моменту
	// все настройки должны быть выставлены
	if (!bRadioButton)
	{
		std::wstring wsKey = oInfo.GetKey();
		if (L"" != wsKey)
			pFieldBase->SetFieldName(wsKey);
		else
			pFieldBase->SetFieldName(m_oFieldsManager.GetNewFieldName());
	}

	return S_OK;
}
//----------------------------------------------------------------------------------------
// Дополнительные функции Pdf рендерера
//----------------------------------------------------------------------------------------
HRESULT CPdfRenderer::CommandDrawTextPdf(const std::wstring& bsUnicodeText, const unsigned int* pGids, const unsigned int unGidsCount, const std::wstring& bsSrcCodeText, const double& dX, const double& dY, const double& dW, const double& dH)
{
	return S_OK;
}
HRESULT CPdfRenderer::PathCommandTextPdf(const std::wstring& bsUnicodeText, const unsigned int* pGids, const unsigned int unGidsCount, const std::wstring& bsSrcCodeText, const double& dX, const double& dY, const double& dW, const double& dH)
{
	return S_OK;
}
HRESULT CPdfRenderer::DrawImage1bpp(NSImages::CPixJbig2* pImageBuffer, const unsigned int& unWidth, const unsigned int& unHeight, const double& dX, const double& dY, const double& dW, const double& dH)
{
	m_oCommandManager.Flush();

	if (!IsPageValid() || !pImageBuffer)
		return S_OK;

	m_pPage->GrSave();
	UpdateTransform();
	
	CImageDict* pPdfImage = m_pDocument->CreateImage();
    pPdfImage->LoadBW(pImageBuffer, unWidth, unHeight);
	m_pPage->DrawImage(pPdfImage, MM_2_PT(dX), MM_2_PT(m_dPageHeight - dY - dH), MM_2_PT(dW), MM_2_PT(dH));

	m_pPage->GrRestore();
	return S_OK;
}
HRESULT CPdfRenderer::EnableBrushRect(const LONG& lEnable)
{
	m_oBrush.EnableBrushRect(LONG_2_BOOL(lEnable));
	return S_OK;
}
HRESULT CPdfRenderer::SetLinearGradient(const double& dX0, const double& dY0, const double& dX1, const double& dY1)
{
	m_oBrush.SetType(c_BrushTypeLinearGradient);
	m_oBrush.SetLinearGradientPattern(dX0, dY0, dX1, dY1);
	return S_OK;
}
HRESULT CPdfRenderer::SetRadialGradient(const double& dX0, const double& dY0, const double& dR0, const double& dX1, const double& dY1, const double& dR1)
{
	m_oBrush.SetType(c_BrushTypeRadialGradient);
	m_oBrush.SetRadialGradientPattern(dX0, dY0, dR0, dX1, dY1, dR1);
	return S_OK;
}
HRESULT CPdfRenderer::DrawImageWith1bppMask(IGrObject* pImage, NSImages::CPixJbig2* pMaskBuffer, const unsigned int& unMaskWidth, const unsigned int& unMaskHeight, const double& dX, const double& dY, const double& dW, const double& dH)
{
	m_oCommandManager.Flush();

	if (!IsPageValid() || !pMaskBuffer || !pImage)
		return S_OK;

	m_pPage->GrSave();
	UpdateTransform();
	CImageDict* pPdfImage = LoadImage((Aggplus::CImage*)pImage, 255);
    pPdfImage->LoadMask(pMaskBuffer, unMaskWidth, unMaskHeight);
	m_pPage->DrawImage(pPdfImage, MM_2_PT(dX), MM_2_PT(m_dPageHeight - dY - dH), MM_2_PT(dW), MM_2_PT(dH));
	m_pPage->GrRestore();
	return S_OK;
}
bool CPdfRenderer::EditPdf(const std::wstring& wsPath, int nPosLastXRef, int nSizeXRef, const std::wstring& sCatalog, int nCatalog, const std::wstring& sEncrypt, const std::wstring& sPassword, int nCryptAlgorithm, int nFormField)
{
	bool bRes = m_pDocument->EditPdf(wsPath, nPosLastXRef, nSizeXRef, sCatalog, nCatalog, sEncrypt, sPassword, nCryptAlgorithm, nFormField);
	if (bRes)
	{
		m_bEdit = true;
	}
	return bRes;
}
bool CPdfRenderer::CreatePageTree(const std::wstring& sPageTree, int nPageTree)
{
	if (!m_bEdit)
	{
		return false;
	}
	return m_pDocument->CreatePageTree(sPageTree, nPageTree);
}
std::pair<int, int> CPdfRenderer::GetPageRef(int nPageIndex)
{
	return m_pDocument->GetPageRef(nPageIndex);
}
bool CPdfRenderer::EditPage(const std::wstring& sPage, int nPage)
{
	if (!IsValid() || !m_bEdit)
		return false;
	m_oCommandManager.Flush();

	m_pPage = m_pDocument->EditPage(sPage, nPage);
	if (m_pPage)
	{
		m_dPageWidth  = PT_2_MM(m_pPage->GetWidth());
		m_dPageHeight = PT_2_MM(m_pPage->GetHeight());
		Reset();
		m_bEditPage = true;
		return true;
	}
	return false;
}
bool CPdfRenderer::AddPage(int nPageIndex)
{
	if (!IsValid() || !m_bEdit)
		return false;
	m_oCommandManager.Flush();

	m_pPage = m_pDocument->AddPage(nPageIndex);
	if (m_pPage)
	{
		m_pPage->SetWidth(MM_2_PT(m_dPageWidth));
		m_pPage->SetHeight(MM_2_PT(m_dPageHeight));
		Reset();
		m_bEditPage = false;
		return true;
	}
	return false;
}
bool CPdfRenderer::DeletePage(int nPageIndex)
{
	if (!m_bEdit)
	{
		return false;
	}
	return m_pDocument->DeletePage(nPageIndex);
}
bool CPdfRenderer::EditClose(const std::wstring& sTrailer, const std::wstring& sInfo)
{
	if (!IsValid() || !m_bEdit)
		return false;
	m_oCommandManager.Flush();

	unsigned int nPagesCount = m_pDocument->GetPagesCount();
	for (int nIndex = 0, nCount = m_vDestinations.size(); nIndex < nCount; ++nIndex)
	{
		TDestinationInfo& oInfo = m_vDestinations.at(nIndex);
		if (nPagesCount > oInfo.unDestPage)
		{
			AddLink(oInfo.pPage, oInfo.dX, oInfo.dY, oInfo.dW, oInfo.dH, oInfo.dDestX, oInfo.dDestY, oInfo.unDestPage);
			m_vDestinations.erase(m_vDestinations.begin() + nIndex);
			nIndex--;
			nCount--;
		}
	}

	bool bRes = m_pDocument->AddToFile(sTrailer, sInfo);
	if (bRes)
	{
		m_bEdit = false;
		m_bEditPage = false;
	}
	return bRes;
}
void CPdfRenderer::PageRotate(int nRotate)
{
	if (m_pPage && m_bEdit)
		m_pPage->SetRotate(nRotate);
}
void CPdfRenderer::Sign(const double& dX, const double& dY, const double& dW, const double& dH, const std::wstring& wsPicturePath, ICertificate* pCertificate)
{
	if (!m_bEdit)
	{
		return;
	}
	CImageDict* pImage = NULL;
	if (!wsPicturePath.empty())
	{
		Aggplus::CImage oImage(wsPicturePath);
		pImage = LoadImage(&oImage, 255);
	}

	m_pDocument->Sign(TRect(MM_2_PT(dX), m_pPage->GetHeight() - MM_2_PT(dY), MM_2_PT(dX + dW), m_pPage->GetHeight() - MM_2_PT(dY + dH)),
		pImage, pCertificate);
}
std::wstring CPdfRenderer::GetEditPdfPath()
{
	if (!m_bEdit)
	{
		return L"";
	}
	return m_pDocument->GetEditPdfPath();
}

NSFonts::IApplicationFonts* CPdfRenderer::GetApplicationFonts()
{
    return m_pAppFonts;
}
//----------------------------------------------------------------------------------------
// Внутренние функции
//----------------------------------------------------------------------------------------
PdfWriter::CImageDict* CPdfRenderer::LoadImage(Aggplus::CImage* pImage, const BYTE& nAlpha)
{
	int nImageW = abs((int)pImage->GetWidth());
	int nImageH = abs((int)pImage->GetHeight());
	BYTE* pData = pImage->GetData();
	int nStride = 4 * nImageW;

	// Картинки совсем маленьких размеров нельзя делать Jpeg2000
	bool bJpeg = false;
	if (nImageH < 100 || nImageW < 100 || m_pDocument->IsPDFA())
		bJpeg = true;

	if (nImageH <= 0 || nImageW <= 0)
		return NULL;

	// TODO: Пока не разберемся как в CxImage управлять параметрами кодирования нельзя писать в Jpeg2000,
	//       т.к. файлы получаются гораздо больше и конвертация идет намного дольше.
	bJpeg = true;

	// Пробегаемся по картинке и определяем есть ли у нас альфа-канал
	bool bAlpha = false;

	CBgraFrame oFrame;
	if (m_pDocument->IsPDFA())
	{
		BYTE* pCopyImage = new BYTE[4 * nImageW * nImageH];
		if (!pCopyImage)
			return NULL;

		::memcpy(pCopyImage, pData, 4 * nImageW * nImageH);

        BYTE* pDataMem = pCopyImage;
		for (int nIndex = 0, nSize = nImageW * nImageH; nIndex < nSize; nIndex++)
		{
            if (pDataMem[3] < 32)
			{
                pDataMem[0] = 255;
                pDataMem[1] = 255;
                pDataMem[2] = 255;
			}
            pDataMem += 4;
		}

		oFrame.put_Width(nImageW);
		oFrame.put_Height(nImageH);
		oFrame.put_Data(pCopyImage);// +4 * (nImageH - 1) * nImageW);
		oFrame.put_Stride(-4* nImageW);
	}
	else
	{
        BYTE* pDataMem = pData;
		for (int nIndex = 0, nSize = nImageW * nImageH; nIndex < nSize; nIndex++)
		{
            // making full-transparent pixels white
			if (pDataMem[3] == 0)
			{
				pDataMem[0] = 255;
				pDataMem[1] = 255;
				pDataMem[2] = 255;
			}

            if (!bAlpha && (pDataMem[3] < 255))
			{
                bAlpha = true;
			}

            pDataMem += 4;
		}
		oFrame.FromImage(pImage);
	}

    oFrame.SetJpegQuality(85.0);

	BYTE* pBuffer = NULL;
	int nBufferSize = 0;
    if (!oFrame.Encode(pBuffer, nBufferSize, bJpeg ? _CXIMAGE_FORMAT_JPG : _CXIMAGE_FORMAT_JP2))
		return NULL;

	if (!pBuffer || !nBufferSize)
		return NULL;

	CImageDict* pPdfImage = m_pDocument->CreateImage();
	if (bAlpha || nAlpha < 255)
		pPdfImage->LoadSMask(pData, nImageW, nImageH, nAlpha, (pImage->GetStride() >= 0) ? false : true);

	if (bJpeg)
        pPdfImage->LoadJpeg(pBuffer, nBufferSize, nImageW, nImageH);
	else
		pPdfImage->LoadJpx(pBuffer, nBufferSize, nImageW, nImageH);

	free(pBuffer);

	return pPdfImage;
}
bool CPdfRenderer::DrawImage(Aggplus::CImage* pImage, const double& dX, const double& dY, const double& dW, const double& dH, const BYTE& nAlpha)
{
	CImageDict* pPdfImage = LoadImage(pImage, nAlpha);
	if (!pPdfImage)
		return false;

	m_pPage->GrSave();
	UpdateTransform();
	m_pPage->DrawImage(pPdfImage, MM_2_PT(dX), MM_2_PT(m_dPageHeight - dY - dH), MM_2_PT(dW), MM_2_PT(dH));
	m_pPage->GrRestore();
	
	return true;
}
bool CPdfRenderer::DrawText(unsigned char* pCodes, const unsigned int& unLen, const double& dX, const double& dY)
{
	if (!pCodes || !unLen)
		return false;

	CTransform& t = m_oTransform;
	m_oCommandManager.SetTransform(t.m11, -t.m12, -t.m21, t.m22, MM_2_PT(t.dx + t.m21 * m_dPageHeight), MM_2_PT(m_dPageHeight - m_dPageHeight * t.m22 - t.dy));

	CRendererTextCommand* pText = m_oCommandManager.AddText(pCodes, unLen, MM_2_PT(dX), MM_2_PT(m_dPageHeight - dY));
	pText->SetFont(m_pFont);
	pText->SetSize(m_oFont.GetSize());
	pText->SetColor(m_oBrush.GetColor1());
	pText->SetAlpha((BYTE)m_oBrush.GetAlpha1());
	pText->SetCharSpace(MM_2_PT(m_oFont.GetCharSpace()));
	pText->SetNeedDoBold(m_oFont.IsNeedDoBold());
	pText->SetNeedDoItalic(m_oFont.IsNeedDoItalic());

	return true;
}
bool CPdfRenderer::PathCommandDrawText(unsigned int* pUnicodes, unsigned int unLen, const double& dX, const double& dY, const unsigned int* pGids)
{
	unsigned char* pCodes = EncodeString(pUnicodes, unLen, pGids);
	if (!pCodes)
		return false;

	m_oPath.AddText(m_pFont, pCodes, unLen * 2, MM_2_PT(dX), MM_2_PT(m_dPageHeight - dY), m_oFont.GetSize(), MM_2_PT(m_oFont.GetCharSpace()));
	return true;
}
void CPdfRenderer::UpdateFont()
{
	m_bNeedUpdateTextFont = false;
    std::wstring wsFontPath = m_oFont.GetPath();
	LONG lFaceIndex         = m_oFont.GetFaceIndex();
	if (L"" == wsFontPath)
		GetFontPath(m_oFont.GetName(), m_oFont.IsBold(), m_oFont.IsItalic(), wsFontPath, lFaceIndex);

	m_oFont.SetNeedDoBold(false);
	m_oFont.SetNeedDoItalic(false);

	m_pFont = NULL;
	if (L"" != wsFontPath)
	{
		m_pFont = GetFont(wsFontPath, lFaceIndex);
		if (m_pFont)
		{
			if (m_oFont.IsItalic() && !m_pFont->IsItalic())
				m_oFont.SetNeedDoItalic(true);

			if (m_oFont.IsBold() && !m_pFont->IsBold())
				m_oFont.SetNeedDoBold(true);
		}
	}
}
void CPdfRenderer::GetFontPath(const std::wstring &wsFontName, const bool &bBold, const bool &bItalic, std::wstring& wsFontPath, LONG& lFaceIndex)
{
	bool bFind = false;
	for (int nIndex = 0, nCount = m_vFonts.size(); nIndex < nCount; nIndex++)
	{
		TFontInfo& oInfo = m_vFonts.at(nIndex);
		if (oInfo.wsFontName == wsFontName && oInfo.bBold == bBold && oInfo.bItalic == bItalic)
		{
			wsFontPath = oInfo.wsFontPath;
			lFaceIndex = oInfo.lFaceIndex;
			bFind = true;
			break;
		}
	}

	if (!bFind)
	{
		NSFonts::CFontSelectFormat oFontSelect;
		oFontSelect.wsName  = new std::wstring(wsFontName);
		oFontSelect.bItalic = new INT(bItalic ? 1 : 0);
		oFontSelect.bBold   = new INT(bBold ? 1 : 0);
		NSFonts::CFontInfo* pFontInfo = m_pFontManager->GetFontInfoByParams(oFontSelect, false);
		if (!NSFonts::CFontInfo::CanEmbedForPreviewAndPrint(pFontInfo->m_usType))
		{
			oFontSelect.Fill(pFontInfo);
			if (NULL != oFontSelect.usType)
				*oFontSelect.usType = NSFONTS_EMBEDDING_RIGHTS_PRINT_AND_PREVIEW;
			else
				oFontSelect.usType = new USHORT(NSFONTS_EMBEDDING_RIGHTS_PRINT_AND_PREVIEW);

			pFontInfo = m_pFontManager->GetFontInfoByParams(oFontSelect, false);
		}

		wsFontPath = pFontInfo->m_wsFontPath;
		lFaceIndex = pFontInfo->m_lIndex;

		m_vFonts.push_back(TFontInfo(wsFontName, bBold, bItalic, wsFontPath, lFaceIndex));
	}
}
PdfWriter::CFontCidTrueType* CPdfRenderer::GetFont(const std::wstring& wsFontPath, const LONG& lFaceIndex)
{
	PdfWriter::CFontCidTrueType* pFont = NULL;
	if (L"" != wsFontPath)
	{
		pFont = m_pDocument->FindCidTrueTypeFont(wsFontPath, lFaceIndex);
		if (pFont)
			return pFont;

		// TODO: Пока мы здесь предполагаем, что шрифты только либо TrueType, либо OpenType
		if (!m_pFontManager->LoadFontFromFile(wsFontPath, lFaceIndex, 10, 72, 72))
		{
			std::wcout << L"PDF Writer: Can't load fontfile " << wsFontPath.c_str() << "\n";
			return NULL;
		}

		std::wstring wsFontType = m_pFontManager->GetFontType();
		if (L"TrueType" == wsFontType || L"OpenType" == wsFontType || L"CFF" == wsFontType)
			pFont = m_pDocument->CreateCidTrueTypeFont(wsFontPath, lFaceIndex);
	}

	return pFont;
}
PdfWriter::CFontCidTrueType* CPdfRenderer::GetFont(const std::wstring& wsFontName, const bool& bBold, const bool& bItalic)
{
	std::wstring wsFontPath;
	LONG lFaceIndex;

	GetFontPath(wsFontName, bBold, bItalic, wsFontPath, lFaceIndex);
	return GetFont(wsFontPath, lFaceIndex);
}
void CPdfRenderer::UpdateTransform()
{
	CTransform& t = m_oTransform;
	m_pPage->SetTransform(t.m11, -t.m12, -t.m21, t.m22, MM_2_PT(t.dx + t.m21 * m_dPageHeight), MM_2_PT(m_dPageHeight - m_dPageHeight * t.m22 - t.dy));
}
void CPdfRenderer::UpdatePen()
{
    TColor oColor = m_oPen.GetTColor();
	m_pPage->SetStrokeColor(oColor.r, oColor.g, oColor.b);
	m_pPage->SetStrokeAlpha((unsigned char)m_oPen.GetAlpha());
	m_pPage->SetLineWidth(MM_2_PT(m_oPen.GetSize()));
	
	LONG lDashCount = 0;
	double* pDashPattern = NULL;
	
	LONG lDashStyle = m_oPen.GetDashStyle();
	if (Aggplus::DashStyleSolid == lDashStyle)
	{
		// Ничего не делаем
	}
	else if (Aggplus::DashStyleCustom == lDashStyle)
	{
		double *pDashPatternMM = m_oPen.GetDashPattern(lDashCount);
		if (pDashPatternMM && lDashCount)
		{
			pDashPattern = new double[lDashCount];
			if (pDashPattern)
			{
				for (LONG lIndex = 0; lIndex < lDashCount; lIndex++)
				{
					pDashPattern[lIndex] = MM_2_PT(pDashPatternMM[lIndex]);
				}
			}
		}
	}
	else
	{
		// TODO: Реализовать другие типы пунктирных линий
	}

	if (pDashPattern && lDashCount)
	{
		m_pPage->SetDash(pDashPattern, lDashCount, MM_2_PT(m_oPen.GetDashOffset()));
		delete[] pDashPattern;
	}

	LONG lCapStyle = m_oPen.GetStartCapStyle();
	if (Aggplus::LineCapRound == lCapStyle)
		m_pPage->SetLineCap(linecap_Round);
	else if (Aggplus::LineCapSquare == lCapStyle)
		m_pPage->SetLineCap(linecap_ProjectingSquare);
	else
		m_pPage->SetLineCap(linecap_Butt);

	LONG lJoinStyle = m_oPen.GetJoinStyle();
	if (Aggplus::LineJoinBevel == lJoinStyle)
		m_pPage->SetLineJoin(linejoin_Bevel);
	else if (Aggplus::LineJoinMiter == lJoinStyle)
	{
		m_pPage->SetLineJoin(linejoin_Miter);
		m_pPage->SetMiterLimit(MM_2_PT(m_oPen.GetMiter()));
	}
	else
		m_pPage->SetLineJoin(linejoin_Round);
}
void CPdfRenderer::UpdateBrush()
{
	m_pShading = NULL;
	m_pShadingExtGrState = NULL;

	LONG lBrushType = m_oBrush.GetType();
	if (c_BrushTypeTexture == lBrushType)
	{
        std::wstring wsTexturePath = m_oBrush.GetTexturePath();
		CImageFileFormatChecker oImageFormat(wsTexturePath);

		CImageDict* pImage = NULL;
		int nImageW = 0;
		int nImageH = 0;
		if (_CXIMAGE_FORMAT_JPG == oImageFormat.eFileType || _CXIMAGE_FORMAT_JP2 == oImageFormat.eFileType)
		{
			pImage = m_pDocument->CreateImage();
			CBgraFrame oFrame;
			oFrame.OpenFile(wsTexturePath);
			nImageH = oFrame.get_Height();
			nImageW = oFrame.get_Width();

			if (pImage)
			{
				if (_CXIMAGE_FORMAT_JPG == oImageFormat.eFileType)
                    pImage->LoadJpeg(wsTexturePath.c_str(), nImageW, nImageH, oFrame.IsGrayScale());
				else
					pImage->LoadJpx(wsTexturePath.c_str(), nImageW, nImageH);
			}
		}
        else if (_CXIMAGE_FORMAT_WMF == oImageFormat.eFileType ||
                 _CXIMAGE_FORMAT_EMF == oImageFormat.eFileType ||
                 _CXIMAGE_FORMAT_SVM == oImageFormat.eFileType ||
                 _CXIMAGE_FORMAT_SVG == oImageFormat.eFileType)
		{
			// TODO: Реализовать отрисовку метафайлов по-нормальному
            MetaFile::IMetaFile* pMeta = MetaFile::Create(m_pAppFonts);
            pMeta->LoadFromFile(wsTexturePath.c_str());

			double dL, dR, dT, dB;
			m_oPath.GetBounds(dL, dT, dR, dB);

			double dNewW = std::max(10.0, dR - dL) / 72 * 300;

			std::wstring wsTempFile = GetTempFile();
            pMeta->ConvertToRaster(wsTempFile.c_str(), _CXIMAGE_FORMAT_PNG, dNewW);

            RELEASEOBJECT(pMeta);

			Aggplus::CImage oImage(wsTempFile);
			nImageW = abs((int)oImage.GetWidth());
			nImageH = abs((int)oImage.GetHeight());
			pImage = LoadImage(&oImage, 255);
		}
		else
		{
			Aggplus::CImage oImage(wsTexturePath);
			nImageW = abs((int)oImage.GetWidth());
			nImageH = abs((int)oImage.GetHeight());
			pImage = LoadImage(&oImage, 255);
		}

		if (pImage)
		{
			BYTE nAlpha = m_oBrush.GetTextureAlpha();
			if (0xFF != nAlpha)
				pImage->AddTransparency(nAlpha);

			LONG lTextureMode = m_oBrush.GetTextureMode();

			double dW = 10;
			double dH = 10;

			double dL, dR, dT, dB;
            CBrushState::TBrushRect& oRect = m_oBrush.GetBrushRect();
            if (!oRect.bUse)
            {
                m_oPath.GetBounds(dL, dT, dR, dB);
            }
            else
            {
                dL = MM_2_PT(oRect.dLeft);
                dB = MM_2_PT(m_dPageHeight - oRect.dTop);
                dR = MM_2_PT(oRect.dLeft + oRect.dWidth);
                dT = MM_2_PT(m_dPageHeight - oRect.dTop - oRect.dHeight);
            }

			double dXStepSpacing = 0, dYStepSpacing = 0;
			if (c_BrushTextureModeStretch == lTextureMode)
			{
				// Растягиваем картинку по размерам пата
				dW = std::max(10.0, dR - dL);
				dH = std::max(10.0, dB - dT);

				// Чтобы избавиться от погрешностей из-за которых могут возникать полоски или обрезание картинки,
				// удвоим расстрояние между соседними тайлами. Плохого тут нет, т.к. нам нужен всего 1 тайл
				dXStepSpacing = dW;
				dYStepSpacing = dH;
			}
			else
			{
				// Размеры картинки заданы в пикселях. Размеры тайла - это размеры картинки в пунктах.
				dW = nImageW * 72 / 96;
				dH = nImageH * 72 / 96;
			}

			// Нам нужно, чтобы левый нижний угол границ нашего пата являлся точкой переноса для матрицы преобразования.
			CMatrix* pMatrix = m_pPage->GetTransform();
			pMatrix->Apply(dL, dT);
			CMatrix oPatternMatrix = *pMatrix;
			oPatternMatrix.x = dL;
			oPatternMatrix.y = dT;
			m_pPage->SetPatternColorSpace(m_pDocument->CreateImageTilePattern(dW, dH, pImage, &oPatternMatrix, imagetilepatterntype_Default, dXStepSpacing, dYStepSpacing));
		}
	}
	else if (c_BrushTypeHatch1 == lBrushType)
	{
        std::wstring wsHatchType = m_oBrush.GetTexturePath();

		double dW = 8 * 72 / 96;
		double dH = 8 * 72 / 96;

        TColor oColor1 = m_oBrush.GetTColor1();
        TColor oColor2 = m_oBrush.GetTColor2();
		BYTE nAlpha1 = (BYTE)m_oBrush.GetAlpha1();
		BYTE nAlpha2 = (BYTE)m_oBrush.GetAlpha2();

		m_pPage->SetPatternColorSpace(m_pDocument->CreateHatchPattern(dW, dH, oColor1.r, oColor1.g, oColor1.b, nAlpha1, oColor2.r, oColor2.g, oColor2.b, nAlpha2, wsHatchType));
	}
	else if (c_BrushTypeRadialGradient == lBrushType || c_BrushTypeLinearGradient == lBrushType)
	{
		TColor* pGradientColors;
		double* pPoints;
		LONG lCount;

		m_oBrush.GetGradientColors(pGradientColors, pPoints, lCount);

		if (lCount > 0)
		{
			unsigned char* pColors = new unsigned char[3 * lCount];
			unsigned char* pAlphas = new unsigned char[lCount];
			if (pColors)
			{
				for (LONG lIndex = 0; lIndex < lCount; lIndex++)
				{
					pColors[3 * lIndex + 0] = pGradientColors[lIndex].r;
					pColors[3 * lIndex + 1] = pGradientColors[lIndex].g;
					pColors[3 * lIndex + 2] = pGradientColors[lIndex].b;
					pAlphas[lIndex]         = pGradientColors[lIndex].a;
				}

				if (c_BrushTypeLinearGradient == lBrushType)
				{
					double dX0, dY0, dX1, dY1;
					m_oBrush.GetLinearGradientPattern(dX0, dY0, dX1, dY1);
					m_pShading = m_pDocument->CreateAxialShading(m_pPage, MM_2_PT(dX0), MM_2_PT(m_dPageHeight - dY0), MM_2_PT(dX1), MM_2_PT(m_dPageHeight - dY1), pColors, pAlphas, pPoints, lCount, m_pShadingExtGrState);
				}
				else //if (c_BrushTypeRadialGradient == lBrushType)
				{
					double dX0, dY0, dR0, dX1, dY1, dR1;
					m_oBrush.GetRadialGradientPattern(dX0, dY0, dR0, dX1, dY1, dR1);
					m_pShading = m_pDocument->CreateRadialShading(m_pPage, MM_2_PT(dX0), MM_2_PT(m_dPageHeight - dY0), MM_2_PT(dR0), MM_2_PT(dX1), MM_2_PT(m_dPageHeight - dY1), MM_2_PT(dR1), pColors, pAlphas, pPoints, lCount, m_pShadingExtGrState);
				}
				delete[] pColors;
				delete[] pAlphas;
			}
		}
	}
	else// if (c_BrushTypeSolid == lBrushType)
	{
        TColor oColor1 = m_oBrush.GetTColor1();
		m_pPage->SetFillColor(oColor1.r, oColor1.g, oColor1.b);
		m_pPage->SetFillAlpha((unsigned char)m_oBrush.GetAlpha1());
	}
}
void CPdfRenderer::Reset()
{
	m_oPen.Reset();
	m_oBrush.Reset();
	m_oFont.Reset();
	m_oPath.Clear();

	// clear font!!!
	m_oFont.SetName(L"");
	m_oFont.SetSize(-1);
	m_oFont.SetStyle(1 << 5);

	m_lClipDepth = 0;
}
HRESULT CPdfRenderer::OnlineWordToPdf          (const std::wstring& wsSrcFile, const std::wstring& wsDstFile, CConvertFromBinParams* pParams)
{
	if (!NSOnlineOfficeBinToPdf::ConvertBinToPdf(this, wsSrcFile, wsDstFile, false, pParams))
		return S_FALSE;

	return S_OK;
}
HRESULT CPdfRenderer::OnlineWordToPdfFromBinary(const std::wstring& wsSrcFile, const std::wstring& wsDstFile, CConvertFromBinParams* pParams)
{
	if (!NSOnlineOfficeBinToPdf::ConvertBinToPdf(this, wsSrcFile, wsDstFile, true, pParams))
		return S_FALSE;

	return S_OK;
}

static inline void UpdateMaxMinPoints(double& dMinX, double& dMinY, double& dMaxX, double& dMaxY, const double& dX, const double& dY)
{
	if (dX < dMinX)
		dMinX = dX;

	if (dX > dMaxX)
		dMaxX = dX;

	if (dY < dMinY)
		dMinY = dY;

	if (dY > dMaxY)
		dMaxY = dY;
}
void CPdfRenderer::CPath::Draw(PdfWriter::CPage* pPage, bool bStroke, bool bFill, bool bEoFill)
{
	for (int nIndex = 0, nCount = m_vCommands.size(); nIndex < nCount; nIndex++)
	{
		CPathCommandBase* pCommand = m_vCommands.at(nIndex);
		pCommand->Draw(pPage);
	}

	if (bStroke && !bFill && !bEoFill)
		pPage->Stroke();
	else if (bStroke && bFill)
		pPage->FillStroke();
	else if (bStroke && bEoFill)
		pPage->EoFillStroke();
	else if (bFill)
		pPage->Fill();
	else if (bEoFill)
		pPage->EoFill();
	else
		pPage->EndPath();
}
void CPdfRenderer::CPath::Clip(PdfWriter::CPage* pPage, bool bEvenOdd)
{
	for (int nIndex = 0, nCount = m_vCommands.size(); nIndex < nCount; nIndex++)
	{
		CPathCommandBase* pCommand = m_vCommands.at(nIndex);
		pCommand->Draw(pPage);
	}

	if (bEvenOdd)
		pPage->Eoclip();
	else
		pPage->Clip();

	pPage->EndPath();
}
void CPdfRenderer::CPath::GetLastPoint(double& dX, double& dY)
{
	dX = 0;
	dY = 0;

	bool bFindMoveTo = false;
	for (int nIndex = m_vCommands.size() - 1; nIndex >= 0; nIndex--)
	{
		CPathCommandBase* pCommand = m_vCommands.at(nIndex);
		if (rendererpathcommand_Close == pCommand->GetType())
		{
			bFindMoveTo = true;
			continue;
		}
		else
		{
			pCommand->GetLastPoint(dX, dY);
			if (!bFindMoveTo || rendererpathcommand_MoveTo == pCommand->GetType())
				break;
		}
	}
}
void CPdfRenderer::CPath::GetBounds(double& dL, double& dT, double& dR, double& dB)
{
	GetLastPoint(dL, dT);
	dR = dL;
	dB = dT;

	for (int nIndex = 0, nCount = m_vCommands.size(); nIndex < nCount; nIndex++)
	{
		CPathCommandBase* pCommand = m_vCommands.at(nIndex);
		pCommand->UpdateBounds(dL, dT, dR, dB);
	}
}
void CPdfRenderer::CPath::CPathMoveTo::Draw(PdfWriter::CPage* pPage)
{
	pPage->MoveTo(x, y);
}
void CPdfRenderer::CPath::CPathMoveTo::UpdateBounds(double& dL, double& dT, double& dR, double& dB)
{
	UpdateMaxMinPoints(dL, dT, dR, dB, x, y);
}
void CPdfRenderer::CPath::CPathLineTo::Draw(PdfWriter::CPage* pPage)
{
	pPage->LineTo(x, y);
}
void CPdfRenderer::CPath::CPathLineTo::UpdateBounds(double& dL, double& dT, double& dR, double& dB)
{
	UpdateMaxMinPoints(dL, dT, dR, dB, x, y);
}
void CPdfRenderer::CPath::CPathCurveTo::Draw(PdfWriter::CPage* pPage)
{
	pPage->CurveTo(x1, y1, x2, y2, xe, ye);
}
void CPdfRenderer::CPath::CPathCurveTo::UpdateBounds(double& dL, double& dT, double& dR, double& dB)
{
	UpdateMaxMinPoints(dL, dT, dR, dB, x1, y1);
	UpdateMaxMinPoints(dL, dT, dR, dB, x2, y2);
	UpdateMaxMinPoints(dL, dT, dR, dB, xe, ye);
}
void CPdfRenderer::CPath::CPathArcTo::Draw(PdfWriter::CPage* pPage)
{
	if (sweepAngle >= 360 - 0.001)
		pPage->Ellipse(x + w / 2, y + h / 2, w / 2, h / 2);
	else
		pPage->EllipseArcTo(x + w / 2, y + h / 2, w / 2, h / 2, 360 - startAngle, 360 - (startAngle + sweepAngle), sweepAngle > 0 ? true : false);
}
void CPdfRenderer::CPath::CPathArcTo::UpdateBounds(double& dL, double& dT, double& dR, double& dB)
{
	UpdateMaxMinPoints(dL, dT, dR, dB, x, y);
	UpdateMaxMinPoints(dL, dT, dR, dB, x + w, y + h);
}
void CPdfRenderer::CPath::CPathClose::Draw(PdfWriter::CPage* pPage)
{
	pPage->ClosePath();
}
void CPdfRenderer::CPath::CPathClose::UpdateBounds(double& dL, double& dT, double& dR, double& dB)
{
}
void CPdfRenderer::CPath::CPathText::Draw(PdfWriter::CPage* pPage)
{
	// TODO: Если данная команда будет часто вызываться, тогда ее нужно будет оптимизировать, точно также как это делается в обычном тексте
	pPage->BeginText();
	pPage->SetFontAndSize(font, fontSize);
	pPage->SetCharSpace(charSpace);
	pPage->SetTextRenderingMode(textrenderingmode_Stroke);
	pPage->DrawText(x, y, codes, codesCount);
	pPage->EndText();
}
void CPdfRenderer::CPath::CPathText::UpdateBounds(double& dL, double& dT, double& dR, double& dB)
{
	UpdateMaxMinPoints(dL, dT, dR, dB, x, y);
}
void CPdfRenderer::CBrushState::Reset()
{
	m_lType = c_BrushTypeSolid;
	m_oColor1.Set(0);
	m_oColor2.Set(0);
	m_nAlpha1 = 255;
	m_nAlpha2 = 255;
	m_wsTexturePath = L"";
	m_lTextureMode  = c_BrushTextureModeStretch;
	m_nTextureAlpha = 255;
	m_dLinearAngle  = 0;
	m_oRect.Reset();

	if (m_pShadingColors)
		delete[] m_pShadingColors;

	if (m_pShadingPoints)
		delete[] m_pShadingPoints;

	m_pShadingColors      = NULL;
	m_pShadingPoints      = NULL;
	m_lShadingPointsCount = 0;
}
void CPdfRenderer::AddLink(PdfWriter::CPage* pPage, const double& dX, const double& dY, const double& dW, const double& dH, const double& dDestX, const double& dDestY, const unsigned int& unDestPage)
{
	CPage* pCurPage  = pPage;
	CPage* pDestPage = m_pDocument->GetPage(unDestPage);
	if (!pCurPage || !pDestPage)
		return;

	CDestination* pDestination = m_pDocument->CreateDestination(unDestPage);
	if (!pDestination)
		return;

	pDestination->SetXYZ(MM_2_PT(dDestX), pDestPage->GetHeight() - MM_2_PT(dDestY), 0);
	CAnnotation* pAnnot = m_pDocument->CreateLinkAnnot(pCurPage, TRect(MM_2_PT(dX), pCurPage->GetHeight() - MM_2_PT(dY), MM_2_PT(dX + dW), m_pPage->GetHeight() - MM_2_PT(dY + dH)), pDestination);
	pAnnot->SetBorderStyle(EBorderSubtype::border_subtype_Solid, 0);
}
bool CPdfRenderer::IsValid()
{
        return m_bValid;
}
bool CPdfRenderer::IsPageValid()
{
        if (!IsValid() || !m_pPage)
            return false;

        return true;
}
void CPdfRenderer::SetError()
{
        m_bValid = false;
}
unsigned char* CPdfRenderer::EncodeString(const unsigned int *pUnicodes, const unsigned int& unCount, const unsigned int *pGIDs)
{
	if (m_bNeedUpdateTextFont)
		UpdateFont();

	if (!m_pFont)
		return NULL;

	unsigned char* pCodes = new unsigned char[unCount * 2];
	if (!pCodes)
		return NULL;

	for (unsigned int unIndex = 0; unIndex < unCount; unIndex++)
	{
		unsigned short ushCode;
		if (pGIDs)
			ushCode = m_pFont->EncodeGID(pGIDs[unIndex], &pUnicodes[unIndex], 1);
		else
			ushCode = m_pFont->EncodeUnicode(pUnicodes[unIndex]);

		pCodes[2 * unIndex + 0] = (ushCode >> 8) & 0xFF;
		pCodes[2 * unIndex + 1] = ushCode & 0xFF;
	}

	return pCodes;
}
unsigned char* CPdfRenderer::EncodeGID(const unsigned int& unGID, const unsigned int* pUnicodes, const unsigned int& unUnicodesCount)
{
	if (m_bNeedUpdateTextFont)
		UpdateFont();

	if (!m_pFont)
		return NULL;

	unsigned char* pCodes = new unsigned char[2];
	if (!pCodes)
		return NULL;

	unsigned short ushCode = m_pFont->EncodeGID(unGID, pUnicodes, unUnicodesCount);
	pCodes[0] = (ushCode >> 8) & 0xFF;
	pCodes[1] = ushCode & 0xFF;
	return pCodes;
}
std::wstring CPdfRenderer::GetDownloadFile(const std::wstring& sUrl)
{
    std::wstring::size_type n1 = sUrl.find(L"www.");
    std::wstring::size_type n2 = sUrl.find(L"http://");
    std::wstring::size_type n3 = sUrl.find(L"ftp://");
    std::wstring::size_type n4 = sUrl.find(L"https://");
    std::wstring::size_type nMax = 3;

    bool bIsNeedDownload = false;
    if (n1 != std::wstring::npos && n1 < nMax)
        bIsNeedDownload = true;
    else if (n2 != std::wstring::npos && n2 < nMax)
        bIsNeedDownload = true;
    else if (n3 != std::wstring::npos && n3 < nMax)
        bIsNeedDownload = true;
    else if (n4 != std::wstring::npos && n4 < nMax)
        bIsNeedDownload = true;

    if (!bIsNeedDownload)
        return L"";

    std::wstring sTempFile = GetTempFile();
    NSNetwork::NSFileTransport::CFileDownloader oDownloader(sUrl, false);
    oDownloader.SetFilePath(sTempFile);

    if (oDownloader.DownloadSync())
        return sTempFile;

    if (NSFile::CFileBinary::Exists(sTempFile))
        NSFile::CFileBinary::Remove(sTempFile);

    return L"";
}
