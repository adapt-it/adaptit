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

#include <wx/wizard.h> // for wxWizard

// other includes
#include "Adapt_It.h"
#include "ChooseCollabOptionsDlg.h"
#include "CollabUtilities.h"

/// This global is defined in Adapt_It.cpp.
extern CChooseCollabOptionsDlg* pChooseCollabOptionsDlg; 

// event handler table
BEGIN_EVENT_TABLE(CChooseCollabOptionsDlg, AIModalDialog)
	EVT_INIT_DIALOG(CChooseCollabOptionsDlg::InitDialog)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_TURN_COLLAB_ON, CChooseCollabOptionsDlg::OnRadioTurnCollabON)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_TURN_COLLAB_OFF, CChooseCollabOptionsDlg::OnRadioTurnCollabOFF)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_READ_ONLY_MODE, CChooseCollabOptionsDlg::OnRadioReadOnlyON)
	EVT_BUTTON(ID_BUTTON_TELL_ME_MORE, CChooseCollabOptionsDlg::OnBtnTellMeMore)
	EVT_BUTTON(wxID_OK, CChooseCollabOptionsDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, CChooseCollabOptionsDlg::OnCancel)
END_EVENT_TABLE()

CChooseCollabOptionsDlg::CChooseCollabOptionsDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Choose How You Want To Work With This Project"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP) // whm 13Jul12 added wxSTAY_ON_TOP
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pChooseCollabOptionsDlgSizer = ChooseCollabOptionsDlgFunc(this, FALSE, TRUE); // second param FALSE enables resize
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

	pStaticAsTextCtrlNotInstalledErrorMsg = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_NOT_INSTALLED_ERROR_MSG);
	wxASSERT(pStaticAsTextCtrlNotInstalledErrorMsg != NULL);
	pStaticAsTextCtrlNotInstalledErrorMsg->SetBackgroundColour(sysColorBtnFace);

	pStaticTextAIProjName = (wxStaticText*)FindWindowById(ID_TEXT_SELECTED_AI_PROJECT);
	wxASSERT(pStaticTextAIProjName != NULL);

	pBtnTellMeMore = (wxButton*)FindWindowById(ID_BUTTON_TELL_ME_MORE);
	wxASSERT(pBtnTellMeMore != NULL);

	pBtnOK = (wxButton*)FindWindowById(wxID_OK);
	wxASSERT(pBtnOK != NULL);

}

CChooseCollabOptionsDlg::~CChooseCollabOptionsDlg() // destructor
{
	
}

void CChooseCollabOptionsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	
	pChooseCollabOptionsDlg = this;
	
	// substitute "Paratext" or "Bibledit" for the %s in the first radio button
	wxString tempStr;
	tempStr = pRadioTurnCollabON->GetLabel();
	m_bEditorIsAvailable = TRUE;
	if (!m_pApp->m_bParatextIsInstalled && !m_pApp->m_bBibleditIsInstalled)
	{
		tempStr = _("%s is NOT INSTALLED on this computer. No collaboration is possible");
		m_bEditorIsAvailable = FALSE;
	}
	wxASSERT(!m_pApp->m_collaborationEditor.IsEmpty());
	tempStr = tempStr.Format(tempStr,m_pApp->m_collaborationEditor.c_str());
	pRadioTurnCollabON->SetLabel(tempStr);

	tempStr = pRadioTurnCollabOFF->GetLabel();
	tempStr = tempStr.Format(tempStr,m_pApp->m_collaborationEditor.c_str());
	pRadioTurnCollabOFF->SetLabel(tempStr);
	
	// Start with the text control for error messages hidden
	pStaticAsTextCtrlNotInstalledErrorMsg->Hide();

	// We need to call the App's GetListOfPTProjects() or GetListOfBEProjects() to ensure that the
	// App's m_pArrayOfCollabProjects is populated before the CollabProjectsAreValid() call below
	// which itself calls CollabProjectHasAtLeastOneBook() which utilizes the m_pArrayOfCollabProjects
	// member.
	// get list of PT/BE projects
	projList.Clear();
	if (m_pApp->m_collaborationEditor == _T("Paratext"))
	{
		projList = m_pApp->GetListOfPTProjects(); // as a side effect, it populates the App's m_pArrayOfCollabProjects
	}
	else if (m_pApp->m_collaborationEditor == _T("Bibledit"))
	{
		projList = m_pApp->GetListOfBEProjects(); // as a side effect, it populates the App's m_pArrayOfCollabProjects
	}
	// In the event that a project folder was copied to the current computer that
	// defined collaboration with Paratext/Bibledit, it is possible that the
	// Paratext/Bibledit project used in that project have not yet been created
	// on the receiving computer.
	// 
	// Do some sanity checks to ensure that the Paratext/Bibledit projects are
	// properly setup for collaboration. We do the same checks here that are done
	// within the CSetupEditorCollaboration class's "Select from List" button
	// handlers, which call CollabProjectHasAtLeastOneBook() for PT/BE projects
	// that are defined in the project config file. If that function returns FALSE
	// for any of the PT/BE projects we disable the "Turn Collaboration ON" button 
	// and change "Note:" text to indicate why it is disabled, i.e., "Target 
	// project (%s) does not have any books created in it.", etc.
	
	// The caller of the ChooseCollabOptionsDlg should have ensured that the source
	// and target projects were defined
	wxASSERT(!m_pApp->m_CollabProjectForSourceInputs.IsEmpty());
	wxASSERT(!m_pApp->m_CollabProjectForTargetExports.IsEmpty());
	
	// whm 5Mar12 Note: Check for a valid PT/BE projects for obtaining source texts,
	// for storing translations, and (optionally) for storing free translations.
	// We check to see if that project does not have any books created, in which case, 
	// we disable the "Turn Collaboration ON" button, and display a message that 
	// indicates the reason for the error.
	wxString errorStr = _T("");
	wxString errProj = _T("");
	if (!CollabProjectsAreValid(m_pApp->m_CollabProjectForSourceInputs, m_pApp->m_CollabProjectForTargetExports, 
							m_pApp->m_CollabProjectForFreeTransExports, m_pApp->m_collaborationEditor, errorStr, errProj))
	{
		pStaticAsTextCtrlNotInstalledErrorMsg->Show(TRUE); // make it visible
		wxString msg;
		msg = _("COLLABORATION DISABLED! - invalid %s projects detected. Ask your administrator for help:%s");
		msg = msg.Format(msg,m_pApp->m_collaborationEditor.c_str(),errorStr.c_str());
		// Note: the returned errProj string is unused here
		
		// set the default to collaboratin OFF
		pRadioTurnCollabON->SetValue(FALSE);
		pRadioTurnCollabOFF->SetValue(TRUE);
		// disable the "Turn collaboration on" radio button
		pRadioTurnCollabON->Disable();
		// change the Note in the text control under the first button
		pStaticAsTextCtrlNotInstalledErrorMsg->ChangeValue(msg);
		pStaticAsTextCtrlNotInstalledErrorMsg->Update();

		// In this case we don't clear out the App's variables but leave them set even though
		// they don't describe valid PT/BE projects. Administrator will have to correct in the
		// SetupEditorCollaboration dialog.
	}

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
		m_bRadioSelectCollabON = FALSE;
		m_bRadioSelectCollabOFF = TRUE;
		pRadioTurnCollabON->Disable();
		
	}
	// Always initialize the read-only button to false
	m_bRadioSelectReadOnlyON = FALSE; // the read-only radio button always starts OFF/FALSE
	pRadioTurnCollabON->SetValue(m_bRadioSelectCollabON);
	pRadioTurnCollabOFF->SetValue(m_bRadioSelectCollabOFF);
	pRadioTurnReadOnlyON->SetValue(m_bRadioSelectReadOnlyON);
	pStaticTextAIProjName->SetLabel(m_aiProjName);

	// ensure focus is on the OK button.
	pBtnOK->SetFocus();

	pChooseCollabOptionsDlgSizer->Layout();
	// The second radio button's label text is likely going to be truncated unless we resize the
	// dialog to fit it. Note: The constructor's call of ChooseCollabOptionsDlgFunc(this, FALSE, TRUE)
	// has its second parameter as FALSE to allow this resize here in InitDialog().
	wxSize dlgSize;
	dlgSize = pChooseCollabOptionsDlgSizer->ComputeFittingWindowSize(this);
	this->SetSize(dlgSize);
	this->CenterOnParent();
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

void CChooseCollabOptionsDlg::OnBtnTellMeMore(wxCommandEvent& WXUNUSED(event))
{
	wxString msg = _("Option 1: Work with my %s Scripture texts (Collaboration on).\nWhen you enter the \"%s\" project With collaboration turned on, only scripture books and chapters will be visible for opening and doing work. Any other documents created outside of collaboration will not be available for work until you turn collaboration off.\n\n");
	msg += _("Option 2: Work with other Adapt It texts (%s texts not available: Collaboration off).\nWhen you enter the \"%s\" project with collaboration turned off, any document files used when collaborating with %s will be hidden. You can still work with scripture files, or any other type of text file, but you must set them up for adaptation yourself. Their data will not be exchanged with %s.\n\n");
	msg += _("Option 3: Read-only mode (All texts accessible but not editable - I'm an advisor or consultant).\nWhen you enter the \"%s\" project in \"read-only\" mode, \"read-only\" means that you can open and read the project's data, but you cannot make any changes to the documents.");
	msg = msg.Format(msg,m_pApp->m_collaborationEditor.c_str(),m_aiProjName.c_str(),
					m_pApp->m_collaborationEditor.c_str(),m_aiProjName.c_str(),m_pApp->m_collaborationEditor.c_str(),m_pApp->m_collaborationEditor.c_str(),
					m_aiProjName.c_str());
	wxMessageBox(msg, _T(""), wxICON_INFORMATION | wxOK);
}



