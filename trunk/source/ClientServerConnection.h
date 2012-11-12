/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ClientServerConnection.h
/// \author			Bill Martin
/// \date_created	30 January 2012
/// \rcs_id $Id$
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the AI_Server, AI_Client and AI_Connection classes. 
/// The AI_Server class is used for listening to connection requests. The AI_Client class allows Adapt It
/// to connect to another instance of Adapt It. The AI_Connection class has the code allowing multiple
/// instances of Adapt It to communicate with each other.
/// \derivation		The AI_Server class is derived from wxServer; the AI_Client class is derived from wxClient
/// and the AI_Connection class is derived from wxConnection.
/////////////////////////////////////////////////////////////////////////////

#ifndef ClientServerConnection_h
#define ClientServerConnection_h

// wx docs say: "By default, the DDE implementation is used under Windows. DDE works within one computer only.
// If you want to use IPC between different workstations you should define wxUSE_DDE_FOR_IPC as 0 before
// including this header [<wx/ipc.h>]-- this will force using TCP/IP implementation even under Windows."
#ifdef useTCPbasedIPC
#define wxUSE_DDE_FOR_IPC 0
#endif
#include <wx/ipc.h> // for wxServer, wxClient and wxConnection

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ClientServerConnection.h"
#endif

class AI_Connection;

class AI_Server : public wxServer
{
public:
	AI_Server(void); // constructor
	virtual ~AI_Server(void); // destructor
    void Advise();
    bool CanAdvise();
	void Disconnect();
	bool IsConnected();
	AI_Connection* GetConnection();
	wxConnectionBase* OnAcceptConnection(const wxString& topic);
protected:
	AI_Connection* m_pConnection;
};

class AI_Client : public wxClient
{
public:
	AI_Client(void); // constructor
	virtual ~AI_Client(void); // destructor
	wxConnectionBase* OnMakeConnection();
};

class AI_Connection : public wxConnection
{
public:
	AI_Connection(void); // constructor
	virtual ~AI_Connection(void); // destructor
	// whm 9Jun12 removed OnRequest() below because it is not currently used and it has compile problems
	// in wxWidgets 2.9.3.
	//virtual wxChar* OnRequest(const wxString& topic, const wxString& item, int* size, wxIPCFormat format);
	virtual bool OnExecute(const wxString& topic, wxChar* data, int size, wxIPCFormat format);
    virtual bool Advise(const wxString& item, wxChar* data, int size = -1, wxIPCFormat format = wxIPC_TEXT);
    wxString m_strAdvise;
protected:
    wxString m_strRequestDate;
    char m_arrayRequestBytes[3];
};

#endif /* ClientServerConnection_h */
