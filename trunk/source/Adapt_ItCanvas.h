/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			Adapt_ItCanvas.h
/// \author			Bill Martin
/// \date_created	12 February 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CAdapt_ItCanvas class. 
/// The CAdapt_ItCanvas class implements the main Adapt It window based on
/// wxScrolledWindow. This is required because wxWidgets' doc/view framework
/// does not have an equivalent for the CScrolledView in MFC.
/// \derivation		The CAdapt_ItCanvas class is derived from wxScrolledWindow.
/////////////////////////////////////////////////////////////////////////////

#ifndef Adapt_ItCanvas_h
#define Adapt_ItCanvas_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Adapt_ItCanvas.h"
#endif

// Forward references
class CMainFrame;
class CAdapt_ItView;

/// The CAdapt_ItCanvas class implements the main Adapt It window based on
/// wxScrolledWindow. This is required because wxWidgets' doc/view framework
/// does not have an equivalent for the CScrolledView in MFC.
/// \derivation		The CAdapt_ItCanvas class is derived from wxScrolledWindow.
class CAdapt_ItCanvas : public wxScrolledWindow
{

public:

	CAdapt_ItCanvas();
	CAdapt_ItCanvas(CMainFrame* frame, const wxPoint& pos, const wxSize& size, const long style);
    //void OnDraw(wxDC& dc); //virtual void OnDraw(wxDC& dc); 
	// since OnDraw in wxScrolledWindow is virtual, 'virtual' keyword is not needed here
    void OnPaint(wxPaintEvent &WXUNUSED(event));
	void DoPrepareDC(wxDC& dc); // this is called OnPrepareDC() in MFC

#if wxUSE_GRAPHICS_CONTEXT
	void UseGraphicContext(bool use) {m_useContext = use; Refresh();};
#endif
	
	//void OnPaint(wxPaintEvent& event); // see note in .cpp file
	//void OnSize(wxSizeEvent& event); // see note in .cpp file
    void OnLButtonDown(wxMouseEvent& event);
    void OnLButtonUp(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	int			ScrollDown(int nStrips);
	void		ScrollIntoView(int nSequNum);
	//void		ScrollToNearTop(int nSequNum); // unused in wx version
	int			ScrollUp(int nStrips);
	//enum AnchorStripLocation GetAnchorSegmentVisibility(int nPrecStrips, int nFollStrips);
	
	// We include a pointer to the view in the canvas class, primarily to trigger the View's
	// virtual OnDraw() method [see implementation of canvas' OnDraw() which calls the View's
	// OnDraw()].
	CAdapt_ItView* pView; 

	// We include a pointer to the owning frame. The wxWidgets Drawing sample program calls PrepareDC
	// both on the canvas and on the owner of the canvas (see drawing.cpp).
	CMainFrame* pFrame;

	// wx Note: wxScrollEvent only appears to intercept scroll events for scroll bars manually
	// placed in wxWindow based windows. In order to handle scroll events for windows like
	// wxScrolledWindow (that have built-in scrollbars), we must use wxScrollWinEvent in the 
	// functions and the EVT_SCROLLWIN macro in the event table.
	void OnScroll(wxScrollWinEvent& event); // process all scroll events of meaning to Adapt It
	
	// These are virtual methods in the Doc hence are automatically virtual 
    bool IsModified() const;
	void DiscardEdits();

	// Should ~CAdapt_ItCanvas destructor be virtual??? 
	// Rule of thumb (from Steve Clamage at cpptips.hyperformix.com): 
	// "Any class that you intend to derive from should have a virtual destructor. 
	// Otherwise you risk undefined behavior if you delete an object via a pointer 
	// to such a base class. In particular, if a class has virtual functions, it 
	// should have a virtual destructor."
	// OnDraw() above is virtual in wxScrolledWindow, and here too as well since 
	// CAdapt_ItCanvas is a derived from wxScrolledWindow. To be safe I think the
	// best thing to do is to make all destructors virtual.
	virtual ~CAdapt_ItCanvas(void); // whm make all destructors virtual
private:
	// class attributes

#if wxUSE_GRAPHICS_CONTEXT
	bool m_useContext;
#endif

	//DECLARE_CLASS(CAdapt_ItCanvas);
	// Used inside a class declaration to declare that the class should 
	// be made known to the class hierarchy, but objects of this class 
	// cannot be created dynamically. The same as DECLARE_ABSTRACT_CLASS.
	
	// or, comment out above and uncomment below to
	DECLARE_DYNAMIC_CLASS(CAdapt_ItCanvas) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CAdapt_ItCanvas)
	
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* Adapt_ItCanvas_h */
