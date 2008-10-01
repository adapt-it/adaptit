/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			Text.h
/// \author			Bill Martin
/// \date_created	26 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CText class. 
/// The CText class lowest level unit in the bundle-strip-pile-cell-text
/// hierarchy of objects forming the view displayed to the user on the
/// canvas of the main window of the Adapt It application.
/// \derivation		The CText class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef Text_h
#define Text_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Text.h"
#endif

/// The CText class is the lowest level unit in the bundle-strip-pile-cell-text
/// hierarchy of objects forming the view displayed to the user on the
/// canvas of the main window of the Adapt It application.
/// \derivation		The CText class is derived from wxObject.
class CText : public wxObject  
{
public:
	CText();

	/// Custom constructor used primarily in CAdapt_ItView::CreateCell()
	CText(wxPoint& start, wxPoint& end, wxFont* pFont,
				const wxString& phrase, const wxColour& color, int nCell);
	
	virtual void Draw(wxDC* pDC);	///< draws the text

	wxRect		m_enclosingRect;	///< the rect where the text will be drawn
	bool		m_bSelected;		///< TRUE if text is within a selection, FALSE otherwise
	wxColour	m_color;	// color of the text (BEW 2Aug08 made it public in
							// support of text gray colouring in vertical edit mode)

protected:
	wxPoint		m_topLeft;	///< point in logical coords, where text is to be displayed
	wxFont*		m_pFont;	///< font in which to draw
	int			m_nCell;	///< the parent cell
public:
	wxString	m_phrase;	///< the phrase to be displayed
	virtual ~CText();

	/// Used inside a class declaration to declare that the objects of 
	/// this class should be dynamically creatable from run-time type 
	/// information. MFC uses DECLARE_DYNCREATE(CRefString)
	DECLARE_DYNAMIC_CLASS(CText) 
};

#endif // Text_h
