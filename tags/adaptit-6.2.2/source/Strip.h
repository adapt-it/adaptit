/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Strip.h
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public
///  License (see license directory)
/// \description	This is the header file for the CStrip class. 
/// The CStrip class represents the next smaller divisions of a CBundle.
/// Each CStrip stores an ordered list of CPile instances, which are
/// displayed in LtoR languages from left to right, and in RtoL languages
/// from right to left.
/// \derivation		The CStrip class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef Strip_h
#define Strip_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Strip.h"
#endif

// forward references:
#include "Pile.h"
class CStrip;
class CLayout;

WX_DECLARE_LIST(CStrip, StripList); // see list definition macro in .cpp file


/// The CStrip class is what defines the ordered sequence of groups of CPiles arranged
/// vertically in the canvas window. The piles are displayed in LtoR languages from left to
/// right, and in RtoL languages from right to left.
/// \derivation The CStrip class is derived from wxObject.
class CStrip : public wxObject  
{
	friend class CPile;
	friend class CLayout;
	friend class CCell;
public:
	// creation (a 2 step process)
	CStrip(); // doesn't set m_pLayout member to nonNull pointer
	CStrip(CLayout* pLayout); // use this one for strip creation

private:
	// attributes
	int				m_nStrip;	// index of this strip in CLayout's m_arrStrips array of pointers
	CLayout*		m_pLayout;  // the owning CLayout
	wxArrayPtrVoid	m_arrPiles; // array of CPile* instances which comprise the strip
	wxArrayInt		m_arrPileOffsets; // offset from left bdry of strip to left bdry of pile
	int				m_nFree;	// how many pixels wide the free space at end is
	bool			m_bValid;  // TRUE if has populated fully, or not involved in any user
                               // editing but FALSE if one or more of its piles were edited or
                               // changed in any way, or if the strip's population of piles
                               // is not yet full
	
public:

	virtual ~CStrip();
	virtual void Draw(wxDC* pDC);
	int		Width();
	int		Height();
	int		Left();
	int		Top();
	int		GetFree();
	void	SetFree(int nFree);
	int		GetStripIndex();
	void	GetStripRect_CellsOnly(wxRect& rect);
	wxRect	GetStripRect_CellsOnly(); // overloaded version
	wxRect	GetStripRect(); // includes the free translation area if in free trans mode
							// (this one is to give backwards compatibility to pre-refactored
							// version calculations for scrolling)
	void	GetFreeTransRect(wxRect& rect);
	wxRect	GetFreeTransRect(); // overloaded version

	PileList::Node* CreateStrip(PileList::Node*& pos, int nStripWidth, int gap); // return 
															// iterator of next for placement
	// next version is overloaded, uses indices, and has nEndPileIndex for the index of
	// the last pile which is to be placed in the emptied strips; this version used for filling
	// emptied strips using a subrange of the available pile pointers only (this means the
	// option keep_strips_keep_piles is used in RecalcLayout(), and that option is never
	// used when printing, or print previewing, and so this version never is used when
	// printing is happening, or print previewing)
	int		CreateStrip(int nInitialPileIndex, int nEndPileIndex, int nStripWidth, int gap);
	int		GetPileCount();
	CPile*	GetPileByIndex(int index);
	wxArrayPtrVoid* GetPilesArray();

	// validity flag for a strip, TRUE if strip is unchanged by user editing
	void	SetValidityFlag(bool bValid);
	bool	GetValidityFlag();

	
	
	DECLARE_DYNAMIC_CLASS(CStrip) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CRefString)

};

#endif // Strip_h
