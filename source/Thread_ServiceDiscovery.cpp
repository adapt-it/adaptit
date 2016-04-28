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
/// by a timer, and works in a burst of 9 consecutive discovery runs. Enough to find several
/// running KBservers. The burst is initiated when the app is first launched. A menu choice
/// will allow the user to request another burst whenever he wants. Each burst occupies about
/// 1.5 minutes. 
/// This Thread_ServiceDiscovery thread is instantiated at each notify from the
/// m_servDiscTimer wxTimer, and when it runs, it creates a CServiceDiscovery instance which
/// acts as the parent of the initial wxServDisc instance it creates (in its creator). wxServDisc
/// runs as a detached thread, it it spawns several instances of itself. It runs only a short
/// time (3 plus 1 second for overheads, approx), finds one KBserver's hostname, ip address (and port)
/// if there is at least one running on the LAN. Then it destroys itself after notifying the
/// parent class, CServiceDiscovery, to send any results to an array on the application, for the
/// ConnectUsingDiscoveryResults() function there to display results to the user, for making a connection.
/// The Thread_ServiceDiscovery thread, however, is "joinable" type;  which is easier for controlling
/// shutdown behaviour.
/// \derivation		The Thread_ServiceDiscovery class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////


// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "Thread_ServiceDiscovery.h"
#endif

#if defined(_KBSERVER)

//#define _shutdown_   // comment out to disable wxLogDebug() calls related to shutting down service discovery

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
#include "wx/utils.h"

// other includes
#include "Adapt_It.h"
#include "ServiceDiscovery.h"
#include "MainFrm.h"
#include "StatusBar.h"
#include "Thread_ServiceDiscovery.h"

extern wxMutex	kbsvr_arrays;

//Thread_ServiceDiscovery::Thread_ServiceDiscovery() :wxThread(wxTHREAD_JOINABLE) <<-- Wait() failed, m_thread was released, 
// so wx treated it as detached, so I'll do so explicitly
Thread_ServiceDiscovery::Thread_ServiceDiscovery() : wxThread()
{
	m_pApp = &wxGetApp();
	m_pApp->GetMainFrame()->nEntriesToEndServiceDiscovery = 0; // used just for logging support

	m_indexOfRun = m_pApp->m_nSDRunCounter; // on the app, it is not yet augmented, so is a correct index value
											// and we augment it to count this instance at the start of Entry()

	wxString progTitle = _("KBservers Discovery");
	wxString msgDisplayed;
	int nTotal = m_pApp->m_numServiceDiscoveryRuns;
	wxString progMsg = _("KBservers? Search number %d of %d");
	msgDisplayed = progMsg.Format(progMsg, m_indexOfRun + 1, nTotal);
	CStatusBar *pStatusBar = (CStatusBar*)m_pApp->GetMainFrame()->m_pStatusBar;
	pStatusBar->UpdateProgress(progTitle, m_indexOfRun + 1, msgDisplayed);

#if defined (_shutdown_)
	wxLogDebug(_T("\nThread_ServiceDiscovery() CREATOR: I am %p"), this);
#endif
}

Thread_ServiceDiscovery::~Thread_ServiceDiscovery()
{
	processID = wxGetProcessId();
#if defined (_shutdown_)
	wxLogDebug(_T(" Thread_ServiceDiscovery::~Thread_ServiceDiscovery() destructor is finishing now. Process ID = %lx "), processID);
#endif
}

void Thread_ServiceDiscovery::OnExit()
{
	// Delay for .2 sec to guarantee that namescan() and addrscan() are dead (to avoid
	// an access violation crash)
	wxMilliSleep(200);

	wxCommandEvent sd_eventCustom(wxEVT_End_ServiceDiscovery);
	sd_eventCustom.SetExtraLong((long)m_indexOfRun); // So CMainFrame's handler knows which ptr in m_pServDiscThread[] to set to NULL
	wxPostEvent(m_pApp->GetMainFrame(), sd_eventCustom); // the CMainFrame hanndler does Wait()
						// then delete of thread, and sets member ptr m_pServDiscThread to NULL
#if defined (_shutdown_)
	wxLogDebug(_T(" Thread_ServiceDiscovery  %p  (106) OnExit() is ending. Delayed .2 sec & posted event wxEVT_End_ServiceDiscovery to MainFrame "),
				this );
#endif
}

void* Thread_ServiceDiscovery::Entry()
{
	// Count this run, and pass the value back to the app's m_nSDRunCounter to be used
	// by the next run as its index
	m_pApp->m_nSDRunCounter = m_indexOfRun + 1;

	processID = wxGetProcessId();
#if defined (_shutdown_)
	wxLogDebug(_T("\nThread_ServiceDiscovery() %p  (119)  Entry(): I am process ID = %lx , and run with index = %d"), 
		this, processID, m_indexOfRun);
#endif

	m_pApp->ServDiscBackground(m_indexOfRun); // internally it scans for: _kbserver._tcp.local.

	// Keep the thread alive until all the work gets done, control will not pass the
	// next line until TestDestroy() returns TRUE
	while (!TestDestroy())
	{
		// It can sleep a bit beween checks
		wxMilliSleep(100); // .1 seconds between each test
	}
	// Give the child threads ample time to get themselves shut down; they need 
	// gpServDisc global pointer to the CServiceDiscovery instance to remain
	// valid for .3 seconds plus however much time the heap cleanups require.
	// So give them ample time - .5 sec should be enough, since CServiceDiscovery's
	// destructor has the job of Delete()ing the owned wxServDisc instance
	wxMilliSleep(500); // .5 sec

	m_pServDisc->m_serviceStr.Clear();
#if defined(_shutdown_)
	wxLogDebug(_T(" thread  %p   End of Entry() m_serviceStr cleared"), this);
#endif

	delete m_pServDisc; // delete the manager class for the wxServDisc instance
#if defined(_shutdown_)
	wxLogDebug(_T(" thread  %p  End of Entry()  m_pServDisc deleted (ie. no CServiceDiscovery)"), this);
#endif
	return (void*)NULL;
}

// TestDestroy is virtual, so my override will be called
bool Thread_ServiceDiscovery::TestDestroy()
{
	if (m_pServDisc->m_bDestroyChildren)
	{
		return TRUE;
	}
	return FALSE;
}

#endif // for _KBSERVER

