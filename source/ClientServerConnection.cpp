/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ClientServerConnection.cpp
/// \author			Bill Martin
/// \date_created	30 January 2012
/// \date_revised	30 January 2012
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the aiServer class. 
/// The aiServer class is used for listening to connection requests. The aiClient class allows Adapt It
/// to connect to another instance of Adapt It. The aiConnection class has the code allowing multiple
/// instances of Adapt It to communicate with each other.
/// \derivation		The aiServer class is derived from wxServer; the aiClient class is derived from wxClient
/// and the aiConnection class is derived from wxConnection.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ClientServerConnection.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ClientServerConnection.h"
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
#include <wx/dialog.h>

#include "Adapt_It.h"
#include "MainFrm.h"
#include "ClientServerConnection.h"

// wx docs say: "By default, the DDE implementation is used under Windows. DDE works within one computer only.
// If you want to use IPC between different workstations you should define wxUSE_DDE_FOR_IPC as 0 before
// including this header [<wx/ipc.h>]-- this will force using TCP/IP implementation even under Windows."
#ifdef useTCPbasedIPC
#define wxUSE_DDE_FOR_IPC 0
#endif
#include <wx/ipc.h> // for wxServer, wxClient and wxConnection

class aiConnection;

aiServer::aiServer() // constructor
{
}

aiServer::~aiServer() // destructor
{
}

wxConnectionBase* aiServer::OnAcceptConnection(const wxString& topic)
{
	const wxString name = wxString::Format(_T("Adapt_ItApp-%s"), wxGetUserId().c_str());
	name.Lower();
	if (topic.Lower() == name)
	{
		// Check that there are no modal dialogs active
		wxWindowList::Node* node = wxTopLevelWindows.GetFirst();
		while (node)
		{
			wxDialog* dialog = wxDynamicCast(node->GetData(), wxDialog);
			if (dialog && dialog->IsModal())
			{
				return FALSE;
			}
			node = node->GetNext();
		}
		return new aiConnection();
	}
	else
		return NULL;
}

aiClient::aiClient() // constructor
{
}

aiClient::~aiClient() // destructor
{
}

wxConnectionBase* aiClient::OnMakeConnection()
{
	return new aiConnection;
}


aiConnection::aiConnection() // constructor
{
}

aiConnection::~aiConnection() // destructor
{
}

bool aiConnection::OnExecute(const wxString& WXUNUSED(topic), wxChar* data, int WXUNUSED(size), wxIPCFormat WXUNUSED(format))
{
	CMainFrame* frame = wxDynamicCast(wxGetApp().GetTopWindow(),CMainFrame);
	wxString filename(data);
	// whm Note: In Adapt It we don't pass a filename from the other instance so filename
	// will always be an empty string, but I will leave the code here as an example of how
	// a program would use OnExecute() and pass a filename string to cause the other instance
	// to handle/open that file.
	if (filename.IsEmpty())
	{
		// raise the main window
		if (frame)
			frame->Raise();
	}
	return TRUE;
}

// other class methods

