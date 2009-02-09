/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Cell.h
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CCell class. 
/// The CCell class represents the next smaller division of a CPile, there
/// potentially being up to five CCells displaying vertically top to bottom
/// within a CPile. Each CCell stores a CText which is for the display of 
/// the cell's text, and changing background colour (for selections, 
/// highlighting) etc.
/// \derivation		The CCell class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef Cell_h
#define Cell_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Cell.h"
#endif

// forward references
class CAdapt_ItDoc;
class CSourceBundle;
class CStrip;
class CPile;
class CText;
class CFont;

/// The CCell class represents the next smaller division of a CPile, there
/// potentially being up to five CCells displaying vertically top to bottom
/// within a CPile. Each CCell stores a CText which is for the display of 
/// the cell's text, and changing background colour (for selections, 
/// highlighting) etc.
/// \derivation		The CCell class is derived from wxObject.
class CCell : public wxObject  
{

public:
	// constructors
	CCell();
//	CCell(CAdapt_ItDoc* pDocument, CSourceBundle* pSourceBundle, 
//												CStrip* pStrip, CPile* pPile); //BEW deprecated 3Feb09
//	CCell(CSourceBundle* pSourceBundle, CStrip* pStrip, CPile* pPile);


	// attributes
public:
	//CText*				m_pText; // note, cell can exist with this ptr NULL; BEW removed 6Feb09
	wxColour				m_color;
	wxColour				m_navColor;
	wxFont*					m_pFont;
	wxPoint					m_ptBotRight;
	wxPoint					m_ptTopLeft;
	int						m_nTextExtent; // not set if m_bDisplay == FALSE
	int						m_nCellIndex; // which one I am of the five
	//bool					m_bDisplay; // TRUE if the cell is to be displayed, BEW deprecated 3Feb09
	//CAdapt_ItDoc*			m_pDoc; // BEW deprecated 3Feb09
	CSourceBundle*			m_pBundle;
	CStrip*					m_pStrip;
	CPile*					m_pPile;
	wxString*				m_pPhrase; // BEW changed 9Feb09 to point at the text, not copy it
	bool					m_bSelected;	///< TRUE if text is within a selection, FALSE otherwise

	// destructor
	virtual ~CCell();
	virtual void Draw(wxDC* pDC);
	// helpers
	//void  DrawCell(wxDC* pDC, wxPoint& start, wxPoint& end, wxFont* pFont,
	//		const wxString& phrase, const wxColour& color, int nCell);
	void	DrawCell(wxDC* pDC);
	void	CreateCell(CSourceBundle* pBundle,CStrip* pStrip, CPile* pPile, wxString* pPhrase,
						int xExtent, wxFont* pFont, wxColour* pColor, wxPoint* pTopLeft, 
						wxPoint* pBotRight, int index, wxColor* pNavTextColor);
	// BEW 9Feb09: DrawTextRTL() is a duplicate of the function of the same name defined
	// also in the CAdapt_ItView class. The one in the view class is used only for RTL drawing
	// in the free translation rectangles at strip bottoms, in free translation mode. Unfortunately
	// when that happens, no CCell is in scope, and so we cannot conveniently remove the duplicate
	// function from CAdapt_ItView at this time. However, the one here in CCell needs to be here
	// to round out the CCell function inventory fully. (If later we move the free translation
	// drawing to the refactored layout code, we can put a copy of DrawTextRTLI() on the CLayout
	// object, for drawing RTL text in free translation mode.)
	void	DrawTextRTL(wxDC* pDC, wxString& str, wxRect& rect);

	DECLARE_DYNAMIC_CLASS(CCell) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CRefString)
};

#endif
