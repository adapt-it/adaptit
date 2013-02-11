/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Thread_CreateEntry.cpp
/// \author			Bruce Waters
/// \date_created	29 January 2013
/// \rcs_id $Id: Thread_CreateEntry.cpp 3065 2013-01-29 02:00:00Z bruce_waters@sil.org $
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

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Thread_CreateEntry.h"
#endif

#if defined(_KBSERVER)

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

// other includes
#include "Adapt_It.h"
#include "KB.h"
//#include "KbServer.h"
#include "Thread_CreateEntry.h"

Thread_CreateEntry::Thread_CreateEntry():wxThread()
{
	m_pApp = &wxGetApp();
}

Thread_CreateEntry::~Thread_CreateEntry()
{

}

void Thread_CreateEntry::OnExit()
{

}

void* Thread_CreateEntry::Entry()
{
	bool bHandledOK = m_pKB->HandleNewPairCreated(m_kbServerType, m_source, m_translation);
	bHandledOK = bHandledOK; // avoid compiler warning
	return (void*)NULL;
}

bool Thread_CreateEntry::TestDestroy()
{
  return true;
}

#endif // for _KBSERVER
