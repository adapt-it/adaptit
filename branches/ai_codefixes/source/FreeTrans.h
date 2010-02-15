/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FreeTrans.h
/// \author			Graeme Costin
/// \date_created	10 Februuary 2010
/// \date_revised	10 Februuary 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the header file for the CFreeTrans class. 
/// The CFreeTrans class presents free translation fields to the user. 
/// The functionality in the CFreeTrans class was originally contained in
/// the CAdapt_ItView class.
/// \derivation		The CFreeTrans class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef FreeTrans_h
#define FreeTrans_h

#ifdef	_FREETR

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "FreeTrans.h"
#endif

#include <wx/gdicmn.h>

// forward declarations
class wxObject;
class CAdapt_ItApp;
class CAdapt_ItCanvas;
class CAdapt_ItView;
class CPile;
class CLayout; // make the "friend class CLayout" declaration work

//GDLC 2010-02-12 Definition of FreeTrElement moved to FreeTrans.h
/// A struct containing the information relevant to writing a subpart of the free
/// translation in a single rectangle under a single strip. Struct members include:
/// horizExtent and subRect.
struct FreeTrElement 
{
	int horizExtent;
	wxRect subRect;
};

//////////////////////////////////////////////////////////////////////////////////
/// The CFreeTrans class presents free translation fields to the user. 
/// The functionality in the CFreeTrans class was originally contained in
/// the CAdapt_ItView class.
/// \derivation		The CFreeTrans class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////
class CFreeTrans : public wxEvtHandler
{
	friend class CLayout;
public:

	CFreeTrans(); // default constructor
	CFreeTrans(CAdapt_ItApp* app); // use this one

	virtual ~CFreeTrans();// destructor // whm added always make virtual

	// Utility functions
	CLayout* CFreeTrans::GetLayout();
	CAdapt_ItView*	CFreeTrans::GetView();
	
	// Public free translation drawing functions
	//GDLC 2010-02-12+ Moved free translation functions here from CAdapt_ItView
	void	DrawFreeTranslations(wxDC* pDC, CLayout* pLayout, 
										 enum DrawFTCaller drawFTCaller);

	// Private free translation drawing functions
	void	StoreFreeTranslation(wxArrayPtrVoid* pPileArray,CPile*& pFirstPile,CPile*& pLastPile, 
					enum EditBoxContents editBoxContents, const wxString& mkrStr);
	void	DestroyElements(wxArrayPtrVoid* pArr);
	CPile*	GetStartingPileForScan(int activeSequNum);
	void	OnAdvancedFreeTranslationMode(wxCommandEvent& WXUNUSED(event));
	void	SegmentFreeTranslation(wxDC* pDC,wxString& str, wxString& ellipsis, int textHExtent,
				int totalHExtent, wxArrayPtrVoid* pElementsArray, wxArrayString* pSubstrings, int totalRects);
	void	SetupCurrentFreeTransSection(int activeSequNum);
	void	StoreFreeTranslationOnLeaving();
	void	OnAdvanceButton(wxCommandEvent& event);
	void	OnNextButton(wxCommandEvent& WXUNUSED(event));
	void	OnPrevButton(wxCommandEvent& WXUNUSED(event));
	void	OnRemoveFreeTranslationButton(wxCommandEvent& WXUNUSED(event));
	void	OnLengthenButton(wxCommandEvent& WXUNUSED(event));

private:
	CAdapt_ItApp*	m_pApp;	// The app owns this

	CLayout*		m_pLayout;
	CAdapt_ItView*	m_pView;

	/// An array of pointers used as a scratch array, for composing the free translations which
	/// are written to screen. Element pointers point to FreeTrElement structs - each of which
	/// contains the information relevant to writing a subpart of the free translation in a
	/// single rectangle under a single strip.
	wxArrayPtrVoid*	m_pFreeTransArray; 

	/// Pointer to first pile in a free translation section. gpLastPile points to the last pile
	/// in the same free translation section.
	CPile* m_pFirstPile;

	/// Pointer to last pile in the free translation section. gpFirstPile points to the first
	/// pile in the same free translation section.
	CPile* m_pLastPile;

	/// An array of pointers to CPile instances. It is created on the heap in OnInit(), 
	/// and disposed of in OnExit().
	wxArrayPtrVoid*	m_pCurFreeTransSectionPileArray;

	/// GDLC 2010-02-13 Moved from CAdapt_It (soon to become obsolete)
	/// The offset to the current free translation string in pSrcPhrase->m_markers.
	int m_nOffsetInMarkersStr; 

	/// The free translation length, including final space if any, in pSrcPhrase->m_markers.
	int m_nLengthInMarkersStr; 

};

#endif	// _FREETR

#endif /* FreeTrans_h */
