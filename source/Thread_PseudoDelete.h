/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_PseudoDelete.h
/// \author			Bruce Waters
/// \date_created	18 February 2013
/// \rcs_id $Id: Thread_PseudoDelete.h 3101 2013-02-18 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the Thread_PseudoDelete class.
/// The Thread_PseudoDelete is a thread class for PUTing a 1 value in the deleted flag
/// within a kbserver entry which is for eventual sharing; it decouples the transmission
/// from the user's normal adapting work, which is needed because of high network latency
/// causing unacceptable delays in the responsiveness of the GUI for the interlinear
/// layout.
/// The thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes.
/// It is created on the heap, and changes the value of deleted in just a single entry 
/// of the kbserver database
/// \derivation		The Thread_PseudoDelete class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

#ifndef Thread_PseudoDelete_h
#define Thread_PseudoDelete_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Thread_PseudoDelete.h"
#endif

#if defined(_KBSERVER)

// forward declarations
class wxThread;
class CKB;
class KbServer;
class CAdapt_ItApp;

class Thread_PseudoDelete : public wxThread
{
public:
	Thread_PseudoDelete(); // creator
	virtual ~Thread_PseudoDelete(void); // destructor

	// keep it simple, forget accessors, just make variables public
	KbServer*			m_pKbSvr; // it knows which type it is
	wxString			m_source;
	wxString			m_translation;
	CAdapt_ItApp*		m_pApp; // to access ptr to KB, and ptr to KbServer

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
