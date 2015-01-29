/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_UploadMulti.h
/// \author			Bruce Waters
/// \date_created	1 March 2013
/// \rcs_id $Id: Thread_UploadMulti.h 3126 2013-03-01 00:00:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the Thread_UploadMulti class.
/// The Thread_UploadMulti is a thread class for uploading a subset of normal (i.e. not
/// pseudo-deleted) local KB entries (i.e. several src/tgt pairs) to the remote kbserver
/// database. Uploading the normal entries of the local KB will be done by creating and
/// firing off many of these threads - up to 50 - so as not to overload the capacitiy of
/// slow processors to clear the threads quickly enough so that memory does not become full
/// of them. Only normal entries from the local KB which are not yet in the remote DB are
/// included in these upload threads.
/// This thread is a "detached" type (the wx default for thread objects); that is, it will
/// destroy itself once it completes. It is created on the heap. The need for a mutex is
/// avoided by ensuring that everything the thread needs for its job is contained
/// internally before it is Run().
/// There is a possibility, small though, of one of the entries being uploaded already
/// being in the KB (because another user caused that to happen between testing for those
/// to be sent, and the actual sending of them) - that would cause the createentry attempt
/// to fail, and the payload of entries would fail to all be entered into the remote DB. So
/// we need to alert the user with a wxMessage() in the event of such failure. The remedy
/// is to tell the user, in that message, to try again when noone else is doing any
/// uploading. We can also advise the user, when clicking the Send All Entries button, to
/// do so when noone else is trying to upload - either manually or by doing adapting.
/// Having a caution in the dialog over the button is probably a good way to do that.
/// \derivation		The Thread_UploadMulti class is derived from wxThread.
/////////////////////////////////////////////////////////////////////////////

#ifndef Thread_UploadMulti_h
#define Thread_UploadMulti_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Thread_UploadMulti.h"
#endif

#if defined(_KBSERVER)

// forward declarations
struct KbServerEntry;
class wxThread;
class KbServer;
class CBString;

//#include "KbServer.h" // needed for KbServerEntry struct definition

class Thread_UploadMulti : public wxThread
{
public:
	Thread_UploadMulti(); // creator
	virtual ~Thread_UploadMulti(void); // destructor

	// keep it simple, forget accessors, just make variables public
	KbServer*			m_pKbSvr;
	int					m_threadIndex;
	wxString			m_password;
	wxString			m_username;
	wxString			m_url;
	CBString			m_jsonUtf8Str;

	// BEW added 29Jan15 for error support. If someone else enters and entry currently
	// being uploaded within this thread's part of the bulk upload, we need to record
	// which thread failed - so we'll store the thread index (which it is of the up to
	// 50 possible, so the index will be between 0 and 49). KbServer.h & .cpp will
	// also have a wxArrayInt to store the curl error code returned, which that class 
	// can then interrogate to see if a recursion of the bulk upload is required to get
	// the missed entries into the remote database. We will allow a max of two recursions.
	// For these reasons, we'll make the returned curlcode value from the curl call
	// be a class member here too, so the caller can store the returned value
	int rv;

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