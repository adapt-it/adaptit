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
#include "Adapt_It.h"
#include "KB.h"
#include "KbServer.h"
#include "KBSharing.h"

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

}

KBSharing::~KBSharing() // destructor
{

}

void KBSharing::OnOK(wxCommandEvent& myevent)
{
	myevent.Skip();
}

void KBSharing::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_pApp = (CAdapt_ItApp*)&wxGetApp();

	m_pBtnGetAll = (wxButton*)FindWindowById(ID_GET_ALL);

}

void KBSharing::OnCancel(wxCommandEvent& myevent)
{
	myevent.Skip();
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

	pKbServer->UploadToKbServer();
	//pKbServer->UploadToKbServerThreaded();
}

#endif
