/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_CreateEntry.h
/// \author			Bruce Waters
/// \date_created	29 January 2013
/// \rcs_id $Id: Thread_CreateEntry.h 3065 2013-01-29 02:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the Thread_CreateEntry class. 
/// The Thread_CreateEntry is an experimental thread class for uploading a single new
/// kbserver entry for eventual sharing; it's an attempt to decouple the upload from the
/// user's normal adapting work, due to high network latency causing unacceptable delays
/// in the responsiveness of the GUI for the interlinear layout.
/// The thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes.
/// It is created on the heap, and uploads just a single new entry to the KB server
/// (according to the current kbserver design, but I've asked for Jonathan to change that
/// a bit, so this thread implementation may have only a short life!)
/// \derivation		The Thread_CreateEntry class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

#ifndef Thread_CreateEntry_h
#define Thread_CreateEntry_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "MyListBox.h"
#endif

#if defined(_KBSERVER)

// forward declarations
class wxThread;
class CAdapt_ItApp;
class CKB;
//class KbServer;

class Thread_CreateEntry : public wxThread
{
public:
	Thread_CreateEntry(); // creator
	virtual ~Thread_CreateEntry(void); // destructor

	// keep it simple, forget accessors, just make variables public
	CAdapt_ItApp*		m_pApp; // to access ptr to KB, and ptr to KbServer
	CKB*				m_pKB; // to access HandleNewPairCreated()
	//KbServer*			m_pKbSvr; // not sure yet if I need it or not
	int					m_kbServerType; 
	bool				m_bUseCache; // our testing will have this as FALSE
	wxString			m_source;
	wxString			m_translation;

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