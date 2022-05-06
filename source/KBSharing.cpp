/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharing.h
/// \author			Bruce Waters
/// \date_created	14 January 2013
/// \rcs_id $Id: KBSharing.h 3025 2013-01-14 18:18:00Z jmarsden6@gmail.com $
/// \copyright		2013 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the KBSharing class.
/// The KBSharing class provides a dialog for the turning on or off KB Sharing, and for
/// controlling the non-automatic functionalities within the KB sharing feature.
/// \derivation		The KBSharing class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in KBSharing.cpp (in order of importance): (search for "TODO")
// 1.
//
// Unanswered questions: (search for "???")
// 1.
//
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KBSharing.h"
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

//#if defined(_KBSERVER)

// other includes
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/spinctrl.h>

#include "Adapt_It.h"
#include "KB.h"
#include "KbServer.h"
#include "KBSharing.h"
#include "Timer_KbServerChangedSince.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast
extern bool gbIsGlossing;

// event handler table
BEGIN_EVENT_TABLE(KBSharing, AIModalDialog)
	EVT_INIT_DIALOG(KBSharing::InitDialog)
	EVT_BUTTON(wxID_OK, KBSharing::OnOK)
	EVT_BUTTON(wxID_CANCEL, KBSharing::OnCancel)
	EVT_BUTTON(ID_GET_ALL, KBSharing::OnBtnGetAll)
	EVT_BUTTON(ID_GET_RECENT, KBSharing::OnBtnChangedSince)
	EVT_BUTTON(ID_SEND_ALL, KBSharing::OnBtnSendAll)
	EVT_RADIOBOX(ID_RADIO_SHARING_OFF, KBSharing::OnRadioOnOff)
	EVT_SPINCTRL(ID_SPINCTRL_RECEIVE, KBSharing::OnSpinCtrlReceiving)
END_EVENT_TABLE()

// NOTE: The update handler for "Controls For Knowledge Base Sharing"
// is OnUpdateKBSharingDlg() at line 554 of MainFrame.cpp line 2668

KBSharing::KBSharing(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Controls For Knowledge Base Sharing"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	kb_sharing_dlg_func(this, TRUE, TRUE);
	// The declaration is: GoToDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

    // whm 5Mar2019 Note: The kb_sharing_dlg_func() dialog now uses the wxStdDialogButtonSizer,
    // and so the ReverseOkCancelButtonsForMac() call below is not required.
	//bool bOK;
	//bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	//bOK = bOK; // avoid warning
	oldReceiveInterval = gpApp->m_nKbServerIncrementalDownloadInterval; // get the current value (minutes)
	receiveInterval = oldReceiveInterval; // initialize to the current value
}

KBSharing::~KBSharing() // destructor
{

}

void KBSharing::OnOK(wxCommandEvent& myevent)
{
	// update the receiving interval, if the user has changed it, and if changed, then
	// restart the timer with the new interval; but do this only provided the timer is
	// currently instantiated and is running (KB sharing might be temporarily disabled, or
	// enabled, we don't care which - and it doesn't matter as far as setting the interval
	// is concerned)
	receiveInterval = m_pSpinReceiving->GetValue();

	if (receiveInterval != oldReceiveInterval && m_pApp->m_pKbServerDownloadTimer != NULL 
		&& m_pApp->m_pKbServerDownloadTimer->IsRunning())
	{
		// The user has changed the interval setting (minutes), so store the new value
		m_pApp->m_nKbServerIncrementalDownloadInterval = receiveInterval;

		// restart the time with the new interval
		m_pApp->m_pKbServerDownloadTimer->Start(60000 * receiveInterval); // param is milliseconds
	}
	// Tell the app whether the user has kb sharing temporarily off, or not
	m_pApp->m_bKBSharingEnabled = bKBSharingEnabled;

	myevent.Skip();
}

void KBSharing::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_pApp = (CAdapt_ItApp*)&wxGetApp();

	m_pBtnGetAll = (wxButton*)FindWindowById(ID_GET_ALL);
	m_pRadioBox = (wxRadioBox*)FindWindowById(ID_RADIO_SHARING_OFF);
	m_pSpinReceiving = (wxSpinCtrl*)FindWindowById(ID_SPINCTRL_RECEIVE);
	// BEW 22Oct20 need to set min and max range, otherwise max defaults to 20
	m_pSpinReceiving->SetRange(1, 120); // values are minutes

	// initialize this; it applies to whatever KBserver(s) are open for business -
	// but only one at a time can be active, since glossing is a different mode
	// than adapting
	bKBSharingEnabled = TRUE;

	// get the current state of the two radio buttons
	KbServer* pKbSvr = NULL; //initialise
	if (gbIsGlossing)
	{
		pKbSvr = m_pApp->GetKbServer(2);
		if (pKbSvr == NULL)
		{
			// Setup a running KbSrvr instance ptr, of glossing type
			bool bSetupForGlossing = m_pApp->SetupForKBServer(2);
			wxUnusedVar(bSetupForGlossing);
		}
	}
	else
	{
		pKbSvr = m_pApp->GetKbServer(1);
		{
			if (pKbSvr == NULL)
			{
				// Setup a running KbSrvr instance ptr, of adapting type
				bool bSetupForAdapting = m_pApp->SetupForKBServer(1);
				wxUnusedVar(bSetupForAdapting);
			}
		}
	}
	m_nRadioBoxSelection = pKbSvr->IsKBSharingEnabled() ? 0 : 1;
	m_pRadioBox->SetSelection(m_nRadioBoxSelection);

	// update the 'save' boolean to whatever is the current state (user may 
	// Cancel and we would need to restore the initial state)
	bSaveKBSharingEnabled = m_nRadioBoxSelection == 0 ? TRUE: FALSE;
	bKBSharingEnabled = bSaveKBSharingEnabled;

	// initialize the spin control to the current value (from project config
	// file, or as recently changed by the user)
	receiveInterval = m_pApp->m_nKbServerIncrementalDownloadInterval;
	// put the value in the box
	if (m_pSpinReceiving != NULL)
	{
		m_pSpinReceiving->SetValue(receiveInterval);
	}
}

void KBSharing::OnCancel(wxCommandEvent& myevent)
{
	m_pApp->m_bKBSharingEnabled = bSaveKBSharingEnabled; 
	myevent.Skip();
}

void KBSharing::OnSpinCtrlReceiving(wxSpinEvent& WXUNUSED(event))
{
	receiveInterval = m_pSpinReceiving->GetValue();
	if (receiveInterval > 120)
		receiveInterval = 120;
	if (receiveInterval < 1)
		receiveInterval = 1;
	// units for the above are minutes; so multiply by 60,000 to get milliseconds
}

void KBSharing::OnRadioOnOff(wxCommandEvent& WXUNUSED(event))
{
	// We shouldn't be able to see the dlg if its not a KB sharing project,
	// let alone get this far!!
	wxASSERT(m_pApp->m_bIsKBServerProject || m_pApp->m_bIsGlossingKBServerProject);

	// Get the new state of the radiobox
	m_nRadioBoxSelection = m_pRadioBox->GetSelection();
	// make the KB sharing state match the new setting; both KbServer instances must be
	// changed in parallel
	if (m_pApp->m_bIsKBServerProject || m_pApp->m_bIsGlossingKBServerProject)
	{
        // respond only if this project is currently designated as one for supporting KB
        // sharing
		if (m_nRadioBoxSelection == 0)
		{
			// This is the first radio button, the one for sharing to be ON
			KbServer* pAdaptingSvr = m_pApp->GetKbServer(1);
			KbServer* pGlossingSvr = m_pApp->GetKbServer(2);
			if (pAdaptingSvr != NULL)
			{
				pAdaptingSvr->EnableKBSharing(TRUE);
			}
			if (pGlossingSvr != NULL)
			{
				pGlossingSvr->EnableKBSharing(TRUE);
			}
			bKBSharingEnabled = TRUE;
		}
		else
		{
			// This is the second radio button, the one for sharing to 
			// be (temporarily) OFF
			KbServer* pAdaptingSvr = m_pApp->GetKbServer(1);
			KbServer* pGlossingSvr = m_pApp->GetKbServer(2);
			if (pAdaptingSvr != NULL)
			{
				pAdaptingSvr->EnableKBSharing(FALSE);
			}
			if (pGlossingSvr != NULL)
			{
				pGlossingSvr->EnableKBSharing(FALSE);
			}
			bKBSharingEnabled = FALSE;
		}
	}
}

void KBSharing::OnBtnGetAll(wxCommandEvent& WXUNUSED(event))
{
	m_pApp->m_bUserRequestsTimedDownload = TRUE;
	KbServer* pKbServer;
	CKB* pKB = NULL; // it will be set either to m_pKB (the adapting KB) or m_pGlossingKB
	if (gbIsGlossing)
	{
		// work with the glossing entries only
		pKbServer = m_pApp->GetKbServer(2); // 2 indicates we deal with glosses
		// which CKB instance is now also determinate
		pKB = m_pApp->m_pGlossingKB;
		if (pKbServer != NULL)
		{
			// enum ClientAction:: getAll (= 2) means "do bulk download"
			pKbServer->DownloadToKB(pKB, getAll);
		}
		else
		{
			// The KbServer[1] instantiation for glossing entries either failed or has not yet been done,
			// tell the developer
			wxString msg = _T("OnBtnGetAll(): KbServer[1] is NULL, so instantiation for glossing entries either failed or has not yet been done.\nApp continues, but nothing was downloaded.");
			wxMessageBox(msg, _T("Error, no glossing KbServer instance available"), wxICON_ERROR | wxOK);
			m_pApp->LogUserAction(msg);
		}
	}
	else
	{
		// work with adaptations entries only
		pKbServer = m_pApp->GetKbServer(1); // 1 indicates we deal with adaptations
		// which CKB instance is now also determinate
		pKB = m_pApp->m_pKB; // the adaptations KB
		if (pKbServer != NULL)
		{
			// enum ClientAction:: getAll (= 2) means "do bulk download"
			pKbServer->DownloadToKB(pKB, getAll);
		}
		else
		{
			// The KbServer[0] instantiation for glossing entries either failed or has not yet been done,
			// tell the developer
			wxString msg = _T("OnBtnGetAll(): The KbServer[0] is NULL, so instantiation for adapting entries either failed or has not yet been done.\nApp continues, but nothing was downloaded.");
			wxMessageBox(msg, _T("Error, no adapting KbServer instance available"), wxICON_ERROR | wxOK);
			m_pApp->LogUserAction(msg);
		}
	}
	m_pApp->m_bUserRequestsTimedDownload = FALSE;
	// make the dialog close
	EndModal(wxID_OK);
}

void KBSharing::OnBtnChangedSince(wxCommandEvent& WXUNUSED(event))
{
	m_pApp->m_bUserRequestsTimedDownload = TRUE; // TRUE means "can do a timed download to local KB"
	KbServer* pKbServer;
	CKB* pKB = NULL; // it will be set either to m_pKB (the adapting KB) or m_pGlossingKB
	if (gbIsGlossing)
	{
		// work with the glossing entries only
		pKbServer = m_pApp->GetKbServer(2); // 2 indicates we deal with glosses
		// which CKB instance is now also determinate
		pKB = m_pApp->m_pGlossingKB;
		if (pKbServer != NULL)
		{
			// enum ClientAction:: changedSince means "do incremental download"
			pKbServer->DownloadToKB(pKB, changedSince);
		}
		else
		{
			// The KbServer[1] instantiation for glossing entries either failed or has not yet been done,
			// tell the developer
			wxString msg = _T("OnBtnChangedSince(): KbServer[1] is NULL, so instantiation for glossing entries either failed or has not yet been done.\nApp continues, but nothing was downloaded.");
			wxMessageBox(msg, _T("Error, no glossing KbServer instance available"), wxICON_ERROR | wxOK);
			m_pApp->LogUserAction(msg);
		}
	}
	else
	{
		// work with adaptations entries only
		pKbServer = m_pApp->GetKbServer(1); // 1 indicates we deal with adaptations
		// which CKB instance is now also determinate
		pKB = m_pApp->m_pKB; // the adaptations KB
		if (pKbServer != NULL)
		{
			// enum ClientAction:: changedSince ( = 1) means "do incremental download"
			pKbServer->DownloadToKB(pKB, changedSince);
		}
		else
		{
			// The KbServer[0] instantiation for glossing entries either failed or has not yet been done,
			// tell the developer
			wxString msg = _T("OnBtnChangedSince(): The KbServer[0] is NULL, so instantiation for adapting entries either failed or has not yet been done.\nApp continues, but nothing was downloaded.");
			wxMessageBox(msg, _T("Error, no adapting KbServer instance available"), wxICON_ERROR | wxOK);
			m_pApp->LogUserAction(msg);
		}
	}
	m_pApp->m_bUserRequestsTimedDownload = FALSE; // FALSE (default) means "cannot do a timed download to local KB"
	// make the dialog close
	EndModal(wxID_OK);
}

void KBSharing::OnBtnSendAll(wxCommandEvent& WXUNUSED(event))
{
	KbServer* pKbServer;

	pKbServer = m_pApp->GetKbServer((gbIsGlossing ? 2 : 1));

	pKbServer->UploadToKbServer();

	// make the dialog close (a good way to say, "it's been done")
	EndModal(wxID_OK);
}

//#endif
