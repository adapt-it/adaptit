/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Layout.h
/// \author			Bruce Waters
/// \date_created	09 February 2009
/// \date_revised	
/// \copyright		2009 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CLayout class. 
/// The CLayout class replaces the legacy CSourceBundle class, and it encapsulated as much as
/// possible of the visible layout code for the document. It manages persistent CPile objects
/// for the whole document - one per CSourcePhrase instance. Copies of the CPile pointers are
/// stored in CStrip objects. CStrip, CPile and CCell classes are used in the refactored layout,
/// but with a revised attribute inventory; and wxPoint and wxRect members are computed on the
/// fly as needed using functions, which reduces the layout computations when the user does things
/// by a considerable amount.
/// \derivation		The CLayout class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef Layout_h
#define Layout_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Layout.h"
#endif

// forward references
class CAdapt_ItDoc;
class CSourceBundle;
class CStrip;
class CPile;
class CText;
class CFont;

WX_DECLARE_LIST(CPile, PileList); // see list definition macro in .cpp file


/// The CLayout class manages the layout of the document. It's private members pull
/// together into one place parameters pertinent to dynamically laying out the strips
/// piles and cells of the layout. Setters in various parts of the application set
/// these private members, and getters are used by the layout functionalities to
/// get the drawing done correctly
/// \derivation		The CLayout class is derived from wxObject.
class CLayout : public wxObject  
{

public:
	// constructors
	CLayout(); // default constructor

	// attributes
	CAdapt_ItApp*		m_pApp;
	CAdapt_ItDoc*		m_pDoc;
	CAdapt_ItView*		m_pView;
	CAdapt_ItCanvas*	m_pCanvas;

public:
	PileList* m_pPiles;


private:
	// two ints define the range of strips to be drawn at next draw
	int			m_nFirstVisibleStrip;
	int			m_nLastVisibleStrip;

	// four ints define the clip rectange top, left, width & height for erasure 
	// prior to draw (window client coordinates, (0,0) is client top left)
	int			m_nClipRectTop;
	int			m_nClipRectLeft;
	int			m_nClipRectWidth;
	int			m_nClipRectHeight;


public:
	// destructor
	virtual ~CLayout();
	virtual void Draw(wxDC* pDC);

	// helpers; setters & getters
	CAdapt_ItApp*		GetApp();
	CAdapt_ItView*		GetView();
	CAdapt_ItCanvas*	GetCanvas();
	CAdapt_ItDoc*		GetDoc();

	// create the list of CPile objects (it's a parallel list to document's m_pSourcePhrases
	// list, and each CPile instance has a member which points to one and only one
	// CSourcePhrase instance in pSrcPhrases)
	bool				CreatePiles(PileList* pPiles, SPList* pSrcPhrases);

	// getters for clipping rectangle
	wxRect				GetClipRect();

	// setters for clipping rectangle
	void				SetClipRectTop(int nTop);
	void				SetClipRectLeft(int nLeft);
	void				SetClipRectWidth(int nWidth);
	void				SetClipRectHeight(int nHeight);

	// setters and getters for visible strip range
	int					GetFirstVisibleStrip();
	int					GetLastVisibleStrip();
	void				SetFirstVisibleStrip(int nFirstVisibleStrip);
	void				SetLastVisibleStrip(int nLastVisibleStrip);

	DECLARE_DYNAMIC_CLASS(CLayout) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information.
};

#endif
