/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_ChangedSince.h
/// \author			Bruce Waters
/// \date_created	12 February 2013
/// \rcs_id $Id: Thread_ChangedSince.h 3092 2013-02-12 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the Thread_ChangedSince class.
/// The Thread_ChangedSince is a "detached" thread class for downloading a bunch of
/// database entries from kbserver, to be merged to the local KB. The download is
/// incremental, that is, those entries added remotely since the time of the last
/// ChangedSince() type of download - whether done by a thread, or manually
/// (synchronously). This thread downloads to a queue, the latter is protected by a mutex,
/// and the main thread at each idle even removes the frontmost struct and merges it's
/// contents to the local KB. This thread does not "see" the latter, it only sees the end
/// of the queue and adds each struct (actually, ptr to struct) to its end.
/// The thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes.
/// \derivation		The Thread_ChangedSince class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

#ifndef Thread_ChangedSince_h
#define Thread_ChangedSince_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Thread_ChangedSince.h"
#endif

#if defined(_KBSERVER)

// forward declarations
class wxThread;
class CAdapt_ItApp;
class CKB;
//class KbServer;

class Thread_ChangedSince : public wxThread
{
public:
	Thread_ChangedSince(); // creator
	virtual ~Thread_ChangedSince(void); // destructor

	// keep it simple, forget accessors, just make variables public
	// hmmm... I don't think I need any of these, except m_pKbSvr in order to grab the
	// timestamp of the last changedsince() type of server access
	CAdapt_ItApp*		m_pApp; // to access ptr to KB, and ptr to KbServer
	//CKB*				m_pKB; // to access the currently active KB instance
	KbServer*			m_pKbSvr; // set it to the currently active instance
	//int				m_kbServerType;
	//wxString			m_source;
	//wxString			m_translation;

	// other methods...

	// wxThread::OnExit() is called when the thread exits at termination - for self
	// destruction termination or by Delete(), but not if Kill() is used - the latter
	// should never be used, it can leave resources in an indeterminate state
	virtual void		OnExit();

	// This must be defined to have our work content - this is where thread execution
	// begins, and when the work is done, OnExit() is called and the thread destroys
	// itself. Do not try to poll the state of this type of thread; if state info is
	// needed, use the main thread (ie. Adapt It GUI)to PostEvent() or AddPendingEvents()
	// as a way to pass signals back to the calling (main) thread -- but for this class we
	// won't need to
	virtual void*		Entry();

protected:

private:

};

#endif // for _KBSERVER

#endif