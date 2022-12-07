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
							  // location; at other piles it stores -1
	bool			m_bIsCurrentFreeTransSection; // BEW added 24Jun05 for support of free translations
	int			m_nWidth; // stores width as calculated from PhraseBox width, so is valid only at active

	// destructor
public:
	virtual		~CPile();

	virtual		void Draw(wxDC* pDC);
	void		DrawNavTextInfoAndIcons(wxDC* pDC); // handles wedge, note icon, retranslation, nav text etc
	void		PrintPhraseBox(wxDC* pDC);
	bool		IsWrapPile();

	// whm 11Nov2022 moved m_nMinWidth here to public
	int			m_nMinWidth; // this stores the actual pile width based on the extent of the text
	int			CalcPileWidth(); // based on the text in the cells only, no account taken of active loc
	int			CalcExtentsBasedWidth(); // a cut down version of CalcPileWidth for use in OnPhraseBoxChanged
	
	// whm 11Nov2022 Note: The following CalcPhraseBoxGapWidth() combines the returned values 
	// from CalcPhraseBoxWidth() + GetExtraWidthForButton()
	int			CalcPhraseBoxGapWidth(); // combines returned values from CalcPhraseBoxWidth() + GetExtraWidthForButton()

	int			CalcPhraseBoxWidth();

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

	void		SetIsCurrentFreeTransSection(bool bIsCurrentFreeTransSection);
	bool		GetIsCurrentFreeTransSection();

private:
	bool		HasFilterMarker(); // returns TRUE if the pointed at CSourcePhrase has \~FILTER in m_markers

	DECLARE_DYNAMIC_CLASS(CPile)
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CClassName)
};

#endif // Pile_h
