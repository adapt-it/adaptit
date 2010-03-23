/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Retranslation.cpp
/// \author			Erik Brommers
/// \date_created	10 March 2010
/// \date_revised	10 March 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the implementation file for the CRetranslation class. 
/// The CRetranslation class presents retranslation-related functionality to the user. 
/// The code in the CRetranslation class was originally contained in
/// the CAdapt_ItView class.
/// \derivation		The CNotes class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifdef	_RETRANS

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
#include <wx/config.h> // for wxConfig
#include <wx/tokenzr.h>
#include <wx/textfile.h> // to get EOL info
#include "Adapt_ItCanvas.h"
#include "Adapt_It_Resources.h"
#include <wx/dir.h> // for wxDir
#include <wx/propdlg.h>
#include <wx/progdlg.h> // for wxProgressDialog
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
#include "RetranslationDlg.h"

///////////////////////////////////////////////////////////////////////////////
// External globals
///////////////////////////////////////////////////////////////////////////////
extern bool gbIsGlossing;
extern bool gbInhibitLine4StrCall;
extern bool gbAutoCaps;
extern bool gbUnmergeJustDone;
extern bool gbSourceIsUpperCase;
extern bool gbNonSourceIsUpperCase;
extern bool gbShowTargetOnly;
extern bool gbVerticalEditInProgress;
extern bool gbHasBookFolders;
extern int gnOldSequNum;
extern char gcharNonSrcUC;
extern wxString gSrchStr;
extern wxString gReplStr;
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
	m_pLayout = m_pApp->m_pLayout;
	m_pView = m_pApp->GetView();
	m_bIsRetranslationCurrent = FALSE;
}

CRetranslation::~CRetranslation()
{
	
}

// Utility functions (these will provide correct pointer values only when called from
// within the class functions belonging to the single CRetranslation instantiation within the app
// class)
CLayout* CRetranslation::GetLayout()
{
	return m_pLayout;
}

CAdapt_ItView* CRetranslation::GetView()	// ON APP
{
	return m_pView;
}

CAdapt_ItApp* CRetranslation::GetApp()
{
	return m_pApp;
}


///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

// we allow this search whether glossing is on or not; as it might be a useful search when
// glossing is ON
bool CRetranslation::DoFindRetranslation(int nStartSequNum, int& nSequNum, int& nCount)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	SPList* pList = pApp->m_pSourcePhrases;
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
				 _T(""), wxICON_INFORMATION);
	
	OnButtonRetranslation(dummyevent);
}

void CRetranslation::DoRetranslationByUpArrow()
{
	wxCommandEvent dummyevent;
	OnButtonRetranslation(dummyevent);
}

void CRetranslation::DoOneDocReport(wxString& name, SPList* pList, wxFile* pFile)
{
	CAdapt_ItApp* pApp = &wxGetApp();
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
	
	// initialize the progress indicator window
	int nTotal;
	nTotal = pList->GetCount();
	wxASSERT(nTotal > 0);
	
#ifdef __WXMSW__
	wxString progMsg = _("%s  - %d of %d Total words and phrases");
	wxString msgDisplayed = progMsg.Format(progMsg,name.c_str(),1,nTotal);
	wxProgressDialog progDlg(_("Retranslation Report"),
							 msgDisplayed,
							 nTotal,    // range
							 pApp->GetMainFrame(),   // parent
							 //wxPD_CAN_ABORT |
							 //wxPD_CAN_SKIP |
							 wxPD_APP_MODAL |
							 // wxPD_AUTO_HIDE | -- try this as well
							 wxPD_ELAPSED_TIME |
							 wxPD_ESTIMATED_TIME |
							 wxPD_REMAINING_TIME
							 | wxPD_SMOOTH // - makes indeterminate mode bar on WinXP very small
							 );
#else
	// wxProgressDialog tends to hang on wxGTK so I'll just use the simpler CWaitDlg
	// notification on wxGTK and wxMAC
	// put up a Wait dialog - otherwise nothing visible will happen until 
	// the operation is done
	CWaitDlg waitDlg(pApp->GetMainFrame());
	// indicate we want the reading file wait message
	waitDlg.m_nWaitMsgNum = 5;	// 5 hides the static leaving only 
	// "Please wait..." in title bar
	waitDlg.Centre();
	waitDlg.Show(TRUE);
	waitDlg.Update();
	// the wait dialog is automatically destroyed when it goes out of scope below.
#endif
	
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
					pApp->ConvertAndWrite(wxFONTENCODING_UTF8,pFile,indexText); // use UTF-8
#endif
				}
				else
				{
					// retranslation goes to end of last verse, use previous indexText
#ifndef _UNICODE
					pFile->Write(prevIndexText); 
#else
					pApp->ConvertAndWrite(wxFONTENCODING_UTF8,pFile,prevIndexText); // use UTF-8
#endif
				}
#ifndef _UNICODE
				pFile->Write(oldText);
				
#else // _UNICODE version
				// use UTF-8 encoding
				pApp->ConvertAndWrite(wxFONTENCODING_UTF8,pFile,oldText);
#endif
			}
			oldText.Empty();
			newText.Empty();
			bJustEnded = FALSE;
			bStartRetrans = TRUE; // get ready for start of next one encountered
		}
		
		// whm note: Yield operations tend to hang in wxGTK, so I'm going to just use the
		// wxBusyInfo message and not worry about the message being repainted if covered
		// and uncovered by another window.
#ifdef __WXMSW__
		// update the progress bar
		if (counter % 1000 == 0) 
		{
			msgDisplayed = progMsg.Format(progMsg,name.c_str(),counter,nTotal);
			progDlg.Update(counter,msgDisplayed);
		}
#endif
		
		if (bStartOver)
			goto b;
	}
	
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
		pApp->ConvertAndWrite(wxFONTENCODING_UTF8,pFile,oldText); // use UTF-8
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
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	CCellList::Node* pos = pApp->m_selection.GetLast(); 
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

void CRetranslation::DoRetranslationReport(CAdapt_ItApp* pApp, CAdapt_ItDoc* pDoc, 
										   wxString& name, wxArrayString* pFileList, 
										   SPList* pList, wxFile* pFile)
{
	if (pFileList->IsEmpty())
	{
		
		// use the open document's pList of srcPhrase pointers
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
		pApp->GetMainFrame()->canvas->Freeze();
		
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
			bool bOK;
			bOK = pDoc->OnOpenDocument(newName);
			pDoc->SetFilename(newName,TRUE);
			
			int nTotal = pApp->m_pSourcePhrases->GetCount();
			if (nTotal == 0)
			{
				wxString str;
				str = str.Format(_T("Bad file:  %s"),newName.c_str());
				wxMessageBox(str,_T(""),wxICON_WARNING);
				wxExit(); //AfxAbort();
			}
			
			// get a local pointer to the list of source phrases
			pPhrases = pApp->m_pSourcePhrases;
			
			// use the now open document's pList of srcPhrase pointers, build
			// the part of the report which pertains to this document
			DoOneDocReport(indexingName,pPhrases,pFile); 
			
			// remove the document
			if (!pPhrases->IsEmpty())
			{
				GetView()->ClobberDocument();
				
				// delete the buffer containing the filed-in source text
				if (pApp->m_pBuffer != NULL)
				{
					delete pApp->m_pBuffer;
					pApp->m_pBuffer = NULL;
				}
			}
		}
		
		// allow the view to respond again to updates
		pApp->GetMainFrame()->canvas->Thaw();
	}
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

void CRetranslation::NewRetranslation()
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	wxCommandEvent dummyevent;
	if (gbShowTargetOnly)
	{
		::wxBell();
		return;
	}
	if (pApp->m_pActivePile == NULL)
	{
		::wxBell();
		return;
	}
	if (pApp->m_selectionLine != -1)
	{
        // if there is at least one srcPhrase with m_bRetranslation == TRUE, then disable
        // the button
		CCellList::Node* pos = pApp->m_selection.GetFirst();
		while (pos != NULL)
		{
			CCell* pCell = (CCell*)pos->GetData();
			CPile* pPile = pCell->GetPile();
			pos = pos->GetNext();
			CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
			if (pSrcPhrase->m_bRetranslation)
			{
				::wxBell(); 
				return;
			}
		}
		OnButtonRetranslation(dummyevent);
		return;
	}
	::wxBell();
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
// BEW 16Feb10, no changes needed for support of _DOCVER5
void CRetranslation::GetSelectedSourcePhraseInstances(SPList*& pList,
													 wxString& strSource, wxString& strAdapt)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	wxString str; str.Empty();
	wxString str2; str2.Empty();
	CCellList::Node* pos = pApp->m_selection.GetFirst(); 
	CCell* pCell = (CCell*)pos->GetData();
	CPile* pPile = pCell->GetPile(); // get the pile first in selection
	pos = pos->GetNext(); // needed for our CCellList list
	CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
	
	pList->Append(pSrcPhrase); // add first to the temporary list
	if (pSrcPhrase->m_targetStr.IsEmpty())
	{
		if (pPile == pApp->m_pActivePile)
		{
			str = pApp->m_targetPhrase;
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
			if (pPile == pApp->m_pActivePile)
			{
				str = pApp->m_targetPhrase;
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

// BEW 18Feb10 updated for _DOCVER5 support (added code to restore endmarkers that were
// moved to the placeholder when it was inserted, ie. moved from the preceding
// CSourcePhrase instance)
void CRetranslation::RemoveNullSourcePhrase(CPile* pRemoveLocPile,const int nCount)
{
	// while this function can handle nCount > 1, in actual fact we use it for creating
	// (manually) only a single placeholder, and so nCount is always 1 on entry
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	CPile* pPile			= pRemoveLocPile;
	int nStartingSequNum	= pPile->GetSrcPhrase()->m_nSequNumber;
	SPList* pList			= pApp->m_pSourcePhrases;
	SPList::Node* removePos = pList->Item(nStartingSequNum); // the position at
	// which we will do the removal
	SPList::Node* savePos = removePos; // we will alter removePos & need to restore it
	wxASSERT(removePos != NULL);
	int nActiveSequNum = pApp->m_nActiveSequNum; // save, so we can restore later on, 
	// since the call to RecalcLayout will clobber some pointers
	
    // we may be removing the m_pActivePile, so get parameters useful for setting up a
    // temporary active pile for the RecalcLayout() call below
	int nRemovedPileIndex = pRemoveLocPile->GetSrcPhrase()->m_nSequNumber;
	
    // get the preceding source phrase, if it exists, whether null or not - we may have to
    // transfer punctuation to it
	CSourcePhrase* pPrevSrcPhrase = NULL;
	if (nStartingSequNum > 0)
	{
		// there is a preceding one, so get it
		CPile* pPile = GetView()->GetPrevPile(pRemoveLocPile);
		wxASSERT(pPile != NULL);
		pPrevSrcPhrase = pPile->GetSrcPhrase();
		wxASSERT(pPrevSrcPhrase != NULL);
	}
	
	// ensure that there are nCount null source phrases which can be removed from this location
	int count = 0;
	CSourcePhrase* pFirstOne = NULL; // whm initialized to NULL
	CSourcePhrase* pLastOne = NULL; // whm initialized to NULL
	while( removePos != 0 && count < nCount)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)removePos->GetData();
		removePos = removePos->GetNext();
		wxASSERT(pSrcPhrase != NULL);
		if (count == 0)
			pFirstOne = pSrcPhrase;
		pLastOne = pSrcPhrase;
		count++;
		if (!pSrcPhrase->m_bNullSourcePhrase)
		{
			//IDS_TOO_MANY_NULL_SRCPHRASES
			wxMessageBox(_T(
							"Warning: you are trying to remove more empty source phrases than exist at that location: the command will be ignored."),
						 _T(""),wxICON_EXCLAMATION);
			if (pApp->m_selectionLine != -1)
				GetView()->RemoveSelection();
			GetView()->Invalidate();
			GetLayout()->PlaceBox();
			return;
		}
	}
	
    // a null source phrase can (as of version 1.3.0) be last in the list, so we can no
    // longer assume there will be a non-null one following, if we are at the end we must
    // restore the active location to an earlier sourcephrase, otherwise, to a following
    // one
	bool bNoneFollows = FALSE;
	CSourcePhrase* pSrcPhraseFollowing = 0;
	if (nStartingSequNum + nCount > pApp->GetMaxIndex())
	{
		// we are at the very end, or wanting to remove more at the end than is possible
		bNoneFollows = TRUE; // flag this condition
	}
	
	if (bNoneFollows)
		pSrcPhraseFollowing = 0;
	else
	{
		pSrcPhraseFollowing = (CSourcePhrase*)removePos->GetData();
		removePos = removePos->GetNext();
		wxASSERT(pSrcPhraseFollowing != NULL);
	}
	
	wxASSERT(pFirstOne != NULL); // whm added; note: if pFirstOne can ever be NULL there
	// should be more code added here to deal with it
    // if the null source phrase(s) carry any punctuation or markers, then these have to be
    // first transferred to the appropriate normal source phrase in the context, whether
    // forwards or backwards, depending on what is stored.
#ifdef _DOCVER5
	// docVersion 5 has endmarkers stored on the CSourcePhrase they are pertinent to, in
	// m_endMarkers, so we search for these on pFirst, and if the member is non-empty,
	// transfer its contents to pPrevSrcPhrase (if the markers were automatically
	// transferred to the placement earlier, then pPrevSrcPhrase is guaranteed to exist)
	if (!pFirstOne->GetEndMarkers().IsEmpty() && pPrevSrcPhrase != NULL)
	{
		wxString emptyStr = _T("");
		pPrevSrcPhrase->SetEndMarkers(pFirstOne->GetEndMarkers());
		pFirstOne->SetEndMarkers(emptyStr);
	}
#endif
	if (!pFirstOne->m_markers.IsEmpty() && !bNoneFollows)
	{
        // BEW comment 25Jul05, if a TextType == none endmarker was initial in m_markers,
        // it will have been moved to the placeholder; so the next line handles other
        // situations as well as moving an endmarker back on to the following sourcephrase
        // which formerly owned it
		pSrcPhraseFollowing->m_markers = pFirstOne->m_markers; // don't clear original
		
		// now all the other things which depend on markers
		pSrcPhraseFollowing->m_inform = pFirstOne->m_inform;
		pSrcPhraseFollowing->m_chapterVerse = pFirstOne->m_chapterVerse;
		pSrcPhraseFollowing->m_bVerse = pFirstOne->m_bVerse;
		pSrcPhraseFollowing->m_bParagraph = pFirstOne->m_bParagraph;
		pSrcPhraseFollowing->m_bChapter = pFirstOne->m_bChapter;
		pSrcPhraseFollowing->m_bSpecialText = pFirstOne->m_bSpecialText;
		pSrcPhraseFollowing->m_bFootnote = pFirstOne->m_bFootnote;
		pSrcPhraseFollowing->m_bFirstOfType = pFirstOne->m_bFirstOfType;
		pSrcPhraseFollowing->m_curTextType = pFirstOne->m_curTextType;
		
		// BEW 05Jan06 if there was a moved note we must ensure that the following
		// sourcephrase gets the note flag set (it might already be TRUE anyway)
		pSrcPhraseFollowing->m_bHasNote = pFirstOne->m_bHasNote;
	}
	// block ammended by BEW 25Jul05
	if (!pFirstOne->m_precPunct.IsEmpty() && !bNoneFollows)
	{
		pSrcPhraseFollowing->m_precPunct = pFirstOne->m_precPunct;
		
		// fix the m_targetStr member (we are just fixing punctuation, so no store needed)
		GetView()->MakeLineFourString(pSrcPhraseFollowing,pSrcPhraseFollowing->m_targetStr);
		
		// anything else
		pSrcPhraseFollowing->m_bFirstOfType = pFirstOne->m_bFirstOfType;
	}
	// BEW added 25Jul05
    // a m_bHasFreeTrans = TRUE value can be ignored provided m_bStartFreeTrans value is
    // FALSE, if the latter is TRUE, then we must move the value to the following
    // sourcephrase
	if (pFirstOne->m_bStartFreeTrans && !bNoneFollows)
	{
		pSrcPhraseFollowing->m_bStartFreeTrans = TRUE;
		pSrcPhraseFollowing->m_bHasFreeTrans = TRUE;
	}
	wxASSERT(pLastOne != NULL); // whm added; note: if pLastOne can ever be NULL there
	// should be more code added here to deal with it
	if (!pLastOne->m_follPunct.IsEmpty() && nStartingSequNum > 0)
	{
		pPrevSrcPhrase->m_follPunct = pLastOne->m_follPunct;
		
		// now the other stuff
		pPrevSrcPhrase->m_bFootnoteEnd = pLastOne->m_bFootnoteEnd;
		pPrevSrcPhrase->m_bBoundary = pLastOne->m_bBoundary;
		
		// fix the m_targetStr member (we are just fixing punctuation, so no store needed)
		GetView()->MakeLineFourString(pPrevSrcPhrase,pPrevSrcPhrase->m_targetStr);
	}
	// BEW added 25Jul05...
    // a m_bHasFreeTrans = TRUE value can be ignored provided m_bEndFreeTrans value is
    // FALSE, if the latter is TRUE, then we must move the value to the preceding
    // sourcephrase
	if (pLastOne->m_bEndFreeTrans && nStartingSequNum > 0)
	{
		pPrevSrcPhrase->m_bEndFreeTrans = TRUE;
		pPrevSrcPhrase->m_bHasFreeTrans = TRUE;
	}
	
	// remove the null source phrases from the list, after removing their 
	// translations from the KB
	CRefString* pRefString = NULL;
	removePos = savePos;
	count = 0;
	while (removePos != NULL && count < nCount)
	{
		SPList::Node* pos2 = removePos; // save current position for RemoveAt call
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)removePos->GetData();
		
		// BEW added 13Mar09 for refactored layout
		GetView()->GetDocument()->DeletePartnerPile(pSrcPhrase);
		removePos = removePos->GetNext();
		wxASSERT(pSrcPhrase != NULL);
		pRefString = GetView()->GetRefString(GetView()->GetKB(),pSrcPhrase->m_nSrcWords,
								  pSrcPhrase->m_key,pSrcPhrase->m_adaption);
		count++;
		if (pRefString != NULL)
			// don't need to worry about m_bHasKBEntry flag, since pSrcPhrase
			// will be deleted next
			GetView()->RemoveRefString(pRefString,pSrcPhrase,pSrcPhrase->m_nSrcWords);
		delete pSrcPhrase;
		pList->DeleteNode(pos2); 
	}
	
    // calculate the new active sequ number - it could be anywhere, but all we need to know
    // is whether or not the last removal of the sequence was done preceding the former
    // active sequ number's location
	if (nStartingSequNum + nCount < nActiveSequNum)
		pApp->m_nActiveSequNum = nActiveSequNum - nCount;
	else
	{
		if (bNoneFollows)
			pApp->m_nActiveSequNum = nStartingSequNum - 1;
		else
			pApp->m_nActiveSequNum = nStartingSequNum;
	}
	
    // update the sequence numbers, starting from the location of the first one removed;
    // but if we removed at the end, no update is needed
	if (!bNoneFollows)
		GetView()->UpdateSequNumbers(nStartingSequNum);
	
	// for getting over the hump of the call below to RecalcLayout() we only need a temporary
	// reasonably accurate active pile pointer set, and only if the removal was done at the
	// active location - if it wasn't, then RecalcLayout() will set things up correctly
	// without a failure
	if (nRemovedPileIndex == nActiveSequNum)
	{
		// set a temporary one, we'll use the pile which is now at nRemovedPileIndex
		// location, which typically is the one immediately following the placeholder's
		// old location; but if deleted from the doc's end, we'll use the last valid
		// document location
		int nMaxDocIndex = pApp->GetMaxIndex();
		if (nRemovedPileIndex > nMaxDocIndex)
			pApp->m_pActivePile = GetView()->GetPile(nMaxDocIndex);
		else
			pApp->m_pActivePile = GetView()->GetPile(nRemovedPileIndex);
	}
	
    // in case the active location is going to be a retranslation, check and if so, advance
    // past it; but if at the end, then back up to a valid preceding location
	CSourcePhrase* pSP = GetView()->GetSrcPhrase(pApp->m_nActiveSequNum);
	CPile* pNewPile;
	if (pSP->m_bRetranslation)
	{
		CPile* pPile = GetView()->GetPile(pApp->m_nActiveSequNum);
		do {
			pNewPile = GetView()->GetNextPile(pPile);
			if (pNewPile == NULL)
			{
				// move backwards instead, and find a suitable location
				pPile = GetView()->GetPile(pApp->m_nActiveSequNum);
				do {
					pNewPile = GetView()->GetPrevPile(pPile);
					pPile = pNewPile;
				} while (pNewPile->GetSrcPhrase()->m_bRetranslation);
				goto b;
			}
			pPile = pNewPile;
		} while (pNewPile->GetSrcPhrase()->m_bRetranslation);
	b:		pApp->m_pActivePile = pNewPile;
		pApp->m_nActiveSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
	}
	
    // we need to set m_targetPhrase to what it will be at the new active location, else if
    // the old string was real long, the CalcPileWidth() call will compute enormous and
    // wrong box width at the new location
	pSP = GetView()->GetSrcPhrase(pApp->m_nActiveSequNum);
	if (!pApp->m_bHidePunctuation) // BEW 8Aug09, removed deprecated m_bSuppressLast from test
		pApp->m_targetPhrase = pSP->m_targetStr;
	else
		pApp->m_targetPhrase = pSP->m_adaption;
	
	// recalculate the layout
#ifdef _NEW_LAYOUT
	GetLayout()->RecalcLayout(pList, keep_strips_keep_piles);
#else
	GetLayout()->RecalcLayout(pList, create_strips_keep_piles);
#endif
	
	// get a new (valid) active pile pointer, now that the layout is recalculated
	pApp->m_pActivePile = GetView()->GetPile(pApp->m_nActiveSequNum);
	wxASSERT(pApp->m_pActivePile);
	
	// create the phraseBox at the active pile, do it using PlacePhraseBox()...
	CSourcePhrase* pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
	wxASSERT(pSrcPhrase != NULL);
	
    // renumber its sequ number, as its now in a new location because of the deletion (else
    // the PlacePhraseBox call below will get the wrong number when it reads its
    // m_nSequNumber attribute)
	pSrcPhrase->m_nSequNumber = pApp->m_nActiveSequNum;
	GetView()->UpdateSequNumbers(pApp->m_nActiveSequNum);
	
	// set m_targetPhrase to the appropriate string
	if (!pSrcPhrase->m_adaption.IsEmpty())
	{
		if (!pApp->m_bHidePunctuation) // BEW 8Aug09, removed deprecated m_bSuppressLast from test
			pApp->m_targetPhrase = pSrcPhrase->m_targetStr;
		else
			pApp->m_targetPhrase = pSrcPhrase->m_adaption;
	}
	else
	{
		pApp->m_targetPhrase.Empty(); // empty string will have to do
	}
	
    // we must remove the source phrase's translation from the KB as if we
    // had clicked here (otherwise PlacePhraseBox will assert)
	pRefString = GetView()->GetRefString(GetView()->GetKB(),pSrcPhrase->m_nSrcWords,
							  pSrcPhrase->m_key,pSrcPhrase->m_adaption);
	
    // it is okay to do the following call with pRefString == NULL, in fact, it must be
    // done whether NULL or not; since if it is NULL, RemoveRefString will clear
    // pSrcPhrase's m_bHasKBEntry to FALSE, which if not done, would result in a crash if
    // the user clicked on a source phrase which had its reference string manually removed
    // from the KB and then clicked on another source phrase. (The StoreAdaption call in
    // the second click would trip the first line's ASSERT.)
	GetView()->RemoveRefString(pRefString,pSrcPhrase,pSrcPhrase->m_nSrcWords);
	
    // save old sequ number in case required for toolbar's Back button - but since it
    // probably has been lost (being the null source phrase location), to be safe we must
    // set it to the current active location
	gnOldSequNum = pApp->m_nActiveSequNum;
	
	// scroll into view, just in case a lot were inserted
	pApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);
	GetView()->Invalidate();
	// now place the box
	GetLayout()->PlaceBox();
}

// same parameters as for RemoveNullSourcePhraseFromLists(), except the second last boolean
// is added in order to control whether m_bRetranslation gets set or not; for
// retranslations we want it set, for editing the source text we want it cleared; and the
// last boolean controls whether or not we also update the sublist passed as the first
// parameter - for a retranslation we don't update it, because the caller will make no more
// use of it; but for an edit of the source text, the caller needs it updated because it
// will be used later when the transfer of standard format markers, if any, is done.
// 
// BEW updated 17Feb10 for support of _DOCVER5 (no changes were needed)
void CRetranslation::UnmergeMergersInSublist(SPList*& pList, SPList*& pSrcPhrases, 
											int& nCount, int& nEndSequNum, bool bActiveLocAfterSelection, 
											int& nSaveActiveSequNum, bool bWantRetranslationFlagSet, 
											bool bAlsoUpdateSublist)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	int nNumElements = 1;
	CRefString* pRefString = (CRefString*)NULL;
	SPList::Node* pos = pList->GetFirst();
	int nTotalExtras = 0; // accumulate the total number of extras added by unmerging,
	// this will be used if the updating of the sublist is asked for
	int nInitialSequNum = pos->GetData()->m_nSequNumber; // preserve this
	// for sublist updating
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
			nNumElements = GetView()->RestoreOriginalMinPhrases(pSrcPhrase,nStartingSequNum);
			
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
			pApp->m_targetPhrase.Empty();
			
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
			pRefString = GetView()->GetRefString(GetView()->GetKB(),pSrcPhrase->m_nSrcWords,
									  pSrcPhrase->m_key,pSrcPhrase->m_adaption);
            // it is okay to do the following call with pRefString == NULL, in fact, it
            // must be done whether NULL or not; since if it is NULL, RemoveRefString will
            // clear pSrcPhrase's m_bHasKBEntry to FALSE, which if not done, would result
            // in a crash if the user clicked on a source phrase which had its reference
            // string manually removed from the KB and then clicked on another source
            // phrase. (The StoreAdaption call in the second click would trip the first
            // line's wxASSERT.)
			GetView()->RemoveRefString(pRefString,pSrcPhrase,pSrcPhrase->m_nSrcWords);
			
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

// BEW 17Feb10 updated to support _DOCVER5 (no changes were needed)
void CRetranslation::BuildRetranslationSourcePhraseInstances(SPList* pRetransList,
															int nStartSequNum,int nNewCount,int nCount,int& nFinish)
{
	// BEW refactored 16Apr09
	CAdapt_ItDoc* pDoc = GetApp()->GetDocument();
	int nSequNum = nStartSequNum - 1;
	nFinish = nNewCount < nCount ? nCount : nNewCount;
	for (int j=0; j<nFinish; j++)
	{
		nSequNum++;
		CPile* pPile = GetView()->GetPile(nSequNum); // needed, because the InsertNullSourcePhrase()
		// clobbered ptrs and so did a RecalcLayout()
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
			GetView()->RemovePunctuation(pDoc,&pIncompleteSrcPhrase->m_key,from_target_text);
			pSrcPhrase->m_adaption = pIncompleteSrcPhrase->m_key;
			//check that all is well
			wxASSERT(pSrcPhrase->m_nSequNumber == pIncompleteSrcPhrase->m_nSequNumber);
			
			// BEW added 13Mar09 for refactored layout
			pDoc->ResetPartnerPileWidth(pSrcPhrase); // resets width and marks the
			// owning strip invalid
		}
		
        // if nNewCount was less than nCount, we must clear any old punctuation off the
        // unused source phrases at the end of the selection (we will leave markers
        // untouched) so that the typed punctuation effectively overrides that on the
        // source
		if (j >= nNewCount)
		{
			pDoc->ResetPartnerPileWidth(pSrcPhrase);
		}
	}
}

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
				delete pSP->m_pMedialMarkers;
				pSP->m_pMedialMarkers = (wxArrayString*)NULL;
				delete pSP->m_pMedialPuncts;
				pSP->m_pMedialPuncts = (wxArrayString*)NULL;
				pSP->m_pSavedWords->Clear(); // remove pointers only
				delete pSP->m_pSavedWords;
				pSP->m_pSavedWords = (SPList*)NULL;
				delete pSP;
				pSP = (CSourcePhrase*)NULL;
			}
		}
		pSaveList->Clear();
	}
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
// BEW updated 17Feb10 for support of _DOCVER5 (no changes were needed)
void CRetranslation::PadWithNullSourcePhrasesAtEnd(CAdapt_ItDoc* pDoc,CAdapt_ItApp* pApp,
												  SPList* pSrcPhrases,int nEndSequNum,int nNewCount,int nCount)
{
	// refactored 16Apr09
	int nEndIndex = 0;
	int nSaveActiveSN = pApp->m_nActiveSequNum;
	if (nNewCount > nCount)
	{
		// null source phrases are needed for padding
		int nExtras = nNewCount - nCount;
		
        // check we are not at the end of the list of CSourcePhrase instances, if we are we
        // will have to add an extra one so that we can insert before it, then remove it
        // later.
		if (nEndSequNum == pApp->GetMaxIndex())
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
			nEndIndex = pApp->GetMaxIndex() + 1;
			pDummySrcPhrase->m_nSequNumber = nEndIndex;
			SPList::Node* posTail;
			posTail = pSrcPhrases->Append(pDummySrcPhrase); 
			
			// we need a valid layout which includes the new dummy element on its own pile
			pApp->m_nActiveSequNum = nEndIndex; // temporary location only
#ifdef _NEW_LAYOUT
			GetLayout()->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
			GetLayout()->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
			pApp->m_pActivePile = GetView()->GetPile(pApp->m_nActiveSequNum); // temporary active location
			
			// now we can do the insertions, preceding the dummy end pile
			CPile* pPile = GetView()->GetPile(nEndIndex);
			wxASSERT(pPile != NULL);
			GetView()->InsertNullSourcePhrase(pDoc,pApp,pPile,nExtras,FALSE,TRUE); // FALSE for restoring
			// the phrase box, TRUE for doing it for a retranslation, and default TRUE for
			// bInsertBefore flag at end
			
			// now remove the dummy element, and make sure memory is not leaked!
			delete pDummySrcPhrase->m_pSavedWords;
			pDummySrcPhrase->m_pSavedWords = (SPList*)NULL;
			delete pDummySrcPhrase->m_pMedialMarkers;
			pDummySrcPhrase->m_pMedialMarkers = (wxArrayString*)NULL;
			delete pDummySrcPhrase->m_pMedialPuncts;
			pDummySrcPhrase->m_pMedialPuncts = (wxArrayString*)NULL;
			SPList::Node *pLast = pSrcPhrases->GetLast();
			pSrcPhrases->DeleteNode(pLast);
			delete pDummySrcPhrase;
			
			// get another valid layout
			pApp->m_nActiveSequNum = nSaveActiveSN; // restore original location
		}
		else
		{
            // not at the end, so we can proceed immediately; get the insertion location's
            // pile pointer
			CPile* pPile = GetView()->GetPile(nEndSequNum + 1); // nEndIndex is out of scope here
			wxASSERT(pPile != NULL);
			GetView()->InsertNullSourcePhrase(pDoc,pApp,pPile,nExtras,FALSE,TRUE); // FALSE is for 
			// restoring the phrase box, TRUE is for doing it for a retranslation
			pApp->m_nActiveSequNum = nSaveActiveSN;
		}
	}
	else
		; // no padding needed
}

// BEW 17Feb10, updated for support of _DOCVER5 (no changes needed)
void CRetranslation::ClearSublistKBEntries(SPList* pSublist)
{
	SPList::Node* pos = pSublist->GetFirst();
	while (pos != NULL)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		CRefString* pRefString = GetView()->GetRefString(GetView()->GetKB(),pSrcPhrase->m_nSrcWords,
											  pSrcPhrase->m_key,pSrcPhrase->m_adaption);
        // it is okay to do the following call with pRefString == NULL, the function will
        // just exit early, having done nothing
		GetView()->RemoveRefString(pRefString,pSrcPhrase,pSrcPhrase->m_nSrcWords);
		pSrcPhrase->m_bRetranslation = FALSE; // make sure its off
		pSrcPhrase->m_bHasKBEntry = FALSE;	  // ditto
	}
}

// BEW 17Feb10, updated for support of _DOCVER5 (no changes needed)
void CRetranslation::InsertSublistAfter(SPList* pSrcPhrases, SPList* pSublist, int nLocationSequNum)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
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
		GetApp()->GetDocument()->CreatePartnerPile(pSPhr);
		
		// since we must now insert before the inserted node above, we need to get a
		// previous node (which will actually be the just inserted source phrase)
		newInsertBeforePos = newInsertBeforePos->GetPrevious();
		
		// If the m_bNotInKB flag is FALSE, we must re-store the translation in
		// the KB. We can get the former translation string from the m_adaption member.
		if (!pSPhr->m_bNotInKB && !pSPhr->m_adaption.IsEmpty())
		{
			bool bOK = GetView()->StoreText(pApp->m_pKB,pSPhr,pSPhr->m_adaption);
			if (!bOK)
			{
				// never had a problem here, so this message can stay in English
				wxMessageBox(_T(
								"Warning: redoing the StoreText operation failed in OnButtonRetranslation\n"),
							 _T(""), wxICON_EXCLAMATION);
			}
		}
	}
}

// BEW 16Feb10, no changes needed for support of _DOCVER5
bool CRetranslation::IsConstantType(SPList* pList)
{
	SPList::Node* pos = pList->GetFirst(); 
	if (pos == NULL)
	{
		wxMessageBox(_T(
						"Error accessing sublist in IsConstantType function\n"),
					 _T(""), wxICON_EXCLAMATION);
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
// BEW 17Feb10, updated for support of _DOCVER5 (no changes needed)
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
		GetApp()->GetDocument()->DeletePartnerPile(pSrcPhrase);
		
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
			delete pSrcPhrase->m_pMedialMarkers;
			pSrcPhrase->m_pMedialMarkers = (wxArrayString*)NULL;
			
			if (pSrcPhrase->m_pMedialPuncts->GetCount() > 0)
			{
				pSrcPhrase->m_pMedialPuncts->Clear(); // can clear the strings safely
			}
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
			delete pSrcPhrase->m_pSavedWords;		// and delete the list from the heap
			pSrcPhrase->m_pSavedWords = (SPList*)NULL;
			
			// finally delete the source phrase copy itself
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
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	bool bGotTranslation;
	bool bNoError = TRUE;
	if (gbAutoCaps)
	{
		bNoError = GetView()->SetCaseParameters(pSrcPhrase->m_key);
	}
	
    // although this function strictly speaking is not necessarily invoked in the context
    // of an unmerge, the gbUnmergeJustDone flag being TRUE gives us the behaviour we want;
    // ie. we certainly DON'T want OnButtonRestore() called from here!
	gbUnmergeJustDone = TRUE; // prevent second OnButtonRestore() call from within
	// ChooseTranslation() within LookUpSrcWord() if user happens to
	// cancel the Choose Translation dialog (see CPhraseBox code)
	bGotTranslation = pApp->m_pTargetBox->LookUpSrcWord(GetView(),pApp->m_pActivePile);
	gbUnmergeJustDone = FALSE; // clear flag to default value, since it is a global boolean
	wxASSERT(pApp->m_pActivePile); // it was created in the caller just prior to this
	// function being called 
	if (bGotTranslation)
	{
        // we have to check here, in case the translation it found was a "<Not In KB>" - in
        // which case, we must display m_targetStr and ensure that the pile has an asterisk
        // above it, etc
		if (translation == _T("<Not In KB>"))
		{
			str.Empty(); // phrase box must be shown empty
			pApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry = FALSE;
			pApp->m_pActivePile->GetSrcPhrase()->m_bNotInKB = TRUE;
			str = pApp->m_pActivePile->GetSrcPhrase()->m_targetStr;
		}
		else
		{
			str = translation; // set using the global var, set in LookUpSrcWord call
		}
		
		if (gbAutoCaps && gbSourceIsUpperCase)
		{
			bNoError = GetView()->SetCaseParameters(str,FALSE);
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
		if (pApp->m_bCopySource)
		{
			// copy source key
			str = GetView()->CopySourceKey(pApp->m_pActivePile->GetSrcPhrase(),pApp->m_bUseConsistentChanges);
			pApp->m_pTargetBox->m_bAbandonable = TRUE;
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
		while ((pPile = GetView()->GetPrevPile(pPile)) != NULL && pPile->GetSrcPhrase()->m_bRetranslation)
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
	
	while ((pPile = GetView()->GetNextPile(pPile)) != NULL && pPile->GetSrcPhrase()->m_bRetranslation)
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

// pList is the sublist of (formerly) selected source phrase instances, pSrcPhrases is the
// document's list (the whole lot), nCount is the count of elements in pList (it will be
// reduced as each null source phrase is eliminated), bActiveLocAfterSelection is a flag in
// the caller, nSaveActiveSequNum is the caller's saved value for the active sequence
// number
// BEW updated 17Feb10 for support of _DOCVER5 (no changes were needed)
void CRetranslation::RemoveNullSrcPhraseFromLists(SPList*& pList,SPList*& pSrcPhrases,
												 int& nCount,int& nEndSequNum,bool bActiveLocAfterSelection,
												 int& nSaveActiveSequNum)
{
	// refactored 16Apr09
	// find the null source phrase in the sublist
	CRefString* pRefString = 0;
	SPList::Node* pos = pList->GetFirst();
	while (pos != NULL)
	{
		SPList::Node* savePos = pos;
		CSourcePhrase* pSrcPhraseCopy = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext(); 
		wxASSERT(pSrcPhraseCopy != NULL);
		if (pSrcPhraseCopy->m_bNullSourcePhrase)
		{
            // we've found a null source phrase in the sublist, so get rid of its KB
            // presence, then delete it from the (temporary) sublist, and its instance from
            // the heap
			pRefString = GetView()->GetRefString(GetView()->GetKB(),pSrcPhraseCopy->m_nSrcWords,
									  pSrcPhraseCopy->m_key,pSrcPhraseCopy->m_adaption);
			if (pRefString != NULL)
			{
				// don't care about m_bHasKBEntry flag value, since pSrcPhraseCopy will be
				// deleted next
				GetView()->RemoveRefString(pRefString,pSrcPhraseCopy,pSrcPhraseCopy->m_nSrcWords);
			}
			delete pSrcPhraseCopy;
			pSrcPhraseCopy = (CSourcePhrase*)NULL;
			pList->DeleteNode(savePos); 
			
            // the main list on the app still stores the (now hanging) pointer, so find
            // where it is and remove it from that list too
			SPList::Node* mainPos = pSrcPhrases->GetFirst();
			wxASSERT(mainPos != 0);
			mainPos = pSrcPhrases->Find(pSrcPhraseCopy); // search from the beginning
			wxASSERT(mainPos != NULL); // it must be there somewhere
			
			// BEW added 13Mar09 for refactor of layout; delete its partner pile too 
			GetView()->GetDocument()->DeletePartnerPile(pSrcPhraseCopy);
			pSrcPhrases->DeleteNode(mainPos); 
			
			nCount -= 1; // since there is one less source phrase in the selection now
			nEndSequNum -= 1;
			if (bActiveLocAfterSelection)
				nSaveActiveSequNum -= 1;
			
            // now we have to renumber the source phrases' sequence number values - since
            // the temp sublist list has pointer copies, we need only do this in the main
            // list using a call to UpdateSequNumbers, and so the value of nSaveSequNum
            // will still be correct for the first element in the sublist - even if there
            // were deletions at the start of the sublist
			GetView()->UpdateSequNumbers(0); // start from the very first in the list to be safe
		}
	}
}

// old code was based on code in TokenizeText in doc file; new code is based on code in
// RemovePunctuation() which is much smarter & handles word-building punctuation properly
void CRetranslation::RestoreOriginalPunctuation(CSourcePhrase *pSrcPhrase)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxString src = pSrcPhrase->m_srcPhrase;
	
	// first, clear any punctuation resulting from the retranslation
	pSrcPhrase->m_precPunct.Empty();
	pSrcPhrase->m_follPunct.Empty();
	pSrcPhrase->m_pMedialPuncts->Clear();
	pSrcPhrase->m_bHasInternalPunct = FALSE;
	
	wxString punctSet = pApp->m_punctuation[0]; // from version 1.3.6, contains spaces 
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


///////////////////////////////////////////////////////////////////////////////
// Event handlers
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the app's Idle handler
/// \remarks	Handler for the Retranslation button pressed event.
/////////////////////////////////////////////////////////////////////////////////
// BEW 17Feb10, updated for support of _DOCVER5 (no changes needed, but the
// InsertNullSourcePhrase() function called from PadWithNullSourcePhrasesAtEnd() had to
// have a number of changes to handle placeholder inserts for retranslation and for manual
// placeholder inserting & the left or right association choice)
void CRetranslation::OnButtonRetranslation(wxCommandEvent& event)
{
	// refactored 16Apr09
    // Since the Do a Retranslation toolbar button has an accelerator table hot key (CTRL-R
    // see CMainFrame) and wxWidgets accelerator keys call menu and toolbar handlers even
    // when they are disabled, we must check for a disabled button and return if disabled.
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	CAdapt_ItDoc* pDoc = pApp->GetDocument();
	CMainFrame* pFrame = pApp->GetMainFrame();
	wxToolBarBase* pToolBar = pFrame->GetToolBar();
	wxASSERT(pToolBar != NULL);
	if (!pToolBar->GetToolEnabled(ID_BUTTON_RETRANSLATION))
	{
		::wxBell();
		return;
	}
	
	if (gbIsGlossing)
	{
		// IDS_NOT_WHEN_GLOSSING
		wxMessageBox(_(
					   "This particular operation is not available when you are glossing."),
					 _T(""), wxICON_INFORMATION);
		return;
	}
	SPList* pList = new SPList; // list of the selected CSourcePhrase objects 
	wxASSERT(pList != NULL);
	SPList* pSrcPhrases = pApp->m_pSourcePhrases;
	CPile* pStartingPile = NULL;
	
    // determine the active sequ number, so we can determine whether or not the active
    // location lies within the selection (if its not in the selection, we will need to
    // recreate the phrase box at the former active location when done - be careful,
    // because if the active location lies after the selection and the selection contains
    // null src phrases or merged phrases, then the value of nFormerActiveSequNum will need
    // to be updated as we remove null src phrases and / or unmerge merged phrases)
	int nSaveActiveSequNum = pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber;
	
	CSourcePhrase* pSrcPhrase;
	wxString strAdapt; // accumulates the existing adaptation text for the selection
	strAdapt.Empty();
	
	wxString str; // a temporary storage string
	str.Empty();
	wxString str2; // second temporary storage string
	str2.Empty();
	wxString strSource; // the source text which is to be retranslated
	strSource.Empty();
	CCellList::Node* pos = pApp->m_selection.GetFirst();
	int nCount = pApp->m_selection.GetCount(); // number of src phrase instances in selection
	
	if (nCount == (int)pApp->m_pSourcePhrases->GetCount())
	{
		//IDS_RETRANS_NOT_ALL_OF_DOC
		wxMessageBox(_(
					   "Sorry, for a retranslation your selection must not include all the document contents - otherwise there would be no possible place for the phrase box afterwards. Shorten the selection then try again."),
					 _T(""),wxICON_INFORMATION);
		return;
	}
	
	CCell* pCell = (CCell*)pos->GetData();
	CPile* pPile = pCell->GetPile(); // get the pile first in the selection
	pos = pos->GetNext(); // needed for our CCellList to effect MFC's GetNext()
	
	pStartingPile = pPile; 
	pSrcPhrase = pPile->GetSrcPhrase();
	
	int nSaveSequNum = pSrcPhrase->m_nSequNumber; // save its sequ number, everything depends
	// on this - its the first in the sublist list
    // get a list of the selected CSourcePhrase instances (some might not be minimal ones
    // so if this is the case we must later restore them to minimal ones, and some might be
    // placeholders, so these must be later eliminated after their text, if any, is
    // preserved & any punctuation transferred) and also accumulate the words in the source
    // and target text into string variables
	GetSelectedSourcePhraseInstances(pList, strSource, strAdapt);
	
	// check that the selection is text of a single type - if it isn't, then tell the user and
	// abandon the operation
	bool bConstType = IsConstantType(pList);
	if (!bConstType)
	{
		// IDS_TYPE_CHANGE_ERR
		wxMessageBox(_(
					   "Sorry, the selection contains text of more than one type. Select only one text type at a time. The operation will be ignored."),
					 _T(""), wxICON_EXCLAMATION);
		GetView()->RemoveSelection();
		delete pList;
		pList = (SPList*)NULL;
		pApp->m_pTargetBox->SetFocus();
		pApp->m_pTargetBox->SetSelection(pApp->m_nStartChar,pApp->m_nEndChar);
		GetView()->Invalidate();
		GetLayout()->PlaceBox();
		return;
	}
	
    // check for a retranslation in the selection, and abort the retranslatiaon operation
    // if there is one
	if (IsRetranslationInSelection(pList))
	{
		// IDS_NO_RETRANSLATION_IN_SEL
		wxMessageBox(_(
					   "Sorry, but this operation is not permitted when the selection contains any part of a retranslation. First remove the retranslation and then try again."),
					 _T(""), wxICON_EXCLAMATION);
		pList->Clear();
		delete pList;
		pList = (SPList*)NULL;
		GetView()->RemoveSelection();
		pApp->m_pTargetBox->SetFocus();
		pApp->m_pTargetBox->SetSelection(pApp->m_nStartChar,pApp->m_nEndChar);
		GetView()->Invalidate();
		GetLayout()->PlaceBox();
		return;
	}
	
    // need to clobber the selection here, so the selection globals will be set to -1,
    // otherwise RecalcLayout will fail at its RestoreSelection() call; and any unmergers
    // or other layout changes done immediately below will invalidate layout pointers which
    // RemoveSelection() relies on, and produce a crash.
	GetView()->RemoveSelection();
	
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
	if (pApp->m_pActivePile != NULL)
	{
		// the active location is not within the retranslation section, so update before
		// throwing it all out
		GetView()->MakeLineFourString(pApp->m_pActivePile->GetSrcPhrase(),pApp->m_targetPhrase);
		GetView()->RemovePunctuation(pDoc,&pApp->m_targetPhrase,from_target_text);
		gbInhibitLine4StrCall = TRUE;
		bool bOK = GetView()->StoreText(pApp->m_pKB,pApp->m_pActivePile->GetSrcPhrase(),pApp->m_targetPhrase);
		gbInhibitLine4StrCall = FALSE;
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
			int nFormerStrip = pApp->m_pActivePile->GetStripIndex();
			pDoc->ResetPartnerPileWidth(pApp->m_pActivePile->GetSrcPhrase()); // & mark 
			// the active strip invalid
			int nCurStripIndex = pStartingPile->GetStripIndex();
			if (nCurStripIndex != nFormerStrip)
			{
				CStrip* pFormerStrip = (CStrip*)GetLayout()->GetStripArray()->Item(nFormerStrip);
				CPile* pItsFirstPile = (CPile*)pFormerStrip->GetPilesArray()->Item(0);
				CSourcePhrase* pItsFirstSrcPhrase = pItsFirstPile->GetSrcPhrase();
				pDoc->ResetPartnerPileWidth(pItsFirstSrcPhrase,TRUE); // TRUE is 
				// bNoActiveLocationCalculation
			}
		}
	}
	pApp->m_targetPhrase.Empty();
	if (pApp->m_pTargetBox != NULL)
	{
		pApp->m_pTargetBox->ChangeValue(pApp->m_targetPhrase);
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
	
	// check for any null source phrases in the selection, and delete any found from both the
	// temporary list (pList), and from the original source phrases list on the app (see above)
	while (IsNullSrcPhraseInSelection(pList))
	{
		RemoveNullSrcPhraseFromLists(pList, pSrcPhrases, nCount, nEndSequNum,
									 bActiveLocAfterSelection, nSaveActiveSequNum);
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
	pApp->m_nActiveSequNum = nSaveActiveSequNum; // legally can be a wrong location, eg. 
	// in the retrans, & it won't break
#ifdef _NEW_LAYOUT
	GetLayout()->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
	GetLayout()->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
	pApp->m_pActivePile = GetView()->GetPile(pApp->m_nActiveSequNum);
	
	// create the CRetranslationDlg dialog
	CRetranslationDlg dlg(pApp->GetMainFrame());
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
	GetView()->GetContext(nSaveSequNum,nEndSequNum,preceding,following,precedingTgt,followingTgt);
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
	//		CSourcePhrase* pSP = pApp->m_pActivePile->m_pSrcPhrase;
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
		
        // tokenize the retranslation into a list of new CSourcePhrase instances on the
        // heap (they are incomplete - only m_key and m_nSequNumber are set); nSaveSequNum
        // is the absolute sequence number for first source phrase in the sublist - it is
        // used to define the starting sequence number to be stored on the first element of
        // the sublist, and higher numbers on succeeding ones
		nNewCount = GetView()->TokenizeTextString(pRetransList,retrans,nSaveSequNum);
		
		// augment the active sequ num if it lay after the selection
		if (bActiveLocAfterSelection && nNewCount > nCount)
			nSaveActiveSequNum += nNewCount - nCount;
		else
		{
			// augment it also if the active location lay within the selection
			// and null source phrases were inserted
			if (bActiveLocWithinSelection && nNewCount > nCount)
				nSaveActiveSequNum += nNewCount - nCount;
		}
		pApp->m_nActiveSequNum = nSaveActiveSequNum; // ensure any call to 
		// InsertNullSrcPhrase() will work right
		
        // we must have a valid layout, so we have to recalculate it before we go any
        // further, because if preceding code unmerged formerly merged phrases, or if null
        // phrases were deleted, then the layout's pointers will be clobbered; but we won't
        // draw it yet because later we must ensure the active location is not within the
        // retranslation and set it safely before a final layout calculation to get it all
        // correct
#ifdef _NEW_LAYOUT
		GetLayout()->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
		GetLayout()->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
		pApp->m_pActivePile = GetView()->GetPile(pApp->m_nActiveSequNum);
		
		// get a new valid starting pile pointer
		pStartingPile = GetView()->GetPile(nSaveSequNum);
		wxASSERT(pStartingPile != NULL);
		
		// determine if we need extra null source phrases inserted, 
		// and insert them if we do
		PadWithNullSourcePhrasesAtEnd(pDoc,pApp,pSrcPhrases,nEndSequNum,
									  nNewCount,nCount);
        // copy the retranslation's words, one per source phrase, to the constituted
        // sequence of source phrases (including any null ones) which are to display it;
        // but ignore any markers and punctuation if they were encountered when the
        // retranslation was parsed, so that the original source text's punctuation
        // settings in the document are preserved. Export will get the possibly new
        // punctuation settings by copying m_targetStr, so we do not need to alter
        // m_precPunct and m_follPunct on the document's CSourcePhrase instances.
		int nFinish = -1; // it gets set to a correct value in the following call
		BuildRetranslationSourcePhraseInstances(pRetransList,nSaveSequNum,nNewCount,
												nCount,nFinish);
        // delete the temporary list and delete the pointers to the CSourcePhrase
        // instances on the heap
		GetView()->DeleteTempList(pRetransList);
		
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
			bool bSetSafely = GetView()->SetActivePilePointerSafely(pApp,pSrcPhrases,nSaveActiveSequNum,
														 pApp->m_nActiveSequNum,nFinish);
			m_bSuppressRemovalOfRefString = FALSE; // permit RemoveRefString() in subsequent 
			// PlacePhraseBox() calls
			m_bIsRetranslationCurrent = FALSE;
			if(!bSetSafely)
			{
				// IDS_ALL_RETRANSLATIONS
				wxMessageBox(_(
							   "Warning: your document is full up with retranslations. This makes it impossible to place the phrase box anywhere in the document."),
							 _T(""), wxICON_EXCLAMATION);
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
			bSetSafely = GetView()->SetActivePilePointerSafely(pApp,pSrcPhrases,nSaveActiveSequNum,
													pApp->m_nActiveSequNum,nFinish);
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
		
		// now remove the unwanted ones - be careful, some of these single-word ones will point
		// to memory that any merged source phrases in the saved list will point to in their
		// m_pSavedWords sublists, so don't delete the memory in the latter sublists,
		// just remove the pointers!
		RemoveUnwantedSourcePhraseInstancesInRestoredList(pSrcPhrases,nCurCount,nSaveSequNum,
														  pSaveList);
		
		// we can assume nExtras is either 0 or positive
		if (nSaveActiveSequNum > nSaveSequNum + nOldCount - 1)
			nSaveActiveSequNum -= nExtras; // decrement only if it lay after the original
		// selection
		
		// renumber the sequence numbers
		GetView()->UpdateSequNumbers(0);
		
		// remove the pointers in the saved list, and delete the list, but leave the instances
		// undeleted since they are now pointed at by elements in the pSrcPhrases list
		if (pSaveList->GetCount() > 0)
		{
			pSaveList->Clear();
		}
		delete pSaveList; // don't leak memory
		pSaveList = (SPList*)NULL;
	}
	
	// delete the temporary list after removing its pointer copies (copy constructor was not
	// used on this list, so removal of pointers is sufficient)
	pList->Clear();
	delete pList;
	pList = (SPList*)NULL;
	
	// recalculate the layout
	pApp->m_nActiveSequNum = nSaveActiveSequNum;
#ifdef _NEW_LAYOUT
	GetLayout()->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
	GetLayout()->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
	pApp->m_pActivePile = GetView()->GetPile(nSaveActiveSequNum);
	
	// get the CSourcePhrase at the active location
	pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
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
	GetLayout()->m_docEditOperationType = retranslate_op;
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
		bCommandPosted = GetView()->VerticalEdit_CheckForEndRequiringTransition(-1, nextStep, TRUE);
		// no Invalidate() call made in this block, because a later point in the process
		// should draw the layout anew (I'm guessing, but I think it's a safe guess)
	}
	else
	{
		if (pSrcPhrase->m_targetStr.IsEmpty() && !pSrcPhrase->m_bHasKBEntry && !pSrcPhrase->m_bNotInKB)
		{
			pApp->m_pTargetBox->m_bAbandonable = TRUE;
			RestoreTargetBoxText(pSrcPhrase,str3); // for getting a suitable m_targetStr contents
		}
		else
		{
			str3 = pSrcPhrase->m_targetStr; // if we have something
			pApp->m_pTargetBox->m_bAbandonable = FALSE;
		}
		
        // it is okay to do the Remove call with pRefString == NULL, in fact, it must be
        // done whether NULL or not; since if it is NULL, RemoveRefString will clear
        // pSrcPhrase's m_bHasKBEntry to FALSE, which if not done, would result in a crash
        // if the user clicked on a source phrase which had its reference string manually
        // removed from the KB and then clicked on another source phrase. (The
        // StoreAdaption call in the second click would trip the first line's ASSERT.)
		CRefString* pRefString = GetView()->GetRefString(GetView()->GetKB(),pSrcPhrase->m_nSrcWords,
											  pSrcPhrase->m_key,pSrcPhrase->m_adaption);
		GetView()->RemoveRefString(pRefString,pSrcPhrase,pSrcPhrase->m_nSrcWords);
		
		pApp->m_targetPhrase = str3; // the Phrase Box can have punctuation as well as text
		pApp->m_pTargetBox->ChangeValue(str3);
		pApp->m_nStartChar = -1;
		pApp->m_nEndChar = -1;
		
		// layout again, so that the targetBox won't encroach on the next cell's adaption text 
#ifdef _NEW_LAYOUT
		GetLayout()->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
		GetLayout()->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
		pApp->m_pActivePile = GetView()->GetPile(pApp->m_nActiveSequNum);
		
		// remove selection and update the display
		GetView()->RemoveSelection();
		GetView()->Invalidate();
		GetLayout()->PlaceBox();
	}
	
	// ensure respect for boundaries is turned back on
	if (!pApp->m_bRespectBoundaries)
		GetView()->OnButtonFromIgnoringBdryToRespectingBdry(event);
	m_bInsertingWithinFootnote = FALSE; // restore default value
	gnOldSequNum = nSaveOldSequNum; // restore the value we set earlier
}

// BEW 18Feb10, modified for support of _DOCVER5 (some code added to handle transferring
// endmarker content from the last placeholder back to end of the CSourcePhrase list of
// non-placeholders, prior to showing the dialog)
void CRetranslation::OnButtonEditRetranslation(wxCommandEvent& event)
{
    // Since the Edit Retranslation toolbar button has an accelerator table hot key (CTRL-E
    // see CMainFrame) and wxWidgets accelerator keys call menu and toolbar handlers even
    // when they are disabled, we must check for a disabled button and return if disabled.
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	CAdapt_ItDoc* pDoc = pApp->GetDocument(); 
	CMainFrame* pFrame = pApp->GetMainFrame();
	wxToolBarBase* pToolBar = pFrame->GetToolBar();
	wxASSERT(pToolBar != NULL);
	if (!pToolBar->GetToolEnabled(ID_BUTTON_EDIT_RETRANSLATION))
	{
		::wxBell();
		return;
	}
	
	if (gbIsGlossing)
	{
		// IDS_NOT_WHEN_GLOSSING
		wxMessageBox(_(
					   "This particular operation is not available when you are glossing."),
					 _T(""), wxICON_INFORMATION);
		return;
	}
	SPList* pList = new SPList; // list of the CSourcePhrase objects in the retranslation section
	SPList* pSrcPhrases = pApp->m_pSourcePhrases;
	CPile* pStartingPile = NULL;
	CSourcePhrase* pSrcPhrase;
	int nSaveActiveSequNum = pApp->m_pActivePile->GetSrcPhrase()->m_nSequNumber;
	
    // get the source phrases which comprise the section which is retranslated; but first
    // check if we have a selection, and if so start from the first pile in the selection;
    // otherwise, we have an error condition.
	CCell* pCell;
	CCellList::Node* cpos;
	if (pApp->m_selectionLine != -1)
	{
		// there is a selection current
		cpos = pApp->m_selection.GetFirst();
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
										 _T(""), wxICON_EXCLAMATION);
				GetView()->RemoveSelection();
				delete pList;
				GetView()->Invalidate();
				GetLayout()->PlaceBox();
				return;
			}
		}
	}
	
	// also check that the end of the selection is also part of the retranslation,
	// if not, return
	cpos = pApp->m_selection.GetLast();
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
	GetView()->RemoveSelection();
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
	if (pApp->m_pActivePile != NULL)
	{
		CSourcePhrase* pActiveSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
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
			GetView()->MakeLineFourString(pApp->m_pActivePile->GetSrcPhrase(),pApp->m_targetPhrase);
			GetView()->RemovePunctuation(pDoc,&pApp->m_targetPhrase,from_target_text);
			if (!pApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry)
			{
				gbInhibitLine4StrCall = TRUE;
				bool bOK = GetView()->StoreText(pApp->m_pKB,pApp->m_pActivePile->GetSrcPhrase(),
									 pApp->m_targetPhrase);
				gbInhibitLine4StrCall = FALSE;
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
					int nFormerStrip = pApp->m_pActivePile->GetStripIndex();
					pDoc->ResetPartnerPileWidth(pApp->m_pActivePile->GetSrcPhrase()); // mark 
					// the owning  strip invalid
					int nCurStripIndex = pStartingPile->GetStripIndex();
					if (nCurStripIndex != nFormerStrip)
					{
						CStrip* pFormerStrip = (CStrip*)GetLayout()->GetStripArray()->Item(nFormerStrip);
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
	pApp->m_targetPhrase.Empty();
	if (pApp->m_pTargetBox->GetHandle() != NULL && pApp->m_pTargetBox->IsShown())
	{
		pApp->m_pTargetBox->ChangeValue(pApp->m_targetPhrase); // clear it
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
    // with the replacement string found in the global gReplStr
	if (m_bReplaceInRetranslation)
	{
		ReplaceMatchedSubstring(gSrchStr,gReplStr,strAdapt);
		
		// clear the globals for next time
		m_bReplaceInRetranslation = FALSE;
		gSrchStr.Empty();
		gReplStr.Empty();
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
#ifdef _DOCVER5
	// BEW 18Feb10, for docVersion = 5, the m_endMarkers member of CSourcePhrase will have
	// had an final endmarkers moved to the last placeholder, so we have to check for a
	// non-empty member on the last placeholder, and if non-empty, save it's contents to a
	// wxString, set a flag to signal this condition obtained, and in the block which
	// follows put the endmarkers back on the last CSourcePhrase which is not a placeholder
	wxString endmarkersStr = _T("");
	bool bEndHasEndMarkers = FALSE;
#endif
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
#ifdef _DOCVER5
			// likewise, test for a non-empty m_endMarkers member at the end - there can
			// only be one such member which has content - the last one
			if (!pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				endmarkersStr = pSrcPhrase->GetEndMarkers();
				bEndHasEndMarkers = TRUE;
			}
#endif
            // null source phrases in a retranslation are never stored in the KB, so we
            // need only remove their pointers from the lists and delete them from the heap
			SPList::Node* pos1 = pSrcPhrases->Find(pSrcPhrase);
			wxASSERT(pos1 != NULL); // it has to be there
			pSrcPhrases->DeleteNode(pos1);	// remove its pointer from m_pSourcePhrases list
			// on the doc
			// BEW added 13Mar09 for refactor of layout; delete its partner pile too 
			GetApp()->GetDocument()->DeletePartnerPile(pSrcPhrase);
			
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
#ifdef _DOCVER5
	// handle transferring of m_endMarkers content
	if (bEndHasEndMarkers)
	{
		SPList::Node* tpos = pList->GetLast();
		CSourcePhrase* pSPend = (CSourcePhrase*)tpos->GetData();
		pSPend->SetEndMarkers(endmarkersStr);
	}
#endif
	
    // update the sequence number in the whole source phrase list on the app & update
    // indices for bounds
	GetView()->UpdateSequNumbers(0);
	
    // now we can work out where to place the phrase box on exit from this function - it is
    // currently the nSaveActiveSequNum value, unless the active location was within the
    // selection, in which case we must make the active location the first pile after the
    // selection
	if (bActiveLocWithinSelection)
		nSaveActiveSequNum = nEndSequNum + 1;
	
    // clear the selection, else RecalcLayout() call will fail at the RestoreSelection()
    // call within it
	GetView()->RemoveSelection();
	
	// we must have a valid layout, so we have to recalculate it before we go any further,
	// because if preceding code deleted null phrases, the layout's pointers would be clobbered
	// and moving the dialog window would crash the app when Draw messages use the dud pointers
	pApp->m_nActiveSequNum = nSaveActiveSequNum; // legally can be a wrong location eg.
	// in the retrans, & nothing will break
#ifdef _NEW_LAYOUT
	GetLayout()->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
	GetLayout()->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
	pApp->m_pActivePile = GetView()->GetPile(pApp->m_nActiveSequNum);
	
	bool bConstType;
	bConstType = IsConstantType(pList); // need this only in case m_bInsertingWithinFootnote
	// needs to be set
	
	// put up the CRetranslationDlg dialog
	CRetranslationDlg dlg(pApp->GetMainFrame());
	
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
	GetView()->GetContext(nSaveSequNum,nEndSequNum,preceding,following,precedingTgt,followingTgt);
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
		nNewCount = GetView()->TokenizeTextString(pRetransList,retrans,nSaveSequNum);
		
        // ensure any call to InsertNullSrcPhrase() will work right - that function saves
        // the pApp->m_nActiveSequNum value, and increments it by how many null source
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
		pApp->m_nActiveSequNum = nSaveActiveSequNum;
		
        // we must have a valid layout, so we have to recalculate it before we go any
        // further, because if preceding code deleted null phrases, then the layout's
        // pointers will be clobbered
#ifdef _NEW_LAYOUT
		GetLayout()->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
		GetLayout()->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
		pApp->m_pActivePile = GetView()->GetPile(pApp->m_nActiveSequNum);
		
		// get a new valid starting pile pointer
		pStartingPile = GetView()->GetPile(nSaveSequNum);
		wxASSERT(pStartingPile != NULL);
		
		// determine if we need extra null source phrases inserted, and insert them if we do
		PadWithNullSourcePhrasesAtEnd(pDoc,pApp,pSrcPhrases,nEndSequNum,nNewCount,nCount);
		
		// copy the retranslation's words, one per source phrase, to the constituted sequence of
		// source phrases (including any null ones) which are to display it
		int nFinish = -1; // it gets set to a correct value in the following call
		BuildRetranslationSourcePhraseInstances(pRetransList,nSaveSequNum,nNewCount,
												nCount,nFinish);
        // delete the temporary list and delete the pointers to the CSourcePhrase instances
        // on the heap
		GetView()->DeleteTempList(pRetransList);
		
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
			bool bSetSafely = GetView()->SetActivePilePointerSafely(pApp,pSrcPhrases,nSaveActiveSequNum,
														 pApp->m_nActiveSequNum,nFinish);
			m_bSuppressRemovalOfRefString = FALSE; // permit RemoveRefString() in subsequent 
			// PlacePhraseBox() calls
			m_bIsRetranslationCurrent = FALSE;
			if(!bSetSafely)
			{
				// IDS_ALL_RETRANSLATIONS
				wxMessageBox(_(
							   "Warning: your document is full up with retranslations. This makes it impossible to place the phrase box anywhere in the document."),
							 _T(""), wxICON_EXCLAMATION);
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
			bSetSafely = GetView()->SetActivePilePointerSafely(pApp,pSrcPhrases,nSaveActiveSequNum,
													pApp->m_nActiveSequNum,nFinish);
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
			GetApp()->GetDocument()->CreatePartnerPile(pSPhr);
			
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
		GetView()->UpdateSequNumbers(0);
		
        // remove the pointers in the saved list, and delete the list, but leave the
        // instances undeleted since they are now pointed at by elements in the pSrcPhrases
        // list
		if (pSaveList->GetCount() > 0)
		{
			pSaveList->Clear();
		}
		delete pSaveList; // don't leak memory
	}
	
	// delete the temporary list after removing its pointer copies
	pList->Clear();
	delete pList;
	
    // recalculate the layout from the first strip in the selection, 
    // to force the text to change color
	pApp->m_nActiveSequNum = nSaveActiveSequNum;
#ifdef _NEW_LAYOUT
	GetLayout()->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
	GetLayout()->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
	pApp->m_pActivePile = GetView()->GetPile(nSaveActiveSequNum);
	
	// get the CSourcePhrase at the active location
	pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
	wxASSERT(pSrcPhrase != NULL);
	
	// determine the text to be shown, if any, in the target box when it is recreated
	// BEW additions 08Sep08 for support of vertical editing mode
	wxString str3; // use this one for m_targetStr contents
	// define the operation type, so PlacePhraseBoxInLayout() can do its job correctly
	GetLayout()->m_docEditOperationType = edit_retranslation_op;
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
		bCommandPosted = GetView()->VerticalEdit_CheckForEndRequiringTransition(-1, nextStep, TRUE);
		// no Invalidate() call made in this block, because a later point in the process
		// should draw the layout anew (I'm guessing, but I think it's a safe guess)
	}
	else
	{
		str3.Empty();
		
		// we want text with punctuation, for the 4-line version
		if (!pSrcPhrase->m_targetStr.IsEmpty() && 
			(pSrcPhrase->m_bHasKBEntry || pSrcPhrase->m_bNotInKB))
		{
			str3 = pSrcPhrase->m_targetStr;
			pApp->m_pTargetBox->m_bAbandonable = FALSE;
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
				pApp->m_pTargetBox->m_bAbandonable = TRUE;
				RestoreTargetBoxText(pSrcPhrase,str3); // for getting a suitable 
				// m_targetStr contents
			}
			else
			{
				str3 = pSrcPhrase->m_targetStr;
				pApp->m_pTargetBox->m_bAbandonable = FALSE;
			}
		}
		
        // it is okay to do the Remove call with pRefString == NULL, in fact, it must be
        // done whether NULL or not; since if it is NULL, RemoveRefString will clear
        // pSrcPhrase's m_bHasKBEntry to FALSE, which if not done, would result in a crash
        // if the user clicked on a source phrase which had its reference string manually
        // removed from the KB and then clicked on another source phrase. (The
        // StoreAdaption call in the second click would trip the first line's ASSERT.)
		CRefString* pRefString = GetView()->GetRefString(GetView()->GetKB(),pSrcPhrase->m_nSrcWords,
											  pSrcPhrase->m_key,pSrcPhrase->m_adaption);
		GetView()->RemoveRefString(pRefString,pSrcPhrase,pSrcPhrase->m_nSrcWords);
		
		pApp->m_targetPhrase = str3;
		if (pApp->m_pTargetBox != NULL)
		{
			pApp->m_pTargetBox->ChangeValue(str3);
		}
		
        // layout again, so that the targetBox won't encroach on the next cell's adaption
        // text (can't just layout the strip, because if the text is long then source
        // phrases get pushed off into limbo and we get access violation & null pointer
        // returned in the GetPile call)
#ifdef _NEW_LAYOUT
		GetLayout()->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
		GetLayout()->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
		pApp->m_pActivePile = GetView()->GetPile(pApp->m_nActiveSequNum);
		
		// get a new valid active pile pointer
		pApp->m_pActivePile = GetView()->GetPile(pApp->m_nActiveSequNum);
		
		pApp->m_nStartChar = -1;
		pApp->m_nEndChar = -1;
		
		// remove selection and update the display
		GetView()->RemoveSelection();
		GetView()->Invalidate();
		GetLayout()->PlaceBox();
	}
	
	// ensure respect for boundaries is turned back on
	if (!pApp->m_bRespectBoundaries)
		GetView()->OnButtonFromIgnoringBdryToRespectingBdry(event);
	m_bInsertingWithinFootnote = FALSE; // restore default value
}

// BEW 18Feb10, modified for support of _DOCVER5 (some code added to handle transferring
// endmarker content from the last placeholder back to end of the CSourcePhrase list of
// non-placeholders)
void CRetranslation::OnRemoveRetranslation(wxCommandEvent& event)
{
	// Invalid function when glossing is ON, so it just returns.
	if (gbIsGlossing)
	{
		// IDS_NOT_WHEN_GLOSSING
		wxMessageBox(_("This particular operation is not available when you are glossing."),
					 _T(""), wxICON_INFORMATION);
		return;
	}
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	CAdapt_ItDoc* pDoc = pApp->GetDocument();
	SPList* pList = new SPList; // list of the CSourcePhrase objects in the retranslation section
	SPList* pSrcPhrases = pApp->m_pSourcePhrases;
	CPile* pStartingPile = NULL;
	CSourcePhrase* pSrcPhrase = NULL;
	CCell* pCell = NULL;
	
    // get the source phrases which comprise the section which is retranslated; first check
    // if we have a selection, and if so start from the first pile in the selection;
    // otherwise, the location to start from must be the target box's location (ie. the
    // active pile); if it's neither of those then we have an error condition
	CCellList::Node* cpos;
	if (pApp->m_selectionLine != -1)
	{
		// there is a selection current
		cpos = pApp->m_selection.GetFirst();
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
										 _T(""), wxICON_EXCLAMATION);
				GetView()->RemoveSelection();
				delete pList;
				GetView()->Invalidate();
				GetLayout()->PlaceBox();
				return;
			}
		}
	}
	
	// also check that the end of the selection is also part of the 
	// retranslation, if not, return
	cpos = pApp->m_selection.GetLast();
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
	GetView()->RemoveSelection();
	CPile* pFirstPile = 0;
	GetRetranslationSourcePhrasesStartingAnywhere(pStartingPile,pFirstPile,pList);
	
	int nStartingSequNum = pFirstPile->GetSrcPhrase()->m_nSequNumber;
	
    // We must first check if the active location is outside the selection - since there
    // could be a just-edited entry in the phrase box which is not yet entered in the
    // knowledge base, and the active location's source phrase doesn't yet have its
    // m_adaption and m_targetStr members updated, so we must check for this condition and
    // if it obtains then we must first update everything at the active location before we
    // proceed
	if (pApp->m_pActivePile != NULL)
	{
		CSourcePhrase* pActiveSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
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
			GetView()->MakeLineFourString(pApp->m_pActivePile->GetSrcPhrase(),pApp->m_targetPhrase);
			GetView()->RemovePunctuation(pDoc, &pApp->m_targetPhrase, from_target_text);
			if (pApp->m_targetPhrase != pApp->m_pActivePile->GetSrcPhrase()->m_adaption)
			{
				gbInhibitLine4StrCall = TRUE;
				bool bOK = GetView()->StoreText(pApp->m_pKB,pApp->m_pActivePile->GetSrcPhrase(),
									 pApp->m_targetPhrase);
				gbInhibitLine4StrCall = FALSE;
				if (!bOK)
					return; // can't proceed until a valid adaption (which could be null)
				// is supplied for the former active pile's srcPhrase
				else
				{
					// make the former strip be marked invalid - new layout
					// code will then tweak the layout from that point on
					int nFormerStrip = pApp->m_pActivePile->GetStripIndex();
					int nCurStripIndex = pStartingPile->GetStripIndex();
					if (nCurStripIndex != nFormerStrip)
					{
						CStrip* pFormerStrip = (CStrip*)
						GetLayout()->GetStripArray()->Item(nFormerStrip);
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
	CMainFrame* pMainFrm = pApp->GetMainFrame();
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
#ifdef _DOCVER5
	// BEW 18Feb10, for docVersion = 5, the m_endMarkers member of CSourcePhrase will have
	// had an final endmarkers moved to the last placeholder, so we have to check for a
	// non-empty member on the last placeholder, and if non-empty, save it's contents to a
	// wxString, set a flag to signal this condition obtained, and in the block which
	// follows put the endmarkers back on the last CSourcePhrase which is not a placeholder
	wxString endmarkersStr = _T("");
	bool bEndHasEndMarkers = FALSE;
#endif
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
#ifdef _DOCVER5
			// likewise, test for a non-empty m_endMarkers member at the end - there can
			// only be one such member which has content - the last one
			if (!pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				endmarkersStr = pSrcPhrase->GetEndMarkers();
				bEndHasEndMarkers = TRUE;
			}
#endif
            // null source phrases in a retranslation are never stored in the KB, so we
            // need only remove their pointers from the lists and delete them from the heap
			nDeletions++; // count it
			SPList::Node* pos1 = pSrcPhrases->Find(pSrcPhrase); 
			wxASSERT(pos1 != NULL); // it has to be there
			pSrcPhrases->DeleteNode(pos1); // remove its pointer from m_pSourcePhrases
			// list on the doc
			
			// BEW added 13Mar09 for refactor of layout; delete its partner pile too 
			GetApp()->GetDocument()->DeletePartnerPile(pSrcPhrase);
			
			delete pSrcPhrase->m_pMedialPuncts;
			delete pSrcPhrase->m_pMedialMarkers;
			pSrcPhrase->m_pSavedWords->Clear();
			delete pSrcPhrase->m_pSavedWords;
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
			if (GetView()->IsItNotInKB(pSrcPhrase))
				pSrcPhrase->m_bNotInKB = TRUE;
			else
				pSrcPhrase->m_bNotInKB = FALSE;
			pSrcPhrase->m_bHasKBEntry = FALSE;
			pSrcPhrase->m_bBeginRetranslation = FALSE;
			pSrcPhrase->m_bEndRetranslation = FALSE;
			
			// we have to restore the original punctuation too
			RestoreOriginalPunctuation(pSrcPhrase);
			
			// these pSrcPhrase instances have to have their partner piles' 
			// widths recalculated
			GetApp()->GetDocument()->ResetPartnerPileWidth(pSrcPhrase);
		}
	}
	
	if ((int)pList->GetCount() < nCount)
	{
		// handle transferring the indication of the end of a free translation
		if (bEndIsAlsoFreeTransEnd)
		{
			SPList::Node* spos = pList->GetLast();
			CSourcePhrase* pSPend = (CSourcePhrase*)spos->GetData();
			pSPend->m_bEndFreeTrans = TRUE;
		}
#ifdef _DOCVER5
		// handle transferring of m_endMarkers content
		if (bEndHasEndMarkers)
		{
			SPList::Node* pos = pList->GetLast();
			CSourcePhrase* pSPend = (CSourcePhrase*)pos->GetData();
			pSPend->SetEndMarkers(endmarkersStr);
		}
#endif
		// update the sequence numbers to be consecutive across the deletion location
		GetView()->UpdateSequNumbers(nStartingSequNum);
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
	pApp->m_nActiveSequNum = nStartingSequNum;
	
	// define the operation type, so PlaceBox() // can do its job correctly
	GetLayout()->m_docEditOperationType = remove_retranslation_op;
	
	// now do the recalculation of the layout & update the active pile pointer
#ifdef _NEW_LAYOUT
	GetLayout()->RecalcLayout(pSrcPhrases, keep_strips_keep_piles);
#else
	GetLayout()->RecalcLayout(pSrcPhrases, create_strips_keep_piles);
#endif
	pApp->m_pActivePile = GetView()->GetPile(pApp->m_nActiveSequNum); // need this up-to-date so that
	// RestoreTargetBoxText( ) call will not fail in the code which is below
	// get the text to be displayed in the target box, if any
	SPList::Node* spos = pList->GetFirst();
	pSrcPhrase = (CSourcePhrase*)spos->GetData();
	wxString str3;
	if (pSrcPhrase->m_targetStr.IsEmpty() && !pSrcPhrase->m_bHasKBEntry && 
		!pSrcPhrase->m_bNotInKB)
	{
		pApp->m_pTargetBox->m_bAbandonable = TRUE;
		RestoreTargetBoxText(pSrcPhrase,str3); // for getting a suitable 
		// m_targetStr contents
	}
	else
	{
		str3 = pSrcPhrase->m_targetStr; // if we have something
		pApp->m_pTargetBox->m_bAbandonable = FALSE;
	}
	pApp->m_targetPhrase = str3; // update what is to be shown in the phrase box
	
	// ensure the selection is removed
	GetView()->RemoveSelection();
	
	// scroll if necessary
	pApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);
	
	pList->Clear();
	delete pList;
	
	GetView()->Invalidate();
	GetLayout()->PlaceBox();
	
	// ensure respect for boundaries is turned back on
	if (!pApp->m_bRespectBoundaries)
		GetView()->OnButtonFromIgnoringBdryToRespectingBdry(event);
	m_bInsertingWithinFootnote = FALSE; // restore default value
}

void CRetranslation::OnRetransReport(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	
	// BEW added 05Jan07 to enable work folder on input to be restored when done
	wxString strSaveCurrentDirectoryFullPath = GetApp()->GetDocument()->GetCurrentDirectory();
	
	if (gbIsGlossing)
	{
		// IDS_NOT_WHEN_GLOSSING
		wxMessageBox(_(
					   "This particular operation is not available when you are glossing."),
					 _T(""),wxICON_INFORMATION);
		return;
	}
	wxASSERT(pApp != NULL);
	CAdapt_ItDoc* pDoc;
	CPhraseBox* pBox;
	CAdapt_ItView* pView;
	pApp->GetBasePointers(pDoc,pView,pBox); // this is 'safe' when no doc is open
	wxString name; // name for the document, to be used in the report
	
	pApp->m_acceptedFilesList.Clear();
	int answer;
	
    // only put up the message box if a document is open (and the update handler also
    // disables the command if glossing is on)
	if (GetLayout()->GetStripArray()->GetCount() > 0)
	{
		// IDS_RETRANS_REPORT_ADVICE
		answer = wxMessageBox(_(
								"The retranslation report will be based on this open document only.\nTo get a report based on many or all documents,\nclose the document and select this command again.\nDo you want a report only for this document?"),
							  _T(""),wxYES_NO);
		if (!(answer == wxYES))
		{
			// a "Yes" answer is a choice for reporting only for the current document,
			// a "No" answer exits to allow the user to close the document and then
			// the report can be chosen again and it will do all documents
			return;
		}
	}
	
	// can proceed, so get output filename and put up file dialog
	// make the working directory the "<Project Name>" one
	bool bOK;
	bOK = ::wxSetWorkingDirectory(pApp->m_curProjectPath); // ignore failures
	int len;
	wxString reportFilename,defaultDir;
	if (GetLayout()->GetStripArray()->GetCount() > 0)
	{
		wxASSERT(pDoc != NULL);
		reportFilename = pApp->m_curOutputFilename;
		
		// make a suitable default output filename for the export function
		len = reportFilename.Length();
		reportFilename.Remove(len-4,4); // remove the .adt or .xml extension
		name = reportFilename; // use for the document name in the report
		reportFilename += _(" report.txt"); // make it a *.txt file type // localization?
	}
	else
	{
		// construct a general default filename, and "name" will be defined in
		// DoRetranslationReport()
		reportFilename = _("retranslation report.txt"); // localization?
		name.Empty();
	}
	// set the default folder to be shown in the dialog 
	if (pApp->m_retransReportPath.IsEmpty())
	{
		defaultDir = pApp->m_curProjectPath;
	}
	else
	{
		defaultDir = pApp->m_retransReportPath;
	}
	
	// get a file dialog
	wxString filter;
	filter = _("Adapt It Reports (*.txt)|*.txt||"); //IDS_REPORT_FILTER
	wxFileDialog fileDlg(
						 (wxWindow*)wxGetApp().GetMainFrame(), // MainFrame is parent window for file dialog
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
		int length = pApp->m_targetPhrase.Length();
		pApp->m_nStartChar = length;
		pApp->m_nEndChar = length;
		pApp->m_pTargetBox->SetSelection(length,length);
		pApp->m_pTargetBox->SetFocus();
        // whm added 05Jan07 to restore the former current working directory for safety
        // sake to what it was on entry, since there was a wxSetWorkingDirectory call made
        // above
		bOK = ::wxSetWorkingDirectory(strSaveCurrentDirectoryFullPath);
		return; // user cancelled
	}
	
	// update m_retransReportPath
	wxString exportPath = fileDlg.GetPath();
	wxString fname = fileDlg.GetFilename(); 
	int nameLen = fname.Length();
	int pathLen = exportPath.Length();
	wxASSERT(nameLen > 0 && pathLen > 0);
	pApp->m_retransReportPath = exportPath.Left(pathLen - nameLen - 1);
	
	// get the user's desired path
	wxString reportPath = fileDlg.GetPath();
	
	wxFile f; //CStdioFile f;
	if( !f.Open( reportPath, wxFile::write)) 
	{
#ifdef __WXDEBUG__
		wxLogError(_("Unable to open report file.\n")); 
		wxMessageBox(_("Unable to open report file."),_T(""), wxICON_WARNING);
#endif
        // whm added 05Jan07 to restore the former current working directory for safety
        // sake to what it was on entry, since there was a wxSetWorkingDirectory call made
        // above
		bOK = ::wxSetWorkingDirectory(strSaveCurrentDirectoryFullPath);
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
	f.Write(pApp->m_eolStr);
	f.Write(header2);
	f.Write(pApp->m_eolStr);
	f.Write(pApp->m_eolStr);
#else
	pApp->ConvertAndWrite(wxFONTENCODING_UTF8,&f,header1); // use UTF-8 encoding
	pApp->ConvertAndWrite(wxFONTENCODING_UTF8,&f,pApp->m_eolStr); // use UTF-8 encoding
	pApp->ConvertAndWrite(wxFONTENCODING_UTF8,&f,header2); // use UTF-8 encoding
	pApp->ConvertAndWrite(wxFONTENCODING_UTF8,&f,pApp->m_eolStr); // use UTF-8 encoding
	pApp->ConvertAndWrite(wxFONTENCODING_UTF8,&f,pApp->m_eolStr); // use UTF-8 encoding
#endif
	
	// save entry state (only necessary if entry state had book mode on)
	BookNamePair* pSave_BookNamePair = pApp->m_pCurrBookNamePair;
	int nSave_BookIndex = pApp->m_nBookIndex;
	wxString save_bibleBooksFolderPath = pApp->m_bibleBooksFolderPath;
	
	// output report data
	wxArrayString* pFileList = &pApp->m_acceptedFilesList;
	if (GetLayout()->GetStripArray()->GetCount() > 0)
	{
		// a document is open, so only do the report for this current document
		wxASSERT(pFileList->IsEmpty()); // must be empty, 
		// DoRetranslationReport() uses this as a flag
		
		DoRetranslationReport(pApp,pDoc,name,pFileList,pApp->m_pSourcePhrases,&f);
	}
	else
	{
        // no document is open, so enumerate all the doc files, and do a report based on
        // those the user chooses (remember that in our version of this SDI app, when no
        // document is open, in fact we have an open unnamed empty document, so pDoc is
        // still valid)
		// BEW modified 06Sept05 for support of Bible book folders in the Adaptations
		// folder 
		wxASSERT(pDoc != NULL);
		
		// determine whether or not there are book folders present
        // whm note: AreBookFoldersCreated() has the side effect of changing the current
        // work directory to the passed in pApp->m_curAdaptionsPath.
		gbHasBookFolders = pApp->AreBookFoldersCreated(pApp->m_curAdaptionsPath);
		
		// do the Adaptations folder's files first
        // whm note: EnumerateDocFiles() has the side effect of changing the current work
        // directory to the passed in pApp->m_curAdaptionsPath.
		bool bOK = pApp->EnumerateDocFiles(pDoc, pApp->m_curAdaptionsPath);
		if (bOK)
		{
			// bale out if there are no files to process, and no book folders too
			if (pApp->m_acceptedFilesList.GetCount() == 0 && !gbHasBookFolders)
			{
				// nothing to work on, so abort the operation
				// IDS_NO_DOCUMENTS_YET
				wxMessageBox(_(
							   "Sorry, there are no saved document files yet for this project. At least one document file is required for the operation you chose to be successful. The command will be ignored."),
							 _T(""),wxICON_EXCLAMATION);
                // whm added 05Jan07 to restore the former current working directory for
                // safety sake to what it was on entry, since the EnumerateDocFiles call
                // made above changes the current working dir to the Adaptations folder
                // (MFC version did not add the line below)
				bOK = ::wxSetWorkingDirectory(strSaveCurrentDirectoryFullPath);
				return;
			}
			// because of prior EnumerateDocFiles call, pFileList will have
			// document filenames in it
			DoRetranslationReport(pApp,pDoc,name,pFileList,pApp->m_pSourcePhrases,&f);
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
			bool bOK = (::wxSetWorkingDirectory(pApp->m_curAdaptionsPath) && 
						finder.Open(pApp->m_curAdaptionsPath));
			if (!bOK)
			{
				// highly unlikely, so English will do
				wxString s1, s2, s3;
				s1 = _T(
						"Failed to set the current directory to the Adaptations folder in OnRetransReport function, ");
				s2 = _T(
						"processing book folders, so the book folder document files contribute nothing.");
				s3 = s3.Format(_T("%s%s"),s1.c_str(),s2.c_str());
				wxMessageBox(s3,_T(""), wxICON_EXCLAMATION);
                // whm added 05Jan07 to restore the former current working directory for
                // safety sake to what it was on entry, since the wxSetWorkingDirectory
                // call made above changes the current working dir to the Adaptations
                // folder (MFC version did not add the line below)
				bOK = ::wxSetWorkingDirectory(strSaveCurrentDirectoryFullPath);
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
					if (finder.Exists(pApp->m_curAdaptionsPath + pApp->PathSeparator + str))
					{
                        // User-defined folders can be in the Adaptations folder without
                        // making the app confused as to whether or not Bible Book folders
                        // are present or not
						
						// we have found a folder, check if it matches one of those in
						// the array of BookNamePair structs (using the seeName member)
						if (pApp->IsDirectoryWithin(str,pApp->m_pBibleBooks))
						{
							// we have found a folder name which belongs to the set of
							// Bible book folders, so construct the required path to the
							// folder and enumerate is documents then call
							// DoTransformationsToGlosses() to process any documents within
							wxString folderPath = pApp->m_curAdaptionsPath;
							folderPath += pApp->PathSeparator + str; 
							
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
							bOK = pApp->EnumerateDocFiles(pDoc, folderPath, TRUE); // TRUE 
							// == suppress dialog
							if (!bOK)
							{
                                // don't process any directory which gives an error, but
                                // continue looping -- this is a highly unlikely error, so
                                // an English message will do
								wxString errStr;
								errStr = errStr.Format(_T(
														  "Error returned by EnumerateDocFiles in Book Folder loop, directory %s skipped."),
													   folderPath.c_str());
								wxMessageBox(errStr,_T(""), wxICON_EXCLAMATION);
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
							DoRetranslationReport(pApp,pDoc,name,pFileList,
												  pApp->m_pSourcePhrases,&f);
							// restore parent folder as current
							bOK = ::wxSetWorkingDirectory(pApp->m_curAdaptionsPath); 
							wxASSERT(bOK);
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
		pApp->m_acceptedFilesList.Clear();
	}
	
	// close the file
	f.Close();
	
	// restore the former book mode parameters (even if book mode was not on on entry)
	pApp->m_pCurrBookNamePair = pSave_BookNamePair;
	pApp->m_nBookIndex = nSave_BookIndex;
	pApp->m_bibleBooksFolderPath = save_bibleBooksFolderPath;
	// now, if the user opens the Document tab of the Start Working wizard, and book
	// mode is on, then at least the path and index and book name are all consistent
	
    // make sure that book mode is off if there is no valid folder path (if there are docs
    // in book folders, they will store a T (ie. TRUE) for the book mode saved value and so
    // when opened they will turn book mode back on, but if we started with book mode off,
    // then m_bibleBooksFolderPath would be empty, and if we attempt to open the Document
    // tab of the wizard after finishing the report, then we'd get a crash - book mode
    // would be on, but the folder path undefined, -> crash when OnSetActive() of the
    // wizard is called - so the code below ensures this can't happen)
	if (pApp->m_bBookMode)
	{
		if (pApp->m_bibleBooksFolderPath.IsEmpty())
		{
			// set safe defaults for when mode is off
			pApp->m_bBookMode = FALSE;
			pApp->m_nBookIndex = -1;
			pApp->m_nDefaultBookIndex = 39;
			pApp->m_nLastBookIndex = 39;
		}
	}
	
	int length = pApp->m_targetPhrase.Length();
	pApp->m_nStartChar = length;
	pApp->m_nEndChar = length;
	if (pApp->m_pTargetBox != NULL && pApp->m_pTargetBox->IsShown())
	{
		pApp->m_pTargetBox->SetSelection(length,length);
		pApp->m_pTargetBox->SetFocus();
	}
	// BEW added 05Jan07 to restore the former current working directory
	// to what it was on entry
	bOK = ::wxSetWorkingDirectory(strSaveCurrentDirectoryFullPath);
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
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT( pApp != NULL);
	
	if (!gbIsGlossing && pApp->m_bKBReady && pApp->m_pKB != NULL)
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
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	if (gbIsGlossing || gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_selectionLine != -1)
	{
		// we require both head and tail of the selection to lie within the retranslation
		CCellList::Node* cpos = pApp->m_selection.GetFirst();
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
			cpos = pApp->m_selection.GetLast();
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
		if (pApp->m_pTargetBox != NULL)
		{
			if (pApp->m_pTargetBox->IsShown())
			{
				CSourcePhrase* pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
				if (pSrcPhrase->m_bRetranslation)
				{
					event.Enable(TRUE);
					return;
				}
			}
		}
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
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	if (gbIsGlossing || gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (!gbIsGlossing && pApp->m_selectionLine != -1)
	{
		// we require both head and tail of the selection to lie within the retranslation
		CCellList::Node* cpos = pApp->m_selection.GetFirst();
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
			cpos = pApp->m_selection.GetLast();
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
		if (pApp->m_pTargetBox != NULL)
		{
			if (pApp->m_pTargetBox->IsShown())
			{
				CSourcePhrase* pSrcPhrase = pApp->m_pActivePile->GetSrcPhrase();
				if (pSrcPhrase->m_bRetranslation)
				{
					event.Enable(TRUE);
					return;
				}
			}
		}
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
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	if (gbIsGlossing || gbShowTargetOnly)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_pActivePile == NULL)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_selectionLine != -1)
	{
        // if there is at least one srcPhrase with m_bRetranslation == TRUE, then disable
        // the button
		CCellList::Node* pos = pApp->m_selection.GetFirst();
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

#endif
