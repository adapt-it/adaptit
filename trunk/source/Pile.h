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
	int				m_nWidth;
	int				m_nMinWidth;
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
	int			GetStripIndex();
	CStrip*		GetStrip();
	void		SetStrip(CStrip* pStrip);
	CCell*		GetCell(int nCellIndex);
	CCell**		GetCellArray();
	int			GetPileIndex();
	int			GetWidth(); // BEW added 14Mar11

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

private:
	bool		HasFilterMarker(); // returns TRUE if the pointed at CSourcePhrase has \~FILTER in m_markers

	DECLARE_DYNAMIC_CLASS(CPile) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CClassName)
	
};

#endif // Pile_h
