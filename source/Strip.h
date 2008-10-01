/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			Strip.h
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
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
class CAdapt_ItDoc;
class CPile;
class CSourceBundle;

/// The CStrip class represents the next smaller divisions of a CBundle.
/// Each CStrip stores an ordered list of CPile instances, which are
/// displayed in LtoR languages from left to right, and in RtoL languages
/// from right to left.
/// \derivation		The CStrip class is derived from wxObject.
class CStrip : public wxObject  
{

public:
	CStrip();
	CStrip(CAdapt_ItDoc* pDocument, CSourceBundle* pSourceBundle); // normal constructor


	// attributes
public:
	// whm Note: Initialization of CStrip class members happens in the order in which
	// they are declared. To avoid potential null pointer problems, I have moved the
	// CSourceBundle* m-pBundle line up before the declaration of CPile* m-pPile[36] line.
	void DestroyPiles();
	int					m_nStripIndex;
	int					m_nPileHeight;
	int					m_nPileCount;
	int					m_nVertOffset;
	int					m_nFree;	// how many pixels wide the free space at end is
	wxRect				m_rectStrip;
	CSourceBundle*		m_pBundle; // the owning souce bundle (can only be one)
	CPile*				m_pPile[36];	// allow 36 per row
	CAdapt_ItDoc*		m_pDoc;	// the doc, to give access to GetApp(), etc.
	wxRect				m_rectFreeTrans; // BEW added 24Jun05 for support of free translations display


	virtual ~CStrip();

	virtual void Draw(wxDC* pDC);
	
	DECLARE_DYNAMIC_CLASS(CStrip) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CRefString)

};

#endif // Strip_h
