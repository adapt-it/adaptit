/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_UploadOne.h
/// \author			Bruce Waters
/// \date_created	1 March 2013
/// \rcs_id $Id: Thread_UploadOne.h 3126 2013-03-01 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the Thread_UploadOne class.
/// The Thread_UploadOne is a thread class for uploading a single normal (i.e. not
/// pseudo-deleted) local KB entry (i.e. a src/tgt pair) to the remote kbserver database.
/// Uploading the normal entries of the local KB will be done by creating and firing off
/// many of these threads - perhaps slowed down in their creation by a timer, so as not to
/// overload the capacitiy of slow processors to clear the threads quickly enough so that
/// memory does not become full of them. Network latency determines how many will be
/// hanging around, and, of course, the timer interval.
/// The thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes.
/// It is created on the heap, and uploads just a single new entry to the KB server.
/// Errors are ignored - and of course anytime the src/tgt pair is sent to a DB which
/// already has that pair, the creation attempt for the entry in the remote DB will fail.
/// Well, we just don't care - we just want to get the ones not already in there, into
/// there - and those are the ones that will succeed.
/// \derivation		The Thread_UploadOne class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

#ifndef Thread_UploadOne_h
#define Thread_UploadOne_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Thread_UploadOne.h"
#endif

#if defined(_KBSERVER)

// forward declarations
struct KbServerEntry;
class wxThread;
class KbServer;
//class CAdapt_ItApp;
#include "KbServer.h" // needed for KbServerEntry struct definition

class Thread_UploadOne : public wxThread
{
public:
	Thread_UploadOne(); // creator
	virtual ~Thread_UploadOne(void); // destructor

	// keep it simple, forget accessors, just make variables public
	//CAdapt_ItApp*		m_pApp;
	KbServer*			m_pKbSvr; // it knows which type it is
	KbServerEntry		m_entry;
	wxString			m_kbType;
	wxString			m_password;
	wxString			m_username;
	wxString			m_srcLangCode;
	wxString			m_tgtLangCode;
	wxString			m_url;

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