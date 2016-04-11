/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_ServiceDiscovery.h
/// \author			Bruce Waters
/// \date_created	18 February 2013
/// \rcs_id $Id: Thread_ServiceDiscovery.h 3101 2013-02-18 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the Thread_ServiceDiscovery class.
/// The Thread_ServiceDiscovery is a thread class for KB Sharing/Syncing support, specifically,
/// a Zeroconf-based service discovery module that listens for _kbserver._tcp.local. multicasts
/// from one or more KBserver servers running on the LAN. The service discovery is on, governed
/// by a timer, for the life of the application's session, but a GUI choice (in Preferences) 
/// allows the user to turn it off if unneeded. This thread is instantiated at each notify from
/// the m_servDiscTimer wxTimer, and when it runs, it creates a CServiceDiscovery instance which
/// acts as the parent of the initial wxServDisc instance it creates (in its creator). wxServDisc
/// runs as a detached thread, it it spawns several instances of itself. It runs only a short
/// time (5 plus 1 second for overheads), finds one KBserver's hostname, ip address (and port)
/// if there is at least one running on the LAN. Then it destroys itself after notifying the
/// parent class, CServiceDiscovery, to send any results to an array on the application, for the
/// DoServiceDiscovery() function there to display results to the user, for making a connection.
/// The thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes.
/// \derivation		The Thread_ServiceDiscovery class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////


// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "Thread_ServiceDiscovery.h"
#endif

#if defined(_KBSERVER)

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#include "wx/thread.h"

// other includes
#include "Adapt_It.h"
#include "ServiceDiscovery.h"
#include "Thread_ServiceDiscovery.h"

extern wxMutex	kbsvr_arrays;

Thread_ServiceDiscovery::Thread_ServiceDiscovery() :wxThread()
{
	m_pApp = &wxGetApp();
}

Thread_ServiceDiscovery::~Thread_ServiceDiscovery()
{
	wxLogDebug(_T(" Thread_ServiceDiscovery::~Thread_ServiceDiscovery() destructor is finishing now "));
}

void Thread_ServiceDiscovery::OnExit()
{
	wxLogDebug(_T(" Thread_ServiceDiscovery::OnExit() is finishing now "));
}

void* Thread_ServiceDiscovery::Entry()
{
	wxLogDebug(_T("Hello again, I'm on a thread now - life is quieter here..."));

	m_pApp->ServDiscBackground(); // internally it scans for: _kbserver._tcp.local.

	// Keep the thread alive until all the work gets done. The app member boolean
	// m_bServiceDiscoveryThreadCanDie will be set TRUE at the end of onSDHalting()
	// and TestDestroy() polls the app for that value going true
	while (!TestDestroy())
	{
		wxMilliSleep(500); // sleep thread for a half second
	}

	wxLogDebug(_T("Thread_ServiceDiscovery::Entry() is finishing now, returning NULL"));

	delete m_pApp->m_pServDisc;
	m_pApp->m_pServDisc = NULL;

	return (void*)NULL;
}

// TestDestroy is virtual, so my override will be called
bool Thread_ServiceDiscovery::TestDestroy()
{
	if (m_pApp->m_bServiceDiscoveryThreadCanDie)
	{
		return TRUE;
	}
	return FALSE;
}

#endif // for _KBSERVER

