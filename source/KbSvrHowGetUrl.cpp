/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KbSvrHowGetUrl.cpp
/// \author			Bruce Waters
/// \date_created	25 January 2016
/// \rcs_id $Id: KbSvrHowGetUrl.cpp 3028 2016-01-25 11:38:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the KbSvrHowGetUrl class.
/// The KbSvrHowGetUrl class provides a dialog for letting the user decide whether 
/// to do service discovery on the local LAN to find a running KBserver, or to 
/// manually type in a URL he knows (e.g. for a KBserver running on the web, or
/// on a different subnet of the LAN, where service discovery would be of no benefit).
/// The dialog consists of a wxRadioBox (two radio buttons), an OK button and a
/// Cancel button. The same controls are also in the KbSharingSetup.h and .cpp
/// files, and the KbSvrHowGetUrl.h and .cpp files are just a cut-down version
/// of that dialog. 
/// This dialog is used in the ProjectPage of the wizard, and for access to the
/// KB Sharing Manager (tabbed dialog), and in the HookUpToExistingAIProject in
/// the CollabUtilities.cpp file. It is not used in KbSharingSetup.cpp because
/// the same controls are already built in to the latter dialog.
/// \derivation		The KbSvrHowGetUrl class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
///
// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KbSvrHowGetUrl.h"
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

#if defined(_KBSERVER)

// other includes
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include "Adapt_It.h"
#include "MainFrm.h"
#include "helpers.h"
#include "KbSvrHowGetUrl.h"

// event handler table
BEGIN_EVENT_TABLE(KbSvrHowGetUrl, AIModalDialog)
	EVT_INIT_DIALOG(KbSvrHowGetUrl::InitDialog)
	EVT_BUTTON(wxID_OK, KbSvrHowGetUrl::OnOK)
	EVT_BUTTON(wxID_CANCEL, KbSvrHowGetUrl::OnCancel)
END_EVENT_TABLE()

KbSvrHowGetUrl::KbSvrHowGetUrl(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Specify how to get the URL"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

    // This dialog function below is generated in wxDesigner, and defines the controls and
    // sizers for the dialog. The first parameter is the parent which is the pointer this.
    // (Warning, do not use the frame window as the parent - the dialog will show empty and
    // the frame window will resize down to the dialog's size!) The second and third
    // parameters should both be TRUE to utilize the sizers and create the right size
    // dialog.
	m_pApp = &wxGetApp();
	kb_ask_how_get_url_func(this, TRUE, TRUE);
	// The declaration is: functionname( wxWindow *parent, bool call_fit, bool set_sizer );
	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	wxUnusedVar(bOK);

	m_bServiceDiscWanted = TRUE; // initialize
 }

KbSvrHowGetUrl::~KbSvrHowGetUrl() // destructor
{
}

void KbSvrHowGetUrl::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_pBtnOK = (wxButton*)FindWindowById(wxID_OK); // we don't actually use this
	wxUnusedVar(m_pBtnOK);
	m_pRadioBoxHow = (wxRadioBox*)FindWindowById(ID_RADIOBOX_HOW);
	m_nRadioBoxSelection = 0; // top button selected
	m_pRadioBoxHow->SetSelection(m_nRadioBoxSelection);
	m_pTimeout = (wxSpinCtrl*)FindWindowById(ID_SPINCTRL_TIMEOUT);

	// Set the spin control to the current value (divide by 1000 since units are 
	// thousandths of a sec for basic config file storage of the value)
	int value = m_pApp->m_KBserverTimeout / 1000;
	if (value < 4)
	{
		value = 4;
	}
	if (value > 120)
	{
		value = 120;
	}
	m_pTimeout->SetValue(value);

	m_bServiceDiscWanted = m_pApp->m_bServiceDiscoveryWanted;
	m_bUserClickedCancel = FALSE;
}

void KbSvrHowGetUrl::OnOK(wxCommandEvent& myevent)
{
	int nRadioBoxSelection = m_pRadioBoxHow->GetSelection();
	m_bServiceDiscWanted = nRadioBoxSelection == 0 ? TRUE : FALSE;
	m_pApp->m_bServiceDiscoveryWanted = m_bServiceDiscWanted; // Set app member, OnIdle's call
			// of AuthenticateCheckAndSetupKBSharing() will use it, and reset to TRUE afterwards

	// Whether the user wants service discovery or not, if he changes the timeout value, the
	// honour the change. The spin control limits are already (in wxDesigner) set to [8,30] 
	// range with 16 recommended. Small values, eg less than 10, are unlikely to work.
	int value = m_pTimeout->GetValue() * 1000;
	if (m_pApp->m_KBserverTimeout != value)
	{
		m_pApp->m_KBserverTimeout = value;
	}

	m_bUserClickedCancel = FALSE;
	myevent.Skip(); // close the KbSharingSetup dialog
}

void KbSvrHowGetUrl::OnCancel(wxCommandEvent& myevent)
{
	// Cancelling from this dialog should, unlike the KbSharingSetup::OnCancel() handler,
	// remove the sharing state, and clobber the classes KbServer[0] and KbServer[1],
	// because the assumption is, where this is called, that earlier sharing is to be
	// re-setup, and so Cancel has to remove the setup otherwise the app state would be
	// internally inconsistent
	m_bUserClickedCancel = TRUE;

	// Remove any setup
	m_pApp->m_bIsKBServerProject = FALSE;
	m_pApp->m_bIsGlossingKBServerProject = FALSE;
	m_pApp->ReleaseKBServer(1); // the adaptations one
	m_pApp->ReleaseKBServer(2); // the glossings one

	myevent.Skip();  // close dialog
}

#endif
