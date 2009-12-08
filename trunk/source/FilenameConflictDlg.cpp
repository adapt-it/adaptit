/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FilenameConflictDlg.cpp
/// \author			Bruce Waters
/// \date_created	8 December 2009
/// \date_revised	
/// \copyright		2009 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License 
///                 (see license directory)
/// \description	This is the implementation file for the FilenameConflictDlg class. 
/// The FilenameConflictDlg class provides a dialog interface for filename classes when
/// moving or copying a source folder's file to the destination folder where a file of the
/// same name already exists. It is modelled after the Windows dialog which performs a
/// similar set of choices for filename conflicts encountered from Win Explorer, though
/// the layout of the Adapt It version and some wordings for the options are a little
/// different. 
/// \derivation		The FilenameConflictDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////


// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "FilenameConflictDlg.h"
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
#include <wx/filename.h>
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
//#include <wx/valgen.h> // for wxGenericValidator
//#include <wx/imaglist.h> // for wxImageList
#include "Adapt_It.h"
#include "helpers.h" // it has the Get... functions for getting list of files, folders
					 // and optionally sorting
//#include "Adapt_It_wdr.h" // needed for the AIMainFrameIcons(index) function which returns
						  // for index values 10 and 11 the folder icon and file icon
						  // respectively, which we use in the two wxListCtrl instances to
						  // distinguish folder names from filenames; the function returns
						  // the relevant wxBitmap*
#include "FilenameConflictDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

// event handler table
BEGIN_EVENT_TABLE(FilenameConflictDlg, AIModalDialog)

	EVT_INIT_DIALOG(FilenameConflictDlg::InitDialog)
	EVT_BUTTON(wxID_OK, FilenameConflictDlg::OnBnClickedProceed)
	EVT_BUTTON(wxID_CANCEL, FilenameConflictDlg::OnBnClickedCancel)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_REPLACE, FilenameConflictDlg::OnBnClickedCopyAndReplace)	
	EVT_RADIOBUTTON(ID_RADIOBUTTON_NO_COPY, FilenameConflictDlg::OnBnClickedNoCopy)	
	EVT_RADIOBUTTON(ID_RADIOBUTTON_COPY_AND_RENAME, FilenameConflictDlg::OnBnClickedChangeNameAndCopy)	
	EVT_CHECKBOX(ID_CHECKBOX_HANDLE_SAME, FilenameConflictDlg::OnCheckboxHandleSameWay)	

END_EVENT_TABLE()

FilenameConflictDlg::FilenameConflictDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Resolve Filename Conflict"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	FilenameConflictFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_pAdminMoveOrCopy = (AdminMoveOrCopy*)parent; // establish link to parent dialog
	wxASSERT(m_pAdminMoveOrCopy != NULL);
	// point to the parent's source and destination folder paths (no path separator at end
	// of the path strings)
	m_pSrcFolderPath = &(*m_pAdminMoveOrCopy).m_strSrcFolderPath;
	m_pDestFolderPath = &(*m_pAdminMoveOrCopy).m_strDestFolderPath;
	// point to the string array of selected source folder filenames
	m_pSrcFileArray = &(*m_pAdminMoveOrCopy).srcSelectedFilesArray;
}

FilenameConflictDlg::~FilenameConflictDlg() // destructor
{
}

void FilenameConflictDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	// set these pointers up
	/*   ** TODO **
	wxRadioButton* m_pCopyAndReplaceRadioButton;
	wxRadioButton* m_pNoCopyRadioRadioButton;
	wxRadioButton* m_pChangeNameAndCopyRadioButton;
	wxCheckBox* m_pHandleSameWayCheckbox;
	wxButton* m_pProceedButton;
	wxButton* m_pCancelButton;
	wxTextCtrl* m_pSrcFileDataBox;
	wxTextCtrl* m_pDestFileDataBox;
	*/
	m_pCopyAndReplaceRadioButton = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_REPLACE);
	m_pNoCopyRadioRadioButton = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_NO_COPY);
	m_pChangeNameAndCopyRadioButton = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_COPY_AND_RENAME);
	m_pHandleSameWayCheckbox = (wxCheckBox*)FindWindowById(ID_CHECKBOX_HANDLE_SAME);
	m_pProceedButton = (wxButton*)FindWindowById(wxID_OK);
	m_pCancelButton = (wxButton*)FindWindowById(wxID_CANCEL);
	m_pSrcFileDataBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SOURCE_FILE_DETAILS);
	m_pDestFileDataBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_DESTINATION_FILE_DETAILS);

	// make the file data be displayed in the wxTextBox instances
	 
	// ** TODO **
}

void FilenameConflictDlg::OnBnClickedCopyAndReplace(wxCommandEvent& WXUNUSED(event))
{
}

void FilenameConflictDlg::OnBnClickedNoCopy(wxCommandEvent& WXUNUSED(event))
{
}

void FilenameConflictDlg::OnBnClickedChangeNameAndCopy(wxCommandEvent& WXUNUSED(event))
{
}

void FilenameConflictDlg::OnCheckboxHandleSameWay(wxCommandEvent& WXUNUSED(event))
{
}

void FilenameConflictDlg::OnBnClickedProceed(wxCommandEvent& WXUNUSED(event))
{
}

void FilenameConflictDlg::OnBnClickedCancel(wxCommandEvent& WXUNUSED(event))
{
}


