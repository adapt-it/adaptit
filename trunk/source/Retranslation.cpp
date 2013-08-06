/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Retranslation.cpp
/// \author			Erik Brommers
/// \date_created	10 March 2010
/// \rcs_id $Id$
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General
///                 Public License (see license directory)
/// \description	This is the implementation file for the CRetranslation class.
/// The CRetranslation class presents retranslation-related functionality to the user.
/// The code in the CRetranslation class was originally contained in
/// the CAdapt_ItView class.
/// BEW 24Jan13, made the 3 update handlers more robust (avoids false positive
/// for test of selection)
/// \derivation		The CNotes class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "Retranslation.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#if defined(__VISUALC__) && __VISUALC__ >= 1400
#pragma warning(disable:4428)	// VC 8.0 wrongly issues warning C4428: universal-character-name
// encountered in source for a statement like
// ellipsis = _T('\u2026');
// which contains a unicode character \u2026 in a string literal.
// The MSDN docs for warning C4428 are also misleading!
#endif

/////////
#include <wx/docview.h>	// includes wxWidgets doc/view framework
#include <wx/file.h>
#include <wx/clipbrd.h>
#include <wx/filesys.h> // for wxFileName
#include <wx/window.h> // for CaptureMouse()
#include <wx/event.h> // for GetCapturedWindow()
#include <wx/tokenzr.h>
#include <wx/textfile.h> // to get EOL info
#include "Adapt_ItCanvas.h"
#include "Adapt_It_Resources.h"
#include <wx/dir.h> // for wxDir
#include <wx/propdlg.h>
#include <wx/busyinfo.h>
#include <wx/print.h>
#include <wx/dynlib.h> // for wxDynamicLibrary
//////////

#include <wx/object.h>

#include "AdaptitConstants.h"
#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "SourcePhrase.h"
#include "Strip.h"
#include "Pile.h"
#include "Cell.h"
#include "Layout.h"
#include "Retranslation.h"
#include "helpers.h"
#include "MainFrm.h"
#include "Adapt_ItCanvas.h"
#include "Adapt_ItDoc.h"
#include "WaitDlg.h"
#include "Placeholder.h"
#include "KB.h"
#include "RetranslationDlg.h"
#include "StatusBar.h"

///////////////////////////////////////////////////////////////////////////////
// External globals
///////////////////////////////////////////////////////////////////////////////
extern bool gbIsGlossing;
extern bool gbInhibitMakeTargetStringCall;
extern bool gbAutoCaps;
extern bool gbUnmergeJustDone;
extern bool gbSourceIsUpperCase;
extern bool gbNonSourceIsUpperCase;
extern bool gbShowTargetOnly;
extern bool gbVerticalEditInProgress;
extern bool gbHasBookFolders;
extern int gnOldSequNum;
extern wxChar gcharNonSrcUC;
extern EditRecord gEditRecord;
/// This global is defined in PhraseBox.cpp.
extern wxString		translation; // translation, for a matched source phrase key


///////////////////////////////////////////////////////////////////////////////
// Event Table
///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CRetranslation, wxEvtHandler)
	EVT_TOOL(ID_BUTTON_RETRANSLATION, CRetranslation::OnButtonRetranslation)
	EVT_UPDATE_UI(ID_BUTTON_RETRANSLATION, CRetranslation::OnUpdateButtonRetranslation)
	EVT_TOOL(ID_BUTTON_EDIT_RETRANSLATION, CRetranslation::OnButtonEditRetranslation)
	EVT_UPDATE_UI(ID_BUTTON_EDIT_RETRANSLATION, CRetranslation::OnUpdateButtonEditRetranslation)
	EVT_TOOL(ID_REMOVE_RETRANSLATION, CRetranslation::OnRemoveRetranslation)
	EVT_UPDATE_UI(ID_REMOVE_RETRANSLATION, CRetranslation::OnUpdateRemoveRetranslation)
	EVT_MENU(ID_RETRANS_REPORT, CRetranslation::OnRetransReport)
	EVT_UPDATE_UI(ID_RETRANS_REPORT, CRetranslation::OnUpdateRetransReport)
END_EVENT_TABLE()


///////////////////////////////////////////////////////////////////////////////
// Constructors / destructors
///////////////////////////////////////////////////////////////////////////////

CRetranslation::CRetranslation()
{
}

CRetranslation::CRetranslation(CAdapt_ItApp* app)
{
	wxASSERT(app != NULL);
	m_pApp = app;
	m_pLayout = m_pApp->GetLayout();
	m_pView = m_pApp->GetView();
	m_bIsRetranslationCurrent = FALSE;
	// BEW 23Sep10, additional initializations missed earlier on
	m_bReplaceInRetranslation = FALSE;
	m_bInsertingWithinFootnote = FALSE;
	m_bSuppressRemovalOfRefString = FALSE;
}

CRetranslation::~CRetranslation()
{

}

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

// we allow this search whether glossing is on or not; as it might be a useful search when
// glossing is ON
bool CRetranslation::DoFindRetranslation(int nStartSequNum, int& nSequNum, int& nCount)
{
	SPList* pList = m_pApp->m_pSourcePhrases;
	wxASSERT(pList != NULL);
	SPList::Node* pos = pList->Item(nStartSequNum); // starting POSITION
	wxASSERT(pos != NULL);
	CSourcePhrase* pSrcPhrase = NULL;
	SPList::Node* savePos = pos;

	// get past the current retranslation, if we are in one
	pSrcPhrase = (CSourcePhrase*)pos->GetData();
	if (pSrcPhrase->m_bBeginRetranslation)
	{
		// we are at the start of a retranslation, so must access the next srcPhrase
		// before we have a possibility of being out of it, or into the next retranslation
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
	}
	if (pSrcPhrase->m_bRetranslation)
	{
		while (pos != NULL)
		{
			savePos = pos;
			pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			if (!pSrcPhrase->m_bRetranslation || pSrcPhrase->m_bBeginRetranslation)
				break; // break if we are out, or at the beginning of a consecutive one
		}
	}

	// do the search, confining attempts within a single CSourcePhrase instance
	if (pos == NULL)
	{
		// we are at the end of the document
		nSequNum = -1; // undefined
		return FALSE;
	}

	pos = savePos;
	wxASSERT(pos != NULL);
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase != NULL);
		if (pSrcPhrase->m_bRetranslation || pSrcPhrase->m_bBeginRetranslation)
		{
			// we found a retranslation
			nSequNum = pSrcPhrase->m_nSequNumber;
			nCount = 1;
			return TRUE;
		}
	}

	// if we get here, we didn't find a match
	nSequNum = -1;
	return FALSE;
}

void CRetranslation::DoRetranslation()
{
	wxCommandEvent dummyevent;
	// IDS_TOO_MANY_SRC_WORDS
	wxMessageBox(_(
				   "Warning: there are too many source language words in this phrase for this adaptation to be stored in the knowledge base."),
				 _T(""), wxICON_INFORMATION | wxOK);

	OnButtonRetranslation(dummyevent);
}

void CRetranslation::DoRetranslationByUpArrow()
{
	wxCommandEvent dummyevent;
	OnButtonRetranslation(dummyevent);
}

void CRetranslation::DoOneDocReport(wxString& name, SPList* pList, wxFile* pFile)
{
	wxASSERT(!pList->IsEmpty());
	wxString oldText;
	oldText.Empty();
	wxString newText;
	newText.Empty();
	wxString endText = wxTextFile::GetEOL();
	wxString chAndVerse;
	chAndVerse.Empty();
	wxString indexText;
	indexText.Empty();
	wxString prevIndexText;
	prevIndexText.Empty();
	CSourcePhrase* pSrcPhrase = NULL;
	int count = 0;

	// whm 26Aug11 Open a wxProgressDialog instance here for KB Restore operations.
	// The dialog's pProgDlg pointer is passed along through various functions that
	// get called in the process.
	// whm WARNING: The maximum range of the wxProgressDialog (nTotal below) cannot
	// be changed after the dialog is created. So any routine that gets passed the
	// pProgDlg pointer, must make sure that value in its Update() function does not
	// exceed the same maximum value (nTotal).
	//
	// This progress dialog continues for the duration of OnRetransReport(). We need
	// a separate progress dialog inside DoOneDocReport() because each instance of
	// it will be processing a different document with different ranges of values.
	wxString msgDisplayed;
	const int nTotal = pList->GetCount() + 1;
	wxString progMsg = _("%s  - %d of %d Total words and phrases");
	wxFileName fn(m_pApp->m_curOutputFilename);
	msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),1,nTotal);
	CStatusBar* pStatusBar = NULL;
	pStatusBar = (CStatusBar*)m_pApp->GetMainFrame()->m_pStatusBar;
	pStatusBar->StartProgress(_("Generating Report..."), msgDisplayed, nTotal);

	wxLogNull logNo; // avoid spurious messages from the system

	// whm 24Aug11 moved this wxProgressDialog out to the top level
	// OnRetranslationReport() with its pProgDlg pointer passed on
	// down to this routine, so that the progress dialog can be
	// updated for each book that gets processed without the flicker
	// of more than one progress dialog appearing during the process
	// initialize the progress indicator window

	progMsg = _("%s  - %d of %d Total words and phrases");
	msgDisplayed = progMsg.Format(progMsg,name.c_str(),1,nTotal);
	pStatusBar->UpdateProgress(_("Generating Report..."), count, msgDisplayed);

	// whm 24Aug11 Note: The progress dialog is created on the heap back in
	// the OnRetranslationReport(), and its pointer is pass along through
	// DoRetranslationReport() and from there to here in DoOneDocReport().
	// The progress dialog is destroyed back in the top-level caller
	// OnRetranslationReport.

	// compose the output data & write it out, phrase by phrase
	SPList::Node* pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	bool bStartRetrans = TRUE;
	bool bJustEnded = FALSE;
	bool bStartOver = FALSE;
	int counter = 0; // for progress indicator
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase != 0);
		counter++;

		if (!pSrcPhrase->m_chapterVerse.IsEmpty())
		{
			prevIndexText = indexText; // keep old ch & verse, in case a retrans
			// is at previous verse's end
			indexText = name + _T(" ") + pSrcPhrase->m_chapterVerse + endText;
		}

	b:		bStartOver = FALSE;
		if (pSrcPhrase->m_bRetranslation)
		{
			if (bStartRetrans || pSrcPhrase->m_bBeginRetranslation)
			{
				if (oldText.IsEmpty())
				{
					// we've just written out an entry, so we are ready
					// to begin a new one
					oldText = pSrcPhrase->m_srcPhrase;
					newText = pSrcPhrase->m_targetStr;
					bStartRetrans = FALSE;
					count++;
				}
				else
				{
                    // the current entry is not yet written out, (probably m_bBeginRe... is
                    // TRUE) because we are at start of a consecutive retranslation
					bJustEnded = TRUE;
					bStartOver = TRUE; // so we can start a new entry
					// without iterating the loop
					goto a;
				}
			}
			else
			{
				if (!pSrcPhrase->m_bNullSourcePhrase)
				{
					oldText += _T(" ") + pSrcPhrase->m_srcPhrase;
				}
				if (!pSrcPhrase->m_targetStr.IsEmpty())
				{
					newText += _T(" ") + pSrcPhrase->m_targetStr;
				}
			}
			bJustEnded = TRUE;

			// if we are at the end of the file, we have to force the
			// writing of the current one before we exit the loop
			if (counter == (int)pList->GetCount())
				goto a;
		}
		else
		{
			if (bJustEnded)
			{
			a:				oldText += endText;
				newText += endText + endText; // we want a blank line

				// write out the entry to the file
				if (pSrcPhrase->m_chapterVerse.IsEmpty())
				{
					// within a verse, use current indexText
#ifndef _UNICODE
					pFile->Write(indexText);
#else
					m_pApp->ConvertAndWrite(wxFONTENCODING_UTF8,pFile,indexText); // use UTF-8
#endif
				}
				else
				{
					// retranslation goes to end of last verse, use previous indexText
#ifndef _UNICODE
					pFile->Write(prevIndexText);
#else
					m_pApp->ConvertAndWrite(wxFONTENCODING_UTF8,pFile,prevIndexText); // use UTF-8
#endif
				}
#ifndef _UNICODE
				pFile->Write(oldText);

#else // _UNICODE version
				// use UTF-8 encoding
				m_pApp->ConvertAndWrite(wxFONTENCODING_UTF8,pFile,oldText);
#endif
			}
			oldText.Empty();
			newText.Empty();
			bJustEnded = FALSE;
			bStartRetrans = TRUE; // get ready for start of next one encountered
		}

		// update the progress bar
		if (counter % 1000 == 0)
		{
			msgDisplayed = progMsg.Format(progMsg,name.c_str(),counter,nTotal);
			pStatusBar->UpdateProgress(_("Generating Report..."), counter, msgDisplayed);
		}

		if (bStartOver)
			goto b;
	}

	// remove the progress indicator window
	pStatusBar->FinishProgress(_("Generating Report..."));

	if (count == 0)
	{
		oldText.Empty();
		// IDS_NO_RETRANS_IN_DOC
		oldText = oldText.Format(_(
								   "***  There were no retranslations in the %s document. ***"),
								 name.c_str());
		oldText += endText + endText;
#ifndef _UNICODE
		pFile->Write(oldText); //pFile->WriteString(oldText);
#else
		m_pApp->ConvertAndWrite(wxFONTENCODING_UTF8,pFile,oldText); // use UTF-8
#endif
	}
}

/////////////////////////////////////////////////////////////////////////////////
//
// Begin section for  OnButtonEditRetranslation()
//
// Helper functions:
// IsEndInCurrentSelection()
// AccumulateText()
// ReplaceMatchedSubstring()
//
/////////////////////////////////////////////////////////////////////////////////
bool CRetranslation::IsEndInCurrentSelection()
{
	CCellList::Node* pos = m_pApp->m_selection.GetLast();
	CCell* pCell = (CCell*)pos->GetData();
	pos = pos->GetPrevious();
	bool bCurrentSection = TRUE;
	while( pos != NULL)
	{
		pCell = (CCell*)pos->GetData();
		pos = pos->GetPrevious();
		CPile* pPile2 = pCell->GetPile();
		if (pPile2->GetSrcPhrase()->m_bEndRetranslation)
		{
			bCurrentSection = FALSE;
			break;
		}
	}
	return bCurrentSection;
}

void CRetranslation::AccumulateText(SPList* pList,wxString& strSource,wxString& strAdapt)
{
	SPList::Node* pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	wxString str;
	wxString str2;

	while (pos != NULL)
	{
		// accumulate the old retranslation's text
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		str = pSrcPhrase->m_targetStr;
		if (strAdapt.IsEmpty())
		{
			strAdapt += str;
		}
		else
		{
			if (!str.IsEmpty())
				strAdapt += _T(" ") + str;
		}

		// also accumulate the source language text (line 1), provided it is not a null
		// source phrase
		if (!pSrcPhrase->m_bNullSourcePhrase)
		{
			str2 = pSrcPhrase->m_srcPhrase;
			if (strSource.IsEmpty())
			{
				strSource = str2;
			}
			else
			{
				strSource += _T(" ") + str2;
			}
		}
	}
}

void CRetranslation::DoRetranslationReport(CAdapt_ItDoc* pDoc,
										   wxString& name, wxArrayString* pFileList,
										   SPList* pList, wxFile* pFile,
										   const wxString& progressItem)
{
	CStatusBar* pStatusBar = (CStatusBar*)m_pApp->GetMainFrame()->m_pStatusBar;
	if (pFileList->IsEmpty())
	{

		// use the open document's pList of srcPhrase pointers
		// whm Note: DoOneDocReport() has its own progress dialog
		DoOneDocReport(name,pList,pFile);
	}
	else
	{
		// no document is open, so pList will be empty, so we have
		// to iterate over files in pFileList
		wxASSERT(pList->IsEmpty());
		wxASSERT(!pFileList->IsEmpty());

		// iterate over the files (borrow code from DoConsistencyCheck)
		int nCount = pFileList->GetCount();
		if (nCount <= 0)
		{
			// something is real wrong, but this should never happen so
			// an English message will suffice
			wxString error;
			error = error.Format(_T(
									"Error, the file count was found to be %d, so the command was aborted.")
								 ,nCount);
			wxMessageBox(error);
			return;
		}

		// lock view window updates till done
		m_pApp->GetMainFrame()->canvas->Freeze();

		// iterate over the document files
		SPList* pPhrases;
		for (int i=0; i < nCount; i++)
		{
			wxString newName = pFileList->Item(i);
			wxASSERT(!newName.IsEmpty());

			// make a suitable name for the document, for the report
			wxString indexingName = newName;
			int len = indexingName.Length();
			indexingName.Remove(len-4,4); // remove the .adt or .xml extension

			// open the document
			// whm TODO Note: This routine takes much longer than is really necessary just
			// to glean the bits of information needed for doing the Retranslation report.
			// The RetranslationReport() function never displays any View of a document,
			// its Layout etc. But, the OnOpenDocument() and ClobberDocument() calls below
			// have a huge overhead because they do a lot of extra memory allocations and
			// deallocations of stuff not needed to do the report - things that are done in
			// preparation for displaying and laying out a View (that is not needed here).
			bool bOK;
			bOK = pDoc->OnOpenDocument(newName, false);
			wxCHECK_RET(bOK, _T("DoRetranslationReport(): OnOpenDocument() failed, line 544 in Retranslation.cpp"));

			pDoc->SetFilename(newName,TRUE);

			int nTotal = m_pApp->m_pSourcePhrases->GetCount();
			if (nTotal == 0)
			{
				wxString str;
				str = str.Format(_T("Bad file:  %s"),newName.c_str());
				wxMessageBox(str,_T(""),wxICON_EXCLAMATION | wxOK);
				// whm Note: Even though this error should not happen but rarely, it
				// shouldn't result in the entire application stopping!
				return;
			}

			// get a local pointer to the list of source phrases
			pPhrases = m_pApp->m_pSourcePhrases;

			// use the now open document's pList of srcPhrase pointers, build
			// the part of the report which pertains to this document
			// whm Note: DoOneDocReport() has its own progress dialog
			DoOneDocReport(indexingName,pPhrases,pFile);

			// remove the document
			if (!pPhrases->IsEmpty())
			{
				m_pApp->GetView()->ClobberDocument();

				// delete the buffer containing the filed-in source text
				if (m_pApp->m_pBuffer != NULL)
				{
					delete m_pApp->m_pBuffer;
					m_pApp->m_pBuffer = NULL;
				}
			}
			// update the progress bar
			pStatusBar->UpdateProgress(progressItem,(i + 1));
		}

		// allow the view to respond again to updates
		m_pApp->GetMainFrame()->canvas->Thaw();
	}
	// remove the top level progress dialog
	pStatusBar->FinishProgress(progressItem);
}


// finds the strSearch in strAdapt, and replaces it with strReplace,
// updating strAdapt in the caller
void CRetranslation::ReplaceMatchedSubstring(wxString strSearch, wxString& strReplace,
											 wxString& strAdapt)
{
	int nFound = -1;
	int lenAdaptStr = 0;
	int lenTgt = 0;
	int nRight;
	wxString left;
	left.Empty();
	wxString right;
	right.Empty();
	lenTgt = strSearch.Length(); // the search string's length
	lenAdaptStr = strAdapt.Length(); // length of the string in which the search is done
	nFound = strAdapt.Find(strSearch);
	wxASSERT(nFound != -1); // must not have failed, since this was the match done in caller
	left = strAdapt.Left(nFound);
	nRight = nFound + lenTgt;
	wxASSERT(nRight <= lenAdaptStr);
	nRight = lenAdaptStr - nRight;
	right = strAdapt.Right(nRight);

	// put the final string into the strAdapt alias string in caller
	strAdapt = left + strReplace + right;
}

/////////////////////////////////////////////////////////////////////////////////
//
// Begin section for OnButtonRetranslation()
//
// Helper functions used in OnButtonRetranslation(); these are
// (in the order in which they get called):
//
// GetSelectedSourcePhraseInstances()
// CopySourcePhraseList()
// RemoveNullSourcePhraseFromLists()
// UnmergeMergersInSublist()
// TokenizeTextString()
// BuildRetranslationSourcePhraseInstances()
// DeleteSavedSrcPhraseSublist()
// PadWithNullSourcePhrasesAtEnd()
// SetActivePilePointerSafely()
// ClearSublistKBEntries()
// InsertSublistAfter()
// RemoveUnwantedSourcePhraseInstancesInRestoredList()
// RestoreTargetBoxText()
//
/////////////////////////////////////////////////////////////////////////////////
// pList is the list of selected CSourcePhrase instances; pSrcPhrases is the full list
// maintained on the app; strSource is the accumulated source text, strAdapt is the
// accumulated target text (both with punctuation). The pList will only contain copies of
// the pointers to the CSourcePhrase instances on the heap.
//
// BEW 16Feb10, no changes needed for support of doc version 5
void CRetranslation::GetSelectedSourcePhraseInstances(SPList*& pList,
													 wxString& strSource, wxString& strAdapt)
{
	wxString str; str.Empty();
	wxString str2; str2.Empty();
	CCellList::Node* pos = m_pApp->m_selection.GetFirst();
	CCell* pCell = (CCell*)pos->GetData();
	CPile* pPile = pCell->GetPile(); // get the pile first in selection
	pos = pos->GetNext(); // needed for our CCellList list
	CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();

	pList->Append(pSrcPhrase); // add first to the temporary list
	if (pSrcPhrase->m_targetStr.IsEmpty())
	{
		if (pPile == m_pApp->m_pActivePile)
		{
			str = m_pApp->m_targetPhrase;
		}
		else
		{
			str.Empty();
		}
	}
	else
	{
		str = pSrcPhrase->m_targetStr;
	}
	strAdapt += str;

	// accumulate the source language key text, provided it is not a null source phrase
	if (!pSrcPhrase->m_bNullSourcePhrase)
		str2 = pSrcPhrase->m_srcPhrase;
	strSource += str2; // accumulate it

    // fill the list, accumulating any translation text already in the selected source
    // phrases and also the original text (with punctuation) which is to be retranslated
	while (pos != NULL)
	{
 		CCell* pCell = (CCell*)pos->GetData();
		pPile = pCell->GetPile();
		pos = pos->GetNext(); // needed for our list
		pSrcPhrase = pPile->GetSrcPhrase();
		wxASSERT(pSrcPhrase);
		pList->Append(pSrcPhrase);

		// accumulate any adaptation
		if (pSrcPhrase->m_targetStr.IsEmpty())
		{
			if (pPile == m_pApp->m_pActivePile)
			{
				str = m_pApp->m_targetPhrase;
			}
			else
			{
				str.Empty();
			}
		}
		else
		{
			str = pSrcPhrase->m_targetStr;
		}

		if (strAdapt.IsEmpty())
			strAdapt += str;
		else
		{
			if (!str.IsEmpty())
				strAdapt = strAdapt + _T(" ") + str;
		}

		// accumulate the source language text,
		// provided it is not a null source phrase
		if (!pSrcPhrase->m_bNullSourcePhrase)
			str2 = pSrcPhrase->m_srcPhrase;
		else
			str2.Empty();
		if (strSource.IsEmpty())
			strSource += str2;
		else
			strSource = strSource + _T(" ") + str2; // space before a
		// marker will -> CR+LF on Export...
	}
}

// pList is the list to be copied, pCopiedList contains the copies
// Note: the default is a shallow copy; any heap instances of CSourcePhrases which are
// pointed at by elements in the m_pSavedWords list in pList, are also pointed at by
// the copies of elements in the m_pSavedWords list of CSourcePhrase instances in
// pCopiedList. This has implications when destroying such a copied list. For a true
// deep copy of a list of CSourcePhrase instances, the bDoDeepCopy flag must be TRUE. A
// deep copy produces a copied list, pCopiedList, in which everything is a duplicate of
// what was in the original list, and hence every original CSourcePhrase of a merger is
// pointed at by a CSourcePhrase in only one of the pList and pCopiedList lists.
// BEW modified 16Apr08 to enable it to optionally do a deep copy
void CRetranslation::CopySourcePhraseList(SPList*& pList,SPList*& pCopiedList,
										 bool bDoDeepCopy)
{
	SPList::Node* pos = pList->GetFirst(); // original list
	while (pos != NULL)
	{
		CSourcePhrase* pElement = (CSourcePhrase*)pos->GetData(); // original source phrase
		pos = pos->GetNext();// needed for our list
		CSourcePhrase* pNewSrcPhrase = new CSourcePhrase(*pElement); // uses operator=
		wxASSERT(pNewSrcPhrase != NULL);
		if (bDoDeepCopy)
		{
			pNewSrcPhrase->DeepCopy();
		}
		pCopiedList->Append(pNewSrcPhrase);
	}
}


// same parameters as for RemoveNullSourcePhraseFromLists(), except the second last boolean
// is added in order to control whether m_bRetranslation gets set or not; for
// retranslations we want it set, for editing the source text we want it cleared; and the
// last boolean controls whether or not we also update the sublist passed as the first
// parameter - for a retranslation we don't update it, because the caller will make no more
// use of it; but for an edit of the source text, the caller needs it updated because it
// will be used later when the transfer of standard format markers, if any, is done.
//
// BEW updated 17Feb10 for support of doc version 5 (no changes were needed)
void CRetranslation::UnmergeMergersInSublist(SPList*& pList, SPList*& pSrcPhrases,
											int& nCount, int& nEndSequNum, bool bActiveLocAfterSelection,
											int& nSaveActiveSequNum, bool bWantRetranslationFlagSet,
											bool bAlsoUpdateSublist)
{
	int nNumElements = 1;
	SPList::Node* pos = pList->GetFirst();
	int nTotalExtras = 0; // accumulate the total number of extras added by unmerging,
						  // this will be used if the updating of the sublist is asked for
	int nInitialSequNum = pos->GetData()->m_nSequNumber; // preserve this
														 // for sublist updating
	wxString emptyStr = _T("");
	while (pos != NULL)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		int nStartingSequNum = pSrcPhrase->m_nSequNumber;
		nNumElements = 1;
		if (pSrcPhrase->m_nSrcWords > 1)
		{
            // have to restore to original state (RestoreOriginalMinPhrases also appends
            // any m_translation in the CRefString object to pApp->m_targetPhrase, but we
            // don't care about that here as we will abandon pApp->m_targetPhrase's
            // contents anyway)
			nNumElements = m_pView->RestoreOriginalMinPhrases(pSrcPhrase,nStartingSequNum);

            // RestoreOriginalMinPhrases accumulates any m_adaptation text into the view's
            // pApp->m_targetPhrase attribute, which is fine when doing an unmerge of a
            // single merged phrase, but it is not fine in preparing a retranslation. The
            // reason is that if there are a lot of merged phrases in the selection and if
            // these have existing translations in them, unmerging them will result in the
            // accumulation of a very long pApp->m_targetPhrase. When we then get to the
            // RecalcLayout call below, it will rebuild the strips and eventually one of
            // the pile's will have a source phrase with sequence number equalling the old
            // active location's sequence number; but in the unmerging process, the
            // m_adaptation field is cleared for each sourcePhrase, and so when
            // CalcPileWidth is called at the point when the old active pile is reached,
            // the "else" block is used for calculating the text extent because
            // m_adaptation is empty. Unfortunately, the else block calculated the extent
            // by looking at the contents of pApp->m_targetPhrase, as set up by the
            // unmerges being done now - the result is a huge pile width, and this would
            // lead to a crash when the endIndex is reached - since no source phrases can
            // be laid out due to the spurious large pileWidth value. The solution is to
            // clean out the contents of pApp->m_targetPhrase after each call of
            // RestoreOriginalMinPhrases above, so that the caluculated pileWidth values
            // will be correct. So we do it now, where it makes sense - though it could
            // instead be done just once before the RecalcLayout call.
			// RestoreOriginalMinPhrases restores to the KB the adaptations, if any,
			// removed from there at the merger.
			m_pApp->m_targetPhrase.Empty();

			// update nCount etc.
			int nExtras = nNumElements - 1;
			nTotalExtras += nExtras;
			nCount += nExtras;
			nEndSequNum += nExtras;
			if (bActiveLocAfterSelection)
				nSaveActiveSequNum += nExtras;

            // set the flags on the restored original min phrases (the pointers to the
            // piles are clobbered, but the sequence in the m_pSourcePhrases list is
            // restored, so access these)
			for (int i = nStartingSequNum; i <= nStartingSequNum + nExtras; i++)
			{
				// here POSITION pos is redefined in a subscope of the one above
				SPList::Node* pos = pSrcPhrases->Item(i);
				wxASSERT(pos != NULL);
				CSourcePhrase* pSPh = (CSourcePhrase*)pos->GetData();
				pSPh->m_bHasKBEntry = FALSE;
				if (bWantRetranslationFlagSet)
				{
					pSPh->m_bRetranslation = TRUE;
					pSPh->m_bNotInKB = TRUE;
				}
				else
				{
					pSPh->m_bRetranslation = FALSE;
					pSPh->m_bNotInKB = FALSE;
				}
				pSPh->m_adaption.Empty();  // ensure its clear for later on
				pSPh->m_targetStr.Empty(); // ditto
			}
		}
		else
		{
			//  remove the refString from the KB, etc.

			m_pApp->m_pKB->GetAndRemoveRefString(pSrcPhrase, emptyStr, useGlossOrAdaptationForLookup);

			// we must abandon any existing adaptation text
			pSrcPhrase->m_adaption.Empty();
			pSrcPhrase->m_bNotInKB = TRUE;
			if (bWantRetranslationFlagSet)
			{
				pSrcPhrase->m_bRetranslation = TRUE;
				pSrcPhrase->m_bNotInKB = TRUE;
			}
			else
			{
				pSrcPhrase->m_bRetranslation = FALSE;
				pSrcPhrase->m_bNotInKB = FALSE;
			}
			pSrcPhrase->m_bHasKBEntry = FALSE;
			pSrcPhrase->m_targetStr.Empty();
		}
	}

    // do the sublist updating, if required (this just clears the list and then puts copied
    // pointers into the sublist, so no partner pile creation needed here
	if (bAlsoUpdateSublist)
	{
		int nOldCount = pList->GetCount();
		wxASSERT(nOldCount);
		pList->Clear(); // clear the old list of pointer
		// again POSITION pos is redefined in a subscope
		SPList::Node* pos = pSrcPhrases->Item(nInitialSequNum);
		int nNewCount = nOldCount + nTotalExtras;
		CSourcePhrase* pSrcPhrase = 0;
		int index;
		for (index = 0; index < nNewCount; index++)
		{
			pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			wxASSERT(pSrcPhrase);
			pList->Append(pSrcPhrase);
		}
	}
}

// BEW 17Feb10 updated to support doc version 5 (no changes were needed)
// BEW 11Oct10, made a small change, but the essentials were already okay, it already
// copies the punctuated m_srcPhrase string into m_targetStr
void CRetranslation::BuildRetranslationSourcePhraseInstances(SPList* pRetransList,
						int nStartSequNum,int nNewCount,int nCount,int& nFinish)
{
	// BEW refactored 16Apr09
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	int nSequNum = nStartSequNum - 1;
	nFinish = nNewCount < nCount ? nCount : nNewCount;
	for (int j=0; j<nFinish; j++)
	{
		nSequNum++;
		CPile* pPile = m_pView->GetPile(nSequNum); // needed, because the
						// we did a RecalcLayout() in the caller beforehand
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pPile->GetSrcPhrase(); // update this one

		if (j == 0)
		{
			// mark the first one
			pSrcPhrase->m_bBeginRetranslation = TRUE;
			pSrcPhrase->m_bEndRetranslation = FALSE;
		}
		else if (j == nFinish - 1)
		{
			// mark the last one
			pSrcPhrase->m_bEndRetranslation = TRUE;
			pSrcPhrase->m_bBeginRetranslation = FALSE;
		}
		else
		{
			// play safe, ensure any others have both flags cleared
			pSrcPhrase->m_bBeginRetranslation = FALSE;
			pSrcPhrase->m_bEndRetranslation = FALSE;
		}

		if (j < nNewCount)
		{
			// there will be a retranslation word available only when j < nNewCount
			SPList::Node* pos = pRetransList->Item(j);
			CSourcePhrase* pIncompleteSrcPhrase = (CSourcePhrase*)pos->GetData();
			wxASSERT(pIncompleteSrcPhrase != NULL);

            // copy the text across (these "source phrases" actually contain target text in
            // the attributes which otherwise would hold source text, due to the use of
            // TokenizeText for parsing what the user typed)
			pSrcPhrase->m_targetStr = pIncompleteSrcPhrase->m_srcPhrase; //with punctuation
#if defined(_DEBUG)
				// In case the RossJones m_targetStr not sticking bug comes from here
		wxLogDebug(_T("BuildRetranslationSourcePhraseInstances(), line 942 Retranslation.cpp, m_targetStr:  %s"),
					pSrcPhrase->m_targetStr.c_str());
#endif
			m_pView->RemovePunctuation(pDoc,&pIncompleteSrcPhrase->m_key,from_target_text);
			pSrcPhrase->m_adaption = pIncompleteSrcPhrase->m_key;
			//check that all is well
			wxASSERT(pSrcPhrase->m_nSequNumber == pIncompleteSrcPhrase->m_nSequNumber);

			// BEW added 13Mar09 for refactored layout
			pDoc->ResetPartnerPileWidth(pSrcPhrase); // resets width and marks the
													 // owning strip invalid
		}

        // if nNewCount was less than nCount, we must clear any old m_targetStr and
        // m_adaption test off of the unused source phrases at the end of the selection (we
		// will leave markers untouched) -- MakeTargetStringIncludingPunctuation() does
		// not get called for setting up m_targetStr when we are in a retranslation
		if (j >= nNewCount)
		{
			// BEW 11Oct10, added next two tests to remove content from m_adaption and
			// m_targetStr when it's beyond the end of where the new translation ends
			// (this might get done in the caller, but here is probably better)
			if (!pSrcPhrase->m_adaption.IsEmpty())
			{
				pSrcPhrase->m_adaption.Empty();
			}
			if (!pSrcPhrase->m_targetStr.IsEmpty())
			{
				pSrcPhrase->m_targetStr.Empty();
			}
			pDoc->ResetPartnerPileWidth(pSrcPhrase);
		}
	}
}

/* BEW deprecatedd 9Mar11
// pList is a sublist of CSourcePhrase instances, from a reparse of a source text string
// (typically from calling FromMergerMakeSstr() which re-generates the source of the
// merger, including all markers and (if we request it, and we would have) all filtered
// info stored thereon)
// tgtText is the adaptation text, if any, stored on that merger - including punctuation
// gloss is the gloss text, if any, stored on that merger -- Note, we'll not tokenize the
// gloss into an array of words and assign them to the CSourcePhrase instances - it is
// almost certain that this would produce invalid correspondences between source text and
// glosses, so we'll just assign the whole gloss to the m_gloss member of the first
// instance - that way, if a manual edit is needed, the user only has one place to grab
// the whole gloss from
void CRetranslation::ConvertSublistToARetranslation(SPList* pList, wxString& tgtText, wxString& gloss)
{
	// Note: the CSourcePhrase instances in pList are not yet inserted into the
	// m_pSourcePhrases list of the document, so we don't here need to create partner
	// piles for any new ones we may add to the list
	long listCount = (long)pList->GetCount();
	if (listCount == 0)
		return;
	SPList::Node* pos = pList->GetFirst();
	CSourcePhrase* pSrcPhrase = pos->GetData();
	SPList::Node* posEnd = pList->GetLast();
	CSourcePhrase* pEndSrcPhrase = posEnd->GetData();
	int lastSequNum = pEndSrcPhrase->m_nSequNumber;
	TextType theType = pEndSrcPhrase->m_curTextType;
	bool bSpecialText = pEndSrcPhrase->m_bSpecialText;
	bool bFootnoteEnd = pEndSrcPhrase->m_bFootnoteEnd;
	bool bBoundary = pEndSrcPhrase->m_bBoundary;
	bool bHasFreeTrans = pEndSrcPhrase->m_bHasFreeTrans;
	bool bEndFreeTrans = pEndSrcPhrase->m_bEndFreeTrans;

	SPList::Node* posNewLast;

	// put the whole gloss on the first instance
	pSrcPhrase->m_gloss = gloss;

    // tokenize the target text, using space as a delimiter, into words (including any
    // punctuation on each), the final FALSE parameter is bStoreEmptyStringsToo
	wxArrayString arrTgtText;
	wxString delimiters = _T(" ");
	long numTgtElements = SmartTokenize(delimiters, tgtText, arrTgtText, FALSE);

	// if numTgtElements is greater than listCount, we need to add Placeholders at the end
	// of the sublist in order to carry the extra tokens which SmartTokenize() generated;
	// to do this job we reuse some of the code from the PadWithNullSourcePhrasesAtEnd()
	// function
	long nExtrasLong = numTgtElements - listCount;
	if (nExtrasLong > 0)
	{
		// padding is required... & so closing-off things may need shifting to the end
		wxString saveFollOuterPunct = pEndSrcPhrase->GetFollowingOuterPunct();
		wxString saveFollPunct = pEndSrcPhrase->m_follPunct;
		wxString saveInlineNonbindingEndMkr = pEndSrcPhrase->GetInlineNonbindingEndMarkers();
		wxString saveInlineBindingEndMkrs = pEndSrcPhrase->GetInlineBindingEndMarkers();
		wxString saveEndMarkers = pEndSrcPhrase->GetEndMarkers();
		// now clear out these members, as their values will be tranferred to the sublist end
		pEndSrcPhrase->GetFollowingOuterPunct().Empty();
		pEndSrcPhrase->m_follPunct = _T("");
		pEndSrcPhrase->GetInlineNonbindingEndMarkers().Empty();
		pEndSrcPhrase->GetInlineBindingEndMarkers().Empty();
		pEndSrcPhrase->GetEndMarkers().Empty();

		int nExtras = (int)nExtrasLong;
		int index = 0;
		CSourcePhrase* pPlaceholder = NULL;
		for (index = 0; index < nExtras; index++)
		{
			// create on the heap, and return
			pPlaceholder = m_pApp->GetPlaceholder()->CreateBasicPlaceholder();
			pPlaceholder->m_nSequNumber = lastSequNum + index + 1;
			// it's going to be part of a retranslation, so set the flags
			pPlaceholder->m_bRetranslation = TRUE;
			pPlaceholder->m_bNotInKB = TRUE;
			posNewLast = pList->Append(pPlaceholder);
			// handle the flag values, etc, that must propagate to each instance
			pPlaceholder->m_bHasFreeTrans = bHasFreeTrans;
			pPlaceholder->m_curTextType = theType;
			pPlaceholder->m_bSpecialText = bSpecialText;

			// handle the transfers of end-pertinent data
			if (index == nExtras - 1)
			{
				// pPlaceholder is the last to be appended, so add the end stuff
				pPlaceholder->SetFollowingOuterPunct(saveFollOuterPunct);
				pPlaceholder->m_follPunct = saveFollPunct;
				pPlaceholder->SetInlineNonbindingEndMarkers(saveInlineNonbindingEndMkr);
				pPlaceholder->SetInlineBindingEndMarkers(saveInlineBindingEndMkrs);
				pPlaceholder->SetEndMarkers(saveEndMarkers);

				pPlaceholder->m_bEndRetranslation = TRUE;
				if (bBoundary)
				{
					pEndSrcPhrase->m_bBoundary = FALSE;
					pPlaceholder->m_bBoundary = TRUE;
				}
				if (bFootnoteEnd)
				{
					pEndSrcPhrase->m_bFootnoteEnd = FALSE;
					pPlaceholder->m_bFootnoteEnd = TRUE;
				}
				if (bEndFreeTrans)
				{
					pEndSrcPhrase->m_bEndFreeTrans = FALSE;
					pPlaceholder->m_bEndFreeTrans = TRUE;
				}
			}
		} // end of for loop: for (index = 0; index < nExtras; index++)
	} // end of TRUE block for test: if (nExtrasLong > 0)

	// before we insert the target text into each instance, we need to remove target
	// punctuation from each target text substring, and store the result in a different
	// wxArrayString, these elements will be assigned to m_adaption members
	size_t numElements = (int)numTgtElements;
	size_t i = 0; // a new local index
	wxArrayString arrTgtNoPuncts;
	for (i = 0; i < numElements; i++)
	{
		wxString nopuncts = arrTgtText.Item(i);
		m_pView->RemovePunctuation(m_pApp->GetDocument(), &nopuncts, from_target_text); // uses ParseWord()
		arrTgtNoPuncts.Add(nopuncts);
	}

	// handle the first CSourcePhrase instance, then loop over the rest
	pSrcPhrase->m_targetStr = arrTgtText.Item(0);
	pSrcPhrase->m_adaption = arrTgtNoPuncts.Item(0);
	// get the second position; since it was a merger, this instance will always exist
	pos = pos->GetNext();
	i = 1;
	do {
		// fill out the send and later instances
		pSrcPhrase = pos->GetData();
		pSrcPhrase->m_targetStr = arrTgtText.Item(i);
		pSrcPhrase->m_adaption = arrTgtNoPuncts.Item(i);
		pos = pos->GetNext();
		i++;
	} while (pos != NULL && i < numElements);
}
*/

void CRetranslation::DeleteSavedSrcPhraseSublist(SPList* pSaveList)
{
	// refactor 13Mar09; these are shallow copies, and have no partner piles,
	// so nothing to do here
	if (pSaveList->GetCount() > 0)
	{
		SPList::Node* pos = pSaveList->GetFirst();
		wxASSERT(pos != 0);
		while (pos != 0)
		{
			CSourcePhrase* pSP = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			if (pSP != NULL)
			{
				// don't want memory leaks
				if (pSP->m_pMedialMarkers != NULL) // whm 11Jun12 added NULL test
					delete pSP->m_pMedialMarkers;
				pSP->m_pMedialMarkers = (wxArrayString*)NULL;
				if (pSP->m_pMedialPuncts != NULL) // whm 11Jun12 added NULL test
					delete pSP->m_pMedialPuncts;
				pSP->m_pMedialPuncts = (wxArrayString*)NULL;
				pSP->m_pSavedWords->Clear(); // remove pointers only
				if (pSP->m_pSavedWords != NULL) // whm 11Jun12 added NULL test
					delete pSP->m_pSavedWords;
				pSP->m_pSavedWords = (SPList*)NULL;
				if (pSP != NULL) // whm 11Jun12 added NULL test
					delete pSP;
				pSP = (CSourcePhrase*)NULL;
			}
		}
		pSaveList->Clear();
	}
	if (pSaveList != NULL) // whm 11Jun12 added NULL test
		delete pSaveList; // don't leak memory
	pSaveList = (SPList*)NULL;
}

// The padding is done in the main list of source phrases, on the [App]. pSrcPhrases is the
// pointer to this list; nEndSequNumber is the sequence number of the last CSourcePhrase
// instance of the selected source text (and the padding is required when there are more
// words in the target text than the source piles can accomodate). nNewCount is the number
// of target text words - it could be less, more, or the same as the number piles selected
// (we test internally and act accordingly), and nCount is the number of CSourcePhrase
// instances after all nulls removed, and mergers unmerged - both of which were done in the
// caller beforehand. We have to be careful if nEndSequNumber is equal to GetMaxIndex()
// value, because insertion of null source phrases has to take place before a sourcephrase
// instance which would not exist, so we must detect this and temporarily add an extra
// CSourcePhrase instance at the end of the main list, do the insertions preceding it, then
// remove it.
// BEW updated 17Feb10 for support of doc version 5 (no changes were needed)
void CRetranslation::PadWithNullSourcePhrasesAtEnd(CAdapt_ItDoc* pDoc,
							SPList* pSrcPhrases,int nEndSequNum,int nNewCount,int nCount)
{
	// refactored 16Apr09
	int nEndIndex = 0;
	int nSaveActiveSN = m_pApp->m_nActiveSequNum;
	if (nNewCount > nCount)
	{
		// null source phrases are needed for padding
		int nExtras = nNewCount - nCount;

        // check we are not at the end of the list of CSourcePhrase instances, if we are we
        // will have to add an extra one so that we can insert before it, then remove it
        // later.
		if (nEndSequNum == m_pApp->GetMaxIndex())
		{
            // we are at the end, so we must add a dummy sourcephrase; note,
            // m_nActiveSequNum and the caller's nSaveActiveSequNum values will almost
            // certainly be greater than GetMaxIndex() value, and so we must not use these
            // until we adjust them in the caller later on. So we can ignore the active
            // location, and just temporarily treat it as the last pile in the document.
			CSourcePhrase* pDummySrcPhrase = new CSourcePhrase;
			pDummySrcPhrase->m_srcPhrase = _T("dummy"); // something needed, so a pile width
			// can be computed
			pDummySrcPhrase->m_key = pDummySrcPhrase->m_srcPhrase;
			nEndIndex = m_pApp->GetMaxIndex() + 1;
			pDummySrcPhrase->m_nSequNumber = nEndIndex;
			SPList::Node* posTail;
			posTail = pSrcPhrases->Append(pDummySrcPhrase);
			posTail = posTail; // avoid warning
			// we need a valid layout which includes the new dummy element on its own pile
			m_pApp->m_nActiveSequNum = nEndIndex; // temporary location only
#ifdef _NEW_LAYOUT
			pDoc->CreatePartnerPile(pDummySrcPhrase); // must have a CPile instance for
											// index of pDummySrcPhrase, or GetPile() call
											// below will fail
			m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
			m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
			m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum); // temporary active location

			// now we can do the insertions, preceding the dummy end pile
			CPile* pPile = m_pView->GetPile(nEndIndex);
			wxASSERT(pPile != NULL);
			m_pApp->GetPlaceholder()->InsertNullSourcePhrase(pDoc,pPile,nExtras,FALSE,TRUE); // FALSE for restoring
			// the phrase box, TRUE for doing it for a retranslation, and default TRUE for
			// bInsertBefore flag at end

			// now remove the dummy element, and make sure memory is not leaked!
#ifdef _NEW_LAYOUT
			pDoc->DeletePartnerPile(pDummySrcPhrase);
#endif
			if (pDummySrcPhrase->m_pSavedWords != NULL) // whm 11Jun12 added NULL test
				delete pDummySrcPhrase->m_pSavedWords;
			pDummySrcPhrase->m_pSavedWords = (SPList*)NULL;
			if (pDummySrcPhrase->m_pMedialMarkers != NULL) // whm 11Jun12 added NULL test
				delete pDummySrcPhrase->m_pMedialMarkers;
			pDummySrcPhrase->m_pMedialMarkers = (wxArrayString*)NULL;
			if (pDummySrcPhrase->m_pMedialPuncts != NULL) // whm 11Jun12 added NULL test
				delete pDummySrcPhrase->m_pMedialPuncts;
			pDummySrcPhrase->m_pMedialPuncts = (wxArrayString*)NULL;
			SPList::Node *pLast = pSrcPhrases->GetLast();
			pSrcPhrases->DeleteNode(pLast);
			if (pDummySrcPhrase != NULL) // whm 11Jun12 added NULL test
				delete pDummySrcPhrase;

			// get another valid layout
			m_pApp->m_nActiveSequNum = nSaveActiveSN; // restore original location
		}
		else
		{
            // not at the end, so we can proceed immediately; get the insertion location's
            // pile pointer
			CPile* pPile = m_pView->GetPile(nEndSequNum + 1); // nEndIndex is out of scope here
			wxASSERT(pPile != NULL);
			m_pApp->GetPlaceholder()->InsertNullSourcePhrase(pDoc,pPile,nExtras,FALSE,TRUE); // FALSE is for
			// restoring the phrase box, TRUE is for doing it for a retranslation
			m_pApp->m_nActiveSequNum = nSaveActiveSN;
		}
	}
	else
		; // no padding needed
}

// BEW 17Feb10, updated for support of doc version 5 (no changes needed)
void CRetranslation::ClearSublistKBEntries(SPList* pSublist)
{
	SPList::Node* pos = pSublist->GetFirst();
	wxString emptyStr = _T("");
	while (pos != NULL)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();

		m_pApp->m_pKB->GetAndRemoveRefString(pSrcPhrase,emptyStr, useGlossOrAdaptationForLookup);
		pSrcPhrase->m_bRetranslation = FALSE; // make sure its off
		pSrcPhrase->m_bHasKBEntry = FALSE;	  // ditto
	}
}

// BEW 17Feb10, updated for support of doc version 5 (no changes needed)
void CRetranslation::InsertSublistAfter(SPList* pSrcPhrases, SPList* pSublist, int nLocationSequNum)
{
	SPList::Node* pos = pSrcPhrases->Item(nLocationSequNum);
	wxASSERT(pos != 0);
	SPList::Node* pos1 = pSublist->GetLast();
	wxASSERT(pos1 != 0);
	// Get a node called posNextHigher which points to the next node beyond pos
	// in pSrcPhrases and use its position in the Insert() call (which only inserts
	// BEFORE the indicated position). The result should be that the insertions
	// will get placed in the list the same way that MFC's InsertAfter() places them.
	SPList::Node* newInsertBeforePos = pos->GetNext();
	while (pos1 != 0)
	{
		CSourcePhrase* pSPhr = (CSourcePhrase*)pos1->GetData();
		pos1 = pos1->GetPrevious();
		wxASSERT(pSPhr != NULL);
		// wxList has no equivalent to InsertAfter(). The wxList Insert() method
		// inserts the new node BEFORE the current position/node. To emulate what
		// the MFC code does, we insert before using newInsertBeforePos.
		// wx note: If newInsertBeforePos is NULL, it means the insert position is
		// at the end of the list; in this case we just append the item to the end
		// of the list.
		if (newInsertBeforePos == NULL)
			pSrcPhrases->Append(pSPhr);
		else
			pSrcPhrases->Insert(newInsertBeforePos,pSPhr);

		// BEW added 13Mar09 for refactored layout
		m_pApp->GetDocument()->CreatePartnerPile(pSPhr);

		// since we must now insert before the inserted node above, we need to get a
		// previous node (which will actually be the just inserted source phrase)
		newInsertBeforePos = newInsertBeforePos->GetPrevious();

		// If the m_bNotInKB flag is FALSE, we must re-store the translation in
		// the KB. We can get the former translation string from the m_adaption member.
		if (!pSPhr->m_bNotInKB && !pSPhr->m_adaption.IsEmpty())
		{
			bool bOK = m_pApp->m_pKB->StoreText(pSPhr,pSPhr->m_adaption);
			if (!bOK)
			{
				// never had a problem here, so this message can stay in English
				wxMessageBox(_T(
				"Warning: redoing the StoreText operation failed in OnButtonRetranslation\n"),
				_T(""), wxICON_EXCLAMATION | wxOK);
			}
		}
	}
}

// BEW 16Feb10, no changes needed for support of doc version 5
bool CRetranslation::IsConstantType(SPList* pList)
{
	SPList::Node* pos = pList->GetFirst();
	if (pos == NULL)
	{
		wxMessageBox(_T(
		"Error accessing sublist in IsConstantType function\n"),
		_T(""), wxICON_EXCLAMATION | wxOK);
		wxASSERT(FALSE);
		return FALSE;
	}
	CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
	pos = pos->GetNext();
	TextType firstType = pSrcPhrase->m_curTextType;
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		TextType type = pSrcPhrase->m_curTextType;
		if (type != firstType)
			return FALSE;
	}

    // if we get here, it's all one type; so if that type is also the footnote textType
    // then set the m_bInsertingWithinFootnote flag (we want to propragate a footnote type
    // into any padding with null source phrases, in case the user wants to get interlinear
    // RTF output with footnote text suppressed)
	if (pSrcPhrase->m_curTextType == footnote)
		m_bInsertingWithinFootnote = TRUE;
	return TRUE;
}

// pSrcPhrases is the document's list; nCurCount is how many elements are now in the
// 'selection' as modified by removing all nulls and unmerging all mergers,
// nStartingSequNum is where the checking will start for doing the removals of those
// determined to be unwanted, and pSublist is the pointer to the sublist which stores the
// original elements we are in the process of restoring to the main list on the app
//
// BEW 17Feb10, updated for support of doc version 5 (no changes needed)
void CRetranslation::RemoveUnwantedSourcePhraseInstancesInRestoredList(SPList* pSrcPhrases,
								int nCurCount, int nStartingSequNum,SPList* pSublist)
{
	SPList::Node* pos = pSrcPhrases->Item(nStartingSequNum); // first one's position
	int count = 0;
	while (pos != NULL && count < nCurCount)
	{
		SPList::Node* savePos = pos;
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase != NULL);

		// BEW added 13Mar09 for refactor of layout; delete its partner pile too
		m_pApp->GetDocument()->DeletePartnerPile(pSrcPhrase);

        // before we can delete it, we must check it's not one of those (minimal) source
        // phrases which is in the sublist of a merged source phrase in the saved list - if
        // we deleted it, we'd be deleting something we must retain; but we would want to
        // remove it's pointer from the list (because it's to be accessed only by the
        // m_pSavedWords sublist of whichever merged source phrase was formed from it)
		bool bCanDelete = TRUE;
		SPList::Node* posSaveList = pSublist->GetFirst();
		wxASSERT(posSaveList != 0);
		while (posSaveList != 0)
		{
			CSourcePhrase* pSP = (CSourcePhrase*)posSaveList->GetData();
			posSaveList = posSaveList->GetNext();
			wxASSERT(pSP != 0);
			if (pSP->m_nSrcWords == 1)
				continue; // its a copy without a sublist, so ignore it
			else
			{
                // its a copy with a sublist, so see if any source phrase in the sublist is
                // a match for the one we wish to delete; if it is, don't delete it, just
                // remove its pointer from the m_pSourcePhrases list only
				SPList* pL = pSP->m_pSavedWords;
				wxASSERT(pL->GetCount() > 1);
				SPList::Node* pos4 = pL->GetFirst();
				wxASSERT(pos4 != 0);
				while (pos4 != 0)
				{
					CSourcePhrase* pSPhr = (CSourcePhrase*)pos4->GetData();
					pos4 = pos4->GetNext();
					wxASSERT(pSPhr != 0);
					if (pSPhr == pSrcPhrase)
					{
						// we have a match
						bCanDelete = FALSE;
						break;
					}
				}
			}
		}
		if (bCanDelete)
		{
			if (pSrcPhrase->m_pMedialMarkers->GetCount() > 0)
			{
				pSrcPhrase->m_pMedialMarkers->Clear(); // can clear the strings safely
			}
			if (pSrcPhrase->m_pMedialMarkers != NULL) // whm 11Jun12 added NULL test
				delete pSrcPhrase->m_pMedialMarkers;
			pSrcPhrase->m_pMedialMarkers = (wxArrayString*)NULL;

			if (pSrcPhrase->m_pMedialPuncts->GetCount() > 0)
			{
				pSrcPhrase->m_pMedialPuncts->Clear(); // can clear the strings safely
			}
			if (pSrcPhrase->m_pMedialPuncts != NULL) // whm 11Jun12 added NULL test
				delete pSrcPhrase->m_pMedialPuncts;
			pSrcPhrase->m_pMedialPuncts = (wxArrayString*)NULL;

			// don't delete any saved CSourcePhrase instances forming a phrase (and these
			// will never have medial puctuation nor medial markers nor will they store
			// any saved minimal phrases since they are CSourcePhrase instances for single
			// words only) - just clear the pointers
			if (pSrcPhrase->m_pSavedWords->GetCount() > 0)
			{
				pSrcPhrase->m_pSavedWords->Clear(); // just remove the pointers
			}
			if (pSrcPhrase->m_pSavedWords != NULL) // whm 11Jun12 added NULL test
				delete pSrcPhrase->m_pSavedWords;		// and delete the list from the heap
			pSrcPhrase->m_pSavedWords = (SPList*)NULL;

			// finally delete the source phrase copy itself
			if (pSrcPhrase != NULL) // whm 11Jun12 added NULL test
				delete pSrcPhrase;
			pSrcPhrase = (CSourcePhrase*)NULL;

			// remove its pointer from the list
			pSrcPhrases->DeleteNode(savePos);

			// augment the count of how many have been removed
			count++;
		}
		else
		{
			// this is one we cannot delete, but must just remove its pointer from the list
			pSrcPhrases->DeleteNode(savePos);

			// augment the count of how many have been removed
			count++;
		}
	}
}

// the pSrcPhrase supplied by the caller will be a newly created one, and therefore all its
// flags will have default values, and in particular the m_adaption and m_targetStr
// attributes will both be empty. So we must invoke a lookup of the single source word at
// the box location (which will be at the start of the new source text) in the KB to find a
// suitable adaptation to put in the target box; and this may, if there is more than one
// possible adaptation available, put up the Choose Translation dialog. (We can pinch
// suitable code from OnButtonRestore() and modify it a bit for here.) We set str in this
// function, and the caller then takes that and assigns it to pApp->m_targetPhrase.
// Modified, July 2003, for auto capitalization support
void CRetranslation::RestoreTargetBoxText(CSourcePhrase* pSrcPhrase,wxString& str)
{
	bool bGotTranslation;
	bool bNoError = TRUE;
	if (gbAutoCaps)
	{
		bNoError = m_pApp->GetDocument()->SetCaseParameters(pSrcPhrase->m_key);
	}

    // although this function strictly speaking is not necessarily invoked in the context
    // of an unmerge, the gbUnmergeJustDone flag being TRUE gives us the behaviour we want;
    // ie. we certainly DON'T want OnButtonRestore() called from here!
	gbUnmergeJustDone = TRUE; // prevent second OnButtonRestore() call from within
	// ChooseTranslation() within LookUpSrcWord() if user happens to
	// cancel the Choose Translation dialog (see CPhraseBox code)
	bGotTranslation = m_pApp->m_pTargetBox->LookUpSrcWord(m_pApp->m_pActivePile);
	gbUnmergeJustDone = FALSE; // clear flag to default value, since it is a global boolean
	wxASSERT(m_pApp->m_pActivePile); // it was created in the caller just prior to this
	// function being called
	if (bGotTranslation)
	{
        // we have to check here, in case the translation it found was a "<Not In KB>" - in
        // which case, we must display m_targetStr and ensure that the pile has an asterisk
        // above it, etc
		if (translation == _T("<Not In KB>"))
		{
			str.Empty(); // phrase box must be shown empty
			m_pApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry = FALSE;
			m_pApp->m_pActivePile->GetSrcPhrase()->m_bNotInKB = TRUE;
			str = m_pApp->m_pActivePile->GetSrcPhrase()->m_targetStr;
		}
		else
		{
			str = translation; // set using the global var, set in LookUpSrcWord call
		}

		if (gbAutoCaps && gbSourceIsUpperCase)
		{
			bNoError = m_pApp->GetDocument()->SetCaseParameters(str,FALSE);
			if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
			{
				// change first letter to upper case
				str.SetChar(0, gcharNonSrcUC);
			}
		}
	}
	else // no translation found
	{
		// do the copy of source instead, or nothing if Copy Source flag is not set
		if (m_pApp->m_bCopySource)
		{
			// copy source key
			str = m_pView->CopySourceKey(m_pApp->m_pActivePile->GetSrcPhrase(),m_pApp->m_bUseConsistentChanges);
			m_pApp->m_pTargetBox->m_bAbandonable = TRUE;
		}
		else
		{
			str.Empty();
		}
	}
}



void CRetranslation::GetRetranslationSourcePhrasesStartingAnywhere(
								CPile* pStartingPile, CPile*& pFirstPile, SPList* pList)
{
	// refactored 16Apr09 (removed goto statement also & adjusted syntax accordingly)
	pFirstPile = pStartingPile; // first pile in the retranslation section, initialize to
	// the one clicked or selected, in case the first was clicked
	// or selected; in which case the first loop below will not be
	// entered and we'd otherwise not set pFirstPile at all

	CSourcePhrase* pSrcPhrase = pStartingPile->GetSrcPhrase();
	// wx Note: wxList::Insert() Inserts object at front of list, equiv to CObList's AddHead()
	pList->Insert(pSrcPhrase); // add the one we've found already
	CPile* pPile = pStartingPile;
	if (!pFirstPile->GetSrcPhrase()->m_bBeginRetranslation)
	{
		// do this block if we are not at beginning of the retranslation as
		// previously constituted
		while ((pPile = m_pView->GetPrevPile(pPile)) != NULL && pPile->GetSrcPhrase()->m_bRetranslation)
		{
			pList->Insert(pPile->GetSrcPhrase());
			pFirstPile = pPile; // last time thru this loop leaves this variable with the value
			// we want

			// go back only to the source phrase with m_bBeginRetranslation set TRUE, but if none
			// such is found, then continue until the loop test gives an exit
			if (pPile->GetSrcPhrase()->m_bBeginRetranslation)
				break; // we are at the start of the retranslation section as earlier constituted
		}
	}
	pPile = pStartingPile;
	if (pStartingPile->GetSrcPhrase()->m_bEndRetranslation)
		return; // skip next block if we are at the end of the retranslation as previously
	// constituted

	while ((pPile = m_pView->GetNextPile(pPile)) != NULL && pPile->GetSrcPhrase()->m_bRetranslation)
	{
		pList->Append(pPile->GetSrcPhrase());

		// break when we come to one with m_bEndRetranslation flag set TRUE
		if (pPile->GetSrcPhrase()->m_bEndRetranslation)
			break;
	}
}

void CRetranslation::SetNotInKBFlag(SPList* pList,bool bValue)
{
	wxASSERT(pList != NULL);
	SPList::Node* pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	while (pos != 0)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase != NULL);
		pSrcPhrase->m_bNotInKB = bValue;
	}
}

void CRetranslation::SetRetranslationFlag(SPList* pList,bool bValue)
{
	wxASSERT(pList != NULL);
	SPList::Node* pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	while (pos != 0)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase != NULL);
		pSrcPhrase->m_bRetranslation = bValue;
	}
}

// old code was based on code in TokenizeText in doc file; new code is based on code in
// RemovePunctuation() which is much smarter & handles word-building punctuation properly
/* BEW deprecated 12May11, it didn't handle docV5 data well enough, nor all of it
void CRetranslation::RestoreOriginalPunctuation(CSourcePhrase *pSrcPhrase)
{
	wxString src = pSrcPhrase->m_srcPhrase;

	// first, clear any punctuation resulting from the retranslation
	pSrcPhrase->m_precPunct.Empty();
	pSrcPhrase->m_follPunct.Empty();
	pSrcPhrase->m_pMedialPuncts->Clear();
	pSrcPhrase->m_bHasInternalPunct = FALSE;

	wxString punctSet = m_pApp->m_punctuation[0]; // from version 1.3.6, contains spaces
	// as well as punct chars
	// ensure there is at least one space, so "<< <" and similar sequences
	// get spanned properly
	if (punctSet.Find(_T(' ')) == -1)
	{
		// no space in it yet, so put one there (done on the local variable only,
		// so m_punctSet[0] unchanged)
		punctSet += _T(' ');
	}

	if (FindOneOf(src,punctSet) == -1)
		return; // there are no punctuation chars in the string (except possibly
	// word-building ones) and no spaces either

	// get the preceding punctuation
	wxString precStr;
	precStr.Empty();
	int len = 0;
	wxString tempStr;
	tempStr.Empty();
	int totalLength = src.Length();
	precStr = SpanIncluding(src,punctSet);
	len = precStr.Length();
	if (len > 0)
	{
		// has initial punctuation, so strip it off and store it
		pSrcPhrase->m_precPunct = precStr;
		tempStr = pSrcPhrase->m_srcPhrase.Mid(len); // tempStr holds the rest after the punct.
		totalLength -= len; // reduce by size of punctuation chars in set
	}
	else
	{
		// no initial punctuation, so copy the lot to tempStr
		tempStr = pSrcPhrase->m_srcPhrase;
	}

	// now handle any following punctuation
	wxString key = tempStr;
	key = MakeReverse(key);
	wxString follStr;
	follStr.Empty();
	totalLength = key.Length();
	follStr = SpanIncluding(key,punctSet);
	len = follStr.Length();
	if (len > 0)
	{
		// has following punctuation so strip it off, store it (after reversing it again)
		follStr = MakeReverse(follStr);
		pSrcPhrase->m_follPunct = follStr;
		key = key.Mid(len);
		key = MakeReverse(key);
		pSrcPhrase->m_key = key;
	}
	else
	{
		// no following punctuation
		pSrcPhrase->m_key = tempStr;
	}
}
*/

///////////////////////////////////////////////////////////////////////////////
// Event handlers
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks	Handler for the Retranslation button pressed event.
/////////////////////////////////////////////////////////////////////////////////
// BEW 17Feb10, updated for support of doc version 5 (no changes needed, but the
// InsertNullSourcePhrase() function called from PadWithNullSourcePhrasesAtEnd() had to
// have a number of changes to handle placeholder inserts for retranslation and for manual
// placeholder inserting & the left or right association choice)
void CRetranslation::OnButtonRetranslation(wxCommandEvent& event)
{
	// refactored 16Apr09
    // Since the Do a Retranslation toolbar button has an accelerator table hot key (CTRL-R
    // see CMainFrame) and wxWidgets accelerator keys call menu and toolbar handlers even
    // when they are disabled, we must check for a disabled button and return if disabled.
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxASSERT(pFrame != NULL);
	wxAuiToolBarItem *tbi;
	tbi = pFrame->m_auiToolbar->FindTool(ID_BUTTON_RETRANSLATION);
	// Return if the toolbar item is hidden
	if (tbi == NULL)
	{
		return;
	}
	// Return if this toolbar item is disabled
	if (!pFrame->m_auiToolbar->GetToolEnabled(ID_BUTTON_RETRANSLATION))
	{
		::wxBell();
		return;
	}

	if (gbIsGlossing)
	{
		wxMessageBox(_(
					   "This particular operation is not available when you are glossing."),
					 _T(""), wxICON_INFORMATION | wxOK);
		return;
	}

	// BEW 3Oct11, Bill found that it is possible to induce a crash by having the phrase
	// box somewhere, doing ALT+downarrow to get a placeholder inserted, and then do an
	// immediate ALT_uparrow (which invokes the OnButtonRetranslation() handler without
	// there being any selection defined) - the lack of a selection then leaves pList (see
	// below) empty of CSourcePhrases, and that in turn causes UnmergeMergersInSublist()
	// to crash, because it assumes pList will be populated and it tries to access it's
	// first CSourcePhrase instance - returns a NULL pointer for the pos SPList:Node*
	// calculation, and so the attempt to get the srcphrase gives an invalid access error.
	// The error results from the CTRL+uparrow handler in OnSyskeyUp() in PhraseBox.cpp,
	// which creates a 1-cell selection, for the placehandler pile just created. Best way
	// to fix this is to test for the placeholder, and exit OnButtonRetranslation() if
	// that's all there was in the selection. We also will test for an empty selection and
	// exit if so as well. We want to let other single-pile selections pass through
	if (m_pApp->m_selection.IsEmpty())
	{
		::wxBell();
		return;
	}
	else
	{
		int selCount = m_pApp->m_selection.GetCount();
		if (selCount == 1)
		{
			CCellList::Node* pos = m_pApp->m_selection.GetFirst();
			CCell* pCell = (CCell*)pos->GetData();
			CPile* pPile = pCell->GetPile();
			CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
			if (pSrcPhrase->m_bNullSourcePhrase || pSrcPhrase->m_key == _T("..."))
			{
				// must have been a CTRL+uparrow keypress at a placeholder - disallow this
				::wxBell();
				return;
			}
		}
	}

	SPList* pList = new SPList; // list of the selected CSourcePhrase objects
	wxASSERT(pList != NULL);
	SPList* pSrcPhrases = m_pApp->m_pSourcePhrases;
	CPile* pStartingPile = NULL;

    // determine the active sequ number, so we can determine whether or not the active
    // location lies within the selection (if its not in the selection, we will need to
    // recreate the phrase box at the former active location when done - be careful,
    // because if the active location lies after the selection and the selection contains
    // null src phrases or merged phrases, then the value of nFormerActiveSequNum will need
    // to be updated as we remove null src phrases and / or unmerge merged phrases)
	int nSaveActiveSequNum = m_pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber;

	CSourcePhrase* pSrcPhrase;
	wxString strAdapt; // accumulates the existing adaptation text for the selection
	strAdapt.Empty();

	wxString str; // a temporary storage string
	str.Empty();
	wxString str2; // second temporary storage string
	str2.Empty();
	wxString strSource; // the source text which is to be retranslated
	strSource.Empty();
	CCellList::Node* pos = m_pApp->m_selection.GetFirst();
	int nCount = m_pApp->m_selection.GetCount(); // number of src phrase instances in selection

	if (nCount == (int)m_pApp->m_pSourcePhrases->GetCount())
	{
		wxMessageBox(_(
					   "Sorry, for a retranslation your selection must not include all the document contents - otherwise there would be no possible place for the phrase box afterwards. Shorten the selection then try again."),
					 _T(""),wxICON_INFORMATION | wxOK);
		return;
	}

	CCell* pCell = (CCell*)pos->GetData();
	CPile* pPile = pCell->GetPile(); // get the pile first in the selection
	pos = pos->GetNext(); // needed for our CCellList to effect MFC's GetNext()

	pStartingPile = pPile;
	pSrcPhrase = pPile->GetSrcPhrase();

	int nSaveSequNum = pSrcPhrase->m_nSequNumber; // save its sequ number, everything
        // depends on this - its the first in the sublist list get a list of the selected
        // CSourcePhrase instances (some might not be minimal ones so if this is the case
        // we must later restore them to minimal ones, and some might be placeholders, so
        // these must be later eliminated after their text, if any, is preserved & any
        // punctuation transferred) and also accumulate the words in the source and target
        // text into string variables
	GetSelectedSourcePhraseInstances(pList, strSource, strAdapt);

    // check that the selection is text of a single type - if it isn't, then tell the user
    // and abandon the operation
	bool bConstType = IsConstantType(pList);
	if (!bConstType)
	{
		wxMessageBox(_(
"Sorry, the selection contains text of more than one type. Select only one text type at a time. The operation will be ignored."),
		_T(""), wxICON_EXCLAMATION | wxOK);
		m_pView->RemoveSelection();
		if (pList != NULL) // whm 11Jun12 added NULL test
			delete pList; // BEW 3Oct11: beware, don't try deleting the ptrs in pList,
					  // they are shallow copies of some of those in m_pSourcePhrases,
					  // and so deleting them would destroy part of the document;
					  // similarly in other places below in this function
		pList = (SPList*)NULL;
		m_pApp->m_pTargetBox->SetFocus();
		m_pApp->m_pTargetBox->SetSelection(m_pApp->m_nStartChar,m_pApp->m_nEndChar);
		m_pView->Invalidate();
		m_pLayout->PlaceBox();
		return;
	}

    // check for a retranslation in the selection, and abort the retranslatiaon operation
    // if there is one
	if (IsRetranslationInSelection(pList))
	{
		wxMessageBox(_(
"Sorry, but this operation is not permitted when the selection contains any part of a retranslation. First remove the retranslation and then try again."),
		_T(""), wxICON_EXCLAMATION | wxOK);
		pList->Clear();
		if (pList != NULL) // whm 11Jun12 added NULL test
			delete pList;
		pList = (SPList*)NULL;
		m_pView->RemoveSelection();
		m_pApp->m_pTargetBox->SetFocus();
		m_pApp->m_pTargetBox->SetSelection(m_pApp->m_nStartChar,m_pApp->m_nEndChar);
		m_pView->Invalidate();
		m_pLayout->PlaceBox();
		return;
	}

    // need to clobber the selection here, so the selection globals will be set to -1,
    // otherwise RecalcLayout will fail at its RestoreSelection() call; and any unmergers
    // or other layout changes done immediately below will invalidate layout pointers which
    // RemoveSelection() relies on, and produce a crash.
	m_pView->RemoveSelection();

    // copy the list to a 2nd list for saving the original state, in case the user hits the
    // Cancel button in the dialog, and save the old sequ num value for the active
    // location; we don't save copies of the pointers, but instead use the copy constructor
    // to make fresh copies of the original selection's source phrases - but note that the
    // copy constructor (and also operator=) only copies pointers for any CSourcePhrases in
    // each source phrase's m_pSavedWords sublist - which has implications for below (in
    // particular, when deleting the copied list, we must not delete in the sublists, but
    // only remove the pointers, otherwise the originals will have hanging pointers)
	SPList* pSaveList = new SPList;
	wxASSERT(pSaveList != NULL);
	CopySourcePhraseList(pList,pSaveList);

	// BEW added 20Mar07: to suppress KB entry removal during a retranslation or edit of same
	m_bIsRetranslationCurrent = TRUE;

    // deliberately abandon contents of box at active loc'n - we'll reconstitute it as
    // necessary later, depending on where we want to place the targetBox. But before we
    // abandon it, we must first check if the active location is outside the selection -
    // since there could be a just-edited entry in the phrase box which is not yet entered
    // in the knowledge base, and the active location's source phrase doesn't yet have its
    // m_adaption and m_targetStr members updated, so we must check for this condition and
    // if it obtains then we must first update everything at the active location before we
    // empty pApp->m_targetPhrase, etc. For versions later than 1.2.9, the code below is
    // different. We need an unconditional store, because if outside the selection, we must
    // update as the above explanation explains; but for an active location within the
    // selection, we still must update because (a) there is one refString removal already
    // by virtue of the fact that it is the active location (ie. the box is there), and (b)
    // a further refString removal will be done in the UnmergeMergersInSublist() call, and
    // this means the refString for the active location gets removed twice unless we
    // prevent it (it only should be removed once). So by doing an unconditional store, we
    // bump the refCount at the active loc, and then the UnmergeMergersInSublist() call can
    // decrement it again, keeping the count correct. We need to do this also in
    // OnEditSourceText().
	if (m_pApp->m_pActivePile != NULL)
	{
		// the active location is not within the retranslation section, so update before
		// throwing it all out
		m_pView->MakeTargetStringIncludingPunctuation(m_pApp->m_pActivePile->GetSrcPhrase(),
														m_pApp->m_targetPhrase);
		m_pView->RemovePunctuation(pDoc,&m_pApp->m_targetPhrase,from_target_text);
		gbInhibitMakeTargetStringCall = TRUE;
		bool bOK = m_pApp->m_pKB->StoreText(m_pApp->m_pActivePile->GetSrcPhrase(),
											m_pApp->m_targetPhrase);
		gbInhibitMakeTargetStringCall = FALSE;
		if (!bOK)
		{
			m_bIsRetranslationCurrent = FALSE;
			return; // can't proceed until a valid adaption (which could be null) is
			// supplied for the former active pile's srcPhrase
		}
		else
		{
            // make the former strip be invalid - new layout code will then tweak the
            // layout from that point on; mark it invalid as well as the current one
			int nFormerStrip = m_pApp->m_pActivePile->GetStripIndex();
			pDoc->ResetPartnerPileWidth(m_pApp->m_pActivePile->GetSrcPhrase()); // & mark
															// the active strip invalid
			int nCurStripIndex = pStartingPile->GetStripIndex();
			if (nCurStripIndex != nFormerStrip)
			{
				CStrip* pFormerStrip = (CStrip*)m_pLayout->GetStripArray()->Item(nFormerStrip);
				CPile* pItsFirstPile = (CPile*)pFormerStrip->GetPilesArray()->Item(0);
				CSourcePhrase* pItsFirstSrcPhrase = pItsFirstPile->GetSrcPhrase();
				pDoc->ResetPartnerPileWidth(pItsFirstSrcPhrase,TRUE); // TRUE is
												// bNoActiveLocationCalculation
			}
		}
	}
	m_pApp->m_targetPhrase.Empty();
	if (m_pApp->m_pTargetBox != NULL)
	{
		m_pApp->m_pTargetBox->ChangeValue(m_pApp->m_targetPhrase);
	}

    // determine the value for the active sequ number on exit, so we will know where to
    // place the phrase box on return to the caller; we'll place the phrase box at the
    // first location after the retranslation - provided the active location was within the
    // selection; but if it lay outside the selection, we will need to restore it to
    // wherever it was.
	SPList::Node* lastPos = pList->GetLast();
	int nEndSequNum = lastPos->GetData()->m_nSequNumber;
	bool bActiveLocWithinSelection = FALSE;
	if (nSaveActiveSequNum >= nSaveSequNum && nSaveActiveSequNum <= nEndSequNum)
		bActiveLocWithinSelection = TRUE;
	bool bActiveLocAfterSelection = FALSE;
	if (nSaveActiveSequNum > nEndSequNum)
		bActiveLocAfterSelection = TRUE;

    // check for any null source phrases in the selection, and delete any found from both
    // the temporary list (pList), and from the original source phrases list on the app
	// (see above). The outer loop removes only the next placeholder found in pList, so we
	// need a loop to get rid of them all. The Untransfer... call only needs to do its job
	// once, so we use a flag to make that happen
	bool bDoneAlready = FALSE;
	while (IsNullSrcPhraseInSelection(pList))
	{
		// BEW 11Oct10, RemoveNullSrcPhraseFromLists() does not call RemoveNullSrcPhrase()
		// and so it's smarts re marker and punctuation transfer are not used here - we
		// would only get away with this (ie. without losing some punct or marker data) here if
		// the user were to do his retranslations on selections which don't contain
		// placeholders. So I've coded a function to undo previous transfers which can be
		// called first, in a loop, and then all will be well for the subsequent removal(s)
		if (bDoneAlready == FALSE)
		{
			SPList::Node* pos = pList->GetFirst();
			CSourcePhrase* pSP = NULL;
			while (pos != NULL)
			{
				pSP = pos->GetData();
				if (pSP->m_bNullSourcePhrase)
				{
					m_pApp->GetPlaceholder()->UntransferTransferredMarkersAndPuncts(pSrcPhrases, pSP);
				}
				pos = pos->GetNext();
			}
			bDoneAlready = TRUE;
		}
		m_pApp->GetPlaceholder()->RemoveNullSrcPhraseFromLists(pList, pSrcPhrases, nCount,
							nEndSequNum, bActiveLocAfterSelection, nSaveActiveSequNum);
	}

    // at this point pList does not contain any null source phrases, and we have
    // accumulated any adaptations already typed into strAdapt. However, we might have
    // merged phrases in pList to be unmerged, and we have not yet removed the translation
    // for each pSrcPhrase in pList from the KB, so we must do those things next.
	UnmergeMergersInSublist(pList, pSrcPhrases, nCount, nEndSequNum, bActiveLocAfterSelection,
							nSaveActiveSequNum, TRUE, TRUE); // final 2 flags should take
	// default values (TRUE, and FALSE, respectively), but this leads to a
	// crash when there are unmergers to be done - so using TRUE,TRUE fixes it
	// (ie. the pList sublist needs to be updated here too)

    // now we can work out where to place the phrase box on exit from this function - it is
    // currently the nSaveActiveSequNum value, unless the active location was within the
    // selection, in which case we must make the active location the first pile after the
    // selection...
    // BEW added 8May09; if the active location was far from the retranslation (defined as
    // "more than 80 piles from its end"), it is probably better to make the final active
    // location be the CSourcePhrase instance immediately following the retranslation, and
    // set the global gnOldsequNum to the old sequence number value so the Back button can
    // later jump back to the old active location if the user wants
    gnOldSequNum = nSaveActiveSequNum;
	int nSaveOldSequNum = gnOldSequNum; // need this to avoid calls below clobbering the
	// value set
	if (bActiveLocWithinSelection || (abs(nEndSequNum - nSaveActiveSequNum) > 80))
		nSaveActiveSequNum = nEndSequNum + 1;

	// the src phrases in the sublist will not be saved to the KB (because we don't save
	// retranslations) so mark them as not being in the KB; similarly, set the
	// m_bRetranslation flag to TRUE
	SetNotInKBFlag(pList,TRUE);
	SetRetranslationFlag(pList,TRUE);

    // we must have a valid layout, so we have to recalculate it before we go any further,
    // because if preceding code unmerged formerly merged phrases, or if null phrases were
    // deleted, then the layout's pointers will be clobbered, and then if we move the
    // dialog about to be put up, accesses to Draw() for the cells, piles & strips will
    // fail.
	m_pApp->m_nActiveSequNum = nSaveActiveSequNum; // legally can be a wrong location, eg.
	// in the retrans, & it won't break
#ifdef _NEW_LAYOUT
	m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
	m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
	m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);

	// create the CRetranslationDlg dialog
	CRetranslationDlg dlg(m_pApp->GetMainFrame());
	dlg.Centre();

	// initialize the edit boxes
	dlg.m_sourceText = strSource;
	dlg.m_retranslation = strAdapt;
	wxString preceding;
	preceding.Empty();
	wxString following;
	following.Empty();
	wxString precedingTgt;
	precedingTgt.Empty();
	wxString followingTgt;
	followingTgt.Empty();
	m_pView->GetContext(nSaveSequNum,nEndSequNum,preceding,following,precedingTgt,followingTgt);
	dlg.m_preContextSrc = preceding;
	dlg.m_preContextTgt = precedingTgt;
	dlg.m_follContextSrc = following;
	dlg.m_follContextTgt = followingTgt;

	// BEW addition 08Sep08 for support of vertical editing
	bool bVerticalEdit_SuppressPhraseBox = FALSE;
	int nVerticalEdit_nExtras = 0;

    // wx version: The wx version was crashing as soon as this CRetranslationDlg was shown.
    // The crashes were in OnUpdateButtonRestore(), an unrelated update handler, because in
    // the code line
	//		CSourcePhrase* pSP = m_pApp->m_pActivePile->m_pSrcPhrase;
    // m_pActivePile was valid, but the m_pSrcPhrase was uninitialized. It seems the wx
    // version update handlers in wx are more robust than those of MFC, or don't get
    // blocked while a modal dialog is showing, I don't know. At first, to eliminate the
    // crash, I temporarily turned off the update handler before calling ShowModal(), and
    // activating it again after ShowModal returns farther below. Later I decided to
    // incorporate the SetMode() call in the AIModalDialog class upon which all Adapt It
    // modal dialogs are now based. The call is
    // wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED) which stops all background idle
    // processing, including wxUpdateUIEvent event handling
    //
    // show the dialog
	if (dlg.ShowModal() == wxID_OK)
	{
		SPList* pRetransList = new SPList;
		wxASSERT(pRetransList);
		wxString retrans = dlg.m_retranslation;
		int nNewCount = 0; // number of CSourcePhrase instances returned from the
		// tokenization operation

        // tokenize the retranslation (which is target text) into a list of new
        // CSourcePhrase instances on the heap (the m_srcPhrase and m_key members will then
        // contain target text - which a function further below will mine in order to
        // construct the punctuated and unpunctuated strings for the building of the
        // retranslation's target text); nSaveSequNum is the absolute sequence number for
        // first source phrase in the sublist - it is used to define the starting sequence
        // number to be stored on the first element of the sublist, and higher numbers on
        // succeeding ones
		// BEW changed 24Jan11 -- the legacy call used source text punctuation for parsing
		// the target text, which isn't appropriate if source and target text are not the
		// same. So I've used the alternative function which explicitly uses the target
		// text punctuation (when the final param is TRUE)
		//nNewCount = m_pView->TokenizeTextString(pRetransList,retrans,nSaveSequNum);
		nNewCount = m_pView->TokenizeTargetTextString(pRetransList,retrans,nSaveSequNum,TRUE);

		// augment the active sequ num if it lay after the selection
		if (bActiveLocAfterSelection && nNewCount > nCount)
			nSaveActiveSequNum += nNewCount - nCount;
		else
		{
			// augment it also if the active location lay within the selection
            // (BEW 11Oct10, I dunno why I long ago put this extra text here, if nNewCount
            // exceeds nCount, the augmentation won't put the new active sn value into the
            // 'safe' area beyond the retranslation anyway - I'll leave it though as the
            // later call to set the active location safely will take care of things)
			if (bActiveLocWithinSelection && nNewCount > nCount)
				nSaveActiveSequNum += nNewCount - nCount;
		}
		m_pApp->m_nActiveSequNum = nSaveActiveSequNum; // ensure any call to
									// InsertNullSrcPhrase() will work right

        // we must have a valid layout, so we have to recalculate it before we go any
        // further, because if preceding code unmerged formerly merged phrases, or if null
        // phrases were deleted, then the layout's pointers will be clobbered; but we won't
        // draw it yet because later we must ensure the active location is not within the
        // retranslation and set it safely before a final layout calculation to get it all
        // correct
#ifdef _NEW_LAYOUT
		m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
		m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
		m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);

		// get a new valid starting pile pointer
		pStartingPile = m_pView->GetPile(nSaveSequNum);
		wxASSERT(pStartingPile != NULL);

        // determine if we need extra null source phrases inserted, and insert them if we
        // do; nNewCount is how many we end up with, nCount is the retranslation span's
        // srcphrase instance count after unmergers and placeholder removal was done
		PadWithNullSourcePhrasesAtEnd(pDoc,pSrcPhrases,nEndSequNum,nNewCount,nCount);

		// **** Legacy comment -- don't delete, it documents how me need to make changes ****
        // copy the retranslation's words, one per source phrase, to the constituted
        // sequence of source phrases (including any null ones) which are to display it;
        // but ignore any markers and punctuation if they were encountered when the
        // retranslation was parsed, so that the original source text's punctuation
        // settings in the document are preserved. Export will get the possibly new
        // punctuation settings by copying m_targetStr, so we do not need to alter
        // m_precPunct and m_follPunct on the document's CSourcePhrase instances.
        //
		// *** New comment, 11Oct10, for support of new doc version 5 storage members,
		// m_follOuterPunct and the four inline markers' wxString members.
		// The legacy approach above fails to handle good punctuation handling because we
		// need to support punctuation following endmarkers as well as before them. In the
		// retranslation, we ask the user to type punctuation where it needs to be - that
		// won't be in the same places as in the source text of the selection; but the
		// punctuation typed will be parsed by TokenizeTextString() above into only two
		// members, m_precPunct and m_follPunct, so m_follOuterPunct will be ignored. And
		// since it is assured that the user won't type markers when doing the
		// retranslation we then have the problem of how to get the markers into the right
		// places when an SFM export of the target text is asked for. Our solution to this
		// dilemma is the following:
		// (1) When exporting pSrcPhrase instances NOT in a retranslation, the punctuation
		// and markers are reconstituted from the pSrcPhrase members by algorithm.
		// (2) When exporting pSrcPhrase instances in a retranslation, the m_targetStr
		// member is taken "as is" and the pSrcPhrase members for storing puncts (those
		// will be source text puncts, and not in the right places anyway) will be
		// ignored, and so what is on m_targetStr is used - just as the user typed it in
		// the retranslation. The code for building the output SFM target text will have
		// worked out which markers are "medial" to the retranslation, and will present
		// those to the user in a placement dialog - so he then can place each marker in
		// the place where he deems it should be - in this way, he can re-establish, say,
		// an inline endmarker between two consecutive 'following' punctuation characters.
		//
        // The implication of the above rules for export determine how I need to refactor
        // the BuildRetranslationSourcePhraseInstances() function. Now it has to generate
        // the correct m_srcPhrase (ie. with punctuation in its proper place), and store
        // that m_srcPhrase value in the current pSrcPhrase's m_targetStr member. Any
        // markers, even if the user typed some, are just to be ignored - at export time
        // he'll get the chance to place them appropriately - they will be collected as
        // 'medial markers' from the m_pSourcePhrase instances' involved in the
        // retranslation's span. Hence, we can leave the source text punctuations in the
        // pSrcPhrase instances in m_pSourcePhrases untouched, except for changing the
        // m_targetStr value as explained in the previous sentence.
		int nFinish = -1; // it gets set to a correct value in the following call
		BuildRetranslationSourcePhraseInstances(pRetransList,nSaveSequNum,nNewCount,
												nCount,nFinish);
        // delete the temporary list and delete the pointers to the CSourcePhrase
        // instances on the heap
		m_pView->DeleteTempList(pRetransList);

        // remove the unused saved original source phrase copies & their list too this
        // pSaveList list will possibly have copies which contain non-empty sublists,
        // especially in m_pSavedWords, and the source phrases pointed to by that list will
        // only have had their pointers copied, so we must not delete those pointers,
        // otherwise the originals will contain hanging pointers & we'll crash if we were
        // to retry the retranslation on the same data
		DeleteSavedSrcPhraseSublist(pSaveList);

        // set the active pile pointer - do it here (not earlier), after any null source
        // phrases have been inserted, otherwise if we are near the end of file, the
        // pointer could be invalid because no such pile exists yet. Also, since the
        // nSaveActiveSequNum could be greater than the possible max value (if we are
        // making a retranslation at the very end of the file), we must check and if
        // necessary put the active location somewhere before the retranslation
		// BEW addition 08Sep08 for support of vertical editing
		if (!gbVerticalEditInProgress)
		{
			// legacy behaviour
			m_bSuppressRemovalOfRefString = TRUE; // suppress RemoveRefString() call within
												  // PlacePhraseBox()
			bool bSetSafely = m_pView->SetActivePilePointerSafely(
				m_pApp,pSrcPhrases,nSaveActiveSequNum, m_pApp->m_nActiveSequNum,nFinish);
			m_bSuppressRemovalOfRefString = FALSE; // permit RemoveRefString() in subsequent
												   // PlacePhraseBox() calls
			m_bIsRetranslationCurrent = FALSE;
			if(!bSetSafely)
			{
				// IDS_ALL_RETRANSLATIONS
				wxMessageBox(_(
"Warning: your document is full up with retranslations. This makes it impossible to place the phrase box anywhere in the document."),
				_T(""), wxICON_EXCLAMATION | wxOK);
                // BEW changed 19Mar09 for refactored layout, to comment out & so allow the
                // phrase box to be shown at the last pile of the document whether there's
                // a retranslation there or not, if no other safe location is found
				//return;
			}
		}
		else
		{
            // we are in adaptationsStep of vertical editing process, so we want the active
            // pile to be the one immediately following the retranslation; but if that is
            // in the gray text area beyond the end of the editable span, we set a boolean
            // so we can later suppress the reconstruction of the phrase box in the gray
            // area, and just instead immediately cause the dialog asking the user what to
            // do for the next step to be displayed; we also need to deal with the
            // possibility the user's retranslation may make the editable span longer, and
            // update the relevant parameters in gEditRecord
			if (nNewCount > nCount)
			{
				nVerticalEdit_nExtras = nNewCount - nCount;

                // update the relevant parts of the gEditRecord (all spans are affected
                // equally, except the source text edit section is unchanged)
				gEditRecord.nAdaptationStep_ExtrasFromUserEdits += nVerticalEdit_nExtras;
				gEditRecord.nAdaptationStep_NewSpanCount += nVerticalEdit_nExtras;
				gEditRecord.nAdaptationStep_EndingSequNum += nVerticalEdit_nExtras;
			}
			// if the test is equality or less than, then nVerticalEdit_nExtras is 0, and no
			// change to the gEditRecord is required

			// set the potential active location to the CSourcePhrase immediately following
			// the end of the retranslation
			int nPotentialActiveSequNum = nSaveSequNum + nNewCount;

			// determine if this location is within the editable span, if it is, we permit
			// the later restoration of the phrase box there; if not, we suppress the
			// restoration of the phrase box (otherwise it would be in the gray text area)
			if (!(nPotentialActiveSequNum >= gEditRecord.nAdaptationStep_StartingSequNum &&
				  nPotentialActiveSequNum <= gEditRecord.nAdaptationStep_EndingSequNum))
			{
				bVerticalEdit_SuppressPhraseBox = TRUE;
			}

			nSaveActiveSequNum = nPotentialActiveSequNum; // we need a value to work with below
										// even if we suppress reconstituting of the phrase box
			m_bSuppressRemovalOfRefString = TRUE; // suppress RemoveRefString() call within
												  // PlacePhraseBox()
			bool bSetSafely;
			bSetSafely = m_pView->SetActivePilePointerSafely(m_pApp,pSrcPhrases,
							nSaveActiveSequNum, m_pApp->m_nActiveSequNum,nFinish);
			bSetSafely = bSetSafely; // avoid warning TODO: Check for failures? (BEW 2Jan12, No,
									 // we want processing to continue, at worst the
									 // phrase box would open within the retranslation
									 // which is something the app can tolerate okay
			m_bSuppressRemovalOfRefString = FALSE; // permit RemoveRefString() in subsequent
												   // PlacePhraseBox() calls
			m_bIsRetranslationCurrent = FALSE;
		}
	}
	else
	{
		// user cancelled, so we have to restore the original state
		m_bIsRetranslationCurrent = FALSE;
		wxASSERT(pSaveList);
		int nCurCount = nEndSequNum - nSaveSequNum + 1; // what the selection now numbers,
														// after unmerge etc.
		int nOldCount = pSaveList->GetCount();
		wxASSERT(nOldCount >0);
		int nExtras = nCurCount - nOldCount; // needed for adjusting indices
		wxASSERT(nExtras >= 0); // cannot be negative

        // this list's source phrases have not had their KB refString entries removed/count
        // decremented, whichever is required, so we must do so now - otherwise, if they
        // are storing some adaptions, when we try to re-store them in the KB, the
        // StoreAdaptation assert at 1st line will trip.
		ClearSublistKBEntries(pSaveList);

		// insert the original (saved) source phrases after the nEndSequNum one
		InsertSublistAfter(pSrcPhrases,pSaveList,nEndSequNum);

        // now remove the unwanted ones - be careful, some of these single-word ones will
        // point to memory that any merged source phrases in the saved list will point to
        // in their m_pSavedWords sublists, so don't delete the memory in the latter
        // sublists, just remove the pointers!
		RemoveUnwantedSourcePhraseInstancesInRestoredList(pSrcPhrases,nCurCount,
														nSaveSequNum, pSaveList);

		// we can assume nExtras is either 0 or positive
		if (nSaveActiveSequNum > nSaveSequNum + nOldCount - 1)
			nSaveActiveSequNum -= nExtras; // decrement only if it lay after
										   // the original selection

		// renumber the sequence numbers
		m_pView->UpdateSequNumbers(0);

		// remove the pointers in the saved list, and delete the list, but leave the instances
		// undeleted since they are now pointed at by elements in the pSrcPhrases list
		if (pSaveList->GetCount() > 0)
		{
			pSaveList->Clear();
		}
		if (pSaveList != NULL) // whm 11Jun12 added NULL test
			delete pSaveList; // don't leak memory
		pSaveList = (SPList*)NULL;
	}

	// delete the temporary list after removing its pointer copies (copy constructor was not
	// used on this list, so removal of pointers is sufficient)
	pList->Clear();
	if (pList != NULL) // whm 11Jun12 added NULL test
		delete pList;
	pList = (SPList*)NULL;

	// recalculate the layout
	m_pApp->m_nActiveSequNum = nSaveActiveSequNum;
#ifdef _NEW_LAYOUT
	m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
	m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
	m_pApp->m_pActivePile = m_pView->GetPile(nSaveActiveSequNum);

	// get the CSourcePhrase at the active location
	pSrcPhrase = m_pApp->m_pActivePile->GetSrcPhrase();
	wxASSERT(pSrcPhrase != NULL);

    // determine the text to be shown, if any, in the target box when it is recreated When
    // placing the phrasebox after doing a retranslation, or edit of a retranslation, or
    // removal of a retranslation. Introduced in version 1.4.2, because in earlier versions
    // if there was more than one translation available at the location where the phrasebox
    // gets put, the earlier code (in RestoreTargetBoxText( )) would put up the Choose
    // Translation dialog, which is a nuisance when the translation there is already
    // correct; so now we do that call only when we cannot ascertain a valid translation
    // from the source phrase at that point.
	// BEW additions 08Sep08 for support of vertical editing mode
	wxString str3;
	// define the operation type, so PlaceBox() can do its job correctly
	m_pLayout->m_docEditOperationType = retranslate_op;
	if (gbVerticalEditInProgress && bVerticalEdit_SuppressPhraseBox)
	{
		// vertical edit mode is in operation, and a recalc of the layout has been done, so
		// it remains just to determine whether or not to suppress the phrase box and if so
		// to transition to the next step, otherwise send control to the legacy code to have
		// the phrase box created at the active location

		// the active location is in the gray text area, so don't build the phrase box
		// (in wxWidgets, instead hide the phrase box at this point); and instead transition
		// to the next step
		bool bCommandPosted;
		bCommandPosted = m_pView->VerticalEdit_CheckForEndRequiringTransition(-1, nextStep, TRUE);
		// no Invalidate() call made in this block, because a later point in the process
		// should draw the layout anew (I'm guessing, but I think it's a safe guess)
		bCommandPosted = bCommandPosted; // avoid warning (continue processing regardless of outcome)
	}
	else
	{
		if (pSrcPhrase->m_targetStr.IsEmpty() &&
			!pSrcPhrase->m_bHasKBEntry && !pSrcPhrase->m_bNotInKB)
		{
			m_pApp->m_pTargetBox->m_bAbandonable = TRUE;
			RestoreTargetBoxText(pSrcPhrase,str3); // for getting a suitable m_targetStr contents
		}
		else
		{
			str3 = pSrcPhrase->m_targetStr; // if we have something
			m_pApp->m_pTargetBox->m_bAbandonable = FALSE;
		}

		wxString emptyStr = _T("");
		m_pApp->m_pKB->GetAndRemoveRefString(pSrcPhrase,emptyStr,useGlossOrAdaptationForLookup);

		m_pApp->m_targetPhrase = str3; // the Phrase Box can have punctuation as well as text
		m_pApp->m_pTargetBox->ChangeValue(str3);
		m_pApp->m_nStartChar = -1;
		m_pApp->m_nEndChar = -1;

		// layout again, so that the targetBox won't encroach on the next cell's adaption text
#ifdef _NEW_LAYOUT
		m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
		m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
		m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);

		// remove selection and update the display
		m_pView->RemoveSelection();
		m_pView->Invalidate();
		m_pLayout->PlaceBox();
	}

	// ensure respect for boundaries is turned back on
	if (!m_pApp->m_bRespectBoundaries)
	{
		m_pView->OnToggleRespectBoundary(event);
	}
	m_bInsertingWithinFootnote = FALSE; // restore default value
	gnOldSequNum = nSaveOldSequNum; // restore the value we set earlier
}

// BEW 18Feb10, modified for support of doc version 5 (some code added to handle
// transferring endmarker content from the last placeholder back to end of the
// CSourcePhrase list of non-placeholders, prior to showing the dialog)
void CRetranslation::OnButtonEditRetranslation(wxCommandEvent& event)
{
    // Since the Edit Retranslation toolbar button has an accelerator table hot key (CTRL-E
    // see CMainFrame) and wxWidgets accelerator keys call menu and toolbar handlers even
    // when they are disabled, we must check for a disabled button and return if disabled.
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxASSERT(pFrame != NULL);
	wxAuiToolBarItem *tbi;
	tbi = pFrame->m_auiToolbar->FindTool(ID_BUTTON_EDIT_RETRANSLATION);
	// Return if the toolbar item is hidden
	if (tbi == NULL)
	{
		return;
	}
	// Return if this toolbar item is disabled
	if (!pFrame->m_auiToolbar->GetToolEnabled(ID_BUTTON_EDIT_RETRANSLATION))
	{
		::wxBell();
		return;
	}

	if (gbIsGlossing)
	{
		// IDS_NOT_WHEN_GLOSSING
		wxMessageBox(_(
					   "This particular operation is not available when you are glossing."),
					 _T(""), wxICON_INFORMATION | wxOK);
		return;
	}
	SPList* pList = new SPList; // list of the CSourcePhrase objects in the retranslation section
	SPList* pSrcPhrases = m_pApp->m_pSourcePhrases;
	CPile* pStartingPile = NULL;
	CSourcePhrase* pSrcPhrase;
	int nSaveActiveSequNum = m_pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber;

    // get the source phrases which comprise the section which is retranslated; but first
    // check if we have a selection, and if so start from the first pile in the selection;
    // otherwise, we have an error condition.
	CCell* pCell;
	CCellList::Node* cpos;
	if (m_pApp->m_selectionLine != -1)
	{
		// there is a selection current
		cpos = m_pApp->m_selection.GetFirst();
		pCell = (CCell*)cpos->GetData();
		if (pCell != NULL)
		{
			pStartingPile = pCell->GetPile(); // since the selection might be any single
			// srcPhrase, not necessarily the first in the  retranslation,
			// we must reset this later to the true first one
			pSrcPhrase = pStartingPile->GetSrcPhrase();
			if (!pSrcPhrase->m_bRetranslation)
			{
				// an error state
				//IDS_NO_REMOVE_RETRANS
			h:				wxMessageBox(_(
										   "Sorry, the whole of the selection was not within a section of retranslated text, so the command has been ignored."),
										 _T(""), wxICON_EXCLAMATION | wxOK);
				m_pView->RemoveSelection();
				if (pList != NULL) // whm 11Jun12 added NULL test
					delete pList;
				m_pView->Invalidate();
				m_pLayout->PlaceBox();
				return;
			}
		}
	}

	// also check that the end of the selection is also part of the retranslation,
	// if not, return
	cpos = m_pApp->m_selection.GetLast();
	pCell = (CCell*)cpos->GetData();
	CPile* pPile2 = pCell->GetPile();
	if (!pPile2->GetSrcPhrase()->m_bRetranslation)
	{
		// an error state
		goto h;
	}
	else
	{
        // must also check that there is no preceding pile which has a sourcephrase with
        // its m_bEndRetranslation flag set TRUE, if so, the selection lies in a following
        // retranslation section, and so the selection is invalid for delineating one
        // retranslation
		bool bCurrentSection = IsEndInCurrentSelection();
		if (!bCurrentSection)
			goto h;
	}

	// we are in a single retranslation section, so get the source phrases into pList
	m_pView->RemoveSelection();
	CPile* pFirstPile = 0;
	GetRetranslationSourcePhrasesStartingAnywhere(pStartingPile,pFirstPile,pList);

	int nSaveSequNum = pFirstPile->GetSrcPhrase()->m_nSequNumber; // save its sequ number,
	// everything depends on this - its the first in the sublist

    // copy the list to a 2nd list for saving the original state, in case the user hits the
    // Cancel button in the dialog, and save the old sequ num value for the active
    // location; we don't save copies of the pointers, but instead use the copy constructor
    // to make fresh copies of the original selection's source phrases - but note, in the
    // m_pSavedWords sublists, if they have something, the copy constructor only copies the
    // pointers, & doesn't make new copies, so beware that some source phrases might be
    // pointed at from more than one place - which affects how we delete
	SPList* pSaveList = new SPList;
	CopySourcePhraseList(pList,pSaveList);

    // deliberately abandon contents of box at active loc'n - we'll reconstitute it as
    // necessary later, depending on where we want to place the targetBox. But before we
    // abandon it, we must first check if the active location is outside the selection -
    // since there could be a just-edited entry in the phrase box which is not yet entered
    // in the knowledge base, and the active location's source phrase doesn't yet have its
    // m_adaption and m_targetStr members updated, so we must check for this condition and
    // if it obtains then we must first update everything at the active location before we
    // empty m_targetPhrase, etc.
	SPList::Node* pos = 0;
	if (m_pApp->m_pActivePile != NULL)
	{
		CSourcePhrase* pActiveSrcPhrase = m_pApp->m_pActivePile->GetSrcPhrase();
		pos = pList->GetFirst();
		bool bInSelection = FALSE;
		while (pos != NULL)
		{
			CSourcePhrase* pSP = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			if (pSP == pActiveSrcPhrase)
			{
				bInSelection = TRUE;
				break;
			}
		}

		// BEW added 20Mar07: to suppress removing of KB entries during edit
		// of the retranslation
		m_bIsRetranslationCurrent = TRUE;

		if (!bInSelection)
		{
            // the active location is not within the retranslation section, so update
            // before throwing it all out
			m_pView->MakeTargetStringIncludingPunctuation(m_pApp->m_pActivePile->GetSrcPhrase(),m_pApp->m_targetPhrase);
			m_pView->RemovePunctuation(pDoc,&m_pApp->m_targetPhrase,from_target_text);
			if (!m_pApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry)
			{
				gbInhibitMakeTargetStringCall = TRUE;
				bool bOK = m_pApp->m_pKB->StoreText(m_pApp->m_pActivePile->GetSrcPhrase(),
									 m_pApp->m_targetPhrase);
				gbInhibitMakeTargetStringCall = FALSE;
				if (!bOK)
				{
					m_bIsRetranslationCurrent = FALSE;
					return; // can't proceed until a valid adaption (which could be null)
					// is supplied for the former active pile's srcPhrase
				}
				else
				{
					// make the pile at start of former strip have a new pointer - new layout
					// code will then tweak the layout from that point on (see also
					// OnButtonRestore() at lines 14,014 to 14,026)
					int nFormerStrip = m_pApp->m_pActivePile->GetStripIndex();
					pDoc->ResetPartnerPileWidth(m_pApp->m_pActivePile->GetSrcPhrase()); // mark
					// the owning  strip invalid
					int nCurStripIndex = pStartingPile->GetStripIndex();
					if (nCurStripIndex != nFormerStrip)
					{
						CStrip* pFormerStrip = (CStrip*)m_pLayout->GetStripArray()->Item(nFormerStrip);
						CPile* pItsFirstPile = (CPile*)pFormerStrip->GetPilesArray()->Item(0);
						CSourcePhrase* pItsFirstSrcPhrase = pItsFirstPile->GetSrcPhrase();
						// mark this strip invalid too (a little extra insurance)
						pDoc->ResetPartnerPileWidth(pItsFirstSrcPhrase, TRUE); // TRUE
						// is bNoActiveLocationCalculation
					}
				}
			}
		}
	}
	m_pApp->m_targetPhrase.Empty();
	if (m_pApp->m_pTargetBox->GetHandle() != NULL && m_pApp->m_pTargetBox->IsShown())
	{
		m_pApp->m_pTargetBox->ChangeValue(m_pApp->m_targetPhrase); // clear it
	}

    // we have to accumulate now the text comprising the current retranslation, since we
    // won't be able to recover it fully after we throw away any null source phrases which
    // may be present.
	wxString strAdapt; // accumulates the existing adaptation text for the selection
	strAdapt.Empty();
	wxString str; // a temporary storage string
	str.Empty();
	wxString str2; // second temporary storage
	str2.Empty();
	wxString strSource; // the source text which is to be retranslated (now line 1, not line 2)
	strSource.Empty();
	AccumulateText(pList,strSource,strAdapt);


    // if we are invoking this function because of a Find & Replace match within the
    // retranslation, then replace the portion of the strAdapt string which was matched
    // with the replacement string returned by the View's GetReplacementString()
	if (m_bReplaceInRetranslation)
	{
		wxString replStr = m_pView->GetReplacementString();
		ReplaceMatchedSubstring(m_pView->GetSearchString(), replStr, strAdapt);

		// clear the variables for next time
		m_bReplaceInRetranslation = FALSE;
		m_pView->SetSearchString(_T(""));
		m_pView->SetReplacementString(_T(""));
	}

    // determine the value for the active sequ number on exit, so we will know where to
    // place the phrase box on return to the caller; we'll place the phrase box at the
    // first location after the retranslation - provided the active location was within the
    // selection; but if it lay outside the selection, we will need to restore it to
    // wherever it was.
    // int nEndSequNum = ((CSourcePhrase*)pList->GetTail())->m_nSequNumber;
    // break the above down into parts
	SPList::Node* spos = pList->GetLast();
	int nEndSequNum = spos->GetData()->m_nSequNumber;
	bool bActiveLocWithinSelection = FALSE;
	if (nSaveActiveSequNum >= nSaveSequNum && nSaveActiveSequNum <= nEndSequNum)
		bActiveLocWithinSelection = TRUE;
	bool bActiveLocAfterSelection = FALSE;
	if (nSaveActiveSequNum > nEndSequNum)
		bActiveLocAfterSelection = TRUE;

	// we can now clear the m_bEndRetranslation flag on the last entry of the list
	spos = pList->GetLast();
	pSrcPhrase = (CSourcePhrase*)spos->GetData();
	wxASSERT(pSrcPhrase);
	pSrcPhrase->m_bEndRetranslation = FALSE;

    // any null source phrases have to be thrown away, and the layout recalculated after
    // updating the sequence numbers of the source phrases remaining
	pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	int nCount = pList->GetCount();

	// BEW addition 08Sep08 for support of vertical editing
	bool bVerticalEdit_SuppressPhraseBox = FALSE;
	int nVerticalEdit_nExtras = 0;
	int nOriginalCount = nCount;

    // BEW added 01Aug05, to support free translations -- removing null source phrases also
    // removes m_bHasFreeTrans == TRUE instances as well, so the only thing we need check
    // for is whether or not there is m_bEndFreeTrans == TRUE on the last null source
    // phrase removed -- if so, we must set the same bool value to TRUE on the last
    // pSrcPhrase remaining in the list after all the null ones have been deleted. We do
    // this by setting a flag in the block below, and then using the set flag value in the
    // block which follows it

	// BEW 18Feb10, for docVersion = 5, the m_endMarkers member of CSourcePhrase will have
	// had an final endmarkers moved to the last placeholder, so we have to check for a
	// non-empty member on the last placeholder, and if non-empty, save it's contents to a
	// wxString, set a flag to signal this condition obtained, and in the block which
	// follows put the endmarkers back on the last CSourcePhrase which is not a placeholder
	wxString endmarkersStr = _T("");
	bool bEndHasEndMarkers = FALSE;
	bool bEndIsAlsoFreeTransEnd = FALSE;
	while (pos != NULL)
	{
		SPList::Node* savePos = pos;
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		if (pSrcPhrase->m_bNullSourcePhrase)
		{
            // it suffices to test each one, since the m_bEndFreeTrans value will be FALSE
            // on every one, or if not so, then only the last will have a TRUE value
			if (pSrcPhrase->m_bEndFreeTrans)
				bEndIsAlsoFreeTransEnd = TRUE;

			// likewise, test for a non-empty m_endMarkers member at the end - there can
			// only be one such member which has content - the last one
			if (!pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				endmarkersStr = pSrcPhrase->GetEndMarkers();
				bEndHasEndMarkers = TRUE;
			}

            // null source phrases in a retranslation are never stored in the KB, so we
            // need only remove their pointers from the lists and delete them from the heap
			SPList::Node* pos1 = pSrcPhrases->Find(pSrcPhrase);
			wxASSERT(pos1 != NULL); // it has to be there
			pSrcPhrases->DeleteNode(pos1);	// remove its pointer from m_pSourcePhrases list
			// on the doc
			// BEW added 13Mar09 for refactor of layout; delete its partner pile too
			m_pApp->GetDocument()->DeletePartnerPile(pSrcPhrase);

			if (pSrcPhrase != NULL) // whm 11Jun12 added NULL test
				delete pSrcPhrase; // delete the null source phrase itself
			pList->DeleteNode(savePos); // also remove its pointer from the local sublist

			nCount -= 1; // since there is one less source phrase in the selection now
			nEndSequNum -= 1;
			if (bActiveLocAfterSelection)
				nSaveActiveSequNum -= 1;
		}
		else
		{
			// of those source phrases which remain, throw away the contents of their
			// m_adaption and m_targetStr members
			pSrcPhrase->m_adaption.Empty();
			pSrcPhrase->m_targetStr.Empty();
			pSrcPhrase->m_bBeginRetranslation = FALSE;
			pSrcPhrase->m_bEndRetranslation = FALSE;
		}
	}

	// handle transferring the indication of the end of a free translation
	if (bEndIsAlsoFreeTransEnd)
	{
		SPList::Node* tpos = pList->GetLast();
		CSourcePhrase* pSPend = (CSourcePhrase*)tpos->GetData();
		pSPend->m_bEndFreeTrans = TRUE;
	}
	// handle transferring of m_endMarkers content
	if (bEndHasEndMarkers)
	{
		SPList::Node* tpos = pList->GetLast();
		CSourcePhrase* pSPend = (CSourcePhrase*)tpos->GetData();
		pSPend->SetEndMarkers(endmarkersStr);
	}

    // update the sequence number in the whole source phrase list on the app & update
    // indices for bounds
	m_pView->UpdateSequNumbers(0);

    // now we can work out where to place the phrase box on exit from this function - it is
    // currently the nSaveActiveSequNum value, unless the active location was within the
    // selection, in which case we must make the active location the first pile after the
    // selection
	if (bActiveLocWithinSelection)
		nSaveActiveSequNum = nEndSequNum + 1;

    // clear the selection, else RecalcLayout() call will fail at the RestoreSelection()
    // call within it
	m_pView->RemoveSelection();

	// we must have a valid layout, so we have to recalculate it before we go any further,
	// because if preceding code deleted null phrases, the layout's pointers would be clobbered
	// and moving the dialog window would crash the app when Draw messages use the dud pointers
	m_pApp->m_nActiveSequNum = nSaveActiveSequNum; // legally can be a wrong location eg.
	// in the retrans, & nothing will break
#ifdef _NEW_LAYOUT
	m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
	m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
	m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);

	//bool bConstType;
	//bConstType = IsConstantType(pList); // need this only in case m_bInsertingWithinFootnote
	// needs to be set

	// put up the CRetranslationDlg dialog
	CRetranslationDlg dlg(m_pApp->GetMainFrame());

	// initialize the edit boxes
	dlg.m_sourceText = strSource;
	dlg.m_retranslation = strAdapt;
	wxString preceding;
	preceding.Empty();
	wxString following;
	following.Empty();
	wxString precedingTgt;
	precedingTgt.Empty();
	wxString followingTgt;
	followingTgt.Empty();
	m_pView->GetContext(nSaveSequNum,nEndSequNum,preceding,following,precedingTgt,followingTgt);
	dlg.m_preContextSrc = preceding;
	dlg.m_preContextTgt = precedingTgt;
	dlg.m_follContextSrc = following;
	dlg.m_follContextTgt = followingTgt;

	if (dlg.ShowModal() == wxID_OK)
	{
		SPList* pRetransList = new SPList;
		wxASSERT(pRetransList);
		wxString retrans = dlg.m_retranslation;
		int nNewCount = 0; // number of CSourcePhrase instances returned from the
		// tokenization operation

        // tokenize the retranslation into a list of new CSourcePhrase instances on the
        // heap (they are incomplete - only m_key and m_nSequNumber are set)
		nNewCount = m_pView->TokenizeTextString(pRetransList,retrans,nSaveSequNum);

        // ensure any call to InsertNullSrcPhrase() will work right - that function saves
        // the m_pApp->m_nActiveSequNum value, and increments it by how many null source
        // phrases were inserted; so we have to present it with the decremented value
        // agreeing with the present state of the layout (which now lacks the deleted null
        // src phrases - if any)
		if (bActiveLocAfterSelection && nNewCount > nCount)
			nSaveActiveSequNum += nNewCount - nCount;
		else
		{
			// augment it also if the active location lay within the selection
			// and null source phrases were inserted
			if (bActiveLocWithinSelection && nNewCount > nCount)
				nSaveActiveSequNum += nNewCount - nCount;
		}
		m_pApp->m_nActiveSequNum = nSaveActiveSequNum;

        // we must have a valid layout, so we have to recalculate it before we go any
        // further, because if preceding code deleted null phrases, then the layout's
        // pointers will be clobbered
#ifdef _NEW_LAYOUT
		m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
		m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
		m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);

		// get a new valid starting pile pointer
		pStartingPile = m_pView->GetPile(nSaveSequNum);
		wxASSERT(pStartingPile != NULL);

		// determine if we need extra null source phrases inserted, and insert them if we do
		PadWithNullSourcePhrasesAtEnd(pDoc,pSrcPhrases,nEndSequNum,nNewCount,nCount);

		// copy the retranslation's words, one per source phrase, to the constituted sequence of
		// source phrases (including any null ones) which are to display it
		int nFinish = -1; // it gets set to a correct value in the following call
		BuildRetranslationSourcePhraseInstances(pRetransList,nSaveSequNum,nNewCount,
												nCount,nFinish);
        // delete the temporary list and delete the pointers to the CSourcePhrase instances
        // on the heap
		m_pView->DeleteTempList(pRetransList);

        // remove the unused saved original source phrase copies & their list too this
        // pSaveList list will possibly have copies which contain non-empty sublists,
        // especially in m_pSavedWords, and the source phrases pointed to by that list will
        // only have had their pointers copied, so we must not delete those pointers,
        // otherwise the originals will contain hanging pointers & we'll crash if we were
        // to retry the retranslation on the same data
		DeleteSavedSrcPhraseSublist(pSaveList);

        // set the active pile pointer - do it here (not earlier), after any null source
        // phrases have been inserted, otherwise if we are near the end of file, the
        // pointer could be invalid because no such pile exists yet
		// BEW addition 08Sep08 for support of vertical editing
		if (!gbVerticalEditInProgress)
		{
			// legacy behaviour
			m_bSuppressRemovalOfRefString = TRUE;
			bool bSetSafely = m_pView->SetActivePilePointerSafely(m_pApp,pSrcPhrases,nSaveActiveSequNum,
														 m_pApp->m_nActiveSequNum,nFinish);
			m_bSuppressRemovalOfRefString = FALSE; // permit RemoveRefString() in subsequent
												   // PlacePhraseBox() calls
			m_bIsRetranslationCurrent = FALSE;
			if(!bSetSafely)
			{
				// IDS_ALL_RETRANSLATIONS
				wxMessageBox(_(
							   "Warning: your document is full up with retranslations. This makes it impossible to place the phrase box anywhere in the document."),
							 _T(""), wxICON_EXCLAMATION | wxOK);
                // BEW changed 19Mar09 for refactored layout, to comment out & so allow the
                // phrase box to be shown at the last pile of the document whether there's
                // a retranslation there or not
				//return; // we have to return with no phrase box, since we couldn't find
				// anywhere it could be put
			}
		}
		else
		{
            // we are in adaptationsStep of vertical editing process, so we want the active
            // pile to be the one immediately following the retranslation; but if that is
            // in the gray text area beyond the end of the editable span, we set a boolean
            // so we can later suppress the reconstruction of the phrase box in the gray
            // area, and just instead immediately cause the dialog asking the user what to
            // do for the next step to be displayed; we also need to deal with the
            // possibility the user's retranslation may make the editable span longer, and
            // update the relevant parameters in gEditRecord
			nVerticalEdit_nExtras = nNewCount - nOriginalCount; // can be -ve, 0 or +ve

			// update the relevant parts of the gEditRecord
			gEditRecord.nAdaptationStep_ExtrasFromUserEdits += nVerticalEdit_nExtras;
			gEditRecord.nAdaptationStep_NewSpanCount += nVerticalEdit_nExtras;
			gEditRecord.nAdaptationStep_EndingSequNum += nVerticalEdit_nExtras;

            // set the potential active location to the CSourcePhrase immediately following
            // the end of the retranslation
			int nPotentialActiveSequNum = nSaveSequNum + nNewCount;

            // determine if this location is within the editable span, if it is, we permit
            // the later restoration of the phrase box there; if not, we suppress the
            // restoration of the phrase box (otherwise it would be in the gray text area)
			if (!(nPotentialActiveSequNum >= gEditRecord.nAdaptationStep_StartingSequNum &&
				  nPotentialActiveSequNum <= gEditRecord.nAdaptationStep_EndingSequNum))
			{
				bVerticalEdit_SuppressPhraseBox = TRUE;
			}
			nSaveActiveSequNum = nPotentialActiveSequNum; // we need a value to work with below
			// even if we suppress reconstituting of the phrase box
			m_bSuppressRemovalOfRefString = TRUE; // suppress RemoveRefString() call within
			// PlacePhraseBox()
			bool bSetSafely;
			bSetSafely = m_pView->SetActivePilePointerSafely(m_pApp,pSrcPhrases,nSaveActiveSequNum,
													m_pApp->m_nActiveSequNum,nFinish);
			bSetSafely = bSetSafely; // avoid warning TODO: Check for failures? (No, processing
									 // must continue regardless BEW 2Jan12)
			m_bSuppressRemovalOfRefString = FALSE; // permit RemoveRefString() in subsequent
												   // PlacePhraseBox() calls
			m_bIsRetranslationCurrent = FALSE;
		}
	}
	else
	{
		// user cancelled, so we have to restore the original retranslation
		wxASSERT(pSaveList);
		int nCurCount = nEndSequNum - nSaveSequNum + 1; // what the retranslation section
		// now numbers
		int nOldCount = pSaveList->GetCount();
		wxASSERT(nOldCount >0);
        // nOldCount >= nCurCount, because the only thing that could have happened is null
        // source phrases were removed; (and cancellation does not result in parsing what
        // may be in the dialog)
		int nExtras = nOldCount - nCurCount; // needed for adjusting indices
		wxASSERT(nExtras >= 0); // cannot be negative
		m_bIsRetranslationCurrent = FALSE;

        // insert the original (saved) source phrases after the nEndSequNum one (the layout
        // may be different than at start, ie. null ones were removed) - nEndSequNum was
        // reduced however so it has the correct value at this point
		SPList::Node* pos = pSrcPhrases->Item(nEndSequNum);
		wxASSERT(pos != 0);
		SPList::Node* pos1 = pSaveList->GetLast();
		wxASSERT(pos1 != 0);

		// Get a node called newInsertBeforePos which points to the next node beyond pos
		// in pSrcPhrases and use its position in the Insert() call (which only inserts
		// BEFORE the indicated position). The result should be that the insertions
		// will get placed in the list the same way that MFC's InsertAfter() places them.
		SPList::Node* newInsertBeforePos = pos->GetNext();
		while (pos1 != 0)
		{
			// these will be minimal ones, so no restoring in KB is required, as these are
			// effectively not 'encountered' yet
			CSourcePhrase* pSPhr = (CSourcePhrase*)pos1->GetData();
			pos1 = pos1->GetPrevious();
			wxASSERT(pSPhr != NULL);

			// wxList has no equivalent to InsertAfter(). The wxList Insert() method
			// inserts the new node BEFORE the current position/node. To emulate what
			// the MFC code does, we insert before using newInsertBeforePos.
			// wx note: If newInsertBeforePos is NULL, it means the insert position is
			// at the end of the list; in this case we just append the item to the end
			// of the list.
			if (newInsertBeforePos == NULL)
				pSrcPhrases->Append(pSPhr);
			else
				pSrcPhrases->Insert(newInsertBeforePos,pSPhr);

			// BEW added 13Mar09 for refactored layout
			m_pApp->GetDocument()->CreatePartnerPile(pSPhr);

			// since we must now insert before the inserted node above, we need to get a
			// previous node (which will actually be the just inserted source phrase)
			newInsertBeforePos = newInsertBeforePos->GetPrevious();
		}

        // now remove the unwanted ones - be careful, some of these single-word ones will
        // point to memory that any merged source phrases in the saved list will point to
        // in their m_pSavedWords sublists, so don't delete the memory in the latter
        // sublists, just remove the pointers!
		RemoveUnwantedSourcePhraseInstancesInRestoredList(pSrcPhrases, nCurCount,
														  nSaveSequNum, pSaveList);
		// set the active sequ number - it must not be in the retranslation
		int nSequNumImmedAfter = nSaveSequNum + nOldCount;
		if (nSaveActiveSequNum < nSaveSequNum)
		{
			// it earlier than retranslation, so leave it unchanged
			;
		}
		else if (nSaveActiveSequNum < nSequNumImmedAfter)
		{
			// it's still within the retranslation, which is illegal,
			// so put it immed after
			nSaveActiveSequNum = nSequNumImmedAfter;
		}
		else
		{
			// add nExtras to it, to preserve it's former value
			nSaveActiveSequNum += nExtras; // we can assume nExtras is positive
		}

		// renumber the sequence numbers
		m_pView->UpdateSequNumbers(0);

        // remove the pointers in the saved list, and delete the list, but leave the
        // instances undeleted since they are now pointed at by elements in the pSrcPhrases
        // list
		if (pSaveList->GetCount() > 0)
		{
			pSaveList->Clear();
		}
		if (pSaveList != NULL) // whm 11Jun12 added NULL test
			delete pSaveList; // don't leak memory
	}

	// delete the temporary list after removing its pointer copies
	pList->Clear();
	if (pList != NULL) // whm 11Jun12 added NULL test
		delete pList;

    // recalculate the layout from the first strip in the selection,
    // to force the text to change color
	m_pApp->m_nActiveSequNum = nSaveActiveSequNum;
#ifdef _NEW_LAYOUT
	m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
	m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
	m_pApp->m_pActivePile = m_pView->GetPile(nSaveActiveSequNum);

	// get the CSourcePhrase at the active location
	pSrcPhrase = m_pApp->m_pActivePile->GetSrcPhrase();
	wxASSERT(pSrcPhrase != NULL);

	// determine the text to be shown, if any, in the target box when it is recreated
	// BEW additions 08Sep08 for support of vertical editing mode
	wxString str3; // use this one for m_targetStr contents
	// define the operation type, so PlacePhraseBoxInLayout() can do its job correctly
	m_pLayout->m_docEditOperationType = edit_retranslation_op;
	if (gbVerticalEditInProgress && bVerticalEdit_SuppressPhraseBox)
	{
        // vertical edit mode is in operation, and a recalc of the layout has been done, so
        // it remains just to determine whether or not to suppress the phrase box and if so
        // to transition to the next step, otherwise send control to the legacy code to
        // have the phrase box created at the active location

        // the active location is in the gray text area, so don't build the phrase box
        // (in wxWidgets, instead hide the phrase box at this point); and instead
        // transition to the next step
		bool bCommandPosted;
		bCommandPosted = m_pView->VerticalEdit_CheckForEndRequiringTransition(-1, nextStep, TRUE);
		// no Invalidate() call made in this block, because a later point in the process
		// should draw the layout anew (I'm guessing, but I think it's a safe guess)
		bCommandPosted = bCommandPosted; // avoid warning (& keep truckin')
	}
	else
	{
		str3.Empty();

		// we want text with punctuation, for the 4-line version
		if (!pSrcPhrase->m_targetStr.IsEmpty() &&
			(pSrcPhrase->m_bHasKBEntry || pSrcPhrase->m_bNotInKB))
		{
			str3 = pSrcPhrase->m_targetStr;
			m_pApp->m_pTargetBox->m_bAbandonable = FALSE;
		}
		else
		{
            // the Jump( ) call embedded in the PlacePhraseBox( ) which is in turn within
            // SetActivePilePointerSafely( ) will clear the adaptation (or reduce its ref
            // count,) if it exists at the active location; which will cause the above test
            // to land control in this block; so we don't want to do a lookup (it would not
            // find anything if the jump removed the adaptation, and then the source would
            // be copied) because we could then lose the phrasebox contents when in fact
            // they are still good - so if the sourcephrase at the active location has a
            // nonempty target string, we'll use that. Otherwise, get it by a lookup.
			if (pSrcPhrase->m_targetStr.IsEmpty())
			{
				m_pApp->m_pTargetBox->m_bAbandonable = TRUE;
				RestoreTargetBoxText(pSrcPhrase,str3); // for getting a suitable
				// m_targetStr contents
			}
			else
			{
				str3 = pSrcPhrase->m_targetStr;
				m_pApp->m_pTargetBox->m_bAbandonable = FALSE;
			}
		}

		wxString emptyStr = _T("");
		m_pApp->m_pKB->GetAndRemoveRefString(pSrcPhrase,emptyStr,useGlossOrAdaptationForLookup);

		m_pApp->m_targetPhrase = str3;
		if (m_pApp->m_pTargetBox != NULL)
		{
			m_pApp->m_pTargetBox->ChangeValue(str3);
		}

        // layout again, so that the targetBox won't encroach on the next cell's adaption
        // text (can't just layout the strip, because if the text is long then source
        // phrases get pushed off into limbo and we get access violation & null pointer
        // returned in the GetPile call)
#ifdef _NEW_LAYOUT
		m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
		m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
		m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);

		// get a new valid active pile pointer
		m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum);

		m_pApp->m_nStartChar = -1;
		m_pApp->m_nEndChar = -1;

		// remove selection and update the display
		m_pView->RemoveSelection();
		m_pView->Invalidate();
		m_pLayout->PlaceBox();
	}

	// ensure respect for boundaries is turned back on
	if (!m_pApp->m_bRespectBoundaries)
	{
		m_pView->OnToggleRespectBoundary(event);
	}
	m_bInsertingWithinFootnote = FALSE; // restore default value
}

// RemoveRetranslation() does the core work that the OnRemoveRetranslation() handler does,
// but is public and passes in the list which is to be worked on, and doesn't have any of
// the code relating to the GUI. Because RemoveRetranslation() should not be used with
// m_pSourcePhrases document list, but with some other list, we assume there are no
// partner piles and so don't try to remove them. In strAdapt we also return the old
// adaptation for the retranslation, in case the caller may want to use it for some purpose
void CRetranslation::RemoveRetranslation(SPList* pSPList, int first, int last, wxString& strAdapt)
{
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	SPList* pList = new SPList; // list of the CSourcePhrase objects in the retranslation section
	SPList* pSrcPhrases = pSPList; // pSrcPhrases is a local synonym for pSPList
	CSourcePhrase* pSrcPhrase = NULL;

	// get the initial sequence number for pSPList
	int initialSequNum = pSPList->GetFirst()->GetData()->m_nSequNumber;

    // get the source phrases which comprise the section retranslated
	SPList::Node* posStart = pSPList->Item(first);
	wxASSERT(posStart != NULL);
	SPList::Node* posEnd = pSPList->Item(last);
	wxASSERT(posEnd != NULL);
	SPList::Node* pos; // iterator to range over the closed interval
					   // [ posStart , posEnd ]


	// get the range of retranslation CSourcePhrase instances into pList
	wxASSERT(pList->IsEmpty());
	pSrcPhrase = posStart->GetData();
	pList->Append(pSrcPhrase);
	pos = posStart;
	int width = last - first + 1; // for safety first use
	if (posStart == posEnd)
	{
		// there is only one CSourcePhrase in the retranslation, and it is already in
		// pList so do nothing here
		;
	}
	else
	{
		// there is at least a second CSourcePhrase instance in the retranslation
		int count = 1;
		while (pos != posEnd && count < width)
		{
			pos = pos->GetNext();
			pSrcPhrase = pos->GetData();
			count++; // for ensuring bound is not transgressed
			pList->Append(pSrcPhrase); // the one at posEnd will be the last one
									   // added before the loop exits
		}
	}

	// accumulate the translation text of the old retranslation and return it to the
	// caller - though probably we won't ever need to use it, but just in case...
	strAdapt.Empty();
	wxString str2; // a temporary storage string
	str2.Empty();
	SPList::Node* posList = pList->GetFirst();
	wxASSERT(posList != NULL);
	while (posList != NULL)
	{
		// accumulate the old retranslation's text and return it to the caller in case it
		// is needed
		pSrcPhrase = (CSourcePhrase*)posList->GetData();
		posList = posList->GetNext();
		str2 = pSrcPhrase->m_targetStr;
		if (strAdapt.IsEmpty())
		{
			strAdapt += str2;
		}
		else
		{
			if (!str2.IsEmpty())
				strAdapt += _T(" ") + str2;
		}
	}

    // any null source phrases have to be thrown away, and update the sequence numbers of
    // the CSourcePhrase instances remaining in pSPList - use initialSequNum value since we
    // should not assume that the first in pSPList will have value zero
	pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	int nCount = pList->GetCount();
	int nDeletions = 0; // number of null source phrases to be deleted
	wxString endmarkersStr = _T("");
	bool bEndHasEndMarkers = FALSE;
	bool bEndIsAlsoFreeTransEnd = FALSE;
	wxString bindingEMkrs;
	wxString nonbindingEMkrs;
	wxString follPunct;
	wxString follOuterPunct;
	while (pos != NULL)
	{
		SPList::Node* savePos = pos;
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		if (pSrcPhrase->m_bNullSourcePhrase)
		{
            // it suffices to test each one, since the m_bEndFreeTrans value will be FALSE
            // on every one, or if not so, then only the last will have a TRUE value
			if (pSrcPhrase->m_bEndFreeTrans)
				bEndIsAlsoFreeTransEnd = TRUE;

			// likewise, test for a non-empty m_endMarkers member at the end - there can
			// only be one such member which has content - the last one
			if (!pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				endmarkersStr = pSrcPhrase->GetEndMarkers();
				bEndHasEndMarkers = TRUE;
			}

			bindingEMkrs = pSrcPhrase->GetInlineBindingEndMarkers();
			nonbindingEMkrs = pSrcPhrase->GetInlineNonbindingEndMarkers();
			follPunct = pSrcPhrase->m_follPunct;
			follOuterPunct = pSrcPhrase->GetFollowingOuterPunct();

            // null source phrases in a retranslation are never stored in the KB, so we
            // need only remove their pointers from the lists and delete them from the heap
			nDeletions++; // count it
			SPList::Node* pos1 = pSrcPhrases->Find(pSrcPhrase);
			wxASSERT(pos1 != NULL); // it has to be there
			pSrcPhrases->DeleteNode(pos1); // remove its pointer from the passed in pSPList

			if (pSrcPhrase->m_pMedialPuncts != NULL) // whm 11Jun12 added NULL test
				delete pSrcPhrase->m_pMedialPuncts;
			if (pSrcPhrase->m_pMedialMarkers != NULL) // whm 11Jun12 added NULL test
				delete pSrcPhrase->m_pMedialMarkers;
			pSrcPhrase->m_pSavedWords->Clear();
			if (pSrcPhrase->m_pSavedWords != NULL) // whm 11Jun12 added NULL test
				delete pSrcPhrase->m_pSavedWords;
			if (pSrcPhrase != NULL) // whm 11Jun12 added NULL test
				delete pSrcPhrase; // delete the null source phrase itself
			pList->DeleteNode(savePos); // also remove its pointer from the local sublist
		}
		else
		{
            // of those source phrases which remain, throw away the contents of their
            // m_adaption member, clear the flags for start and end, and clear the flag
            // which designates them as retranslations and also the one which says they are
            // not in the KB
			pSrcPhrase->m_adaption.Empty();
			pSrcPhrase->m_targetStr.Empty();
			pSrcPhrase->m_bRetranslation = FALSE;
			if (m_pApp->m_pKB->IsItNotInKB(pSrcPhrase))
				pSrcPhrase->m_bNotInKB = TRUE;
			else
				pSrcPhrase->m_bNotInKB = FALSE;
			pSrcPhrase->m_bHasKBEntry = FALSE;
			pSrcPhrase->m_bBeginRetranslation = FALSE;
			pSrcPhrase->m_bEndRetranslation = FALSE;
		}
	}

	// transfer back info formerly moved to the last placeholder
	if ((int)pList->GetCount() < nCount)
	{
		// handle transferring the indication of the end of a free translation
		if (bEndIsAlsoFreeTransEnd)
		{
			SPList::Node* spos = pList->GetLast();
			CSourcePhrase* pSPend = (CSourcePhrase*)spos->GetData();
			pSPend->m_bEndFreeTrans = TRUE;
		}

		// handle transferring of m_endMarkers content and the other stuff, gathered from
		// the last placeholder, back to the last non-placeholder it was earlier obtained
		// from
		SPList::Node* pos = pList->GetLast();
		CSourcePhrase* pSPend = (CSourcePhrase*)pos->GetData();
		if (bEndHasEndMarkers)
		{
			pSPend->SetEndMarkers(endmarkersStr);
		}
		// 12May11 add the following
		if (!bindingEMkrs.IsEmpty())
		{
			pSPend->SetInlineBindingEndMarkers(bindingEMkrs);
		}
		if (!nonbindingEMkrs.IsEmpty())
		{
			pSPend->SetInlineNonbindingEndMarkers(nonbindingEMkrs);
		}
		if (!follPunct.IsEmpty())
		{
			pSPend->m_follPunct = follPunct;
		}
		if (!follOuterPunct.IsEmpty())
		{
			pSPend->SetFollowingOuterPunct(follOuterPunct);
		}
	}
	// update the sequence numbers to be consecutive within the passed in pSPList content;
	// strictly speaking it isn't necessary if the list was not shortened by deleting some
	// retranslation-final placeholders, but there is no harm in always doing it, and
	// indeed, it is safe to do so
	pDoc->UpdateSequNumbers(initialSequNum, pSPList);

	// remove from the heap the temporary pList
	pList->Clear();
	if (pList != NULL) // whm 11Jun12 added NULL test
		delete pList;
}

// BEW 18Feb10, modified for support of doc version 5 (some code added to handle
// transferring endmarker content from the last placeholder back to end of the
// CSourcePhrase list of non-placeholders)
void CRetranslation::OnRemoveRetranslation(wxCommandEvent& event)
{
	// Invalid function when glossing is ON, so it just returns.
	if (gbIsGlossing)
	{
		// IDS_NOT_WHEN_GLOSSING
		wxMessageBox(_("This particular operation is not available when you are glossing."),
					 _T(""), wxICON_INFORMATION | wxOK);
		return;
	}
	CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
	SPList* pList = new SPList; // list of the CSourcePhrase objects in the retranslation section
	SPList* pSrcPhrases = m_pApp->m_pSourcePhrases;
	CPile* pStartingPile = NULL;
	CSourcePhrase* pSrcPhrase = NULL;
	CCell* pCell = NULL;

    // get the source phrases which comprise the section which is retranslated; first check
    // if we have a selection, and if so start from the first pile in the selection;
    // otherwise, the location to start from must be the target box's location (ie. the
    // active pile); if it's neither of those then we have an error condition
	CCellList::Node* cpos;
	if (m_pApp->m_selectionLine != -1)
	{
		// there is a selection current
		cpos = m_pApp->m_selection.GetFirst();
		pCell = (CCell*)cpos->GetData();
		if (pCell != NULL)
		{
			pStartingPile = pCell->GetPile();
			pSrcPhrase = pStartingPile->GetSrcPhrase();
			if (!pSrcPhrase->m_bRetranslation)
			{
				// an error state
				// IDS_NO_REMOVE_RETRANS
			h:				wxMessageBox(_(
										   "Sorry, the whole of the selection was not within a section of retranslated text, so the command has been ignored."),
										 _T(""), wxICON_EXCLAMATION | wxOK);
				m_pView->RemoveSelection();
				if (pList != NULL) // whm 11Jun12 added NULL test
					delete pList;
				m_pView->Invalidate();
				m_pLayout->PlaceBox();
				return;
			}
		}
	}

	// also check that the end of the selection is also part of the
	// retranslation, if not, return
	cpos = m_pApp->m_selection.GetLast();
	pCell = (CCell*)cpos->GetData();
	CPile* pPile2 = pCell->GetPile();
	if (!pPile2->GetSrcPhrase()->m_bRetranslation)
	{
		// an error state
		goto h;
	}
	else
	{
        // must also check that there is no preceding pile which has a sourcephrase with
        // its m_bEndRetranslation flag set TRUE, if so, the selection lies in a following
        // retranslation section, and so the selection is invalid for delineating one
        // retranslation
		bool bCurrentSection = IsEndInCurrentSelection();
		if (!bCurrentSection)
			goto h;
	}

	// we are in a retranslation section, so get the source phrases into pList
	m_pView->RemoveSelection();
	CPile* pFirstPile = 0;
	GetRetranslationSourcePhrasesStartingAnywhere(pStartingPile,pFirstPile,pList);

	int nStartingSequNum = pFirstPile->GetSrcPhrase()->m_nSequNumber;

    // We must first check if the active location is outside the selection - since there
    // could be a just-edited entry in the phrase box which is not yet entered in the
    // knowledge base, and the active location's source phrase doesn't yet have its
    // m_adaption and m_targetStr members updated, so we must check for this condition and
    // if it obtains then we must first update everything at the active location before we
    // proceed
	if (m_pApp->m_pActivePile != NULL)
	{
		CSourcePhrase* pActiveSrcPhrase = m_pApp->m_pActivePile->GetSrcPhrase();
		pDoc->ResetPartnerPileWidth(pActiveSrcPhrase); // mark its strip as invalid
		SPList::Node* pos = pList->GetFirst();
		bool bInSelection = FALSE;
		while (pos != NULL)
		{
			CSourcePhrase* pSP = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			if (pSP == pActiveSrcPhrase)
			{
				bInSelection = TRUE;
				break;
			}
		}
		if (!bInSelection)
		{
            // the active location is not within the retranslation section, so update
            // before throwing it all out
			m_pView->MakeTargetStringIncludingPunctuation(m_pApp->m_pActivePile->GetSrcPhrase(),m_pApp->m_targetPhrase);
			m_pView->RemovePunctuation(pDoc, &m_pApp->m_targetPhrase, from_target_text);
			if (m_pApp->m_targetPhrase != m_pApp->m_pActivePile->GetSrcPhrase()->m_adaption)
			{
				gbInhibitMakeTargetStringCall = TRUE;
				bool bOK = m_pApp->m_pKB->StoreText(m_pApp->m_pActivePile->GetSrcPhrase(),
												 m_pApp->m_targetPhrase);
				gbInhibitMakeTargetStringCall = FALSE;
				if (!bOK)
					return; // can't proceed until a valid adaption (which could be null)
				// is supplied for the former active pile's srcPhrase
				else
				{
					// make the former strip be marked invalid - new layout
					// code will then tweak the layout from that point on
					int nFormerStrip = m_pApp->m_pActivePile->GetStripIndex();
					int nCurStripIndex = pStartingPile->GetStripIndex();
					if (nCurStripIndex != nFormerStrip)
					{
						CStrip* pFormerStrip = (CStrip*)
						m_pLayout->GetStripArray()->Item(nFormerStrip);
						CPile* pItsFirstPile = (CPile*)
						pFormerStrip->GetPilesArray()->Item(0);
						CSourcePhrase* pItsFirstSrcPhrase =
						pItsFirstPile->GetSrcPhrase();
						// mark this strip as invalid too (some extra insurance)
						pDoc->ResetPartnerPileWidth(pItsFirstSrcPhrase,TRUE); // TRUE
						// is bNoActiveLocationCalculation
					}
				}
			}
		}
	}

    // accumulate the translation text of the old retranslation, so that we can put it in
    // the compose bar's CEdit, in case user wants it preserved
	wxString strAdapt; // accumulates the existing adaptation text for the retranslation
	strAdapt.Empty();
	wxString str2; // a temporary storage string
	str2.Empty();
	SPList::Node* pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	while (pos != NULL)
	{
		// accumulate the old retranslation's text
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		str2 = pSrcPhrase->m_targetStr;
		if (strAdapt.IsEmpty())
		{
			strAdapt += str2;
		}
		else
		{
			if (!str2.IsEmpty())
				strAdapt += _T(" ") + str2;
		}
	}

	// put the text in the compose bar
	CMainFrame* pMainFrm = m_pApp->GetMainFrame();
	wxPanel* pBar = pMainFrm->m_pComposeBar;
	if(pBar != NULL)
	{
		wxTextCtrl* pEdit = (wxTextCtrl*)pBar->FindWindowById(IDC_EDIT_COMPOSE);
		if (pEdit != 0)
		{
			pEdit->ChangeValue(strAdapt);
		}
	}

    // any null source phrases have to be thrown away, and the layout recalculated after
    // updating the sequence numbers of the source phrases remaining
	pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	int nCount = pList->GetCount();
	int nDeletions = 0; // number of null source phrases to be deleted
    // BEW added 01Aug05, to support free translations -- removing null source phrases also
    // removes m_bHasFreeTrans == TRUE instances as well, so the only thing we need check
    // for is whether or not there is m_bEndFreeTrans == TRUE on the last null source
    // phrase removed -- if so, we must set the same bool value to TRUE on the last
    // pSrcPhrase remaining in the list after all the null ones have been deleted. We do
    // this by setting a flag in the block below, and then using the set flag value in the
    // block which follows it

	// BEW 18Feb10, for docVersion = 5, the m_endMarkers member of CSourcePhrase will have
	// had an final endmarkers moved to the last placeholder, so we have to check for a
	// non-empty member on the last placeholder, and if non-empty, save it's contents to a
	// wxString, set a flag to signal this condition obtained, and in the block which
	// follows put the endmarkers back on the last CSourcePhrase which is not a placeholder

	// BEW 12May11, also, for docVersion 5, the final placeholder will have had
	// transferred to it, any non-empty m_inlineBindingEndMarker,
	// m_inlineNonbindingEndMarker, m_follPunct, m_follOuterPunct. Legacy code called
	// RestoreOriginalRetranslation(pSrcPhrase), but that was built when docVersion 4 was
	// in effect and it does things unhelpfully and doesn't handle the above, so I'm going
	// to remove it and instead add code here to do what is needed
	wxString endmarkersStr = _T("");
	bool bEndHasEndMarkers = FALSE;
	bool bEndIsAlsoFreeTransEnd = FALSE;
	// 12May11 add the following
	wxString bindingEMkrs;
	wxString nonbindingEMkrs;
	wxString follPunct;
	wxString follOuterPunct;
	// end 12May11 additions
	while (pos != NULL)
	{
		SPList::Node* savePos = pos;
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		if (pSrcPhrase->m_bNullSourcePhrase)
		{
            // it suffices to test each one, since the m_bEndFreeTrans value will be FALSE
            // on every one, or if not so, then only the last will have a TRUE value
			if (pSrcPhrase->m_bEndFreeTrans)
				bEndIsAlsoFreeTransEnd = TRUE;

			// likewise, test for a non-empty m_endMarkers member at the end - there can
			// only be one such member which has content - the last one
			if (!pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				endmarkersStr = pSrcPhrase->GetEndMarkers();
				bEndHasEndMarkers = TRUE;
			}

			// 12May11 add the following (since transfer was only to the last, this is okay)
			bindingEMkrs = pSrcPhrase->GetInlineBindingEndMarkers();
			nonbindingEMkrs = pSrcPhrase->GetInlineNonbindingEndMarkers();
			follPunct = pSrcPhrase->m_follPunct;
			follOuterPunct = pSrcPhrase->GetFollowingOuterPunct();
			// end 12May11 additions

            // null source phrases in a retranslation are never stored in the KB, so we
            // need only remove their pointers from the lists and delete them from the heap
			nDeletions++; // count it
			SPList::Node* pos1 = pSrcPhrases->Find(pSrcPhrase);
			wxASSERT(pos1 != NULL); // it has to be there
			pSrcPhrases->DeleteNode(pos1); // remove its pointer from m_pSourcePhrases
			// list on the doc

			// BEW added 13Mar09 for refactor of layout; delete its partner pile too
			m_pApp->GetDocument()->DeletePartnerPile(pSrcPhrase);

			if (pSrcPhrase->m_pMedialPuncts != NULL) // whm 11Jun12 added NULL test
				delete pSrcPhrase->m_pMedialPuncts;
			if (pSrcPhrase->m_pMedialMarkers != NULL) // whm 11Jun12 added NULL test
				delete pSrcPhrase->m_pMedialMarkers;
			pSrcPhrase->m_pSavedWords->Clear();
			if (pSrcPhrase->m_pSavedWords != NULL) // whm 11Jun12 added NULL test
				delete pSrcPhrase->m_pSavedWords;
			if (pSrcPhrase != NULL) // whm 11Jun12 added NULL test
				delete pSrcPhrase; // delete the null source phrase itself
			pList->DeleteNode(savePos); // also remove its pointer from the local sublist
		}
		else
		{
            // of those source phrases which remain, throw away the contents of their
            // m_adaption member, and also the m_targetStr member, and clear the flags for
            // start and end, and clear the flags which designate them as retranslations
            // and not in the KB
			pSrcPhrase->m_adaption.Empty();
			pSrcPhrase->m_targetStr.Empty();
			pSrcPhrase->m_bRetranslation = FALSE;
			if (m_pApp->m_pKB->IsItNotInKB(pSrcPhrase))
				pSrcPhrase->m_bNotInKB = TRUE;
			else
				pSrcPhrase->m_bNotInKB = FALSE;
			pSrcPhrase->m_bHasKBEntry = FALSE;
			pSrcPhrase->m_bBeginRetranslation = FALSE;
			pSrcPhrase->m_bEndRetranslation = FALSE;

			// we have to restore the original punctuation too
			// BEW 12May11, removed because it didn't handle all of docV5 stuff, and put
			// extra code instead further below
			//RestoreOriginalPunctuation(pSrcPhrase);

			// these pSrcPhrase instances have to have their partner piles'
			// widths recalculated
			m_pApp->GetDocument()->ResetPartnerPileWidth(pSrcPhrase);
		}
	}

	if ((int)pList->GetCount() < nCount) // TRUE means there was at least one placeholder
	{
		// handle transferring the indication of the end of a free translation
		if (bEndIsAlsoFreeTransEnd)
		{
			SPList::Node* spos = pList->GetLast();
			CSourcePhrase* pSPend = (CSourcePhrase*)spos->GetData();
			pSPend->m_bEndFreeTrans = TRUE;
		}

		// handle transferring of m_endMarkers content and other stuff as per 12May11
		// additions above
		SPList::Node* pos = pList->GetLast();
		CSourcePhrase* pSPend = (CSourcePhrase*)pos->GetData();
		if (bEndHasEndMarkers)
		{
			pSPend->SetEndMarkers(endmarkersStr);
		}
		// 12May11 add the following
		if (!bindingEMkrs.IsEmpty())
		{
			pSPend->SetInlineBindingEndMarkers(bindingEMkrs);
		}
		if (!nonbindingEMkrs.IsEmpty())
		{
			pSPend->SetInlineNonbindingEndMarkers(nonbindingEMkrs);
		}
		if (!follPunct.IsEmpty())
		{
			pSPend->m_follPunct = follPunct;
		}
		if (!follOuterPunct.IsEmpty())
		{
			pSPend->SetFollowingOuterPunct(follOuterPunct);
		}
		// end 12May11 additions

		// update the sequence numbers to be consecutive across the deletion location
		m_pView->UpdateSequNumbers(nStartingSequNum);
	}

	// BEW added 09Sep08 in support of vertical editing mode
	if (gbVerticalEditInProgress && nDeletions != 0)
	{
		gEditRecord.nAdaptationStep_EndingSequNum -= nDeletions;
		gEditRecord.nAdaptationStep_ExtrasFromUserEdits -= nDeletions;
		gEditRecord.nAdaptationStep_NewSpanCount -= nDeletions;
	}

    // we must allow the user the chance to adapt the section of source text which is now
    // no longer a retranslation, so the targetBox must be placed at the first pile of the
    // section - but first we must recalculate the layout
	m_pApp->m_nActiveSequNum = nStartingSequNum;

	// define the operation type, so PlaceBox() // can do its job correctly
	m_pLayout->m_docEditOperationType = remove_retranslation_op;

	// now do the recalculation of the layout & update the active pile pointer
#ifdef _NEW_LAYOUT
	m_pLayout->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
	m_pLayout->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
	m_pApp->m_pActivePile = m_pView->GetPile(m_pApp->m_nActiveSequNum); // need this up-to-date so that
	// RestoreTargetBoxText( ) call will not fail in the code which is below
	// get the text to be displayed in the target box, if any
	SPList::Node* spos = pList->GetFirst();
	pSrcPhrase = (CSourcePhrase*)spos->GetData();
	wxString str3;
	if (pSrcPhrase->m_targetStr.IsEmpty() && !pSrcPhrase->m_bHasKBEntry &&
		!pSrcPhrase->m_bNotInKB)
	{
		m_pApp->m_pTargetBox->m_bAbandonable = TRUE;
		RestoreTargetBoxText(pSrcPhrase,str3); // for getting a suitable
		// m_targetStr contents
	}
	else
	{
		str3 = pSrcPhrase->m_targetStr; // if we have something
		m_pApp->m_pTargetBox->m_bAbandonable = FALSE;
	}
	m_pApp->m_targetPhrase = str3; // update what is to be shown in the phrase box

	// ensure the selection is removed
	m_pView->RemoveSelection();

	// scroll if necessary
	m_pApp->GetMainFrame()->canvas->ScrollIntoView(m_pApp->m_nActiveSequNum);

	pList->Clear();
	if (pList != NULL) // whm 11Jun12 added NULL test
		delete pList;

	m_pView->Invalidate();
	m_pLayout->PlaceBox();

	// ensure respect for boundaries is turned back on
	if (!m_pApp->m_bRespectBoundaries)
	{
		m_pView->OnToggleRespectBoundary(event);
	}
	m_bInsertingWithinFootnote = FALSE; // restore default value
}

void CRetranslation::OnRetransReport(wxCommandEvent& WXUNUSED(event))
{

	// BEW added 05Jan07 to enable work folder on input to be restored when done
	wxString strSaveCurrentDirectoryFullPath = m_pApp->GetDocument()->GetCurrentDirectory();

	m_pApp->LogUserAction(_T("Initiated OnRetranslationReport()"));

	if (gbIsGlossing)
	{
		// IDS_NOT_WHEN_GLOSSING
		wxMessageBox(_(
					   "This particular operation is not available when you are glossing."),
					 _T(""),wxICON_INFORMATION | wxOK);
		m_pApp->LogUserAction(_T("This particular operation is not available when you are glossing."));
		return;
	}
	wxASSERT(m_pApp != NULL);
	CAdapt_ItDoc* pDoc;
	CPhraseBox* pBox;
	CAdapt_ItView* pView;
	m_pApp->GetBasePointers(pDoc,pView,pBox); // this is 'safe' when no doc is open
	wxString docName; // name for the document, to be used in the report

	m_pApp->m_acceptedFilesList.Clear();
	int answer;

    // whm Note: Retranslation reports should be available for a whole project even when
    // a document is currently open. The user should have a choice of whether to do the
    // report for the currently open document only or for the whole project. This OnRetransReport()
    // handler could temporarily close the currently open doc if necessary, and reopen it
    // again once the report is done.

	// only put up the message box if a document is open (and the update handler also
    // disables the command if glossing is on)
    // // BEW 24Aug11, changed to allow the multi-doc choice with document still open
    bool bThisDocOnly = FALSE; // assume multi-doc as default choice, & this is the
							   // only option available if no doc is currently open
	if (m_pApp->IsDocumentOpen())
	{
		// a doc is open, so when that is the case, there is an option for getting a
		// report of just the open one, or the default multi-doc choice; ask user which
		// NOTE,  the BEW changes mean that the doc closure calls DoFileSave_Protected(),
		// which is at a lower level than OnFileSave() which contains the code for sending
		// data to PT or BE; so since a multi-doc choice will have the doc recreated
		// automatically after the report is generated (by a ReOpenDocument() call which
		// knows nothing about collaboration), the doc-specific collaboration information
		// pertinent to transferring data to PT or BE (such as md5 checksums, arrays of
		// these, and the preEdit, postEdit and from-external-editor text files) are not
		// lost or damaged nor made invalid. So we'll freeze the screen and let either a
		// single-doc report, or multi-doc report just happen, whether collaborating or not
		bool bCollaborating = FALSE;
		if (m_pApp->m_bCollaboratingWithParatext || m_pApp->m_bCollaboratingWithBibledit)
		{
			bCollaborating = TRUE;
		}

		//wxString msg = _("Collaborating: the retranslation report will be based on this open document only.");
		//if (m_pApp->m_bCollaboratingWithParatext || m_pApp->m_bCollaboratingWithBibledit)
		//{
		//	wxMessageBox(msg,_T(""),wxICON_INFORMATION | wxOK);
		//	m_pApp->LogUserAction(msg);
		//}
		//else
		//{
		//	msg += _T("\n");
			wxString msg = _(
"To get a report based on many or all documents, click No.\nTo get a report based only on this open document, click Yes.");
			// a "Yes" answer is a choice for reporting only for the current document,
			// a "No" answer will close the current document, scans all docs, builds
			// the report and then reopens the document with the box at its old location
			answer = wxMessageBox(msg,_T(""),wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT);
			if (answer == wxYES)
			{
				bThisDocOnly = TRUE;
				if (bCollaborating)
				{
					msg = _T("User wants Retrans Report based on current doc only. Collaboration is ON");
				}
				else
				{
					msg = _T("User wants Retrans Report based on current doc only. Collaboration is OFF");
				}
				m_pApp->LogUserAction(msg);
			}
			else
			{
				if (bCollaborating)
				{
					msg = _T("User wants Retrans Report based on all docs. Collaboration is ON");
				}
				else
				{
					msg = _T("User wants Retrans Report based on all docs. Collaboration is OFF");
				}
				m_pApp->LogUserAction(msg);
			}
		//}
	}

	bool bOK;

	// whm modified 5Aug11.
	bool bBypassFileDialog_ProtectedNavigation = FALSE;
	wxString defaultDir;
	// Check whether navigation protection is in effect for _REPORTS_OUTPUTS,
	// and whether the App's m_lastRetransReportPath is empty or has a valid path,
	// and set the defaultDir for the export accordingly.
	if (m_pApp->m_bProtectReportsOutputsFolder)
	{
		// Navigation protection is ON, so set the flag to bypass the wxFileDialog
		// and force the use of the special protected folder for the export.
		bBypassFileDialog_ProtectedNavigation = TRUE;
		defaultDir = m_pApp->m_reportsOutputsFolderPath;
	}
	else if (m_pApp->m_lastRetransReportPath.IsEmpty()
		|| (!m_pApp->m_lastRetransReportPath.IsEmpty() && !::wxDirExists(m_pApp->m_lastRetransReportPath)))
	{
		// Navigation protection is OFF so we set the flag to allow the wxFileDialog
		// to appear. But the m_lastKbOutputPath is either empty or, if not empty,
		// it points to an invalid path, so we initialize the defaultDir to point to
		// the special protected folder, even though Navigation protection is not ON.
		// In this case, the user could point the export path elsewhere using the
		// wxFileDialog that will appear.
		bBypassFileDialog_ProtectedNavigation = FALSE;
		defaultDir = m_pApp->m_reportsOutputsFolderPath;
	}
	else
	{
		// Navigation protection is OFF and we have a valid path in m_lastKbOutputPath,
		// so we initialize the defaultDir to point to the m_lastKbOutputPath for the
		// location of the export. The user could still point the export path elsewhere
		// in the wxFileDialog that will appear.
		bBypassFileDialog_ProtectedNavigation = FALSE;
		defaultDir = m_pApp->m_lastRetransReportPath;
	}

	int len;
	wxString reportFilename;

	// whm 29Aug11 modified to make the form of the _retrans_report name
	// conform to the other automatically generated file names under
	// navigation protection mode - in particular when they are used
	// in collaboration (i.e., have a _Collab prefix). The _Collab
	// prefix is changed to _Retrans_Report.
	if (m_pApp->IsDocumentOpen())
	{
		// a document is currently open
		wxASSERT(pDoc != NULL);
		reportFilename = m_pApp->m_curOutputFilename;

		// make a suitable default output filename for the export function
		len = reportFilename.Length();
		reportFilename.Remove(len-4,4); // remove the .adt or .xml extension
		// We are exporting a retranslation report. Get a default file name, set directory.
		// whm Note 8Jul11: When collaboration with PT/BE is ON, and when doing retrans report
		// operations, the reportFilename as obtained from m_curOutputFilename
		// above will be of the form _Collab_45_ACT_CH02.txt. To distinguish these manually
		// produced exports within the _REPORTS_OUTPUTS folder from those generated
		// automatically by our collaboration code, we adjust the reportFilename having a
		// "_Collab..." prefix so that it will have "_Retrans_Report" prefix instead.
		if (reportFilename.Find(_T("_Collab")) == 0)
		{
			// the reportFilename has a _Collab prefix
			reportFilename.Replace(_T("_Collab"),_T("Retrans_Report"));
			reportFilename += _T(".txt");
		}
		else
		{
			// the reportFilename has no _Collab prefix, so append " report.txt" to it
			reportFilename += _(" report.txt"); // make it a *.txt file type // localization?
		}
		docName = reportFilename; // use for the document name in the report
	}
	 else
	{
		// construct a general default filename, and "name" will be defined in
		// DoRetranslationReport()
		reportFilename = _("retranslation report.txt"); // localization?
		docName.Empty();
	}

	// whm modified 7Jul11 to bypass the wxFileDialog when the export is protected from
	// navigation.
	wxString reportPath;
	wxString uniqueFilenameAndPath;
	if (!bBypassFileDialog_ProtectedNavigation)
	{
		// get a file dialog
		wxString filter;
		filter = _("Adapt It Reports (*.txt)|*.txt||");
		wxFileDialog fileDlg(
							 (wxWindow*)m_pApp->GetMainFrame(), // MainFrame is parent window for file dialog
							 _("Filename For Retranslation Report"),
							 defaultDir,
							 reportFilename,
							 filter,
							 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		// | wxHIDE_READONLY); wxHIDE_READONLY deprecated in 2.6 - the checkbox is never shown
		// GDLC wxSAVE & wxOVERWRITE_PROMPT deprecated in 2.8
		fileDlg.Centre();

		if (fileDlg.ShowModal() != wxID_OK)
		{
			int length = m_pApp->m_targetPhrase.Length();
			m_pApp->m_nStartChar = length;
			m_pApp->m_nEndChar = length;
			m_pApp->m_pTargetBox->SetSelection(length,length);
			m_pApp->m_pTargetBox->SetFocus();
			// whm added 05Jan07 to restore the former current working directory for safety
			// sake to what it was on entry, since there was a wxSetWorkingDirectory call made
			// above
			bOK = ::wxSetWorkingDirectory(strSaveCurrentDirectoryFullPath);
			m_pApp->LogUserAction(_T("Cancelled Retrans Report at wxFileDialog"));
			return; // user cancelled
		}
		// get the user's desired path
		reportPath = fileDlg.GetPath();
	}
	else
	{
		reportPath = m_pApp->m_reportsOutputsFolderPath + m_pApp->PathSeparator + reportFilename;
		// Ensure that reportPath is unique so we don't overwrite any existing ones in the
		// appropriate outputs folder.
		uniqueFilenameAndPath = GetUniqueIncrementedFileName(reportPath,incrementViaDate_TimeStamp,TRUE,2,_T("_exported_")); // TRUE - always modify
		// Use the unique path for reportPath
		reportPath = uniqueFilenameAndPath;
	}

	m_pApp->m_bRetransReportInProgress = TRUE;

    // BEW 24Aug11, change protocol so as to allow the user to initiate a multi-doc report
    // even when a document is open. We check here, and if open, we force it to close and
    // then proceed as before.
	wxString savedCurOutputPath = m_pApp->m_curOutputPath;	// includes filename
	wxString savedCurOutputFilename = m_pApp->m_curOutputFilename;
//	int		 savedCurSequNum = m_pApp->m_nActiveSequNum;
	bool	 savedBookmodeFlag = m_pApp->m_bBookMode;
	bool	 savedDisableBookmodeFlag = m_pApp->m_bDisableBookMode;
	int		 savedBookIndex = m_pApp->m_nBookIndex;
	BookNamePair*	pSavedCurBookNamePair = m_pApp->m_pCurrBookNamePair;

	m_pApp->GetMainFrame()->canvas->Freeze();

	// whm 26Aug11 Open a wxProgressDialog instance here for KB Restore operations.
	// The dialog's pProgDlg pointer is passed along through various functions that
	// get called in the process.
	// whm WARNING: The maximum range of the wxProgressDialog (nTotal below) cannot
	// be changed after the dialog is created. So any routine that gets passed the
	// pProgDlg pointer, must make sure that value in its Update() function does not
	// exceed the same maximum value (nTotal).
	//
	// This progress dialog continues for the duration of OnRetransReport(). We need
	// a separate progress dialog inside DoOneDocReport() because each instance of
	// it will be processing a different document with different ranges of values.
	wxString msgDisplayed;
	const int nTotal = m_pApp->GetMaxRangeForProgressDialog(App_SourcePhrases_Count) + 1;
	wxString progMsg = _("%s  - %d of %d Total words and phrases");
	wxFileName fn(m_pApp->m_curOutputFilename);
	msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),1,nTotal);
	CStatusBar* pStatusBar = NULL;
	pStatusBar = (CStatusBar*)m_pApp->GetMainFrame()->m_pStatusBar;
	pStatusBar->StartProgress(_("Generating Retranslation Report..."), msgDisplayed, nTotal);

	bool bDocForcedToClose = FALSE;
	if ((!m_pApp->m_pSourcePhrases->GetCount() == 0) && !bThisDocOnly)
	{
        // doc is open, so close it (for a multi-doc scan), but don't close it if
        // bThisDocOnly is TRUE because the scanning loop in that case expects the doc to
        // be open
		bDocForcedToClose = TRUE;
		bool fsOK = pDoc->DoFileSave_Protected(TRUE, _("Generating Retranslation Report...")); // TRUE - show the wait/progress dialog
		if (!fsOK)
		{
			// something's real wrong!
			wxMessageBox(_(
"Could not save the current document. Retranslation Report command aborted.\nYou can try to continue working, but it would be safer to shut down and relaunch, even if you loose your unsaved edits."),
			_T(""), wxICON_EXCLAMATION | wxOK);
			m_pApp->LogUserAction(_T("Could not close and save the current document. Retranslation Report command aborted."));
			if (bDocForcedToClose)
			{
				bOK = pDoc->ReOpenDocument(	m_pApp, strSaveCurrentDirectoryFullPath,
					savedCurOutputPath, savedCurOutputFilename, /*savedCurSequNum,*/ savedBookmodeFlag,
					savedDisableBookmodeFlag, pSavedCurBookNamePair, savedBookIndex, TRUE); // bMarkAsDirty = TRUE
				bOK = bOK; // avoid warning TODO: Check for failures? (BEW 2Jan12, No, let the
						   // user act on the message above - & so permit him to continue)
			}
			m_pApp->m_bRetransReportInProgress = FALSE;
			m_pApp->GetMainFrame()->canvas->Thaw();
			pStatusBar->FinishProgress(_("Generating Retranslation Report..."));
			return;
		}

        // Ensure the current document's contents are removed, otherwise we will get a
        // doubling of the doc data when OnOpenDocument() is called because the latter will
        // append to whatever is in m_pSourcePhrases
		m_pApp->GetView()->ClobberDocument();
	}

	// update m_lastRetransReportPath
	// whm Note: We set the App's m_lastRetransReportPath variable with the
	// path part of the reportPath just used. We do this even when navigation
	// protection is on, so that the special folders would be the initial path
	// suggested if the administrator were to switch Navigation Protection OFF.
	wxString path, fname, ext;
	wxFileName::SplitPath(reportPath, &path, &fname, &ext);
	m_pApp->m_lastRetransReportPath = path;

	wxLogNull logNo; // avoid spurious messages from the system

	wxFile f;
	if( !f.Open( reportPath, wxFile::write))
	{
#ifdef _DEBUG
		wxLogError(_("Unable to open report file.\n"));
		wxMessageBox(_("Unable to open report file."),_T(""), wxICON_EXCLAMATION | wxOK);
#endif
        // whm added 05Jan07 to restore the former current working directory for safety
        // sake to what it was on entry, since there was a wxSetWorkingDirectory call made
        // above
		bOK = ::wxSetWorkingDirectory(strSaveCurrentDirectoryFullPath);
		m_pApp->LogUserAction(_T("Unable to open report file."));
		if (bDocForcedToClose)
		{
			bOK = pDoc->ReOpenDocument(m_pApp, strSaveCurrentDirectoryFullPath,
				savedCurOutputPath, savedCurOutputFilename, /*savedCurSequNum,*/ savedBookmodeFlag,
				savedDisableBookmodeFlag, pSavedCurBookNamePair, savedBookIndex, TRUE); // bMarkAsDirty = TRUE
		}
		m_pApp->m_bRetransReportInProgress = FALSE;
		m_pApp->GetMainFrame()->canvas->Thaw();
		pStatusBar->FinishProgress(_("Generating Retranslation Report..."));
		return; // just return since it is not a fatal error
	}

	// write a file heading
	wxString header1,header2;
	header1.Empty();
	header2.Empty();
	// header.Format(IDS_RETRANS_HEADER, reportPath);
	// wx note: Since we supply the cross-platform eol chars separately,
	// break header into two parts
	header1 = _("Retranslation Report");
	header2 = header2.Format(_("File Path: %s"), reportPath.c_str());

#ifndef _UNICODE
	f.Write(header1);
	f.Write(m_pApp->m_eolStr);
	f.Write(header2);
	f.Write(m_pApp->m_eolStr);
	f.Write(m_pApp->m_eolStr);
#else
	m_pApp->ConvertAndWrite(wxFONTENCODING_UTF8,&f,header1); // use UTF-8 encoding
	m_pApp->ConvertAndWrite(wxFONTENCODING_UTF8,&f,m_pApp->m_eolStr); // use UTF-8 encoding
	m_pApp->ConvertAndWrite(wxFONTENCODING_UTF8,&f,header2); // use UTF-8 encoding
	m_pApp->ConvertAndWrite(wxFONTENCODING_UTF8,&f,m_pApp->m_eolStr); // use UTF-8 encoding
	m_pApp->ConvertAndWrite(wxFONTENCODING_UTF8,&f,m_pApp->m_eolStr); // use UTF-8 encoding
#endif

	// save entry state (only necessary if entry state had book mode on)
	BookNamePair* pSave_BookNamePair = m_pApp->m_pCurrBookNamePair;
	int nSave_BookIndex = m_pApp->m_nBookIndex;
	wxString save_bibleBooksFolderPath = m_pApp->m_bibleBooksFolderPath;

	// output report data
	wxArrayString* pFileList = &m_pApp->m_acceptedFilesList;
	//if (m_pLayout->GetStripArray()->GetCount() > 0) <<-- it now depends on bThisDocOnly
	if (bThisDocOnly)
	{
		// user wants the report for this current document only
		wxASSERT(pFileList->IsEmpty()); // must be empty,
		// DoRetranslationReport() uses this as a flag
		m_pApp->LogUserAction(_T("Executing DoRetranslationReport() on open doc"));
		DoRetranslationReport(pDoc,docName,pFileList,m_pApp->m_pSourcePhrases,&f,_("Generating Retranslation Report..."));
	}
	else
	{
		// user wants a mult-doc report

 		m_pApp->LogUserAction(_T("Executing DoRetranslationReport() on many docs"));
       // no document is open, so enumerate all the doc files, and do a report based on
        // those the user chooses (remember that in our version of this SDI app, when no
        // document is open, in fact we have an open unnamed empty document, so pDoc is
        // still valid)
		// BEW modified 06Sept05 for support of Bible book folders in the Adaptations
		// folder
		wxASSERT(pDoc != NULL);

		// determine whether or not there are book folders present
        // whm note: AreBookFoldersCreated() has the side effect of changing the current
        // work directory to the passed in m_pApp->m_curAdaptionsPath.
		gbHasBookFolders = m_pApp->AreBookFoldersCreated(m_pApp->m_curAdaptionsPath);

		// do the Adaptations folder's files first
        // whm note: EnumerateDocFiles() has the side effect of changing the current work
        // directory to the passed in m_pApp->m_curAdaptionsPath.
		bool bOK = m_pApp->EnumerateDocFiles(pDoc, m_pApp->m_curAdaptionsPath);
		if (bOK)
		{
			// bale out if there are no files to process, and no book folders too
			if (m_pApp->m_acceptedFilesList.GetCount() == 0 && !gbHasBookFolders)
			{
				// nothing to work on, so abort the operation
				wxMessageBox(_(
"Sorry, there are no saved document files yet for this project. At least one document file is required for the operation you chose to be successful. The command will be ignored."),
							 _T(""),wxICON_EXCLAMATION | wxOK);
                // whm added 05Jan07 to restore the former current working directory for
                // safety sake to what it was on entry, since the EnumerateDocFiles call
                // made above changes the current working dir to the Adaptations folder
                // (MFC version did not add the line below)
				bOK = ::wxSetWorkingDirectory(strSaveCurrentDirectoryFullPath);
				m_pApp->LogUserAction(_T("Sorry, there are no saved document files yet for this project. At least one document file is required for the operation you chose to be successful. The command will be ignored."));
				if (bDocForcedToClose)
				{
					bOK = pDoc->ReOpenDocument(m_pApp, strSaveCurrentDirectoryFullPath,
						savedCurOutputPath, savedCurOutputFilename, /*savedCurSequNum,*/ savedBookmodeFlag,
						savedDisableBookmodeFlag, pSavedCurBookNamePair, savedBookIndex, TRUE); // bMarkAsDirty = TRUE
				}
				m_pApp->m_bRetransReportInProgress = FALSE;
				m_pApp->GetMainFrame()->canvas->Thaw();
				pStatusBar->FinishProgress(_("Generating Retranslation Report..."));
				return;
			}
			// because of prior EnumerateDocFiles call, pFileList will have
			// document filenames in it
			DoRetranslationReport(pDoc,docName,pFileList,m_pApp->m_pSourcePhrases,&f,_("Generating Retranslation Report..."));
		}

		// now do the book folders, if there are any
		if (gbHasBookFolders)
		{
            // process this block only if the project's Adaptations folder contains the set
            // of Bible book folders - these could contain documents, and some or all could
            // be empty; NOTE: the code below is smart enough to ignore any user-created
            // folders which are sisters of the Bible book folders for which the
            // Adaptations folder is the common parent folder
			int nCount;
			wxDir finder;
			// wxDir must call .Open() before enumerating files!
			bool bOK = (finder.Open(m_pApp->m_curAdaptionsPath));
			if (!bOK)
			{
				// highly unlikely, so English will do
				wxString s1, s2, s3;
				s1 = _T(
						"Failed to set the current directory to the Adaptations folder in OnRetransReport function, ");
				s2 = _T(
						"processing book folders, so the book folder document files contribute nothing.");
				s3 = s3.Format(_T("%s%s"),s1.c_str(),s2.c_str());
				wxMessageBox(s3,_T(""), wxICON_EXCLAMATION | wxOK);
                // whm added 05Jan07 to restore the former current working directory for
                // safety sake to what it was on entry, since the wxSetWorkingDirectory
                // call made above changes the current working dir to the Adaptations
                // folder (MFC version did not add the line below)
				bOK = ::wxSetWorkingDirectory(strSaveCurrentDirectoryFullPath);
				m_pApp->LogUserAction(_T("finder.Open() failed in OnRetransReport()"));
				if (bDocForcedToClose)
				{
					bOK = pDoc->ReOpenDocument(m_pApp, strSaveCurrentDirectoryFullPath,
						savedCurOutputPath, savedCurOutputFilename, /*savedCurSequNum,*/ savedBookmodeFlag,
						savedDisableBookmodeFlag, pSavedCurBookNamePair, savedBookIndex, TRUE); // bMarkAsDirty = TRUE
				}
				m_pApp->m_bRetransReportInProgress = FALSE;
				m_pApp->GetMainFrame()->canvas->Thaw();
				pStatusBar->FinishProgress(_("Generating Retranslation Report..."));
				return;
			}
			else
			{
				// whm note: in GetFirst below, wxDIR_FILES | wxDIR_DIRS flag finds files
				// or directories, but not . or .. or hidden files
				wxString str = _T("");
				bool bWorking = finder.GetFirst(&str,wxEmptyString,wxDIR_FILES | wxDIR_DIRS);
				while (bWorking)
				{
					bWorking = finder.GetNext(&str);

                    // whm note: in the MFC version's "if (finder.IsDirectory())" test
                    // below, the finder continues to use the directory path that was
                    // current when the inital finder.FindFile call was made above, even
                    // though the EnumerateDocFiles() call below changes the current
                    // working dir for each of the book folder directories it processes. In
                    // the wx version the finder.Exists(str) call uses whatever the current
                    // working directory is and checks for a sub-directory "str" below that
                    // - a difference we must account for here in the wx version. whm Note:
                    // The Exists() method of wxDIR used below returns TRUE if the passed
                    // name IS a directory.
					if (finder.Exists(m_pApp->m_curAdaptionsPath + m_pApp->PathSeparator + str))
					{
                        // User-defined folders can be in the Adaptations folder without
                        // making the app confused as to whether or not Bible Book folders
                        // are present or not

						// we have found a folder, check if it matches one of those in
						// the array of BookNamePair structs (using the seeName member)
						if (m_pApp->IsDirectoryWithin(str,m_pApp->m_pBibleBooks))
						{
							// we have found a folder name which belongs to the set of
							// Bible book folders, so construct the required path to the
							// folder and enumerate is documents then call
							// DoTransformationsToGlosses() to process any documents within
							wxString folderPath = m_pApp->m_curAdaptionsPath;
							folderPath += m_pApp->PathSeparator + str;

                            // clear the string list of directory names & then enumerate
                            // the directory's file contents; the EnumerateDocFiles() call
                            // sets the current directory to the one given by folderPath
                            // (ie. to a book folder) so after the DoKBRestore() call,
                            // which relies on that directory being current, we must call
                            // ::SetCurrentDirectory(m_curAdaptionsPath) again so that this
                            // outer look which iterates over directories continues
                            // correctly
							pFileList->Clear();
                            // whm note: EnumerateDocFiles() has the side effect of
                            // changing the current work directory to the passed in
                            // folderPath.
							bOK = m_pApp->EnumerateDocFiles(pDoc, folderPath, TRUE); // TRUE
							// == suppress dialog
							if (!bOK)
							{
                                // don't process any directory which gives an error, but
                                // continue looping -- this is a highly unlikely error, so
                                // an English message will do
								m_pApp->GetMainFrame()->canvas->Thaw();
								wxString errStr;
								errStr = errStr.Format(_T(
			"Error returned by EnumerateDocFiles in Book Folder loop, directory %s skipped."),
													   folderPath.c_str());
								wxMessageBox(errStr,_T(""), wxICON_EXCLAMATION | wxOK);
								m_pApp->LogUserAction(errStr);
								m_pApp->GetMainFrame()->canvas->Freeze();
								continue;
							}
							nCount = pFileList->GetCount();
							if (nCount == 0)
							{
								// no documents to work on in this folder, so iterate
								continue;
							}

							// There are files to be processed. TRUE parameter suppresses
							// the statistics dialog.
							DoRetranslationReport(pDoc,docName,pFileList,
												  m_pApp->m_pSourcePhrases,&f, _("Generating Retranslation Report..."));
							// restore parent folder as current
							bOK = ::wxSetWorkingDirectory(m_pApp->m_curAdaptionsPath);
							// the wxASSERT() is a problem when using Freeze() and Thaw()
							// so if it gets thawed here, but control gets past the assert
							// as would be the case in Release build, then re-freeze
							if (!bOK)
							{
								m_pApp->GetMainFrame()->canvas->Thaw();
							}
							wxASSERT(bOK);
							m_pApp->GetMainFrame()->canvas->Freeze();
						}
						else
						{
							continue;
						}
					}
					else
					{
						// its a file, so ignore it
						continue;
					}
				} // end loop for FindFile() scanning all possible files in folder
			}  // end block for bOK == TRUE
		} // end block for test for gbHasBookFolders yielding TRUE

		// clean up the list before returning
		m_pApp->m_acceptedFilesList.Clear();
	}

	// remove the progress dialog
	pStatusBar->FinishProgress(_("Generating Retranslation Report..."));
	// close the file
	f.Close();

	// restore the former book mode parameters (even if book mode was not on on entry)
	m_pApp->m_pCurrBookNamePair = pSave_BookNamePair;
	m_pApp->m_nBookIndex = nSave_BookIndex;
	m_pApp->m_bibleBooksFolderPath = save_bibleBooksFolderPath;
	// now, if the user opens the Document tab of the Start Working wizard, and book
	// mode is on, then at least the path and index and book name are all consistent

    // make sure that book mode is off if there is no valid folder path (if there are docs
    // in book folders, they will store a T (ie. TRUE) for the book mode saved value and so
    // when opened they will turn book mode back on, but if we started with book mode off,
    // then m_bibleBooksFolderPath would be empty, and if we attempt to open the Document
    // tab of the wizard after finishing the report, then we'd get a crash - book mode
    // would be on, but the folder path undefined, -> crash when OnSetActive() of the
    // wizard is called - so the code below ensures this can't happen)
	if (m_pApp->m_bBookMode)
	{
		if (m_pApp->m_bibleBooksFolderPath.IsEmpty())
		{
			// set safe defaults for when mode is off
			m_pApp->m_bBookMode = FALSE;
			m_pApp->m_nBookIndex = -1;
			m_pApp->m_nDefaultBookIndex = 39;
			m_pApp->m_nLastBookIndex = 39;
		}
	}

	int length = m_pApp->m_targetPhrase.Length();
	m_pApp->m_nStartChar = length;
	m_pApp->m_nEndChar = length;
	if (m_pApp->m_pTargetBox != NULL && m_pApp->m_pTargetBox->IsShown())
	{
		m_pApp->m_pTargetBox->SetSelection(length,length);
		m_pApp->m_pTargetBox->SetFocus();
	}
	// BEW added 05Jan07 to restore the former current working directory
	// to what it was on entry
	bOK = ::wxSetWorkingDirectory(strSaveCurrentDirectoryFullPath);

	// whm revised 5Aug11 to always inform the user of the completion of
	// the report operation.
	// Report the completion of the report to the user.
	// Note: For protected navigation situations AI determines the actual
	// filename that is used for the report, and the report itself is
	// automatically saved in the appropriate outputs folder. Especially
	// in these situations where the user has no opportunity to provide a
	// file name nor navigate to a random path, we should inform the user
	// of the successful completion of the report, and indicate the file
	// name that was used and its outputs folder name and location.
	wxFileName fnRpt(reportPath);
	wxString fileNameAndExtOnly = fnRpt.GetFullName();

	wxString msg;
	msg = msg.Format(_("The exported file was named:\n\n%s\n\nIt was saved at the following path:\n\n%s"),fileNameAndExtOnly.c_str(),reportPath.c_str());
	wxMessageBox(msg,_("Export operation successful"),wxICON_INFORMATION | wxOK);
	if (bDocForcedToClose)
	{
		bOK = pDoc->ReOpenDocument(	m_pApp, strSaveCurrentDirectoryFullPath,
			savedCurOutputPath, savedCurOutputFilename, /*savedCurSequNum,*/ savedBookmodeFlag,
			savedDisableBookmodeFlag, pSavedCurBookNamePair, savedBookIndex, TRUE); // bMarkAsDirty = TRUE
	}
	m_pApp->m_bRetransReportInProgress = FALSE;
	m_pApp->GetMainFrame()->canvas->Thaw();
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Tools Menu
///                        is about to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed. This handler disables the "Retranslation Report..."
/// item in the Tools menu if Vertical Editing is in progress, or if the application is in
/// glossing mode, or if the regular KB is not in a ready state. Otherwise it enables the
/// "Retranslation Report..." item on the Tools menu.
/////////////////////////////////////////////////////////////////////////////////
void CRetranslation::OnUpdateRetransReport(wxUpdateUIEvent& event)
{
	// whm added 26Mar12 for read-only mode
	if (m_pApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}

	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	wxASSERT( m_pApp != NULL);

	if (!gbIsGlossing && m_pApp->m_bKBReady && m_pApp->m_pKB != NULL)
	{
		if (gbIsGlossing)
			event.Enable(FALSE); // disable if glossing is ON
		else
			event.Enable(TRUE); // enable, whether doc open or not; glossing OFF
	}
	else
	{
		event.Enable(FALSE); // disable if not got an open project
	}
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever idle processing is enabled. If any
/// of the following conditions are TRUE, this handler disables the "Remove A
/// Retranslation" toolbar item and returns immediately: The application is in glossing
/// mode, the target text only is showing in the main window, the m_pActivePile pointer is
/// NULL, or if the head or tail of the selection do not lie within the retranslation.
/// Otherwise, it enables the toolbar button if there is a selection whose head and tail
/// are part of an existing retranslation.
/////////////////////////////////////////////////////////////////////////////////
void CRetranslation::OnUpdateRemoveRetranslation(wxUpdateUIEvent& event)
{
	// whm added 26Mar12 for read-only mode
	if (m_pApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}

	if (gbIsGlossing || gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	// BEW 24Jan13 added first subtest to avoid spurious false positives
	if (!m_pApp->m_selection.IsEmpty() && m_pApp->m_selectionLine != -1)
	{
		// we require both head and tail of the selection to lie within the retranslation
		CCellList::Node* cpos = m_pApp->m_selection.GetFirst();
		CCell* pCell = (CCell*)cpos->GetData();
		if (pCell != NULL)
		{
			CPile* pPile = pCell->GetPile();
			CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
			if (!pSrcPhrase->m_bRetranslation)
			{
				event.Enable(FALSE);
				return;
			}
			cpos = m_pApp->m_selection.GetLast();
			pCell = (CCell*)cpos->GetData();
			if (pCell != NULL)
			{
				pPile = pCell->GetPile();
				pSrcPhrase = pPile->GetSrcPhrase();
				if (!pSrcPhrase->m_bRetranslation)
				{
					event.Enable(FALSE);
					return;
				}
				else
				{
					event.Enable(TRUE);
					return;
				}
			}
			else
			{
				event.Enable(FALSE);
				return;
			}
		}
		else
		{
			event.Enable(FALSE);
			return;
		}
	}
	else
	{
        // I'll leave the following block here, but it will never be entered because I
        // changed the behaviour to prohibit the phrase box from being placed within a
        // retranslation
        // BEW changed 30Oct12, on shutdown of the app, in Linux, got control entering
        // this block, and a crash due to pSrcPhrase being NULL, so the save thing is
        // to comment it out and let control fall through to disable the edit button
        /*
		if (m_pApp->m_pTargetBox != NULL)
		{
			if (m_pApp->m_pTargetBox->IsShown())
			{
				CSourcePhrase* pSrcPhrase = m_pApp->m_pActivePile->GetSrcPhrase();
				if (pSrcPhrase->m_bRetranslation)
				{
					event.Enable(TRUE);
					return;
				}
			}
		}
		*/
	}
	event.Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever idle processing is enabled. If any
/// of the following conditions are TRUE, this handler disables the "Edit A Retranslation"
/// toolbar item and returns immediately: The application is in glossing mode, the target
/// text only is showing in the main window, the m_pActivePile pointer is NULL, or if the
/// head or tail of the selection do not lie within the retranslation. Otherwise, it
/// enables the toolbar button if there is a selection whose head and tail are part of an
/// existing retranslation.
/////////////////////////////////////////////////////////////////////////////////
void CRetranslation::OnUpdateButtonEditRetranslation(wxUpdateUIEvent& event)
{
	// whm added 26Mar12 for read-only mode
	if (m_pApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}

	if (gbIsGlossing || gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	// BEW 24Jan13 with the commented out test it is possible to crash the app. It can be
	// done by having some text in the phrase box (possibly selected) and doing an
	// ALT+arrow move left or right that brings up something that takes focus from the
	// phrase box (such as a message box), then a subsequent ALT+arrow keypress crashes
	// the app by failing at the CCell* pCell = line just below; somehow, m_selectionLine
	// is 0 even though no selection is set, and so control gets into the TRUE block
	// below. The solution is to have the 2nd subtest last of three, and make the second
	// be a test for a non-empty m_selection, so it will yield FALSE before making the 3rd
	// test
	//if (!gbIsGlossing && m_pApp->m_selectionLine != -1)
	if (!gbIsGlossing && !m_pApp->m_selection.IsEmpty() && m_pApp->m_selectionLine != -1)
	{
		// we require both head and tail of the selection to lie within the retranslation
		CCellList::Node* cpos = m_pApp->m_selection.GetFirst();
		CCell* pCell = (CCell*)cpos->GetData();
		if (pCell != NULL)
		{
			CPile* pPile = pCell->GetPile();
			CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
			if (!pSrcPhrase->m_bRetranslation)
			{
				event.Enable(FALSE);
				return;
			}
			cpos = m_pApp->m_selection.GetLast();
			pCell = (CCell*)cpos->GetData();
			if (pCell != NULL)
			{
				pPile = pCell->GetPile();
				pSrcPhrase = pPile->GetSrcPhrase();
				if (!pSrcPhrase->m_bRetranslation)
				{
					event.Enable(FALSE);
					return;
				}
				else
				{
					event.Enable(TRUE);
					return;
				}
			}
			else
			{
				event.Enable(FALSE);
				return;
			}
		}
		else
		{
			event.Enable(FALSE);
			return;
		}
	}
	else
	{
		// I'll leave the following block here, but it will never be entered because
		// I changed the behaviour to prohibit the phrase box from being placed within
		// a retranslation
		// BEW 30Oct12, comment out, on Linux, when shutting down after a save of the
		// document, this function is called and control enters this else block and
		// pSrcPhrase is NULL leading to a crash. Just let the button be disabled.
		/*
		if (m_pApp->m_pTargetBox != NULL)
		{
			if (m_pApp->m_pTargetBox->IsShown())
			{
				CSourcePhrase* pSrcPhrase = m_pApp->m_pActivePile->GetSrcPhrase();
				if (pSrcPhrase->m_bRetranslation)
				{
					event.Enable(TRUE);
					return;
				}
			}
		}
		*/
	}
	event.Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever idle processing is enabled. If any
/// of the following conditions are TRUE, this handler disables the "Do A Retranslation"
/// toolbar item and returns immediately: The application is in glossing mode, the target
/// text only is showing in the main window, the m_pActivePile pointer is NULL, or if there
/// is a selection in which at least one source phrase already is part of a retranslation.
/// Otherwise, it enables the toolbar button if there is a selection whose source phrases
/// are not part of an existing retranslation.
/////////////////////////////////////////////////////////////////////////////////
void CRetranslation::OnUpdateButtonRetranslation(wxUpdateUIEvent& event)
{
	if (gbIsGlossing || gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (m_pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	// BEW 24Jan13 added first subtest to avoid spurious false positives
	if (!m_pApp->m_selection.IsEmpty() && m_pApp->m_selectionLine != -1)
	{
        // if there is at least one srcPhrase with m_bRetranslation == TRUE, then disable
        // the button
		CCellList::Node* pos = m_pApp->m_selection.GetFirst();
		while (pos != NULL)
		{
			CCell* pCell = (CCell*)pos->GetData();
			CPile* pPile = pCell->GetPile();
			pos = pos->GetNext();
			CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
			if (pSrcPhrase->m_bRetranslation)
			{
				event.Enable(FALSE);
				return;
			}
		}
		event.Enable(TRUE);
		return;
	}
	event.Enable(FALSE);
}

