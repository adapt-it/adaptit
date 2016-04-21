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

#ifndef Thread_ServiceDiscovery_h
#define Thread_ServiceDiscovery_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "Thread_ServiceDiscovery.h"
#endif

#if defined(_KBSERVER)

// forward declarations
class wxThread;
class CServiceDiscovery;
class wxServDisc;
class CAdapt_ItApp;

class Thread_ServiceDiscovery : public wxThread
{
public:
	Thread_ServiceDiscovery();
	virtual ~Thread_ServiceDiscovery(void); // destructor

	unsigned long processID;
	

	// keep it simple, forget accessors, just make variables public
	CAdapt_ItApp*		m_pApp; // to access ptr to KB, and ptr to KbServer

	CServiceDiscovery*  m_pServDisc; // create here with new, so that onSDNotify() does
										// not degrade responsiveness of main thread

	// other methods...

	// wxThread::OnExit() is called when the thread exits at termination - for self
	// destruction termination or by Delete(), but not if Kill() is used - the latter
	// should never be used, it can leave resources in an indeterminate state
	virtual void		OnExit();

	// This must be defined to have our work content - this is where thread execution
	// begins, and when the work is done, OnExit() is called and the thread destroys
	// itself. Do not try to poll the state of this type of thread; if state info is
	// needed, use the main thread (ie. Adapt It GUI)to PostEvent() or AddPendingEvents()
	// or wxQueueEvent() if running under wx later than 2.9 as a way to pass signals back
	// to the calling (main) thread.  But for this class we will use events to clean up
	// the wxServDisc instances, and pass results to the app, and then give the task
	//of cleaning up and destroying the CServiceDiscovery instance to this thread's
	// OnExit() function.
	virtual void*	Entry();

	bool TestDestroy();

	protected:

	void EndServiceDiscovery();

	private:
	
};

#endif // for _KBSERVER

#endif
