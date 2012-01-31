/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ClientServerConnection.h
/// \author			Bill Martin
/// \date_created	30 January 2012
/// \date_revised	30 January 2012
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the aiServer, aiClient and aiConnection classes. 
/// The aiServer class is used for listening to connection requests. The aiClient class allows Adapt It
/// to connect to another instance of Adapt It. The aiConnection class has the code allowing multiple
/// instances of Adapt It to communicate with each other.
/// \derivation		The aiServer class is derived from wxServer; the aiClient class is derived from wxClient
/// and the aiConnection class is derived from wxConnection.
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

class aiServer : public wxServer
{
public:
	aiServer(void); // constructor
	virtual ~aiServer(void); // destructor
	wxConnectionBase* OnAcceptConnection(const wxString& topic);
};

class aiClient : public wxClient
{
public:
	aiClient(void); // constructor
	virtual ~aiClient(void); // destructor
	wxConnectionBase* OnMakeConnection();
};

class aiConnection : public wxConnection
{
public:
	aiConnection(void); // constructor
	virtual ~aiConnection(void); // destructor
	bool OnExecute(const wxString& topic, wxChar* data, int size, wxIPCFormat format);
};

#endif /* ClientServerConnection_h */
