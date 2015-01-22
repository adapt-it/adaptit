/// \project		adaptit
/// \file			Thread_BulkPseudoDelete.h
/// \author			Bruce Waters
/// \date_created	22 October 2013
/// \rcs_id $Id: Thread_BulkPseudoDelete.h 3101 2013-02-18 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the Thread_BulkPseudoDelete class.
/// Thread_BulkPseudoDelete is a thread class for PUTing a 1 value in the deleted flag
/// within a kbserver entry which is for eventual sharing - doing so for potentially
/// thousands of requested deletions from the KB Editor dialog; using a thread decouples
/// the transmissions from the user's normal adapting work, which is needed because of high
/// network latency causing unacceptable delays in the responsiveness of the GUI for the
/// interlinear layout.
/// The thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes, and clear the arrays from which it gets the source
/// phrases and target phrases for the pairs to be pseudo-deleted. (Until those arrays are
/// cleared, the user is prevented from initiating a further bulk delete.)
/// It is created on the heap, and changes the value of the deleted flag in as many entries 
/// of the kbserver database as their are entries in one or other of the input arrays
/// \derivation		The Thread_BulkPseudoDelete class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

#ifndef Thread_BulkPseudoDelete_h
#define Thread_BulkPseudoDelete_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Thread_BulkPseudoDelete.h"
#endif

#if defined(_KBSERVER)

// forward declarations
class wxThread;
class KbServer;
class CAdapt_ItApp;

class Thread_BulkPseudoDelete : public wxThread
{
public:
	Thread_BulkPseudoDelete(); // creator
	virtual ~Thread_BulkPseudoDelete(void); // destructor

	// keep it simple, forget accessors, just make variables public
	KbServer*			m_pKbSvr; // it knows which type it is
	wxArrayString*		m_pSourcePhrasesArray;    // these two will point to string
	wxArrayString*		m_pNonSourcePhrasesArray; // arrays stored in the app class
							// so that the thread will stay alive as long as the
							// application session does
	int					m_kbType;
	CAdapt_ItApp*		m_pApp;

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