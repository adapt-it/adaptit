/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			HtmlFileViewer.cpp
/// \author			Bill Martin
/// \date_created	14 September 2011
/// \date_revised	14 September 2011
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CHtmlFileViewer class. 
/// The CHtmlFileViewer class provides an instance of a framed wxHtmlWindow for use in displaying an Html file.
/// \derivation		The CHtmlFileViewer class is derived from wxFrame.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in HtmlFileViewer.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "HtmlFileViewer.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/html/htmlwin.h>
#include "Adapt_It.h"
#include "HtmlFileViewer.h"

// event handler table
BEGIN_EVENT_TABLE(CHtmlFileViewer, wxFrame)
	//EVT_INIT_DIALOG(CHtmlFileViewer::InitDialog)// Can't use this in CHtmlFileViewer
	EVT_BUTTON(wxID_CANCEL, CHtmlFileViewer::OnCancel)
	// Samples:
	EVT_BUTTON(ID_BITMAPBUTTON_MOVE_BACK, CHtmlFileViewer::OnMoveBack)
	EVT_BUTTON(ID_BITMAPBUTTON_MOVE_FORWARD, CHtmlFileViewer::OnMoveForward)
	EVT_BUTTON(ID_BUTTON_OPEN_HTML_FILE, CHtmlFileViewer::OnOpenHtmlFile)
	//EVT_MENU(ID_SOME_MENU_ITEM, CHtmlFileViewer::OnDoSomething)
	//EVT_UPDATE_UI(ID_SOME_MENU_ITEM, CHtmlFileViewer::OnUpdateDoSomething)
	//EVT_BUTTON(ID_SOME_BUTTON, CHtmlFileViewer::OnDoSomething)
	//EVT_CHECKBOX(ID_SOME_CHECKBOX, CHtmlFileViewer::OnDoSomething)
	//EVT_RADIOBUTTON(ID_SOME_RADIOBUTTON, CHtmlFileViewer::DoSomething)
	//EVT_LISTBOX(ID_SOME_LISTBOX, CHtmlFileViewer::DoSomething)
	//EVT_COMBOBOX(ID_SOME_COMBOBOX, CHtmlFileViewer::DoSomething)
	//EVT_TEXT(IDC_SOME_EDIT_CTRL, CHtmlFileViewer::OnEnChangeEditSomething)
	// ... other menu, button or control events
END_EVENT_TABLE()

CHtmlFileViewer::CHtmlFileViewer(wxWindow* parent, wxString* title, wxString* pathToHtmlFile) // dialog constructor
	: wxFrame(parent, -1, _("Html File Viewer"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	
	// whm 15Sep11 Note: 
	// The HtmlFileViewerDlgFunc() below is created by wxDesigner and it uses a generic 
	// "Foreign Control" with an ID of ID_HTML_WINDOW. The wxDesigner code does not
	// create that "foreign" control but leaves that up to us to do BEFORE the HtmlFileViewerDlgFunc()
	// is called. We create the foreign control as a wxHtmlFileViewer below which has this CHtmlFileViewer
	// dialog as its parent and uses the same ID_HTML_WINDOW that is declared within the wxDesigner
	// function.
	pHtmlWindow = new wxHtmlWindow(this,ID_HTML_WINDOW);
	
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pHtmlFileViewerSizer = HtmlFileViewerDlgFunc(this, false, TRUE); // Note: false for Fit so we can resize it arbitrarily
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	// Note: the "Help for Administrators.htm" file should go into the m_helpInstallPath
	// for each platform, which is determined by the GetDefaultPathForHelpFiles() call.
	adminHelpFilePath = *pathToHtmlFile; //m_pApp->GetDefaultPathForHelpFiles() + m_pApp->PathSeparator + m_pApp->m_adminHelpFileName;
	this->CreateStatusBar();
	pHtmlWindow->SetRelatedFrame(this,*title);
	pHtmlWindow->SetRelatedStatusBar(0);
	// The ReadCustomization() and WriteCustomization() in OnCancel() result in memory leaks, so
	// I've commented them out. Probably we dont' really need to save the position and size of the
	// html window anyway.
	//pHtmlFileViewer->ReadCustomization(wxConfig::Get()); // causes memory leaks
	pHtmlWindow->LoadFile(wxFileName(*pathToHtmlFile));
	
	pBackButton = (wxButton*)FindWindowById(ID_BITMAPBUTTON_MOVE_BACK);
	wxASSERT(pBackButton != NULL);

	pForwardButton = (wxButton*)FindWindowById(ID_BITMAPBUTTON_MOVE_FORWARD);
	wxASSERT(pForwardButton != NULL);

	pTextCtrlHtmlFilePath = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_HTML_FILE_PATH);
	pTextCtrlHtmlFilePath->SetBackgroundColour(m_pApp->sysColorBtnFace);
	wxASSERT(pTextCtrlHtmlFilePath != NULL);

	pTextCtrlHtmlFilePath->ChangeValue(*pathToHtmlFile);

	// whm Note: One problem in using this CHtmlFileViewer class is that,
	// although CHtmlFileViewer is running modeless, it cannot be 
	// accessed (to scroll etc.) while other Adapt It dialogs (such
	// as the Setup Paratext Collaboration dialog) are being shown 
	// modal, so it is limited in what can be shown if the 
	// administrator wants to follow its setup instructions during 
	// the setup of the user's Adapt It settings.
	
	// Set the frame size here - we cannot use InitDialog() below because
	// CHtmlFileViewer is not based on wxDialog, but wxFrame.
	this->SetSize(wxDefaultCoord,wxDefaultCoord,450,500);
}


CHtmlFileViewer::~CHtmlFileViewer() // destructor
{
	
}

/*
// NOTE: InitDialog is not called here because CHtmlFileViewer is derived from wxFrame, not from wxDialog	
void CHtmlFileViewer::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	

}
*/

// event handling functions

void CHtmlFileViewer::OnMoveBack(wxCommandEvent& WXUNUSED(event))
{
	pHtmlWindow->HistoryBack();
}

void CHtmlFileViewer::OnMoveForward(wxCommandEvent& WXUNUSED(event))
{
	pHtmlWindow->HistoryForward();
}

void CHtmlFileViewer::OnOpenHtmlFile(wxCommandEvent& WXUNUSED(event))
{
	wxFileName fn(adminHelpFilePath);
    wxString file = wxFileSelector(_("Open HTML document"), fn.GetPath(),
        wxEmptyString, wxEmptyString, wxT("HTML files|*.htm;*.html"));
	if (!file.IsEmpty())
		pHtmlWindow->LoadFile(wxFileName(file));
}

void CHtmlFileViewer::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	// Note: CHtmlFileViewer is modeless. When the user clicks on the close X box in upper
	// right hand corner of the dialog this handler is called.
	
	// Saves the size and position, fonts etc of the pHtmlFileViewer
	//pHtmlFileViewer->WriteCustomization(wxConfig::Get());
	//delete wxConfig::Set(NULL);

	// We must call destroy because the dialog is modeless.
	Destroy();
	//delete gpApp->m_pEarlierTransDlg; // BEW added 19Nov05, to prevent memory leak // No, this is harmful in wx!!!
	m_pApp->m_pHtmlFileViewer = NULL;
	//wxDialog::OnCancel(event); // we are running modeless so don't call the base class method
}


// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CHtmlFileViewer::OnOK(wxCommandEvent& event) 
{
	// sample code
	//wxListBox* pListBox;
	//pListBox = (wxListBox*)FindWindowById(IDC_LISTBOX_ADAPTIONS);
	//int nSel;
	//nSel = pListBox->GetSelection();
	//if (nSel == LB_ERR) // LB_ERR is #define -1
	//{
	//	wxMessageBox(_T("List box error when getting the current selection"), _T(""), wxICON_EXCLAMATION | wxOK);
	//}
	//m_projectName = pListBox->GetString(nSel);
	
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

