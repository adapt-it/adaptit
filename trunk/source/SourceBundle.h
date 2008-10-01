/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SourceBundle.h
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CSourceBundle class. 
/// The CSourceBundle class represents the on-screen layout of a vertical series
/// of CStrip instances. The View uses some "indices" to define a "bundle" of
/// CSourcePhrase instances from the list maintained in the m_pSourcePhrases
/// list on the app. This bundle gets laid out by the function 
/// RecalcLayout(). LayoutStrip() is sometimes used to layout just a single
/// strip rather than the whole bundle.
/// \derivation		The CSourceBundle class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef SourceBundle_h
#define SourceBundle_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "SourceBundle.h"
#endif

//#include "Strip.h" // Added by ClassView. MFC puts this here instead of in the SourceBundle.cpp file ???

// forward references
class CAdapt_ItDoc;
class CAdapt_ItView;
class CStrip;
class CCell;

/// The CSourceBundle class represents the on-screen layout of a vertical series
/// of CStrip instances. The View uses some "indices" to define a "bundle" of
/// CSourcePhrase instances from the list maintained in the m_pSourcePhrases
/// list on the app. This bundle gets laid out by the function 
/// RecalcLayout(). LayoutStrip() is sometimes used to layout just a single
/// strip rather than the whole bundle.
/// \derivation		The CSourceBundle class is derived from wxObject.
class CSourceBundle : public wxObject  
{

public:
	CSourceBundle(); // default constructor
	CSourceBundle(CAdapt_ItDoc* pDocument, CAdapt_ItView* pView); // normal constructor

	// attributes
public:
	void	DestroyStrips(const int nFirstStrip);


public:
	// whm Note: Ordering of members here should be OK.
	int					m_nStripCount; // how many strips we have
	int					m_nStripIndex; // index to current strip being accessed or created
	CStrip*				m_pStrip[6000]; // enough to handle paginating whole of Luke (1140 verses)
										// for printing - at least it should be if margins are not large
	int					m_nLMargin;
	int					m_nLeading;
	CAdapt_ItDoc*		m_pDoc;
	CAdapt_ItView*		m_pView;


	// destructor
	virtual ~CSourceBundle();

	virtual void Draw(wxDC* pDC);

	DECLARE_DYNAMIC_CLASS(CSourceBundle) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CRefString)
};

#endif // SourceBundle_h
