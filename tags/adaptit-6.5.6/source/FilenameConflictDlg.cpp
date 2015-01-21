/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FilenameConflictDlg.cpp
/// \author			Bruce Waters
/// \date_created	8 December 2009
/// \rcs_id $Id$
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
	EVT_BUTTON(wxID_OK, FilenameConflictDlg::OnBnClickedClose)
	EVT_BUTTON(wxID_CANCEL, FilenameConflictDlg::OnBnClickedCancel)
//	EVT_RADIOBUTTON(ID_RADIOBUTTON_REPLACE, FilenameConflictDlg::OnBnClickedCopyAndReplace)	
//	EVT_RADIOBUTTON(ID_RADIOBUTTON_NO_COPY, FilenameConflictDlg::OnBnClickedNoCopy)	
//	EVT_RADIOBUTTON(ID_RADIOBUTTON_COPY_AND_RENAME, FilenameConflictDlg::OnBnClickedChangeNameAndCopy)	
	EVT_CHECKBOX(ID_CHECKBOX_HANDLE_SAME, FilenameConflictDlg::OnCheckboxHandleSameWay)	

END_EVENT_TABLE()

FilenameConflictDlg::FilenameConflictDlg(wxWindow* parent,
		wxString* pConflictingFilename,
		wxString* pSrcFolderPath ,
		wxString* pDestFolderPath) // dialog constructor
	: AIModalDialog(parent, -1, _("Resolve Filename Conflict"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	FilenameConflictFunc(this, TRUE, TRUE);
	// The declaration is: functionname( wxWindow *parent, bool call_fit, bool set_sizer );
	
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp != NULL);
	bool bOK;
	bOK = pApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	
	m_pAdminMoveOrCopy = (AdminMoveOrCopy*)parent; // establish link to parent dialog
	wxASSERT(m_pAdminMoveOrCopy != NULL);
	// point to the parent's source and destination folder paths (no path separator at end
	// of the path strings)
	m_pSrcFolderPath = pSrcFolderPath;
	m_pDestFolderPath = pDestFolderPath;

	srcDetailsStr.Empty();
	destDetailsStr.Empty();

	srcFilename = *pConflictingFilename;
	destFilename = *pConflictingFilename;
}

FilenameConflictDlg::~FilenameConflictDlg() // destructor
{
}

void FilenameConflictDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	// set these pointers up
	m_pCopyAndReplaceRadioButton = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_REPLACE);
	m_pNoCopyRadioRadioButton = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_NO_COPY);
	m_pChangeNameAndCopyRadioButton = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_COPY_AND_RENAME);
	m_pHandleSameWayCheckbox = (wxCheckBox*)FindWindowById(ID_CHECKBOX_HANDLE_SAME);
	m_pProceedButton = (wxButton*)FindWindowById(wxID_OK);
	m_pCancelButton = (wxButton*)FindWindowById(wxID_CANCEL);
	m_pSrcFileDataBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SOURCE_FILE_DETAILS);
	m_pDestFileDataBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_DESTINATION_FILE_DETAILS);
	m_pNameChangeText = (wxStaticText*)FindWindowById(ID_TEXT_MODIFY_NAME);

	bSameWayValue = FALSE; // default
	m_pHandleSameWayCheckbox->SetValue(FALSE);

	// make the file data be displayed in the wxTextBox instances
	wxString newlineStr = _T("\n");
	srcDetailsStr = srcFilename;
	destDetailsStr = destFilename;
	wxString locationLineSrc = *m_pSrcFolderPath;
	wxString locationLineDest = *m_pDestFolderPath;
	srcDetailsStr += newlineStr + locationLineSrc + _T("\\");
	destDetailsStr += newlineStr + locationLineDest + _T("\\");
	
	wxFileName srcFN(*m_pSrcFolderPath,srcFilename);
	wxFileName destFN(*m_pDestFolderPath,destFilename);

	wxULongLong srcSize = srcFN.GetSize();
	wxULongLong destSize = destFN.GetSize();
	wxString srcSizeStr;
	wxString destSizeStr;
	srcSizeStr = srcFN.GetHumanReadableSize(srcSize,_T("0"),2);
	destSizeStr = srcFN.GetHumanReadableSize(destSize,_T("0"),2);

	srcDetailsStr += newlineStr + srcSizeStr;
	destDetailsStr += newlineStr + destSizeStr;

	wxDateTime srcModTime = srcFN.GetModificationTime();
	wxDateTime destModTime = destFN.GetModificationTime();
	wxString strNewer = _("(newer)");
	wxString strOlder = _("(older)");
	wxString strSameTime = _("(same time and date)");
	wxString strLastMod = _("Last modified: ");
	wxString srcDateStr = srcModTime.FormatDate();
	wxString destDateStr = destModTime.FormatDate();
	wxString srcTimeStr = srcModTime.FormatTime();
	wxString destTimeStr = destModTime.FormatTime();
	wxString srcTimeLineStr;
	wxString destTimeLineStr;
	if (srcModTime.IsEarlierThan(destModTime))
	{
		// source modification time is < destination modification time
		srcTimeLineStr = newlineStr + strLastMod + _T("  ") + srcDateStr + _T("  ") + 
							srcTimeStr + _T("  ") + strOlder;
		destTimeLineStr = newlineStr + strLastMod + _T("  ") + destDateStr + _T("  ") + 
							destTimeStr + _T("  ") + strNewer;
	}
	else if (srcModTime.IsLaterThan(destModTime))
	{
		// source modification time is > destination modification time
		srcTimeLineStr = newlineStr + strLastMod + _T("  ") + srcDateStr + _T("  ") + 
							srcTimeStr + _T("  ") + strNewer;
		destTimeLineStr = newlineStr + strLastMod + _T("  ") + destDateStr + _T("  ") + 
							destTimeStr + _T("  ") + strOlder;
	}
	else
	{
		// source modification time equals destination modification time
		srcTimeLineStr = newlineStr + strLastMod + _T("  ") + srcDateStr + _T("  ") + 
							srcTimeStr + _T("  ") + strSameTime;
		destTimeLineStr = newlineStr + strLastMod + _T("  ") + destDateStr + _T("  ") + 
							destTimeStr + _T("  ") + strSameTime;
	}
	srcDetailsStr += srcTimeLineStr;
	destDetailsStr += destTimeLineStr;

	// now insert it into the boxes
	m_pSrcFileDataBox->ChangeValue(srcDetailsStr);
	m_pDestFileDataBox->ChangeValue(destDetailsStr);

	// compute the modified filename's name for the last option's message
	wxString strNewDestFilename = m_pAdminMoveOrCopy->BuildChangedFilenameForCopy(&destFilename);
	wxString label = m_pNameChangeText->GetLabel();
	strNewDestFilename = strNewDestFilename.Format(label,strNewDestFilename.c_str());
	m_pNameChangeText->SetLabel(strNewDestFilename);

	// make both boxes read only, now that their data is inserted
	m_pSrcFileDataBox->SetEditable(FALSE);
	m_pDestFileDataBox->SetEditable(FALSE);
	 
}
/*
void FilenameConflictDlg::OnBnClickedCopyAndReplace(wxCommandEvent& WXUNUSED(event))
{


}

void FilenameConflictDlg::OnBnClickedNoCopy(wxCommandEvent& WXUNUSED(event))
{


}

void FilenameConflictDlg::OnBnClickedChangeNameAndCopy(wxCommandEvent& WXUNUSED(event))
{


}
*/
void FilenameConflictDlg::OnCheckboxHandleSameWay(wxCommandEvent& WXUNUSED(event))
{
	// give the new value to the caller
	//m_pAdminMoveOrCopy->m_bDoTheSameWay = m_pHandleSameWayCheckbox->GetValue();
	bSameWayValue = m_pHandleSameWayCheckbox->GetValue();
}

void FilenameConflictDlg::OnBnClickedClose(wxCommandEvent& event)
{
	// return the appropriate copyType value for the enum CopyAction
	if (m_pNoCopyRadioRadioButton->GetValue())
		m_pAdminMoveOrCopy->copyType = noCopy;
	else if (m_pChangeNameAndCopyRadioButton->GetValue())
		m_pAdminMoveOrCopy->copyType = copyWithChangedName;
	else
		// one button must be checked, but if someone none are, then don't
		// copy so as to give minimal risk of loss of data
		m_pAdminMoveOrCopy->copyType = 
			m_pCopyAndReplaceRadioButton->GetValue() ? copyAndReplace : noCopy;
	event.Skip();
}

void FilenameConflictDlg::OnBnClickedCancel(wxCommandEvent& event)
{
	event.Skip();
}


