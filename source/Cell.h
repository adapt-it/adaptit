/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Cell.h
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
/// Public License (see license directory)
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
class CFont;

/// The CCell class represents the next smaller division of a CPile, there
/// potentially being up to five CCells displaying vertically top to bottom
/// within a CPile. Each CCell stores a CText which is for the display of 
/// the cell's text, and changing background colour (for selections, 
/// highlighting) etc.
/// \derivation		The CCell class is derived from wxObject.
class CCell : public wxObject  
{
	friend class CLayout;
public:
	// constructors
	CCell();
	CCell(const CCell& cell); // copy constructor



	// attributes
private: 
	bool		m_bSelected;	//< TRUE if text is within a selection, FALSE otherwise
	int			m_nCell;	// index to this particular cell in the pile's array
	wxString*	m_pPhrase;	// point to m_gloss or m_targetStr depending on gbIsGlossing
	CLayout*	m_pLayout;
	CPile*		m_pOwningPile;
	// BEW 9Apr12 added m_bAutoInserted - default FALSE, but set TRUE for an adaptation or
	// gloss CCell instance (depending on whether gbIsGlossing is FALSE or TRUE,
	// respectively) that has just received a CKB adaptation or gloss due to a successful
	// lookup and insertion
	bool		m_bAutoInserted;
public:
	// destructor
	virtual		~CCell();
	virtual void Draw(wxDC* pDC);
	void		DrawCell(wxDC* pDC, wxColor color); // pass in the colour too, as it
					// can be dynamically changed - e.g. to gray in vertical edit mode
	void		CreateCell(CLayout* pLayout, CPile* pOwnerPile, int index);
	wxString*	GetCellText(); // get a pointer to the correct wxString for drawing
	wxFont*		GetFont(); // get the font to be used for drawing this cell's text
	wxColour	GetColor(); // get the colour to be used for drawing this cell's text
	int			Width();
	int			Height();
	int			Left();
	int			Top();
	void		GetCellRect(wxRect& rect);
	void		TopLeft(wxPoint& ptTopLeft);
	void		BottomRight(wxPoint& ptBottomRight);
	wxPoint		GetTopLeft();
	CPile*		GetPile();
	bool		IsSelected();
	void		SetSelected(bool bValue); // set to TRUE or FALSE, explicitly (no default)
	int			GetCellIndex();

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
