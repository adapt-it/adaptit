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
	// Put here the kbserver code for a changedsince from an early time
	KbServer* pKbServer;
	wxString timestamp = _T("1920-01-01 00:00:00"); // earlier than everything!
	if (gbIsGlossing)
	{
		// work with the glossing entries only
		pKbServer = m_pApp->GetKbServer(2); // 2 indicates we deal with glosses
		if (pKbServer != NULL)
		{
			int rv = pKbServer->ChangedSince(timestamp);
			if (rv != 0)
			{
				// there was a cURL error, display it
				wxString msg;
				msg = msg.Format(_T("glossing mode, ChangedSince(): error code returned: %d  Nothing was downloaded, application continues."), rv);
				wxMessageBox(msg, _T("ChangedSince() failed"), wxICON_ERROR | wxOK);
				m_pApp->LogUserAction(msg);
			}
			else
			{
				// the download was successful, so use the results to update the
				// adaptations KB contents
				m_pApp->m_pKB->StoreEntriesFromKbServer(pKbServer);
			}
		}
		else
		{
			// The KbServer[1] instantiation for glossing entries either failed or has not yet been done,
			// tell the developer
			wxString msg = _T("ChangedSince(): NULL returned. The KbServer[1] instantiation for glossing entries either failed or has not yet been done.\nApp continues, but nothing was downloaded.");
			wxMessageBox(msg, _T("Error, no KbServer instance available"), wxICON_ERROR | wxOK);
			m_pApp->LogUserAction(msg);
		}
	}
	else
	{
		// work with adaptations entries only
		pKbServer = m_pApp->GetKbServer(1); // 1 indicates we deal with adaptations
		if (pKbServer != NULL)
		{
			int rv = pKbServer->ChangedSince(timestamp);
			if (rv != 0)
			{
				// there was a cURL error, display it
				wxString msg;
				msg = msg.Format(_T("adapting mode, ChangedSince(): error code returned: %d  Nothing was downloaded, application continues."), rv);
				wxMessageBox(msg, _T("ChangedSince() failed"), wxICON_ERROR | wxOK);
				m_pApp->LogUserAction(msg);
			}
			else
			{
				// the download was successful, so use the results to update the
				// adaptations KB contents
				m_pApp->m_pKB->StoreEntriesFromKbServer(pKbServer);
			}
		}
		else
		{
			// The KbServer[0] instantiation for glossing entries either failed or has not yet been done,
			// tell the developer
			wxString msg = _T("OnBtnGetAll(): NULL returned. The KbServer[0] instantiation for adapting entries either failed or has not yet been done.\nApp continues, but nothing was downloaded.");
			wxMessageBox(msg, _T("Error, no KbServer instance available"), wxICON_ERROR | wxOK);
			m_pApp->LogUserAction(msg);
		}
	}
	// make the dialog close
	EndModal(wxID_OK);
}



