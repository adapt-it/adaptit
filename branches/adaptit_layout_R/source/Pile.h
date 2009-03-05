/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Pile.h
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CPile class. 
/// The CPile class represents the next smaller divisions of a CStrip. The CPile
/// instances are laid out sequentially within a CStrip. Each CPile stores a 
/// stack or "pile" of CCells stacked vertically in the pile. Within a CPile
/// the top CCell, if used, displays the source word or phrase with punctuation,
/// the second CCell displays the same minus the punctuation. The third CCell
/// displays the translation, minus any pnctuation. The fourth CCell displays the
/// translation with punctuation as copied (if the user typed none) or as typed
/// by the user. The fifth CCell displays the Gloss when used.
/// \derivation		The CPile class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef Pile_h
#define Pile_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Pile.h"
#endif

// forward references:
class CAdapt_ItApp;
//class CAdapt_ItDoc;
//class CSourceBundle;
class CStrip;
class CSourcePhrase;
class CCell;
class CLayout;
class CPile;

WX_DECLARE_LIST(CPile, PileList); // see list definition macro in .cpp file

/// The CPile class represents the next smaller divisions of a CStrip. The CPile
/// instances are laid out sequentially within a CStrip. Each CPile stores a 
/// stack or "pile" of CCells stacked vertically in the pile. Within a CPile
/// the top CCell, if used, displays the source word or phrase with punctuation,
/// the second CCell displays the same minus the punctuation. The third CCell
/// displays the translation, minus any pnctuation. The fourth CCell displays the
/// translation with punctuation as copied (if the user typed none) or as typed
/// by the user. The fifth CCell displays the Gloss when used.
/// \derivation		The CPile class is derived from wxObject.
class CPile : public wxObject  
{
	friend class CCell;
	friend class CLayout;
public:
	// constructors
	CPile();
	//CPile(CAdapt_ItDoc* pDocument, CSourceBundle* pSourceBundle, CStrip* pStrip,
	//				CSourcePhrase* pSrcPhrase); // BEW deprecated 3Feb09
	//CPile(CSourceBundle* pSourceBundle, CStrip* pStrip, CSourcePhrase* pSrcPhrase);
	//CPile(CLayout* pLayout, CSourcePhrase* pSrcPhrase); <<< don't need this one

	// operations
public:
	CSourcePhrase*	GetSrcPhrase();
	void	SetSrcPhrase(CSourcePhrase* pSrcPhrase);

	// attributes

private:
	CLayout*		m_pLayout;
	CSourcePhrase*	m_pSrcPhrase;
	CStrip*			m_pOwningStrip;
	CCell*			m_pCell[MAX_CELLS]; // 1 source line, 1 target line, & one gloss line per strip
	int				m_nPile; // what my index is in the strip object
	int				m_nWidth;
	int				m_nMinWidth;
	bool			m_bIsCurrentFreeTransSection; // BEW added 24Jun05 for support of free translations
	/*
	bool			m_bIsActivePile;
	int				m_nPileIndex;
	wxRect			m_rectPile;
	int				m_nHorzOffset;
	//CAdapt_ItDoc*	m_pDoc; // BEW deprecated 3Feb09
	CSourceBundle*	m_pBundle;
	CStrip*			m_pStrip;
	CCell*			m_pCell[5]; // 2 source lines, 2 target lines, & one gloss per strip
	wxColour		m_navTextColor;
	*/

	// implementation
	void			DestroyCells();

	// destructor
public:
	virtual ~CPile();

	virtual void Draw(wxDC* pDC); // *** TODO **** needs new param list and internals
	void	DrawNavTextInfoAndIcons(wxDC* pDC); // handles wedge, note icon, retranslation, nav text etc
	void	PrintPhraseBox(wxDC* pDC);

	//CPile* CreatePile(wxClientDC* pDC, CAdapt_ItApp* pApp, CSourceBundle* pBundle, 
	//				CStrip* pStrip, CSourcePhrase* pSrcPhrase, wxRect* pRectPile);
	//				
	int			CalcPileWidth();
	int			GetStripIndex();
	CStrip*		GetStrip();

	int		Width();
	int		Height();
	int		Left();
	int		Top();
	void	GetPileRect(wxRect& rect);
	void	TopLeft(wxPoint& ptTopLeft);


private:
	bool HasFilterMarker(); // returns TRUE if the pointed at CSourcePhrase has \~FILTER in m_markers

	DECLARE_DYNAMIC_CLASS(CPile) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CClassName)
	
};

#endif // Pile_h
