/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			HtmlFileViewer.h
/// \author			Bill Martin
/// \date_created	14 September 2011
/// \date_revised	14 September 2011
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CHtmlFileViewer class. 
/// The CHtmlFileViewer class provides an instance of a framed wxHtmlWindow for use in displaying an Html file.
/// \derivation		The CHtmlFileViewer class is derived from wxFrame.
/////////////////////////////////////////////////////////////////////////////

#ifndef HtmlFileViewer_h
#define HtmlFileViewer_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "HtmlFileViewer.h"
#endif

class CHtmlFileViewer : public wxFrame
{
public:
	CHtmlFileViewer(wxWindow* parent, wxString* title, wxString* pathToHtmlFile); // constructor
	virtual ~CHtmlFileViewer(void); // destructor
	// other methods
	wxSizer* pHtmlFileViewerSizer;

protected:
	//void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& WXUNUSED(event)); // necessary since CHtmlFileViewer is modeless - must call Destroy
	void OnMoveBack(wxCommandEvent& WXUNUSED(event));
	void OnMoveForward(wxCommandEvent& WXUNUSED(event));
	void OnOpenHtmlFile(wxCommandEvent& WXUNUSED(event));

private:
	CAdapt_ItApp* m_pApp;
	wxString adminHelpFilePath;
	wxHtmlWindow* pHtmlWindow;
	wxButton* pBackButton;
	wxButton* pForwardButton;
	wxTextCtrl* pTextCtrlHtmlFilePath;
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* HtmlFileViewer_h */
