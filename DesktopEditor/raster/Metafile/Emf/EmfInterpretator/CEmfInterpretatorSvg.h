#ifndef CEMFINTERPRETATORSVG_H
#define CEMFINTERPRETATORSVG_H

#include "../../Common/MetaFileUtils.h"
#include "CEmfInterpretatorBase.h"
#include "../EmfParser/CEmfParserBase.h"
#include "../../../../xml/include/xmlwriter.h"

namespace MetaFile
{
	class CEmfInterpretatorSvg : public CEmfInterpretatorBase
	{
	public:
		CEmfInterpretatorSvg(CEmfParserBase* pParser = NULL, unsigned int unWidth = 0, unsigned int unHeight = 0);
		CEmfInterpretatorSvg(const CEmfInterpretatorSvg& oInterpretator);
		virtual ~CEmfInterpretatorSvg();

		void CreateConditional(IMetaFileBase*) override {} ;
		void ChangeConditional() override {};

		InterpretatorType   GetType() const override;

		void SetSize(unsigned int unWidth, unsigned int unHeight);

		void HANDLE_EMR_HEADER(const TEmfHeader& oTEmfHeader) override ;
		void HANDLE_EMR_ALPHABLEND(const TEmfAlphaBlend& oTEmfAlphaBlend, CDataStream &oDataStream) override ;
		void HANDLE_EMR_STRETCHDIBITS(const TEmfStretchDIBITS& oTEmfStretchDIBITS, CDataStream &oDataStream) override ;
		void HANDLE_EMR_BITBLT(const TEmfBitBlt& oTEmfBitBlt, CDataStream &oDataStream) override ;
		void HANDLE_EMR_SETDIBITSTODEVICE(const TEmfSetDiBitsToDevice& oTEmfSetDiBitsToDevice, CDataStream &oDataStream) override ;
		void HANDLE_EMR_STRETCHBLT(const TEmfStretchBLT& oTEmfStretchBLT, CDataStream &oDataStream) override ;
		void HANDLE_EMR_EOF() override ;
		void HANDLE_EMR_SAVEDC() override ;
		void HANDLE_EMR_RESTOREDC(const int &nIndexDC) override ;
		void HANDLE_EMR_MODIFYWORLDTRANSFORM(const TEmfXForm& oXForm, const unsigned int& unMode) override ;
		void HANDLE_EMR_SETWORLDTRANSFORM(const TEmfXForm& oXForm) override ;
		void HANDLE_EMR_CREATEBRUSHINDIRECT(const unsigned int& unBrushIndex, const CEmfLogBrushEx* pBrush) override ;
		void HANDLE_EMR_SETTEXTCOLOR(const TEmfColor& oColor) override ;
		void HANDLE_EMR_SELECTOBJECT(const unsigned int& unObjectIndex) override ;
		void HANDLE_EMR_EXTCREATEFONTINDIRECTW(const unsigned int& unIndex, CEmfLogFont* oLogFont) override ;
		void HANDLE_EMR_SETTEXTALIGN(const unsigned int &unAlign) override ;
		void HANDLE_EMR_SETBKMODE(const unsigned int &unBgMode) override ;
		void HANDLE_EMR_DELETEOBJECT(const unsigned int &unObjectIndex) override ;
		void HANDLE_EMR_SETMITERLIMIT(const unsigned int &unMeterLimit) override ;
		void HANDLE_EMR_EXTCREATEPEN(const unsigned int& unPenIndex, CEmfLogPen* pPen, const std::vector<unsigned int>& arUnused) override ;
		void HANDLE_EMR_CREATEPEN(const unsigned int& unPenIndex, const unsigned int& unWidthX, const CEmfLogPen* pPen) override ;
		void HANDLE_EMR_SETPOLYFILLMODE(const unsigned int& unFillMode) override ;
		void HANDLE_EMR_BEGINPATH() override ;
		void HANDLE_EMR_ENDPATH() override ;
		void HANDLE_EMR_CLOSEFIGURE() override ;
		void HANDLE_EMR_FLATTENPATH() override ;
		void HANDLE_EMR_WIDENPATH() override ;
		void HANDLE_EMR_ABORTPATH() override ;
		void HANDLE_EMR_MOVETOEX(const TEmfPointL& oPoint) override ;
		void HANDLE_EMR_SETARCDIRECTION(const unsigned int& unDirection) override ;
		void HANDLE_EMR_FILLPATH(const TEmfRectL& oBounds) override ;
		void HANDLE_EMR_SETMAPMODE(const unsigned int& unMapMode) override ;
		void HANDLE_EMR_SETWINDOWORGEX(const TEmfPointL& oOrigin) override ;
		void HANDLE_EMR_SETWINDOWEXTEX(const TEmfSizeL& oExtent) override ;
		void HANDLE_EMR_SETVIEWPORTORGEX(const TEmfPointL& oOrigin) override ;
		void HANDLE_EMR_SETVIEWPORTEXTEX(const TEmfSizeL& oExtent) override ;
		void HANDLE_EMR_SETSTRETCHBLTMODE(const unsigned int& unStretchMode) override ;
		void HANDLE_EMR_SETICMMODE(const unsigned int& unICMMode) override ;
		void HANDLE_EMR_CREATEMONOBRUSH(const unsigned int& unBrushIndex, const TEmfDibPatternBrush& oDibBrush, CDataStream &oDataStream) override ;
		void HANDLE_EMR_CREATEDIBPATTERNBRUSHPT(const unsigned int& unBrushIndex, const TEmfDibPatternBrush& oDibBrush, CDataStream &oDataStream) override ;
		void HANDLE_EMR_SELECTCLIPPATH(const unsigned int& unRegionMode) override ;
		void HANDLE_EMR_SETBKCOLOR(const TEmfColor& oColor) override ;
		void HANDLE_EMR_EXCLUDECLIPRECT(const TEmfRectL& oClip) override ;
		void HANDLE_EMR_EXTSELECTCLIPRGN(const unsigned int& unRgnDataSize, const unsigned int& unRegionMode, CDataStream &oDataStream) override ;
		void HANDLE_EMR_SETMETARGN() override ;
		void HANDLE_EMR_SETROP2(const unsigned int& unRop2Mode) override ;
		void HANDLE_EMR_CREATEPALETTE(const unsigned int& unPaletteIndex, const CEmfLogPalette* oEmfLogPalette) override ;
		void HANDLE_EMR_SELECTPALETTE(const unsigned int& unPaletteIndex) override ;
		void HANDLE_EMR_REALIZEPALETTE() override ;
		void HANDLE_EMR_INTERSECTCLIPRECT(const TEmfRectL& oClip) override ;
		void HANDLE_EMR_SETLAYOUT(const unsigned int& unLayoutMode) override ;
		void HANDLE_EMR_SETBRUSHORGEX(const TEmfPointL& oOrigin) override ;
		void HANDLE_EMR_ANGLEARC(const TEmfPointL& oCenter, const unsigned int& unRadius, const double& dStartAngle, const double& dSweepAngle) override ;
		void HANDLE_EMR_ARC(const TEmfRectL& oBox, const TEmfPointL& oStart, const TEmfPointL& oEnd) override ;
		void HANDLE_EMR_ARCTO(const TEmfRectL& oBox, const TEmfPointL& oStart, const TEmfPointL& oEnd) override ;
		void HANDLE_EMR_CHORD(const TEmfRectL& oBox, const TEmfPointL& oStart, const TEmfPointL& oEnd) override ;
		void HANDLE_EMR_ELLIPSE(const TEmfRectL& oBox) override ;
		void HANDLE_EMR_EXTTEXTOUTA(const TEmfExtTextoutA& oTEmfExtTextoutA) override ;
		void HANDLE_EMR_EXTTEXTOUTW(const TEmfExtTextoutW& oTEmfExtTextoutW) override ;
		void HANDLE_EMR_LINETO(const TEmfPointL& oPoint) override ;
		void HANDLE_EMR_PIE(const TEmfRectL& oBox, const TEmfPointL& oStart, const TEmfPointL& oEnd) override ;
		void HANDLE_EMR_POLYBEZIER(const TEmfRectL& oBounds, const std::vector<TEmfPointL>& arPoints) override ;
		void HANDLE_EMR_POLYBEZIER(const TEmfRectL& oBounds, const std::vector<TEmfPointS>& arPoints) override ;
		void HANDLE_EMR_POLYBEZIERTO(const TEmfRectL& oBounds, const std::vector<TEmfPointL>& arPoints) override ;
		void HANDLE_EMR_POLYBEZIERTO(const TEmfRectL& oBounds, const std::vector<TEmfPointS>& arPoints) override ;
		void HANDLE_EMR_POLYDRAW(const TEmfRectL &oBounds, TEmfPointL *arPoints, const unsigned int &unCount, const unsigned char *pAbTypes) override ;
		void HANDLE_EMR_POLYDRAW(const TEmfRectL &oBounds, TEmfPointS *arPoints, const unsigned int &unCount, const unsigned char *pAbTypes) override ;
		void HANDLE_EMR_POLYGON(const TEmfRectL& oBounds, const std::vector<TEmfPointL>& arPoints) override ;
		void HANDLE_EMR_POLYGON(const TEmfRectL& oBounds, const std::vector<TEmfPointS>& arPoints) override ;
		void HANDLE_EMR_POLYLINE(const TEmfRectL& oBounds, const std::vector<TEmfPointL>& arPoints) override ;
		void HANDLE_EMR_POLYLINE(const TEmfRectL& oBounds, const std::vector<TEmfPointS>& arPoints) override ;
		void HANDLE_EMR_POLYLINETO(const TEmfRectL& oBounds, const std::vector<TEmfPointL>& arPoints) override ;
		void HANDLE_EMR_POLYLINETO(const TEmfRectL& oBounds, const std::vector<TEmfPointS>& arPoints) override ;
		void HANDLE_EMR_POLYPOLYGON(const TEmfRectL& oBounds, const std::vector<std::vector<TEmfPointL>>& arPoints) override;
		void HANDLE_EMR_POLYPOLYGON(const TEmfRectL& oBounds, const std::vector<std::vector<TEmfPointS>>& arPoints) override;
		void HANDLE_EMR_POLYPOLYLINE(const TEmfRectL& oBounds, const std::vector<std::vector<TEmfPointL>>& arPoints) override;
		void HANDLE_EMR_POLYPOLYLINE(const TEmfRectL& oBounds, const std::vector<std::vector<TEmfPointS>>& arPoints) override;
		//TODO: Реализовать сохранение полигонов в полигоне
		void HANDLE_EMR_RECTANGLE(const TEmfRectL& oBox) override ;
		void HANDLE_EMR_ROUNDRECT(const TEmfRectL& oBox, const TEmfSizeL& oCorner) override ;
		void HANDLE_EMR_SETPIXELV(const TEmfPointL& oPoint, const TEmfColor& oColor) override ;
		void HANDLE_EMR_SMALLTEXTOUT(const TEmfSmallTextout& oText) override ;
		void HANDLE_EMR_STROKEANDFILLPATH(const TEmfRectL& oBounds) override ;
		void HANDLE_EMR_STROKEPATH(const TEmfRectL& oBounds) override ;

		void HANDLE_EMR_UNKNOWN(CDataStream &oDataStream) override;
		void HANDLE_EMR_FILLRGN(const TEmfRectL& oBounds, unsigned int unIhBrush, const TRegionDataHeader& oRegionDataHeader, const std::vector<TEmfRectL>& arRects) override;

		void HANDLE_EMFPLUS_HEADER(bool bIsEmfPlusDual, bool bIsReferenceDevice, unsigned int unDpiX, unsigned int unDpiY) override {};
		void HANDLE_EMFPLUS_CLEAR(TEmfPlusARGB oColor) override {};
		void HANDLE_EMFPLUS_DRAWARC(char chPenId, double dStartAngle, double dSweepAngle, TEmfPlusRect oRect) override {};
		void HANDLE_EMFPLUS_DRAWARC(char chPenId, double dStartAngle, double dSweepAngle, TEmfPlusRectF oRect) override {};
		void HANDLE_EMFPLUS_DRAWBEZIERS(char chPenId, std::vector<TEmfPlusPointR> arPoints) override {};
		void HANDLE_EMFPLUS_DRAWBEZIERS(char chPenId, std::vector<TEmfPlusPointF> arPoints) override {};
		void HANDLE_EMFPLUS_DRAWBEZIERS(char chPenId, std::vector<TEmfPlusPoint> arPoints) override {};
		void HANDLE_EMFPLUS_DRAWCLOSEDCURVE(char chPenId, double dTension, std::vector<TEmfPlusPointR> arPoints) override {};
		void HANDLE_EMFPLUS_DRAWCLOSEDCURVE(char chPenId, double dTension, std::vector<TEmfPlusPointF> arPoints) override {};
		void HANDLE_EMFPLUS_DRAWCLOSEDCURVE(char chPenId, double dTension, std::vector<TEmfPlusPoint> arPoints) override {};
		void HANDLE_EMFPLUS_DRAWCURVE(char chPenId, double dTension, unsigned int unOffset, unsigned int unNumSegments, std::vector<TEmfPlusPoint> arPoints) override {};
		void HANDLE_EMFPLUS_DRAWCURVE(char chPenId, double dTension, unsigned int unOffset, unsigned int unNumSegments, std::vector<TEmfPlusPointF> arPoints) override {};
		void HANDLE_EMFPLUS_DRAWELLIPSE(char chPenId, TEmfPlusRect oRect) override {};
		void HANDLE_EMFPLUS_DRAWELLIPSE(char chPenId, TEmfPlusRectF oRect) override {};
		void HANDLE_EMFPLUS_DRAWDRIVERSTRING(char chFontId, unsigned int unBrushId, unsigned int unDriverStringOptionsFlags, unsigned int unMatrixPresent, const std::wstring& wsString, const std::vector<TEmfPlusPointF>& arGlyphPos) override {};
		void HANDLE_EMFPLUS_DRAWIMAGE(char chEmfPlusImageId, unsigned int unImageAttributesId, int nSrcUnit, const TEmfPlusRectF& oSrcRect, const TEmfPlusRect& oRectData) override {};
		void HANDLE_EMFPLUS_DRAWIMAGE(char chEmfPlusImageId, unsigned int unImageAttributesId, int nSrcUnit, const TEmfPlusRectF& oSrcRect, const TEmfPlusRectF& oRectData) override {};

	private:
		XmlUtils::CXmlWriter    m_oXmlWriter;
		std::wstring            m_sOutputData; // по идее это не нужно.

		CEmfParserBase          *m_pParser;

		TSvgViewport            m_oViewport;
		TEmfSizeL               m_oSizeWindow;

	public:
		void Begin() override;
		void End() override;
		//Следующие методы ничего не делают

		void DrawBitmap(double dX, double dY, double dW, double dH, BYTE* pBuffer, unsigned int unWidth, unsigned int unHeight) override;

		void DrawString(std::wstring& wsText, unsigned int unCharsCount, double dX, double dY, double* pDx,
						int iGraphicsMode = 1, double dXScale = 1, double dYScale = 1) override {};

		void DrawDriverString(const std::wstring& wsString, const std::vector<TPointD>& arPoints) override {};

		void StartPath() override {};
		void MoveTo(double dX, double dY) override {};
		void LineTo(double dX, double dY) override {};
		void CurveTo(double dX1, double dY1, double dX2, double dY2, double dXe, double dYe) override {};
		void ArcTo(double dLeft, double dTop, double dRight, double dBottom, double dStartAngle, double dSweepAngle) override {};
		void ClosePath() override {};
		void DrawPath(int nType = 0) override {};
		void EndPath() override {};

		void ResetClip() override {};
		void IntersectClip(double dLeft, double dTop, double dRight, double dBottom) override {};
		void StartClipPath(unsigned int unMode, int nFillMode = -1) override {};
		void EndClipPath(unsigned int unMode) override {};

		void UpdateDC() override {};
		void SetTransform(double& dM11, double& dM12, double& dM21, double& dM22, double& dX, double& dY) override {};
		void GetTransform(double* pdM11, double* pdM12, double* pdM21, double* pdM22, double* pdX, double* pdY) override {};

		std::wstring GetFile() { return m_sOutputData; }
	private:
		void WriteNode(const std::wstring& wsNodeName, const NodeAttributes& arAttributes, const std::wstring& wsValueNode = L"");
		void WriteText(const std::wstring& wsText, double dX, double dY, const TEmfRectL& oBounds);

		void AddStroke(NodeAttributes &arAttributes);
		void AddFill(NodeAttributes &arAttributes);
		void AddTransform(NodeAttributes &arAttributes);

		void AddNoneFill(NodeAttributes &arAttributes);

		void UpdateTransform(double dX, double dY);
		void UpdateTransform(const TRectD& oRect);
		void UpdateTransform(const std::vector<TEmfPointL>& arPoints, const NodeAttributes& arAttributes = {});
		void UpdateTransform(const std::vector<TEmfPointS>& arPoints, const NodeAttributes& arAttributes = {});
		void UpdateTransform(TEmfPointL *arPoints, unsigned int unCount);
		void UpdateTransform(TEmfPointS *arPoints, unsigned int unCount);

		double TranslateX(double nX);
		double TranslateY(double nY);

		TPointD TranslatePoint(const TPointD& oPoint);
		TRectD TranslateRect(const TEmfRectL& oRect);

		TPointD GetCutPos();
	};
}


#endif // CEMFINTERPRETATORSVG_H
