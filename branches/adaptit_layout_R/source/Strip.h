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
//class CAdapt_ItDoc; // BEW deprecated 3Feb09
//class CSourceBundle;
class CPile;
class CStrip;
class CLayout; // make the "friend class CLayout" declaration work

WX_DECLARE_LIST(CStrip, StripList); // see list definition macro in .cpp file


/// The CStrip class is what defines the ordered sequence of groups of CPiles arranged vertically
/// in the canvas window. The piles are displayed in LtoR languages from left to right, and in RtoL
/// languages from right to left. 
/// \derivation The CStrip class is derived from wxObject.
class CStrip : public wxObject  
{
	friend class CPile;
	friend class CLayout;
	friend class CCell;
public:
	CStrip();
	//CStrip(CAdapt_ItDoc* pDocument, CSourceBundle* pSourceBundle); // BEW deprecated 3Feb09
	//CStrip(CSourceBundle* pSourceBundle); // normal constructor

	// whm Note: Initialization of CStrip class members happens in the order in which
	// they are declared. To avoid potential null pointer problems, I have moved the
	// CSourceBundle* m-pBundle line up before the declaration of CPile* m-pPile[36] line.
	
	
private:
	// attributes
	int			m_nStrip; // index of this strip in CLayout's m_arrStrips array of pointers
	CLayout*	m_pLayout; // the owning CLayout
	wxArrayInt	m_arrPiles; // array of CPile* instances which comprise the strip
	wxArrayInt	m_arrPileOffsets; // offset from left bdry of strip to left bdry of pile
	int			m_nFree;	// how many pixels wide the free space at end is
	bool		m_bFilled; // TRUE if has populated fully, or up to a wrapping CPile
	bool		m_bAffected; // TRUE if the strip is involved in a user action, and so has
							 // to be checked by the action's cleanup code before drawing
	
	/* legacy stuff not now needed
	int					m_nStripIndex;
	//int				m_nPileHeight;  // BEW deprecated 3Feb09
	int					m_nPileCount;
	int					m_nVertOffset;
	int					m_nFree;	// how many pixels wide the free space at end is
	wxRect				m_rectStrip;
	CPile*				m_pPile[36];	// allow 36 per row
	//CAdapt_ItDoc*		m_pDoc;	// the doc, to give access to GetApp(), etc. BEW deprecated 3Feb09
	wxRect				m_rectFreeTrans; // BEW added 24Jun05 for support of free translations display
	*/
public:

	virtual ~CStrip();
	virtual void Draw(wxDC* pDC);
	

	//  creator
	//int	CreateStrip(wxClientDC* pDC, SPList* pSrcList, int nVertOffset, 
	//				int& nLastSequNumber, int nEndIndex);
	void	CreateStrip(CLayout* pLayout, int nStripWidth, int nIndexOfFirstPile);

	
	DECLARE_DYNAMIC_CLASS(CStrip) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CRefString)

};



// The legacy (pre-4.1.x) CStrip class represents the next smaller divisions of a CBundle. Each
// CStrip stores an ordered list of CPile instances, which are displayed in LtoR languages from
// left to right, and in RtoL languages from right to left. \derivation The CStrip class is derived
// from wxObject.
/*
class CStrip : public wxObject  
{

public:
	CStrip();
	//CStrip(CAdapt_ItDoc* pDocument, CSourceBundle* pSourceBundle); // BEW deprecated 3Feb09
	CStrip(CSourceBundle* pSourceBundle); // normal constructor


	// attributes
public:
	// whm Note: Initialization of CStrip class members happens in the order in which
	// they are declared. To avoid potential null pointer problems, I have moved the
	// CSourceBundle* m-pBundle line up before the declaration of CPile* m-pPile[36] line.
	void DestroyPiles();
	int					m_nStripIndex;
	//int				m_nPileHeight;  // BEW deprecated 3Feb09
	int					m_nPileCount;
	int					m_nVertOffset;
	int					m_nFree;	// how many pixels wide the free space at end is
	wxRect				m_rectStrip;
	CSourceBundle*		m_pBundle; // the owning souce bundle (can only be one)
	CPile*				m_pPile[36];	// allow 36 per row
	//CAdapt_ItDoc*		m_pDoc;	// the doc, to give access to GetApp(), etc. BEW deprecated 3Feb09
	wxRect				m_rectFreeTrans; // BEW added 24Jun05 for support of free translations display


	virtual ~CStrip();
	virtual void Draw(wxDC* pDC);

	//  creator
int	CreateStrip(wxClientDC* pDC, SPList* pSrcList, int nVertOffset, int& nLastSequNumber, int nEndIndex);

	
	DECLARE_DYNAMIC_CLASS(CStrip) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CRefString)

};
*/
#endif // Strip_h
