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

#define _shutdown_   // comment out to disable wxLogDebug() calls related to shutting down service discovery

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
#include "Thread_ServiceDiscovery.h"

extern CServiceDiscovery* gpServiceDiscovery;

extern wxMutex	kbsvr_arrays;

Thread_ServiceDiscovery::Thread_ServiceDiscovery() :wxThread(wxTHREAD_JOINABLE)
{
	m_pApp = &wxGetApp();
	m_pApp->GetMainFrame()->nEntriesToEndServiceDiscovery = 0;
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
	wxPostEvent(m_pApp->GetMainFrame(), sd_eventCustom); // the CMainFrame hanndler does Wait()
						// then delete of thread, and sets member ptr m_pServDiscThread to NULL
#if defined (_shutdown_)
	wxLogDebug(_T(" Thread_ServiceDiscovery::OnExit() is ending. Delayed .2 sec & then posted event wxEVT_End_ServiceDiscovery to MainFrame "));
#endif
}

void* Thread_ServiceDiscovery::Entry()
{
	//wxLogDebug(_T("G'day, I'm on a thread now - life is quieter here..."));

	processID = wxGetProcessId();
#if defined (_shutdown_)
	wxLogDebug(_T("\nThread_ServiceDiscovery() Entry: I am process ID = %lx"), processID);
#endif

	m_pApp->ServDiscBackground(); // internally it scans for: _kbserver._tcp.local.

	// Keep the thread alive until all the work gets done. The app member boolean
	// m_bServiceDiscoveryThreadCanDie will be set TRUE at the end of onSDHalting()
	// and TestDestroy() polls the app for that value going true. TestDestroy() also
	// requires that the CServicDiscovery booleans m_bNamescanIsEnded and
	// m_bAddrscanIsEnded are BOTH TRUE as a condition for returning TRUE. So, control
	// is delayed here until all three of the above booleans have gone TRUE. After they
	// do, Entry() then finishes and OnExit()  is called. In the start of OnExit() is
	// a .2 second time delay, which guarantees that both namescan() and addrscan() are
	// well and truly dead and gone, before the rest of the class instances are destroyed
	while (!TestDestroy()){	}

	EndServiceDiscovery(); // does everything except delete this thread itself

#if defined (_shutdown_)
	wxLogDebug(_T("Thread_ServiceDiscovery::Entry() is finishing now & returning NULL, EndServiceDiscovery() was just called"));
#endif
	return (void*)NULL;
}

// TestDestroy is virtual, so my override will be called
bool Thread_ServiceDiscovery::TestDestroy()
{
	if (m_pApp->m_bServiceDiscoveryThreadCanDie)
	{
		// The two booleans in the next test must be TRUE before TestDestroy() can return
		// a TRUE value, because the child wxServDisc instances, namescan and addrscan must
		// have died before we can delete the wxServDisc instance which is their parent,
		// and the higher level classes too - the CServiceDiscovery instance, and this
		// thread which runs it/them.
		if (m_pServDisc->m_bNamescanIsEnded && m_pServDisc->m_bAddrscanIsEnded)
		{
			wxMilliSleep(200); //  .2 sec sleep before getting the final shutdown moving
			return TRUE;
		}
	}
	return FALSE;
}

//void  Thread_ServiceDiscovery::OnCustomEventEndServiceDiscovery(wxCommandEvent& WXUNUSED(event))
void  Thread_ServiceDiscovery::EndServiceDiscovery()
{
	m_pApp->GetMainFrame()->nEntriesToEndServiceDiscovery++;
#if defined(_shutdown_)
	wxLogDebug(_T("\n thread:: EndServiceDiscovery() just entered, nEntriesToEndServiceDiscovery = %d"),
		m_pApp->GetMainFrame()->nEntriesToEndServiceDiscovery);
#endif
	// BEW 20Apr16, When I had multiple timed service discovery runs working, I connected to one of the
	// three KBservers that I had running, and after doing so, I again clicked the Setup Or Remove KB Sharing
	// dialog, and immediately got a thread.cpp access violation -- logging showed that control had been in
	// this function (the above log message was in the Output window) and after that was the access violation.
	// The violation happens because the following calls removed the code that a running instance of wxServDisc,
	// either namescan() or addrscan(), needed in order to destroy itself. So... what to do? I tried a short
	// timeout here - didn't help, I could do several Setup... dialog accesses, but eventually one of them
	// gives the access violation crash. As it is potentially a timings issue, and currently this function is
	// on the main thread and therefore would be impinged by user activity, I'll try moving this shutdown stuff
	// to Thread_ServiceDiscovery()

	m_pServDisc->m_serviceStr.Clear();

	m_pServDisc->ShutdownWxServDisc(this->m_pServDisc->m_pWxSD);

	delete m_pServDisc; // delete the manager class for the wxServDisc instance

	gpServiceDiscovery = NULL;

#if defined(_shutdown_)
	wxLogDebug(_T(" thread:: EndServiceDiscovery() finished: CServiceDiscovery & owned wxServDisc are gone, gpServiceDiscovery NULL"));
#endif
}

#endif // for _KBSERVER

