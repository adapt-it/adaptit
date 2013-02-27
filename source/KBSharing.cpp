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

#if defined(_KBSERVER)

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
	//EVT_SPINCTRL(ID_SPINCTRL_SEND, KBSharing::OnSpinCtrlSending)
END_EVENT_TABLE()


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
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
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
	if (receiveInterval != oldReceiveInterval && 
		m_pApp->m_pKbServerDownloadTimer != NULL &&
		m_pApp->m_pKbServerDownloadTimer->IsRunning())
	{
		// The user has changed the interval setting (minutes), so store the new value
		m_pApp->m_nKbServerIncrementalDownloadInterval = receiveInterval;

		// restart the time with the new interval
		m_pApp->m_pKbServerDownloadTimer->Start(60000 * receiveInterval); // param is milliseconds
	}
	myevent.Skip();
}

void KBSharing::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_pApp = (CAdapt_ItApp*)&wxGetApp();

	m_pBtnGetAll = (wxButton*)FindWindowById(ID_GET_ALL);
	m_pRadioBox = (wxRadioBox*)FindWindowById(ID_RADIO_SHARING_OFF);
	m_pSpinReceiving = (wxSpinCtrl*)FindWindowById(ID_SPINCTRL_RECEIVE);
	//m_pSpinSending = (wxSpinCtrl*)FindWindowById(ID_SPINCTRL_SEND);

	// get the current state of the two radio buttons
	KbServer* pAdaptingSvr = m_pApp->GetKbServer(1); // both are in same state, so this one is enough
	m_nRadioBoxSelection = pAdaptingSvr->IsKBSharingEnabled() ? 0 : 1;
	m_pRadioBox->SetSelection(m_nRadioBoxSelection);

	// initialize the spin controls to the current values (from project config file, or as
	// recently changed by the user)
	receiveInterval = m_pApp->m_nKbServerIncrementalDownloadInterval;
	//sendInterval = m_pApp->m_nKbServerIncrementalUploadInterval;
	// put the values in the boxes
	if (m_pSpinReceiving != NULL)
	{
		m_pSpinReceiving->SetValue(receiveInterval);
	}
	// BEW deprecated 11Feb13
	//if (m_pSpinSending != NULL)
	//{
	//	m_pSpinSending->SetValue(sendInterval);
	//}
}

void KBSharing::OnCancel(wxCommandEvent& myevent)
{
	myevent.Skip();
}

void KBSharing::OnSpinCtrlReceiving(wxSpinEvent& WXUNUSED(event))
{
	receiveInterval = m_pSpinReceiving->GetValue();
	if (receiveInterval > 10)
		receiveInterval = 10;
	if (receiveInterval < 1)
		receiveInterval = 1;
	// units for the above are minutes; so multiply by 60,000 to get milliseconds
}

void KBSharing::OnRadioOnOff(wxCommandEvent& WXUNUSED(event))
{
	// We shouldn't be able to see the dlg if its not a KB sharing project,
	// let alone get this far!!
	wxASSERT(m_pApp->m_bIsKBServerProject == TRUE);

	// Get the new state of the radiobox
	m_nRadioBoxSelection = m_pRadioBox->GetSelection();
	// make the KB sharing state match the new setting; both KbServer instances must be
	// changed in parallel
	if (m_pApp->m_bIsKBServerProject)
	{
        // respond only if this project is currently designated as one for supporting KB
        // sharing
		if (m_nRadioBoxSelection == 0)
		{
			// This is the first button, the one for sharing to be ON
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
		}
		else
		{
			// This is the second button, the one for sharing to be OFF
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
		}
	}
}

void KBSharing::OnBtnGetAll(wxCommandEvent& WXUNUSED(event))
{
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
	// make the dialog close
	EndModal(wxID_OK);
}

void KBSharing::OnBtnChangedSince(wxCommandEvent& WXUNUSED(event))
{
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
	// make the dialog close
	EndModal(wxID_OK);
}

void KBSharing::OnBtnSendAll(wxCommandEvent& WXUNUSED(event))
{
	KbServer* pKbServer;

	pKbServer = m_pApp->GetKbServer((gbIsGlossing ? 2 : 1));

	// BEW comment 31Jan13
	// I think that temporaly long operations like uploading a whole KB or downloading the
	// server's contents for a given project should not be done on a work thread. My
	// reasoning is the following... The dialog will close (or if EndModal() below is
	// omitted in this handler, the dialog will become responsive again) and the user may
	// think that the upload or download is completed, when in fact a separate process may
	// have a few minutes to run before it completes. The user, on closure of the dialog,
	// may exit the project, or shut down the application - either of which will destroy
	// the code resources which the thread is relying on to do its work. To avoid this
	// kind of problem, long operations should be done synchronously, and be tracked by
	// the progress indicator at least - and they should close the dialog when they complete.
	pKbServer->UploadToKbServer();
	//pKbServer->UploadToKbServerThreaded(); // <<-- repurpose this later, see 31Jan13 comment above
	
	// make the dialog close (a good way to say, "it's been done"
	EndModal(wxID_OK);
}

#endif
