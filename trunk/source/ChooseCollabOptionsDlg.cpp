/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ChooseCollabOptionsDlg.cpp
/// \author			Bill Martin
/// \date_created	18 February 2012
/// \date_revised	18 February 2012
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CChooseCollabOptionsDlg class. 
/// The CChooseCollabOptionsDlg class implements a 3-radio button dialog that allows the user to
/// choose to Turn Collaboration ON, Turn Collaboration OFF, or Turn Read-Only Mode ON. This
/// dialog is called from the ProjectPage of the Start Working wizard if the project just
/// opened is one that has previously been setup to collaborate with Paratext/Bibledit.
/// \derivation		The CChooseCollabOptionsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ChooseCollabOptionsDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ChooseCollabOptionsDlg.h"
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
#include "Adapt_It.h"
#include "ChooseCollabOptionsDlg.h"

// event handler table
BEGIN_EVENT_TABLE(CChooseCollabOptionsDlg, AIModalDialog)
	EVT_INIT_DIALOG(CChooseCollabOptionsDlg::InitDialog)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_TURN_COLLAB_ON, CChooseCollabOptionsDlg::OnRadioTurnCollabON)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_TURN_COLLAB_OFF, CChooseCollabOptionsDlg::OnRadioTurnCollabOFF)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_READ_ONLY_MODE, CChooseCollabOptionsDlg::OnRadioReadOnlyON)
	EVT_BUTTON(wxID_OK, CChooseCollabOptionsDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, CChooseCollabOptionsDlg::OnCancel)
END_EVENT_TABLE()

CChooseCollabOptionsDlg::CChooseCollabOptionsDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Choose How You Want To Work With This Project"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	ChooseCollabOptionsDlgFunc(this, TRUE, TRUE);
	// The declaration is: ChooseCollabOptionsDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	wxColour sysColorBtnFace; // color used for read-only text controls displaying
	// color used for read-only text controls displaying static text info button face color
	sysColorBtnFace = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	
	pRadioTurnCollabON = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_TURN_COLLAB_ON);
	wxASSERT(pRadioTurnCollabON != NULL);

	pRadioTurnCollabOFF = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_TURN_COLLAB_OFF);
	wxASSERT(pRadioTurnCollabOFF != NULL);

	pRadioTurnReadOnlyON = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_READ_ONLY_MODE);
	wxASSERT(pRadioTurnReadOnlyON != NULL);

	pStaticTextAIProjName = (wxStaticText*)FindWindowById(ID_TEXT_SELECTED_AI_PROJECT);
	wxASSERT(pStaticTextAIProjName != NULL);

	pStaticAsTextCtrlTurnCollabOn = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_NOTE_COLLAB_ON);
	wxASSERT(pStaticAsTextCtrlTurnCollabOn != NULL);
	pStaticAsTextCtrlTurnCollabOn->SetBackgroundColour(sysColorBtnFace);

	pStaticAsTextCtrlTurnCollabOff = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_NOTE_COLLAB_OFF);
	wxASSERT(pStaticAsTextCtrlTurnCollabOff != NULL);
	pStaticAsTextCtrlTurnCollabOff->SetBackgroundColour(sysColorBtnFace);

	pStaticAsTextCtrlTurnReadOnlyOn = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_NOTE_READ_ONLY_ON);
	wxASSERT(pStaticAsTextCtrlTurnReadOnlyOn != NULL);
	pStaticAsTextCtrlTurnReadOnlyOn->SetBackgroundColour(sysColorBtnFace);

}

CChooseCollabOptionsDlg::~CChooseCollabOptionsDlg() // destructor
{
	
}

void CChooseCollabOptionsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	
	// substitute "Paratext" or "Bibledit" for the %s in the first radio button
	wxString tempStr;
	tempStr = pRadioTurnCollabON->GetLabel();
	m_bEditorIsAvailable = TRUE;
	if (!m_pApp->m_bParatextIsInstalled && !m_pApp->m_bBibleditIsInstalled)
	{
		tempStr = _("%s is NOT INSTALLED on this computer. No collaboration is possible");
		m_bEditorIsAvailable = FALSE;
	}
	tempStr = tempStr.Format(tempStr,m_pApp->m_collaborationEditor.c_str());
	pRadioTurnCollabON->SetLabel(tempStr);

	tempStr = pRadioTurnCollabOFF->GetLabel();
	tempStr = tempStr.Format(tempStr,m_pApp->m_collaborationEditor.c_str());
	pRadioTurnCollabOFF->SetLabel(tempStr);
	
	tempStr = pStaticAsTextCtrlTurnCollabOff->GetValue();
	tempStr = tempStr.Format(tempStr,m_pApp->m_collaborationEditor.c_str(),m_pApp->m_collaborationEditor.c_str());
	pStaticAsTextCtrlTurnCollabOff->ChangeValue(tempStr);

	// Initialize the radio buttons with the collaboration setting last used which 
	// is in the App's m_bCollaboratingWithParatext or m_bCollaboratingWithBibledit
	// members (kept in the project config file which was read before invoking this
	// dialog). If no collaboration editor is installed, force the ON radio button
	// to be unselected and disabled
	
	
	if (m_bEditorIsAvailable)
	{
		// PT/BE is installed so allow set the radio button appropriately
		if (m_pApp->m_bCollaboratingWithParatext || m_pApp->m_bCollaboratingWithBibledit)
		{
			m_bRadioSelectCollabON = TRUE; // the caller may change this depending on what was selected previously
			m_bRadioSelectCollabOFF = FALSE; // the caller may change this depending on what was selected previously
		}
		else
		{
			m_bRadioSelectCollabON = FALSE; // the caller may change this depending on what was selected previously
			m_bRadioSelectCollabOFF = TRUE; // the caller may change this depending on what was selected previously
		}
	}
	else // m_bEditorIsAvailable is FALSE
	{
		// Neither PT nor BE is installed so disallow setting the first radio button and
		// disable it (its label was changed above in this case)
		m_pApp->m_bCollaboratingWithParatext = FALSE;
		m_pApp->m_bCollaboratingWithBibledit = FALSE;
		m_bRadioSelectCollabON = FALSE; // the caller may change this depending on what was selected previously
		m_bRadioSelectCollabOFF = TRUE; // the caller may change this depending on what was selected previously
		pRadioTurnCollabON->Disable();
		
	}
	// Always initialize the read-only button to false
	m_bRadioSelectReadOnlyON = FALSE; // the read-only radio button always starts OFF/FALSE
	pRadioTurnCollabON->SetValue(m_bRadioSelectCollabON);
	pRadioTurnCollabOFF->SetValue(m_bRadioSelectCollabOFF);
	pRadioTurnReadOnlyON->SetValue(m_bRadioSelectReadOnlyON);
	pStaticTextAIProjName->SetLabel(m_aiProjName);
}

// event handling functions

void CChooseCollabOptionsDlg::OnRadioTurnCollabON(wxCommandEvent& WXUNUSED(event))
{
	// handle the event
	m_bRadioSelectCollabON = TRUE;
	m_bRadioSelectCollabOFF = FALSE;
	m_bRadioSelectReadOnlyON = FALSE;
	pRadioTurnCollabON->SetValue(m_bRadioSelectCollabON);
	pRadioTurnCollabOFF->SetValue(m_bRadioSelectCollabOFF);
	pRadioTurnReadOnlyON->SetValue(m_bRadioSelectReadOnlyON);
}

void CChooseCollabOptionsDlg::OnRadioTurnCollabOFF(wxCommandEvent& WXUNUSED(event))
{
	// handle the event
	m_bRadioSelectCollabON = FALSE;
	m_bRadioSelectCollabOFF = TRUE;
	m_bRadioSelectReadOnlyON = FALSE;
	pRadioTurnCollabON->SetValue(m_bRadioSelectCollabON);
	pRadioTurnCollabOFF->SetValue(m_bRadioSelectCollabOFF);
	pRadioTurnReadOnlyON->SetValue(m_bRadioSelectReadOnlyON);
}

void CChooseCollabOptionsDlg::OnRadioReadOnlyON(wxCommandEvent& WXUNUSED(event))
{
	// handle the event
	// whm Note: When the Read-Only radio button is selected, we force collaboration
	// off so that the wizard will appear.
	m_bRadioSelectCollabON = FALSE;
	m_bRadioSelectCollabOFF = TRUE;
	m_bRadioSelectReadOnlyON = TRUE;
	pRadioTurnCollabON->SetValue(m_bRadioSelectCollabON);
	pRadioTurnCollabOFF->SetValue(m_bRadioSelectCollabOFF);
	pRadioTurnReadOnlyON->SetValue(m_bRadioSelectReadOnlyON);
}

void CChooseCollabOptionsDlg::OnCancel(wxCommandEvent& event)
{
	event.Skip();
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CChooseCollabOptionsDlg::OnOK(wxCommandEvent& event) 
{
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

// other class methods

