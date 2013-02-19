/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_KbEditorUpdateButton.h
/// \author			Bruce Waters
/// \date_created	19 February 2013
/// \rcs_id $Id: Thread_KbEditorUpdateButton.h 3106 2013-02-19 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the Thread_KbEditorUpdateButton class.
/// The Thread_KbEditorUpdateButton is a thread class for the support of the Update
/// button in the KB Editor dialog, in the context of KB sharing. This button allows the
/// selected target text entry to be edited in a wxTextCtrl, typically to fix a typo, and
/// the edited result replaces what was originally selected. Under the hood the KB
/// Editor's code does the following things... the original scr-tgt pair is pseudo-deleted,
/// and the new src-tgt pair is dealt with as follows:
/// (a) the user edited the target text and it has become unique (so a new normal entry is
/// put in the local KB); or
/// (b) the edited target text matches an existing normal entry in the local KB (in which
/// case no change is made to the local KB); or
/// (c) the edited target text matches a pseudo-deleted entry in the local KB (in which
/// case the pseudo-deleted local KB entry is undeleted to become a normal entry).
/// 
/// This Thread_KbEditorUpdateButton class supports scenarios (a) and (c). (Case (b) needs
/// no KB server supporting code.) What Thread_KbEditorUpdateButton does is attempt to
/// pseudo-delete the src-oldTgt entry in the remote DB, and after that completes, for (c)
/// it attempts to undelete the pseudo-deleted src-newTgt entry in the remote DB, provided
/// that the remote DB actually has that entry and has it with deleted flag set to 1 - but
/// if it isn't there (case (a)), it will instead create a normal src-newTgt entry in the
/// remote DB.
/// This thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes.
/// It is created on the heap, and potentially calls several kbserver API functions.
/// \derivation		The Thread_KbEditorUpdateButton class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

#ifndef Thread_KbEditorUpdateButton_h
#define Thread_KbEditorUpdateButton_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Thread_KbEditorUpdateButton.h"
#endif

#if defined(_KBSERVER)

// forward declarations
class wxThread;
class CKB;
class KbServer;
//class CAdapt_ItApp;

class Thread_KbEditorUpdateButton : public wxThread
{
public:
	Thread_KbEditorUpdateButton(); // creator
	virtual ~Thread_KbEditorUpdateButton(void); // destructor

	// keep it simple, forget accessors, just make variables public
	KbServer*			m_pKbSvr; // it knows which type it is
	wxString			m_source;
	wxString			m_oldTranslation;
	wxString			m_newTranslation;

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

	// base class virtual functions must be implemented, even if they do nothing new
	virtual bool    TestDestroy();

protected:

private:

};

#endif // for _KBSERVER

#endif
