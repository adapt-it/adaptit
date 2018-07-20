/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Pile.h
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
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

const int PHRASE_BOX_WIDTH_UNSET = -1;

//GDLC Added 2010-02-10
enum phraseBoxWidthAdjustMode {
	contracting,
	steadyAsSheGoes,
	expanding
};

/// The CPile class represents the next smaller divisions of a CStrip. The CPile instances
/// are laid out sequentially within a CStrip. Each CPile stores a stack or "pile" of
/// CCells stacked vertically in the pile. Within a CPile the top CCell displays the source
/// word or phrase with punctuation, the second CCell displays the translation with
/// punctuation as copied (if the user typed none) or as typed by the user. The third CCell
/// displays the Gloss, when used. In glossing mode, second displays gloss and third
/// displays target text.
/// \derivation		The CPile class is derived from wxObject.
class CPile : public wxObject  
{
	friend class CStrip;
	friend class CCell;
	friend class CLayout;
public:
	// constructors
	CPile();
	CPile(CLayout* pLayout);
	CPile(const CPile& pile); // copy constructor

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
	int				m_nWidth; // stores width as calculated from PhraseBox width, so is valid only at active
							  // location; at other piles it stores -1
	int				m_nMinWidth; // this stores the actual pile width based on the extent of the text
	bool			m_bIsCurrentFreeTransSection; // BEW added 24Jun05 for support of free translations

	// destructor
public:
	virtual		~CPile();

	virtual		void Draw(wxDC* pDC);
	void		DrawNavTextInfoAndIcons(wxDC* pDC); // handles wedge, note icon, retranslation, nav text etc
	void		PrintPhraseBox(wxDC* pDC);
	bool		IsWrapPile();

	int			CalcPileWidth(); // based on the text in the cells only, no account taken of active loc
//GDLC 2010-02-10 Added parameter to CalcPhraseBoxGapWidth with default value steadyAsSheGoes
	int			CalcPhraseBoxGapWidth(enum phraseBoxWidthAdjustMode widthMode = steadyAsSheGoes);
	// BEW 19Jul18, for Bill's dropdown+textctrl+button solution for the phrasebox, we need a separate 
	// function than CalcPhraseBoxGapWidth() - our new function, which calculates the width of the
	// phrasebox in a similar manner to CalcPhraseBoxGapWidth() but with some essential differences...
	// Our new function is CalcPhraseBoxWidth() with the same signature, but
	// (a) the basic length is calculated solely from the width extent of the text in the wxTextControl
	// at the active location (whereas for CalcPhraseBoxGapWidth() it bets the basic length as the max
	// of the text lengths for src, gloss, and wxTextCtrl contents at active location), the button width
	// is the same in both functions, but
	// (b) the slop value is computed, for the CalcPhraseBoxGapWidth() function, in a multiple of 'w'
	// character widths, whereas CalcPhraseBoxWidth() uses the same multiple of 'f' character widths
	// for the phrasebox slop. 'f' is less wide the 'w', so this guarantees that the phrasebox will
	// always fit into the gap left for it. The view's OnDraw() will use this new function, as will
	// ResizeBox() and FixBox() I expect (I've not yet finished the refactoring)
	int			CalcPhraseBoxWidth(enum phraseBoxWidthAdjustMode widthMode = steadyAsSheGoes);

	int			GetStripIndex();
	CStrip*		GetStrip();
	void		SetStrip(CStrip* pStrip);
	CCell*		GetCell(int nCellIndex);
	CCell**		GetCellArray();
	int			GetPileIndex();
	//int		GetWidth(); // BEW added 14Mar11 & removed on 20Nov12, it's dangerous because the
							// value is not -1 only when the pile is the active pile, so
							// it's easy to use it where it shouldn't be used, use Width() instead

	int			Width();
	int			Height();
	int			Left();
	int			Top();
	void		GetPileRect(wxRect& rect);
	wxRect		GetPileRect(); // overloaded version
	void		TopLeft(wxPoint& ptTopLeft);

	void		SetMinWidth(); // sets m_nMinWidth (width large enough for cells, calls CalcPileWidth())
//GDLC 2010-02-10 Added parameter to SetPhraseBoxGapWidth with default value steadyAsSheGoes
	void		SetPhraseBoxGapWidth(enum phraseBoxWidthAdjustMode widthMode = steadyAsSheGoes); 
											// sets m_nWidth (the width to be used at active 
											//location, calls CalcPhraseBoxGapWidth())
	void		SetPhraseBoxGapWidth(int nNewWidth);  // this overload sets m_nWidth to the passed in value
	int			GetMinWidth(); // returns value of m_nMinWidth
	int			GetPhraseBoxGapWidth(); // returns value of m_nWidth
	void		SetIsCurrentFreeTransSection(bool bIsCurrentFreeTransSection);
	bool		GetIsCurrentFreeTransSection();
	int			GetPhraseBoxWidth(); //BEW added 19Jul18, gets Layout's m_currBoxWidth value
	void		SetPhraseBoxWidth(enum phraseBoxWidthAdjustMode widthMode = steadyAsSheGoes); // BEW added 19Jul18
	void		SetPhraseBoxWidth(int boxwidth); // an override, to set an explicit known width

	// BEW added 17July18 so as to allow box + slop to be a different (lesser) value than the gap width
	//int			m_nBoxOnlyWidth; // use this for box width, no longer use the gap width
private:
	bool		HasFilterMarker(); // returns TRUE if the pointed at CSourcePhrase has \~FILTER in m_markers

	DECLARE_DYNAMIC_CLASS(CPile) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CClassName)
	
};

#endif // Pile_h
