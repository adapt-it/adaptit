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
	CSourcePhrase* GetSrcPhrase();
	void	SetSrcPhrase(CSourcePhrase* pSrcPhrase);

	// attributes

private:
	CLayout* m_pLayout;
	CSourcePhrase* m_pSrcPhrase;
	CStrip* m_pOwningStrip;
	CCell* m_pCell[MAX_CELLS]; // 1 source line, 1 target line, & one gloss line per strip
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
	// GDLC 2010-02-10 Added parameter to CalcPhraseBoxGapWidth with default value steadyAsSheGoes
	int			CalcPhraseBoxGapWidth(enum phraseBoxWidthAdjustMode widthMode = steadyAsSheGoes);

	int			m_nNewPhraseBoxGapWidth; // BEW 2Sep21, public, 0 except when expanding or contracting the phrasebox
										 // the phrasebox gap width at the active pile, by a value calculated in
										 // OnPhraseBoxChanged(), and set there.
	int			m_nOldPhraseBoxGapWidth; // the width, in OnPhraseBoxChange(), before any width change is computed
	int			CalcPhraseBoxWidth();
	// BEW 23Aug20 added next two. I need to pass the 'expanding' ( = 2 ) enum value back to view's PlacePhraseBox()
	void		CacheWidthMode(enum phraseBoxWidthAdjustMode enumIn = steadyAsSheGoes);
	enum phraseBoxWidthAdjustMode GetCachedWidthMode();
protected:
	enum phraseBoxWidthAdjustMode cachedWidthMode;
public:
	int			CalcPhraseBoxListWidth(); //BEW added 24Jul18 calculates the width of the listbox
					// for the CSourcePhrase instance at the active location (m_pActivePile) based
					// on the KB's pTU pointer for the CSourcePhrase's m_key member.
					// Internally, each of the non-(pseudo)deleted KB entries has its x-extent
					// measured, the the width is then set to the largest of these values.
					// The intent is that the width of the dropdown phrasebox (Layout's m_curBoxWidth)
					// will always be the larger of m_curBoxWidth and m_curListWidth. This has the
					// additional benefit that the list, when dropped down, will extend to the right
					// hand edge of the phrasebox's dropdown list button

	int			GetStripIndex();
	CStrip*		GetStrip();
	void		SetStrip(CStrip* pStrip);
	CCell*		GetCell(int nCellIndex);
	CCell**		GetCellArray();
	int			GetPileIndex();

	int			Width();
	int			Height();
	int			Left();
	int			Top();
	void		GetPileRect(wxRect& rect);
	wxRect		GetPileRect(); // overloaded version
	void		TopLeft(wxPoint& ptTopLeft);

	void		SetMinWidth(); // sets m_nMinWidth (width large enough for cells, calls CalcPileWidth())
	void		SetMinWidth(int width); // overload, for using when restoring a cached m_nMinWidth value;

//GDLC 2010-02-10 Added parameter to SetPhraseBoxGapWidth with default value steadyAsSheGoes
	int			SetPhraseBoxGapWidth();
	// sets m_nWidth (the width to be used at active 
	//location, calls CalcPhraseBoxGapWidth())

	int			SetPhraseBoxGapWidth(int nNewWidth);  // this overload sets Layout's the m_nWidth pile member
					// to an explicit number of pixels, as calculated externally, and returns its value
					// To be correct, it must have the new boxWidth value + buttonWidth + 1 + interpilegap
					// already added to it, as no further calcs of with will be done.
	
	int			GetMinWidth(); // returns value of m_nMinWidth
	int			GetPhraseBoxGapWidth(); // returns value of m_nWidth
	void		SetIsCurrentFreeTransSection(bool bIsCurrentFreeTransSection);
	bool		GetIsCurrentFreeTransSection();
	int			GetPhraseBoxWidth(); //BEW added 19Jul18, gets Layout's m_curBoxWidth value
	void		SetPhraseBoxWidth(); // BEW added 19Jul18
	void		SetPhraseBoxWidth(int boxwidth); // an override, to set an explicit known width
	int			GetPhraseBoxListWidth(); // BEW added 24Jul18  accessor, gets Layout's m_curListWidth value
	void		SetPhraseBoxListWidth(int listWidth); // BEW changed 27Sep21  accessor, set's Layout's m_curListWidth value, 
					 // after calling int CalcPhraseBoxListWidth() at m_pActivePile

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
