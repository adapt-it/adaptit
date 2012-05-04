/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SetEncodingDlg.h
/// \author			Bill Martin
/// \date_created	5 February 2007
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CSetEncodingDlg class. 
/// The CSetEncodingDlg class provides a means for examining a font's encoding 
/// and changing that encoding to a more appropriate value if desired. See the
/// InitializeFonts() function in the app for more information about mapping
/// encodings between MFC and wxWidgets.
/// Also included are the supporting classes: CEncodingTestBox, FontDisplayCanvas,
/// FontEncodingEnumerator, and FontFacenameEnumerator.
/// \derivation		The CSetEncodingDlg class is derived from AIModalDialog.
/// Some of the code is adapted from the wxWidgets font.cpp sample program.
/////////////////////////////////////////////////////////////////////////////

#ifndef SetEncodingDlg_h
#define SetEncodingDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "SetEncodingDlg.h"
#endif

#include <wx/fontmap.h>

/// CEncodingTestBox is a supporting class of the SetEncodingDlg class.
/// \derivation CEncodingTestBox is derived from wxTextCtrl.
class CEncodingTestBox : public wxTextCtrl
{
public:
	CEncodingTestBox(void);
	CEncodingTestBox(wxWindow *parent, wxWindowID id, const wxString &value,
				const wxPoint &pos, const wxSize &size, int style = 0)
				: wxTextCtrl(parent, id, value, pos, size, style)
	{
	}

    virtual ~CEncodingTestBox(){};
	void OnChar(wxKeyEvent& event);

private:
    DECLARE_EVENT_TABLE()
};

/// FontDisplayCanvas is a supporting class of the SetEncodingDlg class.
/// FontDisplayCanvas is a canvas on which the font sample is shown.
/// \derivation FontDisplayCanvas is derived from wxWindow.
class FontDisplayCanvas: public wxWindow
{
public:
    FontDisplayCanvas( wxWindow *parent );
    virtual ~FontDisplayCanvas(){};

    // accessors for the frame
    const wxFont& GetTextFont() const { return m_font; }
	// The following are handy to have but we don't need them at present
    void SetTextFont(const wxFont& font) { m_font = font; }
    void SetColour(const wxColour& colour) { m_colour = colour; }

    // event handlers
    void OnPaint( wxPaintEvent& WXUNUSED(event) );

private:
    wxColour m_colour;
    wxFont   m_font;

    DECLARE_EVENT_TABLE()
};

/// FontEncodingEnumerator is a supporting class of the SetEncodingDlg class.
/// \derivation FontEncodingEnumerator is derived from wxFontEnumerator.
class FontEncodingEnumerator : public wxFontEnumerator
{
public:
	FontEncodingEnumerator()
		{ m_n = 0; }

	const wxString& GetFacenameText() const
		{ return m_facename_text; }
	const wxString& GetEncodingText() const
		{ return m_encoding_text; }

protected:
	virtual bool OnFontEncoding(const wxString& facename,
								const wxString& encoding)
	{
		// make our list items start with the shorter encoding name followed by the 
		// longer encoding description name in square brackets.
		m_facename_text += facename + _T("\n");
		wxFontEncoding fenc;
		fenc = wxFontMapper::GetEncodingFromName(encoding);
		m_encoding_text += encoding + _T(" [") + wxFontMapper::GetEncodingDescription(fenc) + _T("]\n");
		return true;
	}

private:
	size_t m_n;
	wxString m_encoding_text;
	wxString m_facename_text;
};

/// FontFacenameEnumerator is a supporting class of the SetEncodingDlg class.
/// \derivation FontFacenameEnumerator is derived from wxFontEnumerator.
class FontFacenameEnumerator : public wxFontEnumerator
{
public:
    bool GotAny() const
        { return !m_facenames.IsEmpty(); }

    const wxArrayString& GetFacenames() const
        { return m_facenames; }

protected:
    virtual bool OnFacename(const wxString& facename)
    {
        m_facenames.Add(facename);
        return true;
    }

    private:
        wxArrayString m_facenames;
};

/// The CSetEncodingDlg class provides a means for examining a font's encoding 
/// and changing that encoding to a more appropriate value if desired. See the
/// InitializeFonts() function in the app for more information about mapping
/// encodings between MFC and wxWidgets.
/// \derivation		The CSetEncodingDlg class is derived from AIModalDialog.
/// Some of the code is adapted from the wxWidgets font.cpp sample program.
class CSetEncodingDlg : public AIModalDialog
{
public:
	CSetEncodingDlg(wxWindow* parent); // constructor
	virtual ~CSetEncodingDlg(void); // destructor
    // accessors
    FontDisplayCanvas *GetCanvas() const { return m_canvas; }
	
	wxStaticText* pStaticCurrEncodingIs;
	wxStaticText* pStaticSetFontTitle;
	wxStaticText* pStaticChartFontSize;
	wxTextCtrl* pCurrEncoding;
	CEncodingTestBox* pTestEncodingBox; // not created by wxDesigner, but here locally
	wxListBox* pPossibleEncodings;
	wxListBox* pPossibleFacenames;
	wxScrolledWindow* pScrolledEncodingWindow;
	//wxPanel* pPanelForCanvas;
	wxButton* pApplyEncodingButton;
	wxButton* pIncreaseChartFontSize;
	wxButton* pDecreaseChartFontSize;

	wxString langFontName;
	wxString thisFontsFaceName;
	wxString fontFaceNameSelected;
	wxFontEncoding thisFontsCurrEncoding;
	wxFontEncoding fontEncodingSelected;
	wxString thisFontsEncodingNameAndDescription;
	wxString fontsEncodingNameAndDescriptionSelected;
	wxFontFamily thisFontsFamily;
	wxFontFamily fontFamilySelected;

	wxSizer* pSetEncDlgSizer;
	
	// other methods

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

	void DoChangeFont(const wxFont& font, const wxColour& col = wxNullColour);
    void OnIncFont(wxCommandEvent& WXUNUSED(event)) { DoResizeFont(+2); }
    void OnDecFont(wxCommandEvent& WXUNUSED(event)) { DoResizeFont(-2); }
	void DoResizeFont(int diff);
	void OnBtnChartFontSizeIncrease(wxCommandEvent& WXUNUSED(event));
	void OnBtnChartFontSizeDecrease(wxCommandEvent& WXUNUSED(event));
	void OnListEncodingsChanged(wxCommandEvent& WXUNUSED(event));
	void OnListFacenamesChanged(wxCommandEvent& WXUNUSED(event));
	void OnChar(wxKeyEvent& event);

	// a helper function
	wxString GetEncodingSymbolFromListStr(wxString listStr);

    FontDisplayCanvas* m_canvas;

private:
	int nCurrListSelEncoding;
	int nCurrListSelFaceName;
	int nTempFontSize;
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#endif /* SetEncodingDlg_h */
