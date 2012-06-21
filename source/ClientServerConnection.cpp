/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ClientServerConnection.cpp
/// \author			Bill Martin
/// \date_created	30 January 2012
/// \date_revised	30 January 2012
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the AI_Server class. 
/// The AI_Server class is used for listening to connection requests. The AI_Client class allows Adapt It
/// to connect to another instance of Adapt It. The AI_Connection class has the code allowing multiple
/// instances of Adapt It to communicate with each other.
/// \derivation		The AI_Server class is derived from wxServer; the AI_Client class is derived from wxClient
/// and the AI_Connection class is derived from wxConnection.
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

class AI_Connection;

AI_Server::AI_Server() // constructor
{
	m_pConnection = NULL;
}

AI_Server::~AI_Server() // destructor
{
	Disconnect();
}

void AI_Server::Advise()
{
	// whm 2Feb12 Note: The following data is not currently used in Adapt It. This is only
	// some sample data that shows how AI_Server::Advise() could be used.
/*
    if (CanAdvise())
    {
        wxString testStr = wxDateTime::Now().Format();
        m_pConnection->Advise(m_pConnection->m_strAdvise, (wxChar *)testStr.c_str());
        testStr = wxDateTime::Now().FormatTime() + _T(" ") + wxDateTime::Now().FormatDate();
        m_pConnection->Advise(m_pConnection->m_strAdvise, (wxChar *)testStr.c_str(), (testStr.Length() + 1) * sizeof(wxChar));
    }
*/
}

bool AI_Server::CanAdvise()
{
	return m_pConnection != NULL && !m_pConnection->m_strAdvise.IsEmpty(); 
}

wxConnectionBase* AI_Server::OnAcceptConnection(const wxString& topic)
{
	const wxString name = wxString::Format(_T("Adapt_ItApp-%s"), wxGetUserId().c_str());
	if (!topic.IsEmpty())
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
		return new AI_Connection();
	}
	else
		return NULL;
}

AI_Connection* AI_Server::GetConnection()
{
	return m_pConnection;
}

void AI_Server::Disconnect()
{
	if (m_pConnection)
	{
		m_pConnection->Disconnect();
		delete m_pConnection;
		m_pConnection = (AI_Connection*)NULL;
	}
}

bool AI_Server::IsConnected()
{
	return m_pConnection != NULL;
}

AI_Client::AI_Client() // constructor
{
}

AI_Client::~AI_Client() // destructor
{
}

wxConnectionBase* AI_Client::OnMakeConnection()
{
	return new AI_Connection;
}


AI_Connection::AI_Connection() // constructor
{
}

AI_Connection::~AI_Connection() // destructor
{
}

bool AI_Connection::OnExecute(const wxString& topic, wxChar* data, int size, wxIPCFormat format)
{
	CMainFrame* m_pFrame = wxDynamicCast(wxGetApp().GetTopWindow(),CMainFrame);
	wxString dataStr(data);
	if (dataStr.StartsWith(_T("[Raise]")))
    {
        if (m_pFrame)
        {
			wxString str;
			if (format == wxIPC_TEXT)
				str = _T("wxIPC_TEXT");
			else if (format == wxIPC_UNICODETEXT)
				str = _T("wxIPC_UNICODETEXT");
			wxLogDebug(_T("Main Frame Raised: topic = %s data = %s size = %d format = %s"), topic.c_str(), data, size, str.c_str());
            m_pFrame->Raise();
        }
        return TRUE;
    }
	// whm Note: Other else if () tests could go here for other actions to execute.
	// We don't invoke Adapt It and pass it a filename to load via the "data" invocation.
	// If we did, we could handle that execution here by parsing out of the data string the
	// passed-in filename along with its execute info, i.e., in which the incoming data is
	// something like "[Open][filename]".
	
	wxString str;
	if (format == wxIPC_TEXT)
		str = _T("wxIPC_TEXT");
	else if (format == wxIPC_UNICODETEXT)
		str = _T("wxIPC_UNICODETEXT");
	wxLogDebug(_T("OnRequest: topic = %s data = %s size = %d format = %s"), topic.c_str(), data, size, str.c_str());
	return FALSE;
}
	
	/*
wxChar* AI_Connection::OnRequest(const wxString& topic, const wxString& item, int* size, wxIPCFormat format)
{
	// whm 2Feb12 Note: The following data is not currently used in Adapt It. This is only
	// some sample data that shows how OnRequest() could be used.
    wxChar *data;
    if (item == _T("Date"))
    {
        m_strRequestDate = wxDateTime::Now().Format();
        data = (wxChar *)m_strRequestDate.c_str();
        *size = -1;
    }    
    else if (item == _T("Date+len"))
    {
        m_strRequestDate = wxDateTime::Now().FormatTime() + _T(" ") + wxDateTime::Now().FormatDate();
        data = (wxChar *)m_strRequestDate.c_str();
        *size = (m_strRequestDate.Length() + 1) * sizeof(wxChar);
    }    
    else
    {
        data = NULL;
        *size = 0;
    }
	wxString str;
	if (format == wxIPC_TEXT)
		str = _T("wxIPC_TEXT");
	else if (format == wxIPC_UNICODETEXT)
		str = _T("wxIPC_UNICODETEXT");
	wxLogDebug(_T("OnRequest: topic = %s item = %s data = %s size = %d format = %s"), topic.c_str(), item.c_str(), data, *size, str.c_str());
	return data;
}
*/

bool AI_Connection::Advise(const wxString& item, wxChar* data, int size, wxIPCFormat format)
{
	// whm 2Feb12 Note: The following data is not currently used in Adapt It. This is only
	// some sample data that shows how AI_Connection::Advise() could be used.
	wxString str;
	if (format == wxIPC_TEXT)
		str = _T("wxIPC_TEXT");
	else if (format == wxIPC_UNICODETEXT)
		str = _T("wxIPC_UNICODETEXT");
	wxLogDebug(_T("Advise: item = %s data = %s size = %d format = %s"), item.c_str(), data, size, str.c_str());
    return wxConnection::Advise(item, data, size, format);

}

// other class methods

