/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_DoEntireKbDeletion.h
/// \author			Bruce Waters
/// \date_created	24 August 2013
/// \rcs_id $Id: Thread_DoEntireKbDeletion.h 3092 2013-02-12 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the Thread_DoEntireKbDeletion class.
/// The Thread_DoEntireKbDeletion is a "detached" thread class for deleting all
/// the entries for a specified kb database defined by a srclanguage/nonsourcelanguage
/// code pair; and after all those are deleted so that the code pair owns no entries, the
/// code pair definition is removed as the last step. This thread for a large (eg. 40,000
/// entry) database, in a high latency environment (e.g. approx 5 secs latency per entry
/// deletion), may take days to complete. It can be safely killed at any point, and
/// restarted in a later session - each entry deleted is gone forever, even if the thread
/// does not complete in a single session. Internally, it runs a loop in which a child
/// thread is fired to delete a single entry each time. It tracks how many have been
/// deleted, and the loop completes when the deletion count equals the queue size, or when
/// there is a http error (such as 404 Not Found). At that point the KB definition in the
/// kb table can be deleted without causing a foreign key constraint error.
/// \derivation		The Thread_DoEntireKbDeletion class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

#ifndef Thread_DoEntireKbDeletion_h
#define Thread_DoEntireKbDeletion_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Thread_DoEntireKbDeletion.h"
#endif

#if defined(_KBSERVER)

// forward declarations
class wxThread;
class CAdapt_ItApp;
class CKB;
class KbServer;

class Thread_DoEntireKbDeletion : public wxThread
{
public:
	Thread_DoEntireKbDeletion(KbServer* pStatelessKbServer, long kbDefinitionID, size_t nQueueSize); // creator
	virtual ~Thread_DoEntireKbDeletion(void); // destructor

	// keep it simple, forget accessors, just make variables public
	// hmmm... I don't think I need any of these, except m_pKbSvr in order to grab the
	// timestamp of the last changedsince() type of server access
	CAdapt_ItApp*		m_pApp; // to access ptr to KB, and ptr to KbServer
	KbServer*			m_pKbSvr; // set it to the currently active (stateless) instance
						// (We use a separate stateless instance because this instance
						// has to persist for possibly hours, whether or not the user
						// has the KB Sharing Manager open, or even whether or not he has
						// any shared project currently active)
	size_t				m_TotalEntriesToDelete;
	long				m_idForKbDefinitionToDelete;

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