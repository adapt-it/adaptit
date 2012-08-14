/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ExportFunctions.cpp
/// \author			Bruce Waters, revised for wxWidgets by Bill Martin
/// \date_created	31 January 2008
/// \date_revised	31 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file containing export functions used by Adapt It.
/// BEW 12Apr10, all needed changes for supporting doc version 5 have been done for this file
/// BEW 23June12 added Xhtml export, and an "Export&XHTML..." menu item - the xhtml
/// functionality is not quite finished by end of June but will be shortly
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
	#pragma implementation "ExportFunctions.h"
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

#include "BString.h"
#include <wx/list.h>
#include <wx/progdlg.h> // for wxProgressDialog
#include <wx/tokenzr.h>
#include <wx/filename.h>

// BEW removed 15Jun11 until we support OXES
// BEW reinstated 19May12, for OXES v1 support
//#include "Oxes.h"
#include "Xhtml.h" // BEW 9Jun12
#include "Adapt_It.h"
#include "helpers.h"
#include "ExportFunctions.h"
#include "Adapt_ItDoc.h"
#include "Adapt_ItView.h"
#include "MainFrm.h"
#include "CollabUtilities.h"
#include "Adapt_ItCanvas.h"
#include "KB.h"
#include "ExportInterlinearDlg.h"
#include "ExportSaveAsDlg.h"
#include "PlaceInternalMarkers.h"
#include "PlaceRetranslationInternalMarkers.h"
#include "WaitDlg.h"
#include "Stack.h"
#include "XML.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

extern bool gbIsUnstructuredData;

// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingVisible; // TRUE makes Adapt It revert to Shoebox functionality only

extern bool bPlaceFreeTransInRTFText;	// default is TRUE

extern bool bPlaceBackTransInRTFText;	// default is FALSE

extern bool gbGlossingUsesNavFont;

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

extern bool bPlaceAINotesInRTFText;		// default is FALSE

extern const wxChar* filterMkr;

extern wxString embeddedWholeMkrs;

extern wxString embeddedWholeEndMkrs;

extern wxString charFormatMkrs;

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

extern wxArrayString m_exportBareMarkers;

extern wxArrayInt m_exportFilterFlags;

extern wxString charFormatMkrs;

extern wxString charFormatEndMkrs;

/// This global is defined in Adapt_It.cpp.
extern wxChar gSFescapechar; // the escape char used for start of a standard format marker

extern wxString btHaltingMarkers;

extern wxString freeHaltingMarkers;

extern CSourcePhrase* gpSrcPhrase; // defined in Adapt_ItView.cpp

// BEW 8Jun10, removed support for checkbox "Recognise standard format
// markers only following newlines"
// This global is defined in Adapt_It.cpp.
//extern bool gbSfmOnlyAfterNewlines;

extern SPList gSrcPhrases;

extern const wxChar* filterMkrEnd;

/// This global is defined in Adapt_It.cpp.
extern bool	gbRTL_Layout;	// ANSI version is always left to right reading; this flag can only
							// be changed in the Unicode version, using the extra Layout menu

// Define type safe pointer lists
#include "wx/listimpl.cpp"

// Moved here for Version 3.
// We could use a CMap used as a dictionary associating the style "keys" with their "values."
// Since I can't get CMap to work with a CString key and CString value, I'll use the
// Standard Template Library (STL) map
// The map "key" will be the standard format marker (without backslash).
// The map "value" will be the in-document RTF style tags.
// Version 3 Note: rtfTagsMap moved to global space in the View
// WX Note: Rather than using an std:map from the STL, we'll use the wxHashMap class which acts
// a lot like the STL and is an integral part of the wxWidgets library. The hash map class we've
// declared is MapBareMkrToRTFTags, declared in the View's header file.
MapBareMkrToRTFTags rtfTagsMap;
MapBareMkrToRTFTags::iterator rtfIter; // wx note: rtfIter declared locally as needed

// Returns the input text with all \vn num \vt marker combinations changed to \v num
// Some places use \vn and \vt (non-USFM markers) instead of \v -- we need to correct
// for this whenever it happens
// Used in: DoExportAsXhtml()
// BEW created 21May12
wxString ChangeMkrs_vn_vt_To_v(wxString text)
{
	wxString vnMkr = _T("\\vn "); // include space which precedes the following verse number
	wxString vtMkr = _T("\\vt"); // the "verse text" marker
	wxString vMkr = _T("\\v "); // include space preceding the verse number
	wxString left; left.Empty();
	int vnLen = vnMkr.Len();
	int vtLen = vtMkr.Len();
	int offset = wxNOT_FOUND;
	offset = FindFromPos(text, vnMkr, 0);
	if (offset == wxNOT_FOUND)
	{
		// if \vn is not in the SFM text, then \vt won't be either, so return the text
		// unchanged to the caller
		return text;
	}
	else
	{
		// The \vn and \vt markers exist in the text, so change \vn to \v, and remove all
		// \vt markers. This function should be called before RemoveMultipleSpaces() is
		// called, in clase the removals cause some multiple spaces.
		left = text.Left(offset);
		left += vMkr;
		offset += vnLen;
		text = text.Mid(offset); // copy from where the start of the verse num should be
		offset = 0;
		do {
			offset = FindFromPos(text, vnMkr, offset);
			if (offset == wxNOT_FOUND)
			{
				// we've processed the last pair, so just copy what remains (left will
				// contain the \vt markers still, we use another loop to remove those)
				left += text;
				break;
			}	
			else
			{
				// we found another...
				left += text.Left(offset);
				left += vMkr;
				offset += vnLen;
				text = text.Mid(offset); // trim off the copied stuff from the start
				offset = 0;
			}
		} while (offset != wxNOT_FOUND);
		// now, find and remove all the \vt markers
		wxString text2 = left;
		left.Empty();
		offset = 0;
		do {
			offset = FindFromPos(text2, vtMkr, offset);
			if (offset == wxNOT_FOUND)
			{
				// we've processed the last pair, so just copy what remains (left will
				// contain the \vt markers still, we use another loop to remove those)
				left += text2;
				break;
			}	
			else
			{
				// we found another...
				left += text2.Left(offset);
				offset += vtLen;
				text2 = text2.Mid(offset); // bleed off the initial part
				offset = 0;
			}
		} while (offset != wxNOT_FOUND);
	}
	return left;
}

/////////////////////////////////////////////////////////////////////////////////
/// \return                         nothing
/// \remarks
/// Exports the document as USFM text, either the target text translation, the glosses
/// treated as text, source text, or free translation text; and then filters out
/// inappropriate markers and their content, and what remains is then passed to the
/// DoXhtmlExport() function in the Xhtml.cpp class for the production of xhtml. The latter
/// is saved to a folder, and Pathway support can use the export for producing various
/// commercial media formats for electronic publishing on various types of hardware.
/// 
/// Note: Stylenames used in the present xhtml implementation reflect Text Edit's
/// implementation of xhtml export; and these are incomplete when compared to the data
/// types supported by USFM (footnotes, endnotes and cross references are particular
/// problem spots). Greg Trihus says that future xhtml export will move closer to
/// supporting what Paratext supports. Jim Albright did some design specs, but work in TE
/// was done in such a way that most of those have been ignored. Therefore, expect that
/// this state of flux will require the AI team to periodically upgrade our Xhtml.h & .cpp
/// implementation to keep in step with what Trihus and others do, and possibly also what
/// pre-filtering needs to be done in DoExportAsXhtml() -- this may change in time too.
/// 
/// Note 2: the current Xhtml code handles an unsupported marker, if any such manage to
/// creep through the filters, as an xhtml comment. It therefore doesn't appear in the
/// data output as viewed on another device, but searching for comments will show which
/// markers are not supported.
void DoExportAsXhtml(enum ExportType exportType, bool bBypassFileDialog_ProtectedNavigation,
							wxString defaultDir, wxString exportFilename, wxString filter, bool bShowMessageIfSucceeded)
{
	// First determine whether or not the data is unstructured plain text - Xhtml cannot
	// handle data not structured as scripture text (in our case, that means, "as SFM or
	// USFM")
	wxString msg;
		// gpApp->m_bProtectXhtmlOutputsFolder is TRUE
	wxString bookCode; bookCode.Empty();
	wxString langCode; langCode.Empty();
	wxString text;	// a buffer built from pSrcPhrase->m_targetStr strings
					// - export the USFM to this buffer
	text.Empty();
	wxString DefaultExt;
	//bool bOK = TRUE;
	//int len = 0;

	// bale out if there are no USFM markers in the export
	if (DeclineIfUnstructuredData())
	{	
		return; // a suitable warning has been shown to the user
	}
	// bale out if there is no bookID defined, or it's invalid for a scripture export
	if (DeclineIfNoBookCode(bookCode))
	{	
		return; // a suitable warning has been shown to the user
	}
	// bale out if there is no 2-letter or 3-letter language code defined for this
	// language type (whether source, target, glosses, or free translation)
	if (DeclineIfNoIso639LanguageCode(exportType, langCode))
	{	
		return; // a suitable warning has been shown to the user
	}
	// if control gets to here, we have the correct 
	// (i)   bookID and 
	// (ii)  language type and 
	// (iii) language code 
	// set up, and a language type designation passed in,
	// so we can proceed with building the xhtml.
	// The Xhtml class is instantiated (in OnInit() at app startup) 
	// so start initializing it...
	Xhtml* pToXhtml = gpApp->m_pXhtml; // pToXhtml is a handy pointer
	pToXhtml->SetBookID(bookCode);
	pToXhtml->m_languageCode = langCode;

	// get a suitable book name
	if (gpApp->m_bCollaboratingWithParatext || gpApp->m_bCollaboratingWithBibledit)
	{
		if (gpApp->m_bookName_Current.IsEmpty())
		{
			// if m_bookName_Current has no name in it, show what's in m_CollabBookSelected
			gpApp->m_bookName_Current = gpApp->m_CollabBookSelected;
		}
		else
		{
			// if m_bookName_Current has a name in it, we'll honour it even in collab mode
			;
		}
	}
	// if at this point we've not got a book name - ask the user to supply one via the
	// book name dialog
	if (gpApp->m_bookName_Current.IsEmpty())
	{
		gpApp->GetDocument()->DoBookName();
	}

	// get a default file name - copy the current one for the adaptation document as the base
	//exportFilename = gpApp->m_curOutputFilename;
	//len = exportFilename.Length();
	
	// make a suitable default output filename for the export function
	//exportFilename.Remove(len-3,3); // remove the xml extension
	//exportFilename += _T("xhtml"); // make it an *.xhtml file type
	// get a file Save As dialog for XHTML Output
	DefaultExt = _T("xhtml");
	wxString exportPath;

	if (!bBypassFileDialog_ProtectedNavigation)
	{
		// MainFrame is parent window for file dialog
		wxFileDialog fileDlg((wxWindow*)wxGetApp().GetMainFrame(), 
			_("Filename for XHTML Export"),
			defaultDir, // passed in directory
			exportFilename,	// passed in default filename
			filter, //passed in filter string
			wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
			// GDLC wxSAVE & wxOVERWRITE_PROMPT deprecated in 2.8

		if (fileDlg.ShowModal() != wxID_OK)
		{
			gpApp->LogUserAction(_T("Cancelled DoExportAsXhtml()"));
			return; // user cancelled file dialog so return to what user was doing previously
		}
		exportPath = fileDlg.GetPath();	
	}
	else
	{
		// Set exportPath to the appropriate outputs folder for XHTML (it's _XHTML_OUTPUTS
		// unless some earlier folder choice was used and has been stored in app's
		// m_lastXhtmlOutputPath - in which case defaultDir points there instead)
		exportPath = defaultDir + gpApp->PathSeparator + exportFilename;
	}
	wxLogNull logNo; // avoid spurious messages from the system
	
	// we are committed to the task...
	gpApp->m_bXhtmlExportInProgress = TRUE;

    // get the user's desired path, & update m_lastTargetOutputPath; also send the path and
    // title to the XHTML component for inclusion in the title tag's metadata
	wxString path, fname, ext;
	wxFileName::SplitPath(exportPath, &path, &fname, &ext);
	gpApp->m_lastXhtmlOutputPath = path; // update the last xhtml output path string

	gpApp->m_pXhtml->m_myFilePath = path;
	gpApp->m_pXhtml->m_myFilename = fname;

    // make the renamed css file, save it in the same folder as the xhtml export file is
    // saved in (OnInit() has already determined the platform specific string to go in the
    // m_xmlInstallPath member)
	bool bOkay = TRUE;
	// on windows, C:\Program Files\<installation folder>, on Linux and Mac the path is
	// different -- see comments in OnInit() for details
	wxString pathToAIDefaultCSS = gpApp->m_xmlInstallPath;
	wxString cssFilename = _T("aiDefault.css"); // this name is hard coded, the original
				// file of this name is in adaptit\xml\ and a copy has been moved to the
				// install folder in Program Files, or equiv location on Linux or Mac
	pathToAIDefaultCSS += gpApp->PathSeparator;
	pathToAIDefaultCSS += cssFilename;
	bOkay = ::wxFileExists(pathToAIDefaultCSS);
	if (!bOkay)
	{
		wxString msg;
		msg = msg.Format(_("Cascading Stylesheet file, aiDefault.css, is missing.\n The aiDefault.css file was supposed to be at: %s\nThe export cannot be done unless the file exists at that location.\n(If you can find such a file, you can manually copy it to that location, and try the export again.)"),
			pathToAIDefaultCSS.c_str());
		gpApp->LogUserAction(msg);
		wxMessageBox(msg,_T("Error: CSS file missing"),wxICON_EXCLAMATION | wxOK);
		return;
	}
	bOkay = MakeAndSaveMyCSSFile(path, fname, pathToAIDefaultCSS);
	if (!bOkay)
	{
		return; // suitable error message has been shown already, and a message logged as well
	}
	// get the wxString which is the base text data -- as a USFM export
	// (LogUserAction() calls are done internally, one for each exportType)
	text = GetCleanExportedUSFMBaseText(exportType); // calls RebuildTargetText() etc

	// normalize, by changing any \vn & \vt custom marker combinations to \v, change any
	// non-breaking space markup to NBSP character so that our parser will parse the words
	// as separate tokens, and remove multiple spaces
	text = ApplyNormalizingFiltersToTheUSFMText(text);

	// Convert the USFM data to XHTML
	CBString myxml; myxml.Empty();
	myxml = pToXhtml->DoXhtmlExport(text);

	// write out the xhtml
	if (!WriteXHTML_To_File(exportPath, myxml, bShowMessageIfSucceeded))
	{
		return; // the user has seen a warning of the failure
	}
#if defined(__WXDEBUG__)
	XhtmlExport_DebuggingSupport();
#endif
}

// components for the DoExportAsXhtml() function

// return TRUE if a correctly named .css file was created and saved at the folder specified
// by path param, with name fname + ".css", based on the aiDefault.css file passed in by
// the absolute path pathToAIDefaultCSS param. Return FALSE if something went wrong -- in
// which case the caller should abort the XHTML export with a suitable warning message to
// the user. The caller has the responsibility of determining that the aiDefault.css file
// specified by the pathToAIDefaultCSS parameter does actually exist at that path.
// Called from:  DoExportAsXhtml()
bool MakeAndSaveMyCSSFile(wxString path, wxString fname, wxString pathToAIDefaultCSS)
{
	wxString extension = _T(".css");
	wxString defaultFilename = _T("aiDefault.css");
	wxString pathForCopy = path + gpApp->PathSeparator;
	pathForCopy += defaultFilename;
	// make a copy of the aiDefault.css in the _XHTM_OUTPUTS folder, or wherever
	// m_lastXhtmlOutputPath is pointing at (if user allowed that location to remain in
	// effect) -- TRUE param means "overwrite if one exists with same name"
	bool bOkay = wxCopyFile(pathToAIDefaultCSS, pathForCopy, TRUE);
	if (!bOkay)
	{
		// we don't expect a failure here, so a message for the developer is all that is
		// needed, not localizable
		wxString msg;
		msg = msg.Format(_T("wxCopyFilePath() failed in MakeAndSaveMyCSSFile for an xhtml export attempt.\n The aiDefault.css file was supposed to be at: %s\nThe path for the copy was: %s"),
			pathToAIDefaultCSS.c_str(), pathForCopy.c_str());
		gpApp->LogUserAction(msg);
		wxMessageBox(msg,_T("CSS file creation: copying aiDefault.css failed"),wxICON_EXCLAMATION | wxOK);
		return FALSE;
	}
	// now rename the copied aiDefault.css file to be what is needed for this export; the
	// TRUE param means "overwrite if the new name matches the name of some other file"
	wxString newPathAndName = path + gpApp->PathSeparator;
	newPathAndName += fname + extension;
	bOkay = wxRenameFile(pathForCopy, newPathAndName, TRUE);
	if (!bOkay)
	{
		// we don't expect a failure here, so a message for the developer is all that is
		// needed, not localizable
		wxString msg;
		msg = msg.Format(_T("wxRenameFile() failed in MakeAndSaveMyCSSFile for an xhtml export attempt.\n The aiDefault.css file was not renamed to: %s\nThe copied aiDefault.css file has been removed."),
			newPathAndName.c_str());
		gpApp->LogUserAction(msg);
		wxMessageBox(msg,_T("CSS file rename failed"),wxICON_EXCLAMATION | wxOK);
		wxRemoveFile(pathForCopy); // ignore returned boolean
		return FALSE;
	}
	return TRUE;
}

/// Return TRUE if the data is unstructured with USFM markers, giving a suitable warning,
/// and the caller should then return from the xhtml export attempt; else return FALSE
/// and the caller should continue processing
bool DeclineIfUnstructuredData()
{
	wxString msg;
	CAdapt_ItView* pView = gpApp->GetView();
	SPList* pList = gpApp->m_pSourcePhrases;
	wxASSERT(pList);
	bool bIsUnstructuredData = pView->IsUnstructuredData(pList);
	if (bIsUnstructuredData)
	{
		msg = msg.Format(_(
"Export in XHTML xml format is supported only for data originally marked up in standard format (meaning, the data includes backslash markers).\nThe current document lacks backslash markers."));
		wxMessageBox(msg,_("Unstructured Data"),wxICON_EXCLAMATION | wxOK);
		return TRUE;
	}
	return FALSE;
}

/// Return TRUE if the first CSourcePhrase does not have a bookID, or has an invalid bookID
/// for scripture (such as OTX), giving a suitable warning, and the caller should then
/// return from the xhtml export attempt; else return FALSE and the caller should continue
/// processing. Return the bookCode, if defined, via the signature; else return an empty string
/// Note: as of 8Aug12, GetBookIDFromDoc() supports the full list of 123 Paratext bookID codes.
bool DeclineIfNoBookCode(wxString& bookCode)
{
	wxString msg;
	// check for a valid 3-letter bookCode, it must be present and be valid for an Xhtml export
	bookCode = gpApp->GetBookIDFromDoc(); // get from the first CSourcePhrase instance
	if (bookCode.IsEmpty())
	{
		// not a valid bookCode, or none is defined
		// and in all these cases, an Xhtml export is not possible
		msg = msg.Format(_(
"The book code either is invalid, or does not exist.\nAn xhtml export is not possible without it. (It should be at the start of the adaptation.)\nThe book code obtained was %s"),
		bookCode.c_str());
		wxMessageBox(msg,_("Invalid or Absent Book Code"), wxICON_EXCLAMATION | wxOK);
		return TRUE;
	}
	return FALSE;
}

/// Return TRUE if the appropriate iso639-1 or ios639-3 language code for the export type
/// passed in (whether, src, tgt, glosses, or free translation) has not been defined. 
/// Return FALSE if it exists, and caller will continue processing. Give the user an
/// explanatory warning if the code does not exist yet.
bool DeclineIfNoIso639LanguageCode(ExportType exportType, wxString& langCode)
{
	// check for a 2-letter (iso639-1) or 3-letter (iso639-3) language code. If it's an
	// empty string, disallow the export; tell the user where to define it if it is not
	// yet defined
	wxString msg;
	switch (exportType)
	{
	case sourceTextExport:
		langCode = gpApp->m_sourceLanguageCode;
		break;
	case glossesTextExport:
		langCode = gpApp->m_glossesLanguageCode;
		break;
	case freeTransTextExport:
		langCode = gpApp->m_freeTransLanguageCode;
		break;
	case targetTextExport:
	default:
		langCode = gpApp->m_targetLanguageCode;
		break;
	}
	if (langCode.IsEmpty())
	{
		msg = msg.Format(_(
"The language's 2-letter or 3-letter code code is not set, so the export cannot be done yet. \nYou can set the right code in the Backups & Misc page of Preferences...\nDo so now, and then try the xhtml export again."));
		wxMessageBox(msg,_("No Language Code Is Set"),wxICON_EXCLAMATION | wxOK);
		return TRUE;
	}
	return FALSE;
}

/// Rebuild the type of text (whether src, tgt, glosses or free translation) in the form
/// of USFM marked up scripture text, and apply certain cleanup functions to remove
/// extraneous spaces and less than optimal formatting, and remove custom markers peculiar
/// to Adapt It only - to get a good base USFM text to work with.
wxString GetCleanExportedUSFMBaseText(ExportType exportType)
{
	int nTextLength;
	wxString text;
	bool bRTFOutput = FALSE; // we are working with USFM marked up text

	// do the reconstruction from CSourcePhrase instances in the document...

    // RebuildTargetText, RebuildSourceText, etc, expose previously filtered material as it
    // was before input tokenization, and also exposes new information added and filtered
    // in the document, such as backtranslations, notes, and free translations.
	// Rebuild the AdaptIt USFM text (if there are collected backtranslations in the
	// document, they will be included - so we have to get rid of them later in the caller)
	switch(exportType)
	{
	case sourceTextExport:
		gpApp->LogUserAction(_T("Exporting XHTML from Source Text"));
		nTextLength = RebuildSourceText(text);
		break;
	case glossesTextExport:
		gpApp->LogUserAction(_T("Exporting XHTML from Glosses Text"));
		nTextLength = RebuildGlossesText(text);
		break;
	case freeTransTextExport:
		gpApp->LogUserAction(_T("Exporting XHTML from Free Translation Text"));
		nTextLength = RebuildFreeTransText(text);
		break;
	default:
	case targetTextExport:
		gpApp->LogUserAction(_T("Exporting XHTML from Target Text"));
		nTextLength = RebuildTargetText(text);
		break;
	}
	nTextLength = nTextLength; // whm 27Jun12 added to avoid "set but not used" compiler warning;
	// remove the following markers and their text content... \free, \note, \bt and
	// any \bt-initial custom markers, and \rem (Paratext note marker) from the string
	// which defines the markers not to be included in the export
	ExcludeCustomMarkersAndRemFromExport(); // defined in ExportFunctions.cpp
	text = ApplyOutputFilterToText(text, m_exportBareMarkers, m_exportFilterFlags, bRTFOutput);
	// format for text oriented output in next call, param 2 is from enum ExportType in Adapt_It.h
	FormatMarkerBufferForOutput(text, targetTextExport);
	return text;
}

/// normalize, by changing any \vn & \vt custom marker combinations to \v, change any
/// non-breaking space markup to NBSP character so that our parser will parse the words
/// as separate tokens, and remove multiple spaces; return the changed string
wxString ApplyNormalizingFiltersToTheUSFMText(wxString text)
{
	// If \vn <versenum> \vt <text...> markup pattern has been used, change all the \vn
	// (verse number) markers to \vt, and remove all \t (verse text) markers
	text = ChangeMkrs_vn_vt_To_v(text);

	// change all ~ (USFM non-breaking space markers) to NBSP characters (\u00A0 or ANSI 0xA0)
	text = ChangeTildeToNonBreakingSpace(text);
	
	// remove any multiple spaces
	text = RemoveMultipleSpaces(text);
	return text;
}
/// Open a wxFile for writing, but if it fails to open, abandon the export and warn user
/// and return; but when opened without error, write out the xhtml to file, and close the
/// wxFile, and clear to False the flag which tells the caller that the export is
/// completed.
/// Return TRUE if the writing out was successful, return FALSE if the file descriptor 
/// could not be opened for writing.
bool WriteXHTML_To_File(wxString exportPath, CBString& myxml, bool bShowMessageIfSucceeded)
{
	// save the resulting xml file in the location specified in the caller, and passed in
	// as the exportPath parameter
	wxFile f;
	if( !f.Open( exportPath, wxFile::write))
	{
		#ifdef __WXDEBUG__
		wxLogDebug(_T("Unable to open export target text file for XHTML export\n"));
		#endif
		wxString msg;
		// don't localize this, it's unlikely to ever be seen
		msg = msg.Format(_T("Unable to open the file for exporting the utf8 xhtml productions with path:\n%s"),
		exportPath.c_str());
		wxMessageBox(msg,_T(""),wxICON_EXCLAMATION | wxOK);
		gpApp->LogUserAction(msg);
		gpApp->m_bXhtmlExportInProgress = FALSE;
		return FALSE;
	}
	// output the final form of the string
	DoWrite(f, myxml); // DoWrite() is a global function defined in XML.h & .cpp
	f.Close();
	gpApp->m_bXhtmlExportInProgress = FALSE;

	// whm 7Jul11 Note:
	// For protected navigation situations AI determines the actual
	// filename that is used for the export, and the export itself is
	// automatically saved in the appropriate outputs folder. Since the
	// user has no opportunity to provide a file name nor navigate to
	// a random path, we should inform the user at this point of the 
	// successful completion of the export, and indicate the file name 
	// that was used and its outputs folder name and location.
	wxFileName fn(exportPath);
	wxString fileNameAndExtOnly = fn.GetFullName();
	wxString pathOnly = fn.GetPath();

	wxString msg = _("The exported file was named:\n\n%s\n\nIt was saved at the following path:\n\n%s");
	msg = msg.Format(msg,fileNameAndExtOnly.c_str(), pathOnly.c_str());
	if (bShowMessageIfSucceeded == true)
	{
		wxMessageBox(msg,_("XHTML export operation successful"),wxICON_INFORMATION | wxOK);
	}
	gpApp->LogUserAction(_T("XHTML export operation successful"));

	return TRUE;
}

#if defined(__WXDEBUG__)
void XhtmlExport_DebuggingSupport()
{
	// ***************************  NOTE!  ***************************************************
	// There are 3 #defines just below. If you want to have, for debugging purposes, an
	// indented pretty-formated SECOND file (same filename but with "_IndentedXHTML" appended
	// to the filename title) output to the same folder as the one where the xhtml export goes,
	// then uncomment out DO_INDENT and XHTML_PRETTY here; and also you MUST do the same at the
	// top of Xhtml.cpp. You'll then get a second file dialog which allows you to chose the
	// exported xhtml file, and the indenting and pretty formatting will be done. The pretty
	// formatting verticall lines up <span> tags, that's all. If you just choose DO_INDENT, 
	// you only get <div> tags lined up vertically. If you do not uncomment out those two, but
	// instead uncomment out just DO_CLASS_NAMES you'll still see the extra file dialog, you
	// choose the exported xhtml file as before, but the output is just a vertical list of all
	// the distinct class attribute's stylenames -- such as Section_Head, Line1, Line2, and so
	// forth. Do this if you want to get an inventory of such names for the xhtml just
	// exported. Likewise, do the same uncommenting out at the top of Xhtml.cpp to make this
	// work. Whether you comment them out again or not, these extra jobs are only done in the
	// debug build. So they won't do anything in a release version.
	//  **************************************************************************************
	//#define DO_CLASS_NAMES
	//#define DO_INDENT	// comment out when production xhtml output is wanted
	// do not have the next one turned on unless DO_INDENT is also turned on
	//#define XHTML_PRETTY  // comment out when unpretty but valid indenting of xhtml is wanted

	#if defined(__WXDEBUG__)
	#if defined(DO_INDENT) && defined (XHTML_PRETTY)
		gpApp->m_pXhtml->Indent_Etc_XHTML();
	#endif
	#if !defined(DO_INDENT) && !defined (XHTML_PRETTY) && defined(DO_CLASS_NAMES)
		gpApp->m_pXhtml->Indent_Etc_XHTML();
	#endif
	#endif
}
#endif
// end components for DoExportAsXhtml() function

/// Return exportFilename unchanged if bUseSuffix is FALSE, but if the latter is TRUE then
/// prepare a unique filename by appending date-time info and a counter as a suffix to the
/// passed in exportFilename and return the filename so ammended
wxString PrepareUniqueFilenameForExport(wxString exportFilename, bool bDoAlways, 
			UniqueFileIncrementMethod enumMethod, bool bUseSuffix)
{
	// (BEW comment, this could be generalized more... but this reflects Bill's earlier
	// code - and he always used the code below with bUseSuffix having the
	// value of gpApp->m_bUseSuffixExportDateTimeOnFilename, and enumMethod always with
	// the value of incrementViaDate_TimeStamp (= 1), never incrementViaNextAvailableNumber)
    // bDoAlways means always modify (Bill always gives it TRUE value), and 2 is the num 
    // of digits to use in the number -- he assumes <= 99 is enough I expect.
    // 
    // Note: if bUseSuffix were passed in as FALSE, exportFilename would be passed back unchanged
	wxString uniqueFilename = GetUniqueIncrementedFileName(exportFilename, 
								enumMethod, bDoAlways, 2, _T("_exported_")); 
	if (bUseSuffix)
	{
		// Use the unique filename
		return uniqueFilename;
	}
	return exportFilename;
}

// BEW modified 10Aug09, to support exporting of glosses or free translations as well
// whm 6Aug11 revised for support for protecting inputs/outputs folder navigation
// whm 9Dec11 revised for support of export filename prefix and/or suffix and adjusted
// behaviors related to the prefixes/suffixes
void DoExportAsType(enum ExportType exportType)
{
	CAdapt_ItView* pView = gpApp->GetView();
	wxString exportFilename;
	bool bBypassFileDialog_ProtectedNavigation = FALSE;
	int len = 0;
	CExportSaveAsDlg sadlg(gpApp->GetMainFrame());
	sadlg.exportType = exportType; // whm added 9Dec11
	sadlg.Centre();
	wxString s;
	bool bRTFOutput = FALSE;	// local var - assume SFM output for Source, Target
								// Glosses or Free Translation text
	
	wxString expTypePrefixStr; // a string prefix identifying the type of export
								// it is optionally added depending on value of the App's 
								// m_bUsePrefixExportTypeOnFilename member
	wxString expProjNamePrefixStr; // a string prefix identifying the ai project name
								// it is optionally added depending on value of the App's
								// m_bUsePrefixExportProjectNameOnFilename member

	// Set export type prefixes in case we need them, log the user action
	// and set dialog static title.
	switch (exportType)
	{
	case sourceTextExport:
		{
			// BEW 28July removed the "new_" part of the string, as it is misleading and
			// serves no useful purpose
			//expTypePrefixStr = _("new_source_text_");
			expTypePrefixStr = _("source_text_");
			gpApp->LogUserAction(_T("Initiated Export Source Text"));
			s = _("Export Source Text");
			sadlg.SetTitle(s);
			break;
		}
	case targetTextExport:
		{
			expTypePrefixStr = _("target_text_");
			gpApp->LogUserAction(_T("Initiated Export Target Text"));
			s = _("Export Translation (Target) Text");
			sadlg.SetTitle(s);
			break;
		}
	case glossesTextExport:
		{
			expTypePrefixStr = _("glosses_text_");
			gpApp->LogUserAction(_T("Initiated Export Glosses Text"));
			s = _("Export Glosses As Text");
			sadlg.SetTitle(s);
			break;
		}
	case freeTransTextExport:
		{
			expTypePrefixStr = _("freetrans_text_");
			gpApp->LogUserAction(_T("Initiated Export Free Trans Text"));
			s = _("Export Free Translation Text");
			sadlg.SetTitle(s);
			break;
		}
	}

	// substitute the actual project name string into the %s placeholder
	expProjNamePrefixStr = gpApp->m_sourceName + _T('-') + gpApp->m_targetName + _T('_');
	wxString projNameExp;
	projNameExp = sadlg.pCheckUsePrefixExportProjNameOnFilename->GetLabel();
	projNameExp = projNameExp.Format(projNameExp, expProjNamePrefixStr.c_str());
	sadlg.pCheckUsePrefixExportProjNameOnFilename->SetLabel(projNameExp);

	// substitute the actual export type string into the %s placeholder
	wxString typeExp;
	typeExp = sadlg.pCheckUsePrefixExportTypeOnFilename->GetLabel();
	typeExp = typeExp.Format(typeExp, expTypePrefixStr.c_str());
	sadlg.pCheckUsePrefixExportTypeOnFilename->SetLabel(typeExp);

	// Use the App's m_curOutputFilename as the base filename for exports
	wxString defaultDir;
	exportFilename = gpApp->m_curOutputFilename;
	
	// Show the dialog
	if (sadlg.ShowModal() != wxID_OK)
	{
		return; // user cancelled
	}

	// Any change in the checkbox values are now stored in the App's 
	// m_bUseSuffixExportDateTimeOnFilename and m_bUsePrefixExportTypeOnFilename 
	// members.

	// Make adjustments to the exportFilename based on the user's preference for 
	// exportFilename prefix and suffix.
	// 
	// whm Note 8Jul11: When collaboration with PT/BE is ON, and when doing sfm/rtf 
	// export operations, the exportFilename as obtained from m_curOutputFilename 
	// above will be of the form _Collab_45_ACT_CH02.txt. To distinguish these manually
	// produced exports within the _TARGET_OUTPUTS or _TARGET_RTF_OUTPUTS folder from 
	// those generated automatically by the collaboration process, we remove the
	// "_Collab_" prefix. We then add an exportType prefix if the user ticked
	// the checkbox for using export type prefixes in the ExportSaveAsDlg.
	wxString collabPrefix = _T("_Collab_"); // include the following _ here for removal
	int pos_Collab_;
	pos_Collab_ = exportFilename.Find(collabPrefix);
	if (pos_Collab_ != wxNOT_FOUND)
		exportFilename.Remove(pos_Collab_,collabPrefix.Length());
	if (gpApp->m_bUsePrefixExportTypeOnFilename)
		exportFilename = expTypePrefixStr + exportFilename;
	// whm 21Feb12 added at Kim's request. Put the src and tgt language names as prefix on exportFilename.
	if (gpApp->m_bUsePrefixExportProjectNameOnFilename)
		exportFilename = expProjNamePrefixStr + exportFilename; 

	len = exportFilename.Length();

	// Adjust the export file's extension, the wxFileDialg's filter, and
	// navigation protection settings according to the user's selection 
	// of SFM or RTF.
	wxString filter;
	wxString uniqueFilename;
	switch(sadlg.GetSaveAsType())
	{
	    case ExportSaveAsRTF:
			/////////////////////////////////////
			// Export to RTF
			/////////////////////////////////////
			// make a suitable default output filename for the export function
			exportFilename.Remove(len-3,3); // remove the extension
			exportFilename += _T("rtf"); // make it an *.rtf file type
            // Prepare a unique filename from the exportFilename. This unique filename and
            // path is used when the export is nav protected or when the user has ticked
            // the checkbox at the bottom of the ExportSaveAsDlg to indicate that a
            // date-time stamp is to be suffixed to the export filename, which ensures that
            // any existing exports are not overwritten.
			exportFilename = PrepareUniqueFilenameForExport(exportFilename, TRUE, 
				incrementViaDate_TimeStamp, gpApp->m_bUseSuffixExportDateTimeOnFilename);
			filter = _("Exported Adapt It RTF Documents (*.rtf)|*.rtf|All Files (*.*)|*.*||");
			bRTFOutput = TRUE;
			// determine the defaultDir path, and whether the use is to protected from
			// doing folder navigation
			switch (exportType)
			{
			case sourceTextExport:
				gpApp->LogUserAction(_T("Export Source RTF Text"));
				bBypassFileDialog_ProtectedNavigation = GetDefaultDirectory_ProtectedNav(
					gpApp->m_bProtectSourceRTFOutputsFolder, gpApp->m_sourceRTFOutputsFolderPath, 
					gpApp->m_lastSourceRTFOutputPath, defaultDir);
				break;
			case glossesTextExport:
				gpApp->LogUserAction(_T("Export Glosses RTF Text"));
				bBypassFileDialog_ProtectedNavigation = GetDefaultDirectory_ProtectedNav(
					gpApp->m_bProtectGlossRTFOutputsFolder, gpApp->m_glossRTFOutputsFolderPath, 
					gpApp->m_lastGlossesRTFOutputPath, defaultDir);
				break;
			case freeTransTextExport:
				gpApp->LogUserAction(_T("Export Free Trans RTF Text"));
				bBypassFileDialog_ProtectedNavigation = GetDefaultDirectory_ProtectedNav(
					gpApp->m_bProtectFreeTransRTFOutputsFolder, gpApp->m_freeTransRTFOutputsFolderPath, 
					gpApp->m_lastFreeTransRTFOutputPath, defaultDir);
				break;
			default:
			case targetTextExport:
				gpApp->LogUserAction(_T("Export Target RTF Text"));
				bBypassFileDialog_ProtectedNavigation = GetDefaultDirectory_ProtectedNav(
					gpApp->m_bProtectTargetRTFOutputsFolder, gpApp->m_targetRTFOutputsFolderPath, 
					gpApp->m_lastTargetRTFOutputPath, defaultDir);
				break;
			} // switch (exportType)
			break;
		case ExportSaveAsXHTML:
		case ExportSaveAsPathway:
			/////////////////////////////////////
			// Export to XHTML / Pathway
			/////////////////////////////////////
            
			// (Note: both XHTML and Pathway exports result in the same export;
			// the Pathway export causes Pathway's command line (pathwayb.exe) to get
			// called on the xhtml results after the export occurs below.)
            
			{
			//////////////////////////////////////////////////////////////////////////////
			// Export to XHTML (which type, src or tgt or glosses or free trans is handled
			// within, as is saving to disk, so when DoExportAsXTHML() returns, return
			// from this block also
			/////////////////////////////////////////////////////////////////////////////
			wxString aMsg;
			wxString strType;
			switch(exportType)
			{
			case sourceTextExport:
				strType = _T("sourceTextExport");
				break;
			case targetTextExport:
				strType = _T("targetTextExport");
				break;
			case glossesTextExport:
				strType = _T("glossesTextExport");
				break;
			case freeTransTextExport:
				strType = _T("freeTransTextExport");
				break;
			}
			aMsg = aMsg.Format(_T("Export XHTML Text of type: %s"), strType.c_str());
			gpApp->LogUserAction(aMsg);
			// make a suitable default output filename for the export function
			exportFilename.Remove(len-3,3); // remove the extension
            wxString exportCSS = exportFilename + _T("css");
			exportFilename += _T("xhtml"); // make it a *.txt file type
            // Prepare a unique filename from the exportFilename. This unique filename and
            // path is used when the export is nav protected or when the user has ticked
            // the checkbox at the bottom of the ExportSaveAsDlg to indicate that a
            // date-time stamp is to be suffixed to the export filename, which ensures that
            // any existing exports are not overwritten.
			exportFilename = PrepareUniqueFilenameForExport(exportFilename, TRUE, 
				incrementViaDate_TimeStamp, gpApp->m_bUseSuffixExportDateTimeOnFilename);
			// prepare for getting a file Save As dialog for whatever Text type is to be
			// used for the xhtml output
			filter = _("Exported XHTML Documents (*.xhtml)|*.xhtml||");
			// determine the defaultDir path, and whether the use is to protected from
			// doing folder navigation
			bBypassFileDialog_ProtectedNavigation = GetDefaultDirectory_ProtectedNav(
				gpApp->m_bProtectXhtmlOutputsFolder, gpApp->m_xhtmlOutputsFolderPath, 
				gpApp->m_lastXhtmlOutputPath, defaultDir);

            if (sadlg.GetSaveAsType() == ExportSaveAsPathway)
            {
                // produce the XHTML, storing it in the project's _XHTML_OUTPUTS folder, or
                // in whatever folder path was in m_lastXhtmlOutputPath
                bBypassFileDialog_ProtectedNavigation = true;
                DoExportAsXhtml(exportType, bBypassFileDialog_ProtectedNavigation, defaultDir,
                    exportFilename, filter, false);
                 // Call PathwayB.exe on the exported XHTML.
                 // The full command line should look something like this:
                 //    PathwayB.exe -d "D:\Project2" -if xhtml -f * -c "project.css" -i "Scripture" -n "SEN" -s
                 // (A description of the PathwayB parameters can be found by calling PathwayB.exe
                 // from the command prompt without any parameters.)
                 wxString aMsg = _T("Pathway export - call Pathway on XHTML");
                 gpApp->LogUserAction(aMsg);

				 // sanity checks -- make sure the .xhtml and .css files got exported correctly
				 wxFile fTmp;
				 if (!fTmp.Exists(defaultDir + gpApp->PathSeparator + exportFilename))
				 {
					wxString msg;
					msg = msg.Format(_("Missing intermediate XHTML file: %s.\nThe Pathway export cannot be completed without this file."),
						exportFilename.c_str());
					gpApp->LogUserAction(msg);
					wxMessageBox(msg,_T("Error: Intermediate XHTML file missing"),wxICON_EXCLAMATION | wxOK);
					return;
				 }
				 if (!fTmp.Exists(defaultDir + gpApp->PathSeparator + exportCSS))
				 {
					wxString msg;
					msg = msg.Format(_("Missing Stylesheet file: %s.\nThe Pathway export cannot be completed without this file."),
						exportCSS.c_str());
					gpApp->LogUserAction(msg);
					wxMessageBox(msg,_T("Error: CSS file missing"),wxICON_EXCLAMATION | wxOK);
					return;
				 }

                 wxArrayString textIOArray, errorsIOArray;
                 wxString commandLine;
                 // full path to PathwayB executable
                 wxString PWBatchFilename = gpApp->m_PathwayInstallDirPath + gpApp->PathSeparator + _T("PathwayB.exe");
                 commandLine = _T("\"") + PWBatchFilename + _T("\" -d \"") + defaultDir;
				 commandLine += _T("\" -if xhtml -f \"") + defaultDir + gpApp->PathSeparator + exportFilename;
				 commandLine += _T("\" -c \"") + defaultDir + gpApp->PathSeparator + exportCSS;
                 commandLine += _T("\" -i \"Scripture\" -n \"");
                 // if there is a language code specified, pass it along; if not, use a generic "MP1"
                 commandLine += ((gpApp->m_targetLanguageCode.IsEmpty()) ? _T("MP1") : gpApp->m_targetLanguageCode);
                 // show the dialog to let the user choose the format (TODO: do we want this, or just
                 // take the defaults set up by the admin?)
                 commandLine += _T("\" -s");
                 int code = wxExecute(commandLine, wxEXEC_SYNC);
				 if (code != 0)
				 {
					 // Pathway command line returned an error -- return
					 aMsg = aMsg.Format(_T("Error: Pathway export returned with an error code:\n%d"), code);
					 wxMessageBox(aMsg,_T("Error: Pathway Export"),wxICON_EXCLAMATION | wxOK);
					 aMsg = aMsg.Format(_T("Pathway export - Shell command '%s' terminated with error code %d."), commandLine.c_str(), code);
					 gpApp->LogUserAction(aMsg);
				 }
				 else 
				 {
					 // Pathway didn't complain. Tell the user.
					 aMsg = aMsg.Format(_T("Pathway export returned with no reported errors.\nOutput can be found in the following directory:\n%s"), defaultDir.c_str());
					 wxMessageBox(aMsg,sadlg.GetTitle(),wxICON_INFORMATION | wxOK);
				 }
                 return;
            }
            else // xhtml output
            {
                // produce the XHTML, storing it in a user-chosen folder, or if folder
                // navigation is not protect, in the project's _XHTML_OUTPUTS folder, or
                // in whatever folder path was in m_lastXhtmlOutputPath
                DoExportAsXhtml(exportType, bBypassFileDialog_ProtectedNavigation, defaultDir,
                    exportFilename, filter, true);
                return;
            }
			}
			break;
	    case ExportSaveAsTXT:
	    default:
			/////////////////////////////////////
			// Export to SFM / TXT
			/////////////////////////////////////
			// make a suitable default output filename for the export function
			exportFilename.Remove(len-3,3); // remove the extension
			exportFilename += _T("txt"); // make it a *.txt file type
            // Prepare a unique filename from the exportFilename. This unique filename and
            // path is used when the export is nav protected or when the user has ticked
            // the checkbox at the bottom of the ExportSaveAsDlg to indicate that a
            // date-time stamp is to be suffixed to the export filename, which ensures that
            // any existing exports are not overwritten.
			exportFilename = PrepareUniqueFilenameForExport(exportFilename, TRUE, 
				incrementViaDate_TimeStamp, gpApp->m_bUseSuffixExportDateTimeOnFilename);
			// prepare for getting a file Save As dialog for Source Text Output
			filter = _("All Files (*.*)|*.*|Exported Adapt It Documents (*.txt)|*.txt||");
						// I changed the above to allow *.txt and *.*, with the
						// *.* one first (shows all) so it comes up as default This has the
						// nice property that if the user types an extension in the
						// filename, .txt won't be appended to it.
			bRTFOutput = FALSE;
			// determine the defaultDir path, and whether the use is to protected from
			// doing folder navigation
			switch (exportType)
			{
			case sourceTextExport:
				gpApp->LogUserAction(_T("Export Source SFM Text"));
				bBypassFileDialog_ProtectedNavigation = GetDefaultDirectory_ProtectedNav(
					gpApp->m_bProtectSourceOutputsFolder, gpApp->m_sourceOutputsFolderPath, 
					gpApp->m_lastSourceOutputPath, defaultDir);
				break;
			case glossesTextExport:
				gpApp->LogUserAction(_T("Export Glosses SFM Text"));
				bBypassFileDialog_ProtectedNavigation = GetDefaultDirectory_ProtectedNav(
					gpApp->m_bProtectGlossOutputsFolder, gpApp->m_glossOutputsFolderPath, 
					gpApp->m_lastGlossesOutputPath, defaultDir);
				break;
			case freeTransTextExport:
				gpApp->LogUserAction(_T("Export Free Trans SFM Text"));
				bBypassFileDialog_ProtectedNavigation = GetDefaultDirectory_ProtectedNav(
					gpApp->m_bProtectFreeTransOutputsFolder, gpApp->m_freeTransOutputsFolderPath, 
					gpApp->m_lastFreeTransOutputPath, defaultDir);
				break;
			default:
			case targetTextExport:
				gpApp->LogUserAction(_T("Export Target SFM Text"));
				bBypassFileDialog_ProtectedNavigation = GetDefaultDirectory_ProtectedNav(
					gpApp->m_bProtectTargetOutputsFolder, gpApp->m_targetOutputsFolderPath, 
					gpApp->m_lastTargetOutputPath, defaultDir);
				break;
			} // switch (exportType)
			break;
	} // switch (sadlg.GetSaveAsType())

	wxString exportPath;
	
	// whm modified 7Jul11 to bypass the wxFileDialog when the export is protected from
	// navigation.
	if (!bBypassFileDialog_ProtectedNavigation)
	{
		wxFileDialog fileDlg(
			(wxWindow*)wxGetApp().GetMainFrame(), // MainFrame is parent window for file dialog
			_("Filename For Export"),
			defaultDir,	// empty string causes it to use the current working directory (set above)
			exportFilename,	// default filename
			filter,
			wxFD_SAVE | wxFD_OVERWRITE_PROMPT); // | wxHIDE_READONLY); 
				// wxHIDE_READONLY deprecated in 2.6 - the checkbox is never shown	fileDlg.Centre();
				// GDLC wxSAVE & wxOVERWRITE_PROMPT deprecated in 2.8

		if (fileDlg.ShowModal() != wxID_OK)
		{
			gpApp->LogUserAction(_T("Cancelled DoExportSfmText()"));
			return; // user cancelled file dialog so return to what user was doing previously
		}
		exportPath = fileDlg.GetPath();	
	}
	else
	{
		// Set the exportPath for the appropriate outputs folder
		switch (exportType)
		{
		case sourceTextExport:
			if (!bRTFOutput)
				exportPath = gpApp->m_sourceOutputsFolderPath + gpApp->PathSeparator + exportFilename;
			else
				exportPath = gpApp->m_sourceRTFOutputsFolderPath + gpApp->PathSeparator + exportFilename;
			break;
		case glossesTextExport:
			if (!bRTFOutput)
				exportPath = gpApp->m_glossOutputsFolderPath + gpApp->PathSeparator + exportFilename;
			else
				exportPath = gpApp->m_glossRTFOutputsFolderPath + gpApp->PathSeparator + exportFilename;
			break;
		case freeTransTextExport:
			if (!bRTFOutput)
				exportPath = gpApp->m_freeTransOutputsFolderPath + gpApp->PathSeparator + exportFilename;
			else
				exportPath = gpApp->m_freeTransRTFOutputsFolderPath + gpApp->PathSeparator + exportFilename;
			break;
		case targetTextExport:
			if (!bRTFOutput)
				exportPath = gpApp->m_targetOutputsFolderPath + gpApp->PathSeparator + exportFilename;
			else
				exportPath = gpApp->m_targetRTFOutputsFolderPath + gpApp->PathSeparator + exportFilename;
			break;
		}
	}
	
	wxLogNull logNo; // avoid spurious messages from the system

	// whm 7Jul11 note: We'll allow the saving of the m_last... Paths even when navigation
	// protection is in force - the effect will be that the fixed outputs folders would
	// continue to be used after protection is turned off - unless specified otherwise by
	// the user entering a different path subsequently using the wxFileDialog.
	wxString path, fname, ext;
	wxFileName::SplitPath(exportPath, &path, &fname, &ext);
	if (bRTFOutput)
		switch (exportType)
		{
		case sourceTextExport:
			gpApp->m_lastSourceRTFOutputPath = path;
			break;
		case glossesTextExport:
			gpApp->m_lastGlossesRTFOutputPath = path;
			break;
		case freeTransTextExport:
			gpApp->m_lastFreeTransRTFOutputPath = path;
			break;
		default:
		case targetTextExport:
			gpApp->m_lastTargetRTFOutputPath = path;
			break;
		}
	else
	{
		switch (exportType)
		{
		case sourceTextExport:
			gpApp->m_lastSourceOutputPath = path;
			break;
		case glossesTextExport:
			gpApp->m_lastGlossesOutputPath = path;
			break;
		case freeTransTextExport:
			gpApp->m_lastFreeTransOutputPath = path;
			break;
		default:
		case targetTextExport:
			gpApp->m_lastTargetOutputPath = path;
			break;
		}
	}

	// first determine whether or not the data was unstructured plain text
	SPList* pList = gpApp->m_pSourcePhrases;
	wxASSERT(pList);
	gbIsUnstructuredData = pView->IsUnstructuredData(pList);
	wxString s1 = gSFescapechar;
	wxString paraMkr = s1 + _T("p ");

	// get the CString which is the source text data, as ammended by any edits on it
	// done so far; or the current target text data, or the current glosses data, or the
	// current free translations data, as the case may be
	wxString source;	// a buffer built from pSrcPhrase->m_srcPhrase strings
	source.Empty();
	wxString target;	// a buffer built from pSrcPhrase->m_targetStr strings
	target.Empty();
	wxString glosses;	// a buffer built from pSrcPhrase->m_gloss strings
	glosses.Empty();
	wxString freeTrans;	// a buffer built from filtered free translation strings
						// stored in the m_markers member of pSrcPhrase instances
	freeTrans.Empty();
	int nTextLength;
	// do the reconstruction from CSourcePhrase instances in the document...

	// RebuildSourceText removes filter brackets from the source or target, exposing
	// previously filtered material as it was before input tokenization, and also exposes
	// new information added and filtered in the document, such as backtranslations, notes,
	// and free translations. So does RebuildTargetText. But RebuildGlossesText has no way
	// to assign free translations, notes or collected backtranslations to locations in the
	// output, so those are not in the glosses export. And for RebuildFreeTransText, notes
	// and collected back translations are ignored; but for both the last two functions,
	// other filtered information is harvested and sent to output in the export, unless
	// filter option settings specify otherwise.

	// Rebuild the text and apply the output filter to it.
	switch (exportType)
	{
	case sourceTextExport:
		nTextLength = RebuildSourceText(source);
		nTextLength = nTextLength; // avoid warning TODO: test for failures? (BEW
								   // 3Jan12, No, allow length to be zero)
		// Apply output filter to the source text
		source = ApplyOutputFilterToText(source, m_exportBareMarkers, m_exportFilterFlags, bRTFOutput);

		// format for text oriented output
		FormatMarkerBufferForOutput(source, sourceTextExport);
		
		source = RemoveMultipleSpaces(source);

		if (gbIsUnstructuredData)
			FormatUnstructuredTextBufferForOutput(source,bRTFOutput);

		// do the check for a document with only paragraph markers (not needed, I fixed
		// FormatUnstructuredTextBufferForOutput() instead)
		//if (IsDocWithParagraphMarkersOnly(gpApp->m_pSourcePhrases))
		//{
			// input data had no SFMs, AI will have inserted possibly many \p markers,
			// these have to now be removed
		//	source = RemoveParagraphMarkersOnly(source);
		//}

		if (bRTFOutput)
		{
			DoExportTextToRTF(sourceTextExport, exportPath, fname, source);
			//return; // whm modified 11Jul11. Return below after wxMessageBox
		}
		else
		{
			ChangeCustomMarkersToParatextPrivates(source); // change our custom markers to 
														   // \z... markers for Paratext
		}
		break;
	case glossesTextExport:
		nTextLength = RebuildGlossesText(glosses);
		// Apply output filter to the glosses text
		glosses = ApplyOutputFilterToText(glosses, m_exportBareMarkers, m_exportFilterFlags, bRTFOutput);

		// format for text oriented output
		FormatMarkerBufferForOutput(glosses, glossesTextExport);
		
		glosses = RemoveMultipleSpaces(glosses);

		if (gbIsUnstructuredData)
			FormatUnstructuredTextBufferForOutput(glosses,bRTFOutput);

		if (bRTFOutput)
		{
			DoExportTextToRTF(glossesTextExport, exportPath, fname, glosses);
			//return; // whm modified 11Jul11. Return below after wxMessageBox
		}
		else
		{
			ChangeCustomMarkersToParatextPrivates(glosses); // change our custom markers to 
														   // \z... markers for Paratext
		}
		break;
	case freeTransTextExport:
		nTextLength = RebuildFreeTransText(freeTrans);
		// Apply output filter to the freeTrans text
		freeTrans = ApplyOutputFilterToText(freeTrans, m_exportBareMarkers, m_exportFilterFlags, bRTFOutput);

		// format for text oriented output
		FormatMarkerBufferForOutput(freeTrans, freeTransTextExport);
		
		freeTrans = RemoveMultipleSpaces(freeTrans);

		if (gbIsUnstructuredData)
			FormatUnstructuredTextBufferForOutput(freeTrans,bRTFOutput);

		if (bRTFOutput)
		{
			DoExportTextToRTF(freeTransTextExport, exportPath, fname, freeTrans);
			//return; // whm modified 11Jul11. Return below after wxMessageBox
		}
		else
		{
			ChangeCustomMarkersToParatextPrivates(freeTrans); // change our custom markers 
														   // to \z... markers for Paratext
		}
		break;
	default:
	case targetTextExport:
		nTextLength = RebuildTargetText(target);
		// Apply output filter to the target text
		target =  ApplyOutputFilterToText(target, m_exportBareMarkers, m_exportFilterFlags, bRTFOutput);
		//target = ExportTargetText_For_Collab(gpApp->m_pSourcePhrases); <<- for testing it works right - it does

		// format for text oriented output
		FormatMarkerBufferForOutput(target, targetTextExport);
		
		target = RemoveMultipleSpaces(target);
		
		if (gbIsUnstructuredData)
			FormatUnstructuredTextBufferForOutput(target,bRTFOutput);

		// do the check for a document with only paragraph markers (not needed, I fixed
		// FormatUnstructuredTextBufferForOutput() instead)
		//if (IsDocWithParagraphMarkersOnly(gpApp->m_pSourcePhrases))
		//{
			// input data had no SFMs, AI will have inserted possibly many \p markers,
			// these have to now be removed
		//	target = RemoveParagraphMarkersOnly(target);
		//}

		if (bRTFOutput)
		{
			DoExportTextToRTF(targetTextExport, exportPath, fname, target);	// When targetTextExport function processes
																	// Target, otherwise Source
			//return; // whm modified 11Jul11. Return below after wxMessageBox
		}
		else
		{
			ChangeCustomMarkersToParatextPrivates(target); // change our custom markers to 
														   // \z... markers for Paratext
		}
		break;
	}
	if (bRTFOutput)
	{
		// whm 7Jul11 Note:
		// For protected navigation situations AI determines the actual
		// filename that is used for the export, and the export itself is
		// automatically saved in the appropriate outputs folder. Since the
		// user has no opportunity to provide a file name nor navigate to
		// a random path, we should inform the user at this point of the 
		// successful completion of the export, and indicate the file name 
		// that was used and its outputs folder name and location.
		wxFileName fn(exportFilename);
		wxString fileNameAndExtOnly = fn.GetFullName();

		wxString msg;
		msg = msg.Format(_("The exported file was named:\n\n%s\n\nIt was saved at the following path:\n\n%s"),fileNameAndExtOnly.c_str(),exportPath.c_str());
		wxMessageBox(msg,_("Export operation successful"),wxICON_INFORMATION | wxOK);

		return; // this ends RTF output
	}


	///////////////////// DoExportSfmText() ends here if it is RTF output ////////////////////////

	wxFile f;

	if( !f.Open( exportPath, wxFile::write))
	{
		wxString msg;
		switch (exportType)
		{
		case sourceTextExport:
			#ifdef __WXDEBUG__
			wxLogDebug(_T("Unable to open export source text file\n"));
			#endif
			msg = msg.Format(_("Unable to open the file for exporting the source text with path:\n%s"),exportPath.c_str());
			wxMessageBox(msg,_T(""),wxICON_EXCLAMATION | wxOK);
			break;
		case glossesTextExport:
			#ifdef __WXDEBUG__
			wxLogDebug(_T("Unable to open export glosses text file\n"));
			#endif
			msg = msg.Format(_("Unable to open the file for exporting the glosses text with path:\n%s"),exportPath.c_str());
			wxMessageBox(msg,_T(""),wxICON_EXCLAMATION | wxOK);

			break;
		case freeTransTextExport:
			#ifdef __WXDEBUG__
			wxLogDebug(_T("Unable to open export free translation text file\n"));
			#endif
			msg = msg.Format(_("Unable to open the file for exporting the free translation text with path:\n%s"),exportPath.c_str());
			wxMessageBox(msg,_T(""),wxICON_EXCLAMATION | wxOK);
			break;
		default:
		case targetTextExport:
			#ifdef __WXDEBUG__
			wxLogDebug(_T("Unable to open export target text file\n"));
			#endif
			msg = msg.Format(_("Unable to open the file for exporting the target text with path:\n%s"),exportPath.c_str());
			wxMessageBox(msg,_T(""),wxICON_EXCLAMATION | wxOK);
			break;
		}
		gpApp->LogUserAction(msg);
		return;
	}

	// output the final form of the string
	#ifndef _UNICODE // ANSI
	switch (exportType)
	{
	case sourceTextExport:
		f.Write(source);
		break;
	case glossesTextExport:
		f.Write(glosses);
		break;
	case freeTransTextExport:
		f.Write(freeTrans);
		break;
	default:
	case targetTextExport:
		f.Write(target);
		break;
	}

	#else // Unicode
	switch (exportType)
	{
	case sourceTextExport:
		{
		// Bruce added 8Dec06 two following lines
		wxFontEncoding saveSrcEncoding = gpApp->m_srcEncoding; // I don't want 
			// to mess with checking whether the enforced conversion is safe 
			// to leave in place or not, so I'll restore afterwards
		gpApp->m_srcEncoding = wxFONTENCODING_UTF8; // BEW added 8Dec06 to 
			// force conversion to UTF-8 always when exporting, same as is now 
			// done for SFM export of the target text
		// whm modification 29Nov07 Removed the FALSE parameter from ConvertAndWrite
		// so that source text exports don't get written with a null char embedded as
		// the last character of the file. The spurious null character was causing
		// programs like WinMerge to consider "new source text.txt" files as binary
		// files rather than plain text files. This (and else block below) are the
		// only places where the FALSE parameter was used in the MFC code.
		gpApp->ConvertAndWrite(gpApp->m_srcEncoding,&f,source); // ,FALSE);
		gpApp->m_srcEncoding = saveSrcEncoding; // Bruce added 8Dec06
		}
		break;
	case glossesTextExport:
		wxFontEncoding saveGlossEncoding;
		if (gbGlossingUsesNavFont)
		{
			saveGlossEncoding = gpApp->m_navtextFontEncoding;
			gpApp->m_navtextFontEncoding = wxFONTENCODING_UTF8;
			gpApp->ConvertAndWrite(gpApp->m_navtextFontEncoding,&f,glosses);
			gpApp->m_navtextFontEncoding = saveGlossEncoding; // restore encoding
		}
		else // it uses target text's encoding
		{
			saveGlossEncoding = gpApp->m_tgtEncoding;
			gpApp->m_tgtEncoding = wxFONTENCODING_UTF8;
			gpApp->ConvertAndWrite(gpApp->m_tgtEncoding,&f,glosses);
			gpApp->m_tgtEncoding = saveGlossEncoding; // restore encoding
		}
		break;
	case freeTransTextExport:
		{
		// for free translations we'll temporarily redefine the navTextFontEncoding
		// to be UTF-8, it doesn't really matter what one we use though (in the view,
		// the encoding used is that for the target text font) so long as the 
		// conversion is done to UTF-8
		wxFontEncoding saveFreeTransEncoding = gpApp->m_navtextFontEncoding;
		gpApp->m_navtextFontEncoding = wxFONTENCODING_UTF8;
		gpApp->ConvertAndWrite(gpApp->m_navtextFontEncoding,&f,freeTrans);
		gpApp->m_navtextFontEncoding = saveFreeTransEncoding; // restore encoding
		}
		break;
	default:
	case targetTextExport:
		// assume the encoding is utf-safe & send it out as UTF-8
		wxFontEncoding saveTgtEncoding;
		saveTgtEncoding = gpApp->m_tgtEncoding;
		gpApp->m_tgtEncoding = wxFONTENCODING_UTF8;
		gpApp->ConvertAndWrite(gpApp->m_tgtEncoding,&f,target);
		gpApp->m_tgtEncoding = saveTgtEncoding; // restore encoding
		break;
	}
	#endif // for _UNICODE
	
	// whm 7Jul11 Note:
	// For protected navigation situations AI determines the actual
	// filename that is used for the export, and the export itself is
	// automatically saved in the appropriate outputs folder. Since the
	// user has no opportunity to provide a file name nor navigate to
	// a random path, we should inform the user at this point of the 
	// successful completion of the export, and indicate the file name 
	// that was used and its outputs folder name and location.
	wxFileName fn(exportFilename);
	wxString fileNameAndExtOnly = fn.GetFullName();

	wxString msg = _("The exported file was named:\n\n%s\n\nIt was saved at the following path:\n\n%s");
	msg = msg.Format(msg,fileNameAndExtOnly.c_str(),exportPath.c_str());
	wxMessageBox(msg,_("Export operation successful"),wxICON_INFORMATION | wxOK);
	gpApp->LogUserAction(_T("Export operation successful"));

	f.Close();
}

/// Compute a default directory for displaying it's contents in a File Save dialog which
/// may be opened. Return a boolean which is TRUE if the fixed folder for the particular
/// path information passed in is to be protected from navigation; return FALSE is not to
/// be protected from navigation - and when FALSE is returned, the function tries to use
/// the last path used for that type of information if such a folder exists, but if not it
/// defaults to the fixed folder. In either of the last two scenarios, the File Save will
/// show later after the function returns, and the user would be free to navigate using it
/// to anywhere he likes.
bool GetDefaultDirectory_ProtectedNav(bool bProtectFromNavigation, wxString fixedOutputPath, 
									  wxString lastOutputPath, wxString& defaultDir)
{
	bool bBypassFileDialog_ProtectedNavigation = FALSE;
    // The specific special folders involved depend on whether navigation protection is ON
    // or OFF, and whether the lastOutputPath member points to a valid path
	if (bProtectFromNavigation)
	{
        // Navigation protection in effect - limit source text exports to be saved in the
        // fixed output folder with a name like _XXXXX_OUTPUTS or _XXXXX_OUTPUTS_INPUTS;
        // and it is always a child folder of the folder that m_curProjectPath points to.
        // XXXXX represents SOURCE, or TARGET, or XHTML, or GLOSSES, etc.
		bBypassFileDialog_ProtectedNavigation = TRUE;
		defaultDir = fixedOutputPath;
	}
	else if (lastOutputPath.IsEmpty() ||
			(!lastOutputPath.IsEmpty() && !::wxDirExists(lastOutputPath)))
	{
        // Navigation protection is OFF so we set the flag to allow the wxFileDialog to
        // appear. But the lastOutputPath is either empty or, if not empty, it points to an
        // invalid path, so we initialize the defaultDir to point to the special fixed
        // protected folder, even though Navigation protection is not ON. In this case, the
        // user could point the export path elsewhere using the wxFileDialog that will
        // appear.
		bBypassFileDialog_ProtectedNavigation = FALSE;
		defaultDir = fixedOutputPath;
	}
	else
	{
        // Navigation protection is OFF and we have a valid path in lastOutputPath, so we
        // initialize the defaultDir to point to the lastOutputPath for the location of the
        // export. The user could still point the export path elsewhere in the wxFileDialog
        // that will appear.
		bBypassFileDialog_ProtectedNavigation = FALSE;
		defaultDir = lastOutputPath;
	}
	return bBypassFileDialog_ProtectedNavigation;
}


// changes all ~ (the USFM non-breaking space marker) into \u00A0 (Unicode version) or
// into \A0 (Regular version)
wxString ChangeTildeToNonBreakingSpace(wxString text)
{
	wxChar tilde = _T('~');
#if defined(_UNICODE)
	wxChar nbsp = (wxChar)0x00A0;
#else
	wxChar nbsp = (wxChar)0xA0;
#endif
	size_t count = text.Len();
	size_t index;
	for (index = 0; index < count; index++)
	{
		wxChar aChar = text[index];
		if (aChar == tilde)
		{
			text.SetChar(index,nbsp); // could use: text[index] = nbsp; but I don't trust it
		}
	}
	return text;
}

wxString RemoveCollectedBacktranslations(wxString& str)
{
	wxString out; out.Empty();
	wxString btMkr = _T("\\bt "); // 4 characters to search for
	wxChar bslash = _T('\\');
	int offset = wxNOT_FOUND;
	offset = str.Find(btMkr);
	if (offset == wxNOT_FOUND)
	{
		return str; // there are not any \bt markers in the data
	}
	// continue, there is at least one \bt marker in the data
	wxString Left;
	while (offset != wxNOT_FOUND)
	{
		Left = str.Left(offset);
		str = str.Mid(offset);
		// skip over the marker
		str = str.Mid(4);
		int offset2 = str.Find(bslash);
		if (offset2 == wxNOT_FOUND)
		{
			// no alternative but to assume the rest is all a backtranslation that was
			// collected and therefore we are done
			out += Left;
			return out;
		}
		else
		{
			// remove everything up to this marker, irrespective of whatever it is,
			// because what precedes it is the collected backtranslation text
			str = str.Mid(offset2);
		}
		out += Left;

		// test for the next \bt marker
		offset = str.Find(btMkr);
	}
	// handle the last bit of text
	out += str;
	return out;
}

// Looks in the global wxArrayString m_exportBareMarkers (the doc function,
// GetMarkerInventoryFromCurrentDoc() should have been called just previously to ensure
// the array is populated with the markers actually current in the document), passing in
// the bare marker (ie. backslash removed) or a whole marker - if the latter, it will
// strip off the backslash before using it, to search in the array for the index where the
// marker is located, and then returns that index to the caller.
// The function is used in ExcludeCustomMarkersFromExport() - see below, and the latter
// does the call of GetMarkerInfentoryFromCurrentDoc() - the latter function sets all the
// flags in the global wxArrayInt, m_exportFilterFlags to 0 (FALSE) at its start.
// Returns wxNOT_FOUND if the passed in bareMkr is not in the inventory
int	FindMkrInMarkerInventory(wxString bareMkr)
{
	wxASSERT(!bareMkr.IsEmpty() && bareMkr.Len() > 0);
	if (bareMkr.GetChar(0) == _T('\\'))
		bareMkr = bareMkr.Mid(1);
	int index = m_exportBareMarkers.Index(bareMkr);
	return index;
}

void ExcludeCustomMarkersFromExport()
{
	// populate global m_exportBareMarkers wxArrayString, and make the global wxArrayInt
	// m_exportFilterFlags which is of same size have all items 0 (ie. FALSE) except for
	// those which default to 'filtered' status, and then below we add any extra filtered
	// status indicators for the custom markers we want filtered out
	gpApp->GetDocument()->GetMarkerInventoryFromCurrentDoc_For_Collab();
	// define our custom markers, (doesn't matter if we include their backslash or not,
	// the calls below would remove initial backslash if present) and get their indices
	// within m_exportBareMarkers; and then set the item at same index in
	// m_exportFilterFlags to 1 (ie. TRUE), and then a call of ApplyOutputFilterToText()
	// after a USFM rebuild by a function like RebuildTargetText() will filter out the
	// markers we here specify, wherever they occur, and also remove their text content as
	// well.
	wxString freeTrans = _T("free");
	wxString note = _T("note");
	wxString bt = _T("bt"); // this will also filter out any longer markers beginning with bt
	int index = FindMkrInMarkerInventory(freeTrans);
	if (index != wxNOT_FOUND)
	{
		m_exportFilterFlags[index] = 1;
	}
	index = FindMkrInMarkerInventory(note);
	if (index != wxNOT_FOUND)
	{
		m_exportFilterFlags[index] = 1;
	}
	index = FindMkrInMarkerInventory(bt);
	if (index != wxNOT_FOUND)
	{
		m_exportFilterFlags[index] = 1;
	}
}

// the following is like ExcludeCustomMarkersFromExport(), but adds exclusion of the USFM
// \rem marker and its content from the export.
// Usage: used in XHTML export support -- see DoExportAsType()
// Created: BEW 19May12
void ExcludeCustomMarkersAndRemFromExport()
{
	ExcludeCustomMarkersFromExport(); // exclude \note, \free, and also \bt & friends
	wxString rem = _T("rem"); // bareMkr for \rem
	int index = FindMkrInMarkerInventory(rem);
	if (index != wxNOT_FOUND)
	{
		m_exportFilterFlags[index] = 1;
	}
}

// The default option (2nd param is TRUE) for the following call is almost the functional
// equivalent to a doc version 4 test for a non-empty m_markers member. The difference is
// that the default ignores the m_endMarkers member, but for doc version 4, endmarkers will
// be on the **next** CSourcePhrase's m_marker member, and so the version 4 m_markers test
// would yield TRUE for that following CSourcePhrase (which typically is an unwanted
// result) whereas this function with the default option set, for version 5, will ignore
// the CSourcePhrase which only has endmarker(s) in its m_endMarkers member - which is
// typically what we want our code to do. So to also return TRUE for CSourcePhrase
// instances where the only markers in it are endMarkers, set the 2nd param explicitly to
// FALSE.
bool AreMarkersOrFilteredInfoStoredHere(CSourcePhrase* pSrcPhrase, bool bIgnoreEndMarkers)
{
	// second param is default TRUE
	bool bHasFiltered = HasFilteredInfo(pSrcPhrase);
	bool bHasMarkers = !pSrcPhrase->m_markers.IsEmpty();
	bool bHasEndMarkers = !pSrcPhrase->GetEndMarkers().IsEmpty();
	if (bIgnoreEndMarkers)
	{
		return bHasFiltered || bHasMarkers;
	}
	return bHasFiltered || bHasMarkers || bHasEndMarkers;
}

// test for presence of footnotes in the document; the default is to return TRUE if a
// footnote marker is detected (unfiltered) in m_markers, or (filtered) in m_filteredInfo;
// setting the second param to TRUE means that the test will return TRUE only if a footnote
// marker is found in m_markers, no matter whether or not there one in m_filteredInfo
bool IsFootnoteInDoc(CSourcePhrase* pSrcPhrase, bool bIgnoreFilteredFootnotes)
{
	if (bIgnoreFilteredFootnotes)
	{
		// unfiltered footnotes will result in m_markers storing \f marker
		if (pSrcPhrase->m_markers.Find(_T("\\f ")) != -1)
			return TRUE;
		else
			return FALSE;
	}
	// filtered footnotes will result in m_filteredInfo storing \f marker, so if we don't
	// care whether it is filtered or not, but just want to know if any are in the
	// document, then we must test both members
	if ((pSrcPhrase->m_markers.Find(_T("\\f ")) != -1) || 
		(pSrcPhrase->GetFilteredInfo().Find(_T("\\f ")) != -1))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// test for presence of footnotes in the document; the default is to return TRUE if a
// footnote marker is detected (unfiltered) in m_markers, or (filtered) in m_filteredInfo;
// setting the second param to TRUE means that the test will return TRUE only if a footnote
// marker is found in m_markers, no matter whether or not there one in m_filteredInfo
bool IsEndnoteInDoc(CSourcePhrase* pSrcPhrase, bool bIgnoreFilteredEndnotes)
{
	// Only when "\fe " exists apart from the PngOnly set does it signal existence of endnotes
	if (gpApp->gCurrentSfmSet != PngOnly)
	{
		// unfiltered endnotes will result in m_markers storing an \fe marker
		if (bIgnoreFilteredEndnotes)
		{
			if (pSrcPhrase->m_markers.Find(_T("\\fe ")) != -1)
				return TRUE;
			else
				return FALSE;
		}
		// filtered endnotes will result in m_filteredInfo storing an \fe marker, so if we
		// don't care whether it is filtered or not, but just want to know if any are in
		// the document, then we must test both members
		if ((pSrcPhrase->m_markers.Find(_T("\\fe ")) != -1) || 
			(pSrcPhrase->GetFilteredInfo().Find(_T("\\fe ")) != -1))
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	return FALSE;
}

bool IsFreeTransInDoc(CSourcePhrase* pSrcPhrase)
{
	return !pSrcPhrase->GetFreeTrans().IsEmpty();
}

bool IsBackTransInDoc(CSourcePhrase* pSrcPhrase)
{
	return !pSrcPhrase->GetCollectedBackTrans().IsEmpty();
}

bool IsNoteInDoc(CSourcePhrase* pSrcPhrase)
{
	return !pSrcPhrase->GetNote().IsEmpty();
}

// whm added 15Jul03 and Revised 1Aug03
// whm revised November 2007 to improve reliability with Word 2003
// BEW 10Apr10, updated for support of doc version 5 (changes were needed)
// whm revised July 2011 to improve formatting for OpenOffice/LibreOffice, 
// while maintaining compatible with MS Word.
// whm revised 9Dec11 to better handle export filename prefix/suffix consistent
// with other exports.
void DoExportInterlinearRTF()
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	CAdapt_ItView* pView = gpApp->GetView();
	wxString exportFilename = gpApp->m_curOutputFilename;

	gpApp->LogUserAction(_T("Initiated DoExportInterlinearRTF()"));
	bool bBypassFileDialog_ProtectedNavigation = FALSE;
	
	// establish pointer to the list of Source Phrases,
	// so we can scan them and access them
	SPList* pList = gpApp->m_pSourcePhrases;
	wxASSERT(pList != NULL);


	// make the working directory the "<Project Name>" one, unless there is a path in
	// app's m_lastTargetOutputPath member
	//wxString saveWorkDir;
	//saveWorkDir = ::wxGetCwd();
	wxString defaultDir;
	//bool bOK;
	
	// whm added 7Jul11 support for protecting inputs/outputs folder navigation
	if (gpApp->m_bProtectInterlinearRTFOutputsFolder)
	{
		// Navigation protection in effect - limit source text exports to
		// be saved in the _INTERLINEAR_RTF_OUTPUTS folder which is always a child folder
		// of the folder that m_curProjectPath points to.
		// whm modified 2Aug11. We don't need to call ::wxSetWorkingDirectory() when
		// using protected navigation, because the path for the export is an absolute
		// path directly pointing to the _INTERLINEAR_RTF_OUTPUTS folder.
		bBypassFileDialog_ProtectedNavigation = TRUE;
		defaultDir = gpApp->m_interlinearRTFOutputsFolderPath;
	}
	else if (gpApp->m_lastInterlinearRTFOutputPath.IsEmpty()
		|| (!gpApp->m_lastInterlinearRTFOutputPath.IsEmpty() && !::wxDirExists(gpApp->m_lastInterlinearRTFOutputPath)))
	{
		// Navigation protection is OFF so we set the flag to allow the wxFileDialog 
		// to appear. But the m_lastInterlinearRTFOutputPath is either empty or, if 
		// not empty, it points to an invalid path, so we initialize the defaultDir 
		// to point to  the special protected folder, even though Navigation 
		//  is not ON. In this case, the user could point the export path elsewhere 
		//  using the wxFileDialog that will appear.
		bBypassFileDialog_ProtectedNavigation = FALSE;
		defaultDir = gpApp->m_interlinearRTFOutputsFolderPath;
	}
	else
	{
		// Navigation protection is OFF and we have a valid path in m_lastInterlinearRTFOutputPath,
		// so we initialize the defaultDir to point to the m_lastInterlinearRTFOutputPath for the 
		// location of the export. The user could still point the export path elsewhere 
		// in the wxFileDialog that will appear.
		bBypassFileDialog_ProtectedNavigation = FALSE;
		defaultDir = gpApp->m_lastInterlinearRTFOutputPath;
	}
	// determine whether or not the data was unstructured plain text
	gbIsUnstructuredData = pView->IsUnstructuredData(pList);

	// set defaults
	bool bUsePortrait = gpApp->m_bIsPortraitOrientation;	// Default to the current orientation, from the View
	bool bInclSrcLangRow = TRUE;					// When true the the Source language check box is checked
													// as default and Src lang row is included in output tables.
	bool bInclTgtLangRow = TRUE;					// When true the the Target language check box is checked
													// as default and Tgt lang row is included in output tables.
	bool bInclGlsLangRow = gbGlossingVisible;		// When gbGlossingVisible is true the the Gloss language
													// check box is checked and Gls lang row is included in
													// output tables.
	bool bInclNavLangRow = TRUE;					// When true the the Navigation lang check box is checked
													// as default and Nav lang row is included in output tables.
	bool bNewTableForNewLineMarker = FALSE;			// When true a new table starts for markers assoc with new 
													// lines.
	bool bCenterTableForCenteredMarker = FALSE;		// When true the table is centered for markers assoc with
													// centered text
	// next two added for version 3
	bool bInclFreeTransRow = FALSE;					// When true the Free translation row is included in output
													// tables
	bool bInclBackTransRow = FALSE;					// When true the Back translation row is included in output
													// tables
	bool bOutputAll = TRUE;							// When true the All radio button is selected and the Chapter
													// Verse Range edits are disabled
	bool bOutputPrelim = FALSE;						// When true the Preliminary Material Only button is selected
													// and the Verse Range edits are disabled
	bool bOutputFinal = FALSE;						// When true the Final Material Only button is selected
													// and the Verse Range edits are disabled
	bool bOutputCVRange = FALSE;					// When true the Chapter/Verse Range button is selected
													// and the Verse Range edits are enabled
	bool bMarkerStartsNewLine = FALSE;				// False unless current marker normally starts new line
	bool bNextTableIsCentered = FALSE;				// True when marker is encountered that defines a centered paragraph
	bool bTableIsCentered = FALSE;					// True while current table should be output as centered

	// initialize ExportInterlinear dialog
	CExportInterlinearDlg exdlg(gpApp->GetMainFrame());
	exdlg.Centre();

	int nChFirst = 0;								// start chapter of range
	int nChLast = 0;								// ending chapter of range
	int nVsFirst = 0;								// start verse of range
	int nVsLast = 0;								// ending verse of range
	wxString ChVsFirst = _T("");
	wxString ChVsLast = _T("");
	// whm 26Oct07 added check for footnotes and endnotes to process the \fetN control word properly 
	// (see composition of Doctags below).
	bool bDocHasFootnotes = FALSE;	// assume no footnotes unless found in while loop below
	bool bDocHasEndnotes = FALSE;	// assume no endnotes unless found in while loop below
	bool bDocHasFreeTrans = FALSE;	// assume no free translation unless found in while loop below
	bool bDocHasBackTrans = FALSE;	// assume no back translation unless found in while loop below
	bool bDocHasAINotes = FALSE;	// assume no AI notes unless found in while loop below
	//bool bBeforeInitialChVs = TRUE;	// flag to tell we are in text before 1st ch/vs
	wxString Mkr;

	CSourcePhrase* pSrcPhrase = NULL;
	//SPList::Node* savePos = NULL; //POSITION savePos = NULL;

	// Scan the SrcPhrase structure to get certain information that we need upfront, before 
	// putting up the export interlinear dialog, or before constructing the RTF header items:
	// 1. The Ch/Vs range parameters for exdlg (needed before the dialog is presented)
	// 2. Whether the document contains footnotes and/or endnotes (needed before RTF header is formed)
	// 3. Whether the document contains free translations, back translations and/or AI notes (also needed
	//    before the RTF header is formed)
	// This should work whether we have unstructured data or not
	SPList::Node* pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	bool bFirst = TRUE;
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase);
		//if (!pSrcPhrase->m_markers.IsEmpty())
		// In the following test, the 2nd param, bool bIgnoreEndMarkers, is default TRUE
		if (AreMarkersOrFilteredInfoStoredHere(pSrcPhrase))
		{
			// whm added 26Oct07
			// Check for existence of footnotes and/or endnotes
			// This is done mainly to determine what value for N we will assign to the 
			// problematic \fetN control word when the DocTags part of the RTF header
			// string is composed farther below.
			// Check for footnotes and/or endnotes within m_markers. 
			// Note that filtered information is always exported, so we consider that
			// any "\f " or "\fe " (not PngOnly) within filtered information should 
			// initially set the appropriate boolean flag to TRUE. Below we check
			// against the export output filter to change the final state of these
			// flags if they won't end up being exported in the RTF file.
			//
			// Now, look for existence of "\f " and/or "\fe " markers in the doc.
			// The existence of "\f " always indicates a beginning footnote marker in 
			// any sfm set
			if (IsFootnoteInDoc(pSrcPhrase))
				bDocHasFootnotes = TRUE; // TRUE if marker was in m_markers or m_filteredInfo 
			// Only when "\fe " exists apart from the PngOnly set does it signal existence of endnotes
			if (IsEndnoteInDoc(pSrcPhrase))
				bDocHasEndnotes = TRUE; // TRUE if markers was in m_markers or m_filteredInfo
										// and the PngOnly SFM set is not currently in effect
			// Now, look for existence of free translations in the doc (these are always
			// regared as "filtered")
			if (IsFreeTransInDoc(pSrcPhrase))
				bDocHasFreeTrans = TRUE;
			// Now, look for existence of collected back translations in the doc (these are always
			// regarded as "filtered") (in doc version 5 we ignore any \bt-derived markers
			// such as \btv \bth \bts and so forth -- SAG group uses such, but we give
			// them only basic support (we automatically filter them, and return them in
			// exports of SFM, but otherwise ignore them)
			if (IsBackTransInDoc(pSrcPhrase))
				bDocHasBackTrans = TRUE;
			// Now, look for existence of a note in the doc (these are always regarded as
			// "filtered") 
			if (IsNoteInDoc(pSrcPhrase))
				bDocHasAINotes = TRUE;
		}
		if (pSrcPhrase->m_chapterVerse != _T("") && (pSrcPhrase->m_bChapter || pSrcPhrase->m_bVerse))
		{
			if (bFirst)
			{
				ChVsFirst = pSrcPhrase->m_chapterVerse;		// get the first m_ChapterVerse
				bFirst = FALSE;
				//bBeforeInitialChVs = FALSE;					// turn this off now that we've encountered 1st ch:vs
			}
			ChVsLast = pSrcPhrase->m_chapterVerse;			// get the last m_ChapterVerse
		}
	}

	// if bFirst is still TRUE after the above scan, then there are no chapter or verse markers
	// in the text. We should disable the Chapter/Verse range, Preliminary, and Final Material buttons
	if (bFirst)
		exdlg.m_bDisableRangeButtons = TRUE;
	else
		exdlg.m_bDisableRangeButtons = FALSE;

	int vfirst, vlast;
	// BW added extra parameter Oct 2004, set it  > 0 so it has no effect on Bill's code here
	if (pView->AnalyseReference(ChVsFirst,nChFirst,vfirst,vlast,1))
	{
		// set the FromChapter and FromVerse of dialog to default to what's in the text
		exdlg.m_nFromChapter = nChFirst;
		exdlg.m_nFromVerse = vfirst;
		nVsFirst = vfirst;
	}
	// BW added extra parameter Oct 2004, set it  > 0 so it has no effect on Bill's code here
	if (pView->AnalyseReference(ChVsLast,nChLast,vfirst,vlast,1))
	{
		// set the ToChapter and ToVerse of dialog to default to what's in the text
		exdlg.m_nToChapter = nChLast;
		exdlg.m_nToVerse = vlast;
		nVsLast = vlast;
	}

	int nActualChLast = nChLast; // since user can change nChLast we need to keep the actual value
	int nActualVsLast = nVsLast; // " " "

	// inform OnInitDialog to check/uncheck and enable/disable include Gloss text
	// depending of gbGlossingVisible
	exdlg.m_bIncludeGlossText = gbGlossingVisible;

	// start with the Orientation that the PageSetup dialog would have as contained in 
	// the App's m_bIsPortraitOrientation.
	exdlg.m_bPortraitOrientation = gpApp->m_bIsPortraitOrientation;

	// update the dialog's checkboxes with current App values (redundant: it's done there too)
	exdlg.pCheckUsePrefixExportProjNameOnFilename->SetValue(gpApp->m_bUsePrefixExportProjectNameOnFilename);
	exdlg.pCheckUsePrefixExportTypeOnFilename->SetValue(gpApp->m_bUsePrefixExportTypeOnFilename);
	exdlg.pCheckUseSuffixExportDateTimeStamp->SetValue(gpApp->m_bUseSuffixExportDateTimeOnFilename);

	// substitute the actual project name string into the %s placeholder
	wxString expProjNamePrefixStr = gpApp->m_sourceName + _T('-') + gpApp->m_targetName + _T('_');
	wxString projNameExp;
	projNameExp = exdlg.pCheckUsePrefixExportProjNameOnFilename->GetLabel();
	projNameExp = projNameExp.Format(projNameExp, expProjNamePrefixStr.c_str());
	exdlg.pCheckUsePrefixExportProjNameOnFilename->SetLabel(projNameExp);

	// substitute the actual export type string into the %s placeholder
	wxString typeExp;
	wxString expTypePrefixStr = _T("interlinear_");
	typeExp = exdlg.pCheckUsePrefixExportTypeOnFilename->GetLabel();
	typeExp = typeExp.Format(typeExp, expTypePrefixStr.c_str());
	exdlg.pCheckUsePrefixExportTypeOnFilename->SetLabel(typeExp);

	// show the ExportInterlinear dialog
	if (exdlg.ShowModal() == wxID_OK)
	{
		// get the data from the controls, and use the data to set the various
		// flags needed in the export process.
		bInclSrcLangRow = exdlg.m_bIncludeSourceText;
		bInclTgtLangRow = exdlg.m_bIncludeTargetText;
		bInclGlsLangRow = exdlg.m_bIncludeGlossText;
		bInclNavLangRow = exdlg.m_bIncludeNavText;
		bNewTableForNewLineMarker = exdlg.m_bNewTableForNewLineMarker; // whm 13Oct06 added
		bCenterTableForCenteredMarker = exdlg.m_bCenterTableForCenteredMarker; // whm 13Oct06 added
		
		// 11Nov07 whm modified. If bDocHasFreeTrans == FALSE there are no actual free
		// translations in the document and we do not need an extra row in the table even
		// though the default is to do so in the Export Options dialog.
		if (bDocHasFreeTrans)
			bInclFreeTransRow = bPlaceFreeTransInRTFText; // retrieve from global - version 3
		if (bDocHasBackTrans)
			bInclBackTransRow = bPlaceBackTransInRTFText; // retrieve from global - version 3
		
		nChFirst = exdlg.m_nFromChapter;
		nChLast = exdlg.m_nToChapter;
		nVsFirst = exdlg.m_nFromVerse;
		nVsLast = exdlg.m_nToVerse;
		bUsePortrait = exdlg.m_bPortraitOrientation;
		bOutputAll = exdlg.m_bOutputAll;
		bOutputPrelim = exdlg.m_bOutputPrelim;
		bOutputFinal = exdlg.m_bOutputFinal;
		bOutputCVRange = exdlg.m_bOutputCVRange;
	}
	else
	{
		//bOK = ::wxSetWorkingDirectory(saveWorkDir); // ignore failures
		gpApp->LogUserAction(_T("Cancelled DoExportInterlinearRTF()"));
		return; // user cancelled
	}

	// Make adjustments to the exportFilename based on the user's preference for 
	// exportFilename prefix and suffix.
	// 
	// whm Note 8Jul11: When collaboration with PT/BE is ON, and when doing targetTextExport
	// operations in this case block, the exportFilename as obtained from m_curOutputFilename 
	// above will be of the form _Collab_45_ACT_CH02.txt. To distinguish these manually
	// produced exports within the _INTERLINEAR_RTF_OUTPUTS folder from those generated 
	// automatically by our collaboration code, we remove the "_Collab..." prefix. We
	// then add an exportType prefix "_Interlinear" if the user ticked the checkbox for
	// using export type prefixes in the ExportInterlinearDlg.
	wxString collabPrefix = _T("_Collab_"); // include the following _ here for removal
	int pos_Collab_;
	pos_Collab_ = exportFilename.Find(collabPrefix);
	if (pos_Collab_ != wxNOT_FOUND)
		exportFilename.Remove(pos_Collab_,collabPrefix.Length());
	if (gpApp->m_bUsePrefixExportTypeOnFilename)
		exportFilename = expTypePrefixStr + exportFilename;
	// whm 21Feb12 added at Kim's request. Put the src and tgt language names as prefix on exportFilename.
	if (gpApp->m_bUsePrefixExportProjectNameOnFilename)
		exportFilename = expProjNamePrefixStr + exportFilename; 

	// make a suitable default output filename for the export function
	int len = exportFilename.Length();
	exportFilename.Remove(len-3,3); // remove the .adt or .xml extension
	exportFilename += _T("rtf"); // make it a *.rtf file type

	// whm modified 7Jul11 to bypass the wxFileDialog when the export is protected from
	// navigation.
	wxString exportPath;
	wxString uniqueFilenameAndPath;
	// Prepare a unique filename and path from the exportFilename. This unique filename 
	// and path is used when the export is nav protected or when the user has ticked the
	// checkbox at the bottom of the ExportSaveAsDlg to indicate that a date-time stamp
	// is to be suffixed to the export filename, which ensures that any existing exports
	// are not overwritten.
	uniqueFilenameAndPath = GetUniqueIncrementedFileName(exportFilename,incrementViaDate_TimeStamp,TRUE,2,_T("_exported_")); // TRUE - always modify
	if (gpApp->m_bUseSuffixExportDateTimeOnFilename)
	{
		// Use the unique path for exportPath
		exportFilename = uniqueFilenameAndPath;
	}
	
	if (!bBypassFileDialog_ProtectedNavigation)
	{
		// get a file dialog
		wxString filter;
		filter = _("Exported Adapt It RTF Documents (*.rtf)|*.rtf|All Files (*.*)|*.*||");
		wxFileDialog fileDlg(
			(wxWindow*)wxGetApp().GetMainFrame(), // MainFrame is parent window for file dialog
			_("Filename For Exported Interlinear Document"),
			defaultDir,
			exportFilename,
			filter,
			wxFD_SAVE | wxFD_OVERWRITE_PROMPT); // | wxHIDE_READONLY); wxHIDE_READONLY 
						// deprecated in 2.6 - the checkbox is never shown
						// GDLC wxSAVE & wxOVERWRITE_PROMPT deprecated in 2.8
		fileDlg.Centre();

		if (fileDlg.ShowModal() != wxID_OK)
		{
			gpApp->LogUserAction(_T("Cancelled DoExportInterlinearRTF() from wxFileDialog()"));
			return; // user cancelled
		}
		exportPath = fileDlg.GetPath();	// GDLC 11Aug11 Put this line back in.
	}
	else
	{
		exportPath = gpApp->m_interlinearRTFOutputsFolderPath + gpApp->PathSeparator + exportFilename;
	}

	// whm Note: We set the App's m_lastInterlinearRTFOutputPath variable with the 
	// path part of the exportPath just used. We do this even when navigation 
	// protection is on, so that the special folders would be the initial path 
	// suggested if the administrator were to switch Navigation Protection OFF.
	wxString path, fname, ext;
	wxFileName::SplitPath(exportPath, &path, &fname, &ext);
	gpApp->m_lastInterlinearRTFOutputPath = path;
	
	wxFile f;

	if( !f.Open( exportPath, wxFile::write))
	{
	   #ifdef __WXDEBUG__
		  wxLogError(_("Unable to open export file.\n"));
		  wxMessageBox(_("Unable to open export file."),_T(""),wxICON_EXCLAMATION | wxOK);
	   #endif
		  //bOK = ::wxSetWorkingDirectory(saveWorkDir); // ignore failures
		  gpApp->LogUserAction(_T("Unable to open export file in DoExportInterlinearRTF()."));
		  return;	// whm - set it to return from this error rather than exit.
	}

	// The remainder of the DoExportRTF Function could be done by farming out the bulk
	// of it to a number of helper functions as follows:
	// enum OutputType {Interlinear,Scripture};
	// wxString BuildRTFHeader(Interlinear);	// includes opening brace, standard header control words,
	//										// font table (Interlinear fonts), color table, stylesheet
	//										// (Interlinear styles), doc level control words,
	//										// and header and footer. Note: BuildRTFHeader(Scripture)
	//										// builds a similar header but with all the Scripture
	//										// template fonts and styles matching the PNG Word Scripture
	//										// Template.
	// void FormatOutputTablesAndText();	// Do the table string building and output the data
	//										// (May leave the bulk of this in DoExportInterlinearRTF with
	//										// calls to smaller functions that help build the necessary strings)
	//										// Could call it void FormatScriptureText() when exporting Source
	//										// or Target text to RTF.
	//
	// Design Factors: Conceptually, Adapt_It's List of source phrases (SPList) on the Add contains all
	// the data we need. Our job is to retrieve the data from SPList and write it out to a text file,
	// in the process inserting the needed RTF tags which structure it into nicely formatted RTF tables.

	///////////////////////////// RTF FILE Structure////////////////////////////////
	// The general structure of an RTF file looks like this: (showing sample RTF):
	//   1. Opening brace {
	//   2. Standard header control words:
	//				\rtf1\ansi\ansicpg1252\deff0\deflang1033\deflangfe1033
	//   3. Font Table:
	//				{\fonttbl
	//				{\f0\fswiss\fprq2\fcharset0 Arial;}
	//				{\f1\fswiss\fcharset0\fprq2 System;}
	//				{\f2\fswiss\fcharset0\fprq2 System;}
	//				{\f3\fswiss\fcharset0\fprq2 System;}
	//				{\f4\fswiss\fcharset0\fprq2 System;}} ...
	//              Note: Entire font table enclosed in {} and each defined font within the table must
	//                    also be enclosed in {}
	//				Note: RTF font numbers \fN where N is the number & can be assigned to any number as
	//					  long as they are consistent within the RTF output file. If Word or some
	//					  other editor loads and resaves the file as RTF, the numbers will change and
	//					  Word will add many other fonts (even though they may not be used in the doc)
	//   4. Color Table:
	//				{\colortbl;\red0\green0\blue0;\red0\green0\blue128;\red128\green0\blue0;...}
	//              Note: Word outputs 16 sets of RGB. Entire color table must be enclosed in {}
	//   5. StyleSheet:
	//				{\stylesheet
	//				{\qj \li0\ri0\ltrpar\widctlpar\nooverflow\rin0\lin0\itap0 \f0\fs22 \snext0 Normal;}
	//				{\s1\ql \li0\ri0\keepn\ltrpar\widctlpar\nooverflow\rin0\lin0\itap0 \f1
	//				\fs22 \sbasedon0 \snext1 Source Language;}
	//				{\s2\ql \li0\ri0\keepn\ltrpar\widctlpar\nooverflow\rin0\lin0\itap0 \f2
	//				\fs22 \sbasedon0 \snext2 Target Language;}
	//				Note: Entire style sheet must be enclosed in {} and each defined style within the
	//					  table must also be enclosed in {}
	//				Note: RTF style numbers \sN where N is the number can be assigned to any number as
	//					  long as they are consistent within the RTF output file. If Word or some
	//					  other editor loads and resaves the file as RTF, the numbers will change
	//   6. Selected document level control words (these are not enclosed within braces)
	//				\paperw11904\paperh16836\margt1440\margb1440\margl1440\margr1440
	//				\horzdoc\viewkind1\viewscale100\nolnhtadjtbl\fetN\sectd
	//   7. Header & footer
	//				{\header \pard\plain
	//				\s5\ql \li0\ri0\widctlpar\tqc\tx4512\tqr\tx9024\ltrpar\aspalpha\aspnum
	//				\faauto\adjustright\rin0\lin0\itap0 \f0\fs18\cf1
	//				{\tab Tok Pisin/Nyindrou 1 Timothy Interlinear \par }}
	//				{\footer \pard\plain
	//				\s5\ql \li0\ri0\widctlpar\tqc\tx4512\tqr\tx9024\ltrpar\aspalpha\aspnum
	//				\faauto\adjustright\rin0\lin0\itap0 \f0\fs18\cf1
	//				{1 Timothy Interlinear.rtf \tab}
	//				{\field {\*\fldinst {PAGE}} {\fldrslt{}}}
	//				{\tab Wed, Jun 25, 23:48, 2003 \par}}
	//   8. Other RTF header markings we can ignore (like file tables, list tables, revision and RSID tables)
	//   9. Contents of the file (marked with the various RTF control words for the desired formatting)
	//				Note: Most, if not all actual text/content will be placed in {} where the braces
	//					  are used to delineate the extent of a particular formatting or style
	//				Note: Certain structures, like tables, have their own structure with
	//					  header information (may be prefixed as well as suffixed to the actual table
	//					  data) [see "The structure of a table within an RTF file" below]
	//  10. Closing brace }
	//
	//
	///////////// RTF TABLE Structure////////////////////////////////
	//
	//	Note: In examples in comments:	\f0 = font for Normal;
	//									\f1 = font for Src Lang;
	//									\s1 = style of Src Lang;
	//									\s2 = style of Tgt Lang;
	//									\s3 = style of Gls Lang;
	//									\s4 = style of Nav Lang;
	//									\ts21 = table style Table Grid;
	// The structure of a table within an RTF file looks like this:
	//   1. Table Row and Cell definitions for FIRST ROW [no enclosing braces here]
	//				\trowd \irow0\irowband0\ts21\trgaph108\trleft0
	//				\cellx1663   or, \clvertalt \cltxlrtb\cellx1663 (see below)
	//				\cellx3434
	//				\cellx5205
	//				\cellx6976
	//				\clvertalt \cltxlrtb\cellx8640
	//
	//				Note: There is no RTF table group; instead, tables are specified as paragraph
	//					  properties. A table is represented as a sequence of table rows. A table
	//					  row is a continuous sequence of paragraphs partitioned into cells. The
	//					  table row begins with the \trowd control word and ends with the \row
	//					  control word.
	//					\trowd sets the table row defaults
	//					\irowN N is the row index of the row being defined (zero is first row)
	//					\irowbandN N is the row index of the row, adjusted to account for header
	//					  rows. A header row has a value of ?. We will keep it same as N in \irowN
	//					\ts21 use table style 21 (Table Grid) as default for the row.
	//					\trgaphN N indicates 1/2 the gap size between cells in twips.
	//					\trleftN N indicates the displacement of the left side of the table
	//					  from the left margin (zero is on the margin, neg value is left of it,
	//					  and a positive value indents the whole table row to the right - in twips.

	//					(these below were optional and not currently implemented)
	//					\clvertalt text is vertically aligned to the top of the cell (as opposed
	//					  to \clvertalc for center, and \clvertalb for bottom alignment). We can
	//					  stick with top alignment even with RTL languages.
	//					\cltxtlrtb text in a cell flows from left to right and top to bottom. The
	//					  alternatives are \cltxtbrl for right to left and top to bottom (probably
	//					  our normal default for RTL languages); \cltxbtlr for left to right and
	//					  bottom to top; \cltxlrtbv left to right and top to bottom, vertical; and
	//					\cltxtbrlv top to bottom and right to left, vertical.

	//					\cellxN indicates the position of the right border of the cell in twips.
	//					  Each \cellxN defines the right boundary of a table cell, including its
	//					  half of the space between cells. N is in twips from the left end of the
	//					  row. There are 1440 twips/inch, so the last cellx value in the rows of
	//					  a table should be 8640 or less, if the table is to fit on a normal page
	//					  (having about 6 inches between margins). For a normal table each row
	//					  should have the same number of cellxN control words, and the N values
	//					  should be the same for each row defined in the table. The number
	//					  of \cellxN definitions used must match the number of \cell delimiters
	//					  used to divide up the row text into cells (see 4 below).
	//				Note: Technically Word 2002 only "needs" the row/cell definitions suffixed
	//					  after the row is defined (at 3 & 4 below), but Word repeats it here in
	//					  prefixed position - for "compatibility with older RTF readers" which
	//					  had the row/cell definitions prefixed on the data.
	//
	//   2. New paragraph marker
	//				\pard\plain
	//					\pard - start new paragraph with paragraph properties set to default.
	//					  Our default style is the "Normal" style (see 6 below).
	//					  Distinguished from the \par tag which starts a new paragraph which
	//					  inherits all paragraph properties defined in the previous paragraph.
	//					\plain - reset font (character) formatting properties to a default value
	//					  defined by the application. Our default character style is \f0
	//				Note: The specification says, to avoid problems: "place the entire table
	//					  definition before any paragraph properties, including \pard." Hence, we
	//					  follow Word's procedure of both prefixing the table definition before the
	//					  actual text of the 1st row (1 above), and suffixing it after it (7 below).
	//				Note: "Formatting specified within a group affects only the text within that
	//					  group. Generally, text within a group inherits the formatting of the text
	//					  in the preceding group. However, Microsoft implementations of RTF assume
	//					  that the footnote, annotation, header, and footer groups ... do not
	//					  inherit the formatting of the preceding text. Therefore, to ensure that
	//					  these groups are always formatted correctly, you should set the formatting
	//					  within these groups to the default with the \sectd, \pard, and \plain
	//					  control words, and then add any desired formatting."
	//   3. Style definition for this row
	//				\s1\ql \li0\ri0\widctlpar\intbl\aspalpha\aspnum\faauto\adjustright\rin0\lin0\yts21
	//				\f1\fs24\cf1
	//				Note: See comments in the ///STYLES/// area below for explanation of the style tags.
	//				Note: We simplify the table by having a single style definition here for
	//					  the entire row, rather than applying a style for each cell.
	//				Note: The \intbl control word is required to be embedded in the paragraph style's
	//					  formatting properties to indicated that the paragraph is part of a table.
	//					  The \ytsN control word designates the table style that was applied to
	//					  the row/cell (\yts21 = Table Grid style)
	//   4. The ACTUAL TEXT OF THE ROW with \cell tags used to delimit the contents of each cell}
	//				{This is\cell some\cell Source\cell Language\cell text.\cell }
	//				Note: The textual content of the row must be enclosed in {}
	//					  In prefixed position (of row 0), the row/cell definitions don't require
	//						the \row tag after the last \cellxN value for the row.
	//   5. New paragraph marker \pard\plain
	//   6. Default "Normal" Style definition (probably in case user removed a style the
	//				program will know what the "Normal" style will be for the row)
	//				\ql \li0\ri0\widctlpar\intbl\aspalpha\aspnum\faauto\adjustright\rin0\lin0
	//				\f0\fs24\cf1
	//				Note: The Normal style does not use a \sN tag, but starts with the \ql tag.
	//					  See the ///STYLES/// area below for explanation of the style tags.
	//   7. Table Row and Cell definitions for FIRST ROW [Must use braces here!! in the normal suffix
	//					position for row 0 and subsequent rows]
	//				{\f1 \trowd \irow0\irowband0\ts21\trgaph108\trleft0
	//				\cellx1663  or, \clvertalt \cltxlrtb\cellx1663 (see note for prefixed position above)
	//				\cellx3434
	//				\cellx5205
	//				\cellx6976
	//				\cellx8640/row}
	//				Note: Compare this definition with the one in prefix position in 1 above. Word
	//				wants the font to be used (\f1) to be prefixed to the table row/cell
	//				definitions. All suffixed definitions for the row must end with the \row
	//				contol word before the closing brace of the row/cell definitions.
	//   8. Repeat 3-7 above for second and successive rows in the table.
	//				Note: Step 7 of the last/bottom row of the table must include the \lastrow tag
	//					  as shown here: {\f4 \trowd \irow3\irowband3\lastrow \ts21...\row}
	//   9. New Paragraph \pard
	//  10. Abbreviated Default "Normal" Style definition (compare with 6 above)
	//				\ql \li0\ri0\widctlpar\aspalpha\aspnum\faauto\adjustright\rin0\lin0\itap0
	//				Note: Note addition of \itap0 here. It indicates the paragraph nesting level,
	//				where 0 is the main document, 1 is a table cell, 2 is a nested table cell,
	//				3 is a doubly nested table cell, and so forth. The default is 1. \itap0
	//				apparently signals that we are no longer talking about table paragraphs.
	//  11. New Paragraph {\par }
	//				Note: 10 and 11 above function to place a Normal style paragraph
	//					  after the table. This is needed to separate tables, otherwise
	//					  the tables will "merge" together on the page.

	wxString hstr; // most of our RTF output goes through this string

	// Future development: The specifications for some or all of the RTF Tags below may
	// eventually be read from an external style file

	/////////////////////Header Control Tags/////////////////////////////
	// RTF code words for initial RTF header (H...) properties
	wxString Hrtf1 = _T("\\rtf1");
			// \rtf1 - rtfN where N is the major version number of the
			// Specification. The current specification is 1.7, hence N=1
	wxString Hcharset = _T("\\ansi");
			// NOTE: Should this be changed from \ansi to a value retrievable from the
			// local system which is outputting RTF from AI??? What should happen
			// when _UNICODE is defined ???
			// \ansi - the character set used in this RTF document. The RTF
			// specification currently supports the following character sets:
			//	\ansi	ANSI (the default)
			//	\mac	Apple Macintosh
			//	\pc		IBM PC code page 437
			//	\pca	IBM PC code page 850, used by IBM
			//			Personal System/2 (not implemented
			//			in version 1 of Microsoft Word for OS/2)

	//wxChar* pLoc;
	//pLoc = _tsetlocale( LC_ALL, _T("") ); // setlocale() requires "locale.h" include file
	//char* pLoc;
	//pLoc = setlocale( LC_ALL, _T("") ); // setlocale() requires "locale.h" include file
	//wxString Locale = pLoc;			// setlocale with "" gets current region.codepage info
	//								// and is in the form "English_USA.1252"
	// wx version: wxWidgets has easier to use facilities for determining the locale and other
	// international data about the user's computer than the MFC version.
	// Note: some of the following may want to eventually move to the App.
	// !!! testing below !!!
	wxLocale locale;
	//int systemLanguage = locale.GetSystemLanguage();				// Tries to detect the user's default language setting.
																	// Returns the wxLanguage value or wxLANGUAGE_UNKNOWN if
																	// the language-guessing algorithm failed.

	//wxFontEncoding systemEncoding = locale.GetSystemEncoding();		// Tries to detect the user's default font
																	// encoding. Returns wxFontEncoding value or
																	// wxFONTENCODING_SYSTEM if it couldn't be determined

	wxString systemEncodingName = locale.GetSystemEncodingName();	// Tries to detect the name of the user's default
																	// font encoding. This string isn't particularly
																	// useful for the application as its form is
																	// platform-dependent and so you should probably
																	// use GetSystemEncoding instead. Returns a user
																	// readable string value or an empty string if it
																	// couldn't be determined.
	wxString Locale = systemEncodingName; // this seems closest wx function has '-' also in DoExportInterlinearRTF
	// !!! testing above !!!

	wxString temp;
	temp = Locale.Mid(Locale.Find('-')+1);// get just the codepage number after the '-'
	wxString Hcodepg = _T("\\ansicpg") + temp;
			// \ansicpgN This keyword represents the ANSI code page
			// used to perform the Unicode to ANSI conversion when writing RTF
			// text. N represents the code page in decimal. This is typically
			// set to the default ANSI code page of the run-time environment
			// (for example, \ansicpg1252 for U.S. Windows). The reader can use
			// the same ANSI code page to convert ANSI text back to Unicode.
	wxString HUnicodeNumBytes = _T("\\uc1");
			// NOTE: The spcification's description of this keyword is somewhat
			// misleading. It says,
			// "The \ucN keyword represents the number of bytes corresponding to a
			// given \uN Unicode character. This keyword may be used at any time,
			// and values are scoped like character properties. That is, a \ucN
			// keyword applies only to text following the keyword, and within
			// the same (or deeper) nested braces. On exiting the group, the
			// previous \uc value is restored. The reader must keep a stack of
			// counts seen and use the most recent one to skip the appropriate
			// number of characters when it encounters a \uN keyword. When
			// leaving an RTF group that specified a \uc value, the reader must
			// revert to the previous value. A default of 1 should be assumed
			// if no \uc keyword has been seen in the current or outer scopes."
			// The N of \ucN actually indicates the number of "characters" to
			// "skip" (as only hinted at above).
	wxString Hdeffont = _T("\\deff0");
			// \deff0 - deffN where N specifies the default font to use for any
			// text in the RTF file that has no explicit font specification

			// NOTE: After the output RTF file is loaded into an RTF capable word
			// processor such as Word, and subsequently saved again to RTF,
			// the Word will add the following control words to the RTF output.
			// They will reflect the default fonts on the local system. These
			// include the following codes which are not encoded in the AI RTF
			// output here:
			//	\deflangN and \deflangfeN where N=1033, 3081, etc.
			//	\stshfdbchN \stshflochN \stshfhichN \stshfbiN where N=font to
			//     use by default in the style sheet for Far East chars, ASCII chars
			//	   High-ANSI chars, and Comples Scripts (BiDi) respectively
			// [note: there are also other control words added by Word 2002 and
			//  Word 2003 - not listed here]

	/////////////////////Color Table/////////////////////////
	// RTF code words for color table properties
	// The \cfN code word uses N as index into this table. First empty ; is index 0
	// Build RTF color map
	MapMkrToColorStr colorMap; //CMapStringToString colorMap;
	// BuildColorTableFromUSFMColorAttributes() below populates the colorMap and
	// also returns the RTF colortable string
	wxString ColorTable = BuildColorTableFromUSFMColorAttributes(colorMap);

	// Below is the original hard coded color table:
	//wxString ColorTable = _T("\n{\\colortbl;\\red0\\green0\\blue0;\\red0\\green0\\blue255;\n");
	//	ColorTable += _T("\\red0\\green255\\blue255;\\red0\\green255\\blue0;\n");
	//	ColorTable += _T("\\red255\\green0\\blue255;\\red255\\green0\\blue0;\n");
	//	ColorTable += _T("\\red255\\green255\\blue0;\\red255\\green255\\blue255;\n");
	//	ColorTable += _T("\\red0\\green0\\blue128;\\red0\\green128\\blue128;\n");
	//	ColorTable += _T("\\red0\\green128\\blue0;\\red128\\green0\\blue128;\n");
	//	ColorTable += _T("\\red128\\green0\\blue0;\\red128\\green128\\blue0;\n");
	//	ColorTable += _T("\\red128\\green128\\blue128;\\red192\\green192\\blue192;}\n");

	/////////////////////FONTS//////////////////////////////
	// To compose the Sdef forms we need to get the current pSfmMap in order to look up the
	// appropriate USFMAnalysis struct pSfm and retrieve font properties.
	//MapSfmToUSFMAnalysisStruct* pSfmMap; //CMapStringToOb* pSfmMap;
	//pSfmMap = gpApp->GetCurSfmMap(gpApp->gCurrentSfmSet);

	// the following USFMAnalaysis pointers are used for getting font metrics for the interlinear
	// fonts defined in AI_USFM.xml. These pointers are used immediately below for building the
	// style definition tag strings, and also farther below for setting up the font metrics used
	// in GetTextExtent() calls on the text strings of table cells.
	USFMAnalysis* pSfmNrm = NULL; // __normal
	USFMAnalysis* pSfmSrc = NULL; // _src_lang_interlinear
	USFMAnalysis* pSfmTgt = NULL; // _tgt_lang_interlinear
	USFMAnalysis* pSfmGls = NULL; // _gls_lang_interlinear
	USFMAnalysis* pSfmNav = NULL; // _nav_lang_interlinear
	USFMAnalysis* pSfmFree = NULL; // free
	USFMAnalysis* pSfmBT = NULL; // bt
	USFMAnalysis* pSfmHF = NULL; // _hdr_ftr_interlinear
	USFMAnalysis* pSfmFnC = NULL; // _footnote_caller
	USFMAnalysis* pSfmAR = NULL; // _annotation_ref
	USFMAnalysis* pSfmFn = NULL; // f
	USFMAnalysis* pSfmAT = NULL; // _annotation_text
	USFMAnalysis* pSfmNB = NULL; // _notes_base
	USFMAnalysis* pSfmVB = NULL; // _vernacular_base

	// populate pointers for USFMAnalysis structs for building styles below
	pSfmNrm = pDoc->LookupSFM(wxString(_T("__normal"))); // make the literal a wxString to get the correct LookupSFM function
	wxASSERT(pSfmNrm != NULL);
	pSfmSrc = pDoc->LookupSFM(wxString(_T("_src_lang_interlinear")));
	wxASSERT(pSfmSrc != NULL);
	pSfmTgt = pDoc->LookupSFM(wxString(_T("_tgt_lang_interlinear")));
	wxASSERT(pSfmTgt != NULL);
	pSfmGls = pDoc->LookupSFM(wxString(_T("_gls_lang_interlinear")));
	wxASSERT(pSfmGls != NULL);
	pSfmNav = pDoc->LookupSFM(wxString(_T("_nav_lang_interlinear")));
	wxASSERT(pSfmNav != NULL);
	pSfmFree = pDoc->LookupSFM(wxString(_T("free")));
	wxASSERT(pSfmFree != NULL);
	pSfmBT = pDoc->LookupSFM(wxString(_T("bt")));
	wxASSERT(pSfmBT != NULL);
	pSfmHF = pDoc->LookupSFM(wxString(_T("_hdr_ftr_interlinear")));
	wxASSERT(pSfmHF != NULL);
	pSfmFnC = pDoc->LookupSFM(wxString(_T("_footnote_caller")));
	wxASSERT(pSfmFnC != NULL);
	pSfmAR = pDoc->LookupSFM(wxString(_T("_annotation_ref")));
	wxASSERT(pSfmAR != NULL);
	pSfmFn = pDoc->LookupSFM(wxString(_T("f")));
	wxASSERT(pSfmFn != NULL);
	pSfmAT = pDoc->LookupSFM(wxString(_T("_annotation_text")));
	wxASSERT(pSfmAT != NULL);
	pSfmNB = pDoc->LookupSFM(wxString(_T("_notes_base")));
	wxASSERT(pSfmNB != NULL);
	pSfmVB = pDoc->LookupSFM(wxString(_T("_vernacular_base")));
	wxASSERT(pSfmVB != NULL);

	// Version 2 Interlinear RTF output used Adapt It's fonts at the current fontsize as used on
	// screen. Version 3 uses fontSize and other attribute for the _src_lang_interlinear,
	// _tgt_lang_interlinear, _gls_lang_interlinear, and _nav_lang_interlinear as specified
	// in the AI_USFM.xml control file.
	// Before calling GetTextExtent() on the various displayed strings, we must setup the dC
	// for font attributes as called for in AI_USFM.xml (done farther below).

	// Note: When gbGlossingUsesNavFont == TRUE the Gls Lang row can calculate text extents based
	// on the pNavFnt; otherwise based on the pTgtFnt.
	wxASSERT(gpApp->m_pSourceFont != NULL);
	wxASSERT(gpApp->m_pTargetFont != NULL);
	wxASSERT(gpApp->m_pNavTextFont != NULL);

	// wx version note: Rather than using or changing our Adapt It screen fonts for the
	// various language texts, we create new temporary fonts and copy their properties
	// from our existing m_pSourceFont, m_pTargetFont, and m_pNavTextFont fonts.
	// These new temporary fonts are deleted near the end of DoExportInterlinearRTF.
	// We'll use CopyFontBaseProperties() to copy the font's basic properties (encoding,
	// family, and facename) from the corresponding screen fonts, but then we'll set the 
	// other font properties (point size, style, underline, and weight) from the attributes
	// for exported fonts established in the AI_USFM.xml file
	// 1. The font encoding, (there are about 40 "known font encodings")
	// 2. The font family, (default, decorative, roman script, swiss, modern or teletype)
	// 3. The font face name,
	// 4. The font point size,
	// 5. The font style, (normal, slant or italic)
	// 6. The font underlined flag, (true or false)
	// 7. The font weight, (normal, light or bold)
	wxFont* pRtfSrcFnt = new wxFont;
	wxASSERT(pRtfSrcFnt != NULL);
	CopyFontBaseProperties(gpApp->m_pSourceFont, pRtfSrcFnt);
	wxFont* pRtfTgtFnt = new wxFont;
	wxASSERT(pRtfTgtFnt != NULL);
	CopyFontBaseProperties(gpApp->m_pTargetFont, pRtfTgtFnt);
	wxFont* pRtfNavFnt = new wxFont;
	wxASSERT(pRtfNavFnt != NULL);
	CopyFontBaseProperties(gpApp->m_pNavTextFont, pRtfNavFnt);
	// The free and bt text font extents calculated on nav text modified for fontSize, bold and
	// underline of "free" and "bt" marker attributes
	wxFont* pRtfFreeFnt = new wxFont;
	wxASSERT(pRtfFreeFnt != NULL);
	CopyFontBaseProperties(gpApp->m_pNavTextFont, pRtfFreeFnt);
	wxFont* pRtfBtFnt = new wxFont;
	wxASSERT(pRtfBtFnt != NULL);
	CopyFontBaseProperties(gpApp->m_pNavTextFont, pRtfBtFnt);

	// The basic font properites have been copied, now set the other attributes.

	// Set up the RTF SOURCE font based on pSfmSrc (from _src_lang_interlinear USFM in AI_USFM.xml)
	// assign the USFMAnalysis specified fontSize, bold and underline characteristics to pRtfSrcFnt
	pRtfSrcFnt->SetPointSize(pSfmSrc->fontSize);
	if (pSfmSrc->bold)
		pRtfSrcFnt->SetWeight(wxFONTWEIGHT_BOLD);
	else
		pRtfSrcFnt->SetWeight(wxFONTWEIGHT_NORMAL);
	if (pSfmSrc->underline)
		pRtfSrcFnt->SetUnderlined(TRUE);
	else
		pRtfSrcFnt->SetUnderlined(FALSE);
	if (pSfmSrc->italic)
		pRtfSrcFnt->SetStyle(wxFONTSTYLE_ITALIC);
	else
		pRtfSrcFnt->SetStyle(wxFONTSTYLE_NORMAL);
	// pRtfSrcFnt is deleted toward the end of DoExportInterlinearRTF to avoid memory leaks


	// Set up the RTF TARGET font based on pSfmTgt (from _tgt_lang_interlinear USFM in AI_USFM.xml)
	// assign the USFMAnalysis specified fontSize, bold and underline characteristics to pRtfTgtFnt
	pRtfTgtFnt->SetPointSize(pSfmTgt->fontSize); 
	if (pSfmTgt->bold)
		pRtfTgtFnt->SetWeight(wxFONTWEIGHT_BOLD);
	else
		pRtfTgtFnt->SetWeight(wxFONTWEIGHT_NORMAL);
	if (pSfmTgt->underline)
		pRtfTgtFnt->SetUnderlined(TRUE);
	else
		pRtfTgtFnt->SetUnderlined(FALSE);
	if (pSfmTgt->italic)
		pRtfTgtFnt->SetStyle(wxFONTSTYLE_ITALIC);
	else
		pRtfTgtFnt->SetStyle(wxFONTSTYLE_NORMAL);
	// pRtfTgtFnt is deleted toward the end of DoExportInterlinearRTF to avoid memory leaks


	// Set up the RTF NAV TEXT font based on pSfmNav (from _nav_lang_interlinear USFM in AI_USFM.xml)
	// assign the USFMAnalysis specified fontSize, bold and underline characteristics to navLF
	pRtfNavFnt->SetPointSize(pSfmNav->fontSize);
	if (pSfmNav->bold)
		pRtfNavFnt->SetWeight(wxFONTWEIGHT_BOLD);
	else
		pRtfNavFnt->SetWeight(wxFONTWEIGHT_NORMAL);
	if (pSfmNav->underline)
		pRtfNavFnt->SetUnderlined(TRUE);
	else
		pRtfNavFnt->SetUnderlined(FALSE);
	if (pSfmNav->italic)
		pRtfNavFnt->SetStyle(wxFONTSTYLE_ITALIC);
	else
		pRtfNavFnt->SetStyle(wxFONTSTYLE_NORMAL);
	// pRtfNavFnt is deleted toward the end of DoExportInterlinearRTF to avoid memory leaks

	// Set up the RTF FREE text font based on pSfmFree (from "free" USFM in AI_USFM.xml)
	// Used pNavFnt and navLF as starting basis
	// assign the USFMAnalysis specified fontSize, bold and underline characteristics to navLF
	pRtfFreeFnt->SetPointSize(pSfmFree->fontSize); //navLF.lfHeight = -PointSizeToLFHeight(pSfmFree->fontSize);
	if (pSfmFree->bold)
		pRtfFreeFnt->SetWeight(wxFONTWEIGHT_BOLD);
	else
		pRtfFreeFnt->SetWeight(wxFONTWEIGHT_NORMAL);
	if (pSfmFree->underline)
		pRtfFreeFnt->SetUnderlined(TRUE);
	else
		pRtfFreeFnt->SetUnderlined(FALSE);
	if (pSfmFree->italic)
		pRtfFreeFnt->SetStyle(wxFONTSTYLE_ITALIC);
	else
		pRtfFreeFnt->SetStyle(wxFONTSTYLE_NORMAL);
	// pRtfFreeFnt is deleted toward the end of DoExportInterlinearRTF to avoid memory leaks

	// Set up the RTF BT text font based on pSfmBT (from "bt" USFM in AI_USFM.xml)
	// Used pNavFnt and navLF as starting basis
	pRtfBtFnt->SetPointSize(pSfmBT->fontSize);
	if (pSfmBT->bold)
		pRtfBtFnt->SetWeight(wxFONTWEIGHT_BOLD);
	else
		pRtfBtFnt->SetWeight(wxFONTWEIGHT_NORMAL);
	if (pSfmBT->underline)
		pRtfBtFnt->SetUnderlined(TRUE);
	else
		pRtfBtFnt->SetUnderlined(FALSE);
	if (pSfmBT->italic)
		pRtfBtFnt->SetStyle(wxFONTSTYLE_ITALIC);
	else
		pRtfBtFnt->SetStyle(wxFONTSTYLE_NORMAL);
	// pRtfBtFnt is deleted toward the end of DoExportInterlinearRTF to avoid memory leaks

	// RTF code words for our desired output font (F...) properties
	// Assign font numbers
	wxString FNumNrm = _T("0");
	wxString FNumSrc = _T("1");
	wxString FNumTgt = _T("2");
	wxString FNumGls = _T("3");
	wxString FNumNav = _T("4");

	// get the font charset numbers
	// wxWidgets does not have wxFont methods for determining the charset so I'll
	// arbitrarily set them all to zero (default to ANSI). I think Word and other RTF
	// readers override these values anyway.
	wxString FCharsetNrm = _T("\\fcharset0");	// default to ANSI (0)
	wxString FCharsetSrc = _T("\\fcharset0");
	wxString FCharsetTgt = _T("\\fcharset0");
	wxString FCharsetGls = _T("\\fcharset0");
	wxString FCharsetNav = _T("\\fcharset0");

	// get the Pitch part of the PitchAndFamily
	// wx version: wxWidgets doesn't handle font pitch, so we'll arbirarily set all to
	// wxFONTFAMILY_DEFAULT which in RTF is _T("\\fprq0 ")
	wxString FPitchNrm = _T("\\fprq2 ");// Normal default to variable pitch
	wxString FPitchSrc = _T("\\fprq0 ");
	wxString FPitchTgt = _T("\\fprq0 ");
	wxString FPitchGls = _T("\\fprq0 ");

	wxString FPitchNav = _T("\\fprq0 ");

	// get the Family part of the PitchAndFamily
	wxString FFamNrm;
	wxString FFamSrc;
	wxString FFamTgt;
	wxString FFamGls;
	wxString FFamNav;
#ifdef _RTL_FLAGS
	if (gpApp->m_bSrcRTL)
		FFamSrc = _T("\\fbidi");
	if (gpApp->m_bTgtRTL)
		FFamTgt = _T("\\fbidi");
	//if (gpApp->m_bGlsRTL)
	//	FFamGls = _T("\\fbidi");
	if (gpApp->m_bNavTextRTL)
		FFamNav = _T("\\fbidi");

#else
	FFamNrm = _T("\\fswiss"); // Arial is fswiss and is our default font

	switch(pRtfSrcFnt->GetFamily()) // use our temporary src font
	{
		case wxFONTFAMILY_ROMAN: FFamSrc = _T("\\froman");break;
		case wxFONTFAMILY_SWISS: FFamSrc = _T("\\fswiss");break;
		case wxFONTFAMILY_MODERN: FFamSrc = _T("\\fmodern");break;
		case wxFONTFAMILY_SCRIPT: FFamSrc = _T("\\fscript");break;
		case wxFONTFAMILY_DECORATIVE: FFamSrc = _T("\\fdecor");break;
		case wxFONTFAMILY_DEFAULT: FFamSrc = _T("\\fnil");break;
		default: FFamSrc = _T("\\fnil");
	}

	switch(pRtfTgtFnt->GetFamily()) // use our temporary tgt font
	{
		case wxFONTFAMILY_ROMAN: FFamTgt = _T("\\froman");break;
		case wxFONTFAMILY_SWISS: FFamTgt = _T("\\fswiss");break;
		case wxFONTFAMILY_MODERN: FFamTgt = _T("\\fmodern");break;
		case wxFONTFAMILY_SCRIPT: FFamTgt = _T("\\fscript");break;
		case wxFONTFAMILY_DECORATIVE: FFamTgt = _T("\\fdecor");break;
		case wxFONTFAMILY_DEFAULT: FFamTgt = _T("\\fnil");break;
		default: FFamTgt = _T("\\fnil");
	}

	if (gbGlossingUsesNavFont)
	{
		switch(pRtfNavFnt->GetFamily()) // use our temporary nav text font
		{
			case wxFONTFAMILY_ROMAN: FFamGls = _T("\\froman");break;
			case wxFONTFAMILY_SWISS: FFamGls = _T("\\fswiss");break;
			case wxFONTFAMILY_MODERN: FFamGls = _T("\\fmodern");break;
			case wxFONTFAMILY_SCRIPT: FFamGls = _T("\\fscript");break;
			case wxFONTFAMILY_DECORATIVE: FFamGls = _T("\\fdecor");break;
			case wxFONTFAMILY_DEFAULT: FFamGls = _T("\\fnil");break;
			default: FFamGls = _T("\\fnil");
		}
	}
	else
	{
		switch(pRtfTgtFnt->GetFamily()) // use our temporary tgt font
		{
			case wxFONTFAMILY_ROMAN: FFamGls = _T("\\froman");break;
			case wxFONTFAMILY_SWISS: FFamGls = _T("\\fswiss");break;
			case wxFONTFAMILY_MODERN: FFamGls = _T("\\fmodern");break;
			case wxFONTFAMILY_SCRIPT: FFamGls = _T("\\fscript");break;
			case wxFONTFAMILY_DECORATIVE: FFamGls = _T("\\fdecor");break;
			case wxFONTFAMILY_DEFAULT: FFamGls = _T("\\fnil");break;
			default: FFamGls = _T("\\fnil");
		}
	}

	switch(pRtfNavFnt->GetFamily()) // use our temporary nav text font
	{
		case wxFONTFAMILY_ROMAN: FFamNav = _T("\\froman");break;
		case wxFONTFAMILY_SWISS: FFamNav = _T("\\fswiss");break;
		case wxFONTFAMILY_MODERN: FFamNav = _T("\\fmodern");break;
		case wxFONTFAMILY_SCRIPT: FFamNav = _T("\\fscript");break;
		case wxFONTFAMILY_DECORATIVE: FFamNav = _T("\\fdecor");break;
		case wxFONTFAMILY_DEFAULT: FFamNav = _T("\\fnil");break;
		default: FFamNav = _T("\\fnil");
	}
#endif

	// Font Face Names
#ifdef __WXMSW__
	wxString FNameNrm = _T("Arial");// our default font name on Windows
#elif __WXGTK__
	wxString FNameNrm = _T("Sans");// our default font name on Linux/Ubuntu
#elif __WXMAC__
	// according to this site: http://www.ampsoft.net/webdesign-l/WindowsMacFonts.html
	// the Verdana font is available on both Windows and the Mac
	wxString FNameNrm = _T("Verdana");// our default font name on Mac
#endif
	// Get font face names stored on the App
	wxString FNameSrc = pRtfSrcFnt->GetFaceName();
	wxString FNameTgt = pRtfTgtFnt->GetFaceName();
	wxString FNameGls;
	if (gbGlossingUsesNavFont)
		FNameGls = pRtfNavFnt->GetFaceName();
	else
		FNameGls = pRtfTgtFnt->GetFaceName();

	wxString FNameNav = pRtfNavFnt->GetFaceName();

	// Build final font tag strings - enclosed in {}
	wxString FTagsNrm = _T("{\\f")
						+FNumNrm+FFamNrm+FCharsetNrm+FPitchNrm+FNameNrm
						+_T(";}");
	wxString FTagsSrc = _T("{\\f")
						+FNumSrc+FFamSrc+FCharsetSrc+FPitchSrc+FNameSrc
						+_T(";}");
	wxString FTagsTgt = _T("{\\f")
						+FNumTgt+FFamTgt+FCharsetTgt+FPitchTgt+FNameTgt
						+_T(";}");
	wxString FTagsGls = _T("{\\f")
						+FNumGls+FFamGls+FCharsetGls+FPitchGls+FNameGls
						+_T(";}");
	wxString FTagsNav = _T("{\\f")
						+FNumNav+FFamNav+FCharsetNav+FPitchNav+FNameNav
						+_T(";}");


	/////////////////////STYLES//////////////////////////////
	// Determine RTF code words for our desired output style (S...) properties
	// Assign style numbers
	// Note: Word processors like Word reassign all the style numbers internally
	// once they have been read in by the RTF reader. Hence, if the user decides
	// to output the document to RTF from Word subsequent to reading it in, all
	// the style will likely have different numbers. It depends on the fonts,
	// language settings, etc, installed on the local system.
	wxString SNumNrm = _T("0");
	wxString SNumSrc = _T("1");
	wxString SNumTgt = _T("2");
	wxString SNumGls = _T("3");
	wxString SNumNav = _T("4");
	wxString SNumHdrFtr = _T("5");
	wxString SNumFree = _T("6"); // added for v 3
	wxString SNumBT = _T("7"); // added for v 3

	// Version 3 adds two new character styles and two new paragraph styles to the interlinear output
	// stylesheet. We arbitrarily assign their style numbers to 26, 27, 28, and 29
	wxString SNumFnCaller = _T("26");// N for \csN footnote caller char style - added for version 3
	wxString SNumFnText = _T("27");	// N for \sN footnote text para style - added for version 3
	wxString SNumAnnotRef = _T("28");// N for \csN annotation ref char style - added for version 3
	wxString SNumAnnotText = _T("29");// N for \sN annotation text para style - added for version 3
	// we also need to define Sdefs for _notes_base, _vernacular_base and __normal because some
	// of the styles we need are based on one of these three.
	wxString SNum_notes_base = _T("30");
	wxString SNum_vernacular_base = _T("31");
	wxString SNum__normal = _T("0"); // this is same as version 2's SNumNrm above

	bool bReverseLayout = FALSE;	// False unless set otherwise when _RTL_FLAGS activated
	//bool bReverseLayout = TRUE;	// uncomment for testing RTL layout without _RTL_FLAGS activated

	// RTF code word for paragraph alignment (defaults to left \ql)
	// Assume that for RTL languages they want paragraph alignment to right \qr
	wxString SParaAlignNrm;
	wxString SParaAlignSrc;
	wxString SParaAlignTgt;
	wxString SParaAlignGls;
	wxString SParaAlignNav;

#ifdef _RTL_FLAGS
	bReverseLayout = gbRTL_Layout;			// local flag for RTL layout takes value of
											// global flag when _RTL_FLAGS is active
#else
	bReverseLayout = FALSE;
#endif

//#ifdef _RTL_FLAGS
//
//
//	if (bReverseLayout)
//		SParaAlignNrm = _T("\\qr ");// App doesn't have m_bNrmRTL.
//	else									// Make the Normal paragraph alignment
//		SParaAlignNrm = _T("\\ql ");// to the right for reverse layout
//
//	if (gpApp->m_bSrcRTL)
//		SParaAlignSrc = _T("\\qr");
//	else
//		SParaAlignSrc = _T("\\ql");
//	if (gpApp->m_bTgtRTL)
//		SParaAlignTgt = _T("\\qr");
//	else
//		SParaAlignTgt = _T("\\ql");
//	if (gbGlossingUsesNavFont)
//	{
//		if (gpApp->m_bNavTextRTL)
//			SParaAlignGls = _T("\\qr");
//		else
//			SParaAlignGls = _T("\\ql");
//	}
//	else
//	{
//		if (gpApp->m_bTgtRTL)
//			SParaAlignGls = _T("\\qr");
//		else
//			SParaAlignGls = _T("\\ql");
//	}
//	if (gpApp->m_bNavTextRTL)
//		SParaAlignNav = _T("\\qr");
//	else
//		SParaAlignNav = _T("\\ql");
//#else
	// Testing shows that when \taprtl and \rtlrow is used in our tables it
	// automatically changes the effect of \ql to \qr without the need of
	// swapping these strings programmatically, so we will leave them
	// all \ql which simplifies our coding for _UNICODE RTL languages
	SParaAlignNrm = _T("\\ql ");
	SParaAlignSrc = _T("\\ql ");
	SParaAlignTgt = _T("\\ql ");
	SParaAlignGls = _T("\\ql ");
	SParaAlignNav = _T("\\ql ");

//#endif
	wxString SParaAlignFtr = _T("\\ql ");

	// RTF code words for style properties (same for all styles)
	// TODO: check next two to see if really needed
	wxString Sleftindent  = _T("\\li0");	// left indent=0
	wxString Srightindent = _T("\\ri0");	// right indent=0
	wxString Swidowcontrol = _T("\\widctlpar");

	wxString Sltr_precedenceNrm;
	wxString Sltr_precedenceSrc;
	wxString Sltr_precedenceTgt;
	wxString Sltr_precedenceGls;
	wxString Sltr_precedenceNav;

#ifdef _RTL_FLAGS
	// paragraph display precedence default to \ltrpar
	// use \rtlpar for paragraphs displayed with RTL precedence
	if (bReverseLayout)									// Normal text follows bReverseLayout value
		Sltr_precedenceNrm = _T("\\rtlpar");
	else
		Sltr_precedenceNrm = _T("\\ltrpar");

	if (gpApp->m_bSrcRTL)
		Sltr_precedenceSrc = _T("\\rtlpar");
	else
		Sltr_precedenceSrc = _T("\\ltrpar");
	if (gpApp->m_bTgtRTL)
		Sltr_precedenceTgt = _T("\\rtlpar");
	else
		Sltr_precedenceTgt = _T("\\ltrpar");
	if (gbGlossingUsesNavFont)
	{
		if (gpApp->m_bNavTextRTL)
			Sltr_precedenceGls = _T("\\rtlpar");
		else
			Sltr_precedenceGls = _T("\\ltrpar");
	}
	else
	{
		if (gpApp->m_bTgtRTL)
			Sltr_precedenceGls = _T("\\rtlpar");
		else
			Sltr_precedenceGls = _T("\\ltrpar");
	}
	if (gpApp->m_bNavTextRTL)
		Sltr_precedenceNav = _T("\\rtlpar");
	else
		Sltr_precedenceNav = _T("\\ltrpar");
#else
	Sltr_precedenceNrm = _T("\\ltrpar");
	Sltr_precedenceSrc = _T("\\ltrpar");
	Sltr_precedenceTgt = _T("\\ltrpar");
	Sltr_precedenceGls = _T("\\ltrpar");
	Sltr_precedenceNav = _T("\\ltrpar");

#endif
	wxString Sltr_precedenceFtr = _T("\\ltrpar");

	// more RTF code words for style properties (same for all styles)
	// \aspalpha is "Auto spacing between DBC and English"
	// \aspnum is "Auto spacing between DBC and numbers"
	// I assume these need to be here since Word routinely adds them
	// to its RTF output
	wxString Sautospacingalpha = _T("\\aspalpha"); // used in the "Normal Table" and "Table Grid" styles
	wxString Sautospacingnum = _T("\\aspnum"); // used in the "Normal Table" and "Table Grid" styles

	// ???should these be set differently for RTL???
	// font alignment (the specification only says the default is "Auto")
	wxString SfontalignNrm = _T("\\faauto"); // used in the "Normal Table" and "Table Grid" styles
	//wxString SfontalignSrc = _T("\\faauto");
	//wxString SfontalignTgt = _T("\\faauto");
	//wxString SfontalignGls = _T("\\faauto");
	//wxString SfontalignNav = _T("\\faauto");
	//wxString SfontalignFtr = _T("\\faauto");

	// ???should these be set differently for RTL???
	// Automatically adjust right indent when document grid is defined.
	// The specification gives no more information for what a document
	// grid is or does
	wxString SadjustrightNrm = _T("\\adjustright"); // used in the "Normal Table" and "Table Grid" styles

	// Right indent for left-to-right paragraphs; left indent for
	// right-to-left paragraphs (the default is 0)
	// these seem to have RTL smarts built in, but we won't avail
	// ourselves of them here since we don't want indents within table cells
	wxString SrindentNrm = _T("\\rin0"); // used in the "Normal Table" and "Table Grid" styles

	// Left indent for left-to-right paragraphs; right indent for
	// right-to-left paragraphs (the default is 0)
	// these seem to have RTL smarts built in, but we won't avail
	// ourselves of them here since we don't want indents within table cells
	wxString SlindentNrm = _T("\\lin0"); // used in the "Normal Table" and "Table Grid" styles

	// \itapN is the RTF code word for Paragraph nesting level, where 0 is the main document,
	// 1 is a table cell, 2 is a nested table cell, 3 is a doubly nested table
	// cell, and so forth. The default is 1 (table cell). Since most of our
	// output is in tables, we will only use \itap0 to mark the style of the Normal
	// paragraphs that are between the tables
	wxString Sparnestlvl = _T("\\itap0");

	// Font size is in half points for RTF \fsN code word (the default is 24 for 12 point text)
	// Version 3 note: The style attribute for \fs (font size) now comes mainly from the AI_USFM.xml file
	wxString SfontsizeNrm = _T("\\fs20");		// we'll go with 10pt text for Normal (paragraphs between tables)

	// Version 3 note: The style attribute for \cf (color) now comes from the AI_USFM.xml file
	// Font color uses an index into the color table - cf 1 is black - \\red0\\green0\\blue0;

	// ???Can the language code be retrieved from the OS???
	// 1033 is English, US; 3081 is English, Australia; 9 English, General
	// The specification says, "This [code] table was generated by the
	// Unicode group for use with TrueType and Unicode." For now we will
	// omit these codes from the output. Once an RTF document is loaded
	// into a word processor like word on a local machine, should the
	// local machine then save the document in RTF format, it will
	// automatically add the necessary tags

	// Version 3 note: The style attribute for \sbasedon now comes from the AI_USFM.xml file
	// all paragraph styles based on Normal style

	// Version 3 note: The style attribute for \snext now comes from the AI_USFM.xml file

	// Version 3 note: The style names now come from the AI_USFM.xml file

	// character and table style numbers
	wxString STsNumTblNrm = _T("20");				// for \*\ts20 - Normal Table;
	wxString STsNumTblGrd = _T("21");				// for \*\ts21 - Table Grid;

	// RTF N values for \cellxN are in twips. However, AI keeps tabs on
	// page size in 1/1000ths of an inch
	// nPageWidth for A4 = 8267 (measured in thousandths of an inch)
	// nPageLength for A4 = 11692 (" ")
	// Therefore, we need to temporarily change the MapMode to MM_TWIPS

	// get a device context
	wxClientDC dC(gpApp->GetMainFrame()->canvas);
	
	// whm 8Jun12 changed SaveMapMode to be of type wxMappingMode rather than int (required in 2.9.3)
#if wxCHECK_VERSION(2,9,0)
	wxMappingMode SaveMapMode = dC.GetMapMode();
#else
	int SaveMapMode = dC.GetMapMode();
#endif
	// MFC had MM_LOENGLISH, in which each logical unit is converted to 0.01 inch.
	// and Positive x is to the right; positive y is up.
	// Although wxWidgets compiles with the wxMM_LOENGLISH identifier used here
	// it give a run-time error/assert because wxMM_LOENGLISH is not one of the
	// valid internal mapping modes.
	//dC.SetMapMode(wxMM_LOENGLISH); //dC.SetMapMode(MM_LOENGLISH);
	// whm: We'll see if wxMM_TWIPS works here even though it didn't work under MFC.
	// Nope. wxMM_TEXT, however, seems to work quite well.
	dC.SetMapMode(wxMM_TEXT);
	// MFC PROBLEM??? SetMapMode(MM_TWIPS) followed by a call to GetTextExtent gets
	// a value that looks like twips only for System fonts, but is 14.4 times
	// smaller for truetype and Otype fonts. MFC Library Ref says GetTextExtent
	// returns the size in "logical units." As a work-around I will follow Bruce's
	// example and call GetTextExtent with mapping mode set to MM_LOENGLISH which
	// returns hundredths of an inch, and calculate twips by multiplying by 14.4.

	// The ExportInterlinear dialog restricts the selection to either Portrait (approx. 6 in)
	// or landscape (approx. 9 in). That should be sufficient for most users, and simplifies our task.
	// Our bUsePortrait can be set independently of AI's page setup so we need to determine
	// our MaxRowWidth ourselves, by flipping the page width and length.
	int MaxRowWidth;
	wxString Landscape;
	wxString PageWidth;
	wxString PageLength;
	wxString MarginTop;
	wxString MarginBottom;
	wxString MarginLeft;
	wxString MarginRight;
	// whm Note: The DoExportInterlinearRTF() routine's CExportInterlinearDlg dialog has a "Maximum
	// output table width designed for:" section with "Portrait Orientation" and "Landscape Orientation"
	// radio buttons. The bUsePortrait flag was initialized to agree with the Page Setup dialog's 
	// "Orientation" radio buttons. If the user changes the CExportInterlinearDlg's setting, it will
	// be reflected in the value of bUsePortrait here. If user chooses to format interlinear output 
	// maximized for landscape, we don't automatically change the Page Setup dialog's Orientation 
	// to agree - let the user do that if he/she so wishes.
	// Therefore here we don't call GetPageOrientation() to get the Page Setup's value for Orientation
	// but use the CExportInterlinearDlg's explicit setting.
	//bUsePortrait = (gpApp->GetPageOrientation()== 1); // this is not needed
	
	// Do sanity check: Ensure that the MaxRowWidth never ends up negative which might happen if the
	// m_pageWidth were to be zero when DoExportInterlinearRTF(). This situation would probably signal
	// a problem with the page setup values.
	if (gpApp->m_pageWidth == 0)
	{
		gpApp->m_pageWidth = 8267;
	}
	if (gpApp->m_pageLength == 0)
	{
		gpApp->m_pageLength = 11692;
	}
	wxASSERT(gpApp->m_marginLeft + gpApp->m_marginRight < gpApp->m_pageWidth);
	wxASSERT(gpApp->m_marginBottom + gpApp->m_marginTop < gpApp->m_pageLength);

	if (bUsePortrait)
	{
		// 9024 twips for A4 portrait
		// Bruce has already converted page dimensions to 1000ths of an inch, so we multiply
		// by 1.44 to get twips for these
		MaxRowWidth = (int)(((float)gpApp->m_pageWidth - (float)gpApp->m_marginLeft - (float)gpApp->m_marginRight) *1.44);
		Landscape = _T("");
		PageWidth = _T("\\paperw");
		PageWidth << (int)(gpApp->m_pageWidth*1.44);
		PageLength = _T("\\paperh");
		PageLength << (int)(gpApp->m_pageLength*1.44);
		MarginTop = _T("\\margt");
		MarginTop << (int)(gpApp->m_marginTop*1.44);
		MarginBottom = _T("\\margb");
		MarginBottom << (int)(gpApp->m_marginBottom*1.44);
		MarginLeft = _T("\\margl");
		MarginLeft << (int)(gpApp->m_marginLeft*1.44);
		MarginRight = _T("\\margr");
		MarginRight << (int)(gpApp->m_marginRight*1.44);
	}
	else
	{
		// 13956 twips for A4 landscape
		MaxRowWidth = (int)(((float)gpApp->m_pageLength - (float)gpApp->m_marginLeft - (float)gpApp->m_marginRight) *1.44);
		Landscape = _T("\\lndscpsxn");
		PageWidth = _T("\\paperw");
		PageWidth << (int)(gpApp->m_pageLength*1.44);	// swap Width with Length
		PageLength = _T("\\paperh");
		PageLength << (int)(gpApp->m_pageWidth*1.44);	// swap Width with Length
		MarginTop = _T("\\margt");
		MarginTop << (int)(gpApp->m_marginLeft*1.44);	// swap Top with Left
		MarginBottom = _T("\\margb");
		MarginBottom << (int)(gpApp->m_marginRight*1.44);// swap Bottom with Right
		MarginLeft = _T("\\margl");
		MarginLeft << (int)(gpApp->m_marginTop*1.44);	// swap Left with Top
		MarginRight = _T("\\margr");
		MarginRight << (int)(gpApp->m_marginBottom*1.44);// swap Right with Bottom
	}

	//	Page			A4 -portrait		A4 -landscape
	//	Dimensions		1000ths	twips		1000ths	twips
	//-------------------------------------------------------
	// m_pageWidth		8267	11904		11692	16836
	// m_pageLength		11692	16836		8267	11904
	// m_marginTop		1000	1440		1000	1440
	// m_marginBottom	1000	1440		1000	1440
	// m_marginLeft		1000	1440		1000	1440
	// m_marginRight	1000	1440		1000	1440
	// MaxRowWidth				9024				13956

	int CTabPos = MaxRowWidth / 2;
	int RTabPos = MaxRowWidth;

	// TODO: determine if these below are needed or could be used
	wxString TCenterN;
	TCenterN << CTabPos; // Center tab position in twips from left margin
	wxString TRightN;
	TRightN << RTabPos; // Right tab position in twips from left margin
	wxString TabCenter;
	TabCenter << _T("\\tqc\\tx") << TCenterN;
	wxString TabRight;
	TabRight << _T("\\tqr\\tx") << TRightN;

	// Explanation for the next two groups of string variables and part of what follows:
	// Version 2 utilized the observation that most of the style "definitions" (that are in
	// the RTF header at the beginning of the RTF file) share a number of common elements
	// with the "in-document" style tags (that have to be inserted within the document at
	// every place where the particular style format is required. Thus string variables
	// beginning with Sindoc... are of the later variety, whereas string variables
	// beginning with Sdef... are of the former variety. Since the Sindoc... forms are used
	// as the building blocks for building the Sdef... forms, we build the Sindoc... forms
	// first and use them to build the Sdef... ones. A little further down we utilize a
	// helper function called BuildRTFTagsMap(). It tries to build the regular parts of the
	// strings that will compose the Sindoc... and Sdef... tags. BuildRTFTagsMap() is
	// somewhat general in approach because it serves our DoExportSrcOrTgtRTF() function as
	// well as our present DoExportInterlinearRTF(). Because of the differences in regular
	// RTF output formatting and Interlinear RTF output formatting, we do a fair bit of
	// monkeying around with the strings that BuildRTFTagsMap() produces.

	// Version 3 in-document styles below hold the "in-document" style tags for building
	// Sdef forms below
	wxString Sindoc__normal;		// Normal para style
	wxString SindocSrc;				// \sN Src Lang para style
	wxString SindocTgt;				// \sN Tgt Lang para style
	wxString SindocGls;				// \sN Gls Lang para style
	wxString SindocNav;				// \sN Nav Lang para style
	wxString SindocFnCaller;		// \csN footnote caller char style
	wxString SindocFnText; 			// \sN footnote text para style
	wxString SindocAnnotRef;		// \csN annotation ref char style
	wxString SindocAnnotText;		// \sN annotation text para style
	wxString SindocHdrFtr;			// \sN header-footer text para style
	wxString SindocFree;			// \sN Free translation para style
	wxString SindocBT;				// \sN Back translation para style
	wxString Sindoc_notes_base;     // \sN Notes base para style
	wxString Sindoc_vernacular_base;// \sN Vernacular base para style

	// Version 3 the strings below will hold the style definition tags for the new styles
	wxString Sdef__normal;			// Style definition tags for Normal para style
	wxString SdefSrc;				// Style definition tags for Src Lang para style
	wxString SdefTgt;				// Style definition tags for Tgt Lang para style
	wxString SdefGls;				// Style definition tags for Gls Lang para style
	wxString SdefNav;				// Style definition tags for Nav Lang para style
	wxString SdefFnCaller; 			// Style definition tags for \csN footnote caller char style
	wxString SdefFnText; 			// Style definition tags for \sN footnote text para style
	wxString SdefAnnotRef; 			// Style definition tags for \csN annotation ref char style
	wxString SdefAnnotText;			// Style definition tags for \sN annotation text para style
	wxString SdefHdrFtr;			// Style definition tags for \sN header-footer para style
	wxString SdefFree;				// Style definition tags for \sN Free Translation para style
	wxString SdefBT;				// Style definition tags for \sN Back Translation para style
	wxString Sdef_notes_base;		// Style definition tags for \sN Notes base para style
	wxString Sdef_vernacular_base;	// Style defitition tags for \sN Vernacular base para style

	// whm 26Aug11 Open a wxProgressDialog instance here for export interlinear rtf operations.
	// The dialog's pProgDlg pointer is passed along through various functions that
	// get called in the process.
	// whm WARNING: The maximum range of the wxProgressDialog (nTotal below) cannot
	// be changed after the dialog is created. So any routine that gets passed the
	// pProgDlg pointer, must make sure that value in its Update() function does not 
	// exceed the same maximum value (nTotal).
	wxString msgDisplayed;
	const int nTotal = gpApp->GetMaxRangeForProgressDialog(App_SourcePhrases_Count) + 1;
	wxString progMsg = _("%s  - %d of %d Total words and phrases");
	wxFileName fn(exportFilename);
	msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),1,nTotal);
	wxProgressDialog* pProgDlg;
	pProgDlg = gpApp->OpenNewProgressDialog(_("Export to Interlinear RTF"),msgDisplayed,nTotal,500);
	int counter = 0;	
	// Build final style tag strings - enclosed in {}
	// Style information is used in three ways in our RTF output:
	// 1. Style Definitions in the RTF Header
	// 2. In-Document style specifier strings
	// 3. In-Table style specifier strings
	// The composition of each of the above varies but all share some parts in common
	// For efficiency, we will build the styles in parts, and use these parts to build
	// the three types of style statements

	// Version 2 did not need anything significant here in DoExportInterlinearRTF from the
	// rtfTagsMap used in DoExportSrcOrTgtRTF.
	// Version 3, however, does do some formatting of \note \free and \bt, namely \note can
	// be formatted either as footnotes or balloon text comments, \free and \bt (and
	// \bt...) can be formatted either as footnotes or as a separate new row of merged
	// cells at the bottom of the interlinear tables (separate row feature to be
	// implemented in 3.0.x). The rtfTagsMap is a Standard Template Library (STL) map
	// declared in the View's global space as follows:
	// std::map < wxString, wxString > rtfTagsMap;
	// typedef std::pair <const wxString, wxString > MkrTagStr_Pair;
	// std::map < wxString, wxString > :: const_iterator rtfIter;
	// The map "key" is the standard format marker (without backslash).
	// The map "value" is the in-document RTF style tags.
	// The rtfTagsMap is used here in DoExportInterlinearRTF and in DoExportSrcOrTgtRTF

	// Build Version 3 Sindoc and SDef tag strings for the 4 added styles. Rather than hard
	// coding them we'll start with the Sindoc forms we used in the DoExportSrcOrTgtRTF
	// routines and which are dynamically created from the AI_USFM.xml file and once
	// created are stored in the rtfTagsMap. Then we'll modify their style numbers for use
	// here in DoExportInterlinearRTF (which has fewer styles and hence fewer style
	// numbers).

	rtfTagsMap.clear(); // empty the MapBareMkrToRTFTags map of all elements

	wxArrayString StyleDefStrArray;
	wxArrayString StyleInDocStrArray;

	// For Interlinear RTF output, the OutputFont is likely not used, but if we do use it,
	// it will probably be the nav text font FNumNav.
	// TODO: See if more flexibility is required in selection of RTF font for \note, \free and \bt
	// associated text. In version 3.x these are not formatted with a particular font designation,
	// but they could be. Is there a need for the font selection for these to be on the Export
	// Options dialog? -- BEW answer, "no"
	wxString OutputFont = _T("\\f") + FNumNav;

	BuildRTFTagsMap(StyleDefStrArray,StyleInDocStrArray,OutputFont,colorMap,_T(""));
	// Note: The last parameter wxString Sltr_precedence is null here which means that
	// BuildRTFTagsMap and BuildRTFStyleTagString will not add the precedence tags
	// \ltrpar or \rtlpar to the style strings (as done when DoExportSrcOrTgtRTF calls
	// them). The precedence tags are added below for interlinear output which must
	// supply them for source, target, gloss and nav text styles.
	//
	// The above function has now populated rtfTagsMap with all the in-doc styles including
	// those we need, namely _src_lang_interlinear, _tgt_lang_interlinear, _gls_lang_interlinear,
	// _nav_lang_interlinear, _hdr_ftr_interlinear, SindocFnCaller, SindocFnText,
	// SindocAnnotRef, SindocAnnotText, Sindoc_notes_base, Sindoc_vernacular_base,
	// SindocFree, and SindocBT. We have to do two things: (1) renumber the styles for
	// our use here in DoExportInterlinearRTF, and (2) Create the corresponding Sdef forms from
	// the Sindoc forms by employing some calls to the pSfm maps to determine the extra attributes
	// needed for the Sdef forms (namely the styleName, basedOn, nextStyle, keepTogether,
	// and keepWithNext attributes).
	// 
	// First assign the Sindoc forms directly from those in the rtfTagsMap

	rtfIter = rtfTagsMap.find(_T("__normal"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		Sindoc__normal = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "__normal" !!!
	}
	rtfIter = rtfTagsMap.find(_T("_src_lang_interlinear"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		SindocSrc = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "_src_lang_interlinear" !!!
	}
	rtfIter = rtfTagsMap.find(_T("_tgt_lang_interlinear"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		SindocTgt = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "_tgt_lang_interlinear" !!!
	}
	rtfIter = rtfTagsMap.find(_T("_gls_lang_interlinear"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		SindocGls = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "_gls_lang_interlinear" !!!
	}
	rtfIter = rtfTagsMap.find(_T("_nav_lang_interlinear"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		SindocNav = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "_nav_lang_interlinear" !!!
	}
	rtfIter = rtfTagsMap.find(_T("_hdr_ftr_interlinear"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		SindocHdrFtr = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "_hdr_ftr_interlinear" !!!
	}
	rtfIter = rtfTagsMap.find(_T("_footnote_caller"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		SindocFnCaller = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "_footnote_caller" !!!
	}
	rtfIter = rtfTagsMap.find(_T("f"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		SindocFnText = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "f" !!!
	}
	rtfIter = rtfTagsMap.find(_T("_annotation_ref"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		SindocAnnotRef = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "_annotation_ref" !!!
	}
	rtfIter = rtfTagsMap.find(_T("_annotation_text"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		SindocAnnotText = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "_annotation_text" !!!
	}
	rtfIter = rtfTagsMap.find(_T("_notes_base"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		Sindoc_notes_base = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "_notes_base" !!!
	}
	rtfIter = rtfTagsMap.find(_T("_vernacular_base"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		Sindoc_vernacular_base = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "_vernacular_base" !!!
	}
	rtfIter = rtfTagsMap.find(_T("free"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		SindocFree = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "free" !!!
	}
	rtfIter = rtfTagsMap.find(_T("bt"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the marker key in map
		SindocBT = (wxString)rtfIter->second;
	}
	else
	{
		::wxBell(); 
		wxASSERT(FALSE); // assert here indicates malformed xml attribute for "bt" !!!
	}

	// we need to reassign the style numbers so they match the hard coded stylesheet
	// numbers assigned here in DoExportInterlinearRTF. Also for three of the main table row
	// styles (Src Lang, Tgt Lang, Gls Lang) we need to adjust the assigned font number
	// because BuildRTFTagsMap() set all of them to \f4 Nav Lang

	wxString styleNumStr, tempStr;
	wxString Tintbl = _T("\\intbl");				// Styles used on text in tables must have \intbl tag
	wxString SintblNrm;							// Style tag string for Normal when used within tables
	wxString SintblSrc;							// Style tag string for Src Lang when used within tables
	wxString SintblTgt;							// Style tag string for Tgt Lang when used within tables
	wxString SintblGls;							// Style tag string for Gls Lang when used within tables
	wxString SintblNav;							// Style tag string for Nav Lang when used within tables
	// two below added for version 3
	wxString SintblFree;
	wxString SintblBack;
	wxString TytsN = _T("\\yts")+STsNumTblGrd;	// Reference to table style \ts21 - used in the
												// paragraph style definition when applied to table text
	int startPos, endPos, parPos;

	////////////////////////////////////////////////////////////////////////////////
	// Adjust the In-Document forms for Interlinear use, and build the In-Table forms
	// for Normal, Src Lang, Tgt Lang, Gls Lang, and Nav Lang

	// The "Normal" style already had the \s0 removed from it and we only add a font tag.
	// First, adjust Normal style by removing the "\\par \n\\pard\\plain " prefix,
	parPos = Sindoc__normal.Find(_T("\\par ")+gpApp->m_eolStr+ _T("\\pard\\plain "));
	wxASSERT(parPos != -1);
	tempStr = Sindoc__normal.Mid(parPos + 17 + gpApp->m_eolStr.Length());
	// insert the Nrm precedence into Sindoc__normal
	int posw = tempStr.Find(_T("\\widctlpar"));
	tempStr = InsertInString(tempStr,posw,Sltr_precedenceNrm);
	Sindoc__normal = tempStr;	// Sindoc__normal is used to construct the style def and
								// in-table forms below
	// add \f0 font tag just before \fsN tag
	startPos = Sindoc__normal.Find(_T("\\fs"));
	Sindoc__normal = InsertInString(Sindoc__normal,startPos,_T("\\f") + FNumNrm);
	parPos = tempStr.Find(Swidowcontrol);
	if (parPos != -1)
		parPos += Swidowcontrol.Length();
	else
		parPos = tempStr.Find(_T("\\li")); // \widctrlpar not found so use \liN instead
	// insert the Tintbl and TytsN tags
	tempStr = InsertInString(tempStr,parPos,Tintbl); // no TytsN in normal style
	SintblNrm = tempStr;

	// adjust Src Lang style number and font number, remove the "\\par \n\\pard\\plain " prefix,
	styleNumStr = GetStyleNumberStrFromRTFTagStr(SindocSrc, startPos, endPos);
	// we won't use the existing styleNumStr but now have its startPos and endPos in the string
	// so we can replace it with our hard coded style number which for Src Lang is
	// SNumSrc and holds the numeric string "1"
	SindocSrc.Remove(startPos, endPos - startPos);
	SindocSrc = InsertInString(SindocSrc,startPos, SNumSrc); // "1"
	// we also need to change the font which was set to OutputFont (= "\f4") in BuildRTFTagsMap
	// call above.
	startPos = SindocSrc.Find(OutputFont);
	endPos = startPos + OutputFont.Length();
	SindocSrc.Remove(startPos, endPos - startPos);
	SindocSrc = InsertInString(SindocSrc,startPos, _T("\\f")+FNumSrc);
	// remove the \par \n\pard\plain prefix
	parPos = SindocSrc.Find(_T("\\par ")+gpApp->m_eolStr+ _T("\\pard\\plain "));
	wxASSERT(parPos != -1);
	tempStr = SindocSrc.Mid(parPos + 17 + gpApp->m_eolStr.Length());
	// insert the Src precedence into SindocSrc
	posw = tempStr.Find(_T("\\widctlpar"));
	tempStr = InsertInString(tempStr,posw,Sltr_precedenceSrc);
	SindocSrc = tempStr; // SindocSrc is used to construct the style def and in-table forms below
	// find a suitable place to insert extra tags for in-table form
	parPos = tempStr.Find(Swidowcontrol);
	if (parPos != -1)
		parPos += Swidowcontrol.Length();
	else
		parPos = tempStr.Find(_T("\\li")); // \widctrlpar not found so use \liN instead
	// insert the Tintbl and TytsN tags
	tempStr = InsertInString(tempStr,parPos,Tintbl + TytsN);
	// whm added 23Jul11: For the in-table style add a \qr or \ql depending on the natural 
	// RTL or LTR state for the Nav font which will force the cell text (paragraph) 
	// alignment within the table cells to be right-aligned for RTL and left-aligned for 
	// LRT. We don't do this outside of tables. This seems to be necessary for OpenOffice
	// and LibreOffice.
#ifdef _RTL_FLAGS
	if (gpApp->m_bSrcRTL)
		tempStr += _T("\\qr ");
	else
		tempStr += _T("\\ql ");
#else
	tempStr += _T("\\ql ");
#endif
	SintblSrc = tempStr;

	// adjust Tgt Lang style number and font number, remove the "\\par \n\\pard\\plain " prefix,
	styleNumStr = GetStyleNumberStrFromRTFTagStr(SindocTgt, startPos, endPos);
	// we won't use the existing styleNumStr but now have its startPos and endPos in the string
	// so we can replace it with our hard coded style number which for Tgt Lang is
	// SNumTgt and holds the numeric string "2"
	SindocTgt.Remove(startPos, endPos - startPos);
	SindocTgt = InsertInString(SindocTgt,startPos, SNumTgt); // "2"
	// we also need to change the font which was set to OutputFont (= "\f4") in BuildRTFTagsMap
	// call above.
	startPos = SindocTgt.Find(OutputFont);
	endPos = startPos + OutputFont.Length();
	SindocTgt.Remove(startPos, endPos - startPos);
	SindocTgt = InsertInString(SindocTgt,startPos, _T("\\f")+FNumTgt);
	// remove the \par \n\pard\plain prefix
	parPos = SindocTgt.Find(_T("\\par ")+gpApp->m_eolStr+ _T("\\pard\\plain "));
	wxASSERT(parPos != -1);
	tempStr = SindocTgt.Mid(parPos + 17 + gpApp->m_eolStr.Length());
	// insert the Tgt precedence into SindocTgt
	posw = tempStr.Find(_T("\\widctlpar"));
	tempStr = InsertInString(tempStr,posw,Sltr_precedenceTgt);
	SindocTgt = tempStr; // SindocTgt is used to construct the style def and in-table forms below
	// find a suitable place to insert extra tags for in-table form
	parPos = tempStr.Find(Swidowcontrol);
	if (parPos != -1)
		parPos += Swidowcontrol.Length();
	else
		parPos = tempStr.Find(_T("\\li")); // \widctrlpar not found so use \liN instead
	// insert the Tintbl and TytsN tags
	tempStr = InsertInString(tempStr,parPos,Tintbl + TytsN);
	// whm added 23Jul11: For the in-table style add a \qr or \ql depending on the natural 
	// RTL or LTR state for the Nav font which will force the cell text (paragraph) 
	// alignment within the table cells to be right-aligned for RTL and left-aligned for 
	// LRT. We don't do this outside of tables. This seems to be necessary for OpenOffice
	// and LibreOffice.
#ifdef _RTL_FLAGS
	if (gpApp->m_bTgtRTL)
		tempStr += _T("\\qr ");
	else
		tempStr += _T("\\ql ");
#else
	tempStr += _T("\\ql ");
#endif
	SintblTgt = tempStr;

	// adjust Gls Lang style number and font number, remove the "\\par \n\\pard\\plain " prefix,
	styleNumStr = GetStyleNumberStrFromRTFTagStr(SindocGls, startPos, endPos);
	// we won't use the existing styleNumStr but now have its startPos and endPos in the string
	// so we can replace it with our hard coded style number which for Gls Lang is
	// SNumGls and holds the numeric string "3"
	SindocGls.Remove(startPos, endPos - startPos);
	SindocGls = InsertInString(SindocGls,startPos, SNumGls); // "3"
	// we also need to change the font which was set to OutputFont (= "\f4") in BuildRTFTagsMap
	// call above.
	startPos = SindocGls.Find(OutputFont);
	endPos = startPos + OutputFont.Length();
	SindocGls.Remove(startPos, endPos - startPos);
	SindocGls = InsertInString(SindocGls,startPos, _T("\\f")+FNumGls);
	// remove the \par \n\pard\plain prefix
	parPos = SindocGls.Find(_T("\\par ")+gpApp->m_eolStr+ _T("\\pard\\plain "));
	wxASSERT(parPos != -1);
	tempStr = SindocGls.Mid(parPos + 17 + gpApp->m_eolStr.Length());
	// insert the Gls precedence into SindocGls
	posw = tempStr.Find(_T("\\widctlpar"));
	tempStr = InsertInString(tempStr,posw,Sltr_precedenceGls);
	SindocGls = tempStr; // SindocGls is used to construct the style def and in-table forms below
	// find a suitable place to insert extra tags for in-table form
	parPos = tempStr.Find(Swidowcontrol);
	if (parPos != -1)
		parPos += Swidowcontrol.Length();
	else
		parPos = tempStr.Find(_T("\\li")); // \widctrlpar not found so use \liN instead
	// insert the Tintbl and TytsN tags
	tempStr = InsertInString(tempStr,parPos,Tintbl + TytsN);
	
	// whm added 23Jul11: For the in-table style add a \qr or \ql depending on the natural 
	// RTL or LTR state for the Nav font which will force the cell text (paragraph) 
	// alignment within the table cells to be right-aligned for RTL and left-aligned for 
	// LRT. We don't do this outside of tables. This seems to be necessary for OpenOffice
	// and LibreOffice.
	if (gbGlossingUsesNavFont)
	{
#ifdef _RTL_FLAGS
		if (gpApp->m_bNavTextRTL)
			tempStr = _T("\\qr");
		else
			tempStr = _T("\\ql");
#else
		tempStr = _T("\\ql");

#endif
	}
	else
	{
#ifdef _RTL_FLAGS
		if (gpApp->m_bTgtRTL)
			tempStr = _T("\\qr");
		else
			tempStr = _T("\\ql");
#else
		tempStr = _T("\\ql");

#endif
	}
	SintblGls = tempStr;

	// adjust Nav Lang style number, remove the "\\par \n\\pard\\plain " prefix,
	// we adjust style number only for Nav since BuildRTFTagsMap already set the font to \f4
	styleNumStr = GetStyleNumberStrFromRTFTagStr(SindocNav, startPos, endPos);
	// we won't use the existing styleNumStr but now have its startPos and endPos in the string
	// so we can replace it with our hard coded style number which for Nav Lang is
	// SNumNav and holds the numeric string "4"
	SindocNav.Remove(startPos, endPos - startPos);
	SindocNav = InsertInString(SindocNav,startPos, SNumNav); // "4"
	// SindocNav already has \f4
	// remove the \par \n\pard\plain prefix
	parPos = SindocNav.Find(_T("\\par ")+gpApp->m_eolStr+ _T("\\pard\\plain "));
	wxASSERT(parPos != -1);
	tempStr = SindocNav.Mid(parPos + 17 + gpApp->m_eolStr.Length());
	// insert the Nav precedence into SindocNav
	posw = tempStr.Find(_T("\\widctlpar"));
	tempStr = InsertInString(tempStr,posw,Sltr_precedenceNav);
	SindocNav = tempStr; // SindocNav is used to construct the style def and in-table forms below
	// find a suitable place to insert extra tags for in-table form
	parPos = tempStr.Find(Swidowcontrol);
	if (parPos != -1)
		parPos += Swidowcontrol.Length();
	else
		parPos = tempStr.Find(_T("\\li")); // \widctrlpar not found so use \liN instead
	// insert the Tintbl and TytsN tags
	tempStr = InsertInString(tempStr,parPos,Tintbl + TytsN);
	
	// whm added 23Jul11: For the in-table style add a \qr or \ql depending on the natural 
	// RTL or LTR state for the Nav font which will force the cell text (paragraph) 
	// alignment within the table cells to be right-aligned for RTL and left-aligned for 
	// LRT. We don't do this outside of tables. This seems to be necessary for OpenOffice
	// and LibreOffice.
#ifdef _RTL_FLAGS
	if (gpApp->m_bNavTextRTL)
		tempStr += _T("\\qr ");
	else
		tempStr += _T("\\ql ");
#else
	tempStr += _T("\\ql ");
#endif
	SintblNav = tempStr;

	// adjust Free Trans style number
	styleNumStr = GetStyleNumberStrFromRTFTagStr(SindocFree, startPos, endPos);
	// we won't use the existing styleNumStr but now have its startPos and endPos in the string
	// so we can replace it with our hard coded style number which for free trans style is
	// SNumFree and holds the numeric string "6"
	SindocFree.Remove(startPos, endPos - startPos);
	SindocFree = InsertInString(SindocFree,startPos, SNumFree); // "6"
	// Free trans is a character style so there is no "\par \n\pard\plain " in it
	// nor do we need to insert the Tintbl and TytsN tags
	// so basically the SintblFree form is the same as the SindocFree form
	SintblFree = SindocFree;

	// adjust Back Trans style number
	styleNumStr = GetStyleNumberStrFromRTFTagStr(SindocBT, startPos, endPos);
	// we won't use the existing styleNumStr but now have its startPos and endPos in the string
	// so we can replace it with our hard coded style number which for back trans style is
	// SNumBT and holds the numeric string "7"
	SindocBT.Remove(startPos, endPos - startPos);
	SindocBT = InsertInString(SindocBT,startPos, SNumBT); // "7"
	// Back trans is a character style so there is no "\par \n\pard\plain " in it
	// nor do we need to insert the Tintbl and TytsN tags
	// so basically the SintblBack form is the same as the SindocBT form
	SintblBack = SindocBT;

	// Adjust (renumber) the In-Document forms for Interlinear use
	// for all other styles. Note: GetStyleNumberStrFromRTFTagStr() also returns
	// the startPos and endPos of the style number string. We disregard the
	// returned styleNumStr and only use startPos and endPos to delete and insert
	// the new style number.
	// All of the styles below use the Nav text font \f4 which BuildRTFTagsMap
	// inserted.
	styleNumStr = GetStyleNumberStrFromRTFTagStr(SindocHdrFtr, startPos, endPos);
	SindocHdrFtr.Remove(startPos, endPos - startPos);
	SindocHdrFtr = InsertInString(SindocHdrFtr,startPos, SNumHdrFtr); // "5"

	styleNumStr = GetStyleNumberStrFromRTFTagStr(SindocFnCaller, startPos, endPos);
	SindocFnCaller.Remove(startPos, endPos - startPos);
	SindocFnCaller = InsertInString(SindocFnCaller,startPos, SNumFnCaller); // "26"

	styleNumStr = GetStyleNumberStrFromRTFTagStr(SindocFnText, startPos, endPos);
	SindocFnText.Remove(startPos, endPos - startPos);
	SindocFnText = InsertInString(SindocFnText,startPos, SNumFnText); // "27"
	// footnote (for bt text and free text) text should have same precedence as target text
#ifdef _UNICODE
	if (gpApp->m_bTgtRTL)
	{
		// insert the Tgt precedence into SindocFnText
		posw = SindocFnText.Find(_T("\\widctlpar"));
		SindocFnText = InsertInString(SindocFnText,posw,Sltr_precedenceTgt);
	}
#endif

	styleNumStr = GetStyleNumberStrFromRTFTagStr(SindocAnnotRef, startPos, endPos);
	SindocAnnotRef.Remove(startPos, endPos - startPos);
	SindocAnnotRef = InsertInString(SindocAnnotRef,startPos, SNumAnnotRef); // "28"

	styleNumStr = GetStyleNumberStrFromRTFTagStr(SindocAnnotText, startPos, endPos);
	SindocAnnotText.Remove(startPos, endPos - startPos);
	SindocAnnotText = InsertInString(SindocAnnotText,startPos, SNumAnnotText); // "29"
	// annotation text should have same precedence as nav text
#ifdef _UNICODE
	if (gpApp->m_bNavTextRTL)
	{
		// insert the Nav precedence into SindocFnText
		posw = SindocFnText.Find(_T("\\widctlpar"));
		SindocFnText = InsertInString(SindocFnText,posw,Sltr_precedenceNav);
	}
#endif

	styleNumStr = GetStyleNumberStrFromRTFTagStr(Sindoc_notes_base, startPos, endPos);
	Sindoc_notes_base.Remove(startPos, endPos - startPos);
	Sindoc_notes_base = InsertInString(Sindoc_notes_base,startPos, SNum_notes_base); // "30"

	styleNumStr = GetStyleNumberStrFromRTFTagStr(Sindoc_vernacular_base, startPos, endPos);
	Sindoc_vernacular_base.Remove(startPos, endPos - startPos);
	Sindoc_vernacular_base = InsertInString(Sindoc_vernacular_base,startPos, SNum_vernacular_base); // "31"

	wxString bareMkr;

	// We lookup the markers that we need more specs for in the active USFMAnalysis struct map,
	// and get its pSfm... struct, in order to supply attributes as necessary

	// build the __normal style definition
	Sdef__normal = _T('{');
	Sdef__normal += Sindoc__normal;
	// no basedon in __normal, but can use snext0
	Sdef__normal += _T("\\snext");
	Sdef__normal += SNumNrm;
	Sdef__normal += _T(' ');
	AddAnyStylenameColon(Sdef__normal,pSfmNrm);
	Sdef__normal += _T('}');

	// build the _src_lang_interlinear style definition
	SdefSrc = _T('{') + SindocSrc;
	SdefSrc += _T("\\sbasedon");
	SdefSrc += SNumNrm;
	SdefSrc += _T(' ');
	SdefSrc += _T("\\snext");
	SdefSrc += SNumSrc;
	SdefSrc += _T(' ');
	AddAnyStylenameColon(SdefSrc,pSfmSrc);
	SdefSrc += _T('}');

	// build the _tgt_lang_interlinear style definition
	SdefTgt = _T('{') + SindocTgt;
	SdefTgt += _T("\\sbasedon");
	SdefTgt += SNumNrm;
	SdefTgt += _T(' ');
	SdefTgt += _T("\\snext");
	SdefTgt += SNumTgt;
	SdefTgt += _T(' ');
	AddAnyStylenameColon(SdefTgt,pSfmTgt);
	SdefTgt += _T('}');

	// build the _gls_lang_interlinear style definition
	SdefGls = _T('{') + SindocGls;
	SdefGls += _T("\\sbasedon");
	SdefGls += SNumNrm;
	SdefGls += _T(' ');
	SdefGls += _T("\\snext");
	SdefGls += SNumGls;
	SdefGls += _T(' ');
	AddAnyStylenameColon(SdefGls,pSfmGls);
	SdefGls += _T('}');

	// build the _nav_lang_interlinear style definition
	SdefNav = _T('{') + SindocNav;
	SdefNav += _T("\\sbasedon");
	SdefNav += SNumNrm;
	SdefNav += _T(' ');
	SdefNav += _T("\\snext");
	SdefNav += SNumNav;
	SdefNav += _T(' ');
	AddAnyStylenameColon(SdefNav,pSfmNav);
	SdefNav += _T('}');

	// Build the Sdef forms for Free translation using the above renumbered Sindoc form, and
	// adding the styleName, semicolon and enclosing braces. Since \free is a character style
	// we do not need basedOn or nextStyle info.
	SdefFree = _T('{');
	SdefFree += _T("\\*"); // char style definition needs prefixed \* before \cs
	SdefFree += SindocFree;
	AddAnyStylenameColon(SdefFree,pSfmFree);
	SdefFree += _T('}');

	// Build the Sdef forms for Back translation using the above renumbered Sindoc form, and
	// adding the styleName, semicolon and enclosing braces. Since \bt is a character style
	// we do not need basedOn or nextStyle info.
	SdefBT = _T('{');
	SdefBT += _T("\\*"); // char style definition needs prefixed \* before \cs
	SdefBT += SindocBT;
	AddAnyStylenameColon(SdefBT,pSfmBT);
	SdefBT += _T('}');

	// Build the Sdef forms for _hdr_ftr_interlinear using the above renumbered Sindoc form, and
	// adding the styleName, semicolon and enclosing braces. _hdr_ftr_interlinear is a paragraph
	// style.
	// the headerSty build routine omits the \par \n\pard\plain
	SdefHdrFtr = _T('{');
	SdefHdrFtr += SindocHdrFtr;
	AddAnyStylenameColon(SdefHdrFtr,pSfmHF);
	SdefHdrFtr += _T('}');

	// Build the Sdef forms for footnote caller character style using the above
	// renumbered Sindoc forms, and adding the styleName, semicolon and enclosing braces
	SdefFnCaller = _T('{');
	SdefFnCaller += _T("\\*"); // char style definition needs prefixed \* before \cs
	SdefFnCaller += SindocFnCaller;
	AddAnyStylenameColon(SdefFnCaller,pSfmFnC);
	SdefFnCaller += _T('}');

	// Build the Sdef forms for annotation ref character style using the above
	// renumbered Sindoc forms, and adding the styleName, semicolon and enclosing braces
	SdefAnnotRef = _T('{');
	SdefAnnotRef += _T("\\*"); // char style definition needs prefixed \* before \cs
	SdefAnnotRef += SindocAnnotRef;
	AddAnyStylenameColon(SdefAnnotRef,pSfmAR);
	SdefAnnotRef += _T('}');

	// Build the Sdef forms for footnote text "f" paragraph style using those above
	// renumbered Sindoc forms and attributes needed from pSfm.
	// We use the Sindoc forms above and build the Sdef paragraph forms generally like this:
	// '{' + Sindoc + basedOn + nextStyle + styleName + semicolon + \par \n\pard\plain + sp + '}'
	SdefFnText = _T('{') + SindocFnText;
	// Note: SindocFnText is an unusual case because the BuildRTFTagsMap() does not prefix the
	// "\\par \n\\pard\\plain " string to the Sindoc form because the footnote text style does not
	// require the "\\par \n\\pard\\plain " sequence when creating the special footnote tags. Our
	// FormatRTFFootnoteIntoString() routine which is called later below also does not need the
	// sequence.
	// AddAnyBasedonNext(SdefFnText,pSfmFn); // We do this manually below since footnote text "f" is based
	// on _vernacular_base which is hard coded as SNum_vernacular_base ("31"), and its snext
	// is SNumFnText ("27")
	SdefFnText += _T("\\sbasedon") + SNum_vernacular_base + _T(' '); // \sbasedon31
	SdefFnText += _T("\\snext") + SNumFnText + _T(' '); // \snext27
	AddAnyStylenameColon(SdefFnText,pSfmFn);
	SdefFnText += _T('}');

	// SindocAnnotText will have the "\\par \n\\pard\\plain " string prefixed to it
	// In DoExportInterlinearRTF so we build the Sdef string in reverse order from what was done in
	// DoExportSrcOrTgtRTF, we need to remove the "\\par \n\\pard\\plain " prefix
	// which does not belong in the style definition Sdef string.
	parPos = SindocAnnotText.Find(_T("\\par ")+gpApp->m_eolStr+ _T("\\pard\\plain "));
	wxASSERT(parPos != -1);
	tempStr = SindocAnnotText.Mid(parPos + 17 + gpApp->m_eolStr.Length());
	SdefAnnotText = _T('{') + tempStr;
	//AddAnyBasedonNext(SdefAnnotText,pSfmAT); // We do it manually below since _annotation_text is based
	// on __normal which is hard coded as SNum__normal ("0"), and its snext is SNumAnnotText
	// which is ("29")
	SdefAnnotText += _T("\\sbasedon") + SNum__normal + _T(' '); // \sbasedon0
	SdefAnnotText += _T("\\snext") + SNumAnnotText + _T(' '); // \snext29
	AddAnyStylenameColon(SdefAnnotText,pSfmAT);
	SdefAnnotText += _T('}');

	// do the same for _notes_base
	parPos = Sindoc_notes_base.Find(_T("\\par ")+gpApp->m_eolStr+ _T("\\pard\\plain "));
	wxASSERT(parPos != -1);
	tempStr = Sindoc_notes_base.Mid(parPos + 17 + gpApp->m_eolStr.Length());
	Sdef_notes_base = _T('{') + tempStr;
	//AddAnyBasedonNext(Sdef_notes_base,pSfmNB); // We do it manually below since _notes_base text is based
	// on _vernacular_base and snext is itself.
	Sdef_notes_base += _T("\\sbasedon") + SNum_vernacular_base + _T(' '); // \sbasedon31
	Sdef_notes_base += _T("\\snext") + SNum_notes_base + _T(' ');// \snext30
	AddAnyStylenameColon(Sdef_notes_base,pSfmNB);
	Sdef_notes_base += _T('}');

	// do the same for _vernacular_base
	parPos = Sindoc_vernacular_base.Find(_T("\\par ")+gpApp->m_eolStr+ _T("\\pard\\plain "));
	wxASSERT(parPos != -1);
	tempStr = Sindoc_vernacular_base.Mid(parPos + 17 + gpApp->m_eolStr.Length());
	Sdef_vernacular_base = _T('{') + tempStr;
	//AddAnyBasedonNext(Sdef_vernacular_base,pSfmVB); // We do it manually below since _vernacular_base text
	// is based on __normal
	Sdef_vernacular_base += _T("\\sbasedon") + SNum__normal + _T(' '); // \sbasedon0
	Sdef_vernacular_base += _T("\\snext") + SNum_vernacular_base + _T(' '); // \snext31
	AddAnyStylenameColon(Sdef_vernacular_base,pSfmVB);
	Sdef_vernacular_base += _T('}');

	wxString Ssemihidden = _T("\\ssemihidden"); // used in the "Normal Table" style

	// (see MFC version for additional commented out parts having to do with additional character
	// styles)

	// Build the TABLE styles
	wxString STsTag = _T("\\*\\ts"); // Table styles must be in form \*\tsN
	wxString STsDefTbl = _T("Normal Table;");
	wxString STsTGrd = _T("Table Grid;");
	// Build the Table style (ts) definition strings
	wxString STsTblNrm = _T("{")		//opening brace
		+STsTag+STsNumTblNrm
		+_T("\\tsrowd\\tsvertalt ") // \tsvertalt = "top vertical alignment of cell
		+SParaAlignNrm+Sleftindent+Srightindent
		+Swidowcontrol+Sautospacingalpha
		+Sautospacingnum+SfontalignNrm+SadjustrightNrm
		+SrindentNrm+SlindentNrm+Sparnestlvl
		+_T(" ")
		+SfontsizeNrm
		+_T(" \\snext")+STsNumTblNrm
		+_T(" ")
		+Ssemihidden
		+_T(" ")
		+STsDefTbl
		+_T('}');	// closing brace

	wxString STsTblGrd = _T("{")		//opening brace
		+STsTag+STsNumTblGrd
		+_T("\\tsrowd\\tsvertalt ") // \tsvertalt = "top vertical alignment of cell
		+SParaAlignNrm+Sleftindent+Srightindent
		+Swidowcontrol+Sautospacingalpha
		+Sautospacingnum+SfontalignNrm+SadjustrightNrm
		+SrindentNrm+SlindentNrm+Sparnestlvl
		+_T(" ")
		+_T("\\f")+FNumNrm
		+SfontsizeNrm
		+_T(" \\sbasedon")+STsNumTblNrm
		+_T(" ")
		+_T("\\snext")+STsNumTblGrd
		+_T(" ")
		+STsTGrd
		+_T('}');	// closing brace

	wxString PardPlain = _T("\\pard\\plain") + gpApp->m_eolStr;

	// Build the DOCUMENT tags
	// combine document parameter tags with other doc level tags
	
	// whm added 26Oct07
	// Since different versions of Word (especially Word 2003) are sensitive to whether or not the
	// \fetN control word is used, and what value N has. The RTF Specs all say about \fetN:
	// "Footnote/endnote type. This indicates what type of notes are present in the document.
	//   0	Footnotes only or nothing at all (the default)
	//   1	Endnotes only
	//   2	Both footnotes and endnotes
	// For backward compatibility, if \fet1 is emitted, \endnotes or \enddoc will be emitted 
	// along with \aendnotes or \aenddoc. RTF readers that understand \fet will need to 
	// ignore the footnote-positioning control words and use the endnote control words instead."
	//
	// Note: When we scanned all the pSrcPhrase->m_markers members up near the beginning of
	// DoExportInterlinearRTF, we determined if there were any textual footnotes (or endnotes)
	// within the document and set bDocHasFootnotes (or bDocHasEndnotes) accordingly.
	// Now check if textual footnotes (which were indicated by \f in the text) are
	// to be explicitly removed from the output by the export options filter. If so, we can
	// at this point force the bDocHasFootnotes flag to FALSE (unless other things get formatted
	// as footnotes - see below).
	if (!MarkerIsToBeFilteredFromOutput(_T("f")))
	{
		bDocHasFootnotes = FALSE;
	}
	// Now we must check if textual endtnotes (which were indicated by \fe in the text) are
	// to be explicitly removed from the output by the export options filter. If so, we can
	// at this point force the bDocHasEndnotes flag to FALSE.
	if (!MarkerIsToBeFilteredFromOutput(_T("fe")))
	{
		bDocHasEndnotes = FALSE;
	}
	// Lastly, as far as the RTF \fetN control word goes, if any Free Translations, 
	// Back Translations, and/or AI Notes are to be formatted as footnotes in the final
	// exported RTF document, we need to finally force the bDocHasFootnotes flag back to 
	// TRUE, even if the user had no actual textual footnotes in the document, or has
	// optionally filtered footnotes from output using the ExportOptionsDlg.
	// 
	// Check if Free Translation, Back Translations or AI Notes are to be formatted as 
	// footnotes. When set to FALSE they would be formatted as footnotes
	// The extern globals available from the ExportOptionsDlg are (with their initial defaults):
	//bPlaceFreeTransInRTFText = TRUE; // changed to TRUE; in v 3.0.1 after table row code added
	//bPlaceBackTransInRTFText = FALSE;
	//bPlaceAINotesInRTFText = FALSE;
	// Here is the logic for the final test:
	// If any one of the above are FALSE, AND their markers actually exist in the document, 
	// AND they are not eliminated in the OptionsDlg filter, we will have footnotes in the 
	// actual final output.

	if (   (!bPlaceFreeTransInRTFText && bDocHasFreeTrans && !MarkerIsToBeFilteredFromOutput(_T("free"))) 
		|| (!bPlaceBackTransInRTFText && bDocHasBackTrans && !MarkerIsToBeFilteredFromOutput(_T("bt")))
		|| (!bPlaceAINotesInRTFText   && bDocHasAINotes   && !MarkerIsToBeFilteredFromOutput(_T("note"))) )
		bDocHasFootnotes = TRUE;

	// The following code should at least make the value of N conform to the RTF specifications, 
	// and hopefully avoid some of the situations where a particular version of Word is unable 
	// to open the RTF file. See the options for setting N of the \fetN control word in comments above.
	wxString fetNstr;
	if ((!bDocHasFootnotes && !bDocHasEndnotes) || (bDocHasFootnotes &&  !bDocHasEndnotes)) // has Footnotes or neither
		fetNstr = _T("\\fet0");
	else if (!bDocHasFootnotes && bDocHasEndnotes) // has Endnotes only
		fetNstr = _T("\\fet1");
	else
		fetNstr = _T("\\fet2"); // has both Footnotes and Endnotes


	// Now, build the DocTags string
	wxString DocTags = PageWidth+PageLength+MarginTop+MarginBottom+MarginLeft+MarginRight
					+ gpApp->m_eolStr
					+ _T("\\horzdoc\\viewkind1\\viewscale100\\nolnhtadjtbl")
					+ fetNstr  // whm modified 26Oct07
					+ _T("\\sectd ")
					+Landscape;
		// \horzdoc = horizontal rendering (as opposed to vertical rendering \vertdoc)
		// \viewkindN = view mode of the doc 0 = none; 1= Page Layout view; 2 = Outline view;
		//				3 = Master Document view; 4 = Normal view; 5 = Online Layout view
		//              We specify Page Layout view because balloon comments for \note won't
		//				appear in other views.
		// \viewscaleN = Zoom level of the document; N=percentage (default=100)
		// \noInhtadjtbl = don't adjust line height in table
		// \fet1  - added for version 3 (see following note)
		// NOTE: I spent several days trying to track down why Word would crash when opening
		// an .rtf file in which both free translation and back translation are formatted into
		// separate rows in the tables, and when \note material is also present and formatted as
		// footnotes. It turns out that the addition of \fet1 will prevent Word from crashing (neither
		// Wordpad nor Wordperfect would crash when reading the output). The reasons why adding
		// \fet1 prevents the crash are incomprehensible. What the RTF specification says is given
		// below.
		// The \fetN RTF tag is supposed to indicate what type of notes are present in the document.
		// According to the docs
		// -------------------------------------------------------------------------------------------
		// \fetN Footnote/endnote type. This indicates what type of notes are present in the document.
		//    0 Footnotes only or nothing at all (the default)
		//    1 Endnotes only
		//    2 Both footnotes and endnotes
		// For backward compatibility, if \fet1 is emitted, \endnotes or \enddoc will be emitted
		// along with \aendnotes or \aenddoc. RTF readers that understand \fet will not need to ignore
		// the footnote-positioning control words and use the endnote control words instead.
		// -------------------------------------------------------------------------------------------
		// I cannot make any sense of the above documentation for \fetN! Not only that, but when \fet1
		// is added to the RTF output, Word 2002 does not format the notes as endnotes at all, but rather
		// formats them as footnotes at the bottom of the page! Aaaaargh!
		// I've since noticed that Paratext also emits \fet1 routinely, probably because of the same
		// phenomenon.
		// whm Oct2007 update: To make matters worse, Word 2003 seems to crash on some documents
		// with \fet1 or omiting \fetN, but doesn't crash with \fet0. What to do?!
		// \sectd = reset to default section properties

	//////////////////// HEADER and FOOTER Definition Tags //////////////
	wxString BookName = exportFilename;
	int flen = BookName.Length();
	BookName.Remove(flen-4,4); // remove the .adt or .xml extension (including the .)
	wxString ChVsRange = _T("");
	if (!bOutputAll)
	{
		if (bOutputCVRange)
		{
			ChVsRange = _T("(");
			ChVsRange << nChFirst;
			ChVsRange << _T(":");
			ChVsRange << nVsFirst;
			ChVsRange << _T(" - ");
			ChVsRange << nChLast;
			ChVsRange << _T(":");
			ChVsRange << nVsLast;
			ChVsRange << _T(" Only)");
		}
		else if (bOutputPrelim)
		{
			ChVsRange = _T("(Preliminary Material Only)");
		}
		else if (bOutputFinal)
		{
			ChVsRange = _T("(Final Material Only)");
		}
	}


	wxString FtrLangNames = gpApp->m_curProjectName;// whm changed 19Jun06 to use App's m_curProjectName instead of m_targetName

	wxString HeaderTags = _T("{\\header \\pard\\plain ")
		+ gpApp->m_eolStr
		+ SindocHdrFtr
		+ gpApp->m_eolStr
		+ _T("{\\tab ")
		+GetANSIorUnicodeRTFCharsFromString(FtrLangNames)
		+_T(" - ")
		+ GetANSIorUnicodeRTFCharsFromString(BookName)
		+_T(" ")
		+ChVsRange
		+_T("\\par }}");

	wxString DocName = exportFilename;



	// Note: The original MFC version (before 3.5.2) used CTime::GetLocalTm which changed in
	// behavior and required rewriting to compile and run on VS 2005. The rewrite there was
	// similar to the wx code below. (see note below).
	wxDateTime theTime = wxDateTime::Now();
	wxString DateTime = theTime.Format(_T("%a, %b %d, %H:%M, %Y")).c_str();
	// Note: wxDateTime::Format could simply use the ASNI C strftime %c conversion specifier which 
	// is shown commented out below. Doing so would give the preferred date and time representation for 
	// the current locale. The format actually used above mimics that used in the MFC version.
	//wxString DateTime = theTime.Format(_T("%c")).c_str();

	wxString FooterTags = _T("{\\footer \\pard\\plain ")
		+ gpApp->m_eolStr
		+ SindocHdrFtr
		+ gpApp->m_eolStr
		+ _T("{")
		+ GetANSIorUnicodeRTFCharsFromString(DocName)
		+ _T(" \\tab}")
		+ gpApp->m_eolStr
		+ _T("{\\field {\\*\\fldinst {PAGE}} {\\fldrslt{}}}")
		+ gpApp->m_eolStr
		+ _T("{\\tab ")+DateTime+_T(" \\par}}");

	//////////////////// TABLE/ROW Definition Tags //////////////
	// Now Build the Table/Row Definition strings
	// See the /// RTF TABLE Structure/// comments near the beginning of this function for
	// more information on the overall table structure.

	// Row definition tags
	wxString Trowd = _T("\\trowd");
	wxString Tirow = _T("\\irow");
	wxString Tirowband = _T("\\irowband");
	wxString TRowNum;							// Number of current row, increments for each row
	wxString TtsN = _T("\\ts")+STsNumTblGrd;		// Reference to table style \ts21 - used in table/row def
	int ngaphNum = 40;							// Half the gap between row cells in twips
	wxString TgaphNum = _T("40");				// N for \trgaphN (keep this separate as it may be used
												// in other tags)
	wxString TtrgaphN = _T("\\trgaph")+TgaphNum;	// Half the horizontal gap size between cells in twips
	wxString TindentNum = _T("0");				// N for \trleftN N indicates the displacement of the
												// left side of the table from the left margin (zero
												// is on the margin, neg value is left of it, and a
												// positive value indents the whole table row to the
												// right - in twips. (keep this separate as it may be
												// handy when formatting RTL rows)
	wxString TStartPos;							// Position in twips of the starting position of the table
												// with respect to the edge of its column. \trleft0
	wxString Tjust = _T("");					// The table justification/centering RTF tag: \trqc or ""
												// Tjust is only used to control whether table is centered or not
	wxString TRowPrecedence;					// The cells within the row precedence tag: \rtlrow or \ltrrow
	wxString TDirection;						// Table direction: \taprtl when bReverseLayout else null string
	if (bReverseLayout)
	{
		TStartPos = _T("");						// There is no \trright0 tag in RTF specs
		Tjust = _T("\\trqr");					// Right justify the table row with respect to its 
												// containing column (containing column is the page
												// between the margins in our case)
		TRowPrecedence = _T("\\rtlrow");		// Cells in the table row will have RTL precedence
												// With this tag, we don't have to reorder the row text
												// in our RTF output to get RTL precedence
		TDirection = _T("\\taprtl");			// \taprtl indicates that the table direction is right-to-left.
	}

	else
	{
		TStartPos = _T("\\trleft")+TindentNum;	// \trleft0
		Tjust = _T("\\trql");					// Left justify the table row with respect to its
												// containing column
		TRowPrecedence = _T("\\ltrrow");		// Cells in the table row will have LTR precedence
												// With this tag, the row text order is interpreted as normal
												// LTR in the output and gets the default LTR precedence
		TDirection = _T("");					// There is no \tapltr tag so assign null string to TDirection
	}

	// whm added 11Nov07 to help prevent text in cells from wrapping excessively in Word 2003
	wxString TRautoFit = _T("\\trautofit1");

	wxString Tcellx = _T("\\cellx");				// Tag for \cellxN which defines the right boundary of a
												// table cell, including its half of the space between
												// cells. N is calculated on the fly from the text
												// metrics of the text going into the cell and converted
												// to the string that will be appended to each \cellx in
												// the cell definitions
	wxString TCellNBT;							// Holds N for the current \cellxN of any Back Translation
	wxString Trow = _T("\\row");					// Used to denote the end of a row. (not used in the
												// prefixed table/row defs of the first row)
	wxString Tcell = _T("\\cell ");				// Used to denote the end of a table cell. It is embedded
												// as delimiters in the actual text of the row, i.e.,
												// {This is\cell some\cell Source\cell Language\cell text.\cell }

	// 
	// We now have all the info to output the entire RTF Header for Interlinear
	// RTF output. Most of this header is similar to the non-interlinear
	// RTF output of the Source and/or Translation drafts.
	// Build the header string
	hstr = _T("{") // The whole RTF file must be enclosed in {}
			+Hrtf1+Hcharset+Hcodepg+HUnicodeNumBytes+Hdeffont+gpApp->m_eolStr //_T("\n") // the basic header line
			+_T("{")			// opening brace of the font table
			+_T("\\fonttbl")
			+ gpApp->m_eolStr
			+FTagsNrm+gpApp->m_eolStr 	// Normal font tags
			+FTagsSrc+gpApp->m_eolStr 	// Source	"	"
			+FTagsTgt+gpApp->m_eolStr 	// Target	"	"
			+FTagsGls+gpApp->m_eolStr 	// Gloss	"	"
			+FTagsNav+gpApp->m_eolStr 	// Navigation	"	"
			+_T('}')			// closing brace of the font table
			+ColorTable			// the color table
			+_T("{")			// opening brace of the stylesheet
			+_T("\\stylesheet")
			+ gpApp->m_eolStr
			+Sdef__normal+gpApp->m_eolStr 	// Normal text paragraph style definition tags
			+SdefSrc+gpApp->m_eolStr		// Source language paragraph style definition tags
			+SdefTgt+gpApp->m_eolStr		// Target language	"	"
			+SdefGls+gpApp->m_eolStr		// Gloss language	"	"
			+SdefNav+gpApp->m_eolStr		// Navigation text  "	"
			+SdefHdrFtr+gpApp->m_eolStr		// Header-footer text "  "
			+SdefFree+gpApp->m_eolStr		// Free Translation paragraph style definition tags
			+SdefBT+gpApp->m_eolStr			// Back Translation paragraph style definition tags
			+STsTblNrm+gpApp->m_eolStr 			// Normal Table Char character style definition tags
			+STsTblGrd+gpApp->m_eolStr 			// Table Grid character style definition tags
			+SdefFnCaller+gpApp->m_eolStr 		// Footnote Caller character style definition tags
			+SdefFnText+gpApp->m_eolStr 		// Footnote Text paragraph style " "
			+SdefAnnotRef+gpApp->m_eolStr		// Annotation Reference character style " "
			+SdefAnnotText+gpApp->m_eolStr		// Annotation Text paragraph style " "
			+Sdef_notes_base+gpApp->m_eolStr	// _notes_base paragraph base style " "
			+Sdef_vernacular_base+gpApp->m_eolStr 	// _vernacular_base paragraph base style " "
			+_T('}')			// closing brace of the stylesheet
			+DocTags+gpApp->m_eolStr 	// The document tags
			+HeaderTags+gpApp->m_eolStr // The header tags
			+FooterTags+gpApp->m_eolStr // The footer tags
			+PardPlain;			// This ends the RTF header tags. Now we can
								// build strings of tags to output our RTF tables!!

	// First output to RTF file here!
	// Note: If there is a write error in WriteOutputString() it must go to writeErrExit point
	// to delete font objects and prevent memory leaks
	if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
	{	
		pProgDlg->Destroy();
		return;
	}

	int WidthSrc;				// holds width of Src string as returned by GetTextExtent + ngaphNum * 2
	int WidthTgt;				// holds width of Tgt string as returned by GetTextExtent + ngaphNum * 2
	int WidthGls;				// holds width of Gls string as returned by GetTextExtent + ngaphNum * 2
	int WidthNav;				// holds width of Nav string as returned by GetTextExtent + ngaphNum * 2
	// next two for version 3
	int WidthFreeTrans;			// holds width of Free translation string " " added for v 3
	int WidthBackTrans;			// holds width of Back translation string " " added for v 3
	int MaxColWidth = 0;		// the maximum width needed for the column
	int AccumRowWidth = 0;		// accumulated width of the row

	wxSize Srcextent;			// size of the Src text
	wxSize Tgtextent;			// size of the Tgt text
	wxSize Glsextent;			// size of the Gls text
	wxSize Navextent;			// size of the Nav text
	// next two for version 3
	wxSize FreeTransextent;		// size of the Free trans text
	wxSize BackTransextent;		// size of the Back trans text

	// Note: for the following 4 strings we accumulate all the row text concatenated together without
	// spaces between the words contained in adjacent source phrases. This enables us to get a text
	// metric for the total length of the text that will occupy the table cells of a row (we add the
	// intercellular gap and a fudge factor to calculate the actual length of the row.
	wxString SrcTextStr = _T("");	// holds actual text of Src lang row in table (spaces removed)
	wxString TgtTextStr = _T("");	// holds actual text of Tgt lang row in table (spaces removed)
	wxString GlsTextStr = _T("");	// holds actual text of Gls lang row in table (spaces removed)
	wxString NavTextStr = _T("");	// holds actual text of Nav lang row in table (spaces removed)

	wxString EndLine = gpApp->m_eolStr;
	int nColsInRow = 0;				// the number of columns/cells in the Src, Tgt, Gls or Nav row
	// next two for version 3
	//int nColsInFreeTransRow = 0;//unused 	// the number of columns/cells in the Free trans row
									// this can differ from nColsInRow above
	//int nColsInBackTransRow = 0;//unused	// the number of columns/cells in the Back trans row
									// this can differ from nColsInRow above

	int nNumRowsInTable = 0;		// nNumRowsInTable is up to 6 if output all table rows
	if (bInclSrcLangRow) nNumRowsInTable++;
	if (bInclTgtLangRow) nNumRowsInTable++;
	if (bInclGlsLangRow) nNumRowsInTable++;
	if (bInclNavLangRow) nNumRowsInTable++;
	// next two for version 3
	if (bInclFreeTransRow) nNumRowsInTable++;
	if (bInclBackTransRow) nNumRowsInTable++;

	int nCurrentRow = 0;					// current row being processed
	bool OutputLastCols = FALSE;			// keeps track of whether we've output all columns
											// once we've read all SourcePhrases
	// the next 7 were all CLists in MFC
	wxArrayString cellxNList;		// a list to store \cellxN N values for the Src, Tgt, Gls
									// and Nav rows of the table
	wxArrayString cellxNListFree;	// a list to store \cellxN N values for the Free Trans
									// row of the table
	wxArrayString cellxNListBT;		// a list to store \cellxN N values for the Back Trans
									// row of the table
	// The data in the following 6 lists is composed of pointers to wxStrings created on the heap
	// Therefore we don't call Clear() to empty the lists, but rather we call DeleteContents(TRUE)
	// which instructs the list to call delete on the client contents.
	wxArrayString srcList;
	srcList.Alloc(gpApp->m_pSourcePhrases->GetCount()); // a list to store Source phrases composing row
	wxArrayString tgtList;
	tgtList.Alloc(gpApp->m_pSourcePhrases->GetCount()); // a list to store Target phrases composing row
	wxArrayString glsList;
	glsList.Alloc(gpApp->m_pSourcePhrases->GetCount()); // a list to store Gloss phrases composing row
	wxArrayString navList;
	navList.Alloc(gpApp->m_pSourcePhrases->GetCount()); // a list to store Navigation text phrases composing row
	// next two added for version 3
	// no need to call .Alloc for these two:
	wxArrayString freeTransList; // a list to store Free trans text phrases composing row
	wxArrayString backTransList; // a list to store Back trans text phrases composing row

	wxString FreeTransFitsInRowStr;			// a string to store Free translation being poured into a
											// row
	wxString FreeTransSpillOverStr;			// a string to take any text from FreeTransRowStr that will
											// not fit in current table's Free Trans row.
	wxString BackTransFitsInRowStr;			// a string to store Back translation being poured into a
											// row
	wxString BackTransSpillOverStr;			// a string to take any text from BackTransFitsInRowStr that will
											// not fit in current table's Back Trans row.
	wxString SrcStr;						// Source phrase from pList
	wxString TgtStr;						// Target phrase from pList
	wxString GlsStr;						// Gls phrase from pList
	wxString NavStr;						// Navigation text (from m_inform + m_ChapterVerse)
	// next two for version 3
	wxString FreeTStr;						// Free trans text from text assoc with \free
	wxString BackTStr;						// Back trans text from text assoc with \bt...
	wxString CellDimsSrc = _T("");			// holds output string of \cellx lines for Src text
	wxString CellDimsTgt = _T("");			// holds output string of \cellx lines for Tgt text
	wxString CellDimsGls = _T("");			// holds output string of \cellx lines for Gls text
	wxString CellDimsNav = _T("");			// holds output string of \cellx lines for Nav text
	// next two for version 3
	wxString cellDimsFree = _T("");			// holds output string of \cellx lines for Free trans text
	wxString cellDimsBack = _T("");			// holds output string of \cellx lines for Back trans text

	bool bInPreliminaryMaterial = TRUE;	// TRUE until first \c or \v then FALSE
	bool bSkip = TRUE;
	// initial settings for the bSkip flag are TRUE for output of "All", "Chapter/Verse Range"
	// and "Final" material.
	// However, for "Preliminary" material the initial bSkip setting should be FALSE
	if (bOutputPrelim)
	{
		bSkip = FALSE;
	}

	// since a given scripture file will usually at least have and \id line
	// there should always be at least that much Preliminary Material in an
	// output file. In the case of Chapter/Verse Ranges and Final Material,
	// there is more of a chance nothing could be output. If no significant output
	// is produced, we should notify the user at some point not to expect anything
	// in the RTF output file.
	bool bLimitedOutputFound = FALSE;	// if not bOutputAll and this is still FALSE
										// at the end of output we need to notify
										// user with a message saying couldn't find
										// the range, prelim material or final material

	// these are used in AnalyseReference in the bOutputFinal routine below
	int LMchapter = 0;
	int LMvfirst = 0;	// not used except as parameter to AnalyseReference
	int LMvlast = 0;
	int CVchapter = 0;
	int CVvfirst = 0;	// not used except as parameter to AnalyseReference
	int CVvlast = 0;

	 
	// These are used in while loop below to control output filtering
	bool bCurrentlyFiltering = FALSE;
	bool bHasNoteMarker = FALSE;
	bool bHasFreeMarker = FALSE;
	bool bHasBTMarker = FALSE;
	bool bHasInputFilteredMaterial = FALSE;
	wxString assocNoteMarkerText;
	wxString assocFreeMarkerText;
	wxString assocBTMarkerText;
	wxArrayString MkrList;
	//wxArrayString* pMkrList = &MkrList; // unused
	wxString mkrStr;
	wxString bareMarker;
	wxString bareMarkerForLookup;
	wxString bareBTMarker;
	wxString wholeMarker;
	wxString wholeMarkerLessAsterisk;
	wxString lastWholeMarkerTakingAnEndMarker;
	wxString wantedEndMkr;

	wxString MiscRTF; // use for temp storage of RTF tags until output
	int noteRefNumInt = 0;
	int freeRefNumInt = 0;
	int btRefNumInt = 0;
	wxString noteRefNumStr; // used as N with \*\atnref N to refer to annotation
	wxString freeRefNumStr;
	wxString btRefNumStr;
	wxString callerStr;

	wxString CellxNum;
	CellxNum.Empty();

	// START of MAIN LOOP SCANNING THROUGH ALL SOURCE PHRASES
	// start at beginning of pList of SourcePhrases
	pos = pList->GetFirst(); //pos = pList->GetHeadPosition();
	wxASSERT(pos != NULL);
	while (pos != NULL) // cycle through all SourcePhrases
	{
		counter++;
		if (counter % 200 == 0)
		{
			msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),counter,nTotal);
			pProgDlg->Update(counter,msgDisplayed);
			//::wxSafeYield();
		}

		//savePos = pos;

		// reset
		SrcStr.Empty();
		TgtStr.Empty();
		GlsStr.Empty();
		NavStr.Empty();
		FreeTStr.Empty();
		BackTStr.Empty();
		bHasNoteMarker = FALSE;
		bHasFreeMarker = FALSE;
		bHasBTMarker = FALSE;
		assocNoteMarkerText.Empty();
		assocFreeMarkerText.Empty();
		assocBTMarkerText.Empty();

		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase != 0);

		// handle output of Prelim material only, Ch:Vs range, & final material only cases
		if (!bOutputAll)
		{
			// if output of preliminary material
			// this routine determines when to skip text after preliminary material is output
			if (bOutputPrelim)
			{
				// bInPreliminaryMaterial is TRUE at entry to routine
				// bSkip is FALSE at entry to routine, hence, once
				// we encounter either a chapter marker or verse marker, the
				// InPreliminaryMaterial flag becomes false bSkip becomes true
				// and the routine skips all material after that point
				if (!bInPreliminaryMaterial)
				{
					// skip material that is not preliminary
					bSkip = TRUE; // this is not needed here but doesn't hurt
					goto c;	// no need to process any more source phrases so jump to c:
							// rather than skipping through the remainder of the text
				}
				if(pSrcPhrase->m_chapterVerse != _T("")
					&& (pSrcPhrase->m_bChapter || pSrcPhrase->m_bVerse))

				{
					// we've encountered either a chapter marker or verse marker
					// so we're no longer in preliminaty material
					bInPreliminaryMaterial = FALSE;
				}
			}
			// if output ch/vs range this routine determines when to skip chapter/verse
			// range before and after the desired range is output
			else if (bOutputCVRange)
			{
				// need to check only at chapter and verse boundaries
				if(pSrcPhrase->m_chapterVerse != _T("")
					&& (pSrcPhrase->m_bChapter || pSrcPhrase->m_bVerse))
				{
					bInPreliminaryMaterial = FALSE;
					// we want to bypass all phrases before the desired
					// starting point, and all phrases that occur after
					// the desired ending point
					// Analyse the m_ChapterVerse member
					// BW added extra parameter Oct 2004, set it  > 0 so it has no effect on Bill's code here
					if (pView->AnalyseReference(pSrcPhrase->m_chapterVerse,CVchapter,CVvfirst,CVvlast,1))
					{
						; // do nothing just collect chapter and verse info
					}
				}// end of if at a chapter or verse marker
				// if skipping, include phrases within desired range
				if ((bSkip == TRUE) && (CVchapter == nChFirst) && (CVvfirst >= nVsFirst))
				{
					bSkip = FALSE;
					bLimitedOutputFound = TRUE; // we have something to output
				}
				// if including, skip phrases not within desired range
				if (nChLast < nActualChLast || (nChLast == nActualChLast && nVsLast < nActualVsLast))
				{
					// selected range ends before last verse in actual text
					if ((bSkip == FALSE) && (CVchapter == nChLast) && (CVvlast > nVsLast))
					{
						bSkip = TRUE;
						goto c;	// no need to process any more source phrases so jump to c:
								// rather than skipping through the remainder of the text
					}
				}
				else
				{
					// selected range end coincides with actual end or exceeds it, so
					// we want to complete our output at end of ch/vs material at the point
					// where any Final material starts (stopping at the exact place where
					// bOutputFinal would start - see below)
					if ((CVchapter == nChLast) && (CVvlast >= nVsLast))
					{
						// if we get here we're in potential Final Material territory (at
						// start of last verse), so we'll start looking for a sf marker
						// that will confirm we have reached that point
						//Mkr = pSrcPhrase->m_markers; BEW 12Apr10 added extra member's
						//contents, but not m_freeTrans, m_collectedBackTrans and m_note
						//because these are not relevant here because they don't signal
						//reaching the start of Final Material
						Mkr = pSrcPhrase->GetFilteredInfo() + pSrcPhrase->m_markers;
						// For version 3.x we add special markers that explicitly signal final territory
						// TODO: consider adding an attribute to AI_USFM.xml that would eliminate having
						// to hard code these markers.
						if (   Mkr.Find(_T("\\conc")) != -1 || Mkr.Find(_T("\\glo")) != -1 || Mkr.Find(_T("\\idx")) != -1
							|| Mkr.Find(_T("\\maps")) != -1 || Mkr.Find(_T("\\id BAK")) != -1 || Mkr.Find(_T("\\cov")) != -1
							|| Mkr.Find(_T("\\spine")) != -1
							// above are the USFM ones, below may help determine end of range for PNG
							|| Mkr.Find(_T("\\pp")) != -1 || Mkr.Find(_T("\\h")) != -1 || Mkr.Find(_T("\\ps")) != -1
							|| Mkr.Find(_T("\\pt")) != -1 || Mkr.Find(_T("\\pm")) != -1 || Mkr.Find(_T("\\ip")) != -1
							|| Mkr.Find(_T("\\mt")) != -1 || Mkr.Find(_T("\\is")) != -1 || Mkr.Find(_T("\\im")) != -1
							|| Mkr.Find(_T("\\st")) != -1 || Mkr.Find(_T("\\iq")) != -1 || Mkr.Find(_T("\\iq2")) != -1
							|| Mkr.Find(_T("\\io1")) != -1 || Mkr.Find(_T("\\io2")) != -1|| Mkr.Find(_T("\\io3")) != -1
							|| Mkr.Find(_T("\\div")) != -1 || Mkr.Find(_T("\\dvrf")) != -1 || Mkr.Find(_T("\\s")) != -1
							|| Mkr.Find(_T("\\pq")) != -1 || Mkr.Find(_T("\\r")) != -1)
						{
							bSkip = TRUE;
							goto c;	// no need to process any more source phrases so jump to c:
									// rather than skipping through the remainder of the text
						}
					}
				}
			}// end of if (bOutputCVRange)
			// if output of final material
			// this routine determines when to skip text before final material is encountered/output
			else if (bOutputFinal)
			{
				// Our initial scan before presenting the Export Interlinear Dialog told us what the
				// last chapter and verse numbers were (nChLast and nVsLast). We can safely skip
				// everything at least as far as this point as determined by AnalyseReference. Then
				// we'll need to monitor the content of source phrases after that point until we
				// encounter something which is recognizable as final material.
				//if(pSrcPhrase->m_bChapter || pSrcPhrase->m_bVerse)
				if(pSrcPhrase->m_chapterVerse != _T("")
					&& (pSrcPhrase->m_bChapter || pSrcPhrase->m_bVerse))
				{
					// BW added extra parameter Oct 2004, set it  > 0 so it has no effect on Bill's code here
					if (pView->AnalyseReference(pSrcPhrase->m_chapterVerse,LMchapter,LMvfirst,LMvlast,1))
					{
						; // do nothing just collect chapter and verse info
					}
				}// end of if at a chapter or verse marker

				// we're skipping until we encounter nChLast and nVsLast
				if ((LMchapter == nChLast) && (LMvlast >= nVsLast))
				{
					// if we get here were in potential Final Material territory (at start
					// of last verse), so we'll start looking for a sf marker that will
					// confirm we have reached that point
					//Mkr = pSrcPhrase->m_markers; BEW 12Apr10 added extra member's
					//contents, but not m_freeTrans, m_collectedBackTrans and m_note
					//because these are not relevant here because they don't signal
					//reaching the start of Final Material
						Mkr = pSrcPhrase->GetFilteredInfo() + pSrcPhrase->m_markers;
					// For version 3.x we add special markers that explicitly signal final terrirory
					// TODO: consider adding an attribute to AI_USFM.xml that would eliminate having
					// to hard code these markers.
					if (   Mkr.Find(_T("\\conc")) != -1 || Mkr.Find(_T("\\glo")) != -1 || Mkr.Find(_T("\\idx")) != -1
						|| Mkr.Find(_T("\\maps")) != -1 || Mkr.Find(_T("\\id BAK")) != -1 || Mkr.Find(_T("\\cov")) != -1
						|| Mkr.Find(_T("\\spine")) != -1
						// above are the USFM ones, below may help determine end of range for PNG
						|| Mkr.Find(_T("\\pp")) != -1 || Mkr.Find(_T("\\h")) != -1 || Mkr.Find(_T("\\ps")) != -1
						|| Mkr.Find(_T("\\pt")) != -1 || Mkr.Find(_T("\\pm")) != -1 || Mkr.Find(_T("\\ip")) != -1
						|| Mkr.Find(_T("\\mt")) != -1 || Mkr.Find(_T("\\is")) != -1 || Mkr.Find(_T("\\im")) != -1
						|| Mkr.Find(_T("\\st")) != -1 || Mkr.Find(_T("\\iq")) != -1 || Mkr.Find(_T("\\iq2")) != -1
						|| Mkr.Find(_T("\\io1")) != -1 || Mkr.Find(_T("\\io2")) != -1|| Mkr.Find(_T("\\io3")) != -1
						|| Mkr.Find(_T("\\div")) != -1 || Mkr.Find(_T("\\dvrf")) != -1 || Mkr.Find(_T("\\s")) != -1
						|| Mkr.Find(_T("\\pq")) != -1 || Mkr.Find(_T("\\r")) != -1)
					{
						bSkip = FALSE;
						bLimitedOutputFound = TRUE; // we have something to output
					}
				}
			}// end of else if (bOutputFinal)

			if (bSkip)
			continue;
		}// end of if (!bOutputAll)

		// whm 27Nov05 Note: The Interlinear Export does not really need to worry about the
		// placement of markers within a Retranslation, since it basically strips out all markers
		// as far as the target text is concerned.
		// 
		// BEW 12Apr10 - the above comment is inaccurate. Retranslation stores the marker
		// information, but it has no need to assign markers to specific locations within
		// the target text until that text is exported (and that may cause a Place Markers
		// dialog to be displayed). Not having studied Bill's implementation of RTF
		// before, the above comment therefore suggests that if retranslation-medial
		// markers exist, they would be not included in the RTF interlinear output. If
		// true, while this would result in loss of some possibly non-essential info, such
		// a loss is probably a small price to pay for the greater good of not
		// interrupting the RTF construction process in order to ask the user to manually
		// place markers. So I'm leaving Bill's code unchanged in this matter.
		// 
		// We treat the contents of m_markers separately, especially with regard to the
		// formatting of \note, \free and \bt material which are handled below.
		// The version of the code below for constructing the SrcStr, TgtStr, GlsStr and NavStr was
		// patterned after the code in RebuildTargetText()

		// get the cell texts for this srcPhrase
		SrcStr = pSrcPhrase->m_srcPhrase;
		TgtStr  = pSrcPhrase->m_targetStr;	// this could possibly be empty
		GlsStr = pSrcPhrase->m_gloss;
		// NavStr is built below
		// FreeTStr and BackTStr are also handled below

		// Our RTF output tables do not need to handle any stnd format markers.
		// Instead, our Nav text row displays the m_inform + m_ChapterVerse
		// information
		bHasInputFilteredMaterial = FALSE;
		//if (!pSrcPhrase->m_markers.IsEmpty())
		if (!pSrcPhrase->m_markers.IsEmpty() || HasFilteredInfo(pSrcPhrase))
		{
			wxString realMkr;
			// BEW 12Apr10, Bill's comment above about not needing to handle any stnd
			// format markers should mean that, for the loop, doc version 5 having possibly
			// some endmarkers in the m_endMarkers member won't matter, and we can just
			// ignore them - that's what I'll do here. The helper.cpp function,
			// GetFilteredStuffAsUnfiltered(), returns (for doc version 5) a string
			// functionally equivalent to what would be in m_markers when there is stored
			// filtered information, but with filter bracket markers already removed,
			// (m_markers with initial endmarkers would be on a later CSourcePhrase
			// instance that almost certainly would lack any filtered content, and
			// presumably those markers would be omitted from the RTF anyway, and it is
			// only those which end up in m_endMarkers in doc version 5 anyway, as
			// endmarkers for filtered content will be in m_filteredInfo and so will appear
			// in the returned string from the GetFilteredStuffAsUnfiltered() call. So,
			// I'll proceed on the assumption that the function call will be nearly all we need
			// here and make tempStr have just m_endMarkers if there is no other filtered stuff.
			//wxString tempStr = pSrcPhrase->m_markers;

			// BEW 12Apr10, this section uses doc version 5 structures to advantage, if it
			// doesn't give the desired results, comment it out and uncomment out the
			// commented out section immediately above
			wxString tempStr;
			if (!pSrcPhrase->GetFilteredInfo().IsEmpty())
			{
				tempStr = pSrcPhrase->GetFilteredInfo();
				tempStr = pDoc->RemoveAnyFilterBracketsFromString(tempStr);
			}
			if (!pSrcPhrase->m_markers.IsEmpty())
			{
				tempStr += _T(" ") + pSrcPhrase->m_markers;
			}
			// we don't expect m_markers or m_filteredInfo non-empty on the same instance
			// with m_endMarkers non-empty; if that is not the case, just ignore
			// endmarkers in m_endMarkers member
			if (tempStr.IsEmpty() && !pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				tempStr = pSrcPhrase->GetEndMarkers();
			}
			// Except for any note text, free trans text, or bt text, and
			// except for anything that gets filtered by our export output filter,
			// we need to add the remaining filtered/hidden material to BOTH the
			// source phrase string SrcStr AND the target phrase string TgtStr, since by
			// design, the hidden elements are to propagate through from input source to
			// target output; and in our interlinear case we are displaying both the source
			// and the target on the same display.
			//			
			// Handle any note, back trans or free trans stuff. Rather than putting it into
			// the SrcStr and TgtStr, we'll set it aside in some temporary strings to be
			// formatted into the proper RTF form (comments, footnotes, or eventual extra
			// table row). We assume there is only one string of each, if at all, as that
			// is all that the doc version 5's design can support.
			if (!pSrcPhrase->GetNote().IsEmpty())
			{
				// ignore empty notes, support only those with text content
				assocNoteMarkerText = pSrcPhrase->GetNote();
				assocNoteMarkerText = _T("\\note ") + assocNoteMarkerText;
				if (!MarkerIsToBeFilteredFromOutput(_T("note")))
				{
					bHasNoteMarker = TRUE;
				}
				else
				{
					bHasNoteMarker = FALSE;
				}
			}
			if (!pSrcPhrase->GetFreeTrans().IsEmpty())
			{
				// ignore empty free translations, support only those with text content
				assocFreeMarkerText = pSrcPhrase->GetFreeTrans();
				assocFreeMarkerText = _T("\\free ") + assocFreeMarkerText;
				if (!MarkerIsToBeFilteredFromOutput(_T("free")))
				{
					bHasFreeMarker = TRUE;
				}
				else
				{
					bHasFreeMarker = FALSE;
				}
			}
			if (!pSrcPhrase->GetCollectedBackTrans().IsEmpty())
			{
				// support only our Adapt It \bt 'collected' information (any \bt-derived
				// markers and content can be handled with the m_filteredInfo content)
				assocBTMarkerText = pSrcPhrase->GetCollectedBackTrans();
				assocBTMarkerText = _T("\\bt ") + assocBTMarkerText;
				if (!MarkerIsToBeFilteredFromOutput(_T("bt")))
				{
					bHasBTMarker = TRUE;
				}
				else
				{
					bHasBTMarker = FALSE;
				}
			}			

			// At this point any \note, \free and \bt... material and associated text has
			// been removed from tempStr. What remains are various markers and text composed of:
			// 1. Isolated sfm end markers - normally at the beginning of m_markers
			// 2. Input filtered markers along with their entire associated text and any end marker
			// 3. Various single markers such as \p, \c n, \v n, and beginning markers for assoc
			//    text that will follow in subsequent pSrcPhrases.
			// By design, unless the output filter excludes it, the material that was input filtered
			// would have been copied now to the beginning of both the SrcStr and the TgtStr. But,
			// to make such filtered material more readable, we will set a flag that causes the
			// table building routines below to create an extra column in the table to place this
			// input filtered material (which will occupy both the source and target cells and have
			// "[FILTERED] " prefixed to the NavStr. That way it will not reside in the same column
			// as the source and target text that belongs to this current pSrcPhrase.

			// As an interim step I will not implement the insertion of this extra column of
			// cells into the output table. Instead, I'll just append the "[associated text] "
			// material to the beginning of the existing SrcStr and TgtStr strings, and "[FILTERED] "
			// to the NavStr string.

			// First we will examine all standard format markers and use them to set our output
			// filter flags; we will also enclose text associated with input filtered material
			// within square brackets to keep each of these associated text strings visibly grouped
			// together; then we will remove all the standard format markers since they should not
			// clutter up the interlinear display.
			// We process the remaining contents in a buffer

			// whm corrected 19Jun06 (removed nTheLen++)
			int nTheLen = tempStr.Length();
			// wx Note: pBuffer can be a read-only buffer
			const wxChar* pBuffer = tempStr.GetData();
			//int itemLen = 0; // unused
			wxChar* pBuffStart = (wxChar*)pBuffer;		// save start address of Buffer
			//wxChar* ptr = pBuffStart;	//unused		// point to start of text
			wxChar* pEnd = pBuffStart + nTheLen;// bound past which we must not go
			wxASSERT(*pEnd == (wxChar)0); // ensure there is a null at end of Buffer
			// Setup copy-to buffer textStr2. It needs to be twice the size of input buffer since
			// we will be adding a backslash for every control char we find
			wxString textStr2;
			//wxChar* pBuffer2;
			// wx Note: pBuffer2 must be a writeable buffer
			// whm 8Jun12 modified to use wxStringBuffer
			// Create the wxStringBuffer in a specially scoped block. This is crucial here
			// in this function since the wxString textStr2 is accessed directly within
			// this function (see about line 4691).
			{ // begin special scoped block
				wxStringBuffer pBuffer2(textStr2,nTheLen*2 + 1);
				//pBuffer2 = textStr2.GetWriteBuf(nTheLen*2 + 1); // buffer double size for safety even
													// though pBuffer2 should be same size or smaller
				int mkrLen = 0;
				wxString bareMkrForLookup;
				wxString wholeMkr;
				wxString wholeMkrNoAsteriskSp;
				wxChar* pOld = (wxChar*)pBuffer;  // source
				wxChar* pNew = pBuffer2; // destination

				// We cannot use ApplyOutputFilterToText() here in DoExportInterlinearRTF() because
				// ApplyOutputFilterToText is designed to work on a whole text that completely resides
				// in a wxString. Here we are dealing with the individual source phrases and markers
				// which are all located within the m_markers member of the first source phrase where
				// they are encountered.
				//
				// CODE BELOW FUNCTIONS AS OUTPUT FILTER FOR INTERLINEAR EXPORT
				//bool bHitMkr = FALSE;
				while (pOld < pEnd && *pOld != (wxChar)0)
				{
					if (IsMarkerRTF(pOld,pBuffStart))
					{
						// we are pointing at a marker
						// What kind of marker is it?
						// Get its USFMAnalysis struct
						bareMkrForLookup = pDoc->GetBareMarkerForLookup(pOld);
						wholeMkr = pDoc->GetWholeMarker(pOld);
						wholeMkrNoAsteriskSp = wholeMkr;
						wholeMkrNoAsteriskSp.Replace(_T("*"),_T(""));
						wholeMkrNoAsteriskSp += _T(' ');
						// if we have been filtering and this marker is not to be filtered
						// from output, we set bCurrentlyFiltering back to FALSE
						if (bCurrentlyFiltering
							&& !MarkerIsToBeFilteredFromOutput(bareMarkerForLookup)
							&& embeddedWholeMkrs.Find(wholeMkr + _T(' ')) == -1
							)
						{
							bCurrentlyFiltering = FALSE;
						}

						int nMkrLen = 0; // disregard nMkrLen here; used as dummy for IsVerseMarker

						// is it a chapter marker?
						if (pDoc->IsChapterMarker(pOld))
						{
							// we don't put the \c n in the text; NavStr handles it
							mkrLen = ParseMarkerRTF(pOld,pEnd);
							pOld += mkrLen; // point pOld past \c marker
							mkrLen = pDoc->ParseWhiteSpace(pOld);
							pOld += mkrLen;
							if (pOld >= pEnd)
								break;
							// should be a chapter number so parse it and following whitesp
							mkrLen = pDoc->ParseNumber(pOld);
							pOld += mkrLen; // point pOld past n
							if (pOld >= pEnd)
								break;
							mkrLen = pDoc->ParseWhiteSpace(pOld);
							pOld += mkrLen;
							if (pOld >= pEnd)
								break;
						}
						// is it a verse marker?
						else if (pDoc->IsVerseMarker(pOld,nMkrLen))
						{
							// we don't put the \v n in the text; NavStr handles it
							mkrLen = ParseMarkerRTF(pOld,pEnd);
							pOld += mkrLen; // point pOld past \v marker
							mkrLen = pDoc->ParseWhiteSpace(pOld);
							pOld += mkrLen;
							if (pOld >= pEnd)
								break;
							// should be a verse number so parse it and following whitesp
							mkrLen = pDoc->ParseNumber(pOld);
							pOld += mkrLen; // point pOld past n
							if (pOld >= pEnd)
								break;
							mkrLen = pDoc->ParseWhiteSpace(pOld);
							pOld += mkrLen;
							if (pOld >= pEnd)
								break;
						}
						// is it to be output filtered?
						else if (MarkerIsToBeFilteredFromOutput(bareMkrForLookup))
						{
							// it is one to be filtered from output - if it was input
							// filtered, the marker and its assoc string and any end marker
							// should reside wholly in m_markers.
							if (gpApp->gCurrentFilterMarkers.Find(wholeMkrNoAsteriskSp) != -1)
							{
								// it was input filtered and \~FILTER ... \~FILTER* brackets were
								// removed above. We can expect that the whole marker and assoc
								// text and any end marker is present so we can use the
								// ParseMarkerAndAnyAssociatedText
								int combinedLen = 0;
								combinedLen = ParseMarkerAndAnyAssociatedText(pOld,pBuffStart,pEnd,
									bareMkrForLookup,wholeMkr,TRUE,FALSE);	// TRUE is RTF aware
																			// FALSE don't include
																			// char format markers
								//the marker and assoc text and any end marker is to be filtered
								//from output, so we'll just move pOld and pNew past the whole thing
								//effectively filtering it out
								pOld += combinedLen;
								//pNew += combinedLen; // whm modified 27Dec07 to fix potential bug
							}
							else if (pDoc->IsEndMarker(pOld,pEnd))

							{
								// it's an end marker that is being filtered from output
								// if it is not an end marker of the embedded marker type
								// we stop filtering. If it is an end marker of the embedded
								// marker type we do nothing, because there will probably be
								// the parent end marker coming up soon (which will turn off
								// the filtering for the parent marker.
								if (embeddedWholeEndMkrs.Find(wholeMkr + _T(' ')) != -1)
								{
									// unilaterally stop output filtering
									bCurrentlyFiltering = FALSE;
								}
								// parse the marker and following space
								mkrLen = ParseMarkerRTF(pOld,pEnd);
								pOld += mkrLen;
								mkrLen = pDoc->ParseWhiteSpace(pOld);
								pOld += mkrLen;
								// omit marker itself from pNew
								//bHitMkr = TRUE;
								if (pOld >= pEnd)
									break;
							}
							else if (charFormatMkrs.Find(wholeMkrNoAsteriskSp) != -1)
							{
								// We are filtering one of the character markers that affects font
								// formatting. We don't want to filter out the associated text, only
								// remove the formatting, i.e., by remove the markers and leaving the
								// associated text.
								// We do not set bCurrentlyFiltering = TRUE here, but only parse the
								// marker and following space to effectively omit these formatting
								// markers from output
								mkrLen = ParseMarkerRTF(pOld,pEnd);
								pOld += mkrLen;
								mkrLen = pDoc->ParseWhiteSpace(pOld);
								pOld += mkrLen;
								// omit marker itself from pNew
								//bHitMkr = TRUE;
								if (pOld >= pEnd)
									break;
							}
							else
							{
								// the marker is NOT an end marker but should now be filtered
								// from output (because we are in the "to-be-filtered" block
								// If this marker is wholly contained in our current m_markers,
								// we can filter it and its associated text right here by
								// parsing the whole thing and not advancing the pNew in the
								// process. If this marker is not wholly contained here (or we
								// cannot determine that it is), we'll just set the flag for a
								// subsequent pSrcPhrase to filter it.
								bCurrentlyFiltering = TRUE;
								// parse the marker and following space
								mkrLen = ParseMarkerRTF(pOld,pEnd);
								pOld += mkrLen;
								mkrLen = pDoc->ParseWhiteSpace(pOld);
								pOld += mkrLen;
								// omit marker itself from pNew
								//bHitMkr = TRUE;
								if (pOld >= pEnd)
									break;
							}
						}
						else
						{
							// the marker is NOT one that is to be output filtered.

							// determine if it is a wrap marker which would start a new table
							// if user desires it to do so
							USFMAnalysis* pSfm = NULL;
							pSfm = pDoc->LookupSFM(bareMkrForLookup);
							if (pSfm != NULL)
							{
								bMarkerStartsNewLine = pSfm->wrap;
								bNextTableIsCentered = (pSfm->justification == center);
							}
							else
							{
								// an unknown marker stops table centering
								bNextTableIsCentered = FALSE;
							}

							if (gpApp->gCurrentFilterMarkers.Find(wholeMkrNoAsteriskSp) != -1)
							{
								// it was input filtered and \~FILTER ... \~FILTER* brackets were
								// removed above. We can expect that it will be wholly contained
								// within this m_markers and we can parse it here (and enclose it
								// in [...].
								bHasInputFilteredMaterial = TRUE;
								int combinedLen = 0;
								int mkrAndSpLen = 0;
								combinedLen = ParseMarkerAndAnyAssociatedText(pOld,pBuffStart,pEnd,
									bareMkrForLookup,wholeMkr,TRUE,FALSE);	// TRUE is RTF aware
																			// FALSE don't include
																			// char format markers
								// first parse the marker and following space to skip them
								mkrLen = ParseMarkerRTF(pOld,pEnd);
								mkrAndSpLen = mkrLen;
								pOld += mkrLen;
								mkrLen = pDoc->ParseWhiteSpace(pOld);
								mkrAndSpLen += mkrLen;
								pOld += mkrLen;
								// we add [...] and copy from pOld to pNew
								*pNew++ = _T('[');
								int ct = 0;
								while (ct < (combinedLen - mkrAndSpLen))
								{
									if (IsMarkerRTF(pOld,pBuffStart))
									{
										// we have an embedded marker within the associated text
										// of the parent marker. We want to remove these for the
										// purposes of the interlinear display
										mkrLen = ParseMarkerRTF(pOld,pEnd);
										pOld += mkrLen; // point past the embedded marker
										ct += mkrLen;
									}
									else
									{
										// copy the old char to the new char, but if
										// the previous new char was a space and the current
										// old char is a space skip adding two adjacent spaces to pNew
										if (*(pNew -1) == _T(' ') && *pOld == _T(' '))
										{
											pOld++;
											ct++;
										}
										// skip final space before ] is added in pNew
										else if (!(combinedLen - mkrAndSpLen -ct == 1 && *pOld == _T(' ')))
										{
											*pNew++ = *pOld++;
											ct++;
										}
										else
										{
											ct++; // continue incrementing loop
										}
									}
								}
								*pNew++ = _T(']');
							}
							else if (pDoc->IsEndMarker(pOld,pEnd))
							{
								// it's an end marker that is being filtered from output
								// if it is not an end marker of the embedded marker type
								// we stop filtering. If it is an end marker of the embedded
								// marker type we do nothing, because there will probably be
								// the parent end marker coming up soon (which will turn off
								// the filtering for the parent marker.
								if (bCurrentlyFiltering && embeddedWholeEndMkrs.Find(wholeMkr + _T(' ')) != -1)
								{
									// unilaterally stop output filtering
									bCurrentlyFiltering = FALSE;
								}
								// parse the marker and following space
								mkrLen = ParseMarkerRTF(pOld,pEnd);
								pOld += mkrLen;
								mkrLen = pDoc->ParseWhiteSpace(pOld);
								pOld += mkrLen;
								// omit marker itself from pNew
								//bHitMkr = TRUE;
								if (pOld >= pEnd)
									break;
							}
							else
							{
								// parse the marker and following space
								mkrLen = ParseMarkerRTF(pOld,pEnd);
								pOld += mkrLen;
								mkrLen = pDoc->ParseWhiteSpace(pOld);
								pOld += mkrLen;
								// omit marker itself from pNew
								//bHitMkr = TRUE;
								if (pOld >= pEnd)
									break;
							}
						}
					} // end of if IsMarkerRTF()
					else
					{
						// not a marker so just copy
						*pNew++ = *pOld++;
					}
				}// end of while (pOld < pEnd && *pOld != (TCHAR)0)
				*pNew = (wxChar)0; // add a null at the end of the string in pBuffer2
			} // end of special scoping block
			//textStr2.UngetWriteBuf(); // not used in 2.9.3 with wxStringBuffer

			// add any non-output-filtered stuff from m_markers to SrcStr
			if (!SrcStr.IsEmpty())
				SrcStr = textStr2 + _T(' ') + SrcStr;
			else
				SrcStr = textStr2 + SrcStr;

			// also add any non-output-filtered stuff from m_markers to TgtStr
			if (!TgtStr.IsEmpty())
				TgtStr = textStr2 + _T(' ') + TgtStr;
			else
				TgtStr = textStr2 + TgtStr;

		}// end of if (!pSrcPhrase->m_markers.IsEmpty() || HasFilteredInfo(pSrcPhrase))

		// Actual filtering is done here by simply skipping the stuff below and going back to
		// the top of the overall while loop that is looping through source phrases.
		if (bCurrentlyFiltering)
		{
			bMarkerStartsNewLine = FALSE; // reset since current marker's text is to be filtered from output
			bNextTableIsCentered = FALSE; // reset since current marker's text is to be filtered from output
			continue;
		}

		// Note: we also ignore the phrase-internal markers -- I don't think they need
		// consideration for Interlinear output

		// Version 3 Notes:
		// At this point all the text for a pile (a potential table column) is stored in
		// SrcStr, TgtStr, NavStr and GlsStr, with exception of input filtered material
		// which is dealt with below.

		// The procedure for output of Interlinear text is somewhat similar to the procedures
		// used in RebuildSourceText() and RebuildTargetText(), but the Interlinear has to
		// deal with source and target text at the same time, and also must deal with any
		// Gloss text as well as composing appropriate Nav text.
		//
		// Observations from Bruce's RebuildTargetText() function:
		// This routine builds the text into a single wxString returned by wxString&
		// reference parameter, the function itself returning the length of the text as an
		// int. In general it iterates through all the Doc's SPList of m_pSourcePhrases
		// while rebuilding the text. The specifics for rebuilding the target text are
		// covered below.
		//
		// Rebuilding the TARGET TEXT:
		// Rebuilding the target text is handled differently depending on whether the pSrcPhrase
		// is a Retranslation (call DoPlacementOfMarkersInRetranslation), or not a Retranslation
		// (just gets the target substring from pSrcPhrase's m_targetStr, and places the contents
		// of any of pSrcPhrase's m_markers in the substring BEFORE the substring that was obtained
		// from m_targetStr - only a space inserted if m_markers was empty; if m_markers had any
		// filtered material the filter brackets were removed in the process - and any \free
		// markers strings get their word count inserted between |@ and @|). After this, if
		// pSrcPhrase had any internal markers the CPlaceInterMarkers dialog is called to sort
		// their positions out in the substring.
		//
		// Notes:
		// 1. While examining m_markers we will encounter material which is bracketed by \~FILTER
		//    and \~FILTER* markers. Ordinary filtered material will have no adaptations/target
		//    text associated with it, but, by design, the source representation of such filtered
		//    material needs to be echoed to the output both for the source text and for the target
		//    text.
		//    We also have an interest in the three special markers \bt..., \free, and \note which
		//    are always filtered. Although these do not have single source phrases that they
		//    wholly associate/coordinate with, they do have some bearing on a stretch of text
		//    that is being displayed in the interlinear tables. \bt..., \free, and \note also
		//    subjected to the output filter.
		// 2. Character formatting markers we can always safely ignore as they pass through; and
		//    we always preserve their underlying text.
		// 3. For footnotes, endnotes and crossreferences, like everything else, we subject them
		//    to the output filter. If they were not filtered, they will most likely have
		//    adaptations that we represent in the interlinear table with the other adaptations.

		// The special RTF control characters are '\' '{' and '}'. If any of these are embedded
		// in the text strings they must be "escaped" with a backslash character as "\\" "\{" and "\}".
		SrcStr = EscapeAnyEmbeddedRTFControlChars(SrcStr);
		TgtStr = EscapeAnyEmbeddedRTFControlChars(TgtStr);
		GlsStr = EscapeAnyEmbeddedRTFControlChars(GlsStr);
		
		// FreeTStr and BackTStr are empty at this point so we won't escape chars in them either.
		// We call EscapeAnyEmbeddedRTFControlChars() on the appropriate parts of the NavStr below
		// after we've composed it from the m_inform member, and also before the text associated
		// with any note markers, free translations, and back translations gets added to the
		// nav text (via footnotes).

		// We must be careful not to escape any backslashes or curly braces on strings that
		// already have RTF tags in them - escaping valid RTF tags would certainly cause a crash
		// in Word when it tried to read the resulting RTF file.
		//
		// Also, because we need to call GetTextExtent on the NavStr below we can't convert any
		// of it to RTF Unicode \unnnn representation until after the text extent has been
		// determined. TODO: If users want to localize the navigationText attribute in AI_USFM.xml
		// file to make the m_inform source phrase member show a localized representation on
		// screen and in RTF exports, we could implement that below, but we would need to be sure
		// we don't end up converting any RTF tags that might be in NavStr to RTF Unicode \unnnn
		// representation.

		// Code below handles the current source phrase strings SrcStr, TgtStr, GlsStr,  NavStr,
		// FreeTStr and BackTStr, formatting them into RTF table cells.

		// Before we get TextExtents on the NavStr we need to assemble it in the following
		// parts:
		//   partBeforeInformStr + m_informStr + partAfterInformStr
		// We then call GetTextExtent() on the three concatenated parts. Then after getting
		// an accurate text extent, we can convert the m_informStr middle part to RTF Unicode
		// tags, and reassemble the three parts again into NavStr for later processing.
		//
		//wxString partBeforeInformStr = _T("");
		//wxString m_informStr = _T("");
		//wxString partAfterInformStr = _T("");

		// whm 19Nov10 modified outer test to add the specific tests within the block
		// since docV5 has moved stuff out of m_markers, in particular the "fn end"
		// can happen in m_inform when m_markers is empty.
		if (!pSrcPhrase->m_markers.IsEmpty()
			|| !pSrcPhrase->m_chapterVerse.IsEmpty()
			|| !pSrcPhrase->m_inform.IsEmpty()
			|| pSrcPhrase->m_bBeginRetranslation
			|| pSrcPhrase->m_bEndRetranslation
			|| pSrcPhrase->m_bFootnoteEnd
			|| bHasInputFilteredMaterial
			|| pSrcPhrase->m_bBoundary)
		{
			// construct the Nav text string for top row
			// modified 23Nov05 to put ch:vs first in the NavStr
			if (!pSrcPhrase->m_chapterVerse.IsEmpty())
			{
				if (NavStr.IsEmpty())
					NavStr = pSrcPhrase->m_chapterVerse;
				else
					NavStr += _T(" ") + pSrcPhrase->m_chapterVerse;
			}
			if (pSrcPhrase->m_bFootnoteEnd)
			{
				// whm 19Nov10 added hack here to show "end fn" in interlinear navtext cell 
				// as is done on main window display by my hack in CPile::DrawNavTextInfoAndIcons.
				// The more proper way would probably be to actually add this "end fn" text to
				// the m_inform member when pSrcPhrase->m_bFootnoteEnd is set in parsing.
				if (NavStr.IsEmpty())
				{
					NavStr +=  _("end fn"); // localizable
				}
				else
				{
					NavStr += _T(" ");
					NavStr += _("end fn"); // localizable
				}
			}
			if (!pSrcPhrase->m_inform.IsEmpty())
			{
				// NOTE: We cannot convert m_inform to RTF Unicode representation here because
				// we will be fouling up the GetTextExtent calculations farther below, and also
				// we must not to convert any RTF tags themselves to RTF Unicode representation.
				if (NavStr.IsEmpty())
					NavStr += pSrcPhrase->m_inform;
				else
					NavStr += _T(" ") + pSrcPhrase->m_inform;
			}
			if (pSrcPhrase->m_bBeginRetranslation)
			{
				if (NavStr.IsEmpty())
					NavStr += _T("#RETRANS...");
				else
				{
					NavStr += _T(" ");
					NavStr += _T("#RETRANS...");
				}
			}
			if (pSrcPhrase->m_bEndRetranslation)
			{
				// Note: it seems m_bEndRetranslation in pSrcPhrase is not always set correctly
				if (NavStr.IsEmpty())
					NavStr += _T("...RETRANS#");
				else
				{
					NavStr += _T(" ");
					NavStr += _T("...RETRANS#");
				}
			}
			if (bHasInputFilteredMaterial)
			{
				if (!NavStr.IsEmpty())
					NavStr = _T("[FILTERED] ") + NavStr;
				else
					NavStr = _T("[FILTERED]");
			}
		}

		// Trim SrcStr, TgtStr, GlsStr, and NavStr since no leading or following spaces are needed
		// within cells. Spaces are added elsewhere when footnote material is added to NavStr.
		SrcStr.Trim(FALSE);	// trim left end
		SrcStr.Trim(TRUE);	// trim right end
		TgtStr.Trim(FALSE);	// trim left end
		TgtStr.Trim(TRUE);	// trim right end
		GlsStr.Trim(FALSE); // trim left end
		GlsStr.Trim(TRUE);	// trim right end
		NavStr.Trim(FALSE); // trim left end
		NavStr.Trim(TRUE);	// trim right end

		// Get text extents including gap between piles/columns.
		// Using LO_ENGLISH mapping mode, GetTextExtent returns int values that are in hundredths of
		// an inch. We need to multiply by 14.4 to get twips, add twice the amount of the
		// (half) intercell gap + a fudge factor for word processors like Word which tend to 
		// wrap short strings (like the ... null phrase) of small point size within short cells.

		// whm 8Nov07 added to reduce unwanted wrapping of text in cells. Unicode fonts seem to
		// require a greater factor.
		int RTFCellGapFudgeFactor;
#ifdef _UNICODE
		RTFCellGapFudgeFactor = 280; //440;
#else
		RTFCellGapFudgeFactor = 140; // 220;
#endif

		// Note: pRtfSrcFnt, pRtfTgtFnt, and pRtfNavFnt are based on copies of the equivalent screen fonts
		// but are adjusted for the attributes fontSize, bold, and underline based on the attributes
		// for interlinear fonts as defined in AI_USFM.xml. The adjustments are done above the current
		// outer loop above closer to the beginning of DoExportInterlinearRTF().
		dC.SetFont(*pRtfSrcFnt);
		dC.GetTextExtent(SrcStr,&Srcextent.x,&Srcextent.y);
		WidthSrc = (int)((float)(Srcextent.GetWidth())*14.4) + (ngaphNum*2) + RTFCellGapFudgeFactor; // convert 100ths to twips + gap*2
		dC.SetFont(*pRtfTgtFnt);
		dC.GetTextExtent(TgtStr,&Tgtextent.x,&Tgtextent.y);
		WidthTgt = (int)((float)(Tgtextent.GetWidth())*14.4) + (ngaphNum*2) + RTFCellGapFudgeFactor; // convert 100ths to twips + gap*2
		if (gbGlossingUsesNavFont)
			dC.SetFont(*pRtfNavFnt);
		else
			dC.SetFont(*pRtfTgtFnt);
		dC.GetTextExtent(GlsStr,&Glsextent.x,&Glsextent.y);
		WidthGls = (int)((float)(Glsextent.GetWidth())*14.4) + (ngaphNum*2) + RTFCellGapFudgeFactor; // convert 100ths to twips + gap*2
		dC.SetFont(*pRtfNavFnt);
		dC.GetTextExtent(NavStr,&Navextent.x,&Navextent.y);
		WidthNav = (int)((float)(Navextent.GetWidth())*14.4) + (ngaphNum*2) + RTFCellGapFudgeFactor; // convert 100ths to twips + gap*2
		// FreeTransextent and WidthFreeTrans are determined below
		// BackTransextent and WidthBackTrans are determined below

		// TODO: If users want NavStr to represent Unicode in RTF the conversion should be done here after
		// the above GetTextExtent calls.

		// We insert the special formatting for any \note, \free or \bt... markers below
		// surrounding a suffixed navigation text caller added to NavStr. For most formatting
		// options below (footnotes and balloon comments), the effect will be the addition of
		// a caller we supply in the form of the baremarker letter sequence "note" "free" or
		// "bt", "bt..." etc, suffixed to any existing nav text in the top cell. For the options
		// that require an additonal row added to the bottom of the tables, we create the extra
		// row spanning the tables with the assoc text in merged cells of that row.

		// whm added 5Nov07
		// The pSrcPhrase member m_inform may contain an embedded backslash in cases where
		// the input text parsing routine did not recognize a marker. In such cases the NavStr
		// at this point may contain something like "?\vref?" where the \vref marker was not
		// recognized. We need to ensure that the backslash is escaped in these cases.
		NavStr = EscapeAnyEmbeddedRTFControlChars(NavStr);
		
		if (bHasNoteMarker)
		{
			// We implement Adapt It Notes by simply adding the RTF footnote tags and
			// caller "note" to the NavStr. We need to increase the WidthNav twips value for
			// the text extent of the added " note" (but not the added RTF tags). When
			// bPlaceAINotesInRTFText is TRUE, the destination ends up being a balloon
			// comment in the right margin; when FALSE the destination is a footnote at
			// the bottom of the page. In either case the caller is suffixed to the NavStr
			// string in the top nav text cell for this source phrase.

			int posSp = assocNoteMarkerText.Find(_T(' '));
			wxString bareNoteMarker = assocNoteMarkerText.Mid(1,posSp);
			assocNoteMarkerText = assocNoteMarkerText.Mid(posSp);
			// trim whitespace and pad with spaces
			assocNoteMarkerText.Trim(FALSE); // trim left end
			assocNoteMarkerText.Trim(TRUE); // trim right end
			bareNoteMarker.Trim(FALSE); // trim left end
			bareNoteMarker.Trim(TRUE); // trim right end
			
			// The assocNoteMarkerText string may also contain embedded backslash or curly brace
			// characters inserted within AI Notes or embedded within back or free translation etc.
			// We need to ensure that the backslash is escaped in these cases also.
			assocNoteMarkerText = EscapeAnyEmbeddedRTFControlChars(assocNoteMarkerText);

			assocNoteMarkerText = _T(' ') + assocNoteMarkerText + _T(' ');
			// construct numerically sequenced caller
			noteRefNumInt++; // increment the note N to note 1, note 2, note 3, etc.
			noteRefNumStr = bareNoteMarker + _T(' ');
			noteRefNumStr << noteRefNumInt; // add N to "note N"
			callerStr = noteRefNumStr;

			if (bPlaceAINotesInRTFText)
			{
				// user wants Adapt It notes (\note) formatted as balloon text Comments
				// Note: Commented out MiscRTF lines below were an attempt to format the note
				// so that the comment would "bracket" the entire "note n" caller, but I was
				// not getting what I wanted. TODO: determine if it would still be profitable
				// to get balloon comments to bracket the whole caller.
				MiscRTF.Empty();
				//MiscRTF = _T("{\\*\\atrfstart ") + noteRefNumStr + _T('}');
				//MiscRTF += _T(" ") + noteRefNumStr + _T('}');
				// first output opening brace for the _annotation_ref style
				MiscRTF += _T("{");
				// next output the \cs style tags for _annotation_ref which are in SindocAnnotRef
				// We do not access _annotation_ref from rtfTagsMap here because we have renumbered
				// the styles for our abbreviated interlinear output \stylesheet
				MiscRTF += SindocAnnotRef;
				if (!NavStr.IsEmpty())
					callerStr = _T(' ') + callerStr;
				MiscRTF += callerStr;
				MiscRTF += _T(' ');
				//MiscRTF += _T("\\*\\atrfend ") + noteRefNumStr + _T('}');
				// next output the required RTF tags to prefix an annotation
				MiscRTF += _T("{\\*\\atnid Adapt It Note:}{\\*\\atnauthor       }\\chatn {\\*\\annotation");
				//// insert "\*\atnref N" where N is noteRefNumStr
				//MiscRTF += _T("{\\*\\atnref ") + noteRefNumStr + _T('}');

				// Note: The _annotation_text in-doc tag string already has \par \pard\plain prefixed
				// to it, so the addition of \pard\plain below is not needed
				//MiscRTF += _T(" \\pard\\plain ");

				// We do not access _annotation_text from rtfTagsMap here because we have renumbered
				// the styles for our abbreviated interlinear output \stylesheet
				// output the _annotation_text paragraph style tags
				MiscRTF += SindocAnnotText;
				// output the _annotation_ref style tags again
				MiscRTF += _T('{');
				// next output the \cs style tags for _annotation_ref
				MiscRTF += SindocAnnotRef;
				// add more necessary tags for annotation/comment
				MiscRTF += _T("\\chatn }{");
				// now output the actual note string
				MiscRTF += noteRefNumStr + _T(":"); // prefix dest with "note N:"

				// whm 8Nov07 added the following to force the format of Unicode chars
				// as \uN\'f3 RTF format.
				assocNoteMarkerText = GetANSIorUnicodeRTFCharsFromString(assocNoteMarkerText);

				MiscRTF += assocNoteMarkerText; // should have initial space
				MiscRTF += _T("}}}"); // closing braces for note (annotation)
				// for balloon comment we only use the N part of "note N"
				if (!NavStr.IsEmpty())
				{
					noteRefNumStr += _T(' ');
				}
				//NavStr += noteRefNumStr;
				// recalculate text extent of NavStr since we've added "note N" to it
				dC.SetFont(*pRtfNavFnt); 
				dC.GetTextExtent(noteRefNumStr,&Navextent.x,&Navextent.y);
				WidthNav += (int)((float)(Navextent.GetWidth())*14.4*.7); // increase WidthNav for added "N"
				// add RTF tags to NavStr
				NavStr += MiscRTF;
			}
			else
			{
				// user wants Adapt It notes formatted as footnotes
				// increment WidthNav based on 70% of the text extent of the added callerStr
				// Note: We use 70% because Word automatically reduces the font size of footnote
				// callers to about 70% of its original font size, and so we'll shorted the
				// horizontal extent calculated here by a factor of about .7
				bool addSpBeforeCallerStr = FALSE;
				if (!NavStr.IsEmpty())
				{
					if (NavStr[NavStr.Length()-1] != _T(' '))
						callerStr = _T(' ') + callerStr;
					addSpBeforeCallerStr = TRUE;
				}
				dC.SetFont(*pRtfNavFnt);
				dC.GetTextExtent(callerStr,&Navextent.x,&Navextent.y);
				WidthNav += (int)((float)(Navextent.GetWidth())*14.4*.7); // increase WidthNav for added "note N"

				NavStr += FormatRTFFootnoteIntoString(callerStr,assocNoteMarkerText,
						noteRefNumStr,SindocFnCaller, SindocFnText, SindocAnnotRef,
						SindocAnnotText, addSpBeforeCallerStr);

			}

			// turn off the flag here
			bHasNoteMarker = FALSE;
		}

		if (bHasFreeMarker)
		{
			// We implement Free Translation entries by adding the RTF footnote tags and
			// caller " free" to the NavStr when bPlaceFreeTransInRTFText is FALSE. In those
			// cases we increase the WidthNav twips value for the added text extent of the
			// " free" addition (but not for the RTF tags). When bPlaceFreeTransInRTFText is
			// TRUE, we do table manipulation to create the extra row spanning the tables with
			// the assoc text in merged cells of that row. In this case there is no caller.
			int posSp = assocFreeMarkerText.Find(_T(' '));
			wxString bareFreeMarker = assocFreeMarkerText.Mid(1,posSp);
			assocFreeMarkerText = assocFreeMarkerText.Mid(posSp);
			// trim whitespace and add square brackets
			assocFreeMarkerText.Trim(FALSE); // trim left end
			assocFreeMarkerText.Trim(TRUE); // trim right end
			bareFreeMarker.Trim(FALSE); // trim left end
			bareFreeMarker.Trim(TRUE); // trim right end
			
			// The assocFreeMarkerText string may also contain embedded backslash or curly brace
			// characters inserted within AI Notes or embedded within back or free translation etc.
			// We need to ensure that the backslash is escaped in these cases also.
			assocFreeMarkerText = EscapeAnyEmbeddedRTFControlChars(assocFreeMarkerText);

			assocFreeMarkerText = _T(' ') + assocFreeMarkerText + _T(' ');
			dC.SetFont(*pRtfFreeFnt);
			dC.GetTextExtent(assocFreeMarkerText,&FreeTransextent.x,&FreeTransextent.y);
			WidthFreeTrans = (int)((float)(FreeTransextent.GetWidth())*14.4) + (ngaphNum*2) + RTFCellGapFudgeFactor; // convert 100ths to twips + gap*2
			WidthFreeTrans = WidthFreeTrans; // avoid warning 
			// construct numerically sequenced caller
			freeRefNumInt++; // increment the free N to free 1, free 2, free 3, etc.
			freeRefNumStr = bareFreeMarker + _T(' ');
			freeRefNumStr << freeRefNumInt; // add N to "free N"
			callerStr = freeRefNumStr;

			if (bPlaceFreeTransInRTFText)
			{
				// User wants this Free Translation (\free) formatted as separate row spanning table cells.
				FreeTStr = assocFreeMarkerText;
			}
			else
			{
				// user wants Free Translations formatted as footnotes
				// calculate new Navextent before calling FormatRTFFootnoteIntoString()
				bool addSpBeforeCallerStr = FALSE;
				if (!NavStr.IsEmpty())
				{
					if (NavStr[NavStr.Length()-1] != _T(' '))
						callerStr = _T(' ') + callerStr;
					addSpBeforeCallerStr = TRUE;
				}
				dC.SetFont(*pRtfNavFnt);
				dC.GetTextExtent(callerStr,&Navextent.x,&Navextent.y);
				WidthNav += (int)((float)(Navextent.GetWidth())*14.4*.7); // increase WidthNav for added "note N"

				NavStr += FormatRTFFootnoteIntoString(callerStr,assocFreeMarkerText,
						freeRefNumStr,SindocFnCaller, SindocFnText, SindocAnnotRef,
						SindocAnnotText, addSpBeforeCallerStr);
			}
		}

		if (bHasBTMarker)
		{
			// We implement Back Translation entries by adding the RTF footnote tags and
			// caller " bt" (or " bt...") to the NavStr when bPlaceBackTransInRTFText is FALSE.
			// In those cases we need to increase the WidthNav twips value for the added text
			// extent of the  " bt" addition (but not for the added RTF tags). When
			// bPlaceBackTransInRTFText is TRUE, we do table manipulation to create the extra
			// row spanning the tables with the assoc text in merged cells of that row. In this
			// case there is no caller.
			int posSp = assocBTMarkerText.Find(_T(' '));
			bareBTMarker = assocBTMarkerText.Mid(1,posSp);
			assocBTMarkerText = assocBTMarkerText.Mid(posSp);
			// trim whitespace and add square brackets
			assocBTMarkerText.Trim(FALSE); // trim left end
			assocBTMarkerText.Trim(TRUE); // trim right end
			bareBTMarker.Trim(FALSE); // trim left end
			bareBTMarker.Trim(TRUE); // trim right end
			
			// The assocBTMarkerText string may also contain embedded backslash or curly brace
			// characters inserted within AI Notes or embedded within back or free translation etc.
			// We need to ensure that the backslash is escaped in these cases also.
			assocBTMarkerText = EscapeAnyEmbeddedRTFControlChars(assocBTMarkerText);

			assocBTMarkerText = _T(' ') + assocBTMarkerText + _T(' ');
			dC.SetFont(*pRtfBtFnt);
			dC.GetTextExtent(assocBTMarkerText,&BackTransextent.x,&BackTransextent.y);
			WidthBackTrans = (int)((float)(BackTransextent.GetWidth())*14.4) + (ngaphNum*2) + RTFCellGapFudgeFactor; // convert 100ths to twips + gap*2
			WidthBackTrans = WidthBackTrans; // avoid warning
			// construct numerically sequenced caller
			btRefNumInt++; // increment the bt N to bt 1, bt 2, bt 3, etc.
			btRefNumStr = bareBTMarker + _T(' '); // _T("bt ") or "bt... "
			btRefNumStr << btRefNumInt;  // add N to "bt N"
			callerStr = btRefNumStr;

			if (bPlaceBackTransInRTFText)
			{
				// user wants this Back Translation (\bt...) formatted as separate row spanning table cells
				BackTStr = assocBTMarkerText;
			}
			else
			{
				// user wants Back Translations formatted as footnotes
				// calculate new Navextent before calling FormatRTFFootnoteIntoString()
				bool addSpBeforeCallerStr = FALSE;
				if (!NavStr.IsEmpty())
				{
					if (NavStr[NavStr.Length()-1] != _T(' '))
						callerStr = _T(' ') + callerStr;
					addSpBeforeCallerStr = TRUE;
				}
				dC.SetFont(*pRtfNavFnt);
				dC.GetTextExtent(callerStr,&Navextent.x,&Navextent.y);
				WidthNav += (int)((float)(Navextent.GetWidth())*14.4*.7); // increase WidthNav for added "note N"

				NavStr += FormatRTFFootnoteIntoString(callerStr,assocBTMarkerText,
						btRefNumStr,SindocFnCaller, SindocFnText, SindocAnnotRef,
						SindocAnnotText, addSpBeforeCallerStr);
			}
		}

		// get the MaxWidth of this column
		MaxColWidth = 0;
		if (bInclSrcLangRow && WidthSrc > MaxColWidth)
			MaxColWidth = WidthSrc;
		if (bInclTgtLangRow && WidthTgt > MaxColWidth)
			MaxColWidth = WidthTgt;
		if (bInclGlsLangRow && WidthGls > MaxColWidth)
			MaxColWidth = WidthGls;
		if (bInclNavLangRow && WidthNav > MaxColWidth)
			MaxColWidth = WidthNav;

		// WidthFreeTrans and WidthBackTrans do not enter into the calculations here because
		// we allow the existing column widths to stand; if the free and/or back trans text is
		// too wide, Word will wrap it within the existing columns

		// check if we have a full table
		// whm added test for pSrcPhrase->m_nSequNumber != 0 since we may encounter a marker
		// at the beginning of the file that would otherwise want to start a new line, but
		// obviously wouldn't indicate we have already gotten a "full" table.
		if (MaxColWidth >= MaxRowWidth
			|| (MaxColWidth + AccumRowWidth) >= MaxRowWidth
			|| (bNewTableForNewLineMarker && bMarkerStartsNewLine && pSrcPhrase->m_nSequNumber != 0))
		{
a:
			// we have a "full" table
			// Don't allow extent of table to extend beyond our margin; instead set
			// the column width to equal the max row width and allow Word to wrap
			// the text within the cell(s) if necessary.
			if (MaxColWidth > MaxRowWidth)
				MaxColWidth = MaxRowWidth;

			AccumRowWidth = 0;

			// reset the newline marker flag
			bMarkerStartsNewLine = FALSE;

			// at beginning of output for this table no rows have yet been processed
			bool bSrcProcessed = FALSE;
			bool bTgtProcessed = FALSE;
			bool bGlsProcessed = FALSE;
			bool bNavProcessed = FALSE;
			// two below added for version 3
			bool bFreeTransProcessed = FALSE;
			bool bBackTransProcessed = FALSE;

			// We have all the data for a complete table, so now we format the data into
			// rows.
			// Process the table into the following logical rows (default selections):
			// 1. Row 0 Prefixed data (for compatibility with older RTF readers - repeated in 2 below.
			// 2. Row 0 Data for 1st Row selected for output. This would by default
			//    be the Nav Lang row.
			// 3. Row 1 Data for 2nd Row selected for output. This would by default
			//    be the Src Lang row.
			// 4. Row 2 Data for 3rd Row selected for output. This would by default
			//    be the Tgt Lang row.
			// 5. Row 3 Data for 4th Row selected for output. This would by default
			//    be the Gls Lang row, if glossing is shown. Note: Row 2 and Row 3 would be
			//    reversed of Glossing check box is checked (glossing in progress).
			//    Row 4 Data for any Free Translation when placed in row (selected by default).
			// 6. Row 5 Data for any Back Translation when placed in row (optionally selected, but
			//    not by default).
			nCurrentRow = 0;

			TRowNum.Empty();
			TRowNum << nCurrentRow; //_itot(nCurrentRow,rbuf,10);

			wxString TLastRow;
			if (nCurrentRow == nNumRowsInTable-1)
				TLastRow = _T("\\lastrow");			// generate \lastrow tag if processing last row
			else
				TLastRow = _T("");

			if (bCenterTableForCenteredMarker && bTableIsCentered)
			{
				Tjust = _T("\\trqc"); // center table in margins
			}
			else
			{
				Tjust = _T(""); // allow the default to happen
				if (bReverseLayout)
				{
					Tjust = _T("\\trqr");					// Right justify the table row with respect to its 
															// containing column (containing column is the page
															// between the margins in our case)
					TRowPrecedence = _T("\\rtlrow");		// Cells in the table row will have RTL precedence
															// With this tag, we don't have to reorder the row text
															// in our RTF output to get RTL precedence
					TDirection = _T("\\taprtl");			// \taprtl indicates that the table direction is right-to-left.
				}

				else
				{
					Tjust = _T("\\trql");					// Left justify the table row with respect to its
															// containing column
					TRowPrecedence = _T("\\ltrrow");		// Cells in the table row will have LTR precedence
															// With this tag, the row text order is interpreted as normal
															// LTR in the output and gets the default LTR precedence
					TDirection = _T("");					// There is no \tapltr tag so assign null string to TDirection
				}
			}


			if (nCurrentRow == 0)					// need Table defs prefixed only for row index zero
			{										// of each table
				// Process Row 0 PREFIXED DATA (this is not absolutely required for newer word processors,
				// but is done for compatibility with older RTF readers - including Word 97 and earlier).
				CellDimsSrc.Empty();
				CellDimsTgt.Empty();
				CellDimsGls.Empty();
				CellDimsNav.Empty();
				// below two added for version 3
				cellDimsFree.Empty();
				cellDimsBack.Empty();
				CellxNum.Empty(); // holds the N value string for the current cell being processed
				// the tags below are output as many times as there are cells in this row, with
				// increasing values for N. Save the whole string in the CellDims... variables for
				// use in the other rows in the table

				for (int count=0; count < (int)cellxNList.GetCount(); count++)
				{

					CellxNum = cellxNList.Item(count); // Get N from cellxNList

					CellDimsNav += gpApp->m_eolStr
					+Tcellx+CellxNum;

					CellDimsSrc += gpApp->m_eolStr
					+Tcellx+CellxNum;

					CellDimsTgt += gpApp->m_eolStr
					+Tcellx+CellxNum;

					CellDimsGls += gpApp->m_eolStr
					+Tcellx+CellxNum;
				}

				wxString nullStr = _T("");
				int cellCount;
				if (bInclFreeTransRow)
				{
					// Ensure that any Free Tranlsation row gets assigned the same overall width as
					// the other rows above in the table.
					cellCount = cellxNListFree.GetCount();
					if (cellCount == 0)
					{
						// There are no cells in cellxNListFree, so just add one to
						// ensure the Free Trans row ends at same width as the other rows above.
						cellxNListFree.Add(CellxNum); //cellxNListFree.AddTail(CellxNum);
						// wx note: wxList works with pointers to wxStrings on the heap so must use new in
						// the Append call below; wxList::DeleteContents(true) then deletes the list's client
						// contents (wxStrings).
						freeTransList.Add(nullStr); // keep lists in sync
					}
					else
					{
						// The Free list has one or more cells, so check the last entry in cellxNListFree
						// and if it is not the same as CellxNum above, add the CellxNum value to
						// cellxNListFree (thus making it extend to the same width as rows above it).
						wxASSERT(cellxNListFree.GetCount() > 0);
						wxString TCellNTest = cellxNListFree.Item(cellxNListFree.GetCount()-1);
						if (TCellNTest != CellxNum)
						{
							cellxNListFree.Add(CellxNum);
							// wx note: wxList works with pointers to wxStrings on the heap so must use new in
							// the Append call below; wxList::DeleteContents(true) then deletes the list's client
							// contents (wxStrings).
							freeTransList.Add(nullStr); // keep lists in sync
						}
					}
					// now fill cellDimsFree with the RTF "\cellxN \n" tag lines
					for (int count=0; count < (int)cellxNListFree.GetCount(); count++)
					{

						wxString CellxNumFree = cellxNListFree.Item(count); // Get N from cellxNList
						if (CellxNumFree != _T("0") && CellxNumFree != _T(""))
						{
							// don't list the \cellx0 forms
							cellDimsFree += gpApp->m_eolStr // version 3
							+Tcellx+CellxNumFree;
						}

					}
				}

				if (bInclBackTransRow)
				{
					// Ensure that any Back Tranlsation row gets assigned the same overall width as
					// the other rows above in the table.
					cellCount = cellxNListBT.GetCount();
					if (cellCount == 0)
					{
						// There are no cells in cellxNListBT, so just add one to
						// ensure the Back Trans row ends at same width as the other rows above.
						cellxNListBT.Add(CellxNum);
						// wx note: wxList works with pointers to wxStrings on the heap so must use new in
						// the Append call below; wxList::DeleteContents(true) then deletes the list's client
						// contents (wxStrings).
						backTransList.Add(nullStr); // keep lists in sync
					}
					else
					{
						// The Back list has one or more cells, so check the last entry in cellxNListBT
						// and, if its value is not the same as CellxNum above, add the CellxNum value to
						// cellxNListBT (thus making it extend to the same width as rows above it).
						wxASSERT(cellxNListBT.GetCount() > 0);
						wxString TCellNTest = cellxNListBT.Item(cellxNListBT.GetCount()-1); 
						if (TCellNTest != CellxNum) 
						{
							cellxNListBT.Add(CellxNum);
							// wx note: wxList works with pointers to wxStrings on the heap so must use new in
							// the Append call below; wxList::DeleteContents(true) then deletes the list's client
							// contents (wxStrings).
							backTransList.Add(nullStr); // keep lists in sync
						}
					}
					// now fill cellDimsBack with the RTF "\cellxN \n" tag lines
					for (int count=0; count < (int)cellxNListBT.GetCount(); count++)
					{

						wxString CellxNumBT = cellxNListBT.Item(count); // Get N from cellxNList
						if (CellxNumBT != _T("0") && CellxNumBT != _T(""))
						{
							cellDimsBack += gpApp->m_eolStr // version 3
							+Tcellx+CellxNumBT;
						}
					}
				}
			}// end of if nCurrentRow == 0
			// Process Row 0 Data for 1st row selected for output. If bInclNavLangRow is TRUE
			// this would be the Nav Lang row.
			if (bInclNavLangRow && !bNavProcessed)
			{
				hstr = PardPlain					// start of hstr output string for this table
				+gpApp->m_eolStr
				+Trowd								// \trowd
				+TtsN+TtrgaphN						// \ts21\trgaph108
				+TStartPos							// \trleft0
				+Tjust								// \trqc if centered table, otherwise ""
				+TRowPrecedence						// \rtlrow or \ltrrow depending on bReverseLayout
				+TRautoFit							// whm added 11Nov07 \trautofit to prevent excessive wrapping
				+TDirection;						// \taprtl if bReverseLayout is TRUE "" otherwise
				// the tags above are output once for a given row

				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}
				
				// CellDimsNav below contains as many \cellxN items as there are cells in this row, with
				// increasing values for N
				hstr = CellDimsNav;
				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}
				
				// don't increment nCurrentRow here because it would still be row zero if
				// the Nav Lang row is included
				TRowNum.Empty();
				TRowNum << nCurrentRow; 

				wxString TLastRow;
				TLastRow = _T("");

				bNavProcessed = TRUE;
				hstr = gpApp->m_eolStr
				+PardPlain
				+SintblNav							// Nav Style
				+ gpApp->m_eolStr;
				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}

				// Output the actual Nav row text delimited by \cell tags
				// TODO: check if Unicode version needs to separate actual text of
				// \free and \bt... from the system encoded stuff, with the actual
				// text written as vernacular encoding
				for (int count=0; count < (int)navList.GetCount(); count++)
				{
					// Output each nav string with its own encoding. The navList contains
					// the cell contents for this given cell in the row.
					// Use m_systemEncoding for the nav text row.
					wxString testStr = navList.Item(count);
					if (!WriteOutputString(f,gpApp->m_systemEncoding,testStr))	// Nav text string
					{	
						pProgDlg->Destroy();
						return;
					}
					// whm 8Nov07 note: Unlike the case with source, target and gloss text lists,
					// We cannot here use the tgt encoding, but must retain the m_systemEncoding,
					// because ProcessAnaWriteDestinationText() formats the nav text. If we were
					// to call WriteOutputString with anything other than m_systemEncoding, it would
					// double convert the control words and make a general mess of the destination
					// text.
					// Note: \cell delimiter follows cell contents for each cell in row
					if (!WriteOutputString(f,gpApp->m_systemEncoding,Tcell))					// \cell delimiter
					{	
						pProgDlg->Destroy();
						return;
					}
				}

				hstr = Trow
				+ gpApp->m_eolStr
				+ gpApp->m_eolStr // whm added second m_eolStr to visuall mark end of row in RTF file
				+PardPlain;
				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}
			}// end of if (bInclNavLangRow && !NavProcessed)

			// Process Row 1 Data for 2nd row selected for output. If bInclSrcLangRow is TRUE
			// this would be the Src Lang row.
			if (bInclSrcLangRow && !bSrcProcessed)
			{
				hstr = PardPlain					// start of hstr output string for this table
				+gpApp->m_eolStr
				+Trowd								// \trowd
				+TtsN+TtrgaphN						// \ts21\trgaph108
				+TStartPos							// \trleft0
				+Tjust								// \trqc if centered table, otherwise ""
				+TRowPrecedence						// \rtlrow or \ltrrow depending on bReverseLayout
				+TRautoFit							// whm added 11Nov07 \trautofit to prevent excessive wrapping
				+TDirection;						// \taprtl if bReverseLayout is TRUE "" otherwise
				// the tags above are output once for a given row

				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}
				
				// CellDimsNav below contains as many \cellxN items as there are cells in this row, with
				// increasing values for N
				hstr = CellDimsSrc;
				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}
				
				// if the Nav Lang row above is included then this would be row index 1,
				// otherwise this Src Lang row would still be row index 0
				if (bInclNavLangRow)
					nCurrentRow++;
				TRowNum.Empty();
				TRowNum << nCurrentRow;

				wxString TLastRow;
				TLastRow = _T("");

				bSrcProcessed = TRUE;
				hstr = gpApp->m_eolStr
				+PardPlain
				+SintblSrc							// Src Style
				+gpApp->m_eolStr;
				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}

				// Output the actual Src row text delimited by \cell tags
				for (int count=0; count < (int)srcList.GetCount(); count++)
				{
					if (!WriteOutputString(f,gpApp->m_srcEncoding,srcList.Item(count))) // Src text string
					{	
						pProgDlg->Destroy();
						return;
					}
					if (!WriteOutputString(f,gpApp->m_systemEncoding,Tcell))				// \cell delimiter
					{	
						pProgDlg->Destroy();
						return;
					}
				}

				hstr = Trow
				+ gpApp->m_eolStr
				+ gpApp->m_eolStr // whm added second m_eolStr to visuall mark end of row in RTF file
				+PardPlain;
				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}
			}// end of if (bInclSrcLangRow && !SrcProcessed)

			// Process Rows 2 & 3 Data for 3rd and 4th rows selected for output. If gbIsGlossing this
			// would be Gls Lang and Tgt Lang (assuming bInclGlsLangRow and bInclTgtRow are TRUE); or
			// reversed order of Tgt Lang and Gls Lang (assuming bInclTgtRow and bInclGlsLangRow are
			// TRUE).
			if (gbIsGlossing)
			{
				// When Glossing place the Gls language row above the Tgt language row
				if (bInclGlsLangRow && !bGlsProcessed)
				{
					hstr = PardPlain					// start of hstr output string for this table
					+gpApp->m_eolStr
					+Trowd								// \trowd
					+TtsN+TtrgaphN						// \ts21\trgaph108
					+TStartPos							// \trleft0
					+Tjust								// \trqc if centered table, otherwise ""
					+TRowPrecedence						// \rtlrow or \ltrrow depending on bReverseLayout
					+TRautoFit							// whm added 11Nov07 \trautofit to prevent excessive wrapping
					+TDirection;						// \taprtl if bReverseLayout is TRUE "" otherwise
					// the tags above are output once for a given row

					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}
					
					// CellDimsNav and CellDimsTgt below contains as many \cellxN items as there are cells
					// in this row, with increasing values for N
					if (gbGlossingUsesNavFont)
					{
						hstr = CellDimsNav;
					}
					else
					{
						hstr = CellDimsTgt;
					}
					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}

					if (bInclNavLangRow || bInclSrcLangRow || bInclTgtLangRow)
						nCurrentRow++;
					TRowNum.Empty();
					TRowNum << nCurrentRow;

					wxString TLastRow;
					TLastRow = _T("");

					bGlsProcessed = TRUE;
					hstr = gpApp->m_eolStr
					+PardPlain
					+SintblGls							// Gls Style
					+gpApp->m_eolStr;
					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}

					// Output the actual Gls row text delimited by \cell tags
					if (gbGlossingUsesNavFont)
					{
						for (int count=0; count < (int)glsList.GetCount(); count++)
						{
							// whm 8Nov07 note: We'll use the tgt encoding for gloss text which
							// forces WriteOutputString to use the \uN\'f3 RTF Unicode char format
							if (!WriteOutputString(f,gpApp->m_tgtEncoding,glsList.Item(count))) // Gls text string
							{	
								pProgDlg->Destroy();
								return;
							}
							if (!WriteOutputString(f,gpApp->m_systemEncoding,Tcell))			// \cell delimiter
							{	
								pProgDlg->Destroy();
								return;
							}
						}
					}
					else
					{
						for (int count=0; count < (int)glsList.GetCount(); count++)
						{
							if (!WriteOutputString(f,gpApp->m_tgtEncoding,glsList.Item(count))) // Gls uses Tgt encoding
							{	
								pProgDlg->Destroy();
								return;
							}
							if (!WriteOutputString(f,gpApp->m_systemEncoding,Tcell))				// \cell delimiter
							{	
								pProgDlg->Destroy();
								return;
							}
						}
					}

					hstr = Trow
					+ gpApp->m_eolStr
					+ gpApp->m_eolStr // whm added second m_eolStr to visuall mark end of row in RTF file
					+PardPlain;
					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}
				}// end of if (bInclGlsLangRow && !GlsProcessed)

				if (bInclTgtLangRow && !bTgtProcessed)
				{
					hstr = PardPlain					// start of hstr output string for this table
					+gpApp->m_eolStr
					+Trowd								// \trowd
					+TtsN+TtrgaphN						// \ts21\trgaph108
					+TStartPos							// \trleft0
					+Tjust								// \trqc if centered table, otherwise ""
					+TRowPrecedence						// \rtlrow or \ltrrow depending on bReverseLayout
					+TRautoFit							// whm added 11Nov07 \trautofit to prevent excessive wrapping
					+TDirection;						// \taprtl if bReverseLayout is TRUE "" otherwise
					// the tags above are output once for a given row

					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}
					
					// CellDimsTgt below contains as many \cellxN items as there are cells in this row, with
					// increasing values for N
					hstr = CellDimsTgt;
					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}
					
					if (bInclNavLangRow || bInclSrcLangRow)
						nCurrentRow++;
					TRowNum.Empty();
					TRowNum << nCurrentRow;

					wxString TLastRow;
					TLastRow = _T("");

					bTgtProcessed = TRUE;
					hstr = gpApp->m_eolStr
					+PardPlain
					+SintblTgt							// Tgt Style
					+gpApp->m_eolStr;
					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}

					// Output the actual Tgt row text delimited by \cell tags
					for (int count=0; count < (int)tgtList.GetCount(); count++)
					{
						if (!WriteOutputString(f,gpApp->m_tgtEncoding,tgtList.Item(count))) // Tgt text string
						{	
							pProgDlg->Destroy();
							return;
						}
						if (!WriteOutputString(f,gpApp->m_systemEncoding,Tcell))			// \cell delimiter
						{	
							pProgDlg->Destroy();
							return;
						}
					}

					hstr = Trow
					+ gpApp->m_eolStr
					+ gpApp->m_eolStr // whm added second m_eolStr to visuall mark end of row in RTF file
					+PardPlain;
					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}
				}// end of if (bInclTgtLangRow && !TgtProcessed)

			}
			else
			{
				hstr = PardPlain					// start of hstr output string for this table
				+gpApp->m_eolStr
				+Trowd								// \trowd
				+TtsN+TtrgaphN						// \ts21\trgaph108
				+TStartPos							// \trleft0
				+Tjust								// \trqc if centered table, otherwise ""
				+TRowPrecedence						// \rtlrow or \ltrrow depending on bReverseLayout
				+TRautoFit							// whm added 11Nov07 \trautofit to prevent excessive wrapping
				+TDirection;						// \taprtl if bReverseLayout is TRUE "" otherwise
				// the tags above are output once for a given row

				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}
				
				// CellDimsNav and CellDimsTgt below contains as many \cellxN items as there are cells
				// in this row, with increasing values for N
				if (gbGlossingUsesNavFont)
				{
					hstr = CellDimsNav;
				}
				else
				{
					hstr = CellDimsTgt;
				}
				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}
				
				// When not Glossing place Target Row then Gloss Row last
				if (bInclTgtLangRow && !bTgtProcessed)
				{
					if (bInclNavLangRow || bInclSrcLangRow)
						nCurrentRow++;
					TRowNum.Empty();
					TRowNum << nCurrentRow;

					wxString TLastRow;
					TLastRow = _T("");

					bTgtProcessed = TRUE;
					hstr = gpApp->m_eolStr 
					+PardPlain
					+SintblTgt							// Tgt Style
					+gpApp->m_eolStr;
					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}

					// Output the actual Tgt row text delimited by \cell tags
					for (int count=0; count < (int)tgtList.GetCount(); count++)
					{
						if (!WriteOutputString(f,gpApp->m_tgtEncoding,tgtList.Item(count))) // Tgt text string
						{	
							pProgDlg->Destroy();
							return;
						}
						if (!WriteOutputString(f,gpApp->m_systemEncoding,Tcell))			// \cell delimiter
						{	
							pProgDlg->Destroy();
							return;
						}
					}

					hstr = Trow
					+ gpApp->m_eolStr
					+ gpApp->m_eolStr // whm added second m_eolStr to visuall mark end of row in RTF file
					+PardPlain;
					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}
				}// end of if (bInclTgtLangRow && !TgtProcessed)

				if (bInclGlsLangRow && !bGlsProcessed)
				{
					hstr = PardPlain					// start of hstr output string for this table
					+gpApp->m_eolStr
					+Trowd								// \trowd
					+TtsN+TtrgaphN						// \ts21\trgaph108
					+TStartPos							// \trleft0
					+Tjust								// \trqc if centered table, otherwise ""
					+TRowPrecedence						// \rtlrow or \ltrrow depending on bReverseLayout
					+TRautoFit							// whm added 11Nov07 \trautofit to prevent excessive wrapping
					+TDirection;						// \taprtl if bReverseLayout is TRUE "" otherwise
					// the tags above are output once for a given row

					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}

					// CellDimsNav and CellDimsTgt below contains as many \cellxN items as there are cells
					// in this row, with increasing values for N
					if (gbGlossingUsesNavFont)
					{
						hstr = CellDimsNav;
					}
					else
					{
						hstr = CellDimsTgt;
					}
					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}

					if (bInclNavLangRow || bInclSrcLangRow || bInclTgtLangRow)
						nCurrentRow++;
					TRowNum.Empty();
					TRowNum << nCurrentRow;

					wxString TLastRow;
					TLastRow = _T("");

					bGlsProcessed = TRUE;
					hstr = gpApp->m_eolStr 
					+PardPlain
					+SintblGls							// Gls Style
					+gpApp->m_eolStr;
					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}

					// Output the actual Gls row text delimited by \cell tags
					if (gbGlossingUsesNavFont)
					{
						for (int count=0; count < (int)glsList.GetCount(); count++)
						{
							//if (!WriteOutputString(f,gpApp->m_systemEncoding,*glspos->GetData()))	// Gls text string
							//	return;
							// whm 8Nov07 note: We'll use the tgt encoding for gloss text which
							// forces WriteOutputString to use the \uN\'f3 RTF Unicode char format
							if (!WriteOutputString(f,gpApp->m_tgtEncoding,glsList.Item(count))) // Gls text string
							{	
								pProgDlg->Destroy();
								return;
							}
							if (!WriteOutputString(f,gpApp->m_systemEncoding,Tcell))			// \cell delimiter
							{	
								pProgDlg->Destroy();
								return;
							}
						}
					}
					else
					{
						for (int count=0; count < (int)glsList.GetCount(); count++)
						{
							if (!WriteOutputString(f,gpApp->m_tgtEncoding,glsList.Item(count))) // Gls uses Tgt encoding
							{	
								pProgDlg->Destroy();
								return;
							}
							if (!WriteOutputString(f,gpApp->m_systemEncoding,Tcell))				// \cell delimiter
							{	
								pProgDlg->Destroy();
								return;
							}
						}
					}

					hstr = Trow
					+ gpApp->m_eolStr
					+ gpApp->m_eolStr // whm added second m_eolStr to visuall mark end of row in RTF file
					+PardPlain;
					if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
					{	
						pProgDlg->Destroy();
						return;
					}
				}// end of if (bInclGlsLangRow && !GlsProcessed)
			}// end of else when not glossing

			// added for version 3
			// Process Row 4 Data for 5th row when Free translation is selected in export options
			// for output as a table row. When the export option is selected, bInclFreeTransRow
			// is TRUE.
			if (bInclFreeTransRow && !bFreeTransProcessed)
			{
				hstr = PardPlain					// start of hstr output string for this table
				+gpApp->m_eolStr
				+Trowd								// \trowd
				+TtsN+TtrgaphN						// \ts21\trgaph108
				+TStartPos							// \trleft0
				+Tjust								// \trqc if centered table, otherwise ""
				+TRowPrecedence						// \rtlrow or \ltrrow depending on bReverseLayout
				+TRautoFit							// whm added 11Nov07 \trautofit to prevent excessive wrapping
				+TDirection;						// \taprtl if bReverseLayout is TRUE "" otherwise
				// the tags above are output once for a given row

				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}
				
				// cellDimsFree below contains as many \cellxN items as there are cells in this row, with
				// increasing values for N
				hstr = cellDimsFree;
				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}

				if (bInclNavLangRow || bInclSrcLangRow || bInclTgtLangRow || bInclGlsLangRow)
					nCurrentRow++;
				TRowNum.Empty();
				TRowNum << nCurrentRow;

				wxString TLastRow;
				TLastRow = _T("");

				bFreeTransProcessed = TRUE;
				hstr = gpApp->m_eolStr
				+PardPlain
				+ SintblNav
				+gpApp->m_eolStr // Note: end hstr line after Free Trans Style tag below
				+SintblFree;						// Free Trans Style
				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}

				// Output the actual Free Translation row text delimited by \cell tag(s).
				// Note: The cellxNListFree has a list of cell borders with a minimum of one
				// for the right end of the row that has same \cellxN value as the non-free
				// trans rows above it. cellxNListFree operates parallel with and has the
				// same number of items as freeTransList. When cells do not have \free material,
				// there is only a null string for a given cell in both lists.
				int freepos = 0;
				int cellxNListCount = cellxNListFree.GetCount();
				int freeTransListCount;
				freeTransListCount = freeTransList.GetCount();
				wxASSERT(cellxNListCount == freeTransListCount);
				freeTransListCount = freeTransListCount; // avoid warining 
				for (int count=0; count < cellxNListCount; count++)
				{
					wxString numAtFree = cellxNListFree.Item(count);
					freepos++;
					wxString strAtFree = freeTransList.Item(count);

					if (numAtFree != _T("0") && numAtFree != _T(""))
					{
						if (!WriteOutputString(f,gpApp->m_systemEncoding,Tcell)) // \cell delimiter
						{	
							pProgDlg->Destroy();
							return;
						}
					}
					if (!strAtFree.IsEmpty())
					{
						// Write the strAtFree into this free trans row. If not all of the strAtFree
						// will fit in the remainder of this row, and there is no additional free
						// trans material starting at a subsequent cell in this row nor at the
						// beginning of the following row (whose src phrase has just been read),
						// put any remaining part of the string into the FreeTransSpillOverStr for
						// the next table to process into the beginning of its free translation row.

						wxString numAtFreeEndCell;
						numAtFreeEndCell = _T("0");
						// save our current position in list and scan ahead to find the next non-zero
						// non-null item in the list
						int savepos = freepos;
						while (freepos != (int)cellxNListFree.GetCount() && (numAtFreeEndCell == _T("0") || numAtFreeEndCell.IsEmpty()))
						{
							// scan the cellxNListFree entires until we come to either the
							// end of the list or to any following non-zero, non-null item
							numAtFreeEndCell = cellxNListFree.Item(freepos);
							freepos++;
						}
						// at this point numAtFreeEndCell should contain the cellxN of either the
						// end of the row or the beginning of another free trans item. If freepos
						// is NULL, it signals that we are at the end of the current table's row
						// and we can potentially wrap any excess text over to the next table's
						// free trans row, providing the first cell of that row does not itself
						// start a new free trans element. Since the first source phrase of the
						// next table has just been read (and its extent was too wide to fit the
						// current table), we can just check the status of its bHasFreeMarker. If
						// bHasFreeMarker is FALSE, we can wrap any excess text from strAtFree to
						// the beginning of the next table's free trans row.
						if (freepos == (int)cellxNListFree.GetCount() && !bHasFreeMarker)
						{
							// first, check if the space we have available in the current row
							// is too short for the existing text to fit without wrapping
							int intNumAtFree = wxAtoi(numAtFree); 
							int intNumAtFreeEndCell = wxAtoi(numAtFreeEndCell);
							int extentRemaining = intNumAtFreeEndCell - intNumAtFree - 400;
							// We need to wrap some text to the next table, so we divide up
							// strAtFree into FreeTransFitsInRowStr and FreeTransSpillOverStr.
							// The FreeTransFitsInRowStr we output below, but the
							// FreeTransSpillOverStr will output first in the free trans row
							// of the next table that is created.
							// Ensure dC is selected for the pRtfFreeFnt text font.
							dC.SetFont(*pRtfFreeFnt); // whm 8Nov07 corrected to use rtfFreeFnt
							DivideTextForExtentRemaining(dC, extentRemaining, strAtFree,
								FreeTransFitsInRowStr, FreeTransSpillOverStr);
							// Note: When the DivideTextForExtentRemaining() call above
							// allocates text to FreeTransSpillOverStr, it is fed into
							// the free trans row text for the upcoming table. See below.

						}
						else
						{
							FreeTransFitsInRowStr = strAtFree;
						}
						// return to our last position at numAtFree in the list
						freepos = 0;
						while (freepos != (int)cellxNListFree.GetCount() && freepos != savepos)
						{
							freepos++;
						}
						wxASSERT(freepos == savepos);

						// Free translation uses m_systemEncoding
						// whm 8Nov07 note: We'll use the tgt encoding for Free Trans text which
						// forces WriteOutputString to use the \uN\'f3 RTF Unicode char format
						if (!WriteOutputString(f,gpApp->m_tgtEncoding,FreeTransFitsInRowStr))
						{	
							pProgDlg->Destroy();
							return;
						}
					}

				}

				hstr = Trow
				+ gpApp->m_eolStr
				+ gpApp->m_eolStr // whm added second m_eolStr to visuall mark end of row in RTF file
				+PardPlain;
				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}
			}

			// added for version 3
			// Process Row 5 Data for 6th row when Back translation is selected in export options
			// for output as a table row. When the export option is selected, bInclBackTransRow
			// is TRUE.
			if (bInclBackTransRow && !bBackTransProcessed)
			{
				hstr = PardPlain					// start of hstr output string for this table
				+gpApp->m_eolStr
				+Trowd								// \trowd
				+TtsN+TtrgaphN						// \ts21\trgaph108
				+TStartPos							// \trleft0
				+Tjust								// \trqc if centered table, otherwise ""
				+TRowPrecedence						// \rtlrow or \ltrrow depending on bReverseLayout
				+TRautoFit							// whm added 11Nov07 \trautofit to prevent excessive wrapping
				+TDirection;						// \taprtl if bReverseLayout is TRUE "" otherwise
				// the tags above are output once for a given row

				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}
				
				// cellDimsBack below contains as many \cellxN items as there are cells in this row, with
				// increasing values for N
				hstr = cellDimsBack;
				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}
				
				if (bInclNavLangRow || bInclSrcLangRow || bInclTgtLangRow || bInclGlsLangRow)
					nCurrentRow++;
				TRowNum.Empty();
				TRowNum << nCurrentRow; //_itot(nCurrentRow,rbuf,10);

				wxString TLastRow;
				TLastRow = _T("");

				bBackTransProcessed = TRUE;
				hstr = gpApp->m_eolStr
				+PardPlain
				+ SintblNav
				+gpApp->m_eolStr // Note: end hstr line after Back Trans Style tag below
				+SintblBack;							// Back Trans Style
				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}

				// Output the actual Back Translation row text delimited by \cell tag(s).
				// Note: The cellxNListBT has a list of cell borders with a minimum of one
				// for the right end of the row that has the same \cellxN value as the non-bt
				// trans rows above it. cellxNListBT operates in parallel with and has the
				// same number of items as backTransList. When cells do not have \bt... material,
				// there is only a null string for a given cell in both lists.
				int btpos = 0;
				int cellxNListCount = cellxNListBT.GetCount();
				int backTransListCount;
				backTransListCount = backTransList.GetCount();
				wxASSERT(cellxNListCount == backTransListCount);
				backTransListCount = backTransListCount; // avoid warning
				for (int count=0; count < cellxNListCount; count++)
				{
					wxString numAtBT = cellxNListBT.Item(count);
					btpos++;
					wxString strAtBT = backTransList.Item(count);

					if (numAtBT != _T("0") && numAtBT != _T(""))
					{
						if (!WriteOutputString(f,gpApp->m_systemEncoding,Tcell)) // \cell delimiter
						{	
							pProgDlg->Destroy();
							return;
						}
					}
					if (!strAtBT.IsEmpty())
					{
						// Write the strAtBT into this back trans row. If not all of the strAtBT
						// will fit in the remainder of this row, and there is no additional back
						// trans material starting at a subsequent cell in this row nor at the
						// beginning of the following row (whose src phrase has just been read),
						// put any remaining part of the string into the BackTransSpillOverStr for
						// the next table to process into the beginning of its back translation row.

						wxString numAtBackEndCell;
						numAtBackEndCell = _T("0");
						// save our current position in list and scan ahead to find the next non-zero
						// non-null item in the list
						int savepos = btpos;
						while (btpos != (int)cellxNListBT.GetCount() && (numAtBackEndCell == _T("0") || numAtBackEndCell.IsEmpty()))
						{
							// scan the cellxNListBT entires until we come to either the
							// end of the list or to any following non-zero, non-null item
							numAtBackEndCell = cellxNListBT.Item(btpos);
							btpos++; 
						}
						// At this point numAtBackEndCell should contain the cellxN of either the
						// end of the row or the beginning of another back trans item. If btpos
						// is NULL, it signals that we are at the end of the current table's row
						// and we can potentially wrap any excess text over to the next table's
						// back trans row, providing the first cell of that row does not itself
						// start a new back trans element. Since the first source phrase of the
						// next table has just been read (and its extent was too wide to fit the
						// current table), we can just check the status of its bHasBTMarker. If
						// bHasBTMarker is FALSE, we can wrap any excess text from strAtBT to
						// the beginning of the next table's back trans row.
						if (btpos == (int)cellxNListBT.GetCount() && !bHasBTMarker)
						{
							// First, check if the space we have available in the current row
							// is too short for the existing text to fit without wrapping
							int intNumAtBT = wxAtoi(numAtBT);
							int intNumAtBTEndCell = wxAtoi(numAtBackEndCell);
							int extentRemaining = intNumAtBTEndCell - intNumAtBT - 400;
							// We need to wrap some text to the next table, so we divide up
							// strAtBT into BackTransFitsInRowStr and BackTransSpillOverStr.
							// The BackTransFitsInRowStr we output below, but the
							// BackTransSpillOverStr will output first in the free trans row
							// of the next table that is created.
							// Ensure dC is selected for the pRtfBtFnt font.
							dC.SetFont(*pRtfBtFnt); // whm 8Nov07 corrected to use pRtfBtFnt
							DivideTextForExtentRemaining(dC, extentRemaining, strAtBT,
								BackTransFitsInRowStr, BackTransSpillOverStr);
							// Note: When the DivideTextForExtentRemaining() call above
							// allocates text to BackTransSpillOverStr, it is fed into
							// the back trans row text for the upcoming table. See below.

						}
						else
						{
							BackTransFitsInRowStr = strAtBT;
						}
						// return to our last position at numAtBT in the list
						btpos = 0;
						while (btpos != (int)cellxNListBT.GetCount() && btpos != savepos)
						{
							btpos++;
						}
						wxASSERT(btpos == savepos);

						// whm 8Nov07 note: We'll use the tgt encoding for Back Trans text which
						// forces WriteOutputString to use the \uN\'f3 RTF Unicode char format
						if (!WriteOutputString(f,gpApp->m_tgtEncoding,BackTransFitsInRowStr))
						{	
							pProgDlg->Destroy();
							return;
						}
					}

				}

				hstr = Trow
				+ gpApp->m_eolStr
				+ gpApp->m_eolStr // whm added second m_eolStr to visuall mark end of row in RTF file
				+PardPlain;
				if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
				{	
					pProgDlg->Destroy();
					return;
				}
			}


			// We're at the end of the current table's output, so now output a separating paragraph
			// between the tables to prevent them from fusing together
			hstr = _T("\\pard ") + gpApp->m_eolStr + Sindoc__normal +gpApp->m_eolStr+ _T("{\\par }"+gpApp->m_eolStr); // paragraph between tables
			if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
			{	
				pProgDlg->Destroy();
				return;
			}

			// Note: At the point we have already read the source phrase string data for the first
			// cell of the following table.
			// reset strings to strings just read before ending the table
			SrcTextStr = SrcStr;			// holds text of Src lang row for text metrics
			TgtTextStr = TgtStr;			// holds text of Tgt lang row for text metrics
			GlsTextStr = GlsStr;			// holds text of Gls lang row for text metrics
			NavTextStr = NavStr;			// holds text of Nav lang row for text metrics

			cellxNList.Clear();	// empty the list of N's for \cellxN
			// next two added for version 3
			cellxNListFree.Clear();
			cellxNListBT.Clear();

			srcList.Clear();
			tgtList.Clear();
			glsList.Clear();
			navList.Clear();
			freeTransList.Clear();
			backTransList.Clear();
			//BackTransFitsInRowStr.Empty(); // don't clear this
			//FreeTransFitsInRowStr.Empty(); // don't clear this

			nColsInRow = 0;					// restart at row zero
			
			// Note: when a table is centered it applies to the upcoming table, i.e., this "else" block
			// not the block above which outputs the current/last table 
			if (bCenterTableForCenteredMarker && bNextTableIsCentered)
			{
				bTableIsCentered = TRUE; // center table in margins
			}
			else
			{
				bTableIsCentered = FALSE; // reset now that all rows of current table have been output
			}

			if (OutputLastCols)
				goto b; // b: is exit point to write the last columns of the final table of the data

			wxString cellxN;
			cellxN.Empty();
			cellxN << AccumRowWidth;

			// fill cellxNListFree when we are building a free trans row
			wxString nullStr = _T("");
			if (bInclFreeTransRow)
			{
				if (bHasFreeMarker)
				{
					cellxNListFree.Add(cellxN);
				}
				else
				{
					cellxNListFree.Add(nullStr); // all null string to keep cellxNListFree in sync with freeTransList
				}
			}
			// fill cellxNListBT when we are building a back trans row
			if (bInclBackTransRow)
			{
				if (bHasBTMarker)
				{
					cellxNListBT.Add(cellxN);
				}
				else
				{
					cellxNListBT.Add(nullStr); // all null string to keep cellxNListbt in sync with backTransList
				}
			}

			// Although we go as many columns as we could on the current table, we haven't yet
			// reached the last columns of the final table of data to output so, we start a new
			// table with the current source phrase's data, keeping track of the current data
			// (accumulated width for current column in N and the actual strings to fill the cells
			// with added to the CLists)
			AccumRowWidth += MaxColWidth;

			wxString accStr;
			accStr.Empty();
			accStr << AccumRowWidth;

			// always fill the cellxNList for current Src Lang, Tgt Lang, Gls Lang and Nav Lang data
			// derived from the current source phrase
			cellxNList.Add(accStr); // add N for \cellxN values to retrieve from list later

			// wx note: wxList works with pointers to wxStrings on the heap so must use new in
			// the Append calls below; wxList::DeleteContents(true) then deletes the list's client
			// contents (wxStrings).
			srcList.Add(SrcStr); // collect list of source strings
			tgtList.Add(TgtStr); // collect list of target strings
			glsList.Add(GlsStr); // collect list of gloss strings
			navList.Add(NavStr); // collect list of nav text strings

			// if there is FreeTransSpillOverStr text add it to the FreeStr for the next table
			if (!FreeTransSpillOverStr.IsEmpty())
			{
				if (!FreeTStr.IsEmpty())
				{
					FreeTStr = FreeTransSpillOverStr + _T(' ') + FreeTStr;
				}
				else
				{
					FreeTStr = FreeTransSpillOverStr;
				}
				FreeTransSpillOverStr.Empty();
			}
			FreeTStr.Trim(FALSE); // trim left end
			FreeTStr.Trim(TRUE); // trim right end
			// wx note: wxList works with pointers to wxStrings on the heap so must use new in
			// the Append call below; wxList::DeleteContents(true) then deletes the list's client
			// contents (wxStrings).
			freeTransList.Add(FreeTStr); // collect list of free trans strings (will be list of null
											// strings except for when current src phrase has a \free
											// string in m_markers)

			// if there is BackTransSpillOverStr text add it to the BackStr for the next table
			if (!BackTransSpillOverStr.IsEmpty())
			{
				if (!BackTStr.IsEmpty())
				{
					BackTStr = BackTransSpillOverStr + _T(' ') + BackTStr;
				}
				else
				{
					BackTStr = BackTransSpillOverStr;
				}
				BackTransSpillOverStr.Empty();
			}
			BackTStr.Trim(FALSE); // trim left end
			BackTStr.Trim(TRUE); // trim right end
			// wx note: wxList works with pointers to wxStrings on the heap so must use new in
			// the Append call below; wxList::DeleteContents(true) then deletes the list's client
			// contents (wxStrings).
			backTransList.Add(BackTStr); // collect list of back trans strings (will be list of null
											// strings except for when current src phrase has a \bt...
											// string in m_markers)

			nColsInRow++;


		} // end of if (AccumRowWidth >= MaxRowWidth)
		else
		{
			// there's still room on row so add pile/column to row and keep track of
			// delimited row text

			// Note: when a table is centered it applies to the upcoming table, i.e., this "else" block
			// not the block above which outputs the current/last table 
			if (bCenterTableForCenteredMarker && bNextTableIsCentered)
			{
				bTableIsCentered = TRUE; // center table in margins
			}
			else
			{
				bTableIsCentered = FALSE;
			}

			wxString cellxN;
			cellxN << AccumRowWidth;

			// fill cellxNListFree when we are building a free trans row
			wxString nullStr = _T("");
			if (bInclFreeTransRow)
			{
				if (bHasFreeMarker)
				{
					cellxNListFree.Add(cellxN);
				}
				else
				{
					cellxNListFree.Add(nullStr); // all null string to keep cellxNListFree in sync with freeTransList
				}
			}
			// fill cellxNListBT when we are building a back trans row
			if (bInclBackTransRow)
			{
				if (bHasBTMarker)
				{
					cellxNListBT.Add(cellxN);
				}
				else
				{
					cellxNListBT.Add(nullStr); // all null string to keep cellxNListbt in sync with backTransList
				}
			}

			AccumRowWidth += MaxColWidth;

			cellxN.Empty();
			cellxN << AccumRowWidth;
			cellxNList.Add(cellxN);	// add N for \cellxN values to retrieve from list later

			// wx note: wxList works with pointers to wxStrings on the heap so must use new in
			// the Append calls below; wxList::DeleteContents(true) then deletes the list's client
			// contents (wxStrings).
			srcList.Add(SrcStr); // collect list of source strings
			tgtList.Add(TgtStr); // collect list of target strings
			glsList.Add(GlsStr); // collect list of gloss strings
			navList.Add(NavStr); // collect list of nav text strings

			// if there is FreeTransSpillOverStr text add it to the FreeStr for the next table
			if (!FreeTransSpillOverStr.IsEmpty())
			{
				if (!FreeTStr.IsEmpty())
				{
					FreeTStr = FreeTransSpillOverStr + _T(' ') + FreeTStr;
				}
				else
				{
					FreeTStr = FreeTransSpillOverStr;
				}
				FreeTransSpillOverStr.Empty();
			}
			FreeTStr.Trim(FALSE); // trim left end
			FreeTStr.Trim(TRUE); // trim right end
			// wx note: wxList works with pointers to wxStrings on the heap so must use new in
			// the Append call below; wxList::DeleteContents(true) then deletes the list's client
			// contents (wxStrings).
			freeTransList.Add(FreeTStr); // collect list of free trans strings (will be list of null
											// strings except for when current src phrase has a \free
											// string in m_markers)

			// if there is BackTransSpillOverStr text add it to the BackTStr for the next table
			if (!BackTransSpillOverStr.IsEmpty())
			{
				if (!BackTStr.IsEmpty())
				{
					BackTStr = BackTransSpillOverStr + _T(' ') + BackTStr;
				}
				else
				{
					BackTStr = BackTransSpillOverStr;
				}
				BackTransSpillOverStr.Empty();
			}
			BackTStr.Trim(FALSE); // trim left end
			BackTStr.Trim(TRUE); // trim right end
			// wx note: wxList works with pointers to wxStrings on the heap so must use new in
			// the Append call below; wxList::DeleteContents(true) then deletes the list's client
			// contents (wxStrings).
			backTransList.Add(BackTStr); // collect list of back trans strings (will be list of null
											// strings except for when current src phrase has a \bt...
											// string in m_markers)

			// we're adding piles/columns to existing pile(s)/column(s) in the row
			SrcTextStr += SrcStr;	// holds text of Src lang row for text metrics
			TgtTextStr += TgtStr;	// holds text of Tgt lang row for text metrics
			GlsTextStr += GlsStr;	// holds text of Gls lang row for text metrics
			NavTextStr += NavStr;	// holds text of Nav lang row for text metrics

			nColsInRow++;
		}
	}// end of while there are more SourcePhrases

c:	if (nColsInRow > 0) // c: is jumping off point for when output of Preliminary material
						// we jump here because there may be some columns to be output
	{
		// output the remaining column data
		OutputLastCols = true;
		goto a;			// jump to a: to process the last columns of this output
	}
b:						// b: is exit point to write the last columns of data
	// append last '}' to the RTF file
	hstr = _T('}');
	if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
	{	
		pProgDlg->Destroy();
		return;
	}
	
	// remove the progress indicator window
	pProgDlg->Destroy();

	srcList.Clear();
	tgtList.Clear();
	glsList.Clear();
	navList.Clear();
	freeTransList.Clear();
	backTransList.Clear();

	// Delete font objects used for text metrics to avoid memory leaks.
	// Note: Deletion of these objects could be bypassed in the event of a write error in
	// WriteOutputString which would return to the caller from DoExportInterlinearRTF
	// before reaching this point - with consequent memory leaks. Such occurrences would
	// not happen ordinatily however, so I've not redesigned the function to be strictly
	// preventitive of memory leaks in such situations.
	if (pRtfSrcFnt != NULL) // whm 11Jun12 added NULL test
		delete pRtfSrcFnt;
	pRtfSrcFnt = NULL;
	if (pRtfTgtFnt != NULL) // whm 11Jun12 added NULL test
		delete pRtfTgtFnt;
	pRtfTgtFnt = NULL;
	if (pRtfNavFnt != NULL) // whm 11Jun12 added NULL test
		delete pRtfNavFnt;
	pRtfNavFnt = NULL;
	if (pRtfFreeFnt != NULL) // whm 11Jun12 added NULL test
		delete pRtfFreeFnt;
	pRtfFreeFnt = NULL;
	if (pRtfBtFnt != NULL) // whm 11Jun12 added NULL test
		delete pRtfBtFnt;
	pRtfBtFnt = NULL;

	// return the MapMode to its original setting (probably not required since
	// the current dC goes out of scope when we exit DoExportRTF)
	dC.SetMapMode(SaveMapMode);

	// close the file
	f.Close();

	// wx version note: The dialog is modeless. It will be automatically destroyed when
	// DoExportInterlinearRTF goes out of scope.

	if ((bOutputCVRange || bOutputFinal) && (!bLimitedOutputFound))
	{
		// graciously inform user that there was not Chapter/Verse Range
		// or Final Matter to output
		// IDS_NO_OUTPUT_FOUND
		wxMessageBox(_("No text was found to output in the range you specified. The output file will exist but will be empty."),_T(""),wxICON_INFORMATION | wxOK);
		gpApp->LogUserAction(_T("No text was found to output in the range you specified. The output file will exist but will be empty."));
	}
	
#ifndef __WXGTK__
	// TODO: Determine why the following wxMessageBox() call crashes the application
	// on Linux when navigation protection is ON for _INTERLINEAR_RTF_OUTPUTS. For now
	// I've conditionally compiled the code to omit this informational prompt on the
	// wxGTK port.

	// whm 7Jul11 Note:
	// For protected navigation situations AI determines the actual
	// filename that is used for the export, and the export itself is
	// automatically saved in the appropriate outputs folder. Since the
	// user has no opportunity to provide a file name nor navigate to
	// a random path, we should inform the user at this point of the 
	// successful completion of the export, and indicate the file name 
	// that was used and its outputs folder name and location.
	wxFileName fn2(exportFilename);
	wxString fileNameAndExtOnly = fn2.GetFullName();

	wxString msg = _("The exported file was named:\n\n%s\n\nIt was saved at the following path:\n\n%s");
	msg = msg.Format(msg,fileNameAndExtOnly.c_str(),exportPath.c_str());
	wxMessageBox(msg,_("Export operation successful"),wxICON_INFORMATION | wxOK);

#endif
	gpApp->LogUserAction(_T("Export operation successful"));

	// BEW changed next line 24Jun05 since this doc parameter can't change once the doc is created
	//gbIsUnstructuredData = FALSE; // restore the default value
}// end of DoExportInterlinearRTF()

// whm added 19Jul03 Revised 1Aug2003
// whm revised 1Nov04 to remove special bar code handling
// whm revised October-November 2005 to support USFM 2.04 standard. The routine now
// whm revised November 2007 to improve reliability with Word 2003
// gets marker/style attribute information from the external AI_USFM.xml file
// at program startup.
// BEW no changes needed for support of doc version 5
void DoExportTextToRTF(enum ExportType exportType, wxString exportPath, wxString exportName, wxString& Buffer)
{
	// Buffer contains the entire Source or Target text, depending on value of exportType.
	// The text in Buffer has already been marked up with sfms as it would be output as
	// a .txt file. DoExportTextToRTF's task (after output of RTF header stuff) is to
	// scan the Buffer outputting the language text, and when markers are encountered,
	// output the necessary RTF tags associated with those markers to create the desired
	// formatting
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();

	CStack charStack; // whm added 19Nov10
	CStack* pCharStack = &charStack;
	//CStack paraStack; // whm added 19Nov10
	//CStack* pParaStack = &paraStack;
	static char emptyStr[32];
	memset(emptyStr,0,32); // fill string with nulls

	int nOpeningBraces = 0; // these two used to verify matched pairs of opening and closing curly braces
	int nClosingBraces = 0; // " " " "

	wxFile f; //CStdioFile f;

	if( !f.Open( exportPath, wxFile::write))
	{
	   #ifdef __WXDEBUG__
		  wxLogDebug(_T("Unable to open export source text file\n"));
		  wxMessageBox(_("Unable to open export source text file"),_T(""),wxICON_EXCLAMATION | wxOK);
	   #endif
	  return;
	}

	// Build the RTF file's opening header tags
	// The standard opening header tags
	wxString Hrtf1 = _T("\\rtf1");	// all RTF version specifications of the form 1.x are indicated as 1
	wxString Hcharset = _T("\\ansi"); // the character set used in this RTF markup file

	// wx version: wxWidgets has easier to use facilities for determining the locale and other
	// international data about the user's computer than the MFC version.
	// Note: some of the following may want to eventually move to the App.
	// !!! testing below !!!
	wxLocale locale;
	//int systemLanguage = locale.GetSystemLanguage();				// Tries to detect the user's default language setting.
																	// Returns the wxLanguage value or wxLANGUAGE_UNKNOWN if
																	// the language-guessing algorithm failed.

	//wxFontEncoding systemEncoding = locale.GetSystemEncoding();		// Tries to detect the user's default font
																	// encoding. Returns wxFontEncoding value or
																	// wxFONTENCODING_SYSTEM if it couldn't be determined

	wxString systemEncodingName = locale.GetSystemEncodingName();	// Tries to detect the name of the user's default
																	// font encoding. This string isn't particularly
																	// useful for the application as its form is
																	// platform-dependent and so you should probably
																	// use GetSystemEncoding instead. Returns a user
																	// readable string value or an empty string if it
																	// couldn't be determined.
	wxString Locale = systemEncodingName; // this seems closest with '-' also in DoExportInterlinearRTF
	// !!! testing above !!!

	wxString temp;
	temp = Locale.Mid(Locale.Find('-',0)+1);// get just the codepage number after the '.'
	wxString Hcodepg = _T("\\ansicpg") + temp;
	wxString HUnicodeNumBytes = _T("\\uc1");
	wxString Hdeffont = _T("\\deff0");

	// Build the RTF color map
	MapMkrToColorStr colorMap; //CMapStringToString colorMap;
	// BuildColorTableFromUSFMColorAttributes() below populates the colorMap and
	// also returns the RTF colortable string
	wxString ColorTable = BuildColorTableFromUSFMColorAttributes(colorMap);

	// font table
	wxString OutputFont;
	if (exportType == sourceTextExport)
		OutputFont = _T("\\f2");
	else	// when output Tgt
		OutputFont = _T("\\f3");

	// wxWidgets does not have wxFont methods for determining the charset so I'll arbitrarily
	// set them all to zero (default to ANSI). I think Word and other RTF readers override
	// these values anyway.
	wxString FCharsetSrcNum = _T("0");
	wxString FCharsetTgtNum = _T("0");

	wxString FCharsetNrm = _T("\\fcharset0");	// default to ANSI (0)
	wxString FCharsetSrc = _T("\\fcharset") +FCharsetSrcNum;
	wxString FCharsetTgt = _T("\\fcharset") +FCharsetTgtNum;
	wxString FPitchNrm = _T("\\fprq2 ");// Normal default to variable pitch
	wxString FPitchSrc;
	wxString FPitchTgt;
	// wx version: wxWidgets doesn't handle font pitch, so we'll arbirarily set all to wxFONTFAMILY_DEFAULT
	// which in RTF is _T("\\fprq0 ")
	FPitchSrc = _T("\\fprq0 ");
	FPitchTgt = _T("\\fprq0 ");
	wxString FFamNrm;
	wxString FFamSrc;
	wxString FFamTgt;
#ifdef _RTL_FLAGS
	if (gpApp->m_bSrcRTL)
		FFamSrc = _T("\\fbidi");
	if (gpApp->m_bTgtRTL)
		FFamTgt = _T("\\fbidi");
#else
	FFamNrm = _T("\\fswiss"); // Arial is fswiss and is our default font


	switch(gpApp->m_pSourceFont->GetFamily())
	{
		case wxFONTFAMILY_ROMAN: FFamSrc = _T("\\froman");break;
		case wxFONTFAMILY_SWISS: FFamSrc = _T("\\fswiss");break;
		case wxFONTFAMILY_MODERN: FFamSrc = _T("\\fmodern");break;
		case wxFONTFAMILY_SCRIPT: FFamSrc = _T("\\fscript");break;
		case wxFONTFAMILY_DECORATIVE: FFamSrc = _T("\\fdecor");break;
		case wxFONTFAMILY_DEFAULT: FFamSrc = _T("\\fnil");break;
		default: FFamSrc = _T("\\fnil");
	}

	switch(gpApp->m_pTargetFont->GetFamily())
	{
		case wxFONTFAMILY_ROMAN: FFamTgt = _T("\\froman");break;
		case wxFONTFAMILY_SWISS: FFamTgt = _T("\\fswiss");break;
		case wxFONTFAMILY_MODERN: FFamTgt = _T("\\fmodern");break;
		case wxFONTFAMILY_SCRIPT: FFamTgt = _T("\\fscript");break;
		case wxFONTFAMILY_DECORATIVE: FFamTgt = _T("\\fdecor");break;
		case wxFONTFAMILY_DEFAULT: FFamTgt = _T("\\fnil");break;
		default: FFamTgt = _T("\\fnil");
	}
#endif
	// Font numbers
	wxString FNumNrm = _T("0");
	// Font Face Names
	wxString FNameNrm = _T("Arial");// our default font name
	// Get font face names stored on the App
	wxString FNameSrc = gpApp->m_pSourceFont->GetFaceName(); 
	wxString FNameTgt = gpApp->m_pTargetFont->GetFaceName(); 
	// Build final font tag strings - enclosed in {}
	wxString FTagsNrm = _T("{\\f")
						+FNumNrm+FFamNrm+FCharsetNrm+FPitchNrm+FNameNrm
						+_T(";}");
	wxString FTagsSrc = _T("{\\f2")
						+FFamSrc+FCharsetSrc+FPitchSrc+FNameSrc
						+_T(";}");
	wxString FTagsTgt = _T("{\\f3")
						+FFamTgt+FCharsetTgt+FPitchTgt+FNameTgt
						+_T(";}");

	/////////////////////STYLES//////////////////////////////
	// RTF code words for our desired output style (S...) properties
	// We include all the styles as set up in the PNG Scripture Word Template

	bool bReverseLayout = FALSE;			// False unless set otherwise when _RTL_FLAGS activated
	//bool bReverseLayout = TRUE;			// uncomment for testing RTL layout without _RTL_FLAGS activated

	// RTF code word for paragraph alignment (defaults to left \ql)
	// Assume that for RTL languages they want paragraph alignment to right \qr
	wxString SParaAlignNrm;
	wxString SParaAlignSrc;
	wxString SParaAlignTgt;
#ifdef _RTL_FLAGS
	bReverseLayout = gbRTL_Layout;			// local flag for RTL layout takes value of
											// global flag when _RTL_FLAGS is active
#else
	bReverseLayout = FALSE;
#endif

	wxString Sltr_precedenceNrm;
	wxString Sltr_precedenceSrc;
	wxString Sltr_precedenceTgt;
#ifdef _RTL_FLAGS
	// paragraph display precedence default to \ltrpar
	// use \rtlpar for paragraphs displayed with RTL precedence
	if (bReverseLayout)									// Normal text follows bReverseLayout value
		Sltr_precedenceNrm = _T("\\rtlpar");
	else
		Sltr_precedenceNrm = _T("\\ltrpar");

	if (gpApp->m_bSrcRTL)
		Sltr_precedenceSrc = _T("\\rtlpar");
	else
		Sltr_precedenceSrc = _T("\\ltrpar");
	if (gpApp->m_bTgtRTL)
		Sltr_precedenceTgt = _T("\\rtlpar");
	else
		Sltr_precedenceTgt = _T("\\ltrpar");
#else
	Sltr_precedenceNrm = _T("\\ltrpar");
	Sltr_precedenceSrc = _T("\\ltrpar");
	Sltr_precedenceTgt = _T("\\ltrpar");
#endif
	wxString Sltr_precedence;
	if (exportType == sourceTextExport)
		Sltr_precedence = Sltr_precedenceSrc;
	else	// when output Tgt
		Sltr_precedence = Sltr_precedenceTgt;

	wxString Sltr_precedenceFtr = _T("\\ltrpar");

	int MaxRowWidth;
	wxString Landscape;
	wxString PageWidth;
	wxString PageLength;
	wxString MarginTop;
	wxString MarginBottom;
	wxString MarginLeft;
	wxString MarginRight;
	bool bUsePortrait = (gpApp->GetPageOrientation()== 1);
	if (bUsePortrait)
	{
		// 9024 twips for A4 portrait
		// Bruce has already converted page dimensions to 1000ths of an inch, so we multiply
		// by 1.44 to get twips for these
		MaxRowWidth = (int)(((float)gpApp->m_pageWidth - (float)gpApp->m_marginLeft - (float)gpApp->m_marginRight) *1.44);
		Landscape = _T("");
		PageWidth = _T("\\paperw");
		PageWidth << (int)(gpApp->m_pageWidth*1.44);
		PageLength = _T("\\paperh");
		PageLength << (int)(gpApp->m_pageLength*1.44);
		MarginTop = _T("\\margt");
		MarginTop << (int)(gpApp->m_marginTop*1.44);
		MarginBottom = _T("\\margb");
		MarginBottom << (int)(gpApp->m_marginBottom*1.44);
		MarginLeft = _T("\\margl");
		MarginLeft << (int)(gpApp->m_marginLeft*1.44);
		MarginRight = _T("\\margr");
		MarginRight << (int)(gpApp->m_marginRight*1.44);
	}
	else	// landscape
	{
		// 13956 twips for A4 landscape
		MaxRowWidth = (int)(((float)gpApp->m_pageLength - (float)gpApp->m_marginLeft - (float)gpApp->m_marginRight) *1.44);
		Landscape = _T("\\lndscpsxn");
		PageWidth = _T("\\paperw");
		PageWidth << (int)(gpApp->m_pageLength*1.44);	// swap Width with Length
		PageLength = _T("\\paperh");
		PageLength << (int)(gpApp->m_pageWidth*1.44);	// swap Width with Length
		MarginTop = _T("\\margt");
		MarginTop << (int)(gpApp->m_marginLeft*1.44);	// swap Top with Left
		MarginBottom = _T("\\margb");
		MarginBottom << (int)(gpApp->m_marginRight*1.44);// swap Bottom with Right
		MarginLeft = _T("\\margl");
		MarginLeft << (int)(gpApp->m_marginTop*1.44);	// swap Left with Top
		MarginRight = _T("\\margr");
		MarginRight << (int)(gpApp->m_marginBottom*1.44);// swap Right with Bottom
	}

	int CTabPos = MaxRowWidth / 2;
	int RTabPos = MaxRowWidth;
	wxString TCenterN;
	TCenterN << CTabPos;		// Center tab position in twips from left margin
	wxString TRightN;
	TRightN << RTabPos; 		// Right tab position in twips from left margin
	wxString TabCenter;
	TabCenter << _T("\\tqc\\tx") << TCenterN;
	wxString TabRight;
	TabRight << _T("\\tqr\\tx") << TRightN;

	// Build final style tag strings - enclosed in {}
	// Style information is used in two ways in our RTF output of Src or Tgt text:
	// 1. Style Definitions in the RTF Header
	// 2. In-Document style specifier strings

	// The composition of each of the above varies but all share some parts in common
	// For efficiency, we will build the styles in parts, and use these parts to build
	// the two types of style statements

	// rtfTagsMap is a Standard Template Library (STL) map declared in the View's global space
	// as follows:
	//std::map < wxString, wxString > rtfTagsMap;
	//typedef std::pair <const wxString, wxString > MkrTagStr_Pair;
	//std::map < wxString, wxString > :: const_iterator rtfIter;
	// The map "key" is the standard format marker (without backslash).
	// The map "value" is the in-document RTF style tags.
	// The rtfTagsMap is used here in DoExportSrcOrTgtRTF and in DoExportInterlinearRTF
	rtfTagsMap.clear(); // empty map of all elements

	wxArrayString StyleDefStrArray;
	wxArrayString StyleInDocStrArray;

	BuildRTFTagsMap(StyleDefStrArray,StyleInDocStrArray,OutputFont,colorMap,Sltr_precedence);

	//////////////////// SPECIAL CHAPTER LABELS /////////////////////////
	// According to the USFM docs, the \cl marker defines the chapter "label" to be used when
	// the chosen publishing presentation will render chapter divisions as headings, and not
	// drop cap numerals. The docs also say, "If \cl is entered once before chapter 1 (\c 1)
	// it represents the text for "chapter" to be used throughout the current book. If \cl is
	// used after each individual chapter marker, it represents the particular text to be used
	// for the display of the current chapter heading (usually done if numbers are being
	// presented as words, not numerals)."
	// We assign any \cl assoc text to chapterLabel here which establishes the global chapter
	// label for the book. If the user also chooses to use other \cl markers to assign label
	// text after individual chapter markers, we will assume that such text will only replace
	// this global chapter label text for the individual chapters where it is used (determined
	// in the IsChapterMarker block below).
	wxString chapterLabel = _T("");
	int poscl = Buffer.Find(_T("\\cl ")); // include space
	int posc = Buffer.Find(_T("\\c ")); // include space
	bool usingChapterLabels;
	// Use of chapter labels will interfere with the formatting of chapter numbers in running
	// headers so we'll set a usingChapterLabels flag to inform the header formatting routines
	// below.
	wxString gutterSize;
	if (poscl != -1)
	{
		usingChapterLabels = TRUE;
		gutterSize.Empty(); // do not specify a gutter size when using chapter labels and generic headers
	}
	else
	{
		usingChapterLabels = FALSE;
		gutterSize = _T("\\gutter360"); // 360 is .25 inch gutter

	}
	int posclend;
	int lenBuff = Buffer.Length();
	if (poscl != -1 && posc != -1)
	{
		// both \cl and \c exist
		if (poscl < posc)
		{
			// The first instance of \cl found is before the first instance of \c, therefore,
			// we assume \cl is being used as a label for "chapter" throughout the current book.
			// Determine the end of its assoc text, extract it, and then delete it from the
			// Buffer since it will be handled separately in output (in the IsChapterMarker block
			// below.
			poscl += 4; // point past "\cl "
			posclend = poscl;
			while (posclend < lenBuff && Buffer[posclend] != _T('\\'))
			{
				posclend++;
			}
			chapterLabel = Buffer.Mid(poscl, posclend - poscl);
			// Since we now have the chapter label text, remove the \cl marker and assoc text
			// from the Buffer so it won't be output as body text in RTF printouts.
			Buffer.Remove(poscl, posclend - poscl);
			chapterLabel.Replace(_T("\n"),_T("")); //chapterLabel.Remove(_T('\n'));
			chapterLabel.Replace(_T("\r"),_T(""));
		}
		// if poscl > posc we assume that there will not be a global chapter label, and the
		// user will have a \cl marker after each instance of "\c n" in the text.
	}
	// Note: If either \cl or \c is absent from Buffer, we leave chapterLabel as an empty string.
	// If Buffer has \cl but no \c markers, the \cl marker and its assoc text will not be deleted
	// from the Buffer, but will appear in the output.

	// Note: Because of the Delete operations, the indices for Find operations above should not
	// be used below this point without reinitializing them.

	//////////////////// HEADER and FOOTER Definition Tags //////////////
	wxString HeaderTags = _T("");	// whole header including any left and right parts assembled here
	wxString h_header = _T("");		// for any \h running header "h - File - Header style
	wxString h1_header = _T("");		// for any \h1 running header "h1 - File - Header style
	wxString h2_leftHeader = _T("");	// for any \h2 marker defining "h2 - File - Left Header" style
	wxString h3_rightHeader = _T("");// for any \h3 marker defining "h3 - File - Right Header" style
	// locate and extract any associated text for \h, \h1, \h2, and \h3 residing in m_markers
	int posh, posh1,posh2, posh3, poshEnd, posh1End, posh2End, posh3End;

	// extract any specified header texts
	int lenBuffer = Buffer.Length();
	posh = Buffer.Find(_T("\\h "));
	if (posh != -1)
	{
		posh += 3; // point past "\h "
		poshEnd = posh;
		while (poshEnd < lenBuffer && Buffer[poshEnd] != _T('\\'))
		{
			poshEnd++;
		}
		h_header = Buffer.Mid(posh, poshEnd - posh);
		// Since we now have the header text, remove the header marker and assoc text from the Buffer
		// so it won't be output as body text in RTF printouts.
		posh -= 3; // point back at \h
		Buffer.Remove(posh, poshEnd - posh);
		h_header.Replace(_T("\n"),_T(""));
		h_header.Replace(_T("\r"),_T(""));
	}
	posh1 = Buffer.Find(_T("\\h1 "));
	lenBuffer = Buffer.Length();
	if (posh1 != -1)
	{
		posh1 += 4; // point past "\h1 "
		posh1End = posh1;
		while (posh1End < lenBuffer && Buffer[posh1End] != _T('\\'))
		{
			posh1End++;
		}
		h1_header = Buffer.Mid(posh1, posh1End - posh1);
		// Since we now have the header text, remove the header  marker and assoc text from the Buffer
		// so it won't be output as body text in RTF printouts.
		posh1 -= 4; // point back at \h1
		Buffer.Remove(posh1, posh1End - posh1);
		h1_header.Replace(_T("\n"),_T(""));
		h1_header.Replace(_T("\r"),_T(""));
	}
	posh2 = Buffer.Find(_T("\\h2 "));
	lenBuffer = Buffer.Length();
	if (posh2 != -1)
	{
		posh2 += 4; // point past "\h2 "
		posh2End = posh2;
		while (posh2End < lenBuffer && Buffer[posh2End] != _T('\\'))
		{
			posh2End++;
		}
		h2_leftHeader = Buffer.Mid(posh2, posh2End - posh2);
		// Since we now have the header text, remove the header  marker and assoc text from the Buffer
		// so it won't be output as body text in RTF printouts.
		posh2 -= 4; // point back at \h2
		Buffer.Remove(posh2, posh2End - posh2);
		h2_leftHeader.Replace(_T("\n"),_T(""));
		h2_leftHeader.Replace(_T("\r"),_T(""));
	}
	posh3 = Buffer.Find(_T("\\h3 "));
	lenBuffer = Buffer.Length();
	if (posh3 != -1)
	{
		posh3 += 4; // point past "\h3 "
		posh3End = posh3;
		while (posh3End < lenBuffer && Buffer[posh3End] != _T('\\'))
		{
			posh3End++;
		}
		h3_rightHeader = Buffer.Mid(posh3, posh3End - posh3);
		// Since we now have the header text, remove the header  marker and assoc text from the Buffer
		// so it won't be output as body text in RTF printouts.
		posh3 -= 4; // point back at \h3
		Buffer.Remove(posh3, posh3End - posh3);
		h3_rightHeader.Replace(_T("\n"),_T(""));
		h3_rightHeader.Replace(_T("\r"),_T(""));
	}
	// Note: Because of the Delete operations, the indices for Find operations above should not
	// be used below this point without reinitializing them.


	wxString FooterTags;
	wxString exportFilename = gpApp->m_curOutputFilename;
	wxString BookName = exportFilename;
	int flen = BookName.Length();
	BookName.Remove(flen-4,4); // remove the .adt or xml extension (including the .)
	wxString ChVsRange = _T("");
	wxString LangName;
	if (exportType == sourceTextExport)
		LangName = gpApp->m_pKB->m_sourceLanguageName;
	else
		LangName = gpApp->m_pKB->m_targetLanguageName;

	wxString Sindoc_header;
	rtfIter = rtfTagsMap.find(_T("_header"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the _header marker in the map
		Sindoc_header = (wxString)rtfIter->second;
	}

	wxString facingPage;
	wxString titlePage;
	// Build the RTF header tags using specified header text if any, otherwise use language name
	// and bookname.
	if (!usingChapterLabels
		&& (!h_header.IsEmpty() || !h1_header.IsEmpty() || !h2_leftHeader.IsEmpty() || !h3_rightHeader.IsEmpty()))
	{
		// user has specified some header text via \h, \h1, \h2, and/or \h3 so use what user
		// has specified
		if (!h1_header.IsEmpty() || !h2_leftHeader.IsEmpty() || !h3_rightHeader.IsEmpty())
		{
			// give priority over any \h1, \h2, and \h3 specifications, ignoring any \h content
			facingPage.Empty();
			titlePage.Empty();
			HeaderTags.Empty();

			HeaderTags = gpApp->m_eolStr
						+ _T("{\\header \\pard\\plain ")
						+ gpApp->m_eolStr
						+ Sindoc_header
						+ gpApp->m_eolStr
						+ _T("{")
						+ GetANSIorUnicodeRTFCharsFromString(h2_leftHeader)
						+ _T("\\tab ")
						+ GetANSIorUnicodeRTFCharsFromString(h1_header)
						+ _T("\\tab ")
						+ GetANSIorUnicodeRTFCharsFromString(h3_rightHeader)
						+_T("\\par }}");
		}
		else
		{
			// The user only specified \h, so interpret it in the classical (PNG) style where \h defines
			// the BookName which is placed in the RTF header and is followed by a space and an
			// indication of the chapter(s), i.e., "Mak 2,3" that are included on the current page of
			// text. The macros to accomplish this were implemented by N. Doelman of the Suriname Branch,
			// and later adapted for PNG by David Bevan for the 1998 PNG stylesheet. Without reinventing
			// the wheel, I'm borrowing their complicated set of fields that test for the presence
			// and placement of bookmarks (inserted by macro as Roman Numerals I, II, III, IV, etc.)
			// at the end of each chapter (except that the first chapter's bookmark "I" is placed at
			// the beginning of any intro material preceding the first chapter). In the 1998 stylesheet
			// macro called "CreateScriptureHeaders", the header is formatted with page numbers by
			// default at the outside margin and the header (book name and chapter(s)) at the center
			// tab position in the odd/even header pages. The original macro presented a dialog enabling
			// the user to adjust Header Margin, whether there should be a line below the header, whether
			// page number in header is located at outside margin, inside page, or none; and whether the
			// chapter number format should be arabic, roman numeral upper case, or roman numeral
			// lower case. Here we use the defaults of placing the page number at outside margin and
			// use of arabic number format. The other defaults we ignore.

			// The RTF \facingp tag makes the document have odd and even page headers/footers.
			facingPage = _T("\\facingp");
			// The RTF \titlepg tag makes the current section of the document have a different first
			// page (and hence also different header/footer that can be defined using the \headerf and
			// \footerf tags.
			titlePage = _T("\\titlepg");

			// format the even (left) header
			HeaderTags.Empty();
			HeaderTags = gpApp->m_eolStr;
			HeaderTags += _T("{\\headerl ");
			HeaderTags += Sltr_precedence + _T("\\pard\\plain ");
			HeaderTags += Sltr_precedence;
			HeaderTags += Sindoc_header;
			HeaderTags += _T("{\\field{\\*\\fldinst { PAGE }}{\\fldrslt {}}}");
			HeaderTags += _T("{\\tab "); // tab over to center tab for header and chapter number(s)
			HeaderTags += GetANSIorUnicodeRTFCharsFromString(h_header) + _T("  }");
			// the following field calculations determine the chapter(s) on the current page
			HeaderTags += _T("{\\field{\\*\\fldinst { IF }{\\field{\\*\\fldinst { PageRef }");
			HeaderTags += _T("{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l \\\\* Roman }}{\\fldrslt {}}}{ }}");
			HeaderTags += _T("{\\fldrslt {}}}{<}{\\field{\\*\\fldinst { Page }}{\\fldrslt {}}}{ \"}");
			HeaderTags += _T("{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\* arabic }}{\\fldrslt {}}}{\" \"}");
			HeaderTags += _T("{\\field{\\*\\fldinst { IF }{\\field{\\*\\fldinst { PageRef }");
			HeaderTags += _T("{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\* Roman }}{\\fldrslt {}}}{ }}");
			HeaderTags += _T("{\\fldrslt {}}}{<}{\\field{\\*\\fldinst { Page }}{\\fldrslt {}}}{ \"}");
			HeaderTags += _T("{\\field\\flddirty{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\* arabic }}{\\fldrslt }}");
			HeaderTags += _T("{\\field\\flddirty{\\*\\fldinst { IF }{\\field\\flddirty{\\*\\fldinst { StyleRef \"c - Chapter Number\" }}{\\fldrslt }}");
			HeaderTags += _T("{<}{\\field\\flddirty{\\*\\fldinst { =}{\\field\\flddirty{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l  }}{\\fldrslt }}");
			HeaderTags += _T("{-1 }}}{ \"-}");
			HeaderTags += _T("{\\field\\flddirty{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l \\\\* arabic }}{\\fldrslt }}");
			HeaderTags += _T("{\" \", }{\\field\\flddirty{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l \\\\* arabic }}{\\fldrslt }}");
			HeaderTags += _T("{\" }}}{\" \"}");
			HeaderTags += _T("{\\field{\\*\\fldinst { =}{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\"  }}{\\fldrslt {}}}");
			HeaderTags += _T("{-1 \\\\* arabic }}{\\fldrslt {}}}");
			HeaderTags += _T("{\\field{\\*\\fldinst { IF }{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\" }}{\\fldrslt {}}}");
			HeaderTags += _T("{<}{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l  }}{\\fldrslt {}}}");
			HeaderTags += _T("{ \"-}{\\field\\flddirty{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l \\\\* arabic }}");
			HeaderTags += _T("{\\fldrslt }}{\" \", }{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l \\\\* arabic }}");
			HeaderTags += _T("{\\fldrslt {}}}{\" }}{\\fldrslt {}}}{\" }}{\\fldrslt {}}}	{\" }}{\\fldrslt {}}}");
			HeaderTags += _T("{\\par }}");

			// format the odd (right) header
			HeaderTags += gpApp->m_eolStr;
			HeaderTags += _T("{\\headerr ");
			HeaderTags += Sltr_precedence + _T("\\pard\\plain ");
			HeaderTags += Sltr_precedence;
			HeaderTags += Sindoc_header;
			HeaderTags += _T("{\\tab "); // tab over to center tab position for header and chapter number(s)
			HeaderTags += GetANSIorUnicodeRTFCharsFromString(h_header) + _T("  }");
			HeaderTags += _T("{\\field{\\*\\fldinst { IF }{\\field{\\*\\fldinst { PageRef }");
			HeaderTags += _T("{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l \\\\* Roman }}{\\fldrslt {}}}{ }}");
			HeaderTags += _T("{\\fldrslt {}}}{<}{\\field{\\*\\fldinst { Page }}{\\fldrslt {}}}{ \"}");
			HeaderTags += _T("{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\* arabic }}{\\fldrslt {}}}{\" \"}");
			HeaderTags += _T("{\\field{\\*\\fldinst { IF }{\\field{\\*\\fldinst { PageRef }");
			HeaderTags += _T("{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\* Roman }}{\\fldrslt {}}}{ }}");
			HeaderTags += _T("{\\fldrslt {}}}{<}{\\field{\\*\\fldinst { Page }}{\\fldrslt {}}}{ \"}");
			HeaderTags += _T("{\\field\\flddirty{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\* arabic }}{\\fldrslt }}");
			HeaderTags += _T("{\\field\\flddirty{\\*\\fldinst { IF }{\\field\\flddirty{\\*\\fldinst { StyleRef \"c - Chapter Number\" }}{\\fldrslt }}");
			HeaderTags += _T("{<}{\\field\\flddirty{\\*\\fldinst { =}{\\field\\flddirty{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l  }}{\\fldrslt }}");
			HeaderTags += _T("{-1 }}}{ \"-}");
			HeaderTags += _T("{\\field\\flddirty{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l \\\\* arabic }}{\\fldrslt }}");
			HeaderTags += _T("{\" \", }{\\field\\flddirty{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l \\\\* arabic }}{\\fldrslt }}");
			HeaderTags += _T("{\" }}}{\" \"}");
			HeaderTags += _T("{\\field{\\*\\fldinst { =}{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\"  }}{\\fldrslt {}}}");
			HeaderTags += _T("{-1 \\\\* arabic }}{\\fldrslt {}}}");
			HeaderTags += _T("{\\field{\\*\\fldinst { IF }{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\" }}{\\fldrslt {}}}");
			HeaderTags += _T("{<}{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l  }}{\\fldrslt {}}}");
			HeaderTags += _T("{ \"-}{\\field\\flddirty{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l \\\\* arabic }}");
			HeaderTags += _T("{\\fldrslt }}{\" \", }{\\field{\\*\\fldinst { StyleRef \"c - Chapter Number\" \\\\l \\\\* arabic }}");
			HeaderTags += _T("{\\fldrslt {}}}{\" }}{\\fldrslt {}}}{\" }}{\\fldrslt {}}}	{\" }}{\\fldrslt {}}}");
			// the \headerl tag seems to sometimes automatically insert a tab within the above field, which in
			// addition to the tab below causes the page field to go too far into right margin.
			HeaderTags += _T("{\\tab }");
			// page number goes at left margin
			HeaderTags += _T("{\\field{\\*\\fldinst { PAGE }}{\\fldrslt {}}}");
			HeaderTags += _T("{\\par }}");

			// first page header is empty
			HeaderTags += gpApp->m_eolStr;
			HeaderTags += _T("{\\headerf ");
			HeaderTags += Sltr_precedence + _T("\\pard\\plain ");
			HeaderTags += Sltr_precedence + _T(" \\par}") + gpApp->m_eolStr;
		}
	}
	else
	{
		// user specified no header text so compose one based on language names and bookname
		// we also chose this header format if chapter labels via \cl marker are used since
		// their presence would foul up formatting of chapter numbers in headers
		facingPage.Empty();
		titlePage.Empty();
		HeaderTags.Empty();

		HeaderTags = gpApp->m_eolStr
						+ _T("{\\header \\pard\\plain ")
						+ gpApp->m_eolStr
						+ Sindoc_header
						+ gpApp->m_eolStr
						+ _T("{\\tab ")
						+GetANSIorUnicodeRTFCharsFromString(LangName)
						+_T(" ")
						+ GetANSIorUnicodeRTFCharsFromString(BookName)
						//+_T(" ")
						//+ChVsRange
						+_T("\\par }}");
	}

	// whm Note 26Oct07:
	// We cannot use the EscapeAnyEmbeddedRTFControlChars() function on Buffer here, 
	// because EscapeAnyEmbeddedRTFControlChars() expects its imput buffer to be text 
	// associated with markers, but not the actual markers themselves (the Buffer 
	// here in DoExportSrcOrTgtRTF is the full text including the actual markers). 
	// EscapeAnyEmbeddedRTFControlChars is suitable for DoExportInterlinearRTF at various 
	// places where we know we are dealing with non-marker text, but not here where we 
	// have the whole Buffer. Another place to consider escaping embedded \, {, and } 
	// characters is in the ApplyOutputFilterToText() function which is called on the 
	// Buffer text before it is passed to DoExportSrcOrTgtRTF. Before the 26Oct07 changes, 
	// ApplyOutputFilterToText() already had code to escape the curly brace characters, 
	// but it did not have code to detect non-marker backslash codes and escape them.

	//////////////////// DOCUMENT LEVEL TAGS ///////////////////////////
	// whm added 26Oct07
	// Different versions of Word (especially Word 2003) are sensitive to whether or not the
	// \fetN control word is used, and what value N has. Up to now this flag has merely been
	// hard coded into the RTF header as \fet1, which seemed to make Word 2002 happy. But
	// Word 2003 is sensitive to the 
	// The RTF Specs all say about \fetN:
	// "Footnote/endnote type. This indicates what type of notes are present in the document.
	//   0	Footnotes only or nothing at all (the default)
	//   1	Endnotes only
	//   2	Both footnotes and endnotes
	// For backward compatibility, if \fet1 is emitted, \endnotes or \enddoc will be emitted 
	// along with \aendnotes or \aenddoc. RTF readers that understand \fet will need to 
	// ignore the footnote-positioning control words and use the endnote control words instead."
	// The following code should at least make the value of N conform to the RTF specifications, 
	// and hopefully avoid some of the situations where a particular version of Word is unable 
	// to open the RTF file. See the options for setting N of the \fetN control word in comments above.
	//
	// whm 26Oct07 added check for footnotes and endnotes to process the \fetN control word properly 
	// (see composition of Doctags below and comments in DoExportInterlinearRTF).
	bool bDocHasFootnotes = FALSE;	// assume no footnotes unless found in while loop below
	bool bDocHasEndnotes = FALSE;	// assume no endnotes unless found in while loop below
	bool bDocHasFreeTrans = FALSE;	// assume no free translation unless found in while loop below
	bool bDocHasBackTrans = FALSE;	// assume no back translation unless found in while loop below
	bool bDocHasAINotes = FALSE;	// assume no AI notes unless found in while loop below

	// whm 5Nov07 added the following function to determine if the output Buffer (which has already
	// had the ApplyOutputFilterToText() called on it) has any destination text that the test below
	// will determine what value of N to assign to the problematic \fetN control word.
	DetermineRTFDestinationMarkerFlagsFromBuffer(Buffer,
		bDocHasFootnotes,
		bDocHasEndnotes,
		bDocHasFreeTrans,
		bDocHasBackTrans,
		bDocHasAINotes);

	wxString fetNstr;
	if ((!bDocHasFootnotes && !bDocHasEndnotes) || (bDocHasFootnotes &&  !bDocHasEndnotes)) // has Footnotes or neither
		fetNstr = _T("\\fet0");
	else if (!bDocHasFootnotes && bDocHasEndnotes) // has Endnotes only
		fetNstr = _T("\\fet1");
	else
		fetNstr = _T("\\fet2"); // has both Footnotes and Endnotes

	// combine document parameter tags with other doc level tags
	wxString DocInfo = _T("{\\*\\generator Adapt It 3.x;}");
	wxString DocTags = DocInfo+PageWidth+PageLength+MarginTop+MarginBottom+MarginLeft+MarginRight
					+ gutterSize
					+ facingPage
					+ gpApp->m_eolStr
					+ _T("\\horzdoc\\viewkind1\\viewscale100\\nolnhtadjtbl\\ftnbj")
					+ fetNstr // whm modified 5Nov07
					+ _T("\\sectd ")+Landscape
					+ titlePage;
	// Added \fet1 (format footnotes as endnotes) here for safety even though it doesn't make sense
	// to do so (and Word doesn't turn them into endnotes). In the Interlinear RTF output Word would
	// crash under certain circumstances without the presence of \fet1 here (see note in the
	// Interlinear routine).
	// The \titlepg command needs to follow the \sectd command otherwise the \sectd seems to nullify
	// the effect of \titlepg making the first page header different (i.e., no header content in
	// our case).

	wxString DocName = exportName;

	// Note: The original MFC version (before 3.5.2) used CTime::GetLocalTm which changed in
	// behavior and required rewriting to compile and run on VS 2005. The rewrite there was
	// similar to the wx code below. (see note below).
	wxDateTime theTime = wxDateTime::Now();
	wxString DateTime = theTime.Format(_T("%a, %b %d, %H:%M, %Y")).c_str();
	//wxString DateTime = theTime.Format(_T("%c")).c_str();
	// Note: wxDateTime::Format could simply use the ASNI C strftime %c conversion specifier which 
	// is commented out above. Doing so would give the preferred date and time representation for 
	// the current locale. The format actually used above mimics that used in the MFC version.

	wxString Sindoc_footer;
	rtfIter = rtfTagsMap.find(_T("_footer"));
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for the _footer marker in the map
		Sindoc_footer = (wxString)rtfIter->second;
	}

	// Build RTF footer tags. The footers need to coordinate with the type of headers specified.
	if (!usingChapterLabels
		&& (!h_header.IsEmpty() || !h1_header.IsEmpty() || !h2_leftHeader.IsEmpty() || !h3_rightHeader.IsEmpty()))
	{
		// user has specified some header text via \h, \h1, \h2, and/or \h3 so make footers
		// agree with what user has specified for headers.
		if (!h1_header.IsEmpty() || !h2_leftHeader.IsEmpty() || !h3_rightHeader.IsEmpty())
		{
			// Give priority over any \h1, \h2, and \h3 specifications, ignoring any \h content
			// The \h1, \h2 and \h3 header formats do not include a provision for page numbers,
			// so we'll include the page number with DocName and DateTime in the footer
			FooterTags = gpApp->m_eolStr
								+ _T("{\\footer \\pard\\plain ")
								+ gpApp->m_eolStr
								+ Sindoc_footer
								+ gpApp->m_eolStr
								+ _T("{")
								+ GetANSIorUnicodeRTFCharsFromString(DocName)
								+ _T(" \\tab}")
								+ gpApp->m_eolStr
								+ _T("{\\field {\\*\\fldinst {PAGE}} {\\fldrslt{}}}")
								+ gpApp->m_eolStr
								+ _T("{\\tab ")+DateTime+_T("\\par}}");
		}
		else
		{
			// The user only specified the classical \h header which gets the page number formatted
			// at left or right margin for odd and even pages respectively. The first page, however,
			// does not get a header, so the we put the page number in first page footer \footerf,
			// but not in odd and even footers. We'll leave the header style and tab position in
			// the odd and even footers, as a convenience in case the user wants to put something
			// else there.
			FooterTags = gpApp->m_eolStr;
			FooterTags += _T("{\\footerl ");
			FooterTags += Sltr_precedence + _T(" \\pard\\plain ");
			FooterTags += Sltr_precedence;
			FooterTags += Sindoc_header;
			FooterTags += _T(" \\tab");
			FooterTags += gpApp->m_eolStr; //_T("\n"); // no page number for odd footers
			FooterTags += _T("\\par}");

			FooterTags += gpApp->m_eolStr;
			FooterTags += _T("{\\footerr ");
			FooterTags += Sltr_precedence + _T(" \\pard\\plain ");
			FooterTags += Sltr_precedence;
			FooterTags += Sindoc_header;
			FooterTags += _T(" \\tab");
			FooterTags += gpApp->m_eolStr; //_T("\n"); // no page number for even footers
			FooterTags += _T("\\par}");

			// only the first page footer gets a page number
			FooterTags += gpApp->m_eolStr;
			FooterTags += _T("{\\footerf ");
			FooterTags += Sltr_precedence + _T(" \\pard\\plain ");
			FooterTags += Sltr_precedence;
			FooterTags += Sindoc_header;
			FooterTags += _T(" \\tab");
			FooterTags += gpApp->m_eolStr;
			FooterTags += _T("{\\field {\\*\\fldinst {PAGE}} {\\fldrslt{}}}");
			FooterTags += _T("\\par}");
		}
	}
	else
	{
		// The user specified no header text so compose one based on language names and bookname.
		// We include the page number in this generic footer, since the generic header does not
		// include a page number. We also use this footer format when \cl chapter labels are used
		// in the file.
		FooterTags = gpApp->m_eolStr
							+ _T("{\\footer \\pard\\plain ")
							+ gpApp->m_eolStr
							+ Sindoc_footer
							+ gpApp->m_eolStr
							+ _T("{")
							+ GetANSIorUnicodeRTFCharsFromString(DocName)
							+ _T(" \\tab}")
							+ gpApp->m_eolStr
							+ _T("{\\field {\\*\\fldinst {PAGE}} {\\fldrslt{}}}")
							+ gpApp->m_eolStr
							+ _T("{\\tab ")+DateTime+_T("\\par}}");
	}

	wxString styleSheet;
	wxString defStr;
	wxString TblGridSNum;
	bool bTableGridSnumFound = FALSE;	// once this flag is set we won't have to do defStr.Find for
										// remaining strings in StyleDefStrArray
	styleSheet = _T("{\\stylesheet"); // opening brace and stylesheet tag word
	styleSheet += gpApp->m_eolStr;
	int styCount;
	int dummyStartPos, dummyEndPos; // unused here
	// Add the style definitions held in StyleDefStrArray to the RTF stylesheet.
	// Note: The strings in StyleInDocStrArray were placed in the rtfTagsMap for
	// high speed access during the parsing of the Buffer and output of the necessary
	// RTF tags.
	// While we are examining the StyleDefStrArray contents, we also extract the style number N of the
	// Table Grid \tsN style which is used in the \tsN tag embedded in the table row \trowd set of tags
	// (in case we have an embedded table defined by a series of USFM markers \tr, \th1, \th2...\tr, \tc1,
	// \tc2... etc).
	for (styCount = 0; styCount < (int)StyleDefStrArray.GetCount(); styCount++)
	{
		defStr = StyleDefStrArray.Item(styCount);
		if (!bTableGridSnumFound && defStr.Find(_T("Table Grid")) != -1)
		{
			TblGridSNum = GetStyleNumberStrFromRTFTagStr(defStr, dummyStartPos, dummyEndPos);
			bTableGridSnumFound = TRUE;
		}
		styleSheet += StyleDefStrArray.Item(styCount);
		styleSheet += gpApp->m_eolStr; //_T("\n");
	}
	// we should have found the Table Grid Sdef and successfully retrieved its style number
	wxASSERT(bTableGridSnumFound && !TblGridSNum.IsEmpty());
	wxString tsNTag = _T("\\ts") + TblGridSNum; // used far below in else if (Marker == _T("tr") ... block
	styleSheet += _T('}');	// closing brace of the stylesheet part of the overall RTF header.

	/////////////////////////////////////////////////////////////////////////////
	// We now have all the info to output the entire RTF Header for source/target
	// RTF output.
	wxString hstr;
	hstr = _T("{") // The whole RTF file must be enclosed in {}
		+Hrtf1+Hcharset+Hcodepg+HUnicodeNumBytes+Hdeffont+gpApp->m_eolStr //_T("\n") // the basic header line
		+_T("{")			// opening brace of the font table
		+_T("\\fonttbl")
		+ gpApp->m_eolStr
		+FTagsNrm+gpApp->m_eolStr	// Normal font tags
		+FTagsSrc+gpApp->m_eolStr	// Source	"	"
		+FTagsTgt+gpApp->m_eolStr	// Target	"	"
		+_T('}')			// closing brace of the font table
		+ColorTable;		// the color table

	hstr += styleSheet;
	hstr += DocTags;
	hstr += gpApp->m_eolStr;	// The document tags
	hstr += HeaderTags;
	hstr += gpApp->m_eolStr;	// The header tags
	hstr += FooterTags;
	hstr += gpApp->m_eolStr;	// The footer tags

	CountTotalCurlyBraces(hstr,nOpeningBraces,nClosingBraces);
	// the opening curly brace that opens the rtf doc should make the number
	// of opening curly braces be one more than the number of closing curly braces at this point
	wxASSERT (nOpeningBraces == nClosingBraces + 1);
	// Output header RTF tags (RTF tags are always system encoding)
	if (!WriteOutputString(f,gpApp->m_systemEncoding,hstr))
		return;

	// Note: Except for the final closing brace '}' to close off the RTF file,
	// the remaining RTF output is composed of the actual Buffer's text with
	// the extra RTF tags needed to format that text into the styles determined
	// by the USFM markers embedded within the text.

	// wx version: See wxProgressDialog farther below:

	//////////////// Do processing of Source/Target text Below /////////////////////////////
	// Buffer has the source/target text reconstituted with markers in place.
	// Now scan the buffer containing the marked up source/target text, parsing the significant
	// markers and write out RTF tag strings where required to format the output text

	// Setup pointers to boundary markers and other Buffer scanning preliminaries
	int nTheLen = Buffer.Length();
	// wx version note: Since we require a read-only buffer we use GetData which just returns
	// a const wxChar* to the data in the string.
	const wxChar* pBuffer = Buffer.GetData();
	int itemLen = 0;
	wxChar* ptr = (wxChar*)pBuffer;			// point to start of text
	wxChar* pBufStart = ptr;		// save start address of Buffer
	wxChar* pEnd = pBufStart + nTheLen;// bound past which we must not go // corrected with ++nTheLen commented out
	wxASSERT(*pEnd == _T('\0')); // ensure there is a null at end of Buffer
	wxString workbuff;				// a small working buffer in which to build a string - unused
	int strLen;
	strLen = ClearBuffer();		// clear the View's working buffer & set length of its string to zero
	strLen = strLen; // avoid warning
	wxString LastStyle = _T("");
	wxString LastParaStyle = _T("");
	wxString LastCharacterStyle = _T("");
	wxString LastNonBoxParaStyle = _T("");

	wxFontEncoding EncodingSrcOrTgt;
	if (exportType == sourceTextExport)
		EncodingSrcOrTgt = gpApp->m_srcEncoding;
	else	// when output Tgt
		EncodingSrcOrTgt = gpApp->m_tgtEncoding;

	wxString Marker;				// hold the marker which has leading backslash and any endmarker asterisk.
	wxString MiscRTF;				// holds Temporary RTF tags for output
	wxString WhiteSpace;			// hold white space
	wxString VernacText;			// hold the vernacular text
	wxString checkStr;
	wxString spaceless = gpApp->m_punctuation[0];
	spaceless.Replace(_T(" "),_T("")); // remove any spaces in the string of source text punctuation characters
	wxString precPunct;			// place to store parsed preceding punctuation, including detached quotes
	precPunct.Empty();
	wxString follPunct;			// place to store parsed following punctuation, including detached quotes
	follPunct.Empty();
	bool bUnknownMarker = FALSE;
	bool bLastOutputWasParTag = FALSE;
	bool bLastParagraphWasBoxed = FALSE;
	bool bProcessingEndlessCharMarker = FALSE;
	bool bProcessingCharacterStyle = FALSE;
	//wxString lastCharacterStyleTags = _T("");
	//bool bHitMarker = FALSE;
	bool bHitBTHaltingMkr = FALSE;
	bool bHitFreeHaltingMkr = FALSE;
	bool bHasFreeTransToAddToFootnoteBody = FALSE;
	wxString lastBTMarker;
	wxString lastFreeMarker;
	wxString Sindoc_Paragraph_key = _T("p");
	enum CallerType callerType = word_literal_caller; // set caller to '*' unless user specifies differently
	enum ParseError parseError = no_error;
	int noteRefNumInt = 0;
	int freeRefNumInt = 0;
	int btRefNumInt = 0;
	wxString noteRefNumStr;

	wxString btAssocStr;		// the text associated with the back translation, used to carry
								// over for output when subsequent \bt marker is encountered (and
								// at end of file
	wxString freeAssocStr;		// the text associated with the free translation, used to carry
								// over for output when subsequent \free marker is encountered (and
								// at end of file
	btAssocStr.Empty();
	freeAssocStr.Empty();

	// the following vars and arrays are used when processing a USFM defined table within the text
	bool bProcessingTable = FALSE;

	wxArrayString cellText;
	wxArrayInt rightAlignment;
	wxArrayInt bestTextExtents;

	if (Buffer.Find(_T("\\c 1")) != -1)
	{
		// Note: chapter 1 bookmark (I) is inserted here before looping through
		// actual text; we insert bookmarks for chapter 2 and greater in the IsChapterMarker
		// block below.
		// Inserted RTF bookmark is in the form of a Roman numeral equivalent to the
		// number of the next chapter. These bookmarks are used by the RTF header
		// fields we insert to display the chapters being displayed on the current page.
			wxString rtfBookMark = IntToRoman(1);
			MiscRTF = _T("{\\*\\bkmkstart ") + rtfBookMark + _T('}')
				+ _T("{\\*\\bkmkend ") + rtfBookMark + _T('}');

			CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces);
			if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
				return;
	}

	// whm 26Aug11 Open a wxProgressDialog instance here for export to rtf operations.
	// whm WARNING: The maximum range of the wxProgressDialog (nTotal below) cannot
	// be changed after the dialog is created. So any routine that gets passed the
	// pProgDlg pointer, must make sure that value in its Update() function does not 
	// exceed the same maximum value (nTotal).
	// whm Note: We calculate nTotal differently here and can't use our 
	// GetMaxRangeForProgressDialog() function because we are progressing
	// through the buffer from beginPtr through pEnd.
	wxChar* beginPtr;
	beginPtr = ptr;
	const int nTotal = (pEnd - beginPtr) + 1;
	
	wxString msgDisplayed;
	wxString progMsg = _("Exporting File %s  - %d of %d Total words and phrases");
	wxFileName fn(exportName);
	msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),1,nTotal);
	wxProgressDialog* pProgDlg;
	pProgDlg = gpApp->OpenNewProgressDialog(_("Exporting To Rich Text Format"),msgDisplayed,nTotal,500);
	int counter = 0;
	// Scan through Buffer
	while (ptr < pEnd)
	{
		counter++;
		if (counter % 500 == 0)	// the counter moves character by character through the buffer
									// so pick a large number for updating the progress dialog
		{
			msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str(),ptr - beginPtr,nTotal);
			pProgDlg->Update(ptr - beginPtr,msgDisplayed);
			//::wxSafeYield();
		}

		//bHitMarker = FALSE;
		if (pDoc->IsWhiteSpace(ptr))
		{
			itemLen = pDoc->ParseWhiteSpace(ptr);
			WhiteSpace = wxString(ptr,itemLen);//testing only
			// white space here usually would be part of vernacular so use EncodingSrcOrTgt
			// but don't output \n new lines
			WhiteSpace.Replace(_T("\n"),_T(" "));
			WhiteSpace.Replace(_T("\r"),_T(" "));
			while (WhiteSpace.Find(_T("  ")) != -1)
			{
				WhiteSpace.Remove(WhiteSpace.Find(_T("  ")),1);
			}
			// whitespace cannot contain any curly braces
			//CountTotalCurlyBraces(WhiteSpace,nOpeningBraces,nClosingBraces);
			if (!WriteOutputString(f,EncodingSrcOrTgt,WhiteSpace))
			{
				pProgDlg->Destroy();
				return;
			}

			ptr += itemLen;					// advance the pointer past the white space
		}

		// are we at the end of the text?
		if (pDoc->IsEnd(ptr) || ptr >= pEnd)
			goto d;

		// are we pointing at a standard format marker?
		// Note: Since we are examining text which is going to RTF output, our ApplyOutputFilterToText
		// function may have "escaped" some curly brace characters which would now be encountered
		// within this buffer as \{ or \}. These backslash characters are not sfm marker initial
		// backslashes, so I here use IsMarkerRTF which returns FALSE if the character immediately
		// following the backslash is an opening curly brace { or a closing curly brace }. Using it
		// here also insures that \{ and \} sequences won't be dealt with by other Doc functions
		// within this block like ParseMarker()
b:		if (IsRTFControlWord(ptr,pEnd))
		{
			// whm 8Nov07 comment: The IsRTFControlWord block is placed here to bleed off the cases 
			// where a backslash is escaping a {, }, or \ character in the character stream. 
			itemLen = ParseRTFControlWord(ptr,pEnd);
			VernacText = wxString(ptr,itemLen);
			//CountTotalCurlyBraces(VernacText,nOpeningBraces,nClosingBraces);
			if (!WriteOutputString(f,gpApp->m_systemEncoding,VernacText))
			{
				pProgDlg->Destroy();
				return;
			}
			ptr += itemLen;
		}
		else if (IsMarkerRTF(ptr,pBufStart))
		{
			//bHitMarker = TRUE;
			int nMkrLen = 0;
			// it's a marker of some kind

			if (bUnknownMarker) // check known/unknown status of last marker processed
			{
				// the last marker encountered was an unknown marker and received either
				// an _unknown_char_style or an _unknown_para_style. If it received an
				// _unknown_char_style, the style tags and associated text were placed
				// within a group after an initial brace. Since we now are dealing with
				// a new marker we need to close off that unknown marker's character style
				// group.

				// we're at the end marker and the last style group needs closing so
				// output the closing brace, but only if the unknown marker fit the context
				// of a character style. bProcessingCharacterStyle was determined for last
				// marker farther below.
				if (bProcessingCharacterStyle) // check style type of last marker processed
				{
					MiscRTF = _T('}');

					CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces);
					if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
					{
						pProgDlg->Destroy();
						return;
					}
					
					Item sfm;
					pCharStack->Pop(sfm); // pop an "unknown" character style marker
					// Check if the stack has another character style marker in it.
					// If so, we need to reset the character style to propagate that
					// character style after the current one has closed.
					if (pCharStack->IsEmpty())
					{
						bProcessingCharacterStyle = FALSE;
					}
					else
					{
						// There is another character style marker in the stack
						// so we need to propagate that character style
						Item sfm2;
						pCharStack->Pop(sfm2);
						wxString sfmMkr = wxString::FromAscii(sfm2);
						rtfIter = rtfTagsMap.find(sfmMkr);
						if (rtfIter != rtfTagsMap.end())
						{
							// We found an associated value for Marker in map.
							// We need only output the previous char style tags here without any 
							// opening curly brace. 
							
							pCharStack->Push(Marker.char_str()); // push it back on the stack

							// RTF tags use gpApp->m_systemEncoding
							wxString mkrTags = (wxString)rtfIter->second;
							CountTotalCurlyBraces(mkrTags,nOpeningBraces,nClosingBraces);
							if (!WriteOutputString(f,gpApp->m_systemEncoding,mkrTags))
							{
								pProgDlg->Destroy();
								return;
							}
						}
					}

				}

				// Note: bUnknownMarker status for the current Marker will be set again below
				// but there is no harm in resetting it here since we've now closed off its
				// group and will not be relevant to IsVerseMarker nor IsChapterMarker below.
				bUnknownMarker = FALSE;
			}

			if (bProcessingEndlessCharMarker)
			{
				// The last style was a character style and a group was started, and not closed.
				// Now, we have encountered another marker so close the group
				MiscRTF = _T('}');

				CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces);
				if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
				{
					pProgDlg->Destroy();
					return;
				}
				bProcessingEndlessCharMarker = FALSE;	// code below will turn this flag on if the current
											// marker is also a table character style marker
			}

			// Get the marker, but don't advance ptr at this point
			itemLen = pDoc->ParseMarker(ptr);
			// We need to get the marker into Marker
			Marker = wxString(ptr,itemLen);
			Marker = Marker.Mid(1); // remove backslash - we want everything after
									// the sfm escape character.
			// Check for an unknown Marker
			// Since the rtfTagsMap only stores the bare marker form without asterisk
			// our test for whether Marker is unknown needs to be done with any asterisk
			// temporarily stripped off
			wxString testBareMkr = Marker;
			int astPos = testBareMkr.Find(_T('*'));
			if (astPos != -1)
			{
				testBareMkr.Remove(astPos,1); // delete the *
			}
			// if the marker is a back translation prefix marker \bt... we also need to
			// just use the bt part for lookup in the rtfTagsMap
			if (testBareMkr.Find(_T("bt")) == 0)
			{
				testBareMkr = _T("bt"); // use only the bt prefix part for lookup
			}
			rtfIter = rtfTagsMap.find(testBareMkr);
			bUnknownMarker = rtfIter == rtfTagsMap.end();

			if (bUnknownMarker)
			{
				// Marker is UNKNOWN because it is not in rtfTagsMap
				// We will assign it a special unknown marker style, either an _unknown_para_style
				// or an _unknown_char_style style, depending on the style type of the LastStyle
				// we encountered.
				bProcessingCharacterStyle = IsACharacterStyle(LastStyle,rtfTagsMap); // check LastStyle
			}
			else
			{
				// marker is KNOWN
				bProcessingCharacterStyle = IsACharacterStyle(Marker,rtfTagsMap); // check Marker
				// whm added 27Nov07 - \bt, \note and \free do their own handling of character style closing
				// curly brace, so if Marker is "bt"..., \note or "free" we reset bProcessingCharacterStyle
				// to FALSE. The reason: The associated text for \bt, \note and \free is not immediately
				// output, but delayed until the next appropriate place/marker is encountered (in order
				// to place it AFTER the material to which it applies). Another reason: if there are
				// no intervening markers/halting points between the \bt, \note and \free material, right
				// after the main while loop, text associated with \bt, \note and/or \free would still be
				// pending output - and we don't want the code there to add a closing brace } char
				// prematurely.
				if (Marker == _T("free") 
					|| Marker == _T("note")
					|| Marker.Find(_T("bt")) != -1)
				{
					if (bProcessingCharacterStyle)
						bProcessingCharacterStyle = FALSE;
				}
			}


			// BEGIN PRIMARY IF ELSE/IF BLOCKS
			if (pDoc->IsVerseMarker(ptr,nMkrLen))
			{
				// it's a verse marker, so needs special handling of its following number
				if (nMkrLen == 2)
				{
					Marker = _T("v");		// Marker holds map key
					ptr += 2;				// point past the \v marker
				}
				else
				{
					Marker = _T("vn");		// Marker holds map key
					ptr += 3;				// point past the \vn marker
				}

				// Fix a common error in sfm formatting, where user forgets to
				// put a paragraph marker \p between a section head (\s, \s1, etc) and any
				// following verse \v N, or between a chapter number (\c, \ca, etc) and any
				// following verse, or between a reference (\r, \rq, etc) and any following verse.
				if (LastStyle == _T("s") || LastStyle == _T("s1") || LastStyle == _T("s2") 
					|| LastStyle == _T("s3") || LastStyle == _T("s4") || LastStyle == _T("sr") 
					|| LastStyle == _T("sx") || LastStyle == _T("sz") || LastStyle == _T("sp") 
					|| LastStyle == _T("c") || LastStyle == _T("ca") || LastStyle == _T("cp") 
					|| LastStyle == _T("cl") || LastStyle == _T("cd")
					|| LastStyle == _T("r") || LastStyle == _T("rem") || LastStyle == _T("rq") )
				{
					// Insert a "Paragraph" style to keep the paragraph that this new
					// verse is in from becoming a Section Head, Chapter Number, or Reference
					// paragraph. User may have intended a different paragraph
					// style for the new paragraph, but we don't know what that might have
					// been so we'll use the most common one - "Paragraph" style (\p).
					rtfIter = rtfTagsMap.find(Sindoc_Paragraph_key);
					if (rtfIter != rtfTagsMap.end())
					{
						// we found an associated value for (Paragraph) Marker in map
						// RTF tags use gpApp->m_systemEncoding
						wxString paraTagStr = (wxString)rtfIter->second;
						CountTotalCurlyBraces(paraTagStr,nOpeningBraces,nClosingBraces);
						if (!WriteOutputString(f,gpApp->m_systemEncoding,paraTagStr))
						{
							pProgDlg->Destroy();
							return;
						}
						LastParaStyle = Sindoc_Paragraph_key;	// we just changed it from Section Head to Paragraph
						LastNonBoxParaStyle = Sindoc_Paragraph_key;
						bLastOutputWasParTag = TRUE;
					}
				}

				if (bLastParagraphWasBoxed)
				{
					// There was a small break paragraph inserted and no paragraph style
					// has intervened before this verse number, so propagate the
					// LastNonBoxParaStyle
					rtfIter = rtfTagsMap.find(LastNonBoxParaStyle);
					if (rtfIter != rtfTagsMap.end())
					{
						// we found an associated value for Marker in map
						// RTF tags use gpApp->m_systemEncoding
						wxString lastStyTag = (wxString)rtfIter->second;
						CountTotalCurlyBraces(lastStyTag,nOpeningBraces,nClosingBraces);
						if (!WriteOutputString(f,gpApp->m_systemEncoding,lastStyTag))
						{
							pProgDlg->Destroy();
							return;
						}
					}
					bLastParagraphWasBoxed = FALSE;
				}

				// Now, handle the tags for the present Verse Num
				if (!(exportType == sourceTextExport))
				{
					// In Target text (because of the way the Buffer was filled):
					// verses that follow text (i.e., not those directly following a paragraph tag) need to have
					// a space added before the verse number tags, otherwise they will be juxtaposed to previous
					// text without an intervening space, using source/target encoding
					if (!bLastOutputWasParTag)
					{
						MiscRTF = _T(" ");
						//CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // no braces possible here
						if (!WriteOutputString(f,EncodingSrcOrTgt,MiscRTF))
						{
							pProgDlg->Destroy();
							return;
						}
					}
				}

				// Use the marker as key and query our map and get its
				// associated value (RTF tags) if any, and output them
				if (Marker.Length() != 0)
				{
					rtfIter = rtfTagsMap.find(Marker);
					if (rtfIter != rtfTagsMap.end())
					{
						// we found an associated value for (verse) Marker (\v or \vn) in map
						// Need opening brace before verse number char style group)
						MiscRTF = _T("{");
						CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces);
						if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						{
							pProgDlg->Destroy();
							return;
						}
						// RTF tags use gpApp->m_systemEncoding
						wxString mkrTags = (wxString)rtfIter->second;
						CountTotalCurlyBraces(mkrTags,nOpeningBraces,nClosingBraces); // no braces possible here
						if (!WriteOutputString(f,gpApp->m_systemEncoding,mkrTags))
						{
							pProgDlg->Destroy();
							return;
						}
					}
				}

				itemLen = pDoc->ParseWhiteSpace(ptr);
				// white space here is only delimiter for verse num marker so omit it from output
				ptr += itemLen;	// point at verse number

				itemLen = pDoc->ParseNumber(ptr);
				// verse number is vernacular so output with EncodingSrcOrTgt
				wxString numStr = wxString(ptr,itemLen);
				CountTotalCurlyBraces(numStr,nOpeningBraces,nClosingBraces); // no braces likely here
				if (!WriteOutputString(f,EncodingSrcOrTgt,numStr))
				{
					pProgDlg->Destroy();
					return;
				}
				// add RTF non-breaking space and closing brace to close Verse Num char style group
				MiscRTF = _T("\\~}");
				CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // one closing curly brace here
				if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
				{
					pProgDlg->Destroy();
					return;
				}
				ptr += itemLen;	// point past verse number

				itemLen = pDoc->ParseWhiteSpace(ptr);	// parse past white space after the marker
				// white space here is only delimiting end of verse number so omit it from output

				LastStyle = Marker;
				// We don't update the LastParaStyle here since verse number is a character style
				ptr += itemLen;	// point past the white space

				goto b;			// check if another marker follows
			}// end of if IsVerseMarker

			// some other kind of marker - perhaps it's a chapter marker?
			else if (pDoc->IsChapterMarker(ptr))
			{
				// It's a chapter marker, so needs special handling of its following number,
				// any any immediately following \cl chapter label text.

				// We shouldn't have two chapter number paragraphs in sequence
				// so we won't check for it

				ptr += 2; // point past the \c marker

				itemLen = pDoc->ParseWhiteSpace(ptr);
				// omit output of white space here
				ptr += itemLen;	// point at chapter number

				itemLen = pDoc->ParseNumber(ptr);

				// get actual chapter number and output it
				VernacText = wxString(ptr,itemLen);

				// Note: chapter 1 bookmark (I) is inserted above before looping through
				// actual text; here we insert bookmarks for chapter 2 and greater.
				// Insert an RTF bookmark in the form of a Roman numeral equivalent to the
				// number of the next chapter. These bookmarks are used by the RTF header
				// fields we insert to display the chapters being displayed on the current page.
				if (wxAtoi(VernacText) > 1)
				{
					wxString rtfBookMark = IntToRoman(wxAtoi(VernacText));
					MiscRTF = _T("{\\*\\bkmkstart ") + rtfBookMark + _T('}')
						+ _T("{\\*\\bkmkend ") + rtfBookMark + _T('}');
					CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // 2 opening & 2 closing curly braces here
					if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
					{
						pProgDlg->Destroy();
						return;
					}
				}

				ptr += itemLen;	// point past chapter number
				itemLen = pDoc->ParseWhiteSpace(ptr);	// parse white space following the number
				ptr += itemLen;	// point past it
				// again, omit output of white space here

				// Before we output the chapter number (or its style tags), we need to check to see if
				// a \cl marker immediately follows the current \c n marker. If so, we assign the text
				// assoc with the following \cl marker to chapterLabel, and omit the output the actual
				// chapter number. We also want to use the paragraph style associated with \cl rather than
				// that associated with \c. For example, the user may have "\c 3" followed by
				// "\cl Chapter Three". In this case we first output the paragraph style associated with
				// \cl, then output its assoc text "Chapter Three" instead of the style associated with
				// \c and the number "3".
				//
				// At this point we've parsed the number (in VernacText), and any following whitespace.
				// We'll first check for the existence of and parse any immediately following \cl marker
				// and assoc text. If ParseAnyFollowingChapterLabel() returns zero there is no following
				// \cl marker.
				wxString tempLabel;
				itemLen = ParseAnyFollowingChapterLabel(ptr, pBufStart, pEnd, tempLabel);
				// ParseAnyFollowingChapterLabel() returns zero if no \cl immediately follows the
				// intervening whitespace. It also returns the text associated with any \cl in
				// tempLabel

				if (itemLen != 0 && !tempLabel.IsEmpty())
				{
					// We have a \cl chapter label to deal with and it has associated text with it
					Marker = _T("cl");
					rtfIter = rtfTagsMap.find(Marker);
					if (rtfIter != rtfTagsMap.end())
					{
						// we found an associated value for chapter Marker in map
						// RTF tags use gpApp->m_systemEncoding
						checkStr = (wxString)rtfIter->second;
						CountTotalCurlyBraces(checkStr,nOpeningBraces,nClosingBraces);
						if (!WriteOutputString(f,gpApp->m_systemEncoding,checkStr))
						{
							pProgDlg->Destroy();
							return;
						}
					}

					// output tempLabel instead of VernacText
					CountTotalCurlyBraces(tempLabel,nOpeningBraces,nClosingBraces);
					if (!WriteOutputString(f,EncodingSrcOrTgt,tempLabel))
					{
						pProgDlg->Destroy();
						return;
					}

					ptr += itemLen; // point past the \cl and assoc text
					itemLen = pDoc->ParseWhiteSpace(ptr);	// parse white space following the \cl and assoc text
					ptr += itemLen;	// point past it
				}
				else
				{
					// There was no \cl chapter label immediately following.
					// Before we output the usual c - Chapter Number style tags, we need to check if
					// chapterLabel was defined by a \cl marker prior to the first chapter number \c 1.
					// If \cl was defined, chapterLabel will contain the label text to be output.
					if (!chapterLabel.IsEmpty())
					{
						// The chapterLabel was defined by a \cl marker prior to the first \c n marker.
						Marker = _T("cl");
						// if we are at chapter 1 the LastNonBoxParaStyle before reaching this point would
						// normally have been \cl and its style tags would have been output at the bottom
						// of this loop, so we'll only output the tags here if, for some reason, they
						// weren't just output.
						if (LastNonBoxParaStyle != _T("cl"))
						{
							rtfIter = rtfTagsMap.find(Marker);
							if (rtfIter != rtfTagsMap.end())
							{
								// we found an associated value for chapter Marker in map
								// RTF tags use gpApp->m_systemEncoding
								checkStr = (wxString)rtfIter->second;
								CountTotalCurlyBraces(checkStr,nOpeningBraces,nClosingBraces);
								if (!WriteOutputString(f,gpApp->m_systemEncoding,checkStr))
								{
									pProgDlg->Destroy();
									return;
								}
							}
						}

						// Output tempLabel followed by a space followed by the number in VernacText.
						// We will assume that the label preceeds the number. If this is not the case
						// and the user desires it to be otherwise, s/he will need to use \cl markers
						// following each chapter number to get the desired order.
						wxString temp;
						temp = chapterLabel + _T(' ') + VernacText;
						CountTotalCurlyBraces(temp,nOpeningBraces,nClosingBraces);
						if (!WriteOutputString(f,EncodingSrcOrTgt,temp))
						{
							pProgDlg->Destroy();
							return;
						}
						// no additional parsing needed here
					}
					else
					{
						// No chapterLabel has been defined so we just output the actual number
						// currently in VernacText.
						// Use the marker as key and query our map and get its
						// associated value (RTF tags) if any, and output them
						Marker = _T("c");		// Marker holds map key
						rtfIter = rtfTagsMap.find(Marker);
						if (rtfIter != rtfTagsMap.end())
						{
							// we found an associated value for chapter Marker in map
							// RTF tags use gpApp->m_systemEncoding
							checkStr = (wxString)rtfIter->second;
							CountTotalCurlyBraces(checkStr,nOpeningBraces,nClosingBraces);
							if (!WriteOutputString(f,gpApp->m_systemEncoding,checkStr))
							{
								pProgDlg->Destroy();
								return;
							}
						}

						CountTotalCurlyBraces(VernacText,nOpeningBraces,nClosingBraces);
						if (!WriteOutputString(f,EncodingSrcOrTgt,VernacText))
						{
							pProgDlg->Destroy();
							return;
						}
						// no additional parsing needed here
					}
				}

				LastStyle = Marker;
				LastParaStyle = Marker;
				LastNonBoxParaStyle = Marker;
				bLastOutputWasParTag = TRUE;
				bLastParagraphWasBoxed = FALSE;


				goto b;			// check if another marker follows
			}// end of if IsChapterMarker
			else
			{
				// neither verse nor chapter but another kind of marker, at this point
				// we don't have to worry about a following number, so process the marker

				// NOTE: Among these styles, some need special handling, especially those
				// which are embedded within other paragraphs. These include the character
				// styles which have an sfm marker (Verse Num, Glos Definition) and the
				// footnote begin marker (\f) as well as the footnote end markers (\fe, \fe* \f*).
				// By treating these separately, it simplifies the treatment of all other
				// paragraph styles.
				// Also, we need to be able to shorten the indoc style tags
				// to just \par when two paragraph styles with the same style occur in
				// succession.

				// The RTF standard defines "groups" which can consist of text, control words
				// and/or control symbols. The text and its defining attributes are enclosed
				// within braces {...}. The older sfm standards suffered from the weakness that
				// many attributes did not have end markers to delimit the extent of text the
				// given attribute would apply to. The exception in the old sfm standard was
				// footnotes which had an end marker. In the new USFM 2 standard most markers
				// that define "character" styles have end markers (containing a final *). These
				// end markers make it easier to detect the extent of text to which an attribute
				// marker applies. However, the fact that the embedded footnote and cross-reference
				// markers have optional end markers complicates the processing of such markers.

				// Note for Version 3: The parsing here needs to take into account the new USFM
				// endMarker scheme of using the same base marker plus adding * suffixed to it.
				// Most of these markers which have end markers are character styleType markers,
				// and character styles in RTF are placed in groups defined by { and } braces,
				// with the character style tag string coming first in the group followed by the
				// text to which the character style is applied, followed by the closing brace }.
				// Unfortunately, not all character styleTypes utilize end markers, and some that
				// can have end markers such as the embedded content markers for footnotes and
				// cross-references the end markers are optional. Hence, we have to treat the
				// special situations before the normal situations.
				// The special situations are:
				// 1. Unknown markers. These will be treated as paragraph markers if encountered
				//    while processing paragraph markers, or character markers if encountered
				//    while processing character markers.
				// 2. Markers which are paragraph styles \sN, but that also have corresponding end
				//    markers utilizing *. These include: x...x*, f...f*, fe...fe*, free...free*,
				//    and note...note*.
				// 3. Markers which are character styles \csN, but that have no corresponding end
				//    markers utilizing *. These include: all the table column heading markers
				//    (\th1, \th2, \th3, \th4, thr1, \thr2, \thr3, \thr4) and the table cell data
				//    markers (\tc1, \tc2, \tc3, \tc4, \tcr1, \tcr2, \tcr3, \tcr4), the PNG
				//    verse text marker \vt, the PNG glossary definition marker \gd, the PNG
				//    cross-reference markers \@ and \xr, and the PNG footnote end markers \fe
				//    and \F. These all need special handling to close the character style group
				//    and/or account for their special behavior.
				// 4. Markers which are character styles \csN, and have end markers which are
				//    optional. These are the embedded content markers that can occur optionally
				//    embedded within cross-references or footnotes. These include the cross-reference
				//    content markers \xo, \xt, \xk, \xq, and \xdc, and the footnote content
				//    markers \fr, \fk, \fq, \fqa, \ft, \fdc, \fv, and \fm.

				// The normal situations are:
				// 1. Markers which are paragraph styles and have no corresponding end markers.
				// 2. Markers which are character styles and always have corresponding end markers
				//    using *.

				// Use the marker as key and query our map and get its
				// associated value (RTF tags) if any, and output them

				// Handle formatting and output of any unknown markers
				if (bUnknownMarker)
				{
					// We don't recognize this marker, so attach "Unk Para Style" to it
					// If user intended it to be a character style this may flag the whole
					// enclosing paragraph with a red color (as signal to user)
					if (bLastParagraphWasBoxed)
					{
						// There was a small break paragraph inserted and no paragraph style
						// has intervened, so propagate the LastNonBoxParaStyle
						rtfIter = rtfTagsMap.find(LastNonBoxParaStyle);
						if (rtfIter != rtfTagsMap.end())
						{
							// we found an associated value for Marker in map
							// RTF tags use gpApp->m_systemEncoding
							wxString lastStyTag = (wxString)rtfIter->second;
							CountTotalCurlyBraces(lastStyTag,nOpeningBraces,nClosingBraces);
							if (!WriteOutputString(f,gpApp->m_systemEncoding,lastStyTag))
							{
								pProgDlg->Destroy();
								return;
							}
						}
						bLastParagraphWasBoxed = FALSE;
					}

					wxString unkMarker = Marker;
					if (bProcessingCharacterStyle)
						Marker = _T("_unknown_char_style");
					else
						Marker = _T("_unknown_para_style");
					rtfIter = rtfTagsMap.find(Marker);		// this should not fail
					if (rtfIter != rtfTagsMap.end())
					{
						if (bProcessingCharacterStyle)
						{
							pCharStack->Push(Marker.char_str()); // push an "unknown" character style marker
							// we need to start the character style group with an opening brace
							MiscRTF = _T('{');
							// RTF tags use gpApp->m_systemEncoding
							CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // one opening curly brace here
							if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
							{
								pProgDlg->Destroy();
								return;
							}
						}
						// we found an associated value for "unknown" key in map
						checkStr = (wxString)rtfIter->second;
						// RTF tags use gpApp->m_systemEncoding
						CountTotalCurlyBraces(checkStr,nOpeningBraces,nClosingBraces);
						if (!WriteOutputString(f,gpApp->m_systemEncoding,checkStr))
						{
							pProgDlg->Destroy();
							return;
						}
					}
					// to help the user detect typos, output the unknown marker prefixed to its assoc text
					unkMarker = _T("\\\\") + unkMarker + _T(' '); // RTF requires backslash be escaped with a '\'
					CountTotalCurlyBraces(unkMarker,nOpeningBraces,nClosingBraces);
					if (!WriteOutputString(f,gpApp->m_systemEncoding,unkMarker))
					{
						pProgDlg->Destroy();
						return;
					}

					bLastOutputWasParTag = TRUE;
					// Don't update LastStyle nor LastParaStyle here - we want to maintain the
					// previous paragraph's properties for subsequent processing
				}
				// Handle footnotes, endnotes and cross-references.
				// Note: The following notes apply specifically to footnotes, but with a few minor
				// adjustments, also apply to endnotes and cross-references.
				//
				// In the USFM 2.0 standard, the first non-space character or word
				// after "\f " or "\fe " is supposed to be one of the following that determines
				// the kind of footnote "caller" (the letter/character/symbol used within the
				// text to denote the location of the footnote):
				//   '+' for generating the caller automatically (with progressive letters or numbers)
				//   '-' no caller generated (i.e., the footnote/endnote would require a reference
				//       back to where the footnote/endnote would apply.
				//   '*' where * could be a literal asterisk or a character or even a word used as the
				//       caller (in this case the character/word would be defined by the user).
				// The above behavior poses some challenges for our parsing here. For UsfmOnly sfm set
				// use, we need to check for the existence of the '+', '-' and '*'. The '+' and '-' are
				// easy to interpret, but the '*' is not if the user intends it to be a character or
				// word other than a literal '*' (asterisk).
				// ASSUMPTIONS:
				//  1. We assume that if a '+' is the first non-space character following \f or \fe (in
				// the UsfmOnly set), that the footnote caller should be generated automatically
				// (via use of the \chftn RTF control tag). This applies also to '+' following endnote
				// \fe and crossref \x.
				//  2. We assume that if a '-' is the first non-space character following \f or \fe (in
				// the UsfmOnly set), that the footnote caller should not appear at all in the
				// text at the point the footnote is encountered. This applies also to '-' following
				// endnote \fe and crossref \x.
				//  3. We assume that if a '*' (literal asterisk) or any other non-marker text word is
				// the first non-space character following the \f or \fe (in the UsfmOnly set), that
				// it is intended to be the caller raised and placed within the text at that point.
				// Tests show that Paratext 6 also makes this assumption. This applies also to '*'
				// following endnote \fe and crossref \x.
				//  4. We assume that if there is an embedded content marker (such as \fr, etc.)
				// immediately following the \f or \fe, so that no '+', '-' or other non-marker word
				// is present between the \f and the embedded content marker, that the user intends to
				// have an asterisk '*' used as the caller. Tests show this is what Paratext 6 does.
				// This applies to endnotes, and to the embedded content markers for crossrefs (such as
				// \xo, etc.) immediately following the \x.
				//  5. We assume a footnote that is malformed and has no footnote end marker, will
				// end at the first non-embedded marker encountered following \f or \fe (for UsfmOnly).
				// This assumption has been modified to allow for other character formatting markers
				// and verse markers to intervene.
				//
				// With the above assumptions, we then can parse the word immediately following the
				// \f or \fe markers, and check the contents of that word. If the word immediately
				// following \f or \fe is a '+' or '-' we adjust the RTF tags to set up the kind of
				// caller defined in 1. or 2. above. If the word is something other than '+' or '-'
				// we will assume the word is intended to be caller, and its format will be raised
				// within the text as a footnote caller. Paratext assumes that if there is no '+'
				// or '-' after \f or \fe, the next word, regardless of how long, is always to be used
				// as the caller. For parsing of footnotes, endnotes, and crossrefs we use the dedicated
				// functions ParseFootnote(), ParseEndnote(), and ParseCrossRef() in the appropriate
				// blocks below.
				//
				// Handle footnotes
				else if (Marker == _T("f"))
				{
					// We parse the entire footnote here through the end marker. In doing so
					// we will set itemLen to the entire length of the footnote (including its end marker).
					// Parsing the footnote marker through its entire length here simplifies the
					// outer loop of 'else if' statements because by treating the whole footnote
					// here we don't have to worry about the corresponding footnote end markers
					// and any footnote embedded content markers in the outer loop. The footnote
					// embedded content markers being optional, and are also easier to treat within
					// this inner block of the overall loop.

					// Note: the following ParseFootnote overwrites the itemLen that was determined
					// by ParseMarker() above and will move the ptr at the bottom of the outer
					// loop to point just past the footnote end marker.
					itemLen = ParseFootnote(ptr,pBufStart,pEnd,parseError); // parse the whole footnote
					wxString fnStr;
					wxString nullStr;
					nullStr.Empty(); // no caller supplied as parameter to ProcessAndWriteDestinationText
					fnStr = wxString(ptr,itemLen);

					bool bIsAtEnd = FALSE; // set by ProcessAndWriteDestinationText() below
					callerType = no_caller; // start with this setting, ProcessAndWriteDestinationText
											// may change it

					// ProcessAndWriteDestinationText below handles all the footnote processing
					// and output of RTF tags
					if (!ProcessAndWriteDestinationText(f, EncodingSrcOrTgt, fnStr,
						bIsAtEnd, footnoteDest, rtfTagsMap, pDoc, parseError, callerType,
						nullStr, bHasFreeTransToAddToFootnoteBody, freeAssocStr))
					{
						pProgDlg->Destroy();
						return;
					}
					if (bIsAtEnd) // ProcessAndWriteDestinationText got to the end of the string
					{
						goto b; //check for another marker beyond what ParseFootnote processed
					}

					// add space after destination text unless followed by punctuation
					bool bIsSource = exportType == sourceTextExport;
					if (!PunctuationFollowsDestinationText(itemLen,ptr,pEnd,bIsSource))
					{
						// write a space after the destination text (footnote, endnote, crossref), but
						// only if no punctuation immediately follows it.
						wxString spFollowingDestText = _T(' ');
						if (!WriteOutputString(f,gpApp->m_systemEncoding,spFollowingDestText))
						{
							pProgDlg->Destroy();
							return;
						}
					}

					if (bHasFreeTransToAddToFootnoteBody)
					{
						// the free translation was output in the body of the footnote so reset
						// some variables to reflect that free trans is no longer pending
						bHitFreeHaltingMkr = FALSE;
						bHasFreeTransToAddToFootnoteBody = FALSE;
						// ProcessAdnWriteDestinationText calls freeAssocStr.Empty()
					}

					LastStyle = Marker;
					// Don't update LastParaStyle here - we want to maintain its properties for the
					// rest of the paragraph (if any) after the footnote
				}

				// Handle endnotes
				// Note: Endnotes were not part of the PngOnly set and we exclude the PngOnly
				// case because \fe was the usual footnote end marker there.
				else if (gpApp->gCurrentSfmSet != PngOnly && Marker == _T("fe"))
				{
					// We parse the entire endnote here through the end marker. In doing so
					// we will set itemLen to the entire length of the endnote (including its end marker).
					// Parsing the endnote marker through its entire length here simplifies the
					// outer loop of 'else if' statements because by treating the whole endnote
					// here we don't have to worry about the corresponding endnote end markers
					// and any endnote embedded content markers in the outer loop. The endnote
					// embedded content markers being optional, and are also easier to treat within
					// this inner block of the overall loop.
					// This routine is identical in structure to that of the footnote block above
					// except for the differences between endnote and footnote details. A common
					// function could be created to handle both (and crossrefs too), but I'm
					// doing them separately for now as the functions would require a lot of
					// parameters and a little restructuring.

					// Note: the following ParseEndnote overwrites the itemLen that was determined
					// by ParseMarker() above and will move the ptr at the bottom of the
					// loop to point just past the endnote end marker.
					itemLen = ParseEndnote(ptr,pBufStart,pEnd,parseError);	// parse the whole endnote
					wxString enStr;
					wxString nullStr;
					nullStr.Empty(); // no caller supplied as parameter to ProcessAndWriteDestinationText
					enStr = wxString(ptr,itemLen);


					bool bIsAtEnd = FALSE;
					callerType = no_caller; // start with this setting, ProcessAndWriteDestinationText
											// may change it

					// ProcessAndWriteDestinationText below handles all the endnote processing
					// and output of RTF tags
					if (!ProcessAndWriteDestinationText(f, EncodingSrcOrTgt, enStr,
						bIsAtEnd, endnoteDest, rtfTagsMap, pDoc, parseError, callerType,
						nullStr, bHasFreeTransToAddToFootnoteBody, freeAssocStr))
					{
						pProgDlg->Destroy();
						return;
					}
					if (bIsAtEnd) // ProcessAndWriteDestinationText got to the end of the string
					{
						goto b; //check for another marker beyond what ParseFootnote processed
					}

					// add space after destination text unless followed by punctuation
					bool bIsSource = exportType == sourceTextExport;
					if (!PunctuationFollowsDestinationText(itemLen,ptr,pEnd,bIsSource))
					{
						// write a space after the destination text (footnote, endnote, crossref), but
						// only if no punctuation immediately follows it.
						wxString spFollowingDestText = _T(' ');
						if (!WriteOutputString(f,gpApp->m_systemEncoding,spFollowingDestText))
						{
							pProgDlg->Destroy();
							return;
						}
					}

					if (bHasFreeTransToAddToFootnoteBody)
					{
						// the free translation was output in the body of the footnote so reset
						// some variables to reflect that free trans is no longer pending
						bHitFreeHaltingMkr = FALSE;
						bHasFreeTransToAddToFootnoteBody = FALSE;
						// ProcessAdnWriteDestinationText calls freeAssocStr.Empty()
					}

					LastStyle = Marker;
					// Don't update LastParaStyle here - we want to maintain its properties for the
					// rest of the paragraph (if any) after the endnote
				}
				// Handle cross-references
				else if (gpApp->gCurrentSfmSet == UsfmOnly &&  Marker == _T("x") )
				{
					// We parse the entire crossref here through the end marker. In doing so
					// we will set itemLen to the entire length of the crossref (including its end marker).
					// Parsing the crossref marker through its entire length here simplifies the
					// outer loop of 'else if' statements because by treating the whole crossref
					// here we don't have to worry about the corresponding crossref end markers
					// and any crossref embedded content markers in the outer loop. The crossref
					// embedded content markers being optional, and are also easier to treat within
					// this inner block of the overall loop.
					// This routine is identical in structure to that of the endnote and footnote blocks
					// above except for the differences between endnote and footnote details. A common
					// function could be created to handle them all, but I'm doing them separately for
					// now as the functions would require a lot of parameters and a little restructuring.

					// Note: the following ParseCrossRef overwrites the itemLen that was determined
					// by ParseMarker() above and will move the ptr at the bottom of the
					// loop to point just past the endnote end marker.
					itemLen = ParseCrossRef(ptr,pBufStart,pEnd,parseError); // parse the whole crossref
					wxString crStr;
					wxString nullStr;
					nullStr.Empty(); // no caller supplied as parameter to ProcessAndWriteDestinationText
					crStr = wxString(ptr,itemLen);

					bool bIsAtEnd = FALSE;
					callerType = no_caller; // start with this setting, ProcessAndWriteDestinationText
											// may change it

					// ProcessAndWriteDestinationText below handles all the endnote processing
					// and output of RTF tags
					if (!ProcessAndWriteDestinationText(f, EncodingSrcOrTgt, crStr,
						bIsAtEnd, crossrefDest, rtfTagsMap, pDoc, parseError, callerType,
						nullStr, bHasFreeTransToAddToFootnoteBody, freeAssocStr))
					{
						pProgDlg->Destroy();
						return;
					}
					if (bIsAtEnd) // ProcessAndWriteDestinationText got to the end of the string
					{
						goto b; //check for another marker beyond what ParseFootnote processed
					}

					// add space after destination text unless followed by punctuation
					bool bIsSource = exportType == sourceTextExport;
					if (!PunctuationFollowsDestinationText(itemLen,ptr,pEnd,bIsSource))
					{
						// write a space after the destination text (footnote, endnote, crossref), but
						// only if no punctuation immediately follows it.
						wxString spFollowingDestText = _T(' ');
						if (!WriteOutputString(f,gpApp->m_systemEncoding,spFollowingDestText))
						{
							pProgDlg->Destroy();
							return;
						}
					}

					if (bHasFreeTransToAddToFootnoteBody)
					{
						// the free translation was output in the body of the footnote so reset
						// some variables to reflect that free trans is no longer pending
						bHitFreeHaltingMkr = FALSE;
						bHasFreeTransToAddToFootnoteBody = FALSE;
						// ProcessAdnWriteDestinationText calls freeAssocStr.Empty()
					}

					LastStyle = Marker;
					// Don't update LastParaStyle here - we want to maintain its properties for the
					// rest of the paragraph (if any) after the endnote
				}

				// Handle Glos Definition
				else if (Marker == _T("gd"))
				{
					// We're at a gloss definition within a Glos Main Entry or Glos Sub-entry
					if (bLastParagraphWasBoxed)
					{
						// There was a small break paragraph inserted and no paragraph style
						// has intervened, so propagate the LastNonBoxParaStyle
						rtfIter = rtfTagsMap.find(LastNonBoxParaStyle);
						if (rtfIter != rtfTagsMap.end())
						{
							// we found an associated value for Marker in map
							// RTF tags use gpApp->m_systemEncoding
							wxString lastStyTag = (wxString)rtfIter->second;
							CountTotalCurlyBraces(lastStyTag,nOpeningBraces,nClosingBraces);
							if (!WriteOutputString(f,gpApp->m_systemEncoding,lastStyTag))
							{
								pProgDlg->Destroy();
								return;
							}
						}
						bLastParagraphWasBoxed = FALSE;
					}

					rtfIter = rtfTagsMap.find(Marker);
					if (rtfIter != rtfTagsMap.end())
					{
						// we found an associated value for Marker in map
						checkStr = (wxString)rtfIter->second;
						// RTF tags use gpApp->m_systemEncoding
						CountTotalCurlyBraces(checkStr,nOpeningBraces,nClosingBraces);
						if (!WriteOutputString(f,gpApp->m_systemEncoding,checkStr))
						{
							pProgDlg->Destroy();
							return;
						}
					}
					LastStyle = Marker;
					// Don't update LastParaStyle here - we want to maintain its properties for the
					// rest of the paragraph (if any) after the gloss definition
				}

				// Handle USFM defined tables within the RTF Source/Target text.
				// These tables are composed of a series of \tr new row markers followed by \thN,
				// \thrN, \tcN, and/or \tcrN markers to define cell header and/or cell data making
				// up the columns of the table rows defined by each \tr row marker that is present.
				// As we did for footnotes, endnotes and crossrefs, we could parse the entire table
				// here and output the tags for the table in the block below, rather than trying to
				// construct the RTF tags for the table during repeated passes through the outer loop.
				// However, because of the possibility of embedded footnotes, endnotes, etc, I will
				// attempt to create a table first without recourse to a parse of the entire table
				// at one time. The main challenges are:
				//    1. Determining in advance the optimum cell width extents for \cellxN tags for
				//       all rows.
				//    2. Knowing in advance how many rows are in the table so when a \tr marker is
				//       encountered we are about to process the last row of the table we can output a
				//       \lastrow tag along with the other row tags that preceed the row data.
				// The solution to these challenges is:
				//    1. Once we've encountered the first \tr marker, scan ahead in the Buffer and
				//       determine the text extents for all columns of data and determine the best
				//       assignment of cell widths (extents) to assign to the table columns. During
				//       this scan we can ensure that the table is well formed.
				//    2. During the scan of the Buffer we also count the number of \tr markers
				//       existing in the current table, and use that count to know when we are at the
				//       last row and need to add the \lastrow tag.
				// Like the footnote, endnote, and crossref, a table should have all of its related
				// markers and associated text together in sequence in the Buffer (with the possibility
				// of some other markers embedded within the table). We can expect that there might be
				// some character style format markers embedded within the cell data items and
				// possibly footnotes, endnotes, etc embedded within it. Generally, we should be able to
				// parse the table by parsing through all table markers and any embedded character format
				// markers, footnotes, endnotes, back translation, free translation, and notes, until we
				// come to a non-table paragraph marker, which establishes the end point of the table.
				// Although UBS examples of tables have verse markers collected together above the
				// text of a table (i.e., \v 10-16), we will also allow \v N verse markers to be embedded
				// within the table cell data.
				//
				// The following is an example of a table with four rows (including first header row)
				// and three columns, followed by its USFM marker representation, followed by its
				// RTF table tag representation.

				//Here is the desired table (imagine it is in a 4 row x 3 column Word table):

				//Day   Tribe     Leader
				//1st   Judah     nahshon son of Amminadab
				//2nd   Issachar  Nethanel son of Zuar
				//3rd   Zebulun   Eliab son of Helon

				//Here is the USFM representation:

				//\tr \th1 Day\th2 Tribe\th3 Leader
				//\tr \tc1 1st\tc2 Judah\tc3 Nahshon son of Amminadab
				//\tr \tc1 2nd\tc2 Issachar\tc3 Nethanel son of Zuar
				//\tr \tc1 3rd\tc2 Zebulun\tc3 Eliab son of Helon

				//Here is a sample RTF representation. This is a minimal example. The actual set of
				//tags will differ in some respects and there will be character style tags added to
				//the cells.

				//\pard\plain
				//\trowd \irow0\irowband0\ts31\trgaph40\trleft0\ltrrow
				//\cellx900
				//\cellx2160
				//\cellx5130
				//\pard\plain
				//\s1\qj \li0\ri0\widctlpar\intbl\yts31\ltrpar\nooverflow\rin0\lin0\itap0 \f1\fs22
				//{Day\cell Tribe\cell Leader\cell }
				//\pard\plain
				//\qj \li0\ri0\widctlpar\intbl\ltrpar\nooverflow\rin0\lin0\itap0 \fs22
				//{\trowd \irow0\irowband0\ts31\trgaph40\trleft0\ltrrow
				//\cellx900
				//\cellx2160
				//\cellx5130\row }
				//\pard\plain
				//\s1\qj \li0\ri0\widctlpar\intbl\yts31\ltrpar\nooverflow\rin0\lin0\itap0 \f1\fs22
				//{1st\cell Judah\cell Nahshon son of Amminadab\cell }
				//\pard\plain
				//\qj \li0\ri0\widctlpar\intbl\ltrpar\nooverflow\rin0\lin0\itap0 \fs22
				//{\trowd \irow1\irowband1\ts31\trgaph40\trleft0\ltrrow
				//\cellx900
				//\cellx2160
				//\cellx5130\row }
				//\pard\plain
				//\s1\qj \li0\ri0\widctlpar\intbl\yts31\ltrpar\nooverflow\rin0\lin0\itap0 \f1\fs22
				//{2nd\cell Issachar\cell Nethanel son of Zuar\cell }
				//\pard\plain
				//\qj \li0\ri0\widctlpar\intbl\ltrpar\nooverflow\rin0\lin0\itap0 \fs22
				//{\trowd \irow2\irowband2\ts31\trgaph40\trleft0\ltrrow
				//\cellx900
				//\cellx2160
				//\cellx5130\row }
				//\pard\plain
				//\s1\qj \li0\ri0\widctlpar\intbl\yts31\ltrpar\nooverflow\rin0\lin0\itap0 \f1\fs22
				//{3rd\cell Zebulun\cell Eliab son of Helon\cell }
				//\pard\plain
				//\qj \li0\ri0\widctlpar\intbl\ltrpar\nooverflow\rin0\lin0\itap0 \fs22
				//{\trowd \irow3\irowband3\ts31\trgaph40\lastrow\trleft0\ltrrow  <-- note \lastrow tag
				//\cellx900
				//\cellx2160
				//\cellx5130\row }\pard
				//\qj \li0\ri0\widctlpar\ltrpar\nooverflow\rin0\lin0\itap0 \f0\fs22
				//{\par }

				// Handle the USFM table markers which, except for \tr, are "character" style and none
				// have end markers. This "else if" block attempts to format USFM RTF tables as real
				// Word tables with rows and column cells. It turns out that something in it causes
				// Word to hang when scrolling with the thumb through a page with tables formatted
				// with the routine below. Therefore, I'm commenting out this form and using the
				// else if block below it which doesn't create real Word tables, but simply formats
				// the data as paragraphs (which are color coded to identify the separate columns).

				//else if (Marker == _T("tr") || Marker.Find(_T("th")) == 0 || Marker.Find(_T("tc")) == 0)
				//{
				//	// Handle the table row markers which are "paragraph" style markers
				//	if (Marker == _T("tr"))
				//	{
				//		// \tr always indicates that we are at the first cell in a row
				//		bAtFirstCellInRow = TRUE;

				//		// The marker is a new row marker
				//		if (!bProcessingTable)
				//		{
				//			// we are processing a table
				//			bProcessingTable = TRUE;
				//			// at the beginning of the table processing (before output of the first
				//			// table row's RTF tags, we need to determine the best table dimensions
				//			// so we must look ahead in the buffer to determine the dimensions we are
				//			// to expect for the RTF formatted table.
				//			if (bAtFirstTableRow)
				//			{
				//				// Scan ahead in the buffer to determine the dimensions of the how many rows are in the
				//				// forthcoming table, and determine optimum N values for \cellxN tags. This
				//				// scan must scan the entire table to determine bestTextExtents which could
				//				// be determined in the last row.
				//				GetTableDimensions(ptr, pBuffStart, pEnd, bestTextExtents,
				//					OutputSrc, numRows, numCols, MaxRowWidth);
				//				bAtFirstTableRow = FALSE;
				//				nLastRowIndex = numRows - 1;
				//				nLastColIndex = numCols - 1;
				//			}
				//			if (bLastTableRow)
				//			{
				//				bAtFirstTableRow = TRUE;
				//			}
				//			wxChar rowN[34];
				//			wxChar cellN[34];
				//			_itot(nCurrentRowIndex,rowN,10);
				//			MiscRTF = _T("\\par \\pard\\plain");	// add \par to beginnning of the prefix stuff
				//													// to ensure prev text doesn't end up in 1st
				//													// table cell
				//			MiscRTF += _T("\\trowd \\irow");
				//			MiscRTF += rowN;
				//			MiscRTF += _T("\\irowband");
				//			MiscRTF += rowN;
				//			MiscRTF += _T("\\ts") + TblGridSNum;
				//			MiscRTF += _T("\\trgaph40");
				//			MiscRTF += _T("\\trleft0\\ltrrow");

				//			// add the \cellxN data for the first row
				//			int colCt;
				//			for (colCt = 0; colCt < bestTextExtents.GetCount(); colCt++)
				//			{
				//				_itot(bestTextExtents.GetAt(colCt),cellN,10);
				//				wxString cellNStr = cellN;
				//				if (cellNStr != _T("0"))
				//				{
				//					MiscRTF += _T("\n\\cellx");
				//					MiscRTF += cellN;
				//				}
				//			}

				//			// now output the tags up to this point
				//			// RTF tags use gpApp->m_systemEncoding
				//			if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
				//				return;

				//		}
				//		else
				//		{
				//			// bProcessingTable was already true when the current \tr marker is
				//			// encountered. Since we've encountered this new \tr marker, it must
				//			// signal an end to the delimiting of rows of text with postpositioned
				//			// \cell markers, i.e., we've output the following string:
				//			//    {text1\cell text2\cell text3\cell text4
				//			// Note: The last column's text4 has been output, but, (if bLastCellTagOutput
				//			// is still FALSE, the closing "\cell " for that column's text has not been
				//			// output at this point. So,
				//			// First we need to close off the last column's text with "\cell " and add
				//			// the closing brace '}' to end the delimited representation of the row's text.
				//			MiscRTF = _T("\\cell }");
				//			if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
				//				return;
				//			bLastCellTagOutput = TRUE;

				//			// The remainder of the preceding row's row tags can now be output
				//			// Output the __normal paragraph style tags
				//			rtfIter = rtfTagsMap.find(_T("__normal"));
				//			if (rtfIter != rtfTagsMap.end())
				//			{
				//				// we found an associated value for Marker in map
				//				// RTF tags use gpApp->m_systemEncoding
				//				wxString tempStyle = (wxString)rtfIter->second;
				//				// remove the "\par " from the style tag string here
				//				tempStyle = tempStyle.Mid(5);
				//				if (!WriteOutputString(f,gpApp->m_systemEncoding,tempStyle))
				//					return;
				//			}
				//			// output the "{\trowd \irowN\irowbandN\tsN\trgaph40\trleft0\\ltrrow" part
				//			wxChar rowN[34];
				//			wxChar cellN[34];
				//			_itot(nCurrentRowIndex,rowN,10);// nCurrentRowIndex is incremented for new row below
				//											// so it is the index for the row we are closing off
				//			if (nCurrentRowIndex == nLastRowIndex)
				//				bLastTableRow = TRUE;
				//			MiscRTF = _T("\n{");
				//			MiscRTF += _T("\\trowd \\irow");
				//			MiscRTF += rowN;
				//			MiscRTF += _T("\\irowband");
				//			MiscRTF += rowN;
				//			MiscRTF += _T("\\ts") + TblGridSNum;
				//			if (bLastTableRow)
				//				MiscRTF += _T("\\lastrow");
				//			MiscRTF += _T("\\trgaph40");
				//			MiscRTF += _T("\\trleft0\\ltrrow");
				//			// add the \cellxN data for the first row
				//			int colCt;
				//			for (colCt = 0; colCt < bestTextExtents.GetCount(); colCt++)
				//			{
				//				_itot(bestTextExtents.GetAt(colCt),cellN,10);
				//				wxString cellNStr = cellN;
				//				if (cellNStr != _T("0"))
				//				{
				//					MiscRTF += _T("\n\\cellx");
				//					MiscRTF += cellN;
				//				}
				//			}
				//			// add the \row and closing brace '}'
				//			MiscRTF += _T("\\row }");

				//			// now output the tags up to this point
				//			// RTF tags use gpApp->m_systemEncoding
				//			if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
				//				return;

				//			nCurrentRowIndex++;
				//			nCurrentColIndex = 0; // new row starts at column zero
				//			bAtFirstCellInRow = TRUE;
				//			// Note: This ends the output of the previous row of the table
				//		}

				//		// Now we can output the initial tags for the new row that our current \tr
				//		// marker is starting. This amounts to "\pard\plain", the row's paragraph
				//		// style tags (based on 'm' style), and an opening brace for the cell text
				//		// line.

				//		//   Output the general paragraph style tags for the whole row (use the \m style)
				//		rtfIter = rtfTagsMap.find(_T("m"));
				//		if (rtfIter != rtfTagsMap.end())
				//		{
				//			// we found an associated value for Marker in map
				//			// RTF tags use gpApp->m_systemEncoding
				//			wxString tempStyle = (wxString)rtfIter->second;
				//			// remove the "\par " from the style tag string here
				//			tempStyle = tempStyle.Mid(5);

				//			// change any \qc justification to \ql which looks better
				//			// within table cells that have to wrap (regardless of user
				//			// setting for \m paragraphs
				//			int posqc = tempStyle.Find(_T("\\qc "));
				//			if (posqc != -1)
				//			{
				//				tempStyle.Remove(posqc,3);
				//				tempStyle.Insert(posqc,_T("\\ql"));
				//			}
				//			// output the modified "m" para style for the right justitied cell
				//			if (!WriteOutputString(f,gpApp->m_systemEncoding,tempStyle))
				//				return;
				//		}
				//		MiscRTF = _T("\n{");
				//		//   Output the opening curly brace for the cell text '{'
				//		if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
				//			return;
				//		LastParaStyle = Marker;
				//		LastNonBoxParaStyle = Marker;
				//		//
				//	}
				//	// Handle the table header and normal cell markers. These are "character" style but
				//	// have no end markers.
				//	else if (Marker.Find(_T("th")) == 0 || Marker.Find(_T("tc")) == 0)
				//	{
				//		// we should have processed at least one \tr tag and bProcessingTable should have
				//		// been set to TRUE before arriving here (unless the table was malformed)
				//		wxASSERT(bProcessingTable == TRUE);

				//		// Note: The processing of cell text items delimited by \cell tags is the same
				//		// regardless of which row we are processing.

				//		// If we are at a non-row-initial \th... or \tc... marker, we output the
				//		// "\cell " tag string here to close off the preceeding cell
				//		if (!bAtFirstCellInRow)
				//		{
				//			MiscRTF = _T("\\cell ");
				//			if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
				//				return;
				//			nCurrentColIndex++;	// the current col index increments when each
				//								// \cell is output
				//		}
				//		// Once \cell is output we are no longer at first cell in row
				//		bAtFirstCellInRow = FALSE;

				//		// Output the cell text items for the row delimited with \cell tags

				//		//   extract the nFoundColNum N of the marker, converting with _ttoi()
				//		wxString NvalueStr;
				//		if (Marker.Find(_T("thr")) == 0 || Marker.Find(_T("tcr")) == 0)
				//			NvalueStr = Marker.Mid(3);
				//		else if (Marker.Find(_T("th")) == 0 || Marker.Find(_T("tc")) == 0)
				//			NvalueStr = Marker.Mid(2);
				//		nFoundColNum = _ttoi(NvalueStr);
				//		if (nFoundColNum > maxUSFMCols)
				//		{
				//			nFoundColNum = maxUSFMCols; // set it to the maxUSFMCol
				//		}
				//		nCollIndexFound = nFoundColNum -1;

				//		// output any "\cell " tags for any empty columns in row. We do not output
				//		// the current assoc text of \th... or \tc... here, because we must allow
				//		// the processing of any embedded char format markers, footnotes, endnotes,
				//		// crossrefs, \bt..., \free, and \note material. We do output a "\cell "
				//		// tag string for a previous \th... or \tc... This situation is signalled
				//		// when bAtFirstCellInRow is FALSE
				//		int ct;
				//		int nSaveCurrColIndex = nCurrentColIndex;
				//		if (nCollIndexFound > nCurrentColIndex)
				//		{
				//			MiscRTF = _T("\\cell ");
				//			for (ct = 0; ct < nCollIndexFound - nSaveCurrColIndex; ct++);
				//			{
				//				if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
				//					return;
				//				nCurrentColIndex++; // the current col index increments when each
				//									// \cell is output
				//			}
				//		}

				//		// Note: Once we have processed the last "th" or "tc" item for the current row,
				//		// we need to close off the current row with a \row tag and closing brace '}'
				//		// We cannot do this here, however, even when we are at the last \th... or last
				//		// \tc... in a row, because we are only at the marker itself. We must allow a
				//		// block of the loop below to process the text assoc with this current marker
				//		// along with any embedded char format markers, footnotes, endnotes, crossrefs,
				//		// \bt..., \free, and \note material FIRST. The output of the closing tags
				//		// associated with the current row can occur in two places (1) in the \tr
				//		// handling block above (for ...), and (2) in the block below where we encounter
				//		// our first non-table marker indicating we've reached the end of the table.

				//		// The marker is a table column heading or a table cell data marker \thN \thrN \tcN or
				//		// \tcrN where N is 1, 2, 3, or 4. These are character markers but have no end forms
				//		// so we need to output an opening brace to start the character group, lookup and
				//		// output the RTF tags for the particular table char style, and then note that
				//		// we are within a table so we can determine when to close the groups for these
				//		// table markers.

				//		if (bLastParagraphWasBoxed)
				//		{
				//			// There was a small break paragraph inserted and no paragraph style
				//			// has intervened, so propagate the LastNonBoxParaStyle
				//			rtfIter = rtfTagsMap.find(LastNonBoxParaStyle);
				//			if (rtfIter != rtfTagsMap.end())
				//			{
				//				// we found an associated value for Marker in map
				//				// RTF tags use gpApp->m_systemEncoding
				//				if (!WriteOutputString(f,gpApp->m_systemEncoding,(wxString)rtfIter->second))
				//					return;
				//			}
				//			bLastParagraphWasBoxed = FALSE;
				//		}

				//		wxString tempMkrR;
				//		if (Marker.Find(_T("thr")) == 0 || Marker.Find(_T("tcr")) == 0)
				//		{
				//			// get the "m" marker from map in case we need it in block below
				//			rtfIter = rtfTagsMap.find(_T("m"));
				//			if (rtfIter != rtfTagsMap.end())
				//			{
				//				// we found an associated value for Marker in map
				//				// RTF tags use gpApp->m_systemEncoding
				//				tempMkrR = (wxString)rtfIter->second;
				//				// remove the "\par " from the style tag string here
				//			}
				//		}

				//		rtfIter = rtfTagsMap.find(Marker);
				//		if (rtfIter != rtfTagsMap.end())
				//		{
				//			if (Marker.Find(_T("thr")) == 0 || Marker.Find(_T("tcr")) == 0)
				//			{
				//				// for right aligned cells we need to repeat the "m" paragraph
				//				// style here before the character group for the cell, and the
				//				// "m" paragraph style should only have \pard prefixed to it
				//				// and its justification should be \qr rather than \qc or \ql
				//				// remove the "\par \n\pard\plain" and prefix just \pard
				//				tempMkrR = tempMkrR.Mid(18);
				//				// add \plain
				//				tempMkrR = _T("\\pard") + tempMkrR;
				//				// change justification to \qr
				//				int posqj = tempMkrR.Find(_T("\\qj "));
				//				if (posqj != -1)
				//				{
				//					tempMkrR.Remove(posqj,3);
				//					tempMkrR.Insert(posqj,_T("\\qr"));
				//				}
				//				int posqc = tempMkrR.Find(_T("\\qc "));
				//				if (posqc != -1)
				//				{
				//					tempMkrR.Remove(posqc,3);
				//					tempMkrR.Insert(posqc,_T("\\qr"));
				//				}
				//				// output the modified "m" para style for the right justitied cell
				//				MiscRTF = tempMkrR;
				//				if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
				//					return;
				//			}
				//			// we found an associated value for Marker in map
				//			if (bProcessingCharacterStyle)
				//			{
				//				// non-paragraph style strings need to start with an opening brace {
				//				MiscRTF = _T("{");
				//				// RTF tags use gpApp->m_systemEncoding
				//				if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
				//					return;
				//			}
				//			// RTF tags use gpApp->m_systemEncoding
				//			if (!WriteOutputString(f,gpApp->m_systemEncoding,(wxString)rtfIter->second))
				//				return;
				//		}

				//		// If bProcessingEndlessCharMarker is FALSE we are starting to process a marker that
				//		// defines a character style but has no ending marker
				//		if (!bProcessingEndlessCharMarker)
				//		{
				//			bProcessingEndlessCharMarker = TRUE;
				//		}
				//		LastStyle = Marker;
				//		// Don't update LastParaStyle here - the above table markers are character style
				//		// markers
				//	}

				//}

				// BELOW WAS THE ORIGINAL BLOCK FOR PROCESSING CHARACTER TABLE STYLES:
				// This block did not format USFM tables into real RTF tables, but treated
				// the table rows (defined by \tr) as simple paragraphs
				// Handle the table markers which are "character" style but have no end markers
				else if (Marker.Find(_T("th")) == 0 || Marker.Find(_T("tc")) == 0)
				{
					// The marker is a table column heading or a table cell data marker \thN \thrN \tcN or
					// \tcrN where N is 1, 2, 3, or 4. These are character markers but have no end forms
					// so we need to output an opening brace to start the character group, lookup and
					// output the RTF tags for the particular table char style, and then note that
					// we are within a table so we can determine when to close the groups for these
					// table markers.

					if (bLastParagraphWasBoxed)
					{
						// There was a small break paragraph inserted and no paragraph style
						// has intervened, so propagate the LastNonBoxParaStyle
						rtfIter = rtfTagsMap.find(LastNonBoxParaStyle);
						if (rtfIter != rtfTagsMap.end())
						{
							// we found an associated value for Marker in map
							// RTF tags use gpApp->m_systemEncoding
							wxString lastStyTag = (wxString)rtfIter->second;
							CountTotalCurlyBraces(lastStyTag,nOpeningBraces,nClosingBraces);
							if (!WriteOutputString(f,gpApp->m_systemEncoding,lastStyTag))
							{
								pProgDlg->Destroy();
								return;
							}
						}
						bLastParagraphWasBoxed = FALSE;
					}

					rtfIter = rtfTagsMap.find(Marker);
					if (rtfIter != rtfTagsMap.end())
					{
						// we found an associated value for Marker in map
						if (bProcessingCharacterStyle)
						{
							// non-paragraph style strings need to start with an opening brace {
							MiscRTF = _T("{");
							// RTF tags use gpApp->m_systemEncoding
							CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // one opening brace here
							if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
							{
								pProgDlg->Destroy();
								return;
							}
							
							pCharStack->Push(Marker.char_str()); // push a TABLE (\th... or \tc...) character style marker
						}
						// RTF tags use gpApp->m_systemEncoding
						wxString mkrTags = (wxString)rtfIter->second;
						CountTotalCurlyBraces(mkrTags,nOpeningBraces,nClosingBraces);
						if (!WriteOutputString(f,gpApp->m_systemEncoding,mkrTags))
						{
							pProgDlg->Destroy();
							return;
						}
					}

					// If bProcessingEndlessCharMarker is FALSE we are starting to process a marker that
					// defines a character style but has no ending marker
					if (!bProcessingEndlessCharMarker)
					{
						bProcessingEndlessCharMarker = TRUE;
					}
					LastStyle = Marker;
					// Don't update LastParaStyle here - the above table markers are character style
					// markers
				}

				// Handle any back translation markers beginning with \bt... which are "character"
				// style but have no end markers
				else if (Marker.Find(_T("bt")) == 0)
				{
					// Note: To get the back translation material to follow the text to which
					// it applies, we first output bt... associated strings for any preceding bt...
					// marker at this current bt marker's occurrence (a halting point); then we
					// process the bt... material for the current Marker and store it to output at
					// the next halting point (which would usually be another bt marker if the
					// text is fully free translated); or at the end of the Buffer if no more bt...
					// markers or halting points exist.
					if (!btAssocStr.IsEmpty())
					{
						bool bIsAtEnd = FALSE;
						if (!OutputAnyBTorFreeMaterial(f,gpApp->m_systemEncoding,Marker,_T("bt"),btAssocStr,
							LastStyle,LastParaStyle,btRefNumInt,bLastParagraphWasBoxed,
							parseError,callerType,bProcessingTable,bPlaceBackTransInRTFText,
							single_border,pDoc))
						{
							pProgDlg->Destroy();
							return;
						}
						// We have output the current \bt material at a succeeding \bt point, so
						// reset our bHitBTHaltingMkr flag.
						bHitBTHaltingMkr = FALSE;
						if (bIsAtEnd)
						{
							goto b;
						}
					}

					lastBTMarker = Marker;	// used in OutputAnyBTorFreeMaterial() at bottom of loop
											// where bt material is placed after gap and where Marker
											// is a subsequent non- bt marker.
					// Now process the current bt... marker and assoc text.
					// Parse the \bt... to retrieve its marker and associated text, then
					// use that text above for formatting a boxed paragraph or footnote
					// depending on the value of bPlaceBackTransInRTFText.
					wxString wholeMarker = _T('\\') + Marker;
					wxString btMarker = _T("bt"); // use only the short "bt" form
					itemLen = ParseMarkerAndAnyAssociatedText(ptr,pBufStart,pEnd,btMarker,wholeMarker,TRUE,FALSE);
					// TRUE above means we expect RTF text to parse
					// FALSE above means don't include char format markers
					wxString btStr;
					btStr = wxString(ptr,itemLen);
					// btStr still starts with \bt so just remove the backslash leaving the bare marker
					// to function as caller when the string is used as a footnote, and add back the
					// wholeMarker and space prefixed
					btStr = btStr.Mid(1);
					//if (btStr.Find(_T('\n')) != -1)
					btStr.Replace(_T("\n"),_T(" "));
					btStr.Replace(_T("\r"),_T(" "));
					btAssocStr = btStr;
				}
				// Handle any Adapt It Note markers beginning with \note
				else if (Marker == _T("note"))
				{
					// Parse the \note marker to retrieve its marker and associated text, then
					// use that text below for the special formatting circumstances depending on the
					// value of bPlaceAINotesInRTFText.
					wxString wholeMarker = _T('\\') + Marker;
					wxString noteMarker = _T("note");
					itemLen = ParseMarkerAndAnyAssociatedText(ptr,pBufStart,pEnd,noteMarker,wholeMarker,TRUE,FALSE);
					// TRUE above means we expect RTF text to parse
					// FALSE above means don't include char format markers
					wxString noteStr;
					noteStr = wxString(ptr,itemLen);
					// noteStr still starts with \note so just remove the backslash leaving the bare marker
					// to function as caller when the string is used as a footnote, and add back the
					// wholeMarker and space prefixed
					noteStr.Remove(noteStr.Find(_T("\\note*")),6);

					if (bPlaceAINotesInRTFText)
					{
						// The ExportOptionsDlg checkbox specifies that Adapt It Notes should be
						// "placed in Comments (bubble text) within the right margin. We do this by
						// formatting the note as an RTF annotation using the _annotation_text and
						// _annotation_ref marker styles.

						noteStr.Remove(0,6); // delete the "\note " string prefix

						// first output opening brace for the _annotation_ref style
						MiscRTF = _T('{');
						CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // one opening brace here
						if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						{
							pProgDlg->Destroy();
							return;
						}
						// next output the \cs style tags for _annotation_ref
						rtfIter = rtfTagsMap.find(_T("_annotation_ref"));
						if (rtfIter != rtfTagsMap.end())
						{
							// we found an associated value for Marker in map
							// RTF tags use gpApp->m_systemEncoding
							wxString annotRefTags = (wxString)rtfIter->second;
							CountTotalCurlyBraces(annotRefTags,nOpeningBraces,nClosingBraces);
							if (!WriteOutputString(f,gpApp->m_systemEncoding,annotRefTags))
							{
								pProgDlg->Destroy();
								return;
							}
						}
						// next output the required RTF tags to prefix an annotation
						MiscRTF = _T("{\\*\\atnid Adapt It Note:}{\\*\\atnauthor       }\\chatn {\\*\\annotation \\pard\\plain ");
						CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // 3 open, 2 close braces added here
						if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						{
							pProgDlg->Destroy();
							return;
						}
						// output the _annotation_text paragraph style tags
						rtfIter = rtfTagsMap.find(_T("_annotation_text"));
						if (rtfIter != rtfTagsMap.end())
						{
							// we found an associated value for Marker in map
							// RTF tags use gpApp->m_systemEncoding
							wxString annotTextTags = (wxString)rtfIter->second;
							CountTotalCurlyBraces(annotTextTags,nOpeningBraces,nClosingBraces);
							if (!WriteOutputString(f,gpApp->m_systemEncoding,annotTextTags))
							{
								pProgDlg->Destroy();
								return;
							}
						}
						// output the _annotation_ref style tags again
						MiscRTF = _T('{');
						CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // one open brace here
						if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						{
							pProgDlg->Destroy();
							return;
						}
						// next output the \cs style tags for _annotation_ref
						rtfIter = rtfTagsMap.find(_T("_annotation_ref"));
						if (rtfIter != rtfTagsMap.end())
						{
							// we found an associated value for Marker in map
							// RTF tags use gpApp->m_systemEncoding
							wxString annotRefTags = (wxString)rtfIter->second;
							CountTotalCurlyBraces(annotRefTags,nOpeningBraces,nClosingBraces);
							if (!WriteOutputString(f,gpApp->m_systemEncoding,annotRefTags))
							{
								pProgDlg->Destroy();
								return;
							}
						}
						MiscRTF = _T("\\chatn }{");
						CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // one open one closed added here
						if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						{
							pProgDlg->Destroy();
							return;
						}
						// now output the actual note string
						CountTotalCurlyBraces(noteStr,nOpeningBraces,nClosingBraces);
						// whm 8Nov07 changed below to use m_tgtEncoding to force
						// the use of the \uN\'f3 RTF Unicode chars format.
						if (!WriteOutputString(f,gpApp->m_tgtEncoding,noteStr)) // use m_tgtEncoding here
						{
							pProgDlg->Destroy();
							return;
						}
						MiscRTF = _T("}}}"); // closing braces for note (annotation)
						CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // 3 closing curly braces here
						if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						{
							pProgDlg->Destroy();
							return;
						}
						// update LastStyle only when doing boxed paragraph
						LastStyle = Marker;
					}
					else
					{
						// The ExportOptionsDlg checkbox specifies that the Adapt It note should be
						// formatted as footnotes. We do this by enclosing the \note character style
						// and its text within the footnote destination set of tags. We use a
						// special literal caller "note" in the text with the note itself
						// in the footnote at the foot of the page. This can be handled with our
						// ProcessAndWriteDestinationText() function
						noteStr = noteStr.Mid(1); // just remove the backslash and leave "note" for caller
						noteStr = wholeMarker + _T(' ') + noteStr + _T("\\f* "); // make it end like a footnote by
																			// adding "\f* " so our function
																			// ProcessAndWriteDestinationText
																			// can handle it like one

						bool bIsAtEnd = FALSE;; // set by ProcessAndWriteDestinationText() below
						// construct numerically sequenced caller
						//wxChar buf[34];
						noteRefNumInt++; // increment the bt N to note 1, note 2, note 3, etc.
						wxString bareNoteMarker = Marker; // backslash already removed
						noteRefNumStr = bareNoteMarker + _T(' '); // "note "
						noteRefNumStr << noteRefNumInt; // _itot(noteRefNumInt,buf,10);  // add N to "note N"
						noteRefNumStr += _T(' ');
						wxString callerStr = noteRefNumStr;
						callerType = supplied_by_parameter;

						wxString nullStr = _T("");
						// we'll use system encoding to write the note text
						//if (!ProcessAndWriteDestinationText(f, gpApp->m_systemEncoding, noteStr,
						//	bIsAtEnd, footnoteDest, rtfTagsMap, pDoc, parseError, callerType,
						//	callerStr, FALSE, nullStr)) // FALSE because there is no free trans to suffix to a note
						// whm 8Nov07 note: We should use m_tgtEncoding to force the writing of the
						// note text in the \uN\'f3 RTF Unicode format
						if (!ProcessAndWriteDestinationText(f, gpApp->m_tgtEncoding, noteStr,
							bIsAtEnd, footnoteDest, rtfTagsMap, pDoc, parseError, callerType,
							callerStr, FALSE, nullStr)) // FALSE because there is no free trans to suffix to a note
						{
							pProgDlg->Destroy();
							return;
						}
						if (bIsAtEnd) // ProcessAndWriteDestinationText got to the end of the string
						{
							goto b; //check for another marker beyond what ParseFootnote processed
						}
					}
					// Adapt It Notes only appear as balloon text comments or footnotes; they don't
					// appear as separate paragraphs within the text so we don't set LastStyle
					// or LastParaStyle or LastNonBoxParaStyle here
				}
				// Handle any free translation markers beginning with \free
				else if (Marker == _T("free"))
				{
					// Note: To get the free translation material to follow the text to which
					// it applies, we first output free associated strings for any preceding free
					// marker at this current free marker's occurrence (a halting point); then we
					// process the free material for the current Marker and store it to output at
					// the next halting point (which would usually be another \free marker if the
					// text is fully free translated); or at the end of the Buffer if no more free
					// markers or halting points exist.

					if (!freeAssocStr.IsEmpty())
					{
						bool bIsAtEnd = FALSE;
						if (!OutputAnyBTorFreeMaterial(f,gpApp->m_systemEncoding,Marker,_T("free"),freeAssocStr,
							LastStyle,LastParaStyle,freeRefNumInt,bLastParagraphWasBoxed,
							parseError,callerType,bProcessingTable,bPlaceFreeTransInRTFText,
							double_border,pDoc))
						{
							pProgDlg->Destroy();
							return;
						}
						// We have output the current \free material at a succeeding \free point, so
						// reset our bHitFreeHaltingMkr flag.
						bHitFreeHaltingMkr = FALSE;
						if (bIsAtEnd)
						{
							goto b;
						}
					}

					lastFreeMarker = Marker;// used in OutputAnyBTorFreeMaterial() at bottom of loop
											// where free material is placed after gap and where Marker
											// is a subsequent non- free marker.
					// Now process the current free marker and assoc text.
					// Parse the \free to retrieve its marker and associated text, then
					// use that text above for the special formatting circumstances depending on the
					// value of bPlaceFreeTransInRTFText.
					wxString wholeMarker = _T('\\') + Marker;
					wxString freeMarker = _T("free");
					itemLen = ParseMarkerAndAnyAssociatedText(ptr,pBufStart,pEnd,freeMarker,wholeMarker,TRUE,FALSE);
					// TRUE above means we expect RTF text to parse
					// FALSE above means don't include char format markers
					wxString freeStr;
					freeStr = wxString(ptr,itemLen);
					// freeStr still starts with \free so just remove the backslash leaving the bare marker
					// to function as caller when the string is used as a footnote, and add back the
					// wholeMarker and space prefixed
					freeStr = freeStr.Mid(1);
					// remove the \free* end marker
					int freeEndMkrPos = freeStr.Find(_T("\\free*"));
					freeStr = freeStr.Left(freeEndMkrPos);
					freeAssocStr = freeStr;
					// Is this a free translation of a following footnote, endnote or crossref?
					// If so, set flag to have ProcessAndWriteDestinationText() suffix the
					// free translation to the end of the footnote text formatted in the
					// appropriate boxed paragraph
					if (NextMarkerIsFootnoteEndnoteCrossRef(ptr,pEnd,itemLen))
					{
						bHasFreeTransToAddToFootnoteBody = TRUE;
					}
					else
					{
						bHasFreeTransToAddToFootnoteBody = FALSE;
					}
				}
				// The special cases have been handled, so now handle the regular end markers
				// Handle character end markers
				else if (Marker.Find(_T('*')) != -1)
				{
					// We are at an end marker (Marker has * in it)
					// add the group closing brace } but only if the styleType of the marker
					// is "character"
					// Note: In situations where user has an end marker in the text but it had
					// no corresponding begin marker, bProcessingCharacterStyle would normally
					// be false and no spurious closing brace would be added in the code below.
					if (bProcessingCharacterStyle)
					{
						MiscRTF = _T('}');
						CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // one closing curly brace here
						if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						{
							pProgDlg->Destroy();
							return;
						}
						// if the previous marker was a small break paragraph, we need to propagate
						// the LastNonBoxParaStyle
						// TODO: Check if the following propagation should only be done following
						// a small para break, i.e., when bLastParagraphWasBoxed == TRUE
						//if (bLastParagraphWasBoxed)
						{
							rtfIter = rtfTagsMap.find(LastNonBoxParaStyle);
							if (rtfIter != rtfTagsMap.end())
							{
								// we found an associated value for Marker in map
								wxString lastNBParaStyle = (wxString)rtfIter->second;
								// remove the \par from the style string here because we don't want to
								// insert a paragraph at closing of a character style, just propagate the
								// previous non-boxed paragraph style
								int nbPos = lastNBParaStyle.Find(_T("\\par ")); // whm 18Nov10 added space to find string; previously was "\\par"
								lastNBParaStyle.Remove(nbPos, 5);
								CountTotalCurlyBraces(lastNBParaStyle,nOpeningBraces,nClosingBraces);
								if (!WriteOutputString(f,gpApp->m_systemEncoding,lastNBParaStyle))
								{
									pProgDlg->Destroy();
									return;
								}
							}
						}

						Item sfm1;
						pCharStack->Pop(sfm1); // pop a character style END marker
						// Check if the stack has another character style marker in it.
						// If not, we can set the bProcessingCharacterStyle flag to FALSE.
						// If so, we need to reset the character style to propagate that
						// character style after the current one has closed.
						if (pCharStack->IsEmpty())
						{
							bProcessingCharacterStyle = FALSE;
						}
						else
						{
							// There is another character style marker in the stack
							// so we need to propagate that character style
							Item sfm2;
							pCharStack->Pop(sfm2);
							wxString sfmMkr = wxString::FromAscii(sfm2);
							rtfIter = rtfTagsMap.find(sfmMkr);
							if (rtfIter != rtfTagsMap.end())
							{
								// We found an associated value for Marker in map.
								// We need only output the previous char style tags here without any 
								// opening curly brace. 
								
								pCharStack->Push(Marker.char_str()); // push it back on the stack

								// RTF tags use gpApp->m_systemEncoding
								wxString mkrTags = (wxString)rtfIter->second;
								CountTotalCurlyBraces(mkrTags,nOpeningBraces,nClosingBraces);
								if (!WriteOutputString(f,gpApp->m_systemEncoding,mkrTags))
								{
									pProgDlg->Destroy();
									return;
								}
							}
						}
						
						// Marker is either not a character style or is the same style as the last style
						// marker encountered. 
						bProcessingCharacterStyle = FALSE; // we've finished processing the char style group
					}
				}
				else // all remaining marker/styles
				{
					// we've dealt with the "problem" styles - all remaining ones should be
					// straight forward.
					// Note: Marker will not have any end forms with asterisks nor unknown
					// markers at this point because these were handled in an else if block
					// above.

					//if (bProcessingTable && !bProcessingCharacterStyle)
					//{
					//	// we've been processing a USFM table and have come to a non-table,
					//	// non-character formatting marker which signals the end of the table.
					//	// We need to signal we've reached the end of the table
					//	bProcessingTable = FALSE;
					//	// Since the \cell markers are output after the actual cell text, we need to
					//	// output a final "\cell " tag string to close off the last cell of the last
					//	// row of the table
					//	MiscRTF = _T("\\cell }");
					//	if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
					//		return;
					//	// No need to increment nCurrentColIndex here because it gets reset below.

					//	// The remainder of the preceding row's row tags can now be output
					//	// output \pard\plain
					//	//MiscRTF = _T("\n\\pard\\plain");
					//	//if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
					//	//	return;
					//	// Output the __normal paragraph style tags
					//	rtfIter = rtfTagsMap.find(_T("__normal"));
					//	if (rtfIter != rtfTagsMap.end())
					//	{
					//		// we found an associated value for Marker in map
					//		// RTF tags use gpApp->m_systemEncoding
					//		wxString tempStyle = (wxString)rtfIter->second;
					//		// remove the "\par " from the style tag string here
					//		tempStyle = tempStyle.Mid(5);
					//		if (!WriteOutputString(f,gpApp->m_systemEncoding,tempStyle))
					//			return;
					//	}
					//	// output the "{\trowd \irowN\irowbandN\tsN\trgaph40\trleft0\\ltrrow" part
					//	wxChar rowN[34];
					//	wxChar cellN[34];
					//	_itot(nCurrentRowIndex,rowN,10);// nCurrentRowIndex is incremented for new row below
					//									// so it is the index for the row we are closing off
					//	if (nCurrentRowIndex == nLastRowIndex)
					//		bLastTableRow = TRUE;
					//	MiscRTF = _T("\n{");
					//	MiscRTF += _T("\\trowd \\irow");
					//	MiscRTF += rowN;
					//	MiscRTF += _T("\\irowband");
					//	MiscRTF += rowN;
					//	MiscRTF += _T("\\ts") + TblGridSNum;
					//	if (bLastTableRow)
					//		MiscRTF += _T("\\lastrow");
					//		MiscRTF += _T("\\trgaph40");
					//	MiscRTF += _T("\\trleft0\\ltrrow");
					//	// add the \cellxN data for the first row
					//	int colCt;
					//	for (colCt = 0; colCt < bestTextExtents.GetCount(); colCt++)
					//	{
					//		_itot(bestTextExtents.GetAt(colCt),cellN,10);
					//		wxString cellNStr = cellN;
					//		if (cellNStr != _T("0"))
					//		{
					//			MiscRTF += _T("\n\\cellx");
					//			MiscRTF += cellN;
					//		}
					//
					//	}
					//	// add the \row and closing brace '}'
					//	MiscRTF += _T("\\row }");

					//	// now output the tags up to this point
					//	// RTF tags use gpApp->m_systemEncoding
					//	if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
					//		return;

					//	nCurrentRowIndex++;
					//	nCurrentColIndex = 0; // new row starts at column zero
					//	bAtFirstCellInRow = TRUE;
					//	// Note: This ends the output of the previous row of the table

					//	// Note: In the event that the last \tc... table cell marker is at the end of
					//	// the file, this block (which was entered because of encountering a non-table
					//	// marker, will not be entered and this \cell tag will not be output, therefor
					//	// the tag bLastCellTagOutput below will signal that fact so that it can be
					//	// processed elsewhere if it is still FALSE.
					//	bLastCellTagOutput = TRUE;

					//	// Ensure table related int vars are correctly set in case a subsequent table
					//	// is encountered
					//	nCurrentRowIndex = 0;
					//	nLastRowIndex = 0;
					//	nCurrentColIndex = 0;

					//	// the following is used in the longer routine that attempts to format USFM
					//	// tables as real Word tables.
					//	//nLastColIndex = 0;

					//	// Also we should ensure other table related flags are correctly set in case a
					//	// subsequent table is encountered
					//	bAtFirstTableRow = TRUE;
					//	bAtFirstCellInRow = TRUE;
					//	bLastTableRow = FALSE;


					//	// clear out the arrays
					//	bestTextExtents.RemoveAll();
					//	cellText.RemoveAll();
					//	rightAlignment.RemoveAll();

					//	// lastly reset the overall table processing flag
					//	bProcessingTable = FALSE;
					//}
					//else
					//{
					//	bLastCellTagOutput = FALSE; 
					//}

					// Handle any pending bt and/or free material that should be output before
					// the current Marker.
					if (!btAssocStr.IsEmpty())
					{
						// We have \bt material that is pending output at an appropriate halting point.
						if (IsBTMaterialHaltingPoint(Marker))
						{
							// We are currently at a halting point. The first halting point immediately
							// following \bt material is still preceding the material to which the \bt
							// material applies. We only process the pending \bt material if we have
							// already hit one halting marker.
							if (bHitBTHaltingMkr)
							{
								// We've already hit a halting marker and are currently at the next halting
								// marker. This is the first output opportunity following the material to
								// which the \bt material applies, and we output the pending \bt material
								// here.
								// NOTE: The Marker variable here will not be a \bt... marker but
								// some subsequent marker. We want to feed the last actual \bt...
								// marker to OutputAnyBTorFreeMaterial() below because it uses it
								// to format the bt... caller. The last actual \bt... marker is
								// stored in the string lastBTMarker.
								bool bIsAtEnd = FALSE;
								if (!OutputAnyBTorFreeMaterial(f,gpApp->m_systemEncoding,lastBTMarker,_T("bt"),
									btAssocStr,
									LastStyle,LastParaStyle,btRefNumInt,bLastParagraphWasBoxed,
									parseError,callerType,bProcessingTable,bPlaceBackTransInRTFText,
									single_border,pDoc))
								{
									pProgDlg->Destroy();
									return;
								}
								// Note: when OutputAnyBTorFreeMaterial is called it calls btAssocStr.Empty()
								// which signals that there is no \bt material currently pending.
								if (bIsAtEnd)
								{
									goto b;
								}
								bHitBTHaltingMkr = FALSE;
							}
							else
							{
								// We're at the first halting point. We don't output anything here, but
								// set the bHitBTHaltingMkr for the next pass.
								bHitBTHaltingMkr = TRUE;
							}
						}
						else
						{
							// This Marker isn't a good halting point, so continue looking.
							;
						}
					}

					if (!freeAssocStr.IsEmpty())
					{
						// We have \free material that is pending output at an appropriate halting point.
						if (IsFreeMaterialHaltingPoint(Marker))
						{
							// We are currently at a halting point. The first halting point immediately
							// following \free material is still preceding the material to which the \free
							// material applies. We only process the pending \free material if we have
							// already hit one halting marker.
							if (bHitFreeHaltingMkr)
							{
								// We've already hit a halting marker and are currently at the next halting
								// marker. This is the first output opportunity following the material to
								// which the \free material applies, and we output the pending \free material
								// here.
								// NOTE: The Marker variable here will not be a \free marker but
								// some subsequent marker. We want to feed the last actual \free
								// marker to OutputAnyBTorFreeMaterial() below because it uses it
								// to format the free caller. The last actual \free marker is
								// stored in the string lastFreeMarker.
								bool bIsAtEnd = FALSE;
								if (!OutputAnyBTorFreeMaterial(f,gpApp->m_systemEncoding,lastFreeMarker,_T("free"),
									freeAssocStr,
									LastStyle,LastParaStyle,freeRefNumInt,bLastParagraphWasBoxed,
									parseError,callerType,bProcessingTable,bPlaceFreeTransInRTFText,
									double_border,pDoc))
								{
									pProgDlg->Destroy();
									return;
								}
								// Note: When OutputAnyBTorFreeMaterial is called it calls freeAssocStr.Empty()
								// which signals that there is no \free material currently pending.
								if (bIsAtEnd)
								{
									goto b;
								}
								bHitFreeHaltingMkr = FALSE;
							}
							else
							{
								// We're at the first halting point. We don't output anything here, but
								// set the bHitFreeHaltingMkr for the next pass.
								bHitFreeHaltingMkr = TRUE;
							}
						}
						else
						{
							// This Marker isn't a good halting point, so continue looking.
							;
						}
					}

					if (bProcessingCharacterStyle && bLastParagraphWasBoxed)
					{
						MiscRTF = _T("\\par ") + gpApp->m_eolStr; // insert \par tag and new line into RTF file
						// RTF tags use gpApp->m_systemEncoding
						CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // no curly braces added here
						if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						{
							pProgDlg->Destroy();
							return;
						}
					}

					if (Marker == LastNonBoxParaStyle
						//&& !bProcessingTable
						&& !bProcessingCharacterStyle
						&& !bLastParagraphWasBoxed
						)
					{
						// Most of the "non-problem" paragraph style markers go through here.
						// The marker was a non-boxed paragraph seen just before the current marker,
						// and no small break paragraph has intervened.
						// Output the \par paragraph mark.
						MiscRTF = _T("\\par ") + gpApp->m_eolStr; // insert \par tag and new line into RTF file
						// RTF tags use gpApp->m_systemEncoding
						CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // no curly braces added here
						if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						{
							pProgDlg->Destroy();
							return;
						}
					}
					else
					{
						// Most of the "non-problem" character style markers go through here.
						// The marker is a non-paragraph style, or there was an intervening
						// small paragraph - in any case we need to output the full complement
						// of RTF indoc tags for this style.
						rtfIter = rtfTagsMap.find(Marker);
						if (rtfIter != rtfTagsMap.end())
						{
							// we found an associated value for Marker in map
							if (bProcessingCharacterStyle)
							{
								// non-paragraph style strings need to start with an opening brace {
								MiscRTF = _T("{");
								// RTF tags use gpApp->m_systemEncoding
								CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // one opening curly brace added here
								if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
								{
									pProgDlg->Destroy();
									return;
								}
								
								pCharStack->Push(Marker.char_str()); // push a character style marker
							}
							// RTF tags use gpApp->m_systemEncoding
							wxString mkrTags = (wxString)rtfIter->second;
							CountTotalCurlyBraces(mkrTags,nOpeningBraces,nClosingBraces);
							if (!WriteOutputString(f,gpApp->m_systemEncoding,mkrTags))
							{
								pProgDlg->Destroy();
								return;
							}
						}
					}


					bLastOutputWasParTag = TRUE;
					bLastParagraphWasBoxed = FALSE;
					LastStyle = Marker;
					if (!bProcessingCharacterStyle)
					{
						LastParaStyle = Marker;
						LastNonBoxParaStyle = Marker;
					}
					else
					{
						LastCharacterStyle = Marker;
					}

				} // end of else - process all other non-special treatment markers

				ptr += itemLen;	// advance pointer past the marker

				itemLen = pDoc->ParseWhiteSpace(ptr); // parse white space following the marker
				// Omit output of white space here when there is punctuation following the whitespace, 
				// otherwise include the white space in the output, but only for whitespace following
				// end markers.
				if (Marker.Find(_T('*')) == (int)Marker.Length()-1 && ptr + itemLen + 1 < pEnd && spaceless.Find(*(ptr + itemLen + 1)) == wxNOT_FOUND)
				{
					// We just processed an end marker, and the first char past whitespace is not a
					// punctuation char, so output the whitespace. This is needed following character 
					// end markers.
					// white space here usually would be part of vernacular so use EncodingSrcOrTgt
					// but don't output \n new lines
					WhiteSpace = wxString(ptr,itemLen);//testing only
					WhiteSpace.Replace(_T("\n"),_T(" "));
					WhiteSpace.Replace(_T("\r"),_T(" "));
					while (WhiteSpace.Find(_T("  ")) != -1)
					{
						WhiteSpace.Remove(WhiteSpace.Find(_T("  ")),1);
					}
					if (!WriteOutputString(f,EncodingSrcOrTgt,WhiteSpace))
					{
						pProgDlg->Destroy();
						return;
					}
				}
				else
				{
					// the first char past whitespace is a punctuation char, so don't output the
					// whitespace.
					;
				}
				ptr += itemLen;	// point past it

				goto b;	// check if another marker follows
			}// end of else some other kind besides verse or chapter marker
		}// end of if IsMarkerRTF
		else
		{
			// must be a word or special text, normally vernacular
			//
			// whm added 8Nov07 check for end of text
			// are we at the end of the text?
			if (pDoc->IsEnd(ptr) || ptr >= pEnd)
				goto d;

			if (bLastParagraphWasBoxed)
			{
				// There was a small break paragraph inserted and no paragraph style
				// has intervened, so propagate the LastNonBoxParaStyle
				rtfIter = rtfTagsMap.find(LastNonBoxParaStyle);
				if (rtfIter != rtfTagsMap.end())
				{
					// we found an associated value for Marker in map
					// RTF tags use gpApp->m_systemEncoding
					wxString lastStyTags = (wxString)rtfIter->second;
					CountTotalCurlyBraces(lastStyTags,nOpeningBraces,nClosingBraces);
					if (!WriteOutputString(f,gpApp->m_systemEncoding,lastStyTags))
					{
						pProgDlg->Destroy();
						return;
					}
				}
				bLastParagraphWasBoxed = FALSE;
			}
			// whm 8Nov05 - Use Bruce's ParseWord() function. We'll then process
			// any preceding or following punctuation and output any quotes with
			// the appropriate RTF tags. To do that we'll need to identify the
			// quote elements which can be single quotes (RTF tags are \lquote and
			// \rquote) and double quotes (RTF tags are \ldblquote and \rdglquote).
			//
			precPunct.Empty();
			follPunct.Empty();

			itemLen = ParseWordRTF(ptr, precPunct,follPunct,spaceless);

			// make the word into a wxString
			VernacText = wxString(ptr,itemLen);
			bLastOutputWasParTag = FALSE;

			// Note: Version 3 no longer checks for bar codes because at least one
			// Unicode Adapt It user had problems because the bar code was interpreted
			// wrongly as formatting code when it should have been vernacular text.
			// Users who previously used bar codes will simply have to use the appropriate
			// USFM markers.

			// QUOTES: ParseWord identifies any preceding and following punctuation and
			// places a copy of the leading punctuation in precPunct, and a copy of the
			// following punctuation in follPunct, but it leaves all punctuation on the
			// VernacText word itself. Also, itemLen includes the length of all punctuation
			// on the word.
			// For Unicode and ANSI quote marks we just pass those characters through to
			// output. However, ParseWord also recognizes <, >, << and >> as quote characters
			// when used on the borders of words. We will convert them to their equivalent
			// RTF quote tags as we did in version 2.

			// whm added 8Nov07. Behavior of IsMarkerRTF has changed so that it returns FALSE
			// for the escaped RTF character \\, as well as for \{, and \}. We need to detect this 
			// situation here and parse through such escaped characters as valid text (previously 
			// they were wrongly detected as unknown markers and processed in the block above before
			// the present else block). After correcting the behavior of IsMarkerRTF so that it
			// returns FALSE for the escaped backslash sequence \\, it became possible for 
			// ProcessAndWriteDestinationText to get into an infinite loop causing a program
			// hang/crash. This occurs due to the fact that when ptr points at the initial 
			// backslash of an escaped character, VernacText resolves to an empty string and 
			// itemLen from subsequent parsing functions is always set to zero resulting in 
			// ptr not advancing through the remainder of the destination text. We need to: 
			// (1) parse through any escaped \\, \{, or \} sequences, and (2) ensure that, if
			// VernacText resolves to an empty string (for any other unanticipated reason), the
			// current while loop can continue advancing through the destination string.
			if (VernacText.IsEmpty())
			{
				// we likely have an escaped \\, \{, or \} sequence so parse it
				itemLen = ParseEscapedCharSequence(ptr,pEnd);
				// if itemLen is zero at this point, we had an empty VernacText for some 
				// other unknown reason. In this case it is best to simply advance ptr and 
				// goto b to check for another marker or pEnd.
				if (itemLen == 0)
				{
					ptr++;
					goto b; // check for another marker
				}
				// make the escaped char sequence into a wxString
				VernacText = wxString(ptr,itemLen);
				ptr += itemLen;
				wxASSERT(VernacText != _T(""));
				if (!WriteOutputString(f,EncodingSrcOrTgt,VernacText))
				{
					pProgDlg->Destroy();
					return;
				}
				goto b; // check for another marker
			}

			if (VernacText.Find(_T("|@")) != -1)
			{
				wxASSERT(VernacText.Find(_T("@|")) != -1); // closing marks should also be present
				// this word is the free translation word count of the form |@N@| where N is an
				// number character string. We have no use for this "word" in RTF output so we
				// will simply omit it from output
				ptr += itemLen;
				// no output of this word here
				itemLen = pDoc->ParseWhiteSpace(ptr);
				// also parse the whitespace following
				ptr += itemLen;
				// no output of this whitespace here
			}
			else if (VernacText.Find(_T('<')) == -1
				&& VernacText.Find(_T('>')) == -1)
			{
				// there are no angle quote marks so output in vernacular encoding
				CountTotalCurlyBraces(VernacText,nOpeningBraces,nClosingBraces);
				if (!WriteOutputString(f,EncodingSrcOrTgt,VernacText))
				{
					pProgDlg->Destroy();
					return;
				}
				ptr += itemLen;
			}
			else
			{
				// there is at least one angle quote mark in VernacText so we'll convert
				// them to the appropriate RTF quote tags using the following function
				// Note: The WriteOutputStringConvertingAngleBrackets() function is also used
				// in footnote, endnote, and crossref output elsewhere in DoExportSrcOrTgtRTF.
				CountTotalCurlyBraces(VernacText,nOpeningBraces,nClosingBraces); // no curly braces added here
				if (!WriteOutputStringConvertingAngleBrackets(f,EncodingSrcOrTgt,VernacText,ptr))
				{
					pProgDlg->Destroy();
					return;
				}
				ptr += itemLen;
			}// end of else there's at least one quote mark
		}// end of else not a marker, but word or special text
	}// end of while (ptr < pEnd)

d: // exit point for if ptr == pEnd

	// remove the progress indicator window
	pProgDlg->Destroy();

	// if a set of USFM table markers (\tr, \th1, \th2...\tc1, \tc2... etc) comes at the end of
	// the document there will not be a following marker to signal the processing of the last
	// tags in the table, so we must repeat that here if the bProcessingTable flag is still set.
	if (bProcessingCharacterStyle)
	{
		MiscRTF = _T('}');
		CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // one closing curly braces added here
		if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
		{
			pProgDlg->Destroy();
			return;
		}
		
		Item sfm;
		pCharStack->Pop(sfm); // pop a character style marker
		// Check if the stack has another character style marker in it.
		// If so, we need to reset the character style to propagate that
		// character style after the current one has closed.
		if (pCharStack->IsEmpty())
		{
			bProcessingCharacterStyle = FALSE;
		}
		else
		{
			// There is another character style marker in the stack
			// so we need to propagate that character style
			Item sfm2;
			pCharStack->Pop(sfm2);
			wxString sfmMkr = wxString::FromAscii(sfm2);
			rtfIter = rtfTagsMap.find(sfmMkr);
			if (rtfIter != rtfTagsMap.end())
			{
				// we found an associated value for Marker in map
				// non-paragraph style strings need to start with an opening brace {
				// TESTING!! First we try outputting just the previous char style tags
				// without an opening curly brace. If we have to uncomment the code
				// below, we also will need to add closing braces at the Push() point
				// where the previous char style was "interrupted" by the current one
				//MiscRTF = _T("{");
				//// RTF tags use gpApp->m_systemEncoding
				//CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // one opening curly brace added here
				//if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
				//	return;
				
				pCharStack->Push(Marker.char_str()); // push it back on the stack

				// RTF tags use gpApp->m_systemEncoding
				wxString mkrTags = (wxString)rtfIter->second;
				CountTotalCurlyBraces(mkrTags,nOpeningBraces,nClosingBraces);
				if (!WriteOutputString(f,gpApp->m_systemEncoding,mkrTags))
				{
					pProgDlg->Destroy();
					return;
				}
			}
		}
	}
	//if (bProcessingTable)
	//{
	//	// we've been processing a USFM table and have come to a non-table,
	//	// non-character formatting marker which signals the end of the table.
	//	// We need to signal we've reached the end of the table
	//	bProcessingTable = FALSE;
	//	// Since the \cell markers are output after the actual cell text, we need to
	//	// output a final "\cell " tag string to close off the last cell of the last
	//	// row of the table
	//	MiscRTF = _T("\\cell }");
	//	if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
	//		return;
	//	// No need to increment nCurrentColIndex here because it gets reset below.

	//	// The remainder of the preceding row's row tags can now be output
	//	// Output the __normal paragraph style tags
	//	rtfIter = rtfTagsMap.find(_T("__normal"));
	//	if (rtfIter != rtfTagsMap.end())
	//	{
	//		// we found an associated value for Marker in map
	//		// RTF tags use gpApp->m_systemEncoding
	//		wxString tempStyle = (wxString)rtfIter->second;
	//		// remove the "\par " from the style tag string here
	//		tempStyle = tempStyle.Mid(5);
	//		if (!WriteOutputString(f,gpApp->m_systemEncoding,tempStyle))
	//			return;
	//	}
	//	// output the "{\trowd \irowN\irowbandN\tsN\trgaph40\trleft0\\ltrrow" part
	//	wxChar rowN[34];
	//	wxChar cellN[34];
	//	_itot(nCurrentRowIndex,rowN,10);// nCurrentRowIndex is incremented for new row below
	//									// so it is the index for the row we are closing off
	//	if (nCurrentRowIndex == nLastRowIndex)
	//		bLastTableRow = TRUE;
	//	MiscRTF = _T("\n{");
	//	MiscRTF += _T("\\trowd \\irow");
	//	MiscRTF += rowN;
	//	MiscRTF += _T("\\irowband");
	//	MiscRTF += rowN;
	//	MiscRTF += _T("\\ts") + TblGridSNum;
	//	if (bLastTableRow)
	//		MiscRTF += _T("\\lastrow");
	//		MiscRTF += _T("\\trgaph40");
	//	MiscRTF += _T("\\trleft0\\ltrrow");
	//	// add the \cellxN data for the first row
	//	int colCt;
	//	for (colCt = 0; colCt < bestTextExtents.GetCount(); colCt++)
	//	{
	//		_itot(bestTextExtents.GetAt(colCt),cellN,10);
	//		wxString cellNStr = cellN;
	//		if (cellNStr != _T("0"))
	//		{
	//			MiscRTF += _T("\n\\cellx");
	//			MiscRTF += cellN;
	//		}
	//
	//	}
	//	// add the \row and closing brace '}'
	//	MiscRTF += _T("\\row }");

	//	// now output the tags up to this point
	//	// RTF tags use gpApp->m_systemEncoding
	//	if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
	//		return;

	//	nCurrentRowIndex++;
	//	nCurrentColIndex = 0; // new row starts at column zero
	//	bAtFirstCellInRow = TRUE;
	//	// Note: This ends the output of the previous row of the table

	//	// Note: In the event that the last \tc... table cell marker is at the end of
	//	// the file, this block (which was entered because of encountering a non-table
	//	// marker, will not be entered and this \cell tag will not be output, therefor
	//	// the tag bLastCellTagOutput below will signal that fact so that it can be
	//	// processed elsewhere if it is still FALSE.
	//	bLastCellTagOutput = TRUE;

	//	// Ensure table related int vars are correctly set in case a subsequent table
	//	// is encountered
	//	nCurrentRowIndex = 0;
	//	nLastRowIndex = 0;
	//	nCurrentColIndex = 0;

	//	// the following is used in the longer routine that attempts to format USFM
	//	// tables as real Word tables.
	//	//nLastColIndex = 0;

	//	// Also we should ensure other table related flags are correctly set in case a
	//	// subsequent table is encountered
	//	bAtFirstTableRow = TRUE;
	//	bAtFirstCellInRow = TRUE;
	//	bLastTableRow = FALSE;


	//	// clear out the arrays
	//	bestTextExtents.RemoveAll();
	//	cellText.RemoveAll();
	//	rightAlignment.RemoveAll();

	//	// lastly reset the overall table processing flag
	//	bProcessingTable = FALSE;
	//}

	// If btAssocStr is not empty, we we need to output the last back
	// translated material here.
	if (!btAssocStr.IsEmpty())
	{
		if (!OutputAnyBTorFreeMaterial(f,gpApp->m_systemEncoding,lastBTMarker,_T("bt"),
			btAssocStr,
			LastStyle,LastParaStyle,btRefNumInt,bLastParagraphWasBoxed,
			parseError,callerType,bProcessingTable,bPlaceBackTransInRTFText,
			single_border,pDoc))
		{
			pProgDlg->Destroy();
			return;
		}
	}

	// If freeAssocStr is not empty, we we need to output the last free
	// translated material here.
	if (!freeAssocStr.IsEmpty())
	{
		if (!OutputAnyBTorFreeMaterial(f,gpApp->m_systemEncoding,lastFreeMarker,_T("free"),
			freeAssocStr,
			LastStyle,LastParaStyle,freeRefNumInt,bLastParagraphWasBoxed,
			parseError,callerType,bProcessingTable,bPlaceFreeTransInRTFText,
			double_border,pDoc))
		{
			pProgDlg->Destroy();
			return;
		}
	}


	// add final RTF \par tag and closing brace
	MiscRTF = _T("\\par }");
	CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // one closing curly brace added here
	if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
	{
		pProgDlg->Destroy();
		return;
	}

	// At this point we should have output the same number of opening curly braces and
	// closing curly braces.
	//wxASSERT(nOpeningBraces == nClosingBraces);
	// It is not supposed to happen but to prevent Word from choking on an rtf document
	// that has mismatched braces, i.e., fewer closing braces than opening braces we'll
	// do the following stop-gap measure and add the missing closing braces at the end
	// of the document.
	// Note: The opposite situation where there are more closing braces than opening ones
	// would not normally cause Word to choke, but it will truncate the file at the point
	// where the last closing brace is expected. The only example of this I've seen 
	// truncated it right at the end which didn't matter. It may have resulted from 
	// a Free translation in a source phrase that only contained orphaned punctuation 
	// after a footnote filtering operation.
	if (nClosingBraces < nOpeningBraces)
	{
		MiscRTF.Empty();
		int ct;
		for (ct = 0; ct < nOpeningBraces - nClosingBraces; ct++)
		{
			MiscRTF += _T('}');
			if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
			{
				pProgDlg->Destroy();
				return;
			}
		}
	}
	// remove the progress dialog
	pProgDlg->Destroy();

	// close the file
	f.Close();
}	// end of DoExportSrcOrTgt

int ParseFootnote(wxChar* pChar, wxChar* pBuffStart, wxChar* pEndChar,
									enum ParseError& parseError)
									//const std::map<wxString, wxString>& rtfMap)
{
	// When ParseFootnote() is called pChar should be pointing at a \f footnote
	// begining marker.
	// This function parses the entire footnote text including the footnote END marker
	// and returns the length of the whole footnote string (including markers).
	// Legitimate footnote end markers can be any of \fe, \f*, \fe* or \F. Since
	// the endnote marker \fe should not be found embedded within well-formed footnotes
	// we can assume that, if encountered in our parse, it represents an end marker.
	// We allow the parse to continue over embedded content footnote markers.
	//
	// We need to decide what to do for malformed footnotes. They may be malformed by
	// having improper markers embedded between \f...\f*, or they may be malformed by
	// failing to have a footnote closing marker \f* (or equivalent in PngOnly).
	// Both problems share the difficulty in determining how long we allow the parse
	// to continue once an improper marker is encountered (because of either an improper
	// one embedded between \f...\f* or because an ending footnote marker being absent
	// and parsing continuing into non-footnote material/markers. We also need to consider
	// the special case where a malformed footnote occurs near the end of the file and
	// parsing would hit the end of the text prematurely. We want to prevent program
	// crashes and also prevent malformed footnotes from corrupting too much of the
	// text following the footnote.
	//
	// A rather strict approach would stop parsing as soon
	// as a marker was encountered that was not a legal embedded content marker for
	// footnotes. One problem with this is that a user might innocently try something
	// like \em...\em* to emphasize a word within a footnote. If the parsing were to
	// stop immediately at the \em marker it would leave a "dangling \f* footnote end
	// marker at the end of the otherwise well-formed footnote.
	//
	// A less strict approach might be to allow most non-footnote character styles to
	// be embedded within footnotes, but not allow parses to proceed across verse or
	// chapter boundaries or any new paragraph style. Doing this would allow some
	// flexibility and would prevent footnotes without the proper end markers from
	// running on beyond the current verse into new verses, chapters or any new
	// paragraph - something the user would not have intended anyway. I've taken
	// this less strict approach.

	wxChar* ptr = pChar;
	wxChar* pEnd = pEndChar;
	int itemLen = 0;
	bool bFound = FALSE;
	wxString wholeMkr;
	wxString bareMarkerForLookup;
	wxString wholeMkrUpperCase;
	USFMAnalysis* pSfm;
	bool bIsAParagraphStyle = TRUE;

	parseError = no_error;
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	wxASSERT(pDoc != NULL);
	// we should be pointing at a marker
	wxASSERT(IsMarkerRTF(ptr,pBuffStart));
	itemLen = pDoc->ParseMarker(ptr); // point past initial \f
	ptr += itemLen;
	while (ptr < pEnd && !bFound)
	{
		do
		{
			ptr++;
			itemLen++;
		} while (ptr < pEnd && !IsMarkerRTF(ptr,pBuffStart));
		if (ptr == pEnd)
		{
			// we got to the end of the buffer before we found an end marker
			parseError = premature_buffer_end;
			break;
		}
		// To get to this point we are pointing at some kind of marker
		wholeMkr = pDoc->GetWholeMarker(ptr);
		bareMarkerForLookup = pDoc->GetBareMarkerForLookup(ptr);
		pSfm = pDoc->LookupSFM(bareMarkerForLookup);
		if (pSfm != NULL && pSfm->styleType != paragraph)
		{
			bIsAParagraphStyle = FALSE;
		}
		else
			bIsAParagraphStyle = TRUE;
		wholeMkrUpperCase = wholeMkr;
		wholeMkrUpperCase.MakeUpper();
		
		// whm added 8Nov07. It is possible that wholeMkr (and wholeMkrUpperCase) is an isolated
		// user entered non-marker backslash. In this case we want to continue parsing.
		if (wholeMkrUpperCase == _T("\\"))
		{
			continue;
		}

		// If we are pointing at a marker other than a footnote content marker, or pointing at a
		// verse marker or any paragraph marker other than \f*
		if ((wholeMkrUpperCase.Find(_T('F')) != 1
			&& bIsAParagraphStyle) || wholeMkr == _T("\\v")
			|| (gpApp->gCurrentSfmSet != PngOnly && wholeMkr == _T("\\fe")))
		{
			// inform the caller that there is no end marker on this footnote.
			parseError = no_end_marker;
			break;
		}
		if (pDoc->IsCorresEndMarker(_T("\\f"),ptr,pEnd))
		{
			// ptr currently points at the \ of the end marker
			itemLen = itemLen + wholeMkr.Length();	// get the length of the string including the end marker, but
													// not any following whitespace or punctuation
			bFound = TRUE;
		}
	}
	return itemLen;
}

int ParseEndnote(wxChar* pChar, wxChar* pBuffStart, wxChar* pEndChar,
									enum ParseError& parseError)
{
	// When ParseEndnote() is called pChar should be pointing at a \fe endnote
	// begining marker.
	// This function parses the entire endnote text including the endnote END marker
	// and returns the length of the whole endnote string (including markers).
	// Legitimate endnote end marker is \fe*.
	// We allow the parse to continue over embedded content endnote markers. These
	// are the same as the embedded content footnote markers.
	//
	// We need to decide what to do for malformed endnotes. They may be malformed by
	// having improper markers embedded between \fe...\fe*, or they may be malformed by
	// failing to have a endnote closing marker \fe*.
	// Both problems share the difficulty in determining how long we allow the parse
	// to continue once an improper marker is encountered (because of either an improper
	// one embedded between \fe...\fe* or because an ending endnote marker being absent
	// and parsing continuing into non-endnote material/markers. We also need to consider
	// the special case where a malformed endnote occurs near the end of the file and
	// parsing would hit the end of the text prematurely. We want to prevent program
	// crashes and also prevent malformed endnotes from corrupting too much of the
	// text following the endnote.
	//
	// A rather strict approach would stop parsing as soon
	// as a marker was encountered that was not a legal embedded content marker for
	// endnotes. One problem with this is that a user might innocently try something
	// like \em...\em* to emphasize a word within an endnote. If the parsing were to
	// stop immediately at the \em marker it would leave a "dangling \fe* endnote end
	// marker at the end of the otherwise well-formed endnote.
	//
	// A less strict approach might be to allow most non-endnote character styles to
	// be embedded within endnotes, but not allow parses to proceed across verse or
	// chapter boundaries or any new paragraph style. Doing this would allow some
	// flexibility and would prevent endnotes without the proper end markers from
	// running on beyond the current verse into new verses, chapters or any new
	// paragraph - something the user would not have intended anyway. I've taken
	// this less strict approach.

	wxChar* ptr = pChar;
	wxChar* pEnd = pEndChar;
	int itemLen = 0;
	bool bFound = FALSE;
	wxString wholeMkr;
	wxString bareMarkerForLookup;
	wxString wholeMkrUpperCase;
	USFMAnalysis* pSfm;
	bool bIsAParagraphStyle = TRUE;

	parseError = no_error;
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	wxASSERT(pDoc != NULL);
	// we should be pointing at a marker
	wxASSERT(IsMarkerRTF(ptr,pBuffStart));
	itemLen = pDoc->ParseMarker(ptr); // point past initial \fe
	ptr += itemLen;
	while (ptr < pEnd && !bFound)
	{
		do
		{
			ptr++;
			itemLen++;
		} while (ptr < pEnd && !IsMarkerRTF(ptr,pBuffStart));
		if (ptr == pEnd)
		{
			// we got to the end of the buffer before we found an end marker
			parseError = premature_buffer_end;
			break;
		}
		// To get to this point we are pointing at some kind of marker
		wholeMkr = pDoc->GetWholeMarker(ptr);
		bareMarkerForLookup = pDoc->GetBareMarkerForLookup(ptr);
		pSfm = pDoc->LookupSFM(bareMarkerForLookup);
		if (pSfm != NULL && pSfm->styleType != paragraph)
		{
			bIsAParagraphStyle = FALSE;
		}
		else
			bIsAParagraphStyle = TRUE;
		wholeMkrUpperCase = wholeMkr.MakeUpper();
		
		// whm added 8Nov07. It is possible that wholeMkr (and wholeMkrUpperCase) is an isolated
		// user entered non-marker backslash. In this case we want to continue parsing.
		if (wholeMkrUpperCase == _T("\\"))
		{
			continue;
		}

		// If we are pointing at a marker other than a endnote content marker, or pointing at a
		// verse marker or any paragraph marker other than \fe*
		if ((wholeMkrUpperCase.Find(_T('F')) != 1
			&& bIsAParagraphStyle) || (wholeMkr == _T("\\v")))
		{
			// we encountered a verse marker or a non-endnote paragraph marker. In
			// either case we only return the length of the "good" part of the endnote,
			// and inform the caller that there is no end marker on this endnote.
			//itemLen = itemLen + wholeMkr.Length(); // get the length of the string including the end marker
			parseError = no_end_marker;
			break;
		}
		if (pDoc->IsCorresEndMarker(_T("\\fe"),ptr,pEnd))
		{
			// ptr currently points at the \ of the end marker
			itemLen = itemLen + wholeMkr.Length();	// get the length of the string including the end marker, but
													// not any following whitespace or punctuation
			bFound = TRUE;
		}
	}
	return itemLen;
}

int ParseCrossRef(wxChar* pChar, wxChar* pBuffStart, wxChar* pEndChar,
									enum ParseError& parseError)
									//const std::map<wxString, wxString>& rtfMap)
{
	// When ParseCrossRef() is called pChar should be pointing at a \x crossref
	// begining marker.
	// This function parses the entire crossref text including the crossref END marker
	// and returns the length of the whole crossref string (including markers).
	// Legitimate crossref end marker is \x*.
	// We allow the parse to continue over embedded content crossref markers. These
	// are \xo, \xo*, \xk, \xk*, \xq, \xq*, \xt, \xt*, \xdc, and \xdc*.
	//
	// We need to decide what to do for malformed crossrefs. They may be malformed by
	// having improper markers embedded between \x...\x*, or they may be malformed by
	// failing to have a crossref closing marker \x*.
	// Both problems share the difficulty in determining how long we allow the parse
	// to continue once an improper marker is encountered (because of either an improper
	// one embedded between \x...\x* or because an ending crossref marker being absent
	// and parsing continuing into non-crossref material/markers. We also need to consider
	// the special case where a malformed crossref occurs near the end of the file and
	// parsing would hit the end of the text prematurely. We want to prevent program
	// crashes and also prevent malformed crossrefs from corrupting too much of the
	// text following the crossref.
	//
	// A rather strict approach would stop parsing as soon
	// as a marker was encountered that was not a legal embedded content marker for
	// crossrefs. One problem with this is that a user might innocently try something
	// like \em...\em* to emphasize a word within a crossref. If the parsing were to
	// stop immediately at the \em marker it would leave a "dangling \x* crossref end
	// marker at the end of the otherwise well-formed crossref.
	//
	// A less strict approach might be to allow most non-crossref character styles to
	// be embedded within crossrefs, but not allow parses to proceed across verse or
	// chapter boundaries or any new paragraph style. Doing this would allow some
	// flexibility and would prevent crossrefs without the proper end markers from
	// running on beyond the current verse into new verses, chapters or any new
	// paragraph - something the user would not have intended anyway. I've taken
	// this less strict approach.

	wxChar* ptr = pChar;
	wxChar* pEnd = pEndChar;
	int itemLen = 0;
	bool bFound = FALSE;
	wxString wholeMkr;
	wxString bareMarkerForLookup;
	wxString wholeMkrUpperCase;
	USFMAnalysis* pSfm;
	bool bIsAParagraphStyle = TRUE;

	parseError = no_error;
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	wxASSERT(pDoc != NULL);
	// we should be pointing at a marker
	wxASSERT(IsMarkerRTF(ptr,pBuffStart));
	itemLen = pDoc->ParseMarker(ptr); // point past initial \x
	ptr += itemLen;
	while (ptr < pEnd && !bFound)
	{
		do
		{
			ptr++;
			itemLen++;
		} while (ptr < pEnd && !IsMarkerRTF(ptr,pBuffStart));
		if (ptr == pEnd)
		{
			// we got to the end of the buffer before we found an end marker
			parseError = premature_buffer_end;
			break;
		}
		// To get to this point we are pointing at some kind of marker
		wholeMkr = pDoc->GetWholeMarker(ptr);
		bareMarkerForLookup = pDoc->GetBareMarkerForLookup(ptr);
		pSfm = pDoc->LookupSFM(bareMarkerForLookup);
		if (pSfm != NULL && pSfm->styleType != paragraph)
		{
			bIsAParagraphStyle = FALSE;
		}
		else
			bIsAParagraphStyle = TRUE;
		wholeMkrUpperCase = wholeMkr.MakeUpper();
		
		// whm added 8Nov07. It is possible that wholeMkr (and wholeMkrUpperCase) is an isolated
		// user entered non-marker backslash. In this case we want to continue parsing.
		if (wholeMkrUpperCase == _T("\\"))
		{
			continue;
		}

		// If we are pointing at a marker other than a crossref content marker, or pointing at a
		// verse marker or any paragraph marker other than \x*
		if ((wholeMkrUpperCase.Find(_T('X')) != 1
			&& bIsAParagraphStyle) || (wholeMkr == _T("\\v")))
		{
			// we encountered a verse marker or a non-crossref paragraph marker. In
			// either case we only return the length of the "good" part of the crossref,
			// and inform the caller that there is no end marker on this crossref.
			parseError = no_end_marker;
			break;
		}
		if (pDoc->IsCorresEndMarker(_T("\\x"),ptr,pEnd))
		{
			// ptr currently points at the \ of the end marker
			itemLen = itemLen + wholeMkr.Length();	// get the length of the string including the end marker, but
													// not any following whitespace or punctuation
			bFound = TRUE;
		}
	}
	return itemLen;
}

bool IsACharacterStyle(wxString styleMkr, MapBareMkrToRTFTags& rtfMap)
{
	if (styleMkr.IsEmpty())
		return FALSE;	// LastStyle may be empty string at beginning of file - \id should normally
						// be first marker in file so assume paragraph style when styleMkr is empty string
	// remove asterisk from end markers
	int astPos = styleMkr.Find(_T('*'));
	if (astPos != -1)
	{
		styleMkr.Replace(_T("*"),_T("")); //styleMkr.Remove(_T('*'));
	}
	// use only first two marker letters if a back translation marker
	if (styleMkr.Find(_T("bt")) == 0)
	{
		styleMkr = _T("bt"); // changing the value parameter
	}
	// wx Note: rtfIter is defined in View's global space
	rtfIter = rtfMap.find(styleMkr);	// this should not fail
	if (rtfIter != rtfMap.end())
	{
		// We found an associated value for styleMkr in map
		// We can ignore the other non-paragraph styles \ts (table style), \ds (section style)
		// and \tsrowd (another table style) as they are not used in rtfMap for in-document
		// RTF marking.
		wxString checkStr = (wxString)rtfIter->second;
		if (checkStr.Find(_T("\\cs")) != -1)
			return TRUE;
	}
	return FALSE;
}

// Footnote, Endnote and CrossReferences all involve RTF Destination text, i.e., part of the
// text gets placed at a different destination (usually the bottom of the page).
bool ProcessAndWriteDestinationText(wxFile& f, wxFontEncoding Encoding, wxString& destStr,
		bool& bIsAtEnd, enum DestinationTextType destTxtType,
		MapBareMkrToRTFTags& rtfMap,
		CAdapt_ItDoc* pDoc, enum ParseError& parseError,
		enum CallerType& callerType, wxString suppliedCaller,
		bool bHasFreeTransToAddToFootnoteBody, wxString& freeAssocStr)
{
	// ProcessAndWriteDestinationText() analyzes the destination string destStr
	// and outputs the RTF tags and text associated with it. The destStr is the
	// whole string that was parsed by ParseFootnote(), ParseEndNote(), ParseCrossRef(),
	// or ParseMarkerAndAssociatedText (in the case of \bt \free and \note material which
	// when formatted as footnotes, are handled as footnote destinations).

	// The following strings are used to customize the code in ProcessWriteDestinationText to apply to
	// footnotes, endnotes, or crossrefs, depending on the value of the destTxtType input parameter.
	wxString destWholeMarker;
	wxString destBareMarker;
	wxString destEndMarker;
	wxString destMarkerPrefixChar;
	// First set some variables according to the destination text type (whether for footnote,
	// endnote, or crossref
	switch (destTxtType)
	{
	case footnoteDest:
		{
			destWholeMarker = _T("\\f");
			destBareMarker = _T("f");
			destEndMarker = _T("\\f* "); // needs ending space - only used in parse error condition
			destMarkerPrefixChar = _T("f");
			break;
		}
	case endnoteDest:
		{
			destWholeMarker = _T("\\fe");
			destBareMarker = _T("fe");
			destEndMarker = _T("\\fe* "); // needs ending space - only used in parse error condition
			destMarkerPrefixChar = _T("f"); // not fe here just the first char of embedded prefix
			break;
		}
	case crossrefDest:
		{
			destWholeMarker = _T("\\x");
			destBareMarker = _T("x");
			destEndMarker = _T("\\x* "); // needs ending space - only used in parse error condition
			destMarkerPrefixChar = _T("x");
			break;
		}
	}
	// MFC note: Since wxString's + and =+ operators don't handle concatenations of strings
	// having new line characters, but Find and Replace do, we'll use them to
	// normalize the footnote string to spaces.
	destStr.Replace(_T("\n"),_T(" "));
	destStr.Replace(_T("\r"),_T(" "));

	// convert any double spaces (as a result of above normalization of whitespace) to single spaces
	while (destStr.Find(_T("  ")) != -1)
	{
		destStr.Remove(destStr.Find(_T("  ")),1);
	}
	if (parseError == no_end_marker || parseError == premature_buffer_end)
	{
		// In both error conditions destStr will have no end marker.
		// for footnote parsing purposes we'll suffix an end marker to destStr so
		// it will be processed in the footnote buffer but not in the ptr buffer
		destStr += destEndMarker; // destEndMarker can be "\f* ", "\fe* ", or "\x* "
	}

	// Parse the destStr outputting its parts as RTF.
	// Note: The format of RTF footnotes in general is as follows:
	//Text before footnote{footnote_caller_char_style caller or \chftn {\footnote footnote_text_para_style
	//{footnote_caller_char_style caller or \chftn}
	//{char_style text_of_footnote}...{char_style text_of_footnote}...}}
	//  "{\cs46\fs16\super \chftn  {\footnote \pard\plain \s49\qj \li0\ri0\widctlpar\rtlpar"
	//  "\nooverflow\rin0\lin0\itap0 \f2\fs16 {\cs46\fs16\super \chftn }"
	// In the RTF stream the actual footnote text would immediately follow this tag and the
	// whole thing then ends with three closing braces "}}}" when the footnote end marker (\f*
	// in the usfm set, or \fe or \F in the png set) is encountered.

	// use special pointers and variables for the footnote
	int fnLen = destStr.Length(); //Buffer.Length(); // This is a mistake in MFC version
	const wxChar* pfnBuffer = destStr.GetData();
	int fnItemLen = 0;
	wxChar* fnptr = (wxChar*)pfnBuffer;			// point to start of text
	wxChar* pfnBuffStart = fnptr;		// save start address of Buffer // this should now be correct
	wxChar* pfnEnd = pfnBuffStart + fnLen;	// bound past which we must not go
	wxASSERT(*pfnEnd == _T('\0')); // ensure there is a null at end of Buffer
	wxString fnWholeMkr;
	wxString fnBareMkr;
	wxString fnLastMkr;
	wxString fnLastBareMkr;
	wxString fnWholeMkrNoAsterisk;
	wxString MiscRTF;
	wxString VernacText;
	wxString checkStr;
	// wx Note: rtfIter is defined in the View's global space
	//MapBareMkrToRTFTags::iterator rtfIter; //std::map < wxString, wxString > :: const_iterator rtfIter;

	wxString debugCheckStr;
	debugCheckStr.Empty();
	wxString spaceless = gpApp->m_punctuation[0];
	wxString precPunct;			// place to store parsed preceding punctuation, including detached quotes
	wxString follPunct;			// place to store parsed following punctuation, including detached quotes

	fnLastMkr.Empty();
	bool bLastStyleGroupNeedsClosing = FALSE;	// used when user does not use explicit
												// embedded content end markers within the
												// footnote string
	bool bHitFirstWord = FALSE;	// used to detect caller and output initial tags and set
								// caller type. Once TRUE remains TRUE for remaining
								// footnote processing.
	bool callerIsVernacular = FALSE;
	wxString callerStr;
	int nOpeningBraces = 0; // to ensure we have the same number at the end of the parsing
	int nClosingBraces = 0; // " " "

	if (gpApp->gCurrentSfmSet != PngOnly)
	{
		// We need to determine the callerType the user wants so we'll parse the
		// string until we get to the first word or first marker.
		// If a marker comes first in the string it is technically malformed not having a
		// caller designation under USFM, but in this case we'll assign a default * as the
		// caller, then process the remaining string in a loop of if else blocks.
		// If a word comes first in the string we'll need to determine what the user intends
		// for the caller. If the word is a '+' we set the callerType to auto_number; else,
		// if the word is '-' we set the callerType to no_caller; else we'll just use the
		// word found as a literal caller.
		// When callerType is supplied_by_parameter we will use the actual string passed in
		// via suppliedCaller as the caller

		// parse the string until we get to the first word or first marker
		fnItemLen = pDoc->ParseMarker(fnptr);
		fnptr += fnItemLen; // point past the initial marker
		fnItemLen = pDoc->ParseWhiteSpace(fnptr);
		fnptr += fnItemLen; // point past white space
		// We know that the parsing routine (ParseFootnote, ParseEndnote, or ParseCrossRef) gave
		// us a string with at least "\f" or "\fe" or "\x" in it. Caution: If parse found a
		// non-footnote (or non-endnote or non-crossref) marker immediately after the
		// initial marker, the string returned (after adding a final space above) could be
		// as short as "\f " or "\fe " or "\x ", and we could run out of string to parse, so we
		// need to check to see if we are at the end.

		// Are we at the end of the text? If so, we had an empty footnote and will skip
		// it and just check for another marker beyond what ParseFootnote processed
		if (pDoc->IsEnd(fnptr) || fnptr >= pfnEnd)
		{
			bIsAtEnd = TRUE;
			return TRUE; // makes the caller goto b;
		}

		if (IsMarkerRTF(fnptr,pfnBuffStart))
		{
			// There is a marker immediately after the initial marker and the footnote/endnote/
			// crossref is technically malformed, not having a caller designation under USFM,
			// but we'll just assign a default * as the caller, the callerType being
			// word_literal_caller.
			callerType = word_literal_caller;
			callerStr = _T('*'); // make it a literal asterisk
			// We won't parse the marker and following space here, but do it below in our
			// parsing loop
		}
		else
		{
			// A word of some kind follows immediatly after the initial marker
			// We expect a '+' or '-' or a literal caller like '*' 'a' etc.
			// Use Bruce's ParseWord() but we don't worry about storing punctuation here.
			spaceless.Replace(_T(" "),_T("")); // don't have any spaces in the string of source text punctuation characters
			precPunct.Empty();
			follPunct.Empty();
// ***TODO*** temporarily disabled 11Oct10			
			fnItemLen = ParseWordRTF(fnptr, precPunct, follPunct, spaceless);
			// make the word into a wxString
			VernacText = wxString(fnptr,fnItemLen);
			fnptr += fnItemLen; // point past the word

			debugCheckStr += VernacText; // testing only

			if (callerType == supplied_by_parameter)
			{
				// the caller was supplied in supplied_by_parameter. This occurs when we want to
				// use ProcessAndWriteDestinationText on a \free, \bt... or \note which is
				// being formatted as a footnote.
				wxASSERT(!suppliedCaller.IsEmpty()); // it shouldn't be a null string here
				callerStr = suppliedCaller;
				callerIsVernacular = FALSE; // it will be something like "free 1", "note 2", "bt 6", etc.
			}
			else
			{
				// the caller is not supplied in supplied_by_parameter
				// so work it out from the parsed VernacText
				if (VernacText == _T("+"))
				{
					callerType = auto_number;
					callerStr = _T("\\chftn "); // at one point it seemed to need two spaces but doesn't any longer
				}
				else if (VernacText == _T("-"))
				{
					callerType = no_caller;
					callerStr = _T(' '); // one space here
				}
				else
				{
					// default callerType is word_literal_caller if neither '+' nor '-'
					// is specified
					callerType = word_literal_caller;
					callerStr = VernacText; // use the word user used - vernacular encoding
					// do not add a space on the word literal caller, user will do it if desired ???
					callerIsVernacular = TRUE;
				}
			}

		}
	}
	else
	{
		// in the Png sfm set footnotes did not have any caller
		// prefixed to the string, except for when the caller is supplied
		// by parameter
		// parse the string until we get to the first word or first marker
		fnItemLen = pDoc->ParseMarker(fnptr);
		fnptr += fnItemLen; // point past the initial marker
		fnItemLen = pDoc->ParseWhiteSpace(fnptr);
		fnptr += fnItemLen; // point past white space
		// We know that the parsing routine (ParseFootnote, ParseEndnote, or ParseCrossRef) gave
		// us a string with at least "\f" or "\fe" or "\x" in it. Caution: If parse found a
		// non-footnote (or non-endnote or non-crossref) marker immediately after the
		// initial marker, the string returned (after adding a final space above) could be
		// as short as "\f " or "\fe " or "\x ", and we could run out of string to parse, so we
		// need to check to see if we are at the end.

		if (callerType == supplied_by_parameter)
		{
			// the caller was supplied in supplied_by_parameter. This occurs when we want to
			// use ProcessAndWriteDestinationText on a \free, \bt... or \note which is
			// being formatted as a footnote.

			// For supplied_by_parameter, the caller (note, bt..., free) was prefixed
			// immediatly after the initial marker. It is not needed for PngOnly output of
			// free, bt..., or note.
			spaceless.Replace(_T(" "),_T("")); // don't have any spaces in the string of source text punctuation characters
			precPunct.Empty();
			follPunct.Empty();
// ***TODO*** temporarily disabled 11Oct10			
			fnItemLen = ParseWordRTF(fnptr, precPunct, follPunct, spaceless);
			fnptr += fnItemLen; // point past the word
			// we don't make the word into wxString here, just ignore it

			// Are we at the end of the text? If so, we had an empty footnote and will skip
			// it and just check for another marker beyond what ParseFootnote processed
			if (pDoc->IsEnd(fnptr) || fnptr >= pfnEnd)
			{
				bIsAtEnd = TRUE;
				return TRUE; // makes the caller goto b;
			}
			wxASSERT(!suppliedCaller.IsEmpty()); // it shouldn't be a null string here
			callerStr = suppliedCaller;
			callerIsVernacular = FALSE; // it will be something like "free 1", "note 2", "bt 6", etc.
		}
		else
		{
			// we are using the PngOnly set which had no caller or caller control + or -
			// prefixed to the footnote text
			callerType = word_literal_caller;
			callerStr = _T("*");
			callerIsVernacular = TRUE;
		}
	}

	// before proceeding to analyze the remainder of the string we'll build and
	// output the caller and prefix tags that we can form now that we know the
	// shape of the caller. If will look something like this:
	// {\cs46\fs16\super \chftn  {\footnote \pard\plain \s49\qj \li0\ri0\widctlpar\rtlpar"
	//  "\nooverflow\rin0\lin0\itap0 \f2\fs16 {\cs46\fs16\super \chftn }"
	// Note: This prefix has 3 opening braces and only 1 closing brace. The footnote text
	// that follows will have any number of character styles each of the form {\cs...}
	// 1 opening and 1 closing brace. Therefore, after all our footnote text is processed
	// we will need to suffix 2 ending braces }} to close the whole footnote off. If we end
	// up with more opening braces than closing braces it will throw off the formatting
	// of following text with varying results. If we end up with more closing braces than
	// opening braces it will interpret an extra closing brace as the actual closing
	// brace of the RTF file and truncate the file at that point; so we have to get it
	// exactly right.

	wxString footnoteCallerStyleTagStr;
	wxString fnStyleTagStr;
	footnoteCallerStyleTagStr.Empty();
	rtfIter = rtfMap.find(_T("_footnote_caller"));
	if (rtfIter != rtfMap.end())
	{
		// we found an associated value for the marker key in map
		footnoteCallerStyleTagStr = (wxString)rtfIter->second;
	}
	// add the opening group brace and the caller style tag string
	MiscRTF = _T('{') + footnoteCallerStyleTagStr;
	nOpeningBraces++;
	// determine the caller itself according to its type
	// add the caller itself

	// If we want to prefix an RTF non-breaking space to the caller; it would get encoded
	// along with the MiscRTF stuff here. Note:
	//MiscRTF = _T("\\~") + MiscRTF;

	// if callerIsVernacular we'll output the system encoded tags up to here
	// before proceeding
	if (callerIsVernacular)
	{
		if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
			return FALSE;
		if (!WriteOutputString(f,Encoding,callerStr))
			return FALSE;

		MiscRTF.Empty();
	}
	else
	{
		MiscRTF += callerStr;
	}

	// add the footnote destination group prefix
	MiscRTF += _T("{\\footnote \\pard\\plain ");
	nOpeningBraces++;
	// lookup the style tag string for the bare marker
	fnStyleTagStr.Empty();
	rtfIter = rtfMap.find(destBareMarker); // destBareMarker can be "f", "fe" or "x"
	if (rtfIter != rtfMap.end())
	{
		// we found an associated value for the marker key in map
		fnStyleTagStr = (wxString)rtfIter->second;
	}
	MiscRTF += fnStyleTagStr;
	MiscRTF += _T('{');
	nOpeningBraces++;
	MiscRTF += footnoteCallerStyleTagStr;
	// if callerIsVernacular we'll output the system encoded tags up to here
	// before proceeding
	if (callerIsVernacular)
	{
		if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
			return FALSE;
		if (!WriteOutputString(f,Encoding,callerStr))
			return FALSE;

		MiscRTF.Empty();
	}
	else
	{
		MiscRTF += callerStr;
	}
	MiscRTF += _T('}');

	nClosingBraces++;
	debugCheckStr += MiscRTF; // testing only

	// now output the tags that preceed the actual text
	if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
		return FALSE;

	// are we at the end of the text? If so, we had an empty string and will skip
	// it and just check for another marker
	if (pDoc->IsEnd(fnptr) || fnptr >= pfnEnd)
	{
		bIsAtEnd = TRUE;
		return TRUE; // Note: this causes the caller to branch to b:
	}

	// RTF footnote tags have been fully output, now parse and output any whitespace
	fnItemLen = pDoc->ParseWhiteSpace(fnptr);
	VernacText = wxString(fnptr,fnItemLen);
	fnptr += fnItemLen; // point past the whitespace

	debugCheckStr += VernacText; // testing only

	if (!WriteOutputString(f,Encoding,VernacText))
		return FALSE;

	// are we at the end of the text? If so, we had an empty string and will skip
	// it and just check for another marker
	if (pDoc->IsEnd(fnptr) || fnptr >= pfnEnd)
	{
		bIsAtEnd = TRUE;
		return TRUE; // Note: this causes the caller to branch to b:
	}

	// we've handled the footnote up through the caller, and the remainder is
	// just parsing of destination string text and any character style groups
	// plus dealing with any \free material that also may be within the footnote.

	// first get some strings from the map for possible repeated use below

	// the footnote/endnote/crossref style tag string is already in fnStyleTagStr
	// but starts with \sN, so we'll prefix the \par \pard\plain to it.
	wxString footnoteParaTags;
	footnoteParaTags = _T("\\pard\\plain ") + fnStyleTagStr;	// \plain resets font attributes

	// get the _double_boxed_para style tag string
	wxString doubleBoxedParaTags;
	rtfIter = rtfMap.find(_T("_double_boxed_para"));
	if (rtfIter != rtfMap.end())
	{
		// we found an associated value for the marker key in map
		doubleBoxedParaTags = (wxString)rtfIter->second;
		// remove any initial \par leaving just \pard\plain on the style tags
		int posPar = doubleBoxedParaTags.Find(_T("\\par"));
		if (posPar != -1)
		{
			doubleBoxedParaTags.Remove(posPar,4);
		}
	}
	// get the \free style tag string
	wxString freeTransTagsWithSmallerFontSize;
	rtfIter = rtfMap.find(_T("free"));
	if (rtfIter != rtfMap.end())
	{
		// we found an associated value for the marker key in map
		freeTransTagsWithSmallerFontSize = (wxString)rtfIter->second; // free trans char style tags
	}
	// reduce font size for display of the free trans within footnote
	// The free style in MiscRTF should end with something like
	// "\fs24\cf18 " so we need to change the \fs24 to \fs18
	int fsPos = freeTransTagsWithSmallerFontSize.Find(_T("\\fs"));
	int fsEndPos = fsPos;
	fsEndPos += 3; // point at the first number following \fs
	while (fsEndPos < (int)freeTransTagsWithSmallerFontSize.Length() && wxIsdigit(freeTransTagsWithSmallerFontSize[fsEndPos]))
	{
		freeTransTagsWithSmallerFontSize.Remove(fsEndPos,1);
	}
	freeTransTagsWithSmallerFontSize = InsertInString(freeTransTagsWithSmallerFontSize, fsPos + 3,_T("16"));

	// parse the remainder of the destination text
	// 18Jan06 whm revised to handle embedded \free markers within the footnote,
	// outputting the free material as boxed paragraphs under each section of the
	// footnote to which \free translation applies
fnb: while (fnptr < pfnEnd)
	{
		if (IsRTFControlWord(fnptr,pfnEnd))
		{
			// whm 8Nov07 comment: The IsRTFControlWord block is placed here to bleed off the cases 
			// where a backslash is escaping a {, }, or \ character in the character stream. 
			fnItemLen = ParseRTFControlWord(fnptr,pfnEnd);
			// if fnItemLen is zero at this point, we had an empty VernacText for some 
			// other unknown reason. In this case it is best to simply advance fnptr and 
			// goto fnb to check for another marker or pfnEnd.
			if (fnItemLen == 0)
			{
				fnptr++;
				goto fnb; // check for another marker
			}
			VernacText = wxString(fnptr,fnItemLen);
			//CountTotalCurlyBraces(VernacText,nOpeningBraces,nClosingBraces);
			if (!WriteOutputString(f,gpApp->m_systemEncoding,VernacText))
				return FALSE;
			fnptr += fnItemLen;
		}
		else if (IsMarkerRTF(fnptr,pfnBuffStart))
		{
			fnWholeMkr = pDoc->GetWholeMarker(fnptr);
			fnBareMkr = pDoc->GetBareMarkerForLookup(fnptr);
			fnLastBareMkr = fnLastMkr;
			fnLastBareMkr = fnLastMkr.Mid(1); // remove initial backkslash
			if (fnLastBareMkr.Find(_T('*')) != -1)
			{
				// remove any asterisk on fnLastBareMkr
				fnLastBareMkr.Replace(_T("*"),_T("")); //fnLastBareMkr.Remove(_T('*'));
			}
			// Note: ParseFootnote only allows embedded markers with intitial \f...
			// which can include \fr, \fr*, \fk, \fk*, \fq, \fq*, \fqa, \fqa*,
			// \fl, \fl*, \fp, \fp*, \fv, \fv*, \ft, \ft*,
			// \fdc...\fdc* and/or \fm...\fm*. The same for ParseEndNote.
			// ParseCrossRef only allows embedded markers with initial \x...

			// Deal with the embedded marker(s)

			// Is it an end marker for the overall destination string?
			if (pDoc->IsCorresEndMarker(destWholeMarker,fnptr,pfnEnd))
			{

				if (bHasFreeTransToAddToFootnoteBody)
				{
					// whm added 16Jan05
					// the free trans will be suffixed to the existing footnote
					// as a boxed paragraph below. We need to close off the the
					// existing footnote text with an RTF \par tag
					MiscRTF = _T("\\par");

					debugCheckStr += MiscRTF; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						return FALSE;
				}

				if (bLastStyleGroupNeedsClosing && bHitFirstWord)
				{
					// we're at the end marker and the last style group needs
					// closing so output the closing brace
					MiscRTF = _T('}');
					nClosingBraces++;

					// we've closed the style group so
					bLastStyleGroupNeedsClosing = FALSE;

					debugCheckStr += MiscRTF; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						return FALSE;
				}

				if (bHasFreeTransToAddToFootnoteBody)
				{
					// whm added 16Jan05
					// Suffix the (last) free trans to the existing footnote as a boxed paragraph.
					// The terminating \par was added to the footnote text above, so when the
					// doubleBoxedParaTags was retrieved above, we removed the initial \par tag
					// for output here.

					debugCheckStr += doubleBoxedParaTags; // testing only

					// Add the double boxed paragraph style tags
					if (!WriteOutputString(f,gpApp->m_systemEncoding,doubleBoxedParaTags))
						return FALSE;
					// build MiscRTF for free trans
					MiscRTF = _T('{'); // opening brace of free trans text

					debugCheckStr += MiscRTF; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						return FALSE;
					// output the style with modified font size

					debugCheckStr += freeTransTagsWithSmallerFontSize; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,freeTransTagsWithSmallerFontSize))
						return FALSE;

					// Since we are at the end of the footnote string we can output
					// any free trans string still remaining in freeAssocStr.
					// first remove the "free " and word count |@nn@| stuff from the beginning
					// of freeAssocStr
					int wcPos = freeAssocStr.Find(_T("@|"));
					if (wcPos != -1)
					{
						freeAssocStr = freeAssocStr.Mid(wcPos + 3);
					}
			#ifndef _USE_OLD_CALLS
					freeAssocStr.Trim();
			#else
					freeAssocStr = gpApp->Trim(freeAssocStr);
			#endif
					// we use system encoding for the free trans string

					debugCheckStr += freeAssocStr; // testing only

					// whm added 8Nov07. In Unicode version free trans chars should be in \uN\'fe format
					freeAssocStr = GetANSIorUnicodeRTFCharsFromString(freeAssocStr);

					if (!WriteOutputString(f,gpApp->m_systemEncoding,freeAssocStr))
						return FALSE;

					MiscRTF = _T("}"); // closing brace of free trans text

					debugCheckStr += MiscRTF; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						return FALSE;
					// after the free trans text we need to output the footnote dest
					// para tags (with only \par and \plain prefixed)

					//debugCheckStr += footnoteParaTags; // testing only

					//if (!WriteOutputString(f,gpApp->m_systemEncoding,footnoteParaTags))
					//	return FALSE;
					// freeAssocStr is a reference param, and we have now placed the free trans
					// material so empty the freeAssocStr
					freeAssocStr.Empty();
				}

				// output the closing braces for the destination string and break out
				int bct;
				MiscRTF.Empty();
				if (nOpeningBraces > nClosingBraces)
				{
					int nDiffBraceCount = nOpeningBraces - nClosingBraces;
					for (bct = 0; bct < nDiffBraceCount; bct++)
					{
						MiscRTF += _T('}');
						nClosingBraces++;
						debugCheckStr += MiscRTF; // testing only
					}
					// if my programming does what I've intended there should be
					// two closing braces added
					wxASSERT(MiscRTF.Length() == 2);
					wxASSERT(nClosingBraces == nOpeningBraces);

					if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						return FALSE;
				}

				//MiscRTF = _T(' '); // need a space after closing brackets

				//debugCheckStr += MiscRTF; // testing only

				//if (!WriteOutputString(f,Encoding,MiscRTF))
				//	return FALSE;
				// no other output needed here

				break; // we're finished with the footnote string
			}
			else if (fnWholeMkr == _T("\\free"))
			{
				// We've encountered a \free marker within the footnote string, so
				// we need to output any pending freeAssocStr, then parse the current \free
				// text and place in freeAssocStr for the next opportunity to output, or
				// at the end of the footnote text (handled above).

				// first, check to see if last group needs closing and since we are starting
				// a new paragraph, close off the last footnote paragraph with a \par
				if (bHasFreeTransToAddToFootnoteBody)
				{
					// whm added 16Jan05
					// the free trans will be suffixed to the existing footnote
					// as a boxed paragraph below. We need to close off the the
					// existing footnote text with an RTF \par tag
					MiscRTF = _T("\\par");

					debugCheckStr += MiscRTF; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						return FALSE;
				}
				if (bLastStyleGroupNeedsClosing && fnWholeMkr != fnLastMkr)
				{
					MiscRTF = _T('}');
					nClosingBraces++;

					debugCheckStr += MiscRTF; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						return FALSE;
					bLastStyleGroupNeedsClosing = FALSE;
				}

				if (bHasFreeTransToAddToFootnoteBody)
				{
					// output the current freeAssocStr as boxed text

					debugCheckStr += doubleBoxedParaTags; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,doubleBoxedParaTags))
						return FALSE;
					// build MiscRTF for free trans
					MiscRTF = _T('{'); // opening brace of free trans text

					debugCheckStr += MiscRTF; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						return FALSE;
					// output the style with modified font size

					debugCheckStr += freeTransTagsWithSmallerFontSize; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,freeTransTagsWithSmallerFontSize))
						return FALSE;

					// Since we are at the end of the footnote string we can output
					// any free trans string still remaining in freeAssocStr.
					// first remove the "free " and word count |@nn@| stuff from the beginning
					// of freeAssocStr
					int wcPos = freeAssocStr.Find(_T("@|"));
					if (wcPos != -1)
					{
						freeAssocStr = freeAssocStr.Mid(wcPos + 3);
					}
			#ifndef _USE_OLD_CALLS
					freeAssocStr.Trim();
			#else
					freeAssocStr = gpApp->Trim(freeAssocStr);
			#endif
					// we use system encoding for the free trans string

					debugCheckStr += freeAssocStr; // testing only

					// whm added 8Nov07. In Unicode version free trans chars should be in \uN\'fe format
					freeAssocStr = GetANSIorUnicodeRTFCharsFromString(freeAssocStr);

					if (!WriteOutputString(f,gpApp->m_systemEncoding,freeAssocStr))
						return FALSE;

					MiscRTF = _T("\\par}"); // closing brace of free trans text, include \par here

					debugCheckStr += MiscRTF; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						return FALSE;

					// output the footnoteParaStyle tags here for the next bit of footnote text

					debugCheckStr += footnoteParaTags; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,footnoteParaTags))
						return FALSE;
				}

				MiscRTF = _T('{'); // opening brace for next bit of footnote text

				debugCheckStr += MiscRTF; // testing only

				if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
					return FALSE;
				nOpeningBraces++;
				bLastStyleGroupNeedsClosing = TRUE;

				freeAssocStr.Empty(); // it will be filled again below

				// parse the current \free text for output at the next halting point or at
				// end of footnote text
				wxString wholeMarker = _T("\\free");
				wxString freeMarker = _T("free");
				fnItemLen = ParseMarkerAndAnyAssociatedText(fnptr,pfnBuffStart,pfnEnd,freeMarker,wholeMarker,TRUE,FALSE);
				// TRUE above means we expect RTF text to parse
				// FALSE above means don't include char format markers (shouldn't be any char format markers
				// within \free trans material
				wxString freeStr;
				freeStr = wxString(fnptr,fnItemLen);
				// freeStr still starts with \free so just remove the backslash leaving the bare marker
				// to function as caller when the string is used as a footnote, and add back the
				// wholeMarker and space prefixed
				freeStr = freeStr.Mid(1);
				// remove the \free* end marker
				int freeEndMkrPos = freeStr.Find(_T("\\free*"));
				freeStr = freeStr.Left(freeEndMkrPos);
				freeAssocStr = freeStr;
				bHasFreeTransToAddToFootnoteBody = TRUE;

				fnptr += fnItemLen; // point past marker
				fnItemLen = pDoc->ParseWhiteSpace(fnptr);
				fnptr += fnItemLen; // point past white space
			}
			// Is it an embedded content end marker?
			else if (pDoc->IsEndMarker(fnptr,pfnEnd))
			{
				// We already processed any overall end marker in the else if block above,
				// so we now have encountered an end marker of the embedded content type
				// i.e., \fr*, \fk*, \fq*, \fqa*, \fl*, \fp*, \fv*, \ft*,
				// \fdc*, or \fm* for footnotes, for example. These are all optional markers

				if (fnBareMkr == fnLastBareMkr)
				{
					// we're at the end marker and the last style group needs
					// closing so output the closing brace
					MiscRTF = _T('}');
					nClosingBraces++;

					debugCheckStr += MiscRTF; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						return FALSE;
					bLastStyleGroupNeedsClosing = FALSE; // we've just done it
				}


				fnItemLen = pDoc->ParseMarker(fnptr);
				fnptr += fnItemLen; // point past the marker
				fnItemLen = pDoc->ParseWhiteSpace(fnptr);
				fnptr += fnItemLen; // point past white space
				fnLastMkr = fnWholeMkr;
				goto fnb; // check for another marker

			}
			// Is it an embedded content begin marker?
			// whm 8Jun12 moved parentheses position below to correct position
			else if (fnWholeMkr.Find(destMarkerPrefixChar) == 1) //else if (fnWholeMkr.Find(destMarkerPrefixChar == 1)) //destMarkerPrefixChar can be 'f' or 'x'
			{
				// it's an embedded content (begin) marker so start a group and output
				// its style tags
				// But first, if this marker is a different embedded content marker than
				// the previous marker, and the last one needs closing, we do that first
				if (bLastStyleGroupNeedsClosing && fnWholeMkr != fnLastMkr)
				{
					MiscRTF = _T('}');
					nClosingBraces++;

					debugCheckStr += MiscRTF; // testing only

					if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
						return FALSE;
				}

				// add the opening brace to start the group
				MiscRTF = _T("{");
				nOpeningBraces++;

				debugCheckStr += MiscRTF; // testing only

				if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
					return FALSE;
				rtfIter = rtfMap.find(fnBareMkr);
				if (rtfIter != rtfMap.end())
				{
					// we found an associated value for the marker key in map
					checkStr = (wxString)rtfIter->second;

					debugCheckStr += checkStr; // testing only

					// RTF tags use gpApp->m_systemEncoding
					wxString tmpStr = rtfIter->second;
					if (!WriteOutputString(f,gpApp->m_systemEncoding,tmpStr))
						return FALSE;
				}
				bLastStyleGroupNeedsClosing = TRUE;
				fnItemLen = pDoc->ParseMarker(fnptr);
				fnptr += fnItemLen; // point past marker
				fnItemLen = pDoc->ParseWhiteSpace(fnptr);
				fnptr += fnItemLen; // point past white space
				fnLastMkr = fnWholeMkr;
				goto fnb; // check for another marker
			}
			// It is some other kind of marker
			else
			{
				// it is some unknown marker crept into the destination string which
				// should not happen, but to be safe, we'll output its style
				// tag strings if it is a character style, otherwise mark it
				// as _unknown_char_style.
				rtfIter = rtfMap.find(fnBareMkr);
				if (rtfIter != rtfMap.end())
				{
					// we found an associated value for the marker key in map
					checkStr = (wxString)rtfIter->second;

					debugCheckStr += checkStr; // testing only

					// RTF tags use gpApp->m_systemEncoding
					wxString tmpStr = rtfIter->second;
				if (!WriteOutputString(f,gpApp->m_systemEncoding,tmpStr))
						return FALSE;
				}
				else
				{
					// marker tag string not found and we expect a character style
					// so output the following text in _unknown_char_style
					rtfIter = rtfMap.find(fnBareMkr);
					if (rtfIter != rtfMap.end())
					{
						// we found an associated value for the marker key in map
						checkStr = (wxString)rtfIter->second;

						debugCheckStr += checkStr; // testing only

						// RTF tags use gpApp->m_systemEncoding
						wxString tmpStr = rtfIter->second;
						if (!WriteOutputString(f,gpApp->m_systemEncoding,tmpStr))
							return FALSE;
					}
				}
				bLastStyleGroupNeedsClosing = TRUE;
				fnItemLen = pDoc->ParseMarker(fnptr);
				fnptr += fnItemLen; // point past marker
				fnItemLen = pDoc->ParseWhiteSpace(fnptr);
				fnptr += fnItemLen; // point past white space
				fnLastMkr = fnWholeMkr;
				goto fnb; // check for another marker
			} // end of some other kind of marker
		} // end of if IsMarkerRTF
		else
		{
			// must be a word within destination text
			precPunct.Empty();
			follPunct.Empty();
// ***TODO*** temporarily disabled 11Oct10			
			fnItemLen = ParseWordRTF(fnptr, precPunct, follPunct, spaceless);
			// make the word into a wxString
			VernacText = wxString(fnptr,fnItemLen);

			if (VernacText.IsEmpty())
			{
				// something is wrong
				wxASSERT(FALSE);
				return FALSE;
			}

			if (VernacText.Find(_T("|@")) != -1)
			{
				wxASSERT(VernacText.Find(_T("@|")) != -1); // closing marks should also be present
				// this word is the free translation word count of the form |@N@| where N is an
				// number character string. We have no use for this "word" in RTF output so we
				// will simply omit it from output
				fnptr += fnItemLen;
				// no output of this word here
				fnItemLen = pDoc->ParseWhiteSpace(fnptr);
				// also parse the whitespace following
				fnptr += fnItemLen;
				// no output of this whitespace here
			}
			else if (VernacText.Find(_T('<')) == -1
				&& VernacText.Find(_T('>')) == -1)
			{
				// there are no angle quote marks so output in vernacular encoding
				if (!WriteOutputString(f,Encoding,VernacText))
					return FALSE;
				fnptr += fnItemLen;
			}
			else
			{
				// there is at least one angle quote mark in VernacText so we'll convert
				// them to the appropriate RTF quote tags using the following function:
				if (!WriteOutputStringConvertingAngleBrackets(f,Encoding,VernacText,fnptr))
					return FALSE;
				fnptr += fnItemLen;
			}

			bHitFirstWord = TRUE;

			// RTF string has been fully output, now parse and output any whitespace
			fnItemLen = pDoc->ParseWhiteSpace(fnptr);
			VernacText = wxString(fnptr,fnItemLen);
			fnptr += fnItemLen; // point past the whitespace
			debugCheckStr += VernacText; // testing only
			if (!WriteOutputString(f,Encoding,VernacText))
				return FALSE;
			goto fnb; // check for another marker
		}

	}
	wxASSERT(nOpeningBraces == nClosingBraces);
	return TRUE;
}

// BEW 10Apr10, no changes for doc version 5
wxString GetStyleNumberStrFromRTFTagStr(wxString tagStr, int& startPos, int& endPos)
{
	// There are three kinds of RTF styles that signal the style type in the style tag itself:
	// 1. paragraph style \sN
	// 2. character style \*\csN (as a style definition in the RTF header's stylesheet), or \csN for
	//    in-document character style tag
	// 3. table style \*\tsN (as a style definition), or \tsN (for in-document/unused)

	int sDefStartPos, sIndocStartPos;
	// First, are we dealing with a table style (Normal Table)?
	sDefStartPos = tagStr.Find(_T("{\\*\\ts"));
	sIndocStartPos = tagStr.Find(_T("\\ts"));
	if (sDefStartPos != -1)
		startPos = sDefStartPos;
	else if (sIndocStartPos != -1)
		startPos = sIndocStartPos;
	if (sDefStartPos != -1 || sIndocStartPos != -1)
	{
		// We have a table style
		if (sDefStartPos != -1)
			startPos = startPos + 5; // point at the s in {\*\ts
		else if (sIndocStartPos != -1)
			startPos = startPos + 2; // point at the s in \ts
	}
	else
	{
		// We have either a character or paragraph style
		sDefStartPos = tagStr.Find(_T("{\\*\\cs"));
		sIndocStartPos = tagStr.Find(_T("\\cs")); // no leading { in in-doc string
		if (sDefStartPos != -1)
			startPos = sDefStartPos;
		else if (sIndocStartPos != -1)
			startPos = sIndocStartPos;
		if (sDefStartPos == -1 && sIndocStartPos == -1)
		{
			// we should have a paragraph style with \s
			startPos = tagStr.Find(_T("\\s"));
			wxASSERT(startPos != -1);
			startPos = startPos + 1; // point at the s in {\sN
		}
		else
		{
			// we have a character style with {\*\csN or {\csN
			// startPos was determined above
			if (sDefStartPos != -1)
				startPos = startPos + 5; // point at the s in {\*\cs
			else if (sIndocStartPos != -1)
				startPos = startPos + 2; // point at the s in \cs

		}
	}
	startPos++; // point startPos to the first position of N
	endPos = startPos; // point endPos also to the first position of N
	// scan endPos toward the right end of tempStr until we come to the next \ tag or space
	while (endPos < (int)tagStr.Length() && tagStr[endPos] != _T('\\') && tagStr[endPos] != _T(' '))
	{
		endPos++;
	}
	// endPos should now point to the first \ past the N of \sN or \*\csN or \csN
	// while startPos points to the first char of the existing number N of \sN or \*\csN
	wxString sNumStr;
	sNumStr = tagStr.Mid(startPos,endPos - startPos);
	return sNumStr;
}

void AddAnyStylenameColon(wxString& tempStr, USFMAnalysis* pSfm)
{
	if (pSfm == NULL)
	{
		wxASSERT(FALSE); // Something's wrong with xml attribute !!!
		return;
	}
	// ensure there is a space on tempStr before adding styleName
	if (tempStr.Length() > 0 && tempStr[tempStr.Length() -1] != _T(' '))
	{
		tempStr += _T(' ');
	}
	// add the actual style name which is copied from pSfm's navigationText attribute
	tempStr += pSfm->styleName;
	// add the semi-colon that must end the RTF style definition string
	tempStr += _T(';');
}

// BEW 10Apr10  no changes for doc version 5
bool MarkerIsToBeFilteredFromOutput(wxString bareMarkerForLookup)
{
	bool bFound = FALSE;
	int count;
	for (count = 0; count < (int)m_exportBareMarkers.GetCount(); count++)
	{
		wxString testStr = m_exportBareMarkers.Item(count);
		if (bareMarkerForLookup == m_exportBareMarkers.Item(count))
		{
			bFound = TRUE;
			break;
		}
	}
	if (bFound)
	{
		//bool testFlag; // set but unused
		//if (m_exportFilterFlags.Item(count) == 0)
		//	testFlag = FALSE;
		//else
		//	testFlag = TRUE;
		if (m_exportFilterFlags.GetCount() > 0 && m_exportFilterFlags.Item(count) == TRUE)
		{
			return TRUE;
		}
	}
	return FALSE;
}

wxString GetANSIorUnicodeRTFCharsFromString(wxString inStr)
{
	// This function takes an input string and, for Unicode version returns the
	// string as Unicode characters in RTF representation, i.e., \uN\'f3 where
	// N is the decimal code point representation and \'f3 is the control word
	// for a question mark '?' (which old RTF readers would use if they don't
	// handle Unicode). If the app version is ANSI, this function simply returns
	// the inStr input string unchanged.
	// GetANSIorUnicodeRTFCharsFromString assumes there are no other embedded RTF
	// control words other than possibly the three escaped char sequences \\, \{ 
	// and \}. If \\, \{, or \} are in the input string, 
	// GetANSIorUnicodeRTFCharsFromString removes the initial \ escaping character
	// so that "\\" becomes "\u92\'f3", "\{" becomes "\u123\'f3", and "\}" becomes
	// "\u125\'f3".
	//
#ifndef _UNICODE // ANSI
	// for ANSI we do nothing, just return the inStr
	return inStr;
#else	// _UNICODE
	if (inStr.IsEmpty())
		return inStr;
	wxChar Ch;
	wxString RTFUnicode = _T("");
	int nOSLength = inStr.Length();	// get number of chars in OutStr
	int nCharNum = 0;
	while (nCharNum < nOSLength)
	{
		Ch = inStr[nCharNum];
		if (Ch == _T('\\') && (nCharNum+1 < nOSLength))
		{
			if (inStr[nCharNum+1] == _T('\\') 
				|| inStr[nCharNum+1] == _T('{') 
				|| inStr[nCharNum+1] == _T('}'))
			{
				// the current char is a backslash and the next char is a backslash,
				// a {, or a } which is an escaped RTF sequence. We'll skip writing
				// out the initial backslash as \uN and just write out the actual
				// char as \uN which won't confuse RTF readers because in \uN form
				// it will be interpreted as a text char and not as signalling a
				// control word.
				nCharNum++;
				continue;
			}
		}
		RTFUnicode << _T("\\u");
		RTFUnicode << (int)Ch;
		RTFUnicode << _T("\\\'3f");
		nCharNum++;
	}
	return RTFUnicode;
#endif	// end of if _UNICODE
}

// whm added 21Jun03
bool WriteOutputString(wxFile& f, wxFontEncoding Encoding, const wxString& OutStr)
{
	wxLogNull logNo; // avoid spurious messages from the system

	if (OutStr.Length() != 0)
	{
		// output the final form of the string
	#ifndef _UNICODE // ANSI
			Encoding = Encoding; // eliminate unused warning in ANSI build
			if (!f.Write(OutStr))
				return FALSE;
			// Note: MFC's WriteString does not append a CRLF sequence to the string and
			// neither does wxFile's Write method.

	#else	// _UNICODE
			// Here we construct the RTF rendering of Unicode characters according to the encoding type.
			// If the Encoding is not m_systemEndoding we need to convert to the RTF Unicode rendering
			// using \uN\'3f where N is the decimal representation of the unicode value, and \'3f (hex 3F is '?')
			// The \'3f character should be present for use by old RTF readers that don't know about Unicode.
			// The old readers will skip the \uN (as an unknown control word) and just read the \'3f (question
			// mark).
			if(Encoding != gpApp->m_systemEncoding)
			{
				// Associated font tags. Testing showed that setting up a system of
				// associated fonts outputting their RTF tags made no differenece
				// in Word or WordPad's rendering of unicode data.
				//CString test1 = _T("\\loch\\af5\\hich\\af5\\dbch\\af1");
				//CString test2 = _T("\\loch\\af5\\dbch\\af1\\hich\\af5");
				//f.WriteString(test1);
				//f.WriteString(test2);

				wxString RTFUnicode = _T("");
				
				// whm 8Nov07 moved original code block below to a dedicated function
				// which can also be reused elsewhere
				RTFUnicode = GetANSIorUnicodeRTFCharsFromString(OutStr);
				/*
				wxChar Ch;
				//wxChar rbuf[34];
				int nOSLength = OutStr.Length();	// get number of chars in OutStr
				int nCharNum = 0;
				while (nCharNum < nOSLength)
				{
					Ch = OutStr[nCharNum];
					if (Ch == _T('\\') && (nCharNum+1 < nOSLength))
					{
						if (OutStr[nCharNum+1] == _T('\\') 
							|| OutStr[nCharNum+1] == _T('{') 
							|| OutStr[nCharNum+1] == _T('}'))
						{
							// the current char is a backslash and the next char is a backslash,
							// a {, or a } which is an escaped RTF sequence. We'll skip writing
							// out the initial backslash as \uN and just write out the actual
							// char as \uN which won't confuse RTF readers because in \uN form
							// it will be interpreted as a text char and not as signalling a
							// control word.
							nCharNum++;
							continue;
						}
					}
					RTFUnicode << _T("\\u");  //RTFUnicode += _T("\\u");
					RTFUnicode << (int)Ch; //RTFUnicode += wxSnprintf(rbuf, 34, "%d", Ch); //_itot(Ch,rbuf,10);
					RTFUnicode << _T("\\\'3f"); //RTFUnicode += _T("\\\'3f");
					nCharNum++;
				}
				*/
				if (!f.Write(RTFUnicode))
					return FALSE;
				if (!f.Write(gpApp->m_eolStr)) // wx version adds eolStr to output
					return FALSE;
			}
			else
			{
				// Note: MS Word also embeds the following tags when outputting the associated font stuff
				//if (OutStr == _T("\\cell "))
				//	OutStr = _T("\\loch\\af1\\hich\\af1\\dbch\\af1\\cell \\hich\\af1\\dbch\\af1\\loch\\af1");
				if (!f.Write(OutStr)) // wx version adds eolStr to output
					return FALSE;
				if (!f.Write(gpApp->m_eolStr)) // wx version adds eolStr to output
					return FALSE;
			}
	#endif	// end of if _UNICODE
	}// end of if (OutStr.GetLength() != 0)
	return TRUE;
}

// BEW 12Apr10 no changes needed for doc version 5
int ParseMarkerRTF(wxChar* pChar, wxChar* pEndChar)
{
	int len = 0;
	wxChar* ptr = pChar;
	wxChar* pEnd = pEndChar;
	while (ptr < pEnd && !wxIsspace(*ptr) && *(ptr +1) != _T('{') && *(ptr +1) != _T('}'))
	{
		ptr++;
		len++;
		// The Doc's ParseMarker was ammended 17May06 to halt after asterisk (end marker)
		// and this ParseMarkerRTF should also halt after end marker, especially since
		// in the MFC version parsing sometimes leaves no space after an end marker.
		if (*(ptr -1) == _T('*')) // whm added 22Nov07 to halt after asterisk (end marker)
			break;
	}
	return len;
}

// BEW 12Apr10 no changes needed for doc version 5
bool IsMarkerRTF(wxChar *pChar, wxChar* pBuffStart)
{
	pBuffStart = pBuffStart; // to avoid a compiler warning - later sometime, remove the 2nd parameter
	// This is an RTF aware version of Bruce's IsMarker in the Doc.
	// IsMarkerRTF is used in parsing text on its way to RTF output. RTF formatted text
	// cannot have bare embedded curly brace characters because because they act as
	// RTF controls rather than plain characters in the output stream. In order for them
	// to be interpreted as plain characters they must be "escaped" by prefixing them with
	// a backslash character. Adding these backslashes is done by our ApplyOutputFilterToText()
	// function on text which is being processed for RTF output. This IsMarkerRTF() is
	// designed to return FALSE if the character following a backslash character at pChar is
	// either an opening curly brace { or a closing curly brace }.
	// whm 8Nov07 modified to also return false if the character following a backslash
	// character is another backslash character. This additional check insures that 
	// IsMarkerRTF does not return TRUE for a non-marker backslash entered as normal text 
	// by the user, since ApplyOutputFilterToText() now also escapes such non-marker 
	// backslash characters found in user entered text.
	if (*(pChar +1) == _T('{') || *(pChar +1) == _T('}')|| *(pChar +1) == _T('\\'))
	{
		return FALSE;
	}
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	// the remainder of this function is identical to the Doc's IsMarker() function.
	// BEW changed 10Apr06, because the doc version was made much longer and so it
	// is better to here just call that rather than repeat its contents below
	//return pDoc->IsMarker(pChar,pBuffStart);
	return pDoc->IsMarker(pChar);
	/* legacy IsMarker() code, pre 3.0.9
	if (gbSfmOnlyAfterNewlines)
	{
		if (pDoc->IsPrevCharANewline(pChar,pBuffStart))
		{
			return *pChar == gSFescapechar;
		}
		else
			return FALSE; // can't be a marker, even if it is gSFescapechar
	}
	else
		return *pChar == gSFescapechar;
	*/
}

// BEW 12Apr10 no changes needed for doc version 5 (I think)
int ParseMarkerAndAnyAssociatedText(wxChar* pChar, wxChar* pBuffStart,
					wxChar* pEndChar, wxString bareMarkerForLookup, wxString wholeMarker,
					bool parsingRTFText, bool InclCharFormatMkrs)
{
	// ParseMarkerAndAnyAssociatedText() is used in ApplyOutputFilterToText().
	// When ParseMarkerAndAnyAssociatedText() is called ptr must be pointing at a marker.
	// This function parses the marker, and continues parsing through any associated text
	// and through any end marker that corresponds to the marker. Since this
	// function is used to parse markers and associated text for filtering purposes, it is
	// imperative that we do not allow the parsing to continue beyond the actual associated
	// text of the current marker, otherwise we could filter out material that is not supposed
	// to be filtered.
	// If the user forgets to add a closing marker that requires one, and is trying to
	// filter the marker from output, he may potentially filter out the remainder of the
	// document. The only markers that have optional end markers are embedded content
	// markers, which are only found in footnotes, endnotes and crossrefs. The embedded
	// content markers are grayed out in the export options marker list box, so they cannot
	// be filtered separately from their enclosing footnote/endnote/crossref.
	// Note: when parsingRTFText is TRUE the View's IsMarkerRTF() function is used rather
	// than the Doc's IsMarker() function.

	wxChar* ptr = pChar;
	wxChar* pEnd = pEndChar;
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	USFMAnalysis* pSfm = pDoc->LookupSFM(bareMarkerForLookup); // use LookupSFM which properly handles \bt... forms as \bt
	bool bNeedsEndMarker;
	if (pSfm == NULL)
	{
		bNeedsEndMarker = FALSE; // treat unknown markers as those without end markers
	}
	else if (!pSfm->endMarker.IsEmpty())
	{
		// AI_USFM.xml says it needs an end marker
		bNeedsEndMarker = TRUE;
	}
	else
	{
		// pSfm is not NULL and it doesn't have an end marker
		bNeedsEndMarker = FALSE;
	}

	int itemLen = 0;
	int txtLen = 0;

	ptr++; // point past initial backslash of current marker
	if (ptr == pEnd)
		return 0;
	txtLen++;

	if (bNeedsEndMarker)
	{
		// wholeMarker needs an end marker so parse until we either find the end
		// marker or until the end of the buffer
		// Note: when bNeedsEndMarker the InclCharFormatMkrs flag has no effect since
		// the parsing goes until an end marker is reached or the end of the buffer
		while (ptr < pEnd && !pDoc->IsCorresEndMarker(wholeMarker, ptr, pEnd))
		{
			ptr++;
			txtLen++;
		}
		if (ptr < pEnd)
		{
			// we found a corresponding end marker to we need to parse it too
			if (parsingRTFText)
			{
				itemLen = ParseMarkerRTF(ptr,pEnd);
			}
			else
			{
				itemLen = pDoc->ParseMarker(ptr);
			}
			ptr += itemLen;
			txtLen += itemLen;
		}
		return txtLen;
	}
	else
	{
		// wholeMarker doesn't have an end marker, so parse until we either
		// encounter another marker, or until the end of the buffer
		if (parsingRTFText)
		{
			// RTF aware uses IsMarkerRTF()
			if (!InclCharFormatMkrs)
			{
				while (ptr < pEnd && !IsMarkerRTF(ptr,pBuffStart))
				{
					ptr++;
					txtLen++;
				}
			}
			else
			{
				// we should continue parsing through any character format markers
				// and end markers (charFormatMkrs and charFormatEndMkrs)
				while (ptr < pEnd)
				{
					if (IsMarkerRTF(ptr,pBuffStart) && !IsCharacterFormatMarker(ptr))
					{
						break;
					}
					else
					{
						ptr++;
						txtLen++;
					}
				}
			}
		}
		else
		{
			// normal use of IsMarker()
			if (!InclCharFormatMkrs)
			{
				//while (ptr < pEnd && !pDoc->IsMarker(ptr,pBuffStart))
				while (ptr < pEnd && !pDoc->IsMarker(ptr))
				{
					ptr++;
					txtLen++;
				}
			}
			else
			{
				// we should continue parsing through any character format markers
				// and end markers (charFormatMkrs and charFormatEndMkrs)
				while (ptr < pEnd)
				{
					//if (pDoc->IsMarker(ptr,pBuffStart) && !IsCharacterFormatMarker(ptr))
					if (pDoc->IsMarker(ptr) && !IsCharacterFormatMarker(ptr))
					{
						break;
					}
					else
					{
						ptr++;
						txtLen++;
					}
				}
			}
		}
		return txtLen;
	}
}

wxString EscapeAnyEmbeddedRTFControlChars(wxString& textStr)
{
	// The special RTF control characters are '\' '{' and '}'. If any of these are embedded
	// We can't really deal with stray backslash chars embedded in the text because they
	// get interpreted as unknown markers on input of the text into Adapt It. But we need
	// to deal with any { or } curly braces that may exist (even as legitimate text chars)
	// by escaping them with a backslash for RTF output.
	// in textStr they must be "escaped" with a backslash character as "\\" "\{" and "\}".
	if (textStr.IsEmpty()) // don't do anything if textStr is empty
		return textStr;
	if (textStr.Find(_T('\\')) == -1 && textStr.Find(_T('{')) == -1 && textStr.Find(_T('}')) == -1)
	{
		// there were no special RTF control characters in the textStr so return
		return textStr;
	}
	// if we get here there was at least one control char in textStr to deal with so for the sake of
	// speed we'll insert the '\' escape char before the '\' and/or '{' and/or '}' in a buffer
	int nTheLen = textStr.Length();
	// wx version the pBuffer is read-only so use GetData()
	const wxChar* pBuffer = textStr.GetData();
	wxChar* pBufStart = (wxChar*)pBuffer;		// save start address of Buffer
	wxChar* pEnd;
	pEnd = pBufStart + nTheLen;// bound past which we must not go
	wxASSERT(*pEnd == _T('\0')); // ensure there is a null at end of Buffer
	pEnd = pEnd; // avoid compiler warning
	// Setup copy-to buffer textStr2. It needs to be twice the size of input buffer since
	// we will be adding a backslash for every control char we find
	wxString textStr2;
	//wxChar* pBuffer2;
	// whm 8Jun12 modified to use wxStringBuffer
	// Create the wxStringBuffer in a specially scoped block. This is crucial here
	// in this function since the wxString textStr2 is accessed directly within
	// this function in the return textStr2; statement at the end of the function.
	{ // begin special scoped block
		wxStringBuffer pBuffer2(textStr2,nTheLen*2 + 1);
		//pBuffer2 = textStr2.GetWriteBuf(nTheLen*2 + 1);

		wxChar* pOld = pBufStart;  // source
		wxChar* pNew = pBuffer2; // destination
		while (*pOld != (wxChar)0)
		{
			if (*pOld == _T('\\') || *pOld == _T('{') || *pOld == _T('}'))
			{
				// there is a control char we need to escape to make it a legitimate text char in RTF
				// We "escape" it by inserting a backslash character before it
				*pNew++ = _T('\\');
			}
			// just copy whatever we are pointing at and then advance
			*pNew++ = *pOld++;
		}
		*pNew = (wxChar)0; // add a null at the end of the string in pBuffer2
	} // end of special scoping block
	//textStr2.UngetWriteBuf(); // whm 8Jun12 removed - not needed in wxStringBuffer for 2.9.3
	return textStr2;
}

wxString FormatRTFFootnoteIntoString(wxString callerStr, wxString assocMarkerText,
							wxString noteRefNumStr, wxString SindocFnCaller, wxString SindocFnText,
							wxString SindocAnnotRef, wxString SindocAnnotText,
							bool addSpBeforeCallerStr)
{
	wxString MiscRTF;
	MiscRTF = _T('{'); // add opening char style group brace
	// if NavStr is not empty addSpBeforeCallerStr will be TRUE
	if (addSpBeforeCallerStr)
		MiscRTF += _T(' ');
	// next output the \cs style tags for _footnote_caller which are in rtfTagsMap
	MiscRTF += SindocFnCaller;
	// add the caller and footnote tag prefixes
	MiscRTF += callerStr;
	MiscRTF += _T(" {\\footnote \\pard\\plain ");
	// add the \s paragraph style tags for footnote text \f which are in rtfTagsMap
	MiscRTF += SindocFnText;
	// add dest caller group which is opening brace, the _footnote_caller tags, caller and closing brace
	MiscRTF += _T('{') + SindocFnCaller + callerStr + _T(' ') + _T('}');
	// Note: In the Unicode version assocMarkerText could have encoding other than
	// system encoding. Since assocMarkerText is here being added to MiscRTF which, in
	// the caller eventually gets output with WriteOutputString using m_systemEncoding
	// we don't want to mix the encodings. Therefore, to be safe, for the Unicode version
	// we'll convert the assocMarkerText to RTF unicode chars (as WriteOutputString does
	// for non-systemEncoding). It won't hurt to do so even if the assocMarkerText is in
	// ASCI range.
#ifdef _UNICODE
	wxChar Ch;
	//wxChar rbuf[34];
	wxString RTFUnicode = _T("");
	int nOSLength = assocMarkerText.Length();	// get number of chars in OutStr
	for (int nCharNum = 0; nCharNum < nOSLength; nCharNum++ )
	{
		Ch = assocMarkerText[nCharNum];
		RTFUnicode += _T("\\u");
		// whm 6Nov07 corrected line below which lacked the (int) cast on Ch in previous versions
		// which would generate RTF unicdoe sequences with the actual Ch instead of a decimal value
		// for N in the \uN\? sequence. The (int) cast was correct in the WriteOutputString() function
		// used elsewhere.
		RTFUnicode << (int)Ch; //RTFUnicode << Ch; //RTFUnicode += _itot(Ch,rbuf,10);
		RTFUnicode += _T("\\\'3f");
	}
	// substitute the RTF Unicode ANSI equivalent
	assocMarkerText = RTFUnicode;
#endif

	// add the actual assocMarkerText
	MiscRTF += assocMarkerText;
	// since there is no special char style group on the actual footnote text we end it with
	// just two closing braces
	MiscRTF += _T("}}");
	return MiscRTF;
}

void DivideTextForExtentRemaining(wxClientDC& dC, int extentRemaining, wxString inputStr,
												wxString& fitInRowStr,
												wxString& spillOverStr)
{
	// This function examines the text extent of inputStr to see how much of it can fit
	// within extentRemaining (in twips). If all of the text of inputStr can fit within the extentRemaining
	// the whole of inputStr is returned in fitInRowStr and spillOverStr is empty. If only part of
	// inputStr can fit within extentRemaining, the first part that can fit is returned in fitInRowStr,
	// and the remainder is returned in spillOverStr.
	wxString tempStr = inputStr;
	wxString fitPartStr;
	wxString excessStr;
	wxString workStr;
	wxString tokenStr;
	wxSize extentOfCurrentFreeText;
	wxStringTokenizer tkz(tempStr,_T(" "));
	workStr.Empty();
	while (tkz.HasMoreTokens())
	{
		// add the tokenStr to fitPartStr if it doesn't make fitPartStr get longer than extentRemaining
		tokenStr = tkz.GetNextToken();
		if (workStr.IsEmpty())
			workStr = tokenStr;
		else
		{
			workStr += _T(' ') + tokenStr;
		}
		dC.GetTextExtent(workStr,&extentOfCurrentFreeText.x,&extentOfCurrentFreeText.y);
		if ((int)(float)(extentOfCurrentFreeText.GetWidth())*14.4 > extentRemaining)
		{
			// add remaining tokens to excessStr
			if (!excessStr.IsEmpty())
			{
				excessStr += _T(' ');
				excessStr += tokenStr;
			}
			else
			{
				excessStr += tokenStr;
			}
		}
		else
		{
			if (!fitPartStr.IsEmpty())
			{
				fitPartStr += _T(' ');
				fitPartStr += tokenStr;
			}
			else
			{
				fitPartStr += tokenStr;
			}
		}
	}
	fitInRowStr = fitPartStr;
	spillOverStr = excessStr;
}

// whm 8Nov07 changed behavior to return TRUE if the escaped backslash \\ is detected
// as an RTF control word at the current pChar location. 
// This change also corrects a potential bug by insuring that the current
// pChar pointer ptr currently points at a backslash before testing the character
// following it to see if the following character is a '{', '}' or a '\' character.
bool IsRTFControlWord(wxChar* pChar, wxChar* pEndChar)
{
	if (*pChar != _T('\\')) // whm added this test 8Nov07
		return FALSE;
	wxChar* ptr = pChar;
	wxChar* pEnd = pEndChar;
	if (ptr < pEnd && (*(ptr +1) == _T('{') || *(ptr +1) == _T('}') || *(ptr +1) == _T('\\')))
		return TRUE;
	return FALSE;
}

// whm 8Nov07 changed behavior to agree with the modified logic of IsRTFControlWord so that
// ParseRTFControlWord now parses escaped backslash control words. ParseRTFControlWord assumes
// that pChar is pointing at a valid control word as indicated by a previous call to
// IsRTFControlWord.
int ParseRTFControlWord(wxChar* pChar, wxChar* pEndChar)
{
	int len = 0;
	wxChar* ptr = pChar;
	wxChar* pEnd = pEndChar;
	// When ParseRTFControlWord is called ptr is pointint at the backslash marker
	// so we can safely parse it with
	ptr++;
	len++;
	// then we can continue parsing until we are not pointing at a curly brace or backslash
	// or pEnd or other whitespace
	while (ptr < pEnd && !wxIsspace(*ptr) && (*ptr == _T('{') || *ptr == _T('}') || *ptr == _T('\\')))
	{
		ptr++;
		len++;
	}
	return len;
}

// whm added 9Nov05
// This function outputs OutStr and in the process converts any angle brackets <, <<, > and >> to
// their equivalent RTF tags \lquote, \rquote, \ldblquote, and \rdblquote.
bool WriteOutputStringConvertingAngleBrackets(wxFile& f, wxFontEncoding Encoding, wxString& OutStr, wxChar* inptr)
{
	wxChar* ptr = inptr;
	int vlen = OutStr.Length();
	int Pos = 0;
	wxString VText;
	wxString QuoteText;

	while (Pos < vlen) // keep scanning until we've scanned the whole string
	{
		wxChar TestChar = OutStr[Pos];
		if (Pos < vlen
			&& TestChar != _T('<')
			&& TestChar != _T('>'))
		{
			// it's a vernacular character so output it with vernacular encoding
			VText = TestChar;
			if (!WriteOutputString(f,Encoding,VText))
				return FALSE;
			ptr++;
			Pos++;
		}
		else
		{
			// it's a quote char
			wxChar NextChar;
			if (TestChar == _T('<'))
			{
				if (Pos+1 <= vlen)
				{
					if (Pos+1 == vlen)
					{
						// we are at the end of the string so it must be just <
						QuoteText = _T("\\lquote "); // \lquote is single left quote
						// RTF tags use gpApp->m_systemEncoding
						if (!WriteOutputString(f,gpApp->m_systemEncoding,QuoteText))
							return FALSE;
						// increment our pointers
						Pos++;
						ptr++;
					}
					else
					{
						// there is at least one more character in the string to check
						NextChar = OutStr[Pos+1];
						if (NextChar == _T('<'))
						{
							// we have << so output the left double quote RTF tags with system encoding
							QuoteText = _T("\\ldblquote "); // \ldblquote is left double quote
							// RTF tags use gpApp->m_systemEncoding
							if (!WriteOutputString(f,gpApp->m_systemEncoding,QuoteText))
								return FALSE;
							// increment our pointers
							Pos += 2;
							ptr += 2;
						}
						else
						{
							// we should not normally get here for normal quotations
							// it's a < so output the left single quote RTF tags with system encoding
							QuoteText = _T("\\lquote ");
							// RTF tags use gpApp->m_systemEncoding
							if (!WriteOutputString(f,gpApp->m_systemEncoding,QuoteText))
								return FALSE;
							// increment our pointers
							Pos++;
							ptr++;
						}
					}
				}
			}
			else if (TestChar == _T('>'))
			{
				if (Pos+1 <= vlen)
				{
					if (Pos+1 == vlen)
					{
						// we are at the end of the string so it must be just >
						QuoteText = _T("\\rquote ");
						// RTF tags use gpApp->m_systemEncoding
						if (!WriteOutputString(f,gpApp->m_systemEncoding,QuoteText))
							return FALSE;
						// increment our pointers
						Pos++;
						ptr++;
					}
					else
					{
						// there is at least one more character in the string to check
						NextChar = OutStr[Pos+1];
						if (NextChar == _T('>'))
						{
							// we have >> so output the right double quote RTF tags with system encoding
							QuoteText = _T("\\rdblquote ");
							// RTF tags use gpApp->m_systemEncoding
							if (!WriteOutputString(f,gpApp->m_systemEncoding,QuoteText))
								return FALSE;
							// increment our pointers
							Pos += 2;
							ptr += 2;
						}
						else
						{
							// we should not normally get here for normal quotations
							// it's a > so output the right single quote RTF tags with system encoding
							QuoteText = _T("\\rquote ");
							// RTF tags use gpApp->m_systemEncoding
							if (!WriteOutputString(f,gpApp->m_systemEncoding,QuoteText))
								return FALSE;
							// increment our pointers
							Pos++;
							ptr++;
						}
					}
				}
			}
		}// end of else it's a quote
	}// end of while (Pos < vlen)
	return TRUE;
}

// the charFormatMkrs and charFormatEndMkrs wxStrings are defined as globals in the
// Adapt_ItView.cpp file, and populated with their markers there at their definitions
// BEW comment 6Sep10, Usfm2Oxes adds extra special markers from the USFM 2.3 standard, in
// two private wxString members, the one for markers prepends charFormatMkrs, and the one
// for endmarkers prepends charFormatEndMkrs; and it has a private function,
// IsSpecialTextStyleMkr() for testing the larger lists
bool IsCharacterFormatMarker(wxChar* pChar)
{
	// Returns TRUE if the marker at pChar is a character formatting marker or
	// a character formatting end marker; these are the markers:
	// \qac \qs \qt \nd \tl \dc \bk \pn \wj \k \no \bd \it \bdit \em \sc and the matching
	// list of endmarkers
	wxChar* ptr = pChar;
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	wxASSERT(pDoc);
	wxASSERT(*ptr == _T('\\')); // we should be pointing at the backslash of a marker

	wxString wholeMkr = pDoc->GetWholeMarker(ptr);
	wholeMkr += _T(' '); // add space
	if (charFormatMkrs.Find(wholeMkr) != -1 || charFormatEndMkrs.Find(wholeMkr) != -1)
		return TRUE;
	else
		return FALSE;
}

// BEW 10Apr10, no changes for doc version 5
void BuildRTFTagsMap(wxArrayString& StyleDefStrArray, wxArrayString& StyleInDocStrArray,
									wxString OutputFont,MapMkrToColorStr& colorMap, wxString Sltr_precedence)
{
	int mkrLen = GetMaxMarkerLength(); // Get the maximum length of longest marker. We use this
									// to pad the temporary marker at the beginning of each
									// string in StyleDefStrArray so when we sort the array
									// later the sort will correctly sort by the initial markers.
									// The marker and space padding (to its right) will eventually
									// be removed.

	// Iterate again through the pSfm USFMAnalysis* strings and build the style definition strings
	// placing them in a CStringArray called StyleDefStrArray.
	MapSfmToUSFMAnalysisStruct* pSfmMap; //CMapStringToOb* pSfmMap;
	USFMAnalysis* pSfm;
	wxString key;
	wxString SDefStr,SIndocStr,cMapAssocStr;
	int arrayIndx = 0;

	// Note: Most of the irregular or non-standard tag strings are base styles and other
	// styles that need tags that can only be derived partially from our AI_USFM.xml
	// attributes, or situations where rare/infrequent tags must be added to the RTF tag
	// strings. The inclusion of such attributes in AI_USFM.xml would unnecessarily inflate
	// the number of RTF related style attributes contained there all for the sake of the
	// few that need them. To handle the irregular items I've chosed to use unique
	// styleType enum names for most of those styles that need special handling. They are
	// handled in BuildRTFStyleTagString(). ProcessIrregularTagsInArrayStrings() currently
	// only handles removing the \s0 of the default paragraph style, but could be employed
	// to do other types of special handling if needed.

	// Now add the standard/regular style definition strings to StyleDefStrArray
	// and the standard/regular in-document tag strings to InDocTagsArray
	wxString fullMkr;
	pSfmMap = gpApp->GetCurSfmMap(gpApp->gCurrentSfmSet);
	//bool colorFound = FALSE; // set but unused
	MapSfmToUSFMAnalysisStruct::iterator iter;
	for (iter = pSfmMap->begin(); iter != pSfmMap->end(); ++iter)
	{
		// Retrieve each USFMAnalysis struct from the map
		pSfm = iter->second;
		fullMkr = gSFescapechar + pSfm->marker;
		//colorFound = FALSE;
		MapMkrToColorStr::iterator citer;
		citer = colorMap.find(pSfm->marker); // assigns cMapAssocStr
		if (citer != colorMap.end())
		{
			cMapAssocStr = citer->second;
		}
		else
		{
			cMapAssocStr.empty();
		}
		BuildRTFStyleTagString(pSfm,SDefStr,SIndocStr,arrayIndx,OutputFont,cMapAssocStr,mkrLen,Sltr_precedence);
		// Note: BuildRTFStyleTagString() adds the sfm marker name prefixed to each string and
		// is padded with spaces out to the length of the longest sfm. This prefix is used to sort
		// the arrays and used in other routines below, and afterwards removed. For example:
		// "f              \s49\qj \li0\ri0\widctlpar\rtlpar\nooverflow\rin0\lin0\itap0 \f2\fs16 "
		StyleDefStrArray.Add(SDefStr); // add SDefStr to the array of style definitions
		wxASSERT(StyleDefStrArray[arrayIndx] == SDefStr);
		StyleInDocStrArray.Add(SIndocStr);
		wxASSERT(StyleInDocStrArray[arrayIndx] == SIndocStr);
		arrayIndx++;
	}

	// Sort the CStringArrays and resolve the sbasedonN and snextN references within the strings
	// held within the StyleDefStrArray. After sorting the original N numbers in the \sN and \csN
	// style definition and in_document strings will be out of numerical order, but we make adjustments
	// for that in SortAndResolveStyleIndexRefs. We also resolve the cross-references within the
	// stylesheet strings - primarily for the N in \sbasedonN and \snextN references where N must
	// reference the new numbers assigned to the appropriate styles.
	SortAndResolveStyleIndexRefs(StyleDefStrArray,StyleInDocStrArray);

	// Some sfm tag strings need additional tag manipulation, i.e. Normal and Footnote
	// Currently (10Nov05) the only thing ProcessIrregularTagsInArrayStrings changes is it
	// removes the \s0 from the __normal style.
	ProcessIrregularTagsInArrayStrings(StyleDefStrArray,StyleInDocStrArray);

	// Now add the sfm marker key and in-document tag strings to rtfTagsMap
	// While we are at it, we can remove the sfm prefix from the array strings
	wxString bareMkr,tempSdefStr,tempSindocStr,keyStr;
	int indx;
	int sIndocStrPos;
	int sDefStrPos;
	int totStrings = (int)StyleInDocStrArray.GetCount(); // should be same for sDef and sIndoc array
	for (indx = 0; indx < totStrings; indx++)
	{
		sIndocStrPos = 0;
		sDefStrPos = 0;
		tempSindocStr = StyleInDocStrArray[indx];
		tempSdefStr = StyleDefStrArray[indx];
		while (sIndocStrPos < (int)tempSindocStr.Length() && tempSindocStr[sIndocStrPos] != _T('\\') && tempSindocStr[sIndocStrPos] != _T('{'))
		{
			sIndocStrPos++;
		}
		while (sDefStrPos < (int)tempSdefStr.Length() && tempSdefStr[sDefStrPos] != _T('\\') && tempSdefStr[sDefStrPos] != _T('{'))
		{
			sDefStrPos++;
		}
		// get key from tempSindocStr
		keyStr = tempSindocStr.Mid(0,sIndocStrPos);
		keyStr.Trim(FALSE); // trim left end
		keyStr.Trim(TRUE); // trim right end
		tempSindocStr = tempSindocStr.Mid(sIndocStrPos); // no 2nd param gets remaining string
		tempSdefStr = tempSdefStr.Mid(sDefStrPos);
		// insert the key - string association into the map
		rtfTagsMap[keyStr] = tempSindocStr; //rtfTagsMap.insert(MkrTagStr_Pair(keyStr,tempSindocStr));
		// prefixed stuff is gone so save the remaining string back to their arrays
		StyleInDocStrArray[indx] = tempSindocStr;
		StyleDefStrArray[indx] = tempSdefStr;
	}
}

bool OutputTextAsBoxedParagraph(wxFile& f, wxString& assocText,
					wxString bareMkr, bool bProcessingTable,
					enum BoxedParagraphType boxType)
{
	wxString MiscRTF;
	int nOpeningBraces = 0;
	int nClosingBraces = 0;
	assocText = assocText.Mid(assocText.Find(_T(' '))+1);

	// remove any embedded word count |@...@| stuff from the (free) string
	if (assocText.Find(_T("|@")) != -1)
	{
		assocText = RemoveFreeTransWordCountFromStr(assocText);
	}

	if (boxType == double_border)
	{
		// output the _double_boxed_para paragraph style tags for "free"
		rtfIter = rtfTagsMap.find(_T("_double_boxed_para"));
		if (rtfIter != rtfTagsMap.end())
		{
			// we found an associated value for Marker in map
			// RTF tags use gpApp->m_systemEncoding
			wxString tempStyle = (wxString)rtfIter->second;
			if (bProcessingTable)
			{
				tempStyle = tempStyle.Mid(5); // delete \par when in tables
			}
			CountTotalCurlyBraces(tempStyle,nOpeningBraces,nClosingBraces);
			if (!WriteOutputString(f,gpApp->m_systemEncoding,tempStyle))
				return FALSE;
		}
	}
	else if (boxType == single_border)
	{
		// output the _single_boxed_para paragraph style tags for "bt" and any other bareMkr
		rtfIter = rtfTagsMap.find(_T("_single_boxed_para"));
		if (rtfIter != rtfTagsMap.end())
		{
			// we found an associated value for Marker in map
			// RTF tags use gpApp->m_systemEncoding
			wxString tempStyle = (wxString)rtfIter->second;
			if (bProcessingTable)
			{
				tempStyle = tempStyle.Mid(5); // delete \par when in tables
			}
			CountTotalCurlyBraces(tempStyle,nOpeningBraces,nClosingBraces);
			if (!WriteOutputString(f,gpApp->m_systemEncoding,tempStyle))
				return FALSE;
		}
	}

	MiscRTF = _T('{'); // opening brace for bt/free character style
	CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // add one opening curly brace
	if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
		return FALSE;
	rtfIter = rtfTagsMap.find(bareMkr);
	if (rtfIter != rtfTagsMap.end())
	{
		// we found an associated value for Marker in map
		// RTF tags use gpApp->m_systemEncoding
		wxString tempStyle = (wxString)rtfIter->second;
		CountTotalCurlyBraces(tempStyle,nOpeningBraces,nClosingBraces);
		if (!WriteOutputString(f,gpApp->m_systemEncoding,tempStyle))
			return FALSE;
	}
	// output the actual bt/free string
	CountTotalCurlyBraces(assocText,nOpeningBraces,nClosingBraces);
	if (!WriteOutputString(f,gpApp->m_tgtEncoding,assocText)) // use target encoding for notes
		return FALSE;
	MiscRTF = _T('}'); // closing brace for bt/free character style group
	CountTotalCurlyBraces(MiscRTF,nOpeningBraces,nClosingBraces); // add one closing curly brace
	if (!WriteOutputString(f,gpApp->m_systemEncoding,MiscRTF))
		return FALSE;

	// We should have output the same number of opening and closing curly braces
	wxASSERT(nOpeningBraces == nClosingBraces);

	// insert a small paragraph break after the back translation boxed paragraph
	// which will serve to separate it from any other boxed paragraph.
	//rtfIter = rtfTagsMap.find(_T("_small_para_break"));
	//if (rtfIter != rtfTagsMap.end())
	//{
	//	// we found an associated value for Marker in map
	//	// RTF tags use gpApp->m_systemEncoding
	//	wxString tempStyle = (wxString)rtfIter->second;
	//	if (bProcessingTable)
	//	{
	//		tempStyle = tempStyle.Mid(5); // delete \par when in tables
	//	}
	//	if (!WriteOutputString(f,gpApp->m_systemEncoding,tempStyle))
	//		return FALSE;
	//}
	return TRUE;
}

bool OutputAnyBTorFreeMaterial(wxFile& f, wxFontEncoding WXUNUSED(Encoding),
					wxString Marker,
					wxString bareMkr,
					wxString& assocText,
					wxString& LastStyle,
					wxString& LastParaStyle,
					int& callerRefNumInt,
					bool& bLastParagraphWasBoxed,
					enum ParseError& parseError,
					enum CallerType& callerType,
					bool bProcessingTable,
					bool bPlaceTransInRTFText,
					enum BoxedParagraphType boxType,
					CAdapt_ItDoc* pDoc)
{
	wxString wholeMarker = _T('\\') + Marker;
	if (bPlaceTransInRTFText)
	{
		// The ExportOptionsDlg checkbox specifies that back/free translation should be
		// "placed in a single boxed paragraph within the exported text". We do this by
		// enclosing \bt character style and text inside a _single_boxed_para
		// paragraph style; \free char style and text inside a _double_boxed_para paragraph
		// style; Using different border helps distinguish the two plus Word keeps them
		// as separate boxes rather than melting them together.
		// whm ammended 3Dec05. The box border is a visible indication that this is
		// back or free translation so I think it may be best to remove the "bt... "
		// or "free" prefix from the text in the box
		if (!OutputTextAsBoxedParagraph(f, assocText, bareMkr,
			bProcessingTable, boxType))
		{
			return FALSE;
		}
		bLastParagraphWasBoxed = TRUE;

		// update LastStyle only when doing boxed paragraph
		LastStyle = Marker;
		LastParaStyle = Marker;
		// don't update Last LastNonBoxParaStyle here
	}
	else
	{
		// The ExportOptionsDlg checkbox specifies that back/free translation should be
		// formatted as footnotes. We do this by enclosing the \bt or \free character style
		// and its text within the footnote destination set of tags. We use a
		// special literal caller "bt" or "free" in the text with the assoc text itself
		// in the footnote at the foot of the page. This can be handled with our
		// ProcessAndWriteDestinationText() function

		wxString wholeMarker = _T('\\') + Marker;
		assocText = wholeMarker + _T(' ') + assocText + _T("\\f* ");
		// Note: in the line above we make it end like a footnote by adding "\f* "
		// to fool our function ProcessAndWriteDestinationText into handling it like one.

		bool bIsAtEnd = FALSE;; // set by ProcessAndWriteDestinationText() below
		// construct numerically sequenced caller
		callerRefNumInt++; // increment the N to bt 1, bt 2, bt 3, etc.
		wxString bareMarker = Marker; // backslash already removed; Marker could be bt or bt...
		wxString refNumStr = bareMarker + _T("\\~"); // accommodates "bt " or "bt... " use non-breaking space
		refNumStr << callerRefNumInt; // add N to "bt N"
		refNumStr += _T(' '); // add following space in case other footnotes follow
		wxString callerStr = refNumStr;
		callerType = supplied_by_parameter;

		wxString nullStr = _T("");
		// we'll use system encoding to write the back/free translation text
		//if (!ProcessAndWriteDestinationText(f, gpApp->m_systemEncoding, assocText,
		//	bIsAtEnd, footnoteDest, rtfTagsMap, pDoc, parseError, callerType,
		//	callerStr, FALSE, nullStr))
		// whm 8Nov07 note: We should use m_tgtEncoding to force the writing of the
		// back/free text in the \uN\'f3 RTF Unicode format
		if (!ProcessAndWriteDestinationText(f, gpApp->m_tgtEncoding, assocText,
			bIsAtEnd, footnoteDest, rtfTagsMap, pDoc, parseError, callerType,
			callerStr, FALSE, nullStr))
		{
			return FALSE;
		}
		// Back translation and Free translation text formatted as footnotes don't appear
		// as separate paragraphs within the text so we don't set LastStyle or LastParaStyle or
		// LastNonBoxParaStyle here
	}
	assocText.Empty();

	return TRUE;
}

// BEW 10Apr10, no changes needed for support of doc version 5
wxString BuildColorTableFromUSFMColorAttributes(MapMkrToColorStr& colorMap)
{
	// color table
	// Version 3 sets up the color table dynamically constructing a table with the actual
	// values needed to cover the colors actually used in the AI_USFM.xml specification file.
	// In AI_USFM.xml if an sfm has a color other than default black, it will have a color="n"
	// attribute where n is a 32-bit composite number whose hex representation is 0x00bbggrr.
	// The rr (red) value of RGB can be retrieved by anding the number with 0x000000ff; the
	// gg (green) value can be retrieved by first shifting the number right by 8 bits, then
	// anding the resulting number with 0x000000ff. The bb (blue) value can be retrieved by
	// shifting the number another 8 bits to the right and again anding the resulting number
	// with 0x000000ff.
	// Once a table is dynamically constructed, the RTF tag \cfN is used to apply an RGB value
	// from the color table, where N is the 0-based index into the table (each RGB representation
	// in the table is separated by a semicolon). In MS Word RTF output the first entry is
	// usually omitted (hence the semicolon after the \\colortbl key word). The docs say that
	// this "missing definition indicates that color 0 is the "auto" color."
	// We minimally need black \\red0\\green0\\blue0 which should be cf1 in the table. The other
	// colors can be dynamically created upon scanning for color attribute values in all sfms of
	// the current set. As each sfm with a color attribute is detected, the routine needs to check
	// the color table to see if the color is already present. If not, it needs to be added as a
	// \\redN\\greenN\\blueN; string to the table. Once it is in the table (or found already existing
	// in the table) the program should map the sfm with the index number of the table for later use
	// in building the style tag strings.

	wxArrayString colorTbl;
	colorTbl.Clear();
	// fill the color table with the 17 default values
	// for sorting purposes we start by using strings representing the hex values with leading "0" chars
	// note: the following strings are added to colorTbl in non-sort-order (later below the whole array
	// will be sorted, which will determine the value of N in the \cfN RTF color tags associated with a
	// given usfm marker that specifies a color.
	colorTbl.Add(_T(";")); // first element at index 0 \cf0 is empty for "auto" color
	colorTbl.Add(_T("000000000")); //later becomes "\\red0\\green0\\blue0;"			black int 0 or hex 0x00000000
	colorTbl.Add(_T("000000255")); //later becomes "\\red0\\green0\\blue255;"		blue int 16711680 or hex 0x00ff0000
	colorTbl.Add(_T("000255255")); //later becomes "\\red0\\green255\\blue255;"		aqua int 16776960 or hex 0x00ffff00
	colorTbl.Add(_T("000255000")); //later becomes "\\red0\\green255\\blue0;"		green int 65280 or hex 0x0000ff00
	colorTbl.Add(_T("255000255")); //later becomes "\\red255\\green0\\blue255;"		red/blue mix int 16711935 or hex 0x00ff00ff
	colorTbl.Add(_T("255000000")); //later becomes "\\red255\\green0\\blue0;"		red int 255 or hex 0x000000ff
	colorTbl.Add(_T("255255000")); //later becomes "\\red255\\green255\\blue0;"		yellow int 65535 or hex 0x0000ffff
	colorTbl.Add(_T("255255255")); //later becomes "\\red255\\green255\\blue255;"	white int 16777215 or hex 0x00ffffff
	colorTbl.Add(_T("000000128")); //later becomes "\\red0\\green0\\blue128;"		dark navy int 8388608 or hex 0x00800000
	colorTbl.Add(_T("000128128")); //later becomes "\\red0\\green128\\blue128;"		teal int 8421376 or hex 0x00808000
	colorTbl.Add(_T("000128000")); //later becomes "\\red0\\green128\\blue0;"		dark green 32768 or hex 0x00008000
	colorTbl.Add(_T("128000128")); //later becomes "\\red128\\green0\\blue128;"		deep purple 8388736 or hex 0x00800080
	colorTbl.Add(_T("128000000")); //later becomes "\\red128\\green0\\blue0;"		dark red int 128 or hex 0x00000080
	colorTbl.Add(_T("128128000")); //later becomes "\\red128\\green128\\blue0;"		dark olive green int 32896 or hex 0x00008080
	colorTbl.Add(_T("128128128")); //later becomes "\\red128\\green128\\blue128;"	dark gray int 8421504 or hex 0x00808080
	colorTbl.Add(_T("034139034")); //later becomes "\\red34\\green139\\blue34;"		deep green int 2263842 or hex 0x00228b22

	// populate the string to string map that maps the sfm color="..." attribute to the index for that
	// color in the color table. If a new color is encountered we also add that color to the color
	// table and the map
	colorMap.clear();
	MapSfmToUSFMAnalysisStruct* pSfmMap;
	USFMAnalysis* pSfm;
	wxString key;
	wxString fullMkr;
	int colorInt;
	int rVal,gVal,bVal;
	wxString rStr,gStr,bStr;
	wxString rgbColorTblStr;

	pSfmMap = gpApp->GetCurSfmMap(gpApp->gCurrentSfmSet);

	MapSfmToUSFMAnalysisStruct::iterator iter;
	// enumerate through all markers in pSfmMap and process those markers that
	// have a color="..." attribute
	for( iter = pSfmMap->begin(); iter != pSfmMap->end(); ++iter )
	{
		// Retrieve each USFMAnalysis struct from the map
		pSfm = iter->second;
		fullMkr.Empty();
		fullMkr << gSFescapechar << pSfm->marker;
		colorInt = pSfm->color;
		// retrieve the R,G,B values from the color value. These wxColour methods
		// retrieve the byte values by bitwise shifting where necessary
		wxColour col = Int2wxColour(colorInt);
		rVal = col.Red();
		gVal = col.Green();
		bVal = col.Blue();

		// build strings suitable for sorting using three digit rrrgggbbb string equivalent format
		rgbColorTblStr = rgbColorTblStr.Format(_T("%03d%03d%03d"),rVal,gVal,bVal);

		// now scan the colorTbl wxString array to see if this color table string already exists in
		// the table
		int ct;
		bool exists = FALSE;
		for(ct = 0; ct < (int)colorTbl.GetCount(); ct++)
		{
			if (colorTbl[ct] == rgbColorTblStr)
			{
				exists = TRUE;
				break;
			}
		}
		if (!exists)
		{
			// the color table string does not already exist so we must add it to the default table
			colorTbl.Add(rgbColorTblStr);
		}
		// we map the non-zero color sfms to their index in the color table after sorting below
	}

	// sort the array of rrrgggbbb strings
	colorTbl[0] = _T("      "); // make the first element spaces so it remains at first element in array
	colorTbl.Sort();
	wxASSERT(colorTbl[0] == _T("      "));
	colorTbl[0] = _T(";"); // change the first element back to its original ;
	
	// convert the sorted strings to the RTF color table format
	int asCt;
	wxString changeStr;
	for (asCt = 0; asCt < (int)colorTbl.GetCount(); asCt++)
	{
		changeStr = colorTbl[asCt]; // get the array item (in "rrrgggbbb" form)
		if (changeStr == _T(";")) // first element of color table is ;
		{
			; // do nothing; leave the ";" unchanged in first element of array
		}
		else
		{
			rStr = changeStr.Left(3);
			gStr = changeStr.Mid(3,3);
			bStr = changeStr.Right(3);
			rVal = wxAtoi(rStr); // wxAtoi removes leading zeros
			gVal = wxAtoi(gStr);
			bVal = wxAtoi(bStr);
			rgbColorTblStr.Empty();
			rgbColorTblStr << _T("\\red");
			rgbColorTblStr << (int)rVal;
			rgbColorTblStr << _T("\\green");
			rgbColorTblStr << (int)gVal;
			rgbColorTblStr << _T("\\blue");
			rgbColorTblStr << (int)bVal;
			rgbColorTblStr << _T(';');
			colorTbl[asCt] = rgbColorTblStr; // array item now is \red255\green0\blue64 RTF colortbl format
		}
	}

	// go through the pSfmMap again and build the whole colorMap by associating the 
	// colorIndexStr (\cfN) with the pSfmMap markers that require colors.
	wxString colorIndexStr = _T("\\cf");
	//int cIndex; 
	//cIndex = 0;
	for( iter = pSfmMap->begin(); iter != pSfmMap->end(); ++iter )
	{
		pSfm = iter->second;
		colorInt = pSfm->color;
		wxColour col = Int2wxColour(colorInt);
		rVal = col.Red();
		gVal = col.Green();
		bVal = col.Blue(); 
		// build the RTF color table string, i.e., \red255\green0\blue255;
		rgbColorTblStr.Empty();
		rgbColorTblStr << _T("\\red");
		rgbColorTblStr << (int)rVal;
		rgbColorTblStr << _T("\\green");
		rgbColorTblStr << (int)gVal;
		rgbColorTblStr << _T("\\blue");
		rgbColorTblStr << (int)bVal;
		rgbColorTblStr << _T(';');
		// now map the non-zero color sfms to their index in the color table
		if (colorInt != 0)
		{
			int ct;
			int colorIndex = 0;
			//bool exists = FALSE;
			for(ct = 0; ct < (int)colorTbl.GetCount(); ct++)
			{
				colorIndex = ct;
				if (colorTbl[ct] == rgbColorTblStr)
				{
					//exists = TRUE;
					break;
				}
			}
			// map the associations for fast retrieval later as needed
			colorIndexStr = _T("\\cf");
			colorIndexStr << colorIndex++; // << operator converts cIndex to a string and concatenates it to colorIndexStr
			(colorMap)[pSfm->marker] = colorIndexStr;
		}
	}

	// build the RTF color table header tags from the strings in the CStringArray colorTbl
	wxString ColorTable;
	ColorTable.Empty();
	int ctindx;
	for (ctindx = 0; ctindx < (int)colorTbl.GetCount(); ctindx++)
	{
		ColorTable += colorTbl.Item(ctindx);
		if (ctindx > 0 && ctindx % 3 == 0)
			ColorTable += gpApp->m_eolStr;
	}

	// prefix with a new line, the opening curly bracket and colortable tag key word
	ColorTable = gpApp->m_eolStr + _T("{\\colortbl") + ColorTable;
	// suffix with the closing curly bracket and a new line
	ColorTable += _T("}") + gpApp->m_eolStr;
	return ColorTable;
}

// BEW 12Apr10, no changes needed for doc version 5
void DetermineRTFDestinationMarkerFlagsFromBuffer(wxString& textStr,
		bool& bDocHasFootnotes,
		bool& bDocHasEndnotes,
		bool& bDocHasFreeTrans,
		bool& bDocHasBackTrans,
		bool& bDocHasAINotes)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();

	// Setup input buffer from textStr
	int nTheLen = textStr.Length();
	// wx version: pBuffer is read-only so just get pointer to the string's read-only buffer
	const wxChar* pBuffer = textStr.GetData();
	wxChar* pBufStart = (wxChar*)pBuffer;		// save start address of Buffer
	int itemLen = 0;
	wxChar* pEnd = pBufStart + nTheLen;	// bound past which we must not go
	wxASSERT(*pEnd == _T('\0')); 		// ensure there is a null at end of Buffer
	bool bIsAMarker = FALSE;
	wxString bareMarkerForLookup, wholeMarker;
	wxChar* pOld = pBufStart;  // source
	while (*pOld != (wxChar)0)
	{
		// Use the the View's IsMarkerRTF if bRTFOutput
		bIsAMarker = IsMarkerRTF(pOld,pBufStart);
		if (bIsAMarker)
		{
			// We're pointing at a marker
			wholeMarker = pDoc->GetWholeMarker(pOld); // the whole marker including backslash and any ending *
			// Check for specific markers that would change the destination marker flags.
			// First, look for existence of "\f" and/or "\fe" markers in the doc.
			// The existence of "\f" always indicates a beginning footnote marker in 
			// any sfm set.
			if (wholeMarker == _T("\\f"))
				bDocHasFootnotes = TRUE;
			// Only when "\fe" exists apart from the PngOnly set does it signal existence of endnotes.
			if (gpApp->gCurrentSfmSet != PngOnly)
				if (wholeMarker == _T("\\fe"))
					bDocHasEndnotes = TRUE;
			// Now, look for existence of \free translations in the doc (these are always "filtered").
			if (wholeMarker == _T("\\free"))
					bDocHasFreeTrans = TRUE;
			// Now, look for existence of \bt... in the doc (these are always "filtered").
			if (wholeMarker == _T("\\bt")) // detect any \bt... forms
					bDocHasBackTrans = TRUE;
			// Now, look for existence of \note in the doc (these are always "filtered").
			if (wholeMarker == _T("\\note"))
					bDocHasAINotes = TRUE;
			
			itemLen = ParseMarkerRTF(pOld,pEnd);
			pOld += itemLen;
		}
		else
		{
			// it's not a marker but text
			// just copy whatever we are pointing at and then advance
			pOld++;
		}
	}
}

void CountTotalCurlyBraces(wxString outputStr, int& nOpeningBraces, int& nClosingBraces)
{
	// whm added 8Nov07
	if (outputStr.IsEmpty())
		return; // leave counts as they were and just return

	// counts the number of opening and closing curly braces in outputStr and increments the existing
	// counts in nOpeningBraces and nClosingBraces.
	// ignores curly braces that are "escaped" with a backslash immediately preceeding them in outputStr
	int nOpenBr = nOpeningBraces;
	int nCloseBr = nClosingBraces;
	wxChar OpenBr = _T('{');
	wxChar CloseBr = _T('}');
	wxChar escChar = _T('\\');
	int len = outputStr.Length();
	// wx version note: Since we require a read-only buffer we use GetData which just returns
	// a const wxChar* to the data in the string.
	const wxChar* pBuff = outputStr.GetData();
	wxChar* pBufStart = (wxChar*)pBuff;
	wxChar* pEnd = pBufStart + len;
	wxASSERT(*pEnd == _T('\0'));
	wxChar* ptr = pBufStart;
	while (ptr < pEnd)
	{
		if (*ptr == OpenBr)
		{
			if (ptr == pBuff || (ptr != pBuff && *(ptr -1) != escChar))
			{
				nOpenBr++;
			}
		}
		if (*ptr == CloseBr)
		{
			if (ptr == pBuff || (ptr != pBuff && *(ptr -1) != escChar))
			{
				nCloseBr++;
			}
		}
		ptr++;
	}
	// return counts in the reference parameters
	nOpeningBraces = nOpenBr;
	nClosingBraces = nCloseBr;
}

int ClearBuffer()
{
	gpApp->buffer.Empty();
	return 0;
}

wxString IntToRoman(int num)
{
	// converts the integer num to Roman Numeral equivalent
	// In Roman numerals I = 1, V = 5, X = 10, L = 50, C = 100, D = 500, and M = 1000

	if (num <= 0 || num > 4999)
		return _T("");	// Romans didn't represent zero and ignoring strokes/bars over letters
						// which multiplied the value of the letter by 1000, the strict maximum
						// Roman number is 4999.
						// Our needs are more modest since we only use the Roman numeral for chapter
						// numbers, the usual maximum would should ever need is 150 (XL) for the
						// number of chapters in Psalms.
	wxString numeral;
	numeral.Empty();
	while (num >= 1000)
	{
		numeral += _T("M");
		num -= 1000;
	}
	if (num >= 900)
	{
		numeral += _T("CM");
		num -= 900;
	}
	if (num >= 500)
	{
		numeral += _T("D");
		num -= 500;
	}
	if (num >= 400)
	{
		numeral += _T("CD");
		num -= 400;
	}
	while (num >= 100)
	{
		numeral += _T("C");
		num -= 100;
	}
	if (num >= 90)
	{
		numeral += _T("XC");
		num -= 90;
	}
	if (num >= 50)
	{
		numeral += _T("L");
		num -= 50;
	}
	if (num >= 40)
	{
		numeral += _T("XL");
		num -= 40;
	}
	while (num >= 10)
	{
		numeral += _T("X");
		num -= 10;
	}
	if (num >= 9)
	{
		numeral += _T("IX");
		num -= 9;
	}
	if (num >= 5)
	{
		numeral += _T("V");
		num -= 5;
	}
	if (num >= 4)
	{
		numeral += _T("IV");
		num -= 4;
	}
	while (num >= 1)
	{
		numeral += _T("I");
		num -= 1;
	}
	return numeral;
}

int ParseAnyFollowingChapterLabel(wxChar* pChar, wxChar* pBuffStart, wxChar* pEndChar,
												 wxString& tempLabel)
{
	// This function is called from within the IsChapterMarker() block within the main loop of
	// DoExportSrcOrTgtRTF.
	// When called pChar should be pointing at whatever follows the whitespace following a chapter
	// number.
	// ParseAnyFollowingChapterLabel returns zero and tempLabel is an empty string, if there is
	// no \cl marker at pChar.
	// If pChar is pointing at a \cl marker, the \cl marker is parsed and any text associated with
	// \cl is returned to the caller via the reference parameter tempLabel.
	int len = 0;
	int itemLen; // local var
	wxChar* ptr = pChar;
	wxChar* pEnd = pEndChar;
	wxString tempStr = _T("");
	tempLabel = tempStr;
	// When ParseAnyFollowingChapterLabel is called ptr may be pointing at the backslash marker
	// of a \cl marker if it exists, or could be pointing at another marker or even some non-marker
	// text.
	// If ptr is not pointing at a backslash marker we know their is no \cl immediately following
	// and can return
	if (!IsMarkerRTF(pChar, pBuffStart))
	{
		// we're not pointing at any marker, therefore ther is no \cl immediately following the
		// chapter number
		return len; // len will be zero
	}
	else
	{
		CAdapt_ItDoc* pDoc = gpApp->GetDocument();
		// we're pointing at a marker, is it a \cl marker?
		itemLen = ParseMarkerRTF(ptr, pEnd);
		wxString mkr(ptr,itemLen);
		if (mkr != _T("\\cl"))
		{
			return len; // len will be zero
		}
		// if we get to here we are pointing at a \cl marker
		ptr += itemLen; // point past the marker
		len += itemLen;
		itemLen = pDoc->ParseWhiteSpace(ptr);
		ptr += itemLen;
		len += itemLen;
		// now collect assoc text until we either reach the next marker or the end of the buffer
		while (ptr < pEnd && *ptr != _T('\\'))
		{
			tempStr += *ptr;
			ptr++;
			len++;
		}
		tempStr.Replace(_T("\n"),_T("")); // remove any new line chars
		tempStr.Replace(_T("\r"),_T("")); // remove any new line chars
		tempLabel = tempStr;
		return len;
	}

}

bool PunctuationFollowsDestinationText(int itemLen, wxChar* pChar, wxChar* pEnd, bool OutputSrc)
{
	// return true if punctuation immediately follows in the buffer after the itemLen position
	// otherwise return false.
	wxChar* ptr = pChar + itemLen; // start from the position after the parsed destination text (itemLen)
	wxChar nextCh;
	wxString punctChars;
	if (OutputSrc)
	{
		punctChars = gpApp->m_punctuation[0];
	}
	else
	{
		punctChars = gpApp->m_punctuation[1];
	}
	punctChars.Replace(_T(" "),_T("")); // MFC Remove removes all specified chars from string
	// examine the next character in the buffer to see if it is punctuation
	if (ptr +1 < pEnd)
	{
		// there is a character following within the buffer, so check it
		nextCh = *ptr;
		if (punctChars.Find(nextCh) != -1)
			return TRUE;
	}
	return FALSE;
}

bool NextMarkerIsFootnoteEndnoteCrossRef(wxChar* pChar, wxChar* pEndChar, int itemLen)
{
	// returns TRUE if after pChar is advanced itemLen, the marker at that location is
	// either a footnote (\f), an endnote (\fe), or a Crossref (\x).
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	int iLen;
	wxChar* ptr = pChar;
	wxChar* pEnd = pEndChar;
	if (ptr + itemLen < pEnd)
	{
		ptr += itemLen;
		iLen = pDoc->ParseWhiteSpace(ptr);
		if (ptr + iLen < pEnd)
		{
			ptr += iLen;
			if (*ptr == _T('\\') && ptr + 2 < pEnd)
			{
				if ((*(ptr + 1) == _T('f') && pDoc->IsWhiteSpace(ptr + 2))
					|| (*(ptr + 1) == _T('x') && pDoc->IsWhiteSpace(ptr + 2)))
					return TRUE;
			}
			if (*ptr == _T('\\') && ptr + 3 < pEnd)
			{
				if (*(ptr + 1) == _T('f') && *(ptr + 2) == _T('e')
					&& pDoc->IsWhiteSpace(ptr + 3))
					return TRUE;
			}
		}
	}
	// if we get here the next marker is not \f, \fe, or \x
	return FALSE;
}

bool IsBTMaterialHaltingPoint(wxString Marker)
{
	// Marker as input param is minus backslash, but contains any ending *
	if (Marker.IsEmpty() || Marker.Find(_T('*')) != -1)
	{
		// marker is empty or some kind of ending marker and we don't halt before ending markers
		return FALSE;
	}
	// remove any numeric level suffix from the marker (makes our string of halting markers shorter
	// because we can just compare the non-numeric parts of these markers).
	while (wxIsdigit(Marker[Marker.Length() -1]))
	{
		Marker.Remove(Marker.Length() -1, 1);
	}
	// add the backslash back and terminate with a space for string searches
	wxString wholeMkrSp = _T('\\') + Marker + _T(' ');
	if (btHaltingMarkers.Find(wholeMkrSp) != -1)
		return TRUE;
	return FALSE;
}

bool IsFreeMaterialHaltingPoint(wxString Marker)
{
	// Marker as input param is minus backslash, but contains any ending *
	if (Marker.IsEmpty() || Marker.Find(_T('*')) != -1)
	{
		// marker is empty or some kind of ending marker and we don't halt before ending markers
		return FALSE;
	}
	// remove any numeric level suffix from the marker (makes our string of halting markers shorter
	// because we can just compare the non-numeric parts of these markers).
	while (wxIsdigit(Marker[Marker.Length() -1]))
	{
		Marker.Remove(Marker.Length() -1, 1);
	}
	// add the backslash back and terminate with a space for string searches
	wxString wholeMkrSp = _T('\\') + Marker + _T(' ');
	if (freeHaltingMarkers.Find(wholeMkrSp) != -1)
		return TRUE;

	return FALSE;
}

int ParseEscapedCharSequence(wxChar *pChar, wxChar *pEndChar)
{
	int	length = 0;
	wxChar* ptr = pChar;
	if (*ptr != _T('\\'))
	{
		wxASSERT(FALSE); // something is wrong
		return 0;
	}
	if (ptr < pEndChar)
	{
		ptr++; // point to the next char
		length++;
	}
	else
	{
		return length;
	}
	if (*ptr == _T('\\') || *ptr == _T('{') || *ptr == _T('}'))
	{
		if (ptr < pEndChar)
		{
			ptr++; // point to the next char
			length++;
		}
		else
		{
			return length;
		}
	}
	else
	{
		// the escaping char was not followed by a \, {, or } which should be
		// considered a program error
		::wxBell();
		wxASSERT(FALSE);
	}
	return length;
}

int GetMaxMarkerLength()
{
	MapSfmToUSFMAnalysisStruct* pSfmMap;
	pSfmMap = gpApp->GetCurSfmMap(gpApp->gCurrentSfmSet);
	USFMAnalysis* pSfm;
	MapSfmToUSFMAnalysisStruct::iterator iter; 
	int len;
	int maxLen = 0;
	for( iter = pSfmMap->begin(); iter != pSfmMap->end(); ++iter )
	{
		// Retrieve each USFMAnalysis struct from the map
		pSfm = iter->second;
		len = pSfm->marker.Length();
		if (len > maxLen)
			maxLen = len;
	}
	return maxLen;
}

// BEW 10Apr10, no changes for doc version 5
void BuildRTFStyleTagString(USFMAnalysis* pSfm, wxString& Sdef, wxString& Sindoc,
													  int styleSequNum, wxString outputFontStr,
													  wxString colorTblIndxStr, int mkrMaxLength,
													  wxString Sltr_precedence)
{
	// This is a helper function for DoExportSrcOrTgtRTF().
	// From the passed in USFMAnalsis struct, this function builds two strings: The style definition
	// tag string is returned in sDef, and the in-document style tag string is returned in Sindoc.
	// The strings are built with the necessary RTF tags appropriate for the attributes associated with
	// the sfm defined in the USFMAnalsysis struct. BuildRTFStyleTagString constructs
	// the tags for those sfms which have a fairly uniform inventory of RTF tags depending on their
	// styleType attribute; some sfms require specific formatting tags and are added later in
	// ProcessIrregularTagsInArrayStrings.
	// In the caller the Sdef tag string is used to build the RTF Stylesheet (part of the RTF header)
	// and is also used to construct part of the returned Sindoc tag string which gets embedded at
	// the locations in the output document where the style is applied.
	// Note: Some tag strings may be too irregular and thus need to be set in the rtfMap manually
	// before making calls to this BuildRTFStyleTagString function which is designed for
	// building tag strings which are mostly regular in their construction.

	wxString tempSdef, tempSindoc;
	wxString numberStr;
	numberStr << styleSequNum;

	wxString spacePaddedMarker;
	spacePaddedMarker = pSfm->marker;
	int mct;
	for (mct = 0; mct < mkrMaxLength - (int)pSfm->marker.Length(); mct++)
	{
		spacePaddedMarker += _T(' ');
	}

	// Construct the Sdef and Sindoc tag strings depending on their styleType. The
	// paragraph and character styleTypes are treated immediately below. All other
	// special styleType markers are treated farther below.

	if (pSfm->styleType == paragraph) // all paragraph styleType markers processed here
	{
		Sdef.Empty();
		Sindoc.Empty();
		tempSdef.Empty();
		// start with the style number tag
		tempSdef = _T("\\s") + numberStr;

		// add the alignment paragraph tag based on pSfm's justification attribute
		AddAnyParaAlignment(tempSdef,pSfm);
		// add any first line indent, but only needed if non-zero
		wxString save_ri_N_value;
		wxString save_li_N_value;
		AddAnyParaIndents(tempSdef,pSfm,save_ri_N_value,save_li_N_value);
		// add any space before/space above
		AddAnyParaSpacing(tempSdef,pSfm);
		// add paragraph keep style attributes
		AddAnyParaKeeps(tempSdef,pSfm);
		// always add widow/orphan control to paragraph styles
		tempSdef += _T("\\widctlpar");
		// always add \ltrpar
		tempSdef += Sltr_precedence;
		// always add \nooverflow (probably not needed buy always in Word's RTF output
		tempSdef += _T("\\nooverflow");
		// always add the \rinN value (this seems to override \riN to make it work right for RTL)
		AddAnyRinLin(tempSdef,save_ri_N_value, save_li_N_value);
		// always add the \itap0 and space
		tempSdef += _T("\\itap0 "); // needs the following space
		// add any character bold, italic, underline, smallcaps, etc
		AddAnyCharEnhancements(tempSdef,pSfm);
		// If paragraph style needed a superscript attribute it would go here
		// now add the appropriate font depending on whether we're building source or target RTF output
		if (pSfm->marker != _T("__normal"))
		{
			// __normal style omits the font tag
			tempSdef += outputFontStr; // already has backslash
		}
		// add the font size \fsN where N is half points
		AddAnyFontSizeColor(tempSdef,pSfm, colorTblIndxStr);

		// with tempSdef up to this point we can create the style definition Sdef string parameter
		// Note: we will use the currently built tempSdef string in construction of tempSindoc below.

		// We initially prefix the pSfm->marker to the beginning of the Sdef string. A later call to
		// SortAndResolveStyleIndexRefs() will remove this initial marker prefix after resolving the sbasedonN
		// and snextN references
		Sdef = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the opening { of Sdef which is added below.

		Sdef += _T('{') + tempSdef; // style definitions (Sdef) are enclosed in curly braces. Add opening brace
		// add any \sbasedonN tag using pSfm's basedOn attribute
		// Note: We will juxtapose the referenced to marker name within square brackets so that the
		// string at this point becomes \sbasedon[marker name]. Then after all styles are processed
		// an auxiliary function called void SortAndResolveStyleIndexRefs() can process the array of Sdef
		// strings and resolve the marker names within the square brackets appended to \sbasedon to
		// realize the correct Sdef array index value to substitute as a string in the place
		// of [marker name]. The processing of both \sbasedonN and \snextN uses this method of
		// placing the marker name in square brackets, to be resolved later.
		AddAnyBasedonNext(Sdef,pSfm);

		// The \ssemihidden tag will be handled via the styleType attribute sections

		// add the actual style name which is copied from pSfm's navigationText attribute
		AddAnyStylenameColon(Sdef,pSfm);
		// finally add the closing curly brace that completes the RTF style definition
		Sdef += _T('}');

		// now build the tempSindoc and return in Sindoc. This utilizes the last form of tempSdef which
		// was built above
		if (pSfm->marker != _T("id"))
		{
			tempSindoc = _T("\\par ")+gpApp->m_eolStr+_T("\\pard\\plain ");
		}
		else
		{
			// do not add \par \n prefix to \id because it should come first in the file
			tempSindoc = _T("\\pard\\plain ");
		}
		tempSindoc += tempSdef; // tempSdef was built above
		// Note: the in-document tags never have \sbasedonN or \snextN tags so we do not have to
		// resolve any N references for them. But, we do, need to make sure their style
		// numbers \sN and \csN are numbered the same and in sync with the style numbers in Sdef,
		// so we'll initially prefix the pSfm->marker to the beginning of the Sindoc string too.
		// A later call to SortAndResolveStyleIndexRefs() will remove this initial marker prefix
		// after making sure the style numbers are correct.
		Sindoc = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the first character (\) of Sindoc.
		// The in-document tags are complete so return the string in Sindoc
		Sindoc += tempSindoc;
	}
	else if (pSfm->styleType == character) // all character styleType markers processed here
	{
		// Character styles are built differently from paragraph styles. We'll add the
		// style number and \additive to Sdef later

		Sdef.Empty();
		Sindoc.Empty();
		// start with an empty tempSdef
		tempSdef.Empty();
		// add any character bold, italic, underline, smallcaps, etc
		AddAnyCharEnhancements(tempSdef,pSfm);
		// Usually fontSize for character styles is 0 (zero) since the font size remains
		// a characteristic of the underlying paragraph style. But we'll make room for it
		// in case a user wants to define a strang character style.
		// add the font size \fsN where N is half points (we multiply fontSize by 2)
		AddAnyFontSizeColor(tempSdef,pSfm,colorTblIndxStr);
		// add any superscript
		if (pSfm->superScript)
			tempSdef += _T("\\super");	// Version 2 used \up2 and \super for footnote reference style
										// and verse number style had \up6 only, no \super tag
										// We could make the N of \up (amount of superscript
										// elevation) user specifiable?
										// Check that \super is sufficient for footnote reference style.
		if (tempSdef.IsEmpty())
		{
			// It is possible that tempSdef is empty if there are no significant attributes
			// defined for this character marker in AI_USFM_full.xml.
			// tempSdef must always end in a space, even if there are no significant attributes
			tempSdef = _T(' ');
		}
		// add space if tempSdef does not already end with one
		if (tempSdef.Length() > 0 && tempSdef[tempSdef.Length() -1] != _T(' '))
			tempSdef += _T(' ');
		// Now build the complete Sdef strings
		// We initially prefix the marker with space padding to the beginning of the string.
		// A later call to SortAndResolveStyleIndexRefs() will remove this initial marker and
		// space padding prefix.
		Sdef = spacePaddedMarker;
		// The style definition must begin with \* prefixed to the style number tag.
		Sdef += _T("{\\*\\cs") + numberStr;
		Sdef += _T(' '); // add space
		Sdef += _T("\\additive"); // allways use the additive tag on character style definitions
		Sdef += _T(' '); // add space
		Sdef += tempSdef; // add what we built above
		//AddAnyBasedonNext(Sdef,pSfm);
		// add the actual style name which is copied from pSfm's navigationText attribute
		AddAnyStylenameColon(Sdef,pSfm);
		// finally add the closing curly brace that completes the RTF style definition
		Sdef += _T('}');

		// Now build the complete Sindoc string
		// Note: the in-document tags never have \sbasedonN or \snextN tags so we do not have to
		// resolve any N references for them. But, we do, need to make sure their style
		// numbers \sN and \csN are numbered the same and in sync with the style numbers in Sdef,
		// so we'll initially prefix the pSfm->marker to the beginning of the Sindoc string too.
		// A later call to SortAndResolveStyleIndexRefs() will remove this initial marker prefix
		// after making sure the style numbers are correct.
		Sindoc = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the first character (\) of Sindoc.
		Sindoc += _T("\\cs") + numberStr;	// Sindoc doesn't have prefixed \*
		Sindoc += tempSdef; // add what we built above; tempSdef ends with a space
	}

	else if (pSfm->styleType == table_type) // special treatment of \_normal_table and \_table_grid markers

	{
		Sdef.Empty();
		Sindoc.Empty(); // The Normal Table style is provided only as default table style - it is not
						// referenced in our output, so no indoc or key needed
		// start with an empty tempSdef
		tempSdef.Empty();
		// add the special table tags
		tempSdef += _T("\\tsrowd\\trftsWidthB3\\trpaddl108\\trpaddr108\\trpaddfl3\\trpaddft3\\trpaddfb3\\trpaddfr3");
		tempSdef += _T("\\tscellwidth\\fts0\\tsvertalt\\tsbrdrt\\tsbrdrl\\tsbrdrb\\tsbrdrr\\tsbrdrdgl\\tsbrdrdgr\\tsbrdrh\\tsbrdrv ");
		AddAnyParaAlignment(tempSdef,pSfm);
		wxString save_ri_N_value;
		wxString save_li_N_value;
		AddAnyParaIndents(tempSdef,pSfm, save_ri_N_value, save_li_N_value);
		// add some other constantly occurring Normal Table tags
		tempSdef += _T("\\widctlpar\\aspalpha\\aspnum\\faauto\\adjustright");
		AddAnyRinLin(tempSdef,save_ri_N_value,save_li_N_value);
		// add the \itap0 and space
		tempSdef += _T("\\itap0 "); // needs the following space
		if (pSfm->marker == _T("_table_grid"))
			tempSdef += _T("\\f0"); // add the Normal (Arial) font to Table Grid (get overridden in real data)
		AddAnyFontSizeColor(tempSdef,pSfm, colorTblIndxStr);
		// Now build the complete Sdef string
		// We initially prefix the marker with space padding to the beginning of the string.
		// A later call to SortAndResolveStyleIndexRefs() will remove this initial marker and
		// space padding prefix.
		// now use Sdef
		Sdef = spacePaddedMarker;
		// The Normal Table style definition must begin with \* prefixed to the \ts and style number tag.
		Sdef += _T("{\\*\\ts") + numberStr;
		Sdef += tempSdef; // add what we built above
		AddAnyBasedonNext(Sdef,pSfm);
		// Normal Table style has \ssemihidden but Table Grid does not
		if (pSfm->marker == _T("_normal_table"))
			Sdef += _T("\\ssemihidden");
		AddAnyStylenameColon(Sdef,pSfm);
		// finally add the closing curly brace that completes the RTF style definition
		Sdef += _T('}');

		// Even though we don't need Sindoc for the Normal Table we need to build it to keep the
		// arrays in sync
		Sindoc = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the first character (\) of Sindoc.
		Sindoc += _T("\\ts") + numberStr; // Sindoc doesn't have prefixed {\*
		Sindoc += tempSdef; // add what we built above; tempSdef ends with a space

		// table (Normal Table) style tag string should look something like this:
		// Sdef:
		//  {\*\ts11\tsrowd\trftsWidthB3\trpaddl108\trpaddr108\trpaddfl3\trpaddft3\trpaddfb3\trpaddfr3"
		//	\tscellwidth\fts0\tsvertalt\tsbrdrt\tsbrdrl\tsbrdrb\tsbrdrr\tsbrdrdgl\tsbrdrdgr\tsbrdrh"
		//  \tsbrdrv \ql \li0\ri0\widctlpar\aspalpha\aspnum\faauto\adjustright\rin0\lin0\itap0 \fs20 "
		//  \snext11 \ssemihidden Normal Table;}"
		// Indoc:
		//  \ts11\tsrowd\trftsWidthB3\trpaddl108\trpaddr108\trpaddfl3\trpaddft3\trpaddfb3\trpaddfr3"
		//	\tscellwidth\fts0\tsvertalt\tsbrdrt\tsbrdrl\tsbrdrb\tsbrdrr\tsbrdrdgl\tsbrdrdgr\tsbrdrh"
		//  \tsbrdrv \ql \li0\ri0\widctlpar\aspalpha\aspnum\faauto\adjustright\rin0\lin0\itap0 \fs20 "
	}
	else if (pSfm->styleType == footnote_caller) // special treatment of \_footnote_caller marker
	{
		Sdef.Empty();
		Sindoc.Empty();
		tempSdef.Empty();
		// add any character bold, italic, underline, smallcaps, etc
		AddAnyCharEnhancements(tempSdef,pSfm);
		// Usually fontSize for character styles is 0 (zero) since the font size remains
		// a characteristic of the underlying paragraph style. But we'll make room for it
		// in case a user wants to define a strang character style.
		// add the font size \fsN where N is half points (we multiply fontSize by 2)
		AddAnyFontSizeColor(tempSdef,pSfm,colorTblIndxStr);
		// add any superscript
		if (pSfm->superScript)
			tempSdef += _T("\\super");	// Version 2 used \up2 and \super for footnote reference style
										// and verse number style had \up6 only, no \super tag
										// We could make the N of \up (amount of superscript
										// elevation) user specifiable.
		// add space if tempSdef does not already end with one
		if (tempSdef.Length() > 0 && tempSdef[tempSdef.Length() -1] != _T(' '))
			tempSdef += _T(' ');
		// Now build the complete Sdef strings
		// We initially prefix the marker with space padding to the beginning of the string.
		// A later call to SortAndResolveStyleIndexRefs() will remove this initial marker and
		// space padding prefix.
		Sdef = spacePaddedMarker;
		// The style definition must begin with \* prefixed to the style number tag.
		Sdef += _T("{\\*\\cs") + numberStr;
		Sdef += _T(' '); // add space
		Sdef += _T("\\additive"); // allways use the additive tag on character style definitions
		Sdef += _T(' '); // add space
		Sdef += tempSdef; // add what we built above
		AddAnyBasedonNext(Sdef,pSfm);
		// add the actual style name which is copied from pSfm's navigationText attribute
		Sdef += _T("\\ssemihidden");
		AddAnyStylenameColon(Sdef,pSfm);
		// finally add the closing curly brace that completes the RTF style definition
		Sdef += _T('}');

		// Now build the complete Sindoc string
		// Note: the in-document tags never have \sbasedonN or \snextN tags so we do not have to
		// resolve any N references for them. But, we do, need to make sure their style
		// numbers \sN and \csN are numbered the same and in sync with the style numbers in Sdef,
		// so we'll initially prefix the pSfm->marker to the beginning of the Sindoc string too.
		// A later call to SortAndResolveStyleIndexRefs() will remove this initial marker prefix
		// after making sure the style numbers are correct.
		Sindoc = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the first character (\) of Sindoc.
		Sindoc += _T("\\cs") + numberStr;	// Sindoc doesn't have prefixed \*
		Sindoc += tempSdef; // add what we built above; tempSdef ends with a space

		// footnote_caller style tag strings should look something like this:
		// Sdef:
		//   "{\*\cs46 \additive \super \sbasedon10 \ssemihidden footnote reference;}"
		// Indoc:
		//   "\cs46\super "	// immediately preceeds \chftn which generates the automatic
		//						// footnote number/symbol
	}
	else if (pSfm->styleType == footnote_text) // special treatment of \f marker
	{
		// See paragraph styleType above for notes
		Sdef.Empty();
		Sindoc.Empty();
		tempSdef.Empty();
		tempSdef = _T("\\s") + numberStr;
		AddAnyParaAlignment(tempSdef,pSfm);
		wxString save_ri_N_value;
		wxString save_li_N_value;
		AddAnyParaIndents(tempSdef,pSfm,save_ri_N_value,save_li_N_value);
		AddAnyParaSpacing(tempSdef,pSfm);
		AddAnyParaKeeps(tempSdef,pSfm);
		tempSdef += _T("\\widctlpar");
		tempSdef += Sltr_precedence;
		tempSdef += _T("\\nooverflow");
		AddAnyRinLin(tempSdef,save_ri_N_value, save_li_N_value);
		tempSdef += _T("\\itap0 "); // needs the following space
		AddAnyCharEnhancements(tempSdef,pSfm);
		tempSdef += outputFontStr; // already has backslash
		AddAnyFontSizeColor(tempSdef,pSfm, colorTblIndxStr);
		// with tempSdef up to this point we can create the style definition Sdef string parameter
		// Note: we will use the currently built tempSdef string in construction of tempSindoc below.
		// We initially prefix the pSfm->marker to the beginning of the Sdef string. A later call to
		// SortAndResolveStyleIndexRefs() will remove this initial marker prefix after resolving the sbasedonN
		// and snextN references
		Sdef = spacePaddedMarker;
		Sdef += _T('{') + tempSdef;
		AddAnyBasedonNext(Sdef,pSfm);
		Sdef += _T("\\ssemihidden");
		AddAnyStylenameColon(Sdef,pSfm);
		Sdef += _T('}');

		// now build the tempSindoc and return in Sindoc. The Sindoc for _footnote_text is irregular
		// and refers to the Sindoc of _footnote_caller. Since _footnote_caller may not have
		// been built yet, we won't know the style number for it, so we only build the regular part
		// of it here and add the irregular tags in AdjustIrregularTagsInArrayStrings().
		tempSindoc += tempSdef; // tempSdef was built above
		Sindoc = spacePaddedMarker;
		Sindoc += tempSindoc;

		// footnote_text style tag strings should look something like this:
		// Sdef:
		//  "{\s49\qj \li0\ri0\widctlpar\rtlpar\nooverflow\rin0\lin0\itap0 \f2\fs16 "
		//  "\sbasedon17 \snext49 \ssemihidden footnote text;}"
		// Sindoc:
		// Because of the way footnotes are generated in RTF as destinations more tags need to be
		// added to the Sindoc (these are not added here but in ProcessIrregularTagsInArrayStrings():
		//  "{\cs46\up2\super \chftn  {\footnote \pard\plain " // this part added in ProcessIrregularTagsInArrayStrings()
		//  "\s49\qj \li0\ri0\widctlpar\rtlpar\nooverflow\rin0\lin0\itap0 \f2\fs16 " // this middle part built here
		//  "{\cs46\up2\super \chftn }{"   // this part added in ProcessIrregularTagsInArrayStrings()
		// Actual footnote text follows this open brace, then \f* and/or \fe [PNG] close the
		// actual footnote text with three braces }}}
	}
	else if (pSfm->styleType == default_para_font) // special treatment of \_dft_para_font marker
	{
		Sdef.Empty();
		Sindoc.Empty();
		// start with an empty tempSdef
		tempSdef.Empty();
		//
		// The Default Paragraph Font always takes the font characteristics of the underlying
		// paragraph, hence any character bold, italic, underline, smallcaps, and other font
		// related values are ignored.
		//
		// Build the complete Sdef strings
		// We initially prefix the marker with space padding to the beginning of the string.
		// A later call to SortAndResolveStyleIndexRefs() will remove this initial marker and
		// space padding prefix.
		Sdef = spacePaddedMarker;
		// The style definition must begin with \* prefixed to the style number tag.
		Sdef += _T("{\\*\\cs") + numberStr;
		Sdef += _T(' '); // add space
		Sdef += _T("\\additive"); // allways use the additive tag on character style definitions
		Sdef += _T(' '); // add space
		// The Default Paragraph Font has the \ssemihidden attribute
		Sdef += _T("\\ssemihidden");
		// add the actual style name which is copied from pSfm's navigationText attribute
		AddAnyStylenameColon(Sdef,pSfm);
		// finally add the closing curly brace that completes the RTF style definition
		Sdef += _T('}');

		// Now build a dummy Sindoc string which functions to keep the Sindoc array in parallel
		// with the Sdef array; the Sindoc is not used in RTF formatting.
		Sindoc = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the first character (\) of Sindoc.
		Sindoc += _T("\\cs") + numberStr; // Sindoc doesn't have prefixed \*
		//Sindoc += tempSdef;	// we don't use tempSdef here but since tempSdef ends with a space
		Sindoc += _T(' ');		// we need to add a space so the renumbering parsing will go well

		// The default_para_font style tag strings should look something like this:
		// Sdef:
		//"{\*\cs10 \additive \ssemihidden Default Paragraph Font;}"
		// Used only as base of other styles - No indoc or key needed for RTF but we construct
		// a dummy Sindoc to keep arrays in parallel as follows:
		// Sindoc:
		// {\cs10 "
	}
	else if (pSfm->styleType == footerSty) // special treatment for _footer marker
	{
		Sdef.Empty();
		Sindoc.Empty();
		// start with an empty tempSdef
		tempSdef.Empty();
		// add the special footerSty tags which simply add a centered and right tab to the footer
		// The tab positions are hard coded here, but can be adjusted by the user later after import
		// of the RTF file.

		tempSdef = _T("\\s") + numberStr;
		AddAnyParaAlignment(tempSdef,pSfm);
		wxString save_ri_N_value;
		wxString save_li_N_value;
		AddAnyParaIndents(tempSdef,pSfm,save_ri_N_value,save_li_N_value);
		AddAnyParaSpacing(tempSdef,pSfm);
		AddAnyParaKeeps(tempSdef,pSfm);
		tempSdef += _T("\\widctlpar");
		tempSdef += Sltr_precedence;

		// Word puts the tab tags here so we'll do the same
		tempSdef += _T("\\tqc\\tx4500\\tqr\\tx9000");

		tempSdef += _T("\\nooverflow");
		AddAnyRinLin(tempSdef,save_ri_N_value, save_li_N_value);
		tempSdef += _T("\\itap0 "); // needs the following space
		AddAnyCharEnhancements(tempSdef,pSfm);
		tempSdef += outputFontStr; // already has backslash
		AddAnyFontSizeColor(tempSdef,pSfm, colorTblIndxStr);
		// with tempSdef up to this point we can create the style definition Sdef string parameter
		// Note: we will use the currently built tempSdef string in construction of tempSindoc below.

		Sdef = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the opening { of Sdef which is added below.
		Sdef += _T('{') + tempSdef; // style definitions (Sdef) are enclosed in curly braces. Add opening brace
		AddAnyBasedonNext(Sdef,pSfm);
		AddAnyStylenameColon(Sdef,pSfm);
		Sdef += _T('}');

		// now build the tempSindoc and return in Sindoc. This utilizes the last form of tempSdef which
		// was built above
		//tempSindoc = _T("\\par \n\\pard\\plain "); // extra paragraph not needed
		tempSindoc = tempSdef; // tempSdef was built above
		Sindoc = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the first character (\) of Sindoc.
		// The in-document tags are complete so return the string in Sindoc
		Sindoc += tempSindoc;

		// The footerSty style tag strings should look something like this:
		// Sdef:
		//  "{\s29\qj \li0\ri0\widctlpar\rtlpar\tqc\tx4500\tqr\tx9000\nooverflow\rin0\lin0\itap0 \f2\fs20 "
		//  "\sbasedon17 \snext29 footer;}"
		// Sindoc:
		//  "\par \n\pard\plain \s29\qj \li0\ri0\widctlpar\rtlpar\tqc\tx4500\tqr\tx9000\nooverflow\rin0\lin0\itap0 \f2\fs20 "
	}
	else if (pSfm->styleType == headerSty)
	{
		// The headerSty is constructed similarly to footerSty but has different tab set values
		Sdef.Empty();
		Sindoc.Empty();
		// start with an empty tempSdef
		tempSdef.Empty();
		// add the special headerSty tags which simply add a centered and right tab to the header
		// The tab positions are hard coded here, but can be adjusted by the user later after import
		// of the RTF file.

		tempSdef = _T("\\s") + numberStr;
		AddAnyParaAlignment(tempSdef,pSfm);
		wxString save_ri_N_value;
		wxString save_li_N_value;
		AddAnyParaIndents(tempSdef,pSfm,save_ri_N_value,save_li_N_value);
		AddAnyParaSpacing(tempSdef,pSfm);
		AddAnyParaKeeps(tempSdef,pSfm);
		tempSdef += _T("\\widctlpar");
		tempSdef += Sltr_precedence; // _T("\\ltrpar");

		// Word puts the tab tags here so we'll do the same
		tempSdef += _T("\\tqc\\tx4320\\tqr\\tx8640\\tqr\\tx8700");
		// the extra tab stop at 8700 corrects header problem in DoExportSrcOrTgtRTF in which
		// \headerr gets an extra tab stop. The one at 8700 prevents the page number from
		// extending into the margin.

		tempSdef += _T("\\nooverflow");
		AddAnyRinLin(tempSdef,save_ri_N_value, save_li_N_value);
		tempSdef += _T("\\itap0 "); // needs the following space
		AddAnyCharEnhancements(tempSdef,pSfm);
		tempSdef += outputFontStr; // already has backslash
		AddAnyFontSizeColor(tempSdef,pSfm, colorTblIndxStr);
		// with tempSdef up to this point we can create the style definition Sdef string parameter
		// Note: we will use the currently built tempSdef string in construction of tempSindoc below.

		Sdef = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the opening { of Sdef which is added below.
		Sdef += _T('{') + tempSdef; // style definitions (Sdef) are enclosed in curly braces. Add opening brace
		AddAnyBasedonNext(Sdef,pSfm);
		AddAnyStylenameColon(Sdef,pSfm);

		Sdef += _T('}');



		// now build the tempSindoc and return in Sindoc. This utilizes the last form of tempSdef which
		// was built above
		//tempSindoc = _T("\\par \n\\pard\\plain "); // extra paragraph not needed
		tempSindoc = tempSdef; // tempSdef was built above
		Sindoc = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the first character (\) of Sindoc.
		// The in-document tags are complete so return the string in Sindoc
		Sindoc += tempSindoc;

		// The headerSty style tag strings should look something like this:
		// Sdef:
		//  "{\s31\qj \li0\ri0\widctlpar\rtlpar\tqc\tx4320\tqr\tx8640\nooverflow\rin0\lin0\itap0 \f2\fs20 "
		//  "\sbasedon17 \snext31 header;}"
		// Sindoc:
		//  "\par \n\pard\plain \s31\qj \li0\ri0\widctlpar\rtlpar\tqc\tx4320\tqr\tx8640\nooverflow\rin0\lin0\itap0 \f2\fs20 "
	}
	else if (pSfm->styleType == horiz_rule)
	{
		// The horiz_rule is constructed similarly to paragraph style but has additional border tags added
		Sdef.Empty();
		Sindoc.Empty();
		// start with an empty tempSdef
		tempSdef.Empty();

		tempSdef = _T("\\s") + numberStr;
		AddAnyParaAlignment(tempSdef,pSfm);
		wxString save_ri_N_value;
		wxString save_li_N_value;
		AddAnyParaIndents(tempSdef,pSfm,save_ri_N_value,save_li_N_value);
		AddAnyParaSpacing(tempSdef,pSfm);
		AddAnyParaKeeps(tempSdef,pSfm);
		tempSdef += _T("\\widctlpar");
		tempSdef += Sltr_precedence;

		// Word puts the border/line tags here so we'll do the same
		tempSdef += _T("\\brdrt\\brdrs\\brdrw15\\brsp20 "); // includes ending space

		tempSdef += _T("\\nooverflow");
		AddAnyRinLin(tempSdef,save_ri_N_value, save_li_N_value);

		// Word also puts a gutter control tag here so we'll do the same
		tempSdef += _T("\\rtlgutter");

		tempSdef += _T("\\itap0 "); // needs the following space
		AddAnyCharEnhancements(tempSdef,pSfm);
		// Note: Word does not add font \fN to the horiz_rule
		//tempSdef += outputFontStr; // already has backslash
		AddAnyFontSizeColor(tempSdef,pSfm, colorTblIndxStr);
		// with tempSdef up to this point we can create the style definition Sdef string parameter
		// Note: we will use the currently built tempSdef string in construction of tempSindoc below.

		Sdef = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the opening { of Sdef which is added below.
		Sdef += _T('{') + tempSdef; // style definitions (Sdef) are enclosed in curly braces. Add opening brace
		AddAnyBasedonNext(Sdef,pSfm); // horiz_rule has no \sbasedon but does have \snext
		AddAnyStylenameColon(Sdef,pSfm);
		Sdef += _T('}');

		// now build the tempSindoc and return in Sindoc. This utilizes the last form of tempSdef which
		// was built above
		tempSindoc = _T("\\par ")+gpApp->m_eolStr+_T("\\pard\\plain ");
		tempSindoc += tempSdef; // tempSdef was built above
		Sindoc = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the first character (\) of Sindoc.
		// The in-document tags are complete so return the string in Sindoc
		Sindoc += tempSindoc;

		// The footerSty style tag strings should look something like this:
		// Sdef:
		//  "{\s34\qc \li0\ri0\sb240\widctlpar\rtlpar\brdrt\brdrs\brdrw15\brsp20 \nooverflow\rin0\lin0\itap0 \fs8 "
		//  "\snext34 Horizontal rule;}"
		// Sindoc:
		//  "\par \n\pard\plain \s34\qc \li0\ri0\sb240\widctlpar\rtlpar\brdrt\brdrs\brdrw15\brsp20 \nooverflow\rin0\lin0\itap0 \fs8 "
	}
	else if (pSfm->styleType == boxed_para)
	{
		// The boxed_para style is similar in construction to horiz_rule, headerSty and footerSty above
		// but has more border/line related tags. boxed_para constructs two related styles the
		// _single_boxed_para and the _double_boxed_para.

		Sdef.Empty();
		Sindoc.Empty();
		// start with an empty tempSdef
		tempSdef.Empty();

		tempSdef = _T("\\s") + numberStr;
		AddAnyParaAlignment(tempSdef,pSfm);
		wxString save_ri_N_value;
		wxString save_li_N_value;
		AddAnyParaIndents(tempSdef,pSfm,save_ri_N_value,save_li_N_value);
		AddAnyParaSpacing(tempSdef,pSfm);
		AddAnyParaKeeps(tempSdef,pSfm);
		tempSdef += _T("\\widctlpar");
		tempSdef += Sltr_precedence;

		// Word puts the extra border/line tags here so we'll do the same
		// Here are the tag meanings:
		// \brdrt = border top
		// \brdrb = border bottom
		// \brdrl = border left
		// \brdrr = border right
		// \brdrdb = double border - the significant difference here
		// \brdrwN = border thickness where N is in twips
		// \brdspN = space between borders and the paragraph where N is in twips
		if (pSfm->marker == _T("_single_boxed_para"))
		{
			// tags for single border - doesn't have the \\brdrdb tag
			tempSdef += _T("\\brdrt\\brdrs\\brdrw15\\brsp20 \\brdrl\\brdrs\\brdrw15\\brsp20 ");
			tempSdef += _T("\\brdrb\\brdrs\\brdrw15\\brsp20 \\brdrr\\brdrs\\brdrw15 \\brsp20 "); // includes ending space
		}
		else
		{
			// tags for double border add the \brdrdb tag
			tempSdef += _T("\\brdrt\\brdrdb\\brdrw15\\brsp20 \\brdrl\\brdrdb\\brdrw15\\brsp20 ");
			tempSdef += _T("\\brdrb\\brdrdb\\brdrw15\\brsp20 \\brdrr\\brdrdb\\brdrw15 \\brsp20 "); // includes ending space
		}

		tempSdef += _T("\\nooverflow");
		AddAnyRinLin(tempSdef,save_ri_N_value, save_li_N_value);

		// Word also puts a gutter control tag here so we'll do the same
		tempSdef += _T("\\rtlgutter");

		tempSdef += _T("\\itap0 "); // needs the following space
		AddAnyCharEnhancements(tempSdef,pSfm); // \b should be added
		// Note: Word does not add font \fN to the boxed_para style
		//tempSdef += outputFontStr; // already has backslash
		AddAnyFontSizeColor(tempSdef,pSfm, colorTblIndxStr);
		// with tempSdef up to this point we can create the style definition Sdef string parameter
		// Note: we will use the currently built tempSdef string in construction of tempSindoc below.

		Sdef = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the opening { of Sdef which is added below.
		Sdef += _T('{') + tempSdef; // style definitions (Sdef) are enclosed in curly braces. Add opening brace
		AddAnyBasedonNext(Sdef,pSfm); // boxed_para has both \sbasedonN and \snextN
		AddAnyStylenameColon(Sdef,pSfm);
		Sdef += _T('}');

		// boxed_para does not require Sindoc but we create a dummy one to keep arrays in sync
		tempSindoc = _T("\\par ")+gpApp->m_eolStr+_T("\\pard\\plain ");
		tempSindoc += tempSdef; // tempSdef was built above
		Sindoc = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the first character (\) of Sindoc.
		// The in-document tags are complete so return the string in Sindoc
		Sindoc += tempSindoc;

		// The boxed_para style tag strings should look something like this:
		// Sdef:
		//  "{\s57\qc \li360\ri389\widctlpar\brdrt\brdrdb\brdrw15\brsp20 \brdrl"
		//	"\brdrdb\brdrw15\brsp20 \brdrb\brdrdb\brdrw15\brsp20 \brdrr\brdrdb\brdrw15 \brsp20 "
		//	"\\nooverflow\\rin389\\lin360\\rtlgutter\\itap0 \\b\\fs22\\sbasedon0 \\snext57 Double Boxed Paragraph;}"
		// Sindoc:
		//  "\par \n\pard\plain \s57\qc \li360\ri389\widctlpar\brdrt\brdrdb\brdrw15\brsp20 \brdrl"
		//	"\brdrdb\brdrw15\brsp20 \brdrb\brdrdb\brdrw15\brsp20 \brdrr\brdrdb\brdrw15 \brsp20 "
		//	"\\nooverflow\\rin389\\lin360\\rtlgutter\\itap0 \\b\\fs22 "
		//  Note: The Single Boxed Paragraph style will look similar but won't have the \bdrdb tags
	}
	else if (pSfm->styleType == hidden_note)
	{
		// The hidden_note style just adds the \v hidden RTF tag to the regular paragraph style tag
		// string.
		Sdef.Empty();
		Sindoc.Empty();
		tempSdef.Empty();
		tempSdef = _T("\\s") + numberStr;
		AddAnyParaAlignment(tempSdef,pSfm);
		wxString save_ri_N_value;
		wxString save_li_N_value;
		AddAnyParaIndents(tempSdef,pSfm,save_ri_N_value,save_li_N_value);
		AddAnyParaSpacing(tempSdef,pSfm);
		AddAnyParaKeeps(tempSdef,pSfm);
		tempSdef += _T("\\widctlpar");
		tempSdef += Sltr_precedence;
		tempSdef += _T("\\nooverflow");
		AddAnyRinLin(tempSdef,save_ri_N_value, save_li_N_value);
		tempSdef += _T("\\itap0 "); // needs the following space
		AddAnyCharEnhancements(tempSdef,pSfm);

		// Word adds the hidden attribute right after the char enhancements so we'll do the same
		tempSdef += _T("\\v");

		tempSdef += outputFontStr; // already has backslash
		AddAnyFontSizeColor(tempSdef,pSfm, colorTblIndxStr);
		// with tempSdef up to this point we can create the style definition Sdef string parameter
		// Note: we will use the currently built tempSdef string in construction of tempSindoc below.

		Sdef = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the opening { of Sdef which is added below.

		Sdef += _T('{') + tempSdef; // style definitions (Sdef) are enclosed in curly braces. Add opening brace
		AddAnyBasedonNext(Sdef,pSfm);
		AddAnyStylenameColon(Sdef,pSfm);
		Sdef += _T('}');

		// now build the tempSindoc and return in Sindoc. This utilizes the last form of tempSdef which
		// was built above
		tempSindoc = _T("\\par ")+gpApp->m_eolStr+_T("\\pard\\plain ");
		tempSindoc += tempSdef; // tempSdef was built above
		Sindoc = spacePaddedMarker;	// this temporarily puts the marker with space padding suffixed to it
									// before the first character (\) of Sindoc.
		// The in-document tags are complete so return the string in Sindoc
		Sindoc += tempSindoc;

		// The hidden_note style tag strings should look something like this:
		// Sdef:
		//  "{\s59\qj \fi216\li0\ri0\sb40\widctlpar\ltrpar\nooverflow\rin0\lin0\itap0 \i\v\f2\fs20\cf9 "
		//  "\\sbasedon18 \\snext59 Hidden Note;}"
		// Sindoc:
		//  "\par \n\pard\plain \s59\qj \fi216\li0\ri0\sb40\widctlpar\ltrpar\nooverflow\rin0\lin0\itap0 \i\v\f2\fs20\cf9 "
	}
	else // it is an unknown styleType
	{
		::wxBell();
		wxASSERT(FALSE);
		// If this assert trips in Debug, the AI_USFM_full.xml file is defining a styleType
		// that is unknown and, if not a typo in the xml file, but is an intended addition to
		// the enum styleType, it needs to be added to the else if condition checks above.
		// and similar adjustments made in the enum styleType and the parsing routines in
		// XML.cpp.
	}
}

// BEW 10Apr10 no changes for doc version 5
void SortAndResolveStyleIndexRefs(wxArrayString& StyleDefStrArray,
											wxArrayString& StyleInDocStrArray)
{
	wxString tempSdefStr,tempSindocStr;
	int ct,sDefStartPos,sIndocStartPos,sDefEndPos,sIndocEndPos;
	//wxChar buf[34];
	//wxChar* p;
	wxASSERT(StyleDefStrArray.GetCount() == StyleInDocStrArray.GetCount()); // must be
	int tot = StyleDefStrArray.GetCount();

	// Note: First sort the strings in the arrays so they will appear in alphabetical order
	// within the RTF styletable, with the base styles which are prefixed with an underscore
	// (two underscores for "Normal" style) sorted first.
	// Sort() does an ascending alphabetical sort by default
	StyleDefStrArray.Sort(); //std::sort(StyleDefStrArray.GetData(),StyleDefStrArray.GetData() + StyleDefStrArray.GetSize());
	StyleInDocStrArray.Sort(); //std::sort(StyleInDocStrArray.GetData(),StyleInDocStrArray.GetData() + StyleInDocStrArray.GetSize());

	 //At this point the two arrays should be sorted and have the same relative sorting with
	 //array indices for their markers in the same relative positions.

	// Now go through the strings and renumber the styles, i.e., renumber the N of \sN and \csN.
	// We also will pattern our renumbering on observations of Word's RTF export in which the
	// style table has "Normal" style as style zero (and \s0 is ommitted from the tag string for
	// Normal style only), and the succeeding styles start numbering at a value of 10, skipping
	// style numbers 1 through 9.

	// First renumber the styles
	int sNumInt;
	wxString sNumStr,sDefNumStrOld,sIndocNumStrOld, bufStr;
	for (ct = 0; ct < tot; ct++)
	{
		tempSdefStr = StyleDefStrArray.Item(ct);
		tempSindocStr = StyleInDocStrArray.Item(ct);
		// Get the startPos and endPos of the old number using GetStyleNumberStrFromRTFTagStr().
		// The style number N is the number following {\s or {\*\cs in the string
		// We'll use startPos and endPos below, but disregard the returned old number string
		// which we delete below from the string before assigning a new number.
		sDefNumStrOld = GetStyleNumberStrFromRTFTagStr(tempSdefStr, sDefStartPos, sDefEndPos);
		sIndocNumStrOld = GetStyleNumberStrFromRTFTagStr(tempSindocStr, sIndocStartPos, sIndocEndPos);
		wxASSERT(sDefNumStrOld == sIndocNumStrOld);

		// delete the old N
		tempSdefStr.Remove(sDefStartPos,sDefEndPos - sDefStartPos);
		tempSindocStr.Remove(sIndocStartPos,sIndocEndPos - sIndocStartPos);
		if (ct > 0)
		{
			sNumInt = ct + 9; // skip numbers 1 through 9
		}
		else
		{
			sNumInt = ct;
		}
		sNumStr.Empty();
		sNumStr << sNumInt;
		// add the new N in the existing strings at the same start positions where the old N was deleted
		tempSdefStr = InsertInString(tempSdefStr, sDefStartPos, sNumStr);
		tempSindocStr = InsertInString(tempSindocStr, sIndocStartPos, sNumStr);

		// update the string in StyleDefStrArray
		StyleDefStrArray[ct] = tempSdefStr;
		StyleInDocStrArray[ct] = tempSindocStr;
	}

	// Now scan through the strings in the array and resolve the style references. These are
	// currently indicated suffixed to the pertinent tag words enclosed by square brackets,
	// i.e., \snext[pi]. In this example we need to replace [pi] with the style number N that
	// the pi marker itself is associated with. If pi is the style represented as \s135, then
	// the \snext[pi] needs to be resolved to \snext135 and so on. The only tag words that need
	// to be resolved are \sbasedonN and \snextN. Both can and usually are found in paragraph
	// styles, whereas only \sbasedonN is found in some of the character styles. The N may be
	// on a style which is either before the current style sequence number or after it, so we'll
	// need to scan the whole array of strings until we locate the appropriate referenced marker.
	// Once found, we cannot simply use the array index as our N value because, following Word's
	// RTF output behavior, we skipped some of the style numbers (1-9). We could just add 9 to
	// the index to get N, but I think it would be safer to parse out the actual N used in \sN
	// or \*\csN and use that value. Remember each string currently has the marker prefixed to
	// the string followed by spaces padding the markers out to the length of the longest marker
	// in the sfm set. This was done purposely for the sorting operation, and also makes it
	// easier to search for the appropriate array string that resolves the N values for the
	// \snextN and \sbasedonN keywords.
	// Note: \sbasedonN and \snextN do not occur in the Indoc forms so we need only resolve the
	// references in the sDef forms.

	wxString bareMkr,styleNumberStr;
	int dummyIndx; // used as dummy return parameter for GetStyleNumberStrAssociatedWithMarker() below
	for (ct = 0; ct < tot; ct++)
	{
		tempSdefStr = StyleDefStrArray.Item(ct);

		// resolve any \sbasedon reference
		sDefStartPos = tempSdefStr.Find(_T("\\sbasedon"));
		if (sDefStartPos != -1)
		{
			// we have an \sbasedon attribute
			sDefStartPos = sDefStartPos + 9; // point at the next char after \sbasedon which should be [
			wxASSERT(tempSdefStr[sDefStartPos] == _T('['));
			// now scan to the right with endPos until we encounter the closing ]
			sDefEndPos = sDefStartPos;
			while (sDefEndPos < (int)tempSdefStr.Length() && tempSdefStr[sDefEndPos] != _T(']'))
			{
				sDefEndPos++;
			}
			// extract the marker that resides within the square brackets
			bareMkr = tempSdefStr.Mid(sDefStartPos +1); // gets the marker name and the remainder of the string
			bareMkr = bareMkr.Left(bareMkr.Find(_T(']')));	// extract out the marker name part at
															// the beginning before closing ]
			wxASSERT(!bareMkr.IsEmpty());
			// now we will scan the array to find the string that has bareMkr prefixed to it
			styleNumberStr = GetStyleNumberStrAssociatedWithMarker(bareMkr,StyleDefStrArray, dummyIndx);
			// delete the old [...] reference following \sbasedon
			tempSdefStr.Remove(sDefStartPos,sDefEndPos + 1 - sDefStartPos);
			// insert the new style number reference in its place
			tempSdefStr = InsertInString(tempSdefStr,sDefStartPos, styleNumberStr);
			// update the string in StyleDefStrArray
			StyleDefStrArray[ct] = tempSdefStr; //StyleDefStrArray.SetAt(ct,tempSdefStr);
		}
		// now resolve any \snext reference
		sDefStartPos = tempSdefStr.Find(_T("\\snext"));
		if (sDefStartPos != -1)
		{
			// we have an \snext attribute
			sDefStartPos = sDefStartPos + 6; // point at the next char after \snext which should be [
			wxASSERT(tempSdefStr[sDefStartPos] == _T('['));
			// now scan to the right with endPos until we encounter the closing ]
			sDefEndPos = sDefStartPos;
			while (sDefEndPos < (int)tempSdefStr.Length() && tempSdefStr[sDefEndPos] != _T(']'))
			{
				sDefEndPos++;
			}
			// extract the marker that resides within the square brackets
			bareMkr = tempSdefStr.Mid(sDefStartPos +1); // gets the marker name and the remainder of the string
			bareMkr = bareMkr.Left(bareMkr.Find(_T(']')));	// extract out the marker name part at
															// the beginning before closing ]
			wxASSERT(!bareMkr.IsEmpty());
			// now we will scan the array to find the string that has bareMkr prefixed to it
			styleNumberStr = GetStyleNumberStrAssociatedWithMarker(bareMkr,StyleDefStrArray,dummyIndx);
			// delete the old [...] reference following \snext
			tempSdefStr.Remove(sDefStartPos,sDefEndPos + 1 - sDefStartPos);
			// insert the new style number reference in its place
			tempSdefStr = InsertInString(tempSdefStr, sDefStartPos, styleNumberStr);
			// update the string in StyleDefStrArray
			StyleDefStrArray[ct]= tempSdefStr;
		}
	}
}

// BEW 10Apr10 no changes for doc version 5
void ProcessIrregularTagsInArrayStrings(wxArrayString& StyleDefStrArray,wxArrayString& StyleInDocStrArray)
{
	wxString tempSdefStr,tempSindocStr;
	wxString bareMkr, styleNumberStr;
	int sDefStartPos, sIndocStartPos;
	int sDefEndPos, sIndocEndPos;

	// ************************* Normal style RTF tag modifications ************************
	// First deal with the special Normal style which is logically style zero (\s0) but Word's RTF
	// export omits the explicit tag in the style definition sDef and the in-document sIndoc forms
	int aCount;
	int aTot = StyleDefStrArray.GetCount();
	for (aCount = 0; aCount < aTot; aCount++)
	{
		tempSdefStr = StyleDefStrArray.Item(aCount); // this should be the "Normal" style tag string
		tempSindocStr = StyleInDocStrArray.Item(aCount); // " " " for the Indoc string
		if (tempSdefStr.Find(_T(" Normal;")) != -1)
			break;
	}
	wxASSERT(tempSdefStr.Find(_T(" Normal;")) != -1);
	sDefStartPos = tempSdefStr.Find(_T("{\\s")); // there will be an opening { in the style definition
	sIndocStartPos = tempSindocStr.Find(_T("\\s")); // there won't be an opening { next to \s in in_doc tag string
	wxASSERT(sDefStartPos != -1);
	wxASSERT(sIndocStartPos != -1);
	sDefEndPos = sDefStartPos + 2; // point at the s in {\s
	sIndocEndPos = sIndocStartPos + 1; // point at the s in \s
	// scan toward the right end of the strings until we come to the next \ tag (usually \qj)
	while (sDefEndPos < (int)tempSdefStr.Length() && tempSdefStr[sDefEndPos] != _T('\\'))
	{
		sDefEndPos++;
	}
	while (sIndocEndPos < (int)tempSindocStr.Length() && tempSindocStr[sIndocEndPos] != _T('\\'))
	{
		sIndocEndPos++;
	}
	// sDefEndPos and sIndocEndPos should both point to the last char of the existing number N of \sN
	sDefStartPos++; // point past opening brace {
	tempSdefStr.Remove(sDefStartPos,sDefEndPos - sDefStartPos);
	tempSindocStr.Remove(sIndocStartPos,sIndocEndPos - sIndocStartPos);
	// update the string in StyleDefStrArray and StyleInDocStrArray
	StyleDefStrArray[0] = tempSdefStr;
	StyleInDocStrArray[0] = tempSindocStr;

	// Note: Other special handling of unique style tags could be done here if desired.
}

////////////// The following are helper functions for BuildRTFStyleTagString() ///////
// Some of these functions are also used in DoExportInterLinearRTF()
void AddAnyParaAlignment(wxString& tempStr, USFMAnalysis* pSfm)
{
	if (pSfm == NULL)
	{
		wxASSERT(FALSE); // Something's wrong with xml attribute !!!
		return;
	}
	// add the alignment paragraph tag based on pSfm's justification attribute
	if (pSfm->justification == leading)
		tempStr += _T("\\ql ");
	else if (pSfm->justification == following)
		tempStr += _T("\\qr ");
	else if (pSfm->justification == center)
		tempStr += _T("\\qc ");
	else
		tempStr += _T("\\qj "); // justified justification
}

void AddAnyParaIndents(wxString& tempStr, USFMAnalysis* pSfm, wxString& save_ri_N_value, wxString& save_li_N_value)
{
	if (pSfm == NULL)
	{
		wxASSERT(FALSE); // Something's wrong with xml attribute !!!
		return;
	}
	// add any first line indent, but only needed if non-zero
	if (pSfm->firstLineIndent != 0)
	{
		tempStr << _T("\\fi");
		tempStr <<  (int)(pSfm->firstLineIndent*1440);
	}
	// add the left indent value
	tempStr << _T("\\li");
	save_li_N_value.empty();
	save_li_N_value << (int)(pSfm->leadingMargin*1440); // also used in \linN below
	tempStr << save_li_N_value;
	// add the right indent value
	tempStr << _T("\\ri");
	save_ri_N_value.empty();
	save_ri_N_value << (int)(pSfm->followingMargin*1440); // also used in \rinN below
	tempStr << save_ri_N_value;
}

void AddAnyParaSpacing(wxString& tempStr, USFMAnalysis* pSfm)
{
	if (pSfm == NULL)
	{
		wxASSERT(FALSE); // Something's wrong with xml attribute !!!
		return;
	}
	// add any space before/space above
	if (pSfm->spaceAbove != 0)
	{
		tempStr << _T("\\sb");
		tempStr << pSfm->spaceAbove*20; // spaceAbove is in points and there are 20 twips per point
	}
	// add any space below/space below
	if (pSfm->spaceBelow != 0)
	{
		tempStr << _T("\\sa");
		tempStr << pSfm->spaceBelow*20; // spaceBelow is in points and there are 20 twips per point
	}
}

void AddAnyParaKeeps(wxString& tempStr, USFMAnalysis* pSfm)
{
	if (pSfm == NULL)
	{
		wxASSERT(FALSE); // Something's wrong with xml attribute !!!
		return;
	}
	// add paragraph keep style attributes
	if (pSfm->keepTogether)
	{
		tempStr += _T("\\keep");
	}
	if (pSfm->keepWithNext)
	{
		tempStr += _T("\\keepn");
	}
}

void AddAnyRinLin(wxString& tempStr, wxString save_ri_N_value, wxString save_li_N_value)
{
	// always add the \rinN value (this seems to override \riN to make it work right for RTL)
	if (!save_ri_N_value.IsEmpty())
	{
		tempStr += _T("\\rin");
		tempStr += save_ri_N_value; // \rinN uses same N value as \riN
	}
	// always add the \linN value (this seems to override \liN to make it work right for RTL)
	if (!save_li_N_value.IsEmpty())
	{
		tempStr += _T("\\lin");
		tempStr += save_li_N_value; // \linN uses same N value as \liN
	}
}

void AddAnyCharEnhancements(wxString& tempStr, USFMAnalysis* pSfm)
{
	if (pSfm == NULL)
	{
		wxASSERT(FALSE); // Something's wrong with xml attribute !!!
		return;
	}
	// add any character bold, italic, underline, smallcaps, etc
	if (pSfm->bold)
		tempStr += _T("\\b");
	if (pSfm->italic)
		tempStr += _T("\\i");
	if (pSfm->underline)
		tempStr += _T("\\ul");
	if (pSfm->smallCaps)
		tempStr += _T("\\scaps");
}

void AddAnyFontSizeColor(wxString& tempStr, USFMAnalysis* pSfm, wxString colorTblIndxStr)
{
	if (pSfm == NULL)
	{
		wxASSERT(FALSE); // Something's wrong with xml attribute !!!
		return;
	}
	if (pSfm->fontSize != 0)
	{
		// only add \fsN when fontSize is significant (non-zero)
		// If fontSize is not within range of 1 to 72 points, adjust the size
		// to one of those limits
		if (pSfm->fontSize < 1)
			pSfm->fontSize = 1;
		if (pSfm->fontSize > 72)
			pSfm->fontSize = 72;
		tempStr << _T("\\fs");
		tempStr << pSfm->fontSize*2; // for half-points, multiply fontSize by 2
	}
	// add any color attribute from the color table map (via the colorTblIndxStr parameter)
	if (pSfm->color != 0)
	{
		tempStr << colorTblIndxStr;
	}
	if ((pSfm->fontSize != 0 || pSfm->color != 0) && tempStr.Length() > 0 && tempStr[tempStr.Length() -1] != _T(' '))
	{
		// ensure that a space comes after any font size and/or color
		tempStr << _T(' ');
	}
	// with tempSdef up to this point we can create the style definition Sdef string parameter
	// Note: we will use the currently built tempSdef string in construction of tempSindoc.
}

void AddAnyBasedonNext(wxString& tempStr, USFMAnalysis* pSfm)
{
	// add any \sbasedonN tag using pSfm's basedOn attribute
	// Note: We will juxtapose the referenced to marker name within square brackets so that the
	// string at this point becomes \sbasedon[marker name]. Then after all styles are processed
	// an auxiliary function called void SortAndResolveStyleIndexRefs() can process the array of Sdef
	// strings and resolve the marker names within the square brackets appended to \sbasedon to
	// realize the correct Sdef array index value to substitute as a string in the place
	// of [marker name]. The processing of both \sbasedonN and \snextN uses this method of
	// placing the marker name in square brackets, to be resolved later.
	if (pSfm == NULL)
	{
		wxASSERT(FALSE); // Something's wrong with xml attribute !!!
		return;
	}
	if (!pSfm->basedOn.IsEmpty())
	{
		tempStr += _T("\\sbasedon");
		tempStr += _T("["); // this will be removed by SortAndResolveStyleIndexRefs()
		tempStr += pSfm->basedOn;	// the actual index value string will replace the marker
								// name with its actual Sdef array index value (as a string)
		tempStr += _T(']'); // this will be removed by SortAndResolveStyleIndexRefs()
	}
	// add any \snextN tag using pSfm's nextStyle attribute
	// Note: See the note above for \sbasedonN. The same applies here for \snextN
	if (!pSfm->nextStyle.IsEmpty())
	{
		if (!pSfm->basedOn.IsEmpty())
		{
			tempStr += _T(' '); // if \sbasedonN is specified it must be followed by a space
		}
		tempStr += _T("\\snext");
		tempStr += _T('['); // this will be removed by SortAndResolveStyleIndexRefs()
		tempStr += pSfm->nextStyle;	// the actual index value string will replace the marker
									// name with its actual Sdef array index value (as a string)
		tempStr += _T(']'); // this will be removed by SortAndResolveStyleIndexRefs()
	}
	// ensure that a space after comes after any basedOn and/or nextStyle
	if ((!pSfm->basedOn.IsEmpty() || !pSfm->nextStyle.IsEmpty()) && tempStr.Length() > 0 && tempStr[tempStr.Length() -1] != _T(' '))
	{
		tempStr += _T(' ');
	}
}

wxString RemoveFreeTransWordCountFromStr(wxString freeStr)
{
	int beginPos, endPos;
	beginPos = freeStr.Find(_T("|@"));
	endPos = freeStr.Find(_T("@|"));
	wxASSERT(endPos != -1); // closing marks should also be present
	// we expect that the string "@|" exists; that the next char position is within
	// the string; and that next position is a space
	wxString tempStr = freeStr.Left(beginPos);
	if ((int)freeStr.Length() > endPos + 2 && freeStr[endPos + 2] == _T(' '))
	{
		// the free string has a space after the "|@nn@| " so omit the space too
		tempStr += freeStr.Mid(endPos + 3);
	}
	else
	{
		// the free string ends immediately after or has no space after the "|@nn@|"
		// so remove only the "|@nn@|" part
		// this situation should not normally happen even when the free string is empty
		// but is here for safety
		tempStr += freeStr.Mid(endPos + 2);
	}
	return tempStr;
}

wxString GetStyleNumberStrAssociatedWithMarker(wxString bareMkr,
									wxArrayString& StyleDefStrArray, int& indx)
{
	// This function assumes that the marker name (bareMkr) followed by padded spaces is
	// still prefixed to the tag strings in the parallel arrays. This prefix
	// is only temporary and is removed in DoExportSrcOrTgtRTF() after calls to
	// GetStyleNumberStrAssociatedWithMarker() are complete.
	// Note also that the actual array index in int form is returned in the last
	// parameter.
	wxString styleNumStr, tempStr;
	styleNumStr.Empty();
	int sIndex;
	int dummyStartPos, dummyEndPos; // not used in this routine
	bool found = FALSE;
	indx = -1; // return a value of -1 if not found
	int tot = StyleDefStrArray.GetCount();
	for (sIndex = 0; sIndex < tot; sIndex++)
	{
		if (StyleDefStrArray.Item(sIndex).Find(bareMkr) == 0)
		{
			// we found the bareMkr at the beginning of the string
			found = TRUE;
			indx = sIndex; // return the int value in indx
			break;
		}
	}
	if (found)
	{
		// we found the string now parse out the style number associated with it
		// The style number N is the number following {\s or {\*\cs in the string
		tempStr = StyleDefStrArray.Item(sIndex);
		styleNumStr = GetStyleNumberStrFromRTFTagStr(tempStr, dummyStartPos, dummyEndPos);
	}
	else
	{
		// something went wrong and we couldn't determine a style number N, so return "0"
		// which is the "Normal" style number
		// Since this should not happen in a well-formed AI_USFM.xml file, I'll wxASSERT
		// here so the xml inconsistency will be noticed in debugging
		wxASSERT(FALSE);	// if this asserts, note the string in bareMkr above and check
						// the AI_USFM_full.xml file for a typo. Usually a leading underscore
						// will be the problem. After corrections are made regenerate the
						// AI_USFM.xml file, update the copy in the Adapt It (Unicode) Work
						// folder and try again until this assert does not trip.
		styleNumStr = _T("0");
	}
	return styleNumStr;
}

// A useful utility which takes the filtered information (whether notes is to be included
// or not is specifiable) and then in m_markers, then in m_inlineNonbindingMarkers,
// m_precPunct, and m_inlineBindingEndMarkers, in that order, and any of such which is
// non-empty is appended, with delimiting spaces where appropriate (to comply with good
// USFM markup standards) to the passed in appendHere string, which is then returned to the
// caller. Where this is sometimes used, we may have to delay the placements in the
// caller, and so bAddedSomething is returned so the caller knows to temporarily store the
// results for later on placement. bIncludeNote should be set TRUE to have a note included
// in the gathering of filtered info from pSrcPhrase, FALSE to have note information not
// gathered and hence not returned in the string to the caller along with the rest.
// bDoCountForFreeTrans specifies whether or not word counting for free trans |@ nnn @|
// word count is to be done, using m_targetStr or m_srcPhrase as the case may be for next
// param. bCountInTargetTextLine specifies which line to count the words in, and hence
// whether to do it with m_srcPhrase (use FALSE) or m_targetStr (use TRUE).
// Beware, \x ...\x* cross reference information goes after any m_markers content, but
// other filtered information, if present, goes before it; so we must look for xref stuff
// and locate it properly in the string for output.
// BEW created 11Oct10
wxString AppendSrcPhraseBeginningInfo(wxString appendHere, CSourcePhrase* pSrcPhrase, 
					 bool& bAddedSomething, bool bIncludeNote,
					 bool bDoCountForFreeTrans, bool bCountInTargetTextLine)
{
	bAddedSomething = FALSE;
	bAddedSomething = HasFilteredInfo(pSrcPhrase);
	wxString xrefStr; xrefStr.Empty();
	wxString otherFiltered; otherFiltered.Empty();
	wxString temp;
	wxString mMarkers;
	wxString aSpace = _T(' ');

	// In next call, 1st FALSE means 'count words from src text line' (but
	// this is only used if reconstructing a free translation with wrapping
	// \free and \free* markers) internally; TRUE means 'do the word
	// count', final FALSE is for bIncludeNote
	if (bAddedSomething)
	{
		// entry here means there is something filtered to be collected (includes custom
		// ai stuff, such as free trans, note if requested, collected back trans, as well
		// as the standard filtered stuff in m_filteredInfo)
		temp = GetFilteredStuffAsUnfiltered(pSrcPhrase, bDoCountForFreeTrans, 
												bCountInTargetTextLine, bIncludeNote);
		SeparateOutCrossRefInfo(temp, xrefStr, otherFiltered);
	}
	if (!pSrcPhrase->m_markers.IsEmpty())
	{
		mMarkers = pSrcPhrase->m_markers;
		bAddedSomething = TRUE;
	}
	// get the above stuff into proper USFM order, xrefs must go after m_markers content
	if (bAddedSomething)
	{
		if (!xrefStr.IsEmpty())
		{
			if (!otherFiltered.IsEmpty())
			{
				appendHere = otherFiltered;
			}
			appendHere.Trim();
			appendHere += aSpace;
			if (!mMarkers.IsEmpty())
			{
				appendHere += mMarkers;
			}
			appendHere.Trim();
			appendHere += aSpace;
			appendHere += xrefStr;
		}
		else
		{
			// there is no cross reference info
			if (!otherFiltered.IsEmpty())
			{
				appendHere = otherFiltered;
			}
			appendHere.Trim();
			appendHere += aSpace;
			if (!mMarkers.IsEmpty())
			{
				appendHere += mMarkers;
			}
		}
	}

	if (!pSrcPhrase->GetInlineNonbindingMarkers().IsEmpty())
	{
		appendHere += pSrcPhrase->GetInlineNonbindingMarkers();
		bAddedSomething = TRUE;
	}
	if (!pSrcPhrase->m_precPunct.IsEmpty())
	{
		wxString puncts = pSrcPhrase->m_precPunct;
		puncts.Trim(); // ensure no bogus space can follow the preceding puncts
		appendHere += puncts;
		bAddedSomething = TRUE;
	}
	if (!pSrcPhrase->GetInlineBindingMarkers().IsEmpty())
	{
		wxString binders = pSrcPhrase->GetInlineBindingMarkers();
		binders.Trim(FALSE); // ensure no bogus space precedings an 
							 // inline binding beginmarker
		appendHere += binders;
		bAddedSomething = TRUE;
	}
	return appendHere;
}

// A useful utility which takes the information in m_markers m_inlineBindingMarkers,
// m_precPunct, m_endMarkers, m_follOuterPunct, and m_inlineNonbindingEndMarkers, in that
// order, and any of such which is non-empty is appended, with no spaces added anywhere (to
// comply with good USFM markup standards) to the passed in appendHere string, which is
// then returned to the caller.
wxString AppendSrcPhraseEndingInfo(wxString appendHere, CSourcePhrase* pSrcPhrase)
{
	if (!pSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
	{
		appendHere += pSrcPhrase->GetInlineBindingEndMarkers();
	}
	if (!pSrcPhrase->m_follPunct.IsEmpty())
	{
		appendHere += pSrcPhrase->m_follPunct;
	}
	if (!pSrcPhrase->GetEndMarkers().IsEmpty())
	{
		appendHere += pSrcPhrase->GetEndMarkers();
	}
	if (!pSrcPhrase->GetFollowingOuterPunct().IsEmpty())
	{
		appendHere += pSrcPhrase->GetFollowingOuterPunct();
	}
	if (!pSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
	{
		appendHere += pSrcPhrase->GetInlineNonbindingEndMarkers();
	}
	return appendHere;
}

// BEW created 11Oct10
wxString GetUnfilteredCrossRefsAndMMarkers(wxString prefixStr,
	wxString markersStr, wxString xrefStr,
	bool bAttachFilteredInfo, bool bAttach_m_markers)
{
	wxString markersPrefix = prefixStr;
	wxString aSpace = _T(' ');
	if (!markersStr.IsEmpty())
	{
		if (bAttach_m_markers)
		{
			// any content in m_markers is to be in the returned source text string
			if (!markersStr.IsEmpty())
			{
				markersPrefix.Trim();
				markersPrefix += aSpace + markersStr;
			}
			if (!xrefStr.IsEmpty()  && bAttachFilteredInfo)
			{
				// a xref follows a verse number, so make sure there is an intervening
				// space
				markersPrefix.Trim();
				markersPrefix += aSpace + xrefStr;
			}
		}
		else
		{
			if (!xrefStr.IsEmpty() && bAttachFilteredInfo)
			{
				markersPrefix.Trim();
				markersPrefix += aSpace + xrefStr;
			}
		} // end of else block for test: if (bAttach_m_markers)
	}
	else
	{
		// m_markers is empty, so just put in any xref info if there is any
		if (!xrefStr.IsEmpty() && bAttachFilteredInfo)
		{
			markersPrefix.Trim();
			markersPrefix += aSpace + xrefStr;
		}
	}
	return markersPrefix;
}

// BEW created 11Oct10
wxString GetUnfilteredInfoMinusMMarkersAndCrossRefs(CSourcePhrase* pSrcPhrase,
	SPList* pSrcPhrases, wxString filteredInfo_NoXRef, wxString collBackTransStr,
	wxString freeTransStr, wxString noteStr, bool bDoCount, bool bCountInTargetText)
{
	wxString markersPrefix; markersPrefix.Empty();
	wxString aSpace = _T(' ');
	wxString freeMkr(_T("\\free"));
	wxString freeEndMkr = freeMkr + _T("*");
	wxString noteMkr(_T("\\note"));
	wxString noteEndMkr = noteMkr + _T("*");
	wxString backTransMkr(_T("\\bt"));

	if (!filteredInfo_NoXRef.IsEmpty())
	{
		// this data has any markers and endmarkers already 'in place'
		markersPrefix.Trim();
		if (!filteredInfo_NoXRef.IsEmpty())
		{
			markersPrefix = filteredInfo_NoXRef;
		}
	}
	if (!collBackTransStr.IsEmpty())
	{
		// add the marker too
		markersPrefix.Trim();
		markersPrefix += backTransMkr;
		markersPrefix += aSpace + collBackTransStr;
	}
	if (!freeTransStr.IsEmpty() || pSrcPhrase->m_bStartFreeTrans)
	{
		markersPrefix.Trim();
		markersPrefix += aSpace + freeMkr;

		if (bDoCount)
		{
			// BEW addition 06Oct05; a \free .... \free* section pertains to a certain
			// number of consecutive sourcephrases starting at this one if m_markers
			// contains the \free marker, but the knowledge of how many sourcephrases is
			// marked in the latter instances by which ones have the m_bStartFreeTrans ==
			// TRUE and m_bEndFreeTrans == TRUE, and if we just export the filtered free
			// translation content we will lose all information about its extent in the
			// document. So we have to compute how many target words are involved in the
			// section, and store that count in the exported file -- and the obvious place
			// to do it is after the \free marker and its following space. We will store it
			// as follows: |@nnnn@|<space> so that we can search for the number and find it
			// quickly and remove it if we later import the exported file into a project as
			// source text.
			// (Note: the following call has to do its word counting in the SPList, because
			// only there is the filtered information, if any, still hidden and therefore
			// unable to mess up the word count.)
			int nWordCount = CountWordsInFreeTranslationSection(bCountInTargetText,pSrcPhrases,
					pSrcPhrase->m_nSequNumber); // TRUE means 'count in tgt text'
			// construct an easily findable unique string containing the number
			wxString entry = _T("|@");
			entry << nWordCount; // converts int to string automatically
			entry << _T("@| ");
			// append it after a delimiting space
			markersPrefix += aSpace + entry;
		}
		if (freeTransStr.IsEmpty())
		{
			// we must support empty free translation sections
			markersPrefix += aSpace + freeEndMkr;
		}
		else
		{
			// now the free translation string itself & endmarker
			markersPrefix += aSpace + freeTransStr;
			markersPrefix += freeEndMkr; // don't need space too
		}
	}
	// notes being after free trans means that OXES parsing is easier, as all notes -
	// including one at the free translation anchor point, then become 'embedded' in the
	// sacred text - though the one at the anchor point is 'embedded' at the start of the
	// sacred  text, it's not stretching things to far to consider it embedded like any
	// others in that section of text
	if (!noteStr.IsEmpty() || pSrcPhrase->m_bHasNote)
	{
/* 
		// BEW removed 15Jun11, because OXES's status is unclear, so we'll not support it 
		// until it is needed
		// BEW 19May12 leave it commented out, because our Oxes v1 support will not
		// support inclusion of Adapt It notes, (probably never, but if we change our
		// minds then reinstate this later)
		if (gpApp->m_bOxesExportInProgress)
		{
			// 'numberOfChars' is not the number of characters in the note itself, but
			// rather the number of characters in the words of the adaptation phrase in the
			// m_targetStr member of this merged CSourcePhrase (oxes needs this info)
			int numberOfChars = pSrcPhrase->m_targetStr.Len(); // no space at end
			wxString numStr;
			numStr = numStr.Format(_T("%d"),numberOfChars);
			numStr = _T("@#") + numStr;
			numStr += _T(':'); // divider
			numStr += pSrcPhrase->m_targetStr;
			numStr += _T("#@");
			noteStr = numStr + noteStr;
			// the oxes parser must detect this @#nnn#@ substring and remove it, convert
			// it to int, and use it to count the phrase's length to which the note applies
			// so that the endOffset in the relevant NoteDetails struct can be set
			// correctly, and put the word after the colon into its wordsInSpan member
		}
*/
		markersPrefix.Trim();
		markersPrefix += aSpace + noteMkr;
		if (noteStr.IsEmpty())
		{
			// we don't yet support empty notes elsewhere in the app, but we'll do so here
			markersPrefix += aSpace + noteEndMkr;
		}
		else
		{
			markersPrefix += aSpace + noteStr;
			markersPrefix += noteEndMkr; // don't need space too
		}
	}
	return markersPrefix;
}


// This function takes the m_srcPhrase members from CSourcePhrase instances, along with
// filtered information (in m_filteredInfo) and any free translations, collected back
// translations and notes (the last three types we consider 'filtered' for compatibility
// with legacy builds), and rebuilds the source text in whatever edited state it happens
// to be in currently. The built source text is returned by reference, the return value is
// the length of the CString (not counting the terminating null, and newlines are not
// counted either - but we handle this problem later in the caller where we call the
// function FormatMarkerBufferForOutput().
//
// whm ammended 29Apr05. Since RebuildSourceText() is not used any longer in
// RetokenizeText() but only in export routines, I've rewritten it to always remove any
// filter bracket markers \~FILTER and \~FILTER* from the source text. The export routines
// determine whether to include the marker and associated text by inspecting the underlying
// marker within any filtered material.
//
// whm wx version observations and modifications: 
// This RebuildSourceText() function basically prepares a buffer which is used only for
// writing the source text out to disk in sfm form. (The RTF export routines need to
// process the text bit by bit and add a great amount of RTF code words to the output text,
// hence the RTF routines cannot make use of this RebuildSourceText function.)
//
// I created a routine called FormatMarkerBufferForOutput() which gets called after
// RebuildSourceText() is finished. It goes through and makes all the end-of-line tweaks
// and adjusts the spaces where needed.
//
// Cross-platform code must accommodate the different end-of-line (eol) schemes that the
// various operating systems use for external text-oriented files. The MFC code stores a
// single \n newline character within a buffer which gets automatically transformed into
// \n\r (i.e., CRLF or 0d 0a hex) by MFC CStudioFile's WriteString function. On the other
// hand, the wxFile::Write function needs to be cross-platform, so it doesn't presume to
// use a particular end-of-line scheme and only writes the text as it exists in the buffer
// (the user must determine the desired platform end-of-line chars). So, rather than mess
// with inserting the platform specific end-of-line endings here in RebuildSourceText, I've
// opted to instead relegate that whole chore to a separate function unique to the wx
// version called FormatMarkerBufferForOutput().
// BEW 7Apr10, updated for support of doc version 5 (changes were needed)
// BEW 11Oct10, additional docVersion 5 changes. Read the comments at top of this document
// for more information. The presence of placeholders is a complication, because the these
// can carry moved information from a neighbouring CSourcePhrase. Handling placeholders
// with such moved information requires that we "hold over" various types of moved
// information from off of the placeholder until the first non-placeholder is encountered,
// and the amalgamate it to what is in that one. Fortunately, moved info is only on the
// first if there are a string of placeholders not in a retranslation, or on the last if
// there are a string of placeholders in a retranslation or not in a retranslation. The
// ones which are not first or last can then be ignored, as they carry only ... in the
// source text, and that we want to ignore.
// BEW 7Dec10, added 2nd param, pList, which defaults to NULL; so that by explicitly
// supplying the list pointer, it can rebuild the source text from any SPList (I want this
// capability so that it can be used in the marker filtering mechanism, when the user
// changes a marker from being unfiltered, to filtered, using Preferences)
int RebuildSourceText(wxString& source, SPList* pUseThisList)
{
	wxString str; // local wxString in which to build the source text substrings

	// compose the output data & write it out, phrase by phrase,
	// restoring standard format markers as appropriate
	SPList* pList = NULL;
	if (pUseThisList == NULL)
	{
		pList = gpApp->m_pSourcePhrases;
	}
	else
	{
		pList = pUseThisList;
		gpApp->GetDocument()->UpdateSequNumbers(0, pList);
	}
	wxASSERT(pList != NULL);

	// As we traverse the list of CSourcePhrase instances, the special things we must be
	// careful of are 1. null source phrase placeholders (we ignore these, but we don't
	// ignore any m_markers, m_endMarkers, m_freeTrans, m_collectedBackTrans, m_note,
	// m_filteredInfo, m_precPunt, m_follPunct, and, since 11Oct10, m_follOuterPunct, &
	// inline beginmarkers or endmarkers of binding or non-binding type which are now also
	// in doc version 5 content, which has been moved to them by the user doing a
	// Placeholder insertion where markers or punctuation is located),
	// 2. retranslations (we can ignore the fact these instances belong to a retranslation),
	// 3. mergers (these give us a headache - we have to look at the stored list of
	// original CSourcePhrase instances on each such merged one, and build the source text
	// from those - since they will have any sf markers in the right places, and that info
	// is lost to the parent merged one; and handling filtered information complicates
	// this even further)
	// 
	// BEW added comment 08Oct05: version 3 introduces filtering, and this complicates the
	// picture a little. Mergers are not possible across filtered information, and so saved
	// original sourcephrase instances within a merged sourcephrase will never contain
	// filtered information; moreover, notes, backtranslations and free translations are
	// things which often are created within the document after it has been parsed in, and
	// possibly after the mergers are all done, and so we will find such information on a
	// merged sourcephrase itself but never in those it stores (except the first). This
	// means we must get any filtered information from the merged sourcePhrase's various
	// members, such as m_filteredInfo, m_freeTrans, etc (this first CSourcePhrase may
	// contain both filtered and unfiltered markers and their content), and for the saved
	// original CSourcePhrase instances in the m_pSavedWords array in the merged
	// sourcephrase we must examine all those originals in that sublist - but the first
	// must be given special treatment.
	SPList::Node* pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	source.Empty();
	wxString tempStr;
	//TRACE0("\n");
	// BEW added 16Jan08 A boolean added in support of adequate handling of markers which
	// get added to a placeholder due to it being inserted at the beginning of a stretch of
	// text where there are markers. Before this, placeholders were simply ignored, which
	// meant that if they had received moved content from the m_markers member, then that
	// content would get lost from the source text export. So now we check for moved
	// markers, store them temporarily, and then make sure they are relocated
	// appropriately. Also, a local wxString to hold the markers pending placement at the
	// correct location. 
	// Legacy comment: (We don't need to bother with punctuation movement in the context of
	// placeholder insertion because it is already handled by our choice of using the
	// m_srcPhrase member - which has it still attached.) And a further one to hold any
	// endmarkers
	// BEW 11Oct10, unfortunately that legacy comment no longer holds true. There might be
	// binding beginmarkers between preceding punctuation and the source text word, and/or
	// binding endmarkers between the word and following punctuation. There may also be
	// outer punctuation. So if we tried to just separate m_srcPhrase into preceding
	// puncts, the word, following puncts, in order to be able to replace binding marker or
	// endmarker or both, we'd still not get following outer punctuation handled properly
	// that way. So we have to build up the word not from m_srcPhrase, but from m_key,
	// using the ordering protocols for doc version 5's document model. This enables us to
	// avoid using a placement dialog for the problem of endmarker insertion before outer
	// punctuation. The ramification, however, is that we have to both checking for, store
	// and 'delay' placement of preceding puncts and final puncts on a placeholder, as well
	// as the new storage strings for inline markers, as well as m_markers and m_endMarkers
	// content as before -- so a lot more local storage strings for holding over data will
	// be needed.
	bool bMarkersOnPlaceholder = FALSE;
	wxString strPlaceholderMarkers; // for m_markers content
	wxString strPlaceholderEndMarkers; // for m_endMarkers content
	wxString aSpace = _T(" ");
	// Note, placing the above information is tricky. We have to delay markers relocation
	// until the first non-placeholder CSourcePhrase instance has been fully dealt with,
	// because that is the CSourcePhrase instance on to which they are to be moved...
	// BEW 11Oct10, except that for a retranslation with appended placeholders the last
	// placeholder may have moved information that has to be replaced (appended) to the end
	// of the information on the last non-placeholder within the retranslation. We can
	// handle all the possibilities if we just provide enough storage locations for
	// ending-stuff versus beginning-stuff, and follow the following protocols:
	// (1) get the beginning or ending (moved) info off of the placeholder, it will be one
	// or the other type, not both.
	// (2) First, append ending-stuff to the end of the source text output string so far
	// built up: in the following order: binding endmarker, following puncts, m_endMarkers
	// content, outer punctuation, non-binding endmarker. If any such was stored, skip 3.
	// because 3 won't be needed. Clear all the hold-over storage locations.
	// (3) If beginning-stuff, any or all of it, is non-empty, then set a flag
	// bMarkersOnPlaceholder will do the job nicely, even if there are no markers
	// involved, and keep this information until the next non-placeholder is encountered.
	// When it is, append to the output source text string so far built up, the held over
	// information in the following order (not all may be non-empty of course):
	// m_markers content, non-binding beginmarker, preceding punctuation, binding
	// beginmarker. Then do the call of FromSingleMakeStr() or FromMergerMakeStr()
	// depending on whether or not the non-placeholder is not, or is, a merger. The
	// information just placed won't also be on the pSrcPhrase, or pMergedSrcPhrase passed
	// in to the respective function, so just output the Sstr and append it to whatever is
	// at the end of the so far built up output source text string.
	// 
	// The above protocol does not lend itself to pulling the info off of a placeholder
	// within a function, but rather directly in RebuildSourceText() itself. So the
	// additional local variables are created here, adding to those just above.
	// Ending-stuff storage ones: (for information moved forward to be ending on the 
	// placeholder)
	// I've commented these out, because I can place the information immediately without
	// temporarily storing it in a local string variable -- see a little further down
	//wxString strPlaceholderBindingEndMkrs;
	//wxString strPlaceholderNonbindingEndMkrs;
	//wxString strPlaceholderFollPuncts;
	//wxString strPlaceholderFollOuterPunct;
	// The above strPlaceholderEndMarkers also belongs to the above ending set
	// 
	// Beginning-stuff storage ones: (for information moved backwards to be at the beginning
	// on the placeholder)
	wxString strCollectedBeginnings; // <<-- use this one when its okay to get the lot as 1 string
	// I may not need the next 4, the above strCollectedBeginnings may suffice
	//wxString strPlaceholderBindingMkrs;
	//wxString strPlaceholderNonbindingMkrs;
	//wxString strPlaceholderPrecPuncts;
	//wxString strPlaceholderFilteredInfo; // for m_filteredInfo, freetrans, collbacktrans
										 // but not notes because in docV5 we don't move
										 // a note to or from the placeholder
	// The above strPlaceholderMarkers also belongs to the above beginning set
	// 
	// In case you are wondering... All these shenanigans are not required for target text
	// export for the following reason. The placeholder's m_targetStr will have one or more
	// target text words in it which are vital to the meaning expressed. So, as far as the
	// export code is concerned, it must treat the placeholder just like any
	// non-placeholder - whether or not it has had information moved to it. That's why the
	// RebuildTargetText() function doesn't need all this extra apparatus.

	bool bHasFilteredMaterial = FALSE;
	while (pos != NULL)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();

#ifdef __WXDEBUG__
//		if (pSrcPhrase->m_nSequNumber >= 1297)
//		{
//			wxString strSrcPhrase = pSrcPhrase->m_srcPhrase;
//		}
#endif

		wxASSERT(pSrcPhrase != 0);
		str.Empty();

		// BEW added to following block 16Jan09, for handling relocated markers on 
		// placeholders
		bHasFilteredMaterial = HasFilteredInfo(pSrcPhrase);
		if (pSrcPhrase->m_bNullSourcePhrase)
		{
			// markers placement from a preceding placeholder may be pending but there may
			// be a second or other placeholder following, which must delay their
			// relocation until a non-placeholder CSourcePhrase is encountered. So if the
			// bMarkersOnPlaceholder flag is TRUE, just have this placeholder ignored
			// without the populatiing of any of the local wxString variables above
 			if (!bMarkersOnPlaceholder)
			{
				// markers placement from a preceding placeholder is not pending, so from
				// this placeholder gather any filtered information, and m_markers and
				// endmarkers - and set the flag if there are some from a right
				// association, but if there are some from a left association (ie.
				// ending-stuff as discussed above), then don't set the flag but instead
				// append the relevant material directly to source parameter.
				
				// deal with any ending-stuff first, as it's immediately placeable; do it
				// in the order in which it must be appended to the accumulating str variable
				str = AppendSrcPhraseEndingInfo(str, pSrcPhrase);

				// now deal with the beginning-stuff, which isn't immediately placeable:
				// 2nd, 3rd & 4th booleans as follows:
				// bool bIncludeNote is FALSE
				// bool bDoCountForFreeTrans is TRUE
				// bool bCountInTargetTextLine is FALSE
				strCollectedBeginnings = AppendSrcPhraseBeginningInfo(strCollectedBeginnings,
								pSrcPhrase, bMarkersOnPlaceholder, FALSE, TRUE, FALSE);
				// if bMarkersOnPlaceholder returns TRUE from the above call, it means that
				// there was earlier right association of the placeholder and it then
				// picked up by transfer from the following non-placeholder CSourcePhrase
				// instance, some information which preceded the source text word, maybe
				// punctuation, maybe marker(s) maybe filtered info (to be restored to the
				// output of the export), or may all of such stuff. This stuff has to be
				// held over until the loop "sees" the next CSourcePhrase instance, it is
				// almost certainly the next one, since there's no need for the user to
				// manually insert two placeholders in sequence.
				
			} // end of TRUE block for test: if (!bMarkersOnPlaceholder)

			// If bMarkersOnPlaceholder was set TRUE on the last iteration, then if a
			// second placeholder follows the first one, control will jump to here and the
			// loop will thus iterate over this and any subsequent placeholders, ignoring
			// them, until a non-placeholder is encountered - in which case control goes
			// to the test below.
			continue; // ignore the rest of the current placeholder's information
		} // end of TRUE block for test: if (pSrcPhrase->m_bNullSourcePhrase)
		else if (pSrcPhrase->m_nSrcWords > 1 && !IsFixedSpaceSymbolWithin(pSrcPhrase))
		{
			// BEW 11Oct10, updated these two comments to comply with new info being dealt
			// with. It's a merged sourcephrase, (but not a pseudo-merged conjoined word
			// pair joined with USFM ~ markup) so look at the sublist instead -- except
			// that pre-first-word info must be reclaimed first from the merged
			// sourcephrase itself (mergers across filtered info are not permitted), and
			// the first sourcephrase in the sublist can contribute only its m_key text
			// string, but subsequent ones in the sublist must have pre-word and post-word
			// members examined and such information appended to the exported information
			// at the appropriate places.
			// 
			// BEW added more on 17Jan09: if pre-word info was moved to a preceding
			// placeholder, then bMarkersOnPlaceholder should be TRUE and those markers
			// must be placed now on this CSourcePhrase instance (which will not itself
			// have its pre-word info stored on itself because its former content is now
			// what we are attempting to replace by relocation from the preceding
			// placeholder.
			if (bMarkersOnPlaceholder)
			{
				// pSrcPhrase here is not a placeholder, so filtered material and any
				// m_markers content that got moved to a preceding placeholder, and now
				// pending for placement, should be dealt with now
				if (!strCollectedBeginnings.IsEmpty())
				{
					// append any pre-word information earlier moved to a preceding 
					// placeholder when it was right-associated
					str += strCollectedBeginnings;
					strCollectedBeginnings.Empty();
				}
				// placeholder information is handled, so the flag can be cleared
				bMarkersOnPlaceholder = FALSE;
				// don't add a space, any needed will be already in place in the data
				// obtained from strCollectedBeginnings just above, and if the last data
				// was preceding punctuation for the word, a space inserted here would be
				// a disaster
			}

			// next, deal with pSrcPhrase itself - if it has filtered information, etc, we need
			// a function to aggregate such stuff and return it as a string, so use the
			// helpers.cpp function FromMergerMakeSstr(), but first deal with any stored
			// filtered information, and m_markers content (which is never filtered),
			// before handling the accumulation of material from the merged CSourcePhrase;
			// because we don't gather those info tyhpes in the above function
			wxString xrefStr;
			wxString otherFiltered;
			if (bHasFilteredMaterial)
			{
				// get the filtered stuff, FALSE means that any word-counting required by
				// getting a free translation and its span, the counting will be done in
				// the source text words, not the target text ones; TRUE means 'do the
				// word count'; and 4th param, bIncludeNote which is default TRUE, here
				// will take its default value because this pSrcPhrase isn't a
				// placeholder, and so there may be a note here filtered away.
				tempStr = GetFilteredStuffAsUnfiltered(pSrcPhrase, TRUE, FALSE);
				// separate out the cross reference info & markers
				SeparateOutCrossRefInfo(tempStr, xrefStr, otherFiltered);
				// any filtered info (other than cross reference) goes first
				if (!otherFiltered.IsEmpty())
					str << otherFiltered;
				tempStr.Empty();
				str.Trim();
				// handle m_markers, we delay xrefStr placement as it depends on what is
				// within the m_markers member
				if (!pSrcPhrase->m_markers.IsEmpty())
				{
					tempStr = pSrcPhrase->m_markers;
					// BEW changed 2Jun06, to prevent unwanted space insertion before \f,
					// \fe or \x, so we do it adding a space before tempStr provided one
					// of those markers is not at the start of tempStr
					tempStr = AddSpaceIfNotFFEorX(tempStr, pSrcPhrase);
					if (!xrefStr.IsEmpty())
					{
						// markers must precede cross reference info
						str += tempStr;
						str += xrefStr;
						xrefStr.Empty();
					}
					else
					{
						// xrefString is empty
						str += tempStr;
					}
					tempStr.Empty();
				}
				else
				{
					// m_markers is empty, but we still have to place xrefStr if it is
					// non-empty
					if (!xrefStr.IsEmpty())
					{
						str += xrefStr;
						xrefStr.Empty();
					}
				}
				str.Trim();
				str << aSpace; // end it with a space
				source << str;
			} // end of TRUE block for test: if (bHasFilteredMaterial)
			else
			{
				// handle m_markers
				if (!pSrcPhrase->m_markers.IsEmpty())
				{
					tempStr = pSrcPhrase->m_markers;
					// BEW changed 2Jun06, to prevent unwanted space insertion before \f,
					// \fe or \x, so we do it adding a space before tempStr provided one
					// of those markers is not at the start of tempStr
					tempStr = AddSpaceIfNotFFEorX(tempStr, pSrcPhrase);
					str += tempStr;
					tempStr.Empty();
				}
				source << str;
			} // end of else block for test: if (bHasFilteredMaterial)
			// reconstitute the source text from the merger originals
			str = FromMergerMakeSstr(pSrcPhrase);
			// append it to source
			source.Trim();
			source << aSpace << str;
			str.Empty();
		}
		else
		{
			// it's a single word sourcephrase, or a ~ conjoinded word pair, 
			// so handle it....

			// handle any stnd format markers first
			// 
			// BEW added more on 17Jan09: if m_markers content was moved to a preceding
			// placeholder, then bMarkersOnPlaceholder should be TRUE and those markers
			// must be placed now on this CSourcePhrase instance (which will not itself
			// have its m_markers member having content because its former content is now
			// what we are attempting to replace by relocation from the preceding
			// placeholder.
			str.Empty();
			if (!pSrcPhrase->m_bNullSourcePhrase && bMarkersOnPlaceholder)
			{
			   // pSrcPhrase here is not a placeholder, so filtered material and any
				// m_markers content that got moved to a preceding placeholder, and now
				// pending for placement, should be dealt with now
				if (!strCollectedBeginnings.IsEmpty())
				{
					// append any pre-word information earlier moved to a preceding 
					// placeholder when it was right-associated
					str += strCollectedBeginnings;
					strCollectedBeginnings.Empty();
				}
				// placeholder information is handled, so the flag can be cleared
				bMarkersOnPlaceholder = FALSE;
				// don't add a space, any needed will be already in place in the data
				// obtained from strCollectedBeginnings just above, and if the last data
				// was preceding punctuation for the word, a space inserted here would be
				// a disaster
			}
			tempStr.Empty();

			source << str;
			str.Empty();


			// add stuff, before and after the word, such as binding mkrs, puncts,
			// non-binding markers, endmarkers, outer punct, as needed
			wxString xrefStr;
			wxString mMarkersStr;
			wxString otherFiltered;
			bool bAttachFiltered;
			if (bHasFilteredMaterial)
			{
				bAttachFiltered = TRUE;
			}
			else
			{
				bAttachFiltered = FALSE;
			}
			bool bAttach_m_markers = TRUE;
			// in next call, bCount is TRUE, bCountInTargetText is FALSE (counting
			// words in the source text of the free translation section, if any)
			str = FromSingleMakeSstr(pSrcPhrase, bAttachFiltered, bAttach_m_markers,
									mMarkersStr, xrefStr, otherFiltered, TRUE, FALSE);

			source.Trim();

			// BEW added 13Jul1 next 3 lines: support Paratext usfm-only contentless
			// chapters or books - the legacy code would output \v 1 followed by newline,
			// but Paratext has \v 1 followed by space, then the newline - so test for
			// m_key empty, and if so, add a space to str here and don't trim it off below
			if (pSrcPhrase->m_key.IsEmpty())
			{
				str += aSpace;
			}

			// if we return ']' bracket, we don't want a preceding space
			if (str[0] == _T(']'))
			{
				source << str;
			}
			else if (!source.IsEmpty() && source[source.Len() - 1] == _T('['))
			{
				// in this circumstance we don't want an intervening space
				source << str;
			}
			else
			{
				source << aSpace << str;
			}
			str.Empty();
		}
	}// end of while (pos != NULL) for scanning whole document's CSourcePhrase instances

	source.Trim();
	source << aSpace;

	// update length
	return source.Len();
}

// RebuildText_For_Collaboration() is a wrapper for the RebuildSourceText(),
// RebuildTargetText(), RebuildGlossesText() or RebuildFreeTransText() functions. Which is
// the case is determined by the enum value exportType passed in.
// It is to be used when collaborating with Paratext or Bibledit. It calls
// the appropriate Rebuild...Text() function, using the CSourcePhrase instances SPList pointer passed in (the
// instances may or may not be the m_pSourcePhrases list), and then does filtering and
// tweaking to get USFM compatible with either of those two edit applications - which
// means filtering out our custom markers (\free, \note, and \bt or any derivative of the
// latter) if bFilterCustomMarkers has its default value of TRUE
// ExportType enum is defined in KB.h
// enum ExportType
// {
//	sourceTextExport,
//	targetTextExport,
//	glossesTextExport,
//	freeTransTextExport
// };
// While the intent is just, at this stage, to get either the source text returned, or the
// target text returned, it was almost no cost to have the option of the other types
// (glosses as text, or free translations) as well, so they are supported too.
wxString RebuildText_For_Collaboration(SPList* pList, enum ExportType exportType, bool bFilterCustomMarkers)
{
	//CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	wxString usfmText; usfmText.Empty();
	wxArrayString arrCustomBareMkrs;
	wxString mkr = _T("note");
	arrCustomBareMkrs.Add(mkr);
	mkr = _T("free");
	arrCustomBareMkrs.Add(mkr);
	mkr = _T("bt");
	arrCustomBareMkrs.Add(mkr);
	int nTextLength;
	switch(exportType)
	{
	case targetTextExport:
		nTextLength = RebuildTargetText(usfmText, pList);
		break;
	case freeTransTextExport:
		nTextLength = RebuildFreeTransText(usfmText, pList);
		break;
	case glossesTextExport:
		nTextLength = RebuildGlossesText(usfmText, pList);
		break;
	default:
	case sourceTextExport:
		nTextLength = RebuildSourceText(usfmText, pList);
		break;
	}
	if (nTextLength > 0)
	{
		// filter out unwanted custom markers, if any are present, and their text
		if (bFilterCustomMarkers)
		{
			usfmText = ApplyOutputFilterToText_For_Collaboration(usfmText, arrCustomBareMkrs);
		}
		// format for text oriented output
		FormatMarkerBufferForOutput(usfmText, exportType);
		usfmText = RemoveMultipleSpaces(usfmText);
	}
	return usfmText;
}

// BEW 11Oct10, removed \x from the test, because \x occurs after \v and verse number in
// USFM documentation, and so there should be a preceding space
wxString AddSpaceIfNotFFEorX(wxString str, CSourcePhrase* pSrcPhrase)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	wxString wholeMkr;
	bool bStartsWithMarker = FALSE;
	// whm 11Jun12 added test !str.IsEmpty() && to the test below. GetChar(0) should not be called on an
	// empty string.
	wxASSERT(!str.IsEmpty()); // whm 11Jun12 added. GetChar(0) below should not be called on an empty 
	if (!str.IsEmpty() && str.GetChar(0) == gSFescapechar)
	{
		wholeMkr = pDoc->GetWholeMarker(str);
		bStartsWithMarker = TRUE;
	}
	else
	{
		wholeMkr.Empty();
	}
	if (bStartsWithMarker)
	{
		if (pSrcPhrase->m_nSequNumber != 0 &&
			(wholeMkr != _T("\\f") && wholeMkr != _T("\\fe")))
			//(wholeMkr != _T("\\f") && wholeMkr != _T("\\fe") && wholeMkr != _T("\\x")))
		{
			// add an initial space if one is not already there, and it is not started
			// by a marker for which we want no space insertion
			wxASSERT(!str.IsEmpty()); // whm 11Jun12 added. GetChar(0) should not be called on an empty string
			if (!str.IsEmpty() && str.GetChar(0) != _T(' '))
			{
				str = _T(" ") + str;
			}
		}
	}
	else
	{
		// add an initial space if one is not already there
		// whm 11Jun12 added test !str.IsEmpty() && to the test below. GetChar(0) should not be called on an
		// empty string.
		wxASSERT(!str.IsEmpty()); // whm 11Jun12 added. GetChar(0) below should not be called on an empty 
		// string.
		if (!str.IsEmpty() && str.GetChar(0) != _T(' '))
		{
			str = _T(" ") + str;
		}
	}
	return str;
}

// This function takes the m_markers and m_gloss members from CSourcePhrase instances, and
// builds the glosses as if they were connected text (in whatever edited state it happens
// to be in currently.) The built glosses text is returned by reference, the return value
// is the length of the CString (not counting the terminating null, and newlines are not
// counted either - but we handle this problem later in the caller where we call the
// function FormatMarkerBufferForOutput().
//
// RebuildGlossesText() always removes any filter bracket markers \~FILTER and \~FILTER*
// from the filtered text material in m_markers. The export routines determine whether to
// include any of those markers and their associated text by inspecting the underlying
// markers within any filtered material.
//
// whm wx version observations and modifications: 
// This RebuildGlossesText() function basically prepares a buffer which is used only for
// writing the glosses as text out to disk in sfm form. (The RTF export routines need to
// process the text bit by bit and add a great amount of RTF code words to the output text,
// hence the RTF routines cannot make use of this RebuildGlossesText function directly,
// they use the final buffer contents instead.)
//
// I created a routine called FormatMarkerBufferForOutput() which gets called after
// RebuildGlossesText() is finished. It goes through and makes all the end-of-line tweaks
// and adjusts the spaces where needed.
//
// Cross-platform code must accommodate the different end-of-line (eol) schemes that the
// various operating systems use for external text-oriented files. The MFC code stores a
// single \n newline character within a buffer which gets automatically transformed into
// \n\r (i.e., CRLF or 0d 0a hex) by MFC CStudioFile's WriteString function. On the other
// hand, the wxFile::Write function needs to be cross-platform, so it doesn't presume to
// use a particular end-of-line scheme and only writes the text as it exists in the buffer
// (the user must determine the desired platform end-of-line chars). So, rather than mess
// with inserting the platform specific end-of-line endings here in RebuildSourceText, I've
// opted to instead relegate that whole chore to a separate function unique to the wx
// version called FormatMarkerBufferForOutput().
// BEW created 10Aug09
// BEW 9Apr10, updated for support of doc version 5 (changes were needed)
int RebuildGlossesText(wxString& glosses, SPList* pUseThisList)
{
	wxString str; // local wxString in which to build the 'glosses-as-text' substrings

	CAdapt_ItDoc* pDoc = gpApp->GetDocument();

	SPList* pList = NULL;
	if (pUseThisList == NULL)
	{
		pList = gpApp->m_pSourcePhrases;
	}
	else
	{
		pList = pUseThisList;
		gpApp->GetDocument()->UpdateSequNumbers(0, pList);
	}
	wxASSERT(pList != NULL);
	SPList::Node* pos = pList->GetFirst();
	wxASSERT(pos != NULL);

	// Compose the output data & write it out, phrase by phrase,
	// restoring standard format markers as appropriate....
	// As we traverse the list of CSourcePhrase instances, we do most but not all of what
	// RebuildSourceText() does, but there are significant differences, and this function
	// will be simpler: the special things we must be careful of are: 
	// 1. Null source phrase (ie. placeholders) -- we ignore these, but we don't ignore any
	// m_markers, content which has been moved to them by the user and we do collect from
	// them any non-empty gloss. Also we can't do any punctuation restoration since it
	// wasn't stripped from glosses in the first place.
	// 2. Retranslations -- we can ignore the fact these instances belong within a
	// retranslation. 
	// 3. Mergers (these give us a headache) -- we have to look at the stored list of
	// original CSourcePhrase instances on each such merged one, and build the glosses text
	// from those - since they will have any sf markers in the right places, and that info
	// is lost to the parent merged one. Mergers can occur in the glosses because they may
	// have been done when glossing was off, so we can't assume there aren't any.

	// BEW added comment 08Oct05: version 3 introduces filtering, and this complicates the
	// picture a little. Mergers are not possible across filtered information, and so saved
	// original sourcephrase instances within a merged sourcephrase will never contain
	// filtered information; moreover, notes, backtranslations and free translations are
	// things which often are created within the document after it has been parsed in, and
	// possibly after the mergers are all done, and so we will find filtered information on
	// a merged sourcephrase itself and never in those it stores. This means we must use
	// m_markers and other members from the merged sourcephrase and the other members will
	// contain filtered markers. We do not collect any filtered information of the
	// following kinds: free translations, notes or collected back translations, because
	// (a) it's difficult (b) such information does not necessarily logically apply to the
	// glosses, (c) such information may exist where no glosses exist in order to hang it
	// on (such as a section of the document which is free translated but for which the
	// user provides no glossing information) -- so we just ignore those kinds of filtered
	// material, but unfiltered markers have to be harvested in order to set up the
	// required SFM structure, and other filtered information we harvest as well - eg.
	// cross references, headings, etc -- these are things which are stored only in the
	// m_filteredInfo member.
	glosses.Empty();
	wxString tempStr;

	// BEW added 16Jan08 A boolean added in support of adequate handling of markers which
	// get added to a placeholder due to it being inserted at the beginning of a stretch of
	// text where there are markers. Before this, placeholders were simply ignored, which
	// meant that if they had received moved content from the m_markers member, then that
	// content would get lost from the source text export. So now we check for moved
	// markers, store them temporarily, and then make sure they are relocated
	// appropriately. Also, a local wxString to hold the markers pending placement at the
	// correct location; similarly another for endmarkers. Moved filtered markers we ignore
	// if they are free translations, notes or collected back translations, but we harvest
	// the rest.
	// BEW 11Oct10: changed to better support doc version 5.
	// (1) It is inappropriate to restored filtered information into a glosses export. The
	// filtered info cannot have any glosses, and so what business does it have in this
	// kind of export? None. So I've removed it.
	// (2) DocV5 has a richer marker storage model, and adds m_follOuterPunct member to
	// CSourcePhrase. Glossing doesn't strip or replace punctuation, so m_follOuterPunct
	// is irrelevant. Markers are relevant, but not all. The only inline markers which are
	// relevant to glossing export are those for footnotes, endnotes and crossReferences
	// (providing those info types are not filtered.) All other inline markers, those I'm
	// calling binding markers, and non-binding markers, should be ignored. So glossing
	// only replaces markers found in m_markers and m_endMarkers (which means the recent
	// addition of extra string members for storing those inline types can be ignored here).
	bool bMarkersOnPlaceholder = FALSE;
	wxString strPlaceholderMarkers;
	wxString strPlaceholderEndMarkers;
	wxString aSpace = _T(" ");
	// Note, placing the above information is tricky. We have to delay markers relocation
	// until the first non-placeholder CSourcePhrase instance has been dealt with, because
	// that is the CSourcePhrase instance on to which they are to be moved.

	//bool bHasFilteredMaterial = FALSE;
	while (pos != NULL)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();

		wxASSERT(pSrcPhrase != 0);
		str.Empty();

		// BEW added to following block 16Jan09, for handling relocated markers on
		// placeholders 
		//bHasFilteredMaterial = HasFilteredInfo(pSrcPhrase);
		if (pSrcPhrase->m_bNullSourcePhrase)
		{
			// markers placement from a preceding placeholder may be pending but there may
			// be a second or other placeholder following, which must delay their
			// relocation until a non-placeholder CSourcePhrase is encountered. So if the
			// bMarkersOnPlaceholder flag is TRUE, just have this placeholder ignored
			// without the population of the local wxString variable
 			if (!bMarkersOnPlaceholder)
			{
				// markers placement from a preceding placeholder is not pending, so from
				// this placeholder gather any m_markers and m_endMarkers information - and
				// set the flag
				/* BEW removed 11Oct10
 				if (bHasFilteredMaterial)
				{
					strPlaceholderMarkers = pSrcPhrase->GetFilteredInfo();
					if (!strPlaceholderMarkers.IsEmpty())
					{
						strPlaceholderMarkers = pDoc->RemoveAnyFilterBracketsFromString(strPlaceholderMarkers);
						bMarkersOnPlaceholder = TRUE;
					}
					if (!pSrcPhrase->m_markers.IsEmpty())
					{
						if (strPlaceholderMarkers.IsEmpty())
						{
							strPlaceholderMarkers = pSrcPhrase->m_markers;
						}
						else
						{
							strPlaceholderMarkers += aSpace + pSrcPhrase->m_markers;
						}
						bMarkersOnPlaceholder = TRUE;
					}
					if (!pSrcPhrase->GetEndMarkers().IsEmpty())
					{
						// while I've provided for storing any endmarkers on the placeholder,
						// the only circumstance I can think of where this may be relevant is
						// endmarkers earlier transferred to the end of a long retranslation
						// which was padded with final placeholders; so I guess the thing to
						// do is to 'place' any non-zero content in this string at the end of
						// the source string, once a non-placeholder is encountered, and then
						// clear the strPlaceholderEndMarkers string -- we'd have to do this
						// much further below
						strPlaceholderEndMarkers = pSrcPhrase->GetEndMarkers();
						bMarkersOnPlaceholder = TRUE;
					}
				} // end of TRUE block for test: if (bHasFilteredMaterial)
				*/
			}
			continue; // ignore the rest of the current placeholder's information
		}
		else if (pSrcPhrase->m_nSrcWords > 1)
		{
			// it's a merged sourcephrase, so look at the sublist instead -- filtered
			// information must be reclaimed from the merged sourcephrase itself (that was
			// done above), and the first sourcephrase in the sublist can contribute only
			// its m_gloss text string, but subsequent ones in the sublist must have their
			// m_markers member examined and its contents restored to the exported
			// information, likewise endmarkers.
			// 
			// BEW added more on 17Jan09: if m_markers content was moved to a preceding
			// placeholder, then bMarkersOnPlaceholder should be TRUE and those markers
			// must be placed now on this CSourcePhrase instance (which will not itself
			// have its m_markers member having content because its former content is now
			// what we are attempting to replace by relocation from the preceding placeholder.
			/* BEW removed 11Oct10
			tempStr.Empty();
			if (bMarkersOnPlaceholder)
			{
				// pSrcPhrase here is not a placeholder, so if any endmarkers on a
				// preceding placeholder are stored ready for placement, they must be
				// handled immediately before pSrcPhrase's information is dealt with
				if (bMarkersOnPlaceholder && !strPlaceholderEndMarkers.IsEmpty())
				{
					tempStr += strPlaceholderEndMarkers;
					strPlaceholderEndMarkers.Empty();
				}

				// filtered material and any m_markers content that got moved to a
				// preceding placeholder, and now pending for placement, should be dealt
				// with now (if we do something here, the above endmarker placement won't
				// have happened, and vise versa - it will be one or the other)
				if (bMarkersOnPlaceholder && !strPlaceholderMarkers.IsEmpty())
				{
					// relocate any non-endmarkers earlier moved to a preceding placeholder
					if (tempStr.IsEmpty())
					{
						tempStr = strPlaceholderMarkers;
					}
					else
					{
						tempStr += aSpace + strPlaceholderMarkers;
					}
					strPlaceholderMarkers.Empty(); // ready for next encounter of a placeholder
				}
				// placeholder information transfer is done, so the flag can be cleared
				bMarkersOnPlaceholder = FALSE; // clear to default FALSE value

				glosses.Trim();
				glosses << aSpace << tempStr;
			} // end of TRUE block for test: if (bMarkersOnPlaceholder)
			*/
			tempStr.Empty();

			// BEW changed 2Jun06, to prevent unwanted space insertion before \f, \fe
			// or \x, so we do it by refraining to do any space insertion at the start
			// of str when it starts with one of these markers
			wxString wholeMkr;
			bool bStartsWithMarker = FALSE;
			//wxASSERT(!str.IsEmpty()); // whm 11Jun12 added. GetChar(0) should not be called on an empty string
			// Tests show that str can be an empty string in export of glosses.
			if (!str.IsEmpty() && str.GetChar(0) == gSFescapechar)
			{
				wholeMkr = pDoc->GetWholeMarker(str);
				bStartsWithMarker = TRUE;
			}
			else
			{
				wholeMkr.Empty();
			}
			if (bStartsWithMarker)
			{
				if (pSrcPhrase->m_nSequNumber != 0 &&
					(wholeMkr != _T("\\f") && wholeMkr != _T("\\fe") && 
					wholeMkr != _T("\\x")))
				{
					// add an initial space if one is not already there, and it is not
					// started by a marker for which we want no space insertion
					wxASSERT(!str.IsEmpty()); // whm 11Jun12 added.
					if (str.GetChar(0) != _T(' '))
					{
						str = _T(" ") + str;
					}
				}
			}
			else
			{
				// add an initial space if one is not already there
				wxASSERT(!str.IsEmpty()); // whm 11Jun12 added. GetChar(0) should not be called on an empty string
				if (!str.IsEmpty() && str.GetChar(0) != _T(' '))
				{
					str = _T(" ") + str;
				}
			}
			glosses += str;


			// BEW changed 23Aug09, the earlier code which scans the sublist in order to
			// get marker placement done automatically is problematic, because the glosses
			// may have been added after mergers were done in adaptation mode, and so the
			// sublist would then contain no glosses, they would be only on the merged one
			// we are now dealing with. The correct approach is to take the merger at face
			// value, but to examine the sublist and collect markers into a list, and then
			// allow the user to "place" them - same as is done for adaptation mode.
			if (pSrcPhrase->m_bHasInternalMarkers)
			{
				str = FromMergerMakeGstr(pSrcPhrase);
			}
			else
			{
				// Legacy comment: when no phrase internal markers to be placed, just
				// append any filtered info, then any m_markers content, then the gloss,
				// and endmarkers will be handled later below
				// New comment: when no phrase internal markers to be placed, just append
				// any m_markers content, then the gloss, and endmarkers will be handled
				// later below
				/* BEW removed 11Oct10
				if (!pSrcPhrase->GetFilteredInfo().IsEmpty())
				{
					wxString filteredStuff = pSrcPhrase->GetFilteredInfo();
					filteredStuff = pDoc->RemoveAnyFilterBracketsFromString(filteredStuff);
					str << filteredStuff;
				}
				*/
				if (!pSrcPhrase->m_markers.IsEmpty())
				{
					str.Trim();
					str << aSpace << pSrcPhrase->m_markers;
				}
				if (!pSrcPhrase->m_gloss.IsEmpty())
				{
					str.Trim();
					str << aSpace << pSrcPhrase->m_gloss;
				}
			}

			// now add any endmarkers on the merger
			if (!pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				str << pSrcPhrase->GetEndMarkers();
			}

			// add an initial space if one is not already there
			wxASSERT(!str.IsEmpty()); // whm 11Jun12 added. GetChar(0) should not be called on an empty string
			if (!str.IsEmpty() && str.GetChar(0) != _T(' '))
			{
				str = _T(" ") + str;
			}
			// append the result to glosses string
			glosses += str;
		}
		else
		{
			// it's a single word sourcephrase, so handle it....

			// handle any stnd format markers first
			// 
			// BEW added more on 17Jan09: if m_markers content was moved to a preceding
			// placeholder, then bMarkersOnPlaceholder should be TRUE and those markers
			// must be placed now on this CSourcePhrase instance (which will not itself
			// have its m_markers member having content because its former content is now
			// what we are attempting to replace by relocation from the preceding placeholder.
			str.Empty();
			tempStr.Empty();
			if (bMarkersOnPlaceholder)
			{
				// pSrcPhrase here is not a placeholder, so if any endmarkers on a
				// preceding placeholder are stored ready for placement, they must be
				// handled immediately before pSrcPhrase's information is dealt with
				if (bMarkersOnPlaceholder && !strPlaceholderEndMarkers.IsEmpty())
				{
					tempStr += strPlaceholderEndMarkers;
					strPlaceholderEndMarkers.Empty();
				}

				// filtered material and any m_markers content that got moved to a
				// preceding placeholder, and now pending for placement, should be dealt
				// with now (if we do something here, the above endmarker placement won't
				// have happened, and vise versa - it will be one or the other)
				if (bMarkersOnPlaceholder && !strPlaceholderMarkers.IsEmpty())
				{
					// relocate any non-endmarkers earlier moved to a preceding placeholder
					if (tempStr.IsEmpty())
					{
						tempStr = strPlaceholderMarkers;
					}
					else
					{
						tempStr += aSpace + strPlaceholderMarkers;
					}
					strPlaceholderMarkers.Empty(); // ready for next encounter of a placeholder
				}
				// placeholder information transfer is done, so the flag can be cleared
				bMarkersOnPlaceholder = FALSE; // clear to default FALSE value

				glosses.Trim();
				glosses << aSpace << tempStr;
			}
			tempStr.Empty();

			// BEW changed 2Jun06, to prevent unwanted space insertion before \f, \fe
			// or \x, so we do it by refraining to do any space insertion at the start
			// of str when it starts with one of these markers
			wxString wholeMkr;
			bool bStartsWithMarker = FALSE;
			wxASSERT(!str.IsEmpty()); // whm 11Jun12 added. GetChar(0) should not be called on an empty string
			if (!str.IsEmpty() && str.GetChar(0) == gSFescapechar)
			{
				wholeMkr = pDoc->GetWholeMarker(str);
				bStartsWithMarker = TRUE;
			}
			else
			{
				wholeMkr.Empty();
			}
			if (bStartsWithMarker)
			{
				if (pSrcPhrase->m_nSequNumber != 0 &&
					(wholeMkr != _T("\\f") && wholeMkr != _T("\\fe") && 
					wholeMkr != _T("\\x")))
				{
					// add an initial space if one is not already there, and it is not
					// started by a marker for which we want no space insertion
					wxASSERT(!str.IsEmpty()); // whm 11Jun12 added. GetChar(0) should not be called on an empty string.
					if (!str.IsEmpty() && str.GetChar(0) != _T(' '))
					{
						str = _T(" ") + str;
					}
				}
			}
			else
			{
				// add an initial space if one is not already there
				wxASSERT(!str.IsEmpty()); // whm 11Jun12 added. GetChar(0) should not be called on an empty string.
				if (!str.IsEmpty() && str.GetChar(0) != _T(' '))
				{
					str = _T(" ") + str;
				}
			}
			glosses += str;
			str.Empty();

			/* BEW removed 11Oct10
			// add any m_filteredInfo content with filter bracket markers removed
			if (!pSrcPhrase->GetFilteredInfo().IsEmpty())
			{
				wxString filteredStuff = pSrcPhrase->GetFilteredInfo();
				filteredStuff = pDoc->RemoveAnyFilterBracketsFromString(filteredStuff);
				str << filteredStuff;
			}
			*/
			// add any m_markers content
			if (!pSrcPhrase->m_markers.IsEmpty())
			{
				str.Trim();
				str << aSpace << pSrcPhrase->m_markers;
			}
			// add the gloss, but only if it is non-empty
			if (!pSrcPhrase->m_gloss.IsEmpty())
			{
				str.Trim();
				str << aSpace << pSrcPhrase->m_gloss;
			}
			// finally add any endmarkers
			if (!pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				str << pSrcPhrase->GetEndMarkers();
			}

			// insert an initial space if one is not already there
			wxASSERT(!str.IsEmpty()); // whm 11Jun12 added. GetChar(0) should not be called on an empty string
			if (!str.IsEmpty() && str.GetChar(0) != _T(' '))
			{
				str = _T(" ") + str;
			}
			// append the result to glosses string
			glosses += str;
			str.Empty();
		} // end of block for a single CSourcePhrase instance
	}// end of while (pos != NULL) for scanning whole document's CSourcePhrase instances

	// update length
	return glosses.Length();
}

// This function takes the m_markers member from CSourcePhrase instances, and builds the
// free translation from the free translation sections stored filtered in m_markers (in
// whatever edited state it happens to be in currently.) The built free translation text is
// returned by reference, the return value is the length of the CString (not counting the
// terminating null, and newlines are not counted either - but we handle this problem later
// in the caller where we call the function FormatMarkerBufferForOutput().
//
// RebuildFreeTransText() always removes any filter bracket markers \~FILTER and \~FILTER*
// from the filtered text material in m_markers. The export routines determine whether to
// include any of those markers and their associated text by inspecting the underlying
// markers within any filtered material. Notes and collected back translations are never
// included
//
// whm wx version observations and modifications: 
// This RebuildFreeTransText() function basically prepares a buffer which is used only for
// writing the free translation sections' strings out to disk in sfm form. (The RTF export
// routines need to process the text bit by bit and add a great amount of RTF code words to
// the output text, hence the RTF routines cannot make use of this RebuildFreeTransText
// function directly, they use the final buffer contents instead.)
//
// I created a routine called FormatMarkerBufferForOutput() which gets called after
// RebuildFreeTransText() is finished. It goes through and makes all the end-of-line tweaks
// and adjusts the spaces where needed.
//
// Cross-platform code must accommodate the different end-of-line (eol) schemes that the
// various operating systems use for external text-oriented files. The MFC code stores a
// single \n newline character within a buffer which gets automatically transformed into
// \n\r (i.e., CRLF or 0d 0a hex) by MFC CStudioFile's WriteString function. On the other
// hand, the wxFile::Write function needs to be cross-platform, so it doesn't presume to
// use a particular end-of-line scheme and only writes the text as it exists in the buffer
// (the user must determine the desired platform end-of-line chars). So, rather than mess
// with inserting the platform specific end-of-line endings here in RebuildFreeTransText,
// I've opted to instead relegate that whole chore to a separate function unique to the wx
// version called FormatMarkerBufferForOutput().
// BEW created 10Aug09
// BEW 9Apr10, updated for support of doc version 5 (changes were needed)
int RebuildFreeTransText(wxString& freeTrans, SPList* pUseThisList)
{
	wxString str; // local wxString in which to build the freeTrans text substrings

	//CAdapt_ItDoc* pDoc = gpApp->GetDocument();

	SPList* pList = NULL;
	if (pUseThisList == NULL)
	{
		pList = gpApp->m_pSourcePhrases;
	}
	else
	{
		pList = pUseThisList;
		gpApp->GetDocument()->UpdateSequNumbers(0, pList);
	}
	wxASSERT(pList != NULL);
	SPList::Node* pos = pList->GetFirst();
	wxASSERT(pos != NULL);

	// Compose the output data & write it out, phrase by phrase, restoring standard format
	// markers as appropriate...
	// As we traverse the list of CSourcePhrase instances, we do most but not all of what
	// RebuildGlossesText() does, but there are significant differences, and this function
	// will be simpler: the special things we must be careful of are: 
	// 1. null source phrase placeholders (we ignore these, but we don't ignore any
	// m_markers, content which has been moved to them by the user's placeholder insertion
	// 2. retranslations (we can ignore the fact these instances may belong within a
	// retranslation), 
	// 3. mergers - we'll just take what's on the merged CSourcePhrase instance, we won't
	// look at the originals it stores -- this decision may result in loss of some
	// markers, but that's okay as they are not likely to be important ones and to keep
	// them would require frequent calls of a Place... dialog, which would deter a user
	// from using this functionality

	// BEW added comment 08Oct05: version 3 introduces filtering, and this complicates the
	// picture a little. Mergers are not possible across filtered information, and so saved
	// original sourcephrase instances within a merged sourcephrase will never contain
	// filtered information; moreover, notes, backtranslations are things which often are
	// created within the document after it has been parsed in, and possibly after the
	// mergers are all done, and so we will find filtered information on a merged
	// sourcephrase itself and never in the originals it stores. This means we must use
	// m_filteredInfo from the merged sourcephrase and it can only contain filtered
	// markers. We do not collect any filtered information of the following kinds: notes or
	// collected back translations, because (a) it's difficult (b) such information does
	// not necessarily logically apply to the free translations, (c) such information may
	// exist where no free translations exist -- so we just ignore those kinds of filtered
	// material, but unfiltered markers have to be harvested in order to set up the
	// required SFM structure, and other filtered information we harvest as well - eg.
	// cross references, headings, etc; however, some markers won't be correctly placed
	// because of the way free translation strings are stored, such as \it \it* for an
	// italicized word, or \k and \k* for a keyword, etc -- this will be in the output
	// together, after the free translation section in which they were stored. No big deal.
	// 
	// BEW comment 11Jun10, docVersion 5 doesn't store all the filtered material in
	// m_markers any more, but in dedicated wxString members of the CSourcePhrase; also,
	// we will ignore any markers which have the textType 'none' - because these are the
	// things likely to be medial in a merger (the user doesn't see them in the GUI), and
	// if medial and a RTF export is wanted, that could result in an endmarker without
	// matching start-marker, etc - which would cause the RTF scanning loop to terminate
	// prematurely - so it's best to just get rid of such markers 
	freeTrans.Empty();
	wxString tempStr;
	// BEW added 16Jan08 A boolean added in support of adequate handling of markers which
	// get added to a placeholder due to it being inserted at the beginning of a stretch of
	// text where there are markers. Before this, placeholders were simply ignored, which
	// meant that if they had received moved content from the m_markers member, then that
	// content would get lost from the source text export. So now we check for moved
	// markers, store them temporarily, and then make sure they are relocated
	// appropriately. Also, a local wxString to hold the markers pending placement at the
	// correct location. Moved filtered markers we ignore if they are notes or collected
	// back translations, but we harvest the rest.
	// BEW 11Oct10, removed exporting of other filtered info along with the free
	// translation; that is, we no longer unfilter things like filtered cross references,
	// etc, in order to place it within the free translation - that was madness and I
	// never should have done it in the first place.
	bool bMarkersOnPlaceholder = FALSE;
	wxString strPlaceholderMarkers;
	wxString strPlaceholderEndMarkers;
	wxString aSpace = _T(" ");
	// Note, placing the above information is tricky. We have to delay markers relocation
	// until the first non-placeholder CSourcePhrase instance has been dealt with, because
	// that is the CSourcePhrase instance on to which they are to be moved.

	bool bHasFilteredMaterial = FALSE;
	while (pos != NULL)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();

		wxASSERT(pSrcPhrase != 0);
		str.Empty();

		// BEW added to following block 16Jan09, for handling relocated markers on
		// placeholders 
		bHasFilteredMaterial = HasFilteredInfo(pSrcPhrase); // let it be calculated, but
								// if it is TRUE then I'll use that to comment out the
								// relevant code blocks (BEW 11Oct10)
		if (pSrcPhrase->m_bNullSourcePhrase)
		{
			// markers placement from a preceding placeholder may be pending but there may
			// be a second or other placeholder following, which must delay their
			// relocation until a non-placeholder CSourcePhrase is encountered. So if the
			// bMarkersOnPlaceholder flag is TRUE, just have this placeholder ignored
			// without the population of the local wxString variable
 			if (!bMarkersOnPlaceholder)
			{
				// markers placement from a preceding placeholder is not pending, so from
				// this placeholder gather any m_markers and m_endMarkers information - and
				// set the flag
 				if (bHasFilteredMaterial)
				{
					/* BEW removed 11Oct10
					strPlaceholderMarkers = pSrcPhrase->GetFilteredInfo();
					if (!strPlaceholderMarkers.IsEmpty())
					{
						strPlaceholderMarkers = pDoc->RemoveAnyFilterBracketsFromString(strPlaceholderMarkers);
						bMarkersOnPlaceholder = TRUE;
					}
					if (!pSrcPhrase->m_markers.IsEmpty())
					{
						if (strPlaceholderMarkers.IsEmpty())
						{
							strPlaceholderMarkers = pSrcPhrase->m_markers;
						}
						else
						{
							strPlaceholderMarkers += aSpace + pSrcPhrase->m_markers;
						}
						bMarkersOnPlaceholder = TRUE;
					}
					if (!pSrcPhrase->GetEndMarkers().IsEmpty())
					{
						// while I've provided for storing any endmarkers on the placeholder,
						// the only circumstance I can think of where this may be relevant is
						// endmarkers earlier transferred to the end of a long retranslation
						// which was padded with final placeholders; so I guess the think to
						// do is to 'place' any non-zero content in this string at the end of
						// the source string, once a non-placeholder is encountered, and then
						// clear the strPlaceholderEndMarkers string -- we'd have to do this
						// much further below
						strPlaceholderEndMarkers = pSrcPhrase->GetEndMarkers();
						bMarkersOnPlaceholder = TRUE;
					}
				*/
				}
			}
			continue; // ignore the rest of the current placeholder's information
		}
		else
		{
			// it's a merged or an unmerged sourcephrase non-placeholder; if a merger then
			// ignore the sublist, and just take anything useful from the merger itself,
			// and likewise for a nonmerger
			// 
			// BEW added more on 17Jan09: if m_markers content was moved to a preceding
			// placeholder, then bMarkersOnPlaceholder should be TRUE and those markers
			// must be placed now on this CSourcePhrase instance (which will not itself
			// have its m_markers member having content because its former content is now
			// what we are attempting to replace by relocation from the preceding
			// placeholder.
 			tempStr.Empty();
			if (bMarkersOnPlaceholder)
			{
				// pSrcPhrase here is not a placeholder, so if any endmarkers on a
				// preceding placeholder are stored ready for placement, they must be
				// handled immediately before pSrcPhrase's information is dealt with
				if (bMarkersOnPlaceholder && !strPlaceholderEndMarkers.IsEmpty())
				{
					tempStr += strPlaceholderEndMarkers;
					strPlaceholderEndMarkers.Empty();
				}

				// filtered material and any m_markers content that got moved to a
				// preceding placeholder, and now pending for placement, should be dealt
				// with now (if we do something here, the above endmarker placement won't
				// have happened, and vise versa - it will be one or the other)
				if (bMarkersOnPlaceholder && !strPlaceholderMarkers.IsEmpty())
				{
					// relocate any non-endmarkers earlier moved to a preceding placeholder
					if (tempStr.IsEmpty())
					{
						tempStr = strPlaceholderMarkers;
					}
					else
					{
						tempStr += aSpace + strPlaceholderMarkers;
					}
					strPlaceholderMarkers.Empty(); // ready for next encounter of a placeholder
				}
				// placeholder information transfer is done, so the flag can be cleared
				bMarkersOnPlaceholder = FALSE; // clear to default FALSE value

				freeTrans.Trim();
				freeTrans << aSpace << tempStr;
			}
			tempStr.Empty();
		   
			// append any m_markers content, then the free translation (if non-empty), and
			// endmarkers will be handled later below
			/* BEW removed 11Oct10
			if (!pSrcPhrase->GetFilteredInfo().IsEmpty())
			{
				wxString filteredStuff = pSrcPhrase->GetFilteredInfo();
				filteredStuff = pDoc->RemoveAnyFilterBracketsFromString(filteredStuff);
				str << filteredStuff;
			}
			*/
			if (!pSrcPhrase->m_markers.IsEmpty())
			{
				str.Trim();
				str << aSpace << pSrcPhrase->m_markers;
			}
			if (!pSrcPhrase->GetFreeTrans().IsEmpty())
			{
				str.Trim();
				str << aSpace << pSrcPhrase->GetFreeTrans();
			}
			if (!pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				str.Trim();
				str << pSrcPhrase->GetEndMarkers();
			}
			// add an initial space if one is not already there
			// whm 9Jun12 added !str.IsEmpty() to the test below because wxWidgets 2.9.3
			// asserts if str is empty and str.GetChar(0) is called - 0 is a bad index
			// into an empty string!
			if (!str.IsEmpty() && str.GetChar(0) != _T(' '))
			{
				str = _T(" ") + str;
			}
			// append the substring to the passed in freeTrans string
			freeTrans += str;
			str.Empty();
		}

	}// end of while (pos != NULL) for scanning whole document's CSourcePhrase instances

	// remove any marker or end-marker which has textType of 'none'
	RemoveMarkersOfType(none, freeTrans);

	// update length
	return freeTrans.Length();
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// \returns                nothing
/// \param  theTextType ->  the TextType enum value for the marker type to be removed
/// \param  text        ->  the wxString which has the text we wish to remove from
/// \remarks
/// Currently called only from RebuildFreeTransText(), to remove markers of TextType none
/// Creates a copy-buffer of same size as for the text param, and then scans through the passed
/// in buffer looking for markers with the passed in TextType value. Any such are skipped,
/// and all other characters scanned over are copied to the copy-buffer. The final
/// contents of the copy-buffer are then returned to the caller by value via the text parameter
/// BEW created 11Jun10, to fix an RTF free translation bug where the RTF production
/// halted prematurely because a merger over a \w marker made it medial to the merger, and
/// so not in the free translation SFM, leaving its \w* end-marker in the SFM as an orphan,
/// and that kills the RTF production code. \w is of TextType none, and so all markers of
/// that type should be omitted from a free translation export (i.e. remove markers for
/// things like bolding, glossary markers, wordlist markers, italics markers, and so forth)
/// BEW changed 11Oct10, to better support doc version 5, and to fix some unwanted
/// behaviour. Not all markers we want to exclude are of TextType none - unfortunately
/// that lets the inner footnote or cross reference markers like \fk \fv \fq etc \xo etc
/// to remain in the output. I'll add code to remove them too. We'll keep \f and \f* as
/// these delineate a footnote, similarly \x and \x*, also \fe and \fe*, but anything
/// between these will be removed.
///////////////////////////////////////////////////////////////////////////////////////////////
void RemoveMarkersOfType(enum TextType theTextType, wxString& text)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	int len = text.Len();

	// Since we require a read-only buffer for our main buffer we use GetData
	// which just returns a const wxChar* to the data in the string.
	const wxChar* pBuff = text.GetData();
	wxChar* pBufStart = (wxChar*)pBuff;
	wxChar* pEnd = pBufStart + len;
	wxASSERT(*pEnd == _T('\0'));
	wxChar* pOld = pBufStart;

	// For a copy-to buffer we'll base it on text2 with a same-sized buffer
	wxString text2;
	// Our copy-to buffer must be writeable so we must use GetWriteBuf() for it
	// whm 8Jun12 modified to use wxStringBuffer
	// Create the wxStringBuffer in a specially scoped block. This is crucial here
	// in this function since the wxString text2 is accessed directly within
	// this function in the text = text2; statement at the end of the function.
	{ // begin special scoped block
		wxStringBuffer pBuff2(text2,len + 1);
		//wxChar* pBuff2 = text2.GetWriteBuf(len + 1);
		wxChar* pNew = pBuff2;

		wxString wholeMkr;
		wxString bareMkr;
		wholeMkr.Empty();
		bool bIsMkrOfTypeToRemove = FALSE;
		int itemLen; // stores length of parsed marker
		while (*pOld != (wxChar)0 && pOld < pEnd)
		{
			// scan & copy across until next backslash (this test assumes we will no longer
			// support the legacy feature that \ is to be interpretted as a SF escape
			// character only before newlines)
			while (*pOld != gSFescapechar && pOld < pEnd)
			{
				*pNew++ = *pOld++;
			}
			if (pOld == pEnd)
			{
				// all transfers are done, return the string to the caller
				*pNew = (wxChar)0; // terminate the new buffer string with null char
				//text2.UngetWriteBuf(); // whm 8Jun12 removed - not used with wxStringBuffer
				//text = text2; // replace old string with new one
				// Use break here rather than return so the control will pass the end of the
				// special scope block to restore text2 to its normal state before it is used
				// to assign a value to the text reference parameter.				
				break; //return; 
			}
			// at an SF marker, we determine if it is one we wish to remove, or not, and in
			// the former case we jump it and and continue searching, but if not for removal
			// then we copy it across & continue searching
			//if (pDoc->IsMarker(pOld, pBufStart))
			if (pDoc->IsMarker(pOld))
			{
				// we are pointing at a marker; get the marker into the wholeMkr
				wholeMkr = pDoc->GetWholeMarker(pOld);

				// get the length of the marker
				itemLen = pDoc->ParseMarker(pOld);
				// check integrity of results from ParseMarker
				wxASSERT(itemLen > 0);
				wxASSERT(itemLen == (int)wholeMkr.Len());

				// get the bare marker, lookupable, (ie. no \ and no final *)
				bareMkr = wholeMkr.Mid(1); // remove initial backslash
				if (bareMkr.Last() == _T('*'))
				{
					bareMkr = bareMkr.Left(itemLen - 2);
				}

				// is it a marker of the type we want to remove?
				bool bSatifiesAdditionalRemovalTests = FALSE; // BEW added 11Oct10
				USFMAnalysis* pUSFMStruct = pDoc->LookupSFM(bareMkr);
				// BEW added 12N0v10 - protect against it being an unknown marker (as then
				// pUSFMStruct will be returned as NULL)
				if (pUSFMStruct == NULL)
				{
					bIsMkrOfTypeToRemove = FALSE; // we don't remove unknown marker types
				}
				else
				{
					bIsMkrOfTypeToRemove = pUSFMStruct->textType == theTextType ? TRUE : FALSE;

					// BEW 11Oct10, additional brute force tests, a little inneficiency here
					// won't be noticed.
				wxString mkrPlusSpace = gSFescapechar;
					mkrPlusSpace += bareMkr + _T(' ');
					if ((gpApp->m_inlineBindingMarkers.Find(mkrPlusSpace) != wxNOT_FOUND) ||
						(gpApp->m_inlineNonbindingMarkers.Find(mkrPlusSpace) != wxNOT_FOUND) ||
						(bareMkr == _T("fr") || bareMkr == _T("fk") || bareMkr == _T("fqa") ||
						 bareMkr == _T("fq") || bareMkr == _T("fl") || bareMkr == _T("fp") ||
						 bareMkr == _T("fv") || bareMkr == _T("ft") || bareMkr == _T("fdc") ||
						 bareMkr == _T("fm") || bareMkr == _T("xot") || bareMkr == _T("xk") ||
						 bareMkr == _T("xq") || bareMkr == _T("xt") || bareMkr == _T("xo") ||
						 bareMkr == _T("xnt") || bareMkr == _T("xdc")) )
					{
					bSatifiesAdditionalRemovalTests = TRUE;
					}
				}
				bool bRemovedIt = FALSE;
				if (bIsMkrOfTypeToRemove)
				{
					// don't copy it across, just put pOld pointing at first character after
					// it
					pOld += itemLen;
					bRemovedIt = TRUE;
				}
				else if (!bRemovedIt && bSatifiesAdditionalRemovalTests)
				{
					// don't copy it across, just put pOld pointing at first character after
					// it
					pOld += itemLen;
				}
				else
				{
					// it's not for removal, so copy it across
					int ctmkr;
					for (ctmkr = 0; ctmkr < itemLen; ctmkr++)
					{
						*pNew++ = *pOld++;
					}
				}
			} // end of TRUE block for test: if (pDoc->IsMarker(pOld, pBufStart))
		} // end of while (*pOld != (wxChar)0 && pOld < pEnd)
		*pNew = (wxChar)0; // terminate the new buffer string with null char
	} // end of special scoping block
	//text2.UngetWriteBuf(); // whm 8Jun12 removed - not used with wxStringBuffer
	text = text2; // replace old string with new one
}

// BEW revised 31Oct05
// BEW 7Apr10, updated for doc version 5 (changes were needed)
int RebuildTargetText(wxString& target, SPList* pUseThisList)
{
	SPList* pList = NULL;
	if (pUseThisList == NULL)
	{
		pList = gpApp->m_pSourcePhrases;
	}
	else
	{
		pList = pUseThisList;
		gpApp->GetDocument()->UpdateSequNumbers(0, pList);
	}
	wxASSERT(pList != NULL);

	wxString aSpace = _T(' ');
	//wxChar fwdslash = _T('/');	// a placeholder for newline (CString doesn't count newlines)
	//wxChar newline = _T('\n');	// before returning we replace forward slash placeholders
								// with newlines

	wxString targetstr; // accumulate the target text here

	SPList::Node* pos = pList->GetFirst();
	wxASSERT(pos != NULL);
	while (pos != NULL)
	{
		SPList::Node* savePos = pos;
		wxString str;
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase != 0);

//#if defined(__WXDEBUG__)
//		if (pSrcPhrase->m_nSequNumber == 4223)
//		{
//			int break_here = 1;
//		}
//#endif
		if (pSrcPhrase->m_bRetranslation)
		{
			// in the following call, str gets assigned internal markers
			// Note: we pass in savePos, and in the function we loop over all the
			// retranslation's CSourcePhrase instances, and when the internal loop exits
			// we then, still within the function, call the Placement dialog for any
			// medial markers needing placement, and then set savePos to be the Node*
			// immediately after the retranslation, or NULL if at end of doc
			pos = DoPlacementOfMarkersInRetranslation(savePos,pList,str); 
		}
		else
		{
			// not a retranslation, so get the target text for this srcPhrase
			str  = pSrcPhrase->m_targetStr; // this could possibly be empty

			// handle any stnd format markers, including internal ones -- most stnd format
			// markers can be handled silently. Place the phrase initial ones silently,
			// then if there are any phrase medial ones we need to internally use a dialog
			// to place those; distinguish mergers from single CSourcePhrase instances, the
			// former needs more complex code - and encapsulate the processing in a
			// function for each, for doc version 5;
			// BEW 11Oct10, both FromMergerMakeTstr() and FromSingleMakeTstr need changes
			// to handle the 4 extra wxString members for inline marker storage, and for
			// m_follOuterPunct support.
			if (pSrcPhrase->m_nSrcWords > 1 && !IsFixedSpaceSymbolWithin(pSrcPhrase))
			{
				// this pSrcPhrase stores a merger; first TRUE is bDoCount (of words
				// in the free translation section, if any such section), and second TRUE
				// is bCountInTargetText
				str = FromMergerMakeTstr(pSrcPhrase, str, TRUE, TRUE);
			}
			else
			{
				// this pSrcPhrase stores a single CSourcePhrase instance for a single
				// target text word (which could be the empty string); or a pair of words
				// with USFM fixed space symbol ~ conjoining them; first TRUE is bDoCount
				// (of words in the free translation section, if any such section), and
				// second TRUE is bCountInTargetText
				str = FromSingleMakeTstr(pSrcPhrase, str, TRUE, TRUE);
			}
		}

 		// handle when str contains only [ or ], we join [ to what follows, and ] to what
		// precedes, so we don't want space after [ and no space before ]; here, deal with
		// the case of str containing ]
		bool bPlacedAlready = FALSE;
		// whm 9Jun12 modified to add !str.IsEmpty() in the test
		if (!str.IsEmpty() && str[0] == _T(']'))
		{
			targetstr.Trim(); // remove any final space before appending ]
			targetstr << str;
			bPlacedAlready = TRUE;
		}
	   // BEW added 30May07, The retranslation text's block can present us with a str with
		// no initial space, and targetstr may not end with a space, so we have to check
		// for no space and add one if needed; but the check is only wanted if targetstr
		// has something in it and str is not empty.
		if (targetstr.Length() > 0 && !str.IsEmpty())
		{
			if (targetstr[targetstr.Length() - 1] != _T(' ') && str[0] != _T(' '))
				targetstr += _T(' ');
		}
		// after the above, targetstr will end with space, if it is not empty

		// handle when str contains only [, we don't want space after [ 
		if (!targetstr.IsEmpty() && targetstr[targetstr.Len() - 2] == _T('['))
		{
			// in this circumstance we don't want the space which follows [
			targetstr.Trim();
			str.Trim(FALSE); // remove any initial whitespace
			targetstr << str;
		}
		else
		{
			if (!bPlacedAlready)
			{
				targetstr << str << aSpace;
			}
		}
	}// end of while (pos != NULL)

	int textLen = targetstr.Length();
	target = targetstr; // return all the text in one long wxString
	return textLen;
}// end of RebuildTargetText

/* BEW 13Dec10: remove these commented out functions later, if no use for them by the time 6.0.0 is ready to ship
// support removal of \p markers temporarily inserted, when doc has no SFMs
bool IsDocWithParagraphMarkersOnly(SPList* pSrcPhrasesList)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	if (pSrcPhrasesList->IsEmpty())
		return FALSE;
	// we scan only until we find some marker other than \p
	bool bFoundParagraphMarker = FALSE; // use this to prevent unneeded 
										// finds after the first
	SPList::Node* pos = NULL;
	pos = pSrcPhrasesList->GetFirst();
	int offset = wxNOT_FOUND;
	int offset2 = wxNOT_FOUND;
	while (pos != NULL)
	{
		CSourcePhrase* pSrcPhrase = pos->GetData();
		pos = pos->GetNext();
		wxString markers = pSrcPhrase->m_markers;
		if (!markers.IsEmpty())
		{
			offset = markers.Find(_T("\\p"));
			if (offset != wxNOT_FOUND)
			{
				wxString beforeStr;
				beforeStr = markers.Left(offset);
				if (!beforeStr.IsEmpty())
				{
					// if there is a backslash in beforeStr, we assume some other marker
					// than \p is present in beforeStr, and so return FALSE; otherwise,
					// test for markers in afterStr after the \p marker
					offset2 = beforeStr.Find(gSFescapechar);
					if (offset2 != wxNOT_FOUND)
					{
						return FALSE;
					}
				}
				// beware, afterStr can contain more than one \p marker, because AI puts
				// one such in for each newline - so a few blank lines in the original
				// file will result in m_markers like this "\p \p \p \p " and so we have
				// to bleed these out, until we are left with an afterStr with some other
				// marker, or with no marker
				wxString afterStr;
				afterStr = markers.Mid(offset);
				wxString wholeMkr = pDoc->GetWholeMarker(afterStr);
				if (wholeMkr == _T("\\p"))
				{
					bFoundParagraphMarker = TRUE;
				}
				if (bFoundParagraphMarker)
				{
					// remove \p from afterStr and then test for other markers present; do
					// the removal of \p repeatedly if there are more than one in sequence
					afterStr = afterStr.Mid(2);
					if (!afterStr.IsEmpty())
					{
						// if there is a backslash in afterStr, we assume some other marker
						// than \p is present, and so return FALSE; otherwise, keep scanning
retry:					offset2 = afterStr.Find(gSFescapechar);
						if (offset2 != wxNOT_FOUND)
						{
							// if it is a \p marker, then shorten afterStr and retest
							if (afterStr[offset2 + 1] == _T('p'))
							{
								// it's a \p marker too, so shorten the string & retry
								afterStr = afterStr.Mid(offset2 + 2);
								goto retry;
							}
							else
							{
								// it's some other marker, so return FALSE
								return FALSE;
							}
						}
					}
				}
				else
				{
					// there is no \p marker in m_markers nor any marker beginning with \p, 
					// so check for a backslash there -- if there is, we've some other
					// marker present and so can return FALSE; if not, keep scanning
					offset2 = markers.Find(gSFescapechar);
					if (offset2 != wxNOT_FOUND)
					{
						return FALSE;
					}
				}
			} // end of TRUE block for test: if (offset != wxNOT_FOUND)
		} // end of TRUE block for test: if (!markers.IsEmpty())
	}
	// we got through the whole document without finding a marker other than \p
	if (!bFoundParagraphMarker)
	{
		// we didn't find a \p marker either -- treat this the same as if there were other
		// markers, return FALSE, because we don't want the caller to try remove \p markers
		return FALSE;
	}
	return TRUE; // didn't find any other markers than \p
}

// str is the output from an export function, such as a source or target text SFM export,
// or even a glosses or free translation export; if the input from which the doc was
// created had no SFMs, then the export will have just \p markers - these need to be
// removed and this function will do it; the \p-less string is returned
wxString RemoveParagraphMarkersOnly(wxString& str)
{
	wxString s;
	size_t len = str.Len();
	const wxChar* pBuffer = str.GetData(); // read only
	const wxChar* pEnd = pBuffer + len;
	wxChar* ptr = (wxChar*)pBuffer; // our iterator
	wxChar* aux = ptr; // auxiliary pointer, to delimit start of a text span
	while (ptr < pEnd)
	{
		if (*ptr == gSFescapechar && *(ptr + 1) == _T('p'))
		{
			// we've matched a \p sequence, so send everything from aux up to where ptr is
			// pointing, to the string s
			wxString aSpan(aux, ptr);
			if (!aSpan.IsEmpty())
			{
				s += aSpan;
			}
			ptr += 2; // advance over \p
			aux = ptr; // mark the start of the next span
		}
		else
		{
			ptr++;
		}
	}
	return s;
}
*/

// BEW 26Aug10, added ChangeCustomMarkersToParatextPrivates() in order to support the USFM
// 2.3 new feature, where a \z prefix is supplied to 3rd party developers for custom
// markers which Paratext will ignore. 
// BEW 30Aug10, the Oxes documentation says that the appropriate USFM would be
// \zAnnotation, and so I'll change from \zNote to \zAnnotation, to comply with this
// This function will change:
// \bt     to      \zbt
// \free   to      \zfree
// \free*  to      \zfree*
// \note   to      \zAnnotation
// \note*  to      \zAnnotation*
// but it will be called late, so that these changes are NOT made in the wxString buffer
// that is passed on to the RTF-construction functions. 
// Note: we retain the use of \bt, \free, and \note internally; so importing must convert
// these \z- based alternatives back to our legacy ones
void ChangeCustomMarkersToParatextPrivates(wxString& buffer)
{
	// converting the start markers, also converts all their matching endmarkers too
	wxString oldFree = _T("\\free");
	wxString newFree = _T("\\zfree");
	wxString oldNote = _T("\\note");
	wxString newNote = _T("\\zAnnotation");
	wxString oldBt = _T("\\bt "); // include space, any of SAG's \bt-derived markers we
								  // will not convert; and our \bt has no endmarker so
								  // this is safe to do
	wxString newBt = _T("\\zbt "); // need the space here too, since we are replacing

	// the conversions are done with the 3rd param, replaceAll, left at default TRUE
	int count = buffer.Replace(oldFree,newFree);
	count = buffer.Replace(oldNote,newNote);
	count = buffer.Replace(oldBt,newBt);
	count = count; // avoid compiler warnings
}

// ApplyOutputFilterToText takes an input string textStr, scans through its string buffer
// and builds a new wxString minus the markers and associated text whose flags are set to
// be filtered from output in bareMarkerArray and filterFlagsArray; then returns the new
// string.
wxString ApplyOutputFilterToText(wxString& textStr, wxArrayString& bareMarkerArray, 
								 wxArrayInt& filterFlagsArray, bool bRTFOutput)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();

	// Setup input buffer from textStr
	// wx version: we use the textLen passed in as parameter rather than getting length
	// of the string with embedded new line chars. It is used to set pEnd in the read-only
	// buffer and used to determine the size (textLen*2 + 1) and pEnd2 for the writeable
	// buffer below
	int nTheLen = textStr.Length();
	// wx version: pBuffer is read-only so just get pointer to the string's read-only buffer
	const wxChar* pBuffer = textStr.GetData();
	wxChar* pBufStart = (wxChar*)pBuffer;		// save start address of Buffer
	int itemLen = 0;
	wxChar* pEnd = pBufStart + nTheLen;// bound past which we must not go
	wxASSERT(*pEnd == _T('\0')); // ensure there is a null at end of Buffer
	bool bHitMkr = FALSE;
	bool bIsAMarker = FALSE;

	// Setup copy-to buffer from textStr2.
	// Since we also may add backslash escape characters to the textStr2 buffer
	// when bRTFOutput is TRUE in order to account for any embedded curly braces,
	// we'll set the textStr2 to twice the size of textStr. When bRTFOutput is FALSE
	// we'll always get the same or smaller string at end of processing and the size
	// of textStr2 in that case can be the same size as textStr.
	wxString textStr2;
	wxChar* pEnd2;
	
	// whm 8Jun12 modified to use wxStringBuffer
	int buffSizeMultiplier;
	if (bRTFOutput)
		buffSizeMultiplier = 2;
	else
		buffSizeMultiplier = 1;
	
	// Begin a new block where pBuffer2 is created as wxStringBuffer from textStr2.
	// Note: this block is required to be able to return textStr2 to its usable 
	// state at the end of the block below before returning the value of textStr2
	// to the caller.
	{
		wxStringBuffer pBuffer2(textStr2,nTheLen*buffSizeMultiplier + 1);
		pEnd2 = pBuffer2 + nTheLen*buffSizeMultiplier;
		if (buffSizeMultiplier == 1)
			*pEnd2 = (wxChar)0;
		/*
		if (bRTFOutput)
		{
			// whm 8Jun12 modified to use wxStringBuffer
			//wxStringBuffer pBuffer2(textStr2,nTheLen*2 + 1);
			//pBuffer2 = textStr2.GetWriteBuf(nTheLen*2 + 1);
			//pEnd2 = pBuffer2 + nTheLen*2; // whm added
			// wx version note: This buffer does not have, nor need a null char placed
			// at the end of it. We assume being twice pBuffer's size would be sufficient
			// for all cases. Moreover, the pointer pNew only checks for a null char in
			// the pBuffer, not this one.
		}
		else
		{
			// wx version note: This buffer does not have, nor need a null char placed
			// at the end of it. We assume being the same as pBuffer's size would be sufficient
			// for all cases. Moreover, the pointer pNew only checks for a null char in
			// the pBuffer, not this one.
			// whm 8Jun12 modified to use wxStringBuffer
			// wxStringBuffer pBuffer2(textStr2,nTheLen + 1);
			// pBuffer2 = textStr2.GetWriteBuf(nTheLen + 1);
			// pEnd2 = pBuffer2 + nTheLen; // whm added
			// *pEnd2 = (wxChar)0; // whm added
		}
		*/

		wxString bareMarkerForLookup,bareMarkerInInputArray, wholeMarker;
		int nMarkersInArray = bareMarkerArray.GetCount();
		if (nMarkersInArray != (int)filterFlagsArray.GetCount())
		{
			::wxBell();
			wxASSERT(FALSE);
		}

		wxChar* pOld = pBufStart;  // source
		wxChar* pNew = pBuffer2; // destination
		while (*pOld != (wxChar)0)
		{
			// Use the the View's IsMarkerRTF if bRTFOutput
			if (bRTFOutput)
			{
				bIsAMarker = IsMarkerRTF(pOld,pBufStart);
			}
			else
			{
				//bIsAMarker = pDoc->IsMarker(pOld,pBufStart);
				bIsAMarker = pDoc->IsMarker(pOld);
			}

			if (bIsAMarker)
			{
				// We're pointing at a marker. Look it up and see if we should skip over it and
				// its associated text
				wholeMarker = pDoc->GetWholeMarker(pOld); // the whole marker including backslash and any ending *

				// whm added 7Nov07
			// If wholeMarker is just a bare backslash it means that we have an embedded user entered
			// back slash that is not part of a marker. In RTF outputs we need to escape it so that
			// it does not look like some RTF control word to the RTF file reader, likely causing
				// MS Word to choke on it.
				if (wholeMarker == _T("\\"))
				{
					if (bRTFOutput)
					{
						// check for embedded backslash in the text
						if (*pOld == _T('\\'))
						{
							// prefix the backslash with another backslash in pNew to escape it.
							*pNew++ = _T('\\');
						}
					}

					*pNew++ = *pOld++;

				// The tests below don't make sense for an isolated non-marker backslash,
				// and we shouldn't set bHitMarker to TRUE, so just continue processing at
					// the top of the while loop
					continue;
				}

				bareMarkerForLookup = pDoc->GetBareMarkerForLookup(pOld); // strips off backslash and any ending *
				// if we encounter an end marker before its beginning form (as might happen if we
				// are using ApplyOutputFilterToText on an m_endMarkers string, we want to check it
				// for filtering in isolated fashion without parsing to find any associated text.
				if (wholeMarker.Find(_T('*')) != -1 && !bHitMkr)
				{
					// we hit this end marker before seeing a beginning marker in the input text
					// if its non-end form it to be filtered we'll parse over it and omit it from
					// the text, otherwise we'll leave it alone
					if (MarkerIsToBeFilteredFromOutput(bareMarkerForLookup))
					{
						if (bRTFOutput)
						{
							itemLen = ParseMarkerRTF(pOld,pEnd);
						}
						else
						{
							itemLen = pDoc->ParseMarker(pOld);
						}
						pOld += itemLen;
						// don't increment pNew here
						itemLen = pDoc->ParseWhiteSpace(pOld);
						pOld += itemLen;
						// don't increment pNew here
						continue; // BEW added 22June11, control needs to iterate if this block was entered
					}
				}

				bHitMkr = TRUE;
				// for output filter purposes we treat all \bt... initial markers as simple \bt, so
				// change any \bt... ones to just \bt
				if (bareMarkerForLookup.Find(_T("bt")) == 0)
				{
					bareMarkerForLookup = _T("bt"); // change all \bt... initial ones to just \bt
				}
				if (MarkerIsToBeFilteredFromOutput(bareMarkerForLookup))
				{
					// This marker needs to be filtered out/skipped over. We have to parse over
					// the marker and its associated text and any end marker it may have. In the
					// process we increment pOld but not pNew.

					wxString wholeMkrNoAsteriskSp = _T('\\') + bareMarkerForLookup + _T(' ');
					if (charFormatMkrs.Find(wholeMkrNoAsteriskSp) != -1)
					{
						// we are filtering one of the character markers that affects font formatting
						// we don't want to filter out the associated text, only remove the formatting
						// i.e., by removing the markers and leaving the associated text
						// parse the initial marker
						itemLen = pDoc->ParseMarker(pOld);
						pOld += itemLen;
						// don't increment pNew here
						itemLen = pDoc->ParseWhiteSpace(pOld);
						pOld += itemLen;
						// don't increment pNew here
						// parse the associated text until we come to the end marker
						while (pOld < pEnd && !pDoc->IsCorresEndMarker(wholeMarker, pOld, pEnd))
						{
							// just copy whatever we are pointing at and then advance
							*pNew++ = *pOld++;
						}
						if (pOld < pEnd)
						{
							// we found a corresponding end marker to we need to parse it too
							itemLen = pDoc->ParseMarker(pOld);
							pOld += itemLen;
							// don't increment pNew here

							// parse remaining whitespace but preserve new lines
							if (*pOld == _T('\n'))
							{
								// just copy the new line and then advance
								*pNew++ = *pOld++;
							}
							else
							{
								itemLen = pDoc->ParseWhiteSpace(pOld);
								pOld += itemLen;
								// don't increment pNew here
							}
						}
					}
					else
					{
						// we are filtering both marker(s) and associated text
						itemLen = ParseMarkerAndAnyAssociatedText(pOld, pBufStart, pEnd,
													bareMarkerForLookup, wholeMarker,FALSE,FALSE);
						// 1st FALSE above means don't consider this text to be RTF text at this point
						// because it won't be RTF compatible text until ApplyOutputFilterToText is done
						// with it
						// 2nd FALSE above means don't include char format markers

						pOld += itemLen; // advance pOld past marker and its associated text
						// don't increment pNew for the skipped marker
						// if after parsing the marker we're now pointing at a new line, preserve it

						// parse any remaining white space but preserve new lines
						// BEW 18Apr2011 -- just preserving newline means that the one and
						// only space between the endmarker and the following text is also
						// removed, and if there was not a space before the begin marker, this
						// makes a bogus coalescence of two words - so we need to retain at
						// least one space
						if (*pOld == _T('\n') || *pOld == _T(' '))
						{
							// just copy the new line or one space and then advance over it
							*pNew++ = *pOld++;
						}
						//else
						// remove any additional spaces
						if (*pOld == _T(' '))
						{
							itemLen = pDoc->ParseWhiteSpace(pOld);
							pOld += itemLen;
							// don't increment pNew here
						}
					}
				}
				else
				{
					// not filtering
					// just copy whatever we are pointing at and then advance
					if (bRTFOutput)
					{
						// check for embedded curly braces in the text
						if (*pOld == _T('{') || *pOld == _T('}'))
						{
							// there is a curly brace so prefix it with a backslash in pNew to escape it.
							*pNew++ = _T('\\');
						}
					}
					*pNew++ = *pOld++;
				}
			}
			else
			{
				// it's not a marker but text
				// just copy whatever we are pointing at and then advance
				if (bRTFOutput)
				{
					// check for embedded curly braces in the text
					if (*pOld == _T('{') || *pOld == _T('}'))
					{
						// there is a curly brace so prefix it with a backslash in pNew to escape it.
						*pNew++ = _T('\\');
					}
				}
				*pNew++ = *pOld++;
			}
		}
		*pNew = (wxChar)0; // add a null at the end of the string in pBuffer2
	} // end the scoping block where pBuffer2 is created as wxStringBuffer from textStr2.
	  // Note: this block ending is required to return textStr2 to its usable state.
	//textStr2.UngetWriteBuf(); // whm 8Jun12 not used with wxStringBuffer
	return textStr2;
}

// ApplyOutputFilterToText_For_Collaboration takes an input string textStr, scans through
// its string buffer and builds a new wxString minus certain markers and associated text -
// these are passed in, minus their initial backslash, in the bareMarkerArray. It then
// returns the new string. This (simpler) variant of ApplyOutputFilterToText() is designed
// for use in Paratext or Bibledit collaboration, to ensure removal of unwanted material
// that Paratext or Bibledit is not going to want to see - this is our custom markers
// (\free, \note, \bt and any derivatives of the latter).
// ApplyOutputFilterToText_For_Collaboration() does not rely on or use the filtering
// mechanisms and structures designed by Bill, it is self-contained, and has no provision
// for filtering for RTF purposes, but only for (U)SFM output.
wxString ApplyOutputFilterToText_For_Collaboration(wxString& textStr, wxArrayString& bareMarkerArray) 
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();

	// Setup input buffer from textStr
	int nTheLen = textStr.Length();
	// wx version: pBuffer is read-only so just get pointer to the string's read-only buffer
	const wxChar* pBuffer = textStr.GetData();
	wxChar* pBufStart = (wxChar*)pBuffer;		// save start address of Buffer
	int itemLen = 0;
	wxChar* pEnd = pBufStart + nTheLen;// bound past which we must not go
	wxASSERT(*pEnd == _T('\0')); // ensure there is a null at end of Buffer
	bool bHitMkr = FALSE;
	bool bIsAMarker = FALSE;

	// Setup copy-to buffer from textStr2.
	wxString textStr2;
	//wxChar* pBuffer2;
	wxChar* pEnd2;
	// This buffer does not have, nor need a null char placed at the end of it. We assume
	// being the same as pBuffer's size would be sufficient for all cases. Moreover, the
	// pointer pNew only checks for a null char in the pBuffer, not this one.
	// whm 8Jun12 modified to use wxStringBuffer
	// Create the wxStringBuffer in a specially scoped block. This is crucial here
	// in this function since the wxString textStr2 is accessed directly within
	// this function in the return textStr2; statement at the end of the function.
	{ // begin special scoped block
		wxStringBuffer pBuffer2(textStr2,nTheLen + 1);
		//pBuffer2 = textStr2.GetWriteBuf(nTheLen + 1);
		pEnd2 = pBuffer2 + nTheLen;
		*pEnd2 = (wxChar)0;

		wxString bareMarkerForLookup, bareMarkerInInputArray, wholeMarker;
		//int nMarkersInArray = bareMarkerArray.GetCount();

		wxChar* pOld = pBufStart;  // source
		wxChar* pNew = pBuffer2; // destination
		while (*pOld != (wxChar)0)
		{
			bIsAMarker = pDoc->IsMarker(pOld);
			if (bIsAMarker)
			{
				// We're pointing at a marker. Look it up and see if we should skip over it and
				// its associated text
				wholeMarker = pDoc->GetWholeMarker(pOld); // the whole marker including backslash and any ending *

				if (wholeMarker == _T("\\"))
			{
					*pNew++ = *pOld++;

				// The tests below don't make sense for an isolated non-marker backslash,
				// and we shouldn't set bHitMarker to TRUE, so just continue processing at
					// the top of the while loop
					continue;
				}

				bareMarkerForLookup = pDoc->GetBareMarkerForLookup(pOld); // strips off backslash and any ending *
				// if we encounter an end marker before its beginning form (as might happen if we
				// are using the function on an m_endMarkers string , we want to check it
				// for filtering in isolated fashion without parsing to find any associated text.
				if (wholeMarker.Find(_T('*')) != wxNOT_FOUND && !bHitMkr)
				{
					// we hit this end marker before seeing a beginning marker in the input text
					// if its non-end form it to be filtered we'll parse over it and omit it from
					// the text, otherwise we'll leave it alone
					if (IsBareMarkerInArray(bareMarkerForLookup, bareMarkerArray))
					{
						itemLen = pDoc->ParseMarker(pOld);
						pOld += itemLen;
						// don't increment pNew here
						itemLen = pDoc->ParseWhiteSpace(pOld);
						pOld += itemLen; // and don't increment pNew here
						continue; // iterate
					}
				}

				bHitMkr = TRUE;
				// for output filter purposes we treat all \bt... initial markers as simple \bt, so
				// change any \bt... ones to just \bt
				if (bareMarkerForLookup.Find(_T("bt")) == 0)
				{
					bareMarkerForLookup = _T("bt"); // change all \bt... initial ones to just \bt
				}
				if (IsBareMarkerInArray(bareMarkerForLookup, bareMarkerArray))
				{
					// This marker needs to be filtered out/skipped over. We have to parse over
					// the marker and its associated text and any end marker it may have. In the
					// process we increment pOld but not pNew.

					wxString wholeMkrNoAsteriskSp = _T('\\') + bareMarkerForLookup + _T(' ');
					// charFormatMkrs is defined and populated near top of Adapt_ItView.cpp
					if (charFormatMkrs.Find(wholeMkrNoAsteriskSp) != wxNOT_FOUND)
					{
						// we are filtering one of the character markers that affects font formatting
						// we don't want to filter out the associated text, only remove the formatting
						// i.e., by removing the markers and leaving the associated text
						// parse the initial marker
						itemLen = pDoc->ParseMarker(pOld);
						pOld += itemLen;
						// don't increment pNew here
						itemLen = pDoc->ParseWhiteSpace(pOld);
						pOld += itemLen;
						// don't increment pNew here
						// parse the associated text until we come to the end marker
						while (pOld < pEnd && !pDoc->IsCorresEndMarker(wholeMarker, pOld, pEnd))
						{
							// just copy whatever we are pointing at and then advance
							*pNew++ = *pOld++;
						}
						if (pOld < pEnd)
						{
							// we found a corresponding end marker to we need to parse it too
							itemLen = pDoc->ParseMarker(pOld);
							pOld += itemLen;
							// don't increment pNew here

							// parse remaining whitespace but preserve new lines
							if (*pOld == _T('\n'))
							{
								// just copy the new line and then advance
								*pNew++ = *pOld++;
							}
							else
							{
								itemLen = pDoc->ParseWhiteSpace(pOld);
								pOld += itemLen;
								// don't increment pNew here
							}
						}
					}
					else
					{
						// we are filtering both marker(s) and associated text
						itemLen = ParseMarkerAndAnyAssociatedText(pOld, pBufStart, pEnd,
											bareMarkerForLookup, wholeMarker, FALSE, FALSE);
						// 1st FALSE above means don't consider this text to be RTF text
						// 2nd FALSE above means don't include char format markers

						pOld += itemLen; // advance pOld past marker and its associated text
						// don't increment pNew for the skipped marker
						// if after parsing the marker we're now pointing at a new line, preserve it

						// parse any remaining white space but preserve new lines
						// BEW 18Apr2011 -- just preserving newline means that the one and
						// only space between the endmarker and the following text is also
						// removed, and if there was not a space before the begin marker, this
						// makes a bogus coalescence of two words - so we need to retain at
						// least one space
						if (*pOld == _T('\n') || *pOld == _T(' '))
						{
							// just copy the new line or one space and then advance over it
							*pNew++ = *pOld++;
						}
						//else
						// remove any additional spaces
						if (*pOld == _T(' '))
						{
							itemLen = pDoc->ParseWhiteSpace(pOld);
							pOld += itemLen;
							// don't increment pNew here
						}
					}
				}
				else
				{
					// not filtering
					// just copy whatever we are pointing at and then advance
					*pNew++ = *pOld++;
				}
			}
			else
			{
				// it's not a marker but text
				// just copy whatever we are pointing at and then advance
				*pNew++ = *pOld++;
			}
		}
		*pNew = (wxChar)0; // add a null at the end of the string in pBuffer2
	} // end of special scoping block
	//textStr2.UngetWriteBuf(); // whm 8Jun12 removed - not used with wxStringBuffer
	return textStr2;
}


// whm added the following to eliminate problems in some legacy functions. The exact format
// that results of the sfm file should be nearly identical to that produced by the legacy
// MFC version. One difference noted is that the wx version currently leaves any extra
// spaces at end of lines before eol, whereas the MFC version left them on text lines, but
// removed them at the ends of markers that have no associated text. There is no
// detrimental effects either way.
// whm 17Sep11 modified to ensure that the buffer for output ends with an eol
void FormatMarkerBufferForOutput(wxString& text, enum ExportType expType)
{
	// remove any whitespace from the beginning of the string, and end
	text.Trim(FALSE);
	// BEW 13Jul11, wrapped the text.Trim() line in a test - when we have a doc parsed
	// from contentless USFM from Paratext, the last bit of text will be "...\v 13 " if
	// verse 13 happens to be the last verse parsed when forming the doc. We don't want
	// that last space trimmed off for such data, because we want exports to preserve it.
	// We don't have access to CSourcePhrase instances here, so just refrain from the
	// Trim() when exporting source text, as this is the situation we are supporting for
	// PT contentless USFM data. (we could be more clever, and test for the text ending in
	// a \v or \vn followed by space and digits or letters then space - but all that is a
	// lot for something that matters little, so I won't bother)
	if (expType != sourceTextExport)
	{
		text.Trim();
	}

	// whm 17Sep11 added: Test if text ends with an eol. If not, add the appropriate eol 
	// at the end of the text. This is needed especially for texts that are transferred 
	// back to PT/BE during chapter sized text collaboration, otherwise we could be inserting 
	// text back into PT/BE which lacks an eol between the last verse of a chapter and the 
	// next \c n, that is, without an ending eol, Paratext would end up with something like 
	// this:
	// 
	// \c 2
	// \v 1 This is the first verse of chapter 1
	// ...
	// \v 20 This is the last verse of chapter 2.\c 3
	// \v 1 This is the first verse of chapter 3.
	// ...
	// 
	// where the \c 3 ends up concatenated to the end of the last verse of the previous 
	// chapter, instead of starting on a new line.
	if ((gpApp->m_bCollaboratingWithParatext || gpApp->m_bCollaboratingWithBibledit) && gpApp->m_bCollabByChapterOnly)
	{
		int textLen;
		textLen = text.Length();
		if (text.GetChar(textLen - 1) != _T('\n') || text.GetChar(textLen - 1) != _T('\r'))
		{
			// the text does not end with either \n or \r eol, so add the appropriate
			// eol at the end of the text.
			text += gpApp->m_eolStr;
		}
	}

	// FormatMarkerBufferForOutput assumes the complete text to be output as a text file is
	// present in str. It adds end-of-line characters before all standard format markers except
	// for those which have the inLine attribute. It is not possible to completely reconstruct
	// the line breaks as they were in the input text. Instead this routine makes the output
	// fairly easy to read, allowing the inLine markers and associated text to remain embedded
	// within the lines of the text.
	// FormatMarkerBufferForOutput makes the following assumptions:
	// 1. End markers never have eol char(s) inserted preceding them regardless of their wrap attribute.
	// 2. Non-end markers generally get eol char(s) inserted preceding them if they do NOT have the
	//    inLine="1" attribute in AI_USFM.xml.
	// 3. Markers which are not preceded by eol char(s) are preceded by a space, except for the
	//    following which in which no space is preceding them: \f, \fe; however, if there is
	//    punctuation preceding the marker, no space should be inserted (it would detach
	//    the punctuation from the word which follows - if the marker is one which binds
	//    closely to the word, such as one of the character formatting markers --- those
	//    in the 'special set' of USFM
	// See also notes below.
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	int curMkrPos = 0;
	int len = text.Length();
	// Since we require a read-only buffer for our main buffer we
	// use GetData which just returns a const wxChar* to the data in the string.
	const wxChar* pBuff = text.GetData();
	wxChar* pBufStart = (wxChar*)pBuff;
	wxChar* pEnd = pBufStart + len;
	wxASSERT(*pEnd == _T('\0'));

	// For a copy-to buffer we'll base it on text2 with a double-sized buffer
	wxString text2;
	// Our copy-to buffer must be writeable so we must use GetWriteBuf() for it
	// whm 8Jun12 modified to use wxStringBuffer
	// Create the wxStringBuffer in a specially scoped block. This is crucial here
	// in this function since the wxString text2 is accessed directly within
	// this function in text = text2; statement at the end of the function.
	{ // begin special scoped block
		wxStringBuffer pBuff2(text2,len*2 + 1);
		//wxChar* pBuff2 = text2.GetWriteBuf(len*2 + 1);
		//wxChar* pBufStart2 = pBuff2;
		//wxChar* pEnd2;
		//pEnd2 = pBufStart2 + len*2;
		wxChar* pOld = pBufStart;
		wxChar* pNew = pBuff2;

		wxString wholeMkr;
		wholeMkr.Empty();
		// BEW 11Oct10, changed signature so as to match the punctuation used to the export
		// type - formerly a target text export was using source language's punctuation.
	//
		// Get a spaceless array of the appropriate language's punctuation characters
		wxString spacelessPuncts;
		if (expType == sourceTextExport)
		{
			spacelessPuncts = gpApp->m_punctuation[0]; // 0 for source language,
		}
		else
		{
			// we'll assume the target language's punctuation should also be used for exports
			// of gloss text, and free translation, at least until someone complains
			spacelessPuncts = gpApp->m_punctuation[1]; // 1 for target language
		}
		while (spacelessPuncts.Find(_T(' ')) != -1)
		{
		spacelessPuncts.Remove(spacelessPuncts.Find(_T(' ')),1);
			// used in DetachedNonQuotePunctuationFollows() below
		}
		// whm 8Jun12 modified for wxWidgets-2.9.3 wxStrlen_() is invalid, use wxStrlen()
		int lenEolStr = wxStrlen(gpApp->m_eolStr); //int lenEolStr = wxStrlen_(gpApp->m_eolStr);
		bool bDetachedNonquotePunctuationFollows = FALSE;
		//bool IsWrapMarker = FALSE;
		bool IsInLineMarker = FALSE;
		int itemLen; // gets length of parsed item
		int ctmkr;   // used to count item chars while copying from pOld to pNew
		while (*pOld != (wxChar)0 && pOld < pEnd)
		{
			// scan the whole text for standard format markers
			//if (pDoc->IsMarker(pOld, pBufStart))
			if (pDoc->IsMarker(pOld))
			{
				// we are pointing at a marker; get the marker into the wholeMkr
				wholeMkr = pDoc->GetWholeMarker(pOld);
				wholeMkr = MakeReverse(wholeMkr);

				// get the length of the marker
				itemLen = pDoc->ParseMarker(pOld);
				// check integrity of results from ParseMarker
				wxASSERT(itemLen > 0);
				wxASSERT(itemLen == (int)wholeMkr.Length());

				// We handle specific markers that need to have end-of-line char(s)
			// inserted before them, or that need to have a certain kind of
				// spacing before or after them for properly formatted output. These
				// specific markers include:
				//    chapter markers \c n (no space after the number)
				//    end markers: never a space before end markers
				//      (tuck puncts left to be adjacent to end marker)
			// All other markers encountered get end-of-line chars inserted before
				// them except for the following situations:
				//    Don't add eol char(s) if the marker is at beginning of buffer
			//    Don't add eol char(s) if the marker is an inline marker
				//    (defined in AI_USFM.xml)
				//    Don't added eol char(s) if a [ bracket precedes
				if (pDoc->IsEndMarker(pOld,pEnd))
				{
					// wholeMkr is an end marker, so after wholeMkr is a place where
					// white space should be removed so as to tuck up following detached
					// non-quote punctuation to the end of the endmarker
					// BEW 11Oct10, if docVersion 5 is coded right, this next test should never
					// require any tucking up, our code should never have a space after the
					// endmarker. Period. But no harm to keep this as it would correct a the
					// situation if a bogus space somehow crept in.
					wholeMkr = MakeReverse(wholeMkr);
					wxChar* pPosAfterMkr = pOld;
				pPosAfterMkr += wholeMkr.Length(); // make curMkrPos be at next
													   // char after endmarker
					bDetachedNonquotePunctuationFollows = DetachedNonQuotePunctuationFollows(
													pOld,pEnd,pPosAfterMkr,spacelessPuncts);
					// parse the end marker
					itemLen = pDoc->ParseMarker(pOld);
					for (ctmkr = 0; ctmkr < itemLen; ctmkr++)
					{
						*pNew++ = *pOld++;
					}
					curMkrPos = (int)(pOld - pBufStart); // update value
					// parse through any white space; but if bDetachedNonquotePunctuationFollows
					// is true, do not add the white space to the new buffer to effect the tuck
					// with any following detached non-quote punctuation.
					itemLen = pDoc->ParseWhiteSpace(pOld);
					if (bDetachedNonquotePunctuationFollows)
					{
						for (ctmkr = 0; ctmkr < itemLen; ctmkr++)
							pOld++;
					}
					curMkrPos = (int)(pOld - pBufStart); // update value
				}
				else
				{
					// It is some other marker besides a chapter marker or an end marker;
				// if we're not at the beginning of the file, insert the end-of-line
					// char(s) before the marker, except for certain conditions (see below).

					// we don't do anything if the marker is at the start of the buffer
					if (pOld != pBufStart)
					{
						// determine if the marker is an inline marker
						wholeMkr = MakeReverse(wholeMkr);
						//IsWrapMarker = FALSE;
						IsInLineMarker = FALSE;
						switch(gpApp->gCurrentSfmSet)
						{
						case UsfmOnly:
							{
								//if (gpApp->UsfmWrapMarkersStr.Find(wholeMkr + _T(' ')) != -1)
								//	IsWrapMarker = TRUE;
								if (gpApp->UsfmInLineMarkersStr.Find(wholeMkr + _T(' ')) != -1)
									IsInLineMarker = TRUE;
								break;
							}
						case PngOnly:
							{
								//if (gpApp->PngWrapMarkersStr.Find(wholeMkr + _T(' ')) != -1)
								//	IsWrapMarker = TRUE;
								// whm ammended 23Nov07 to consider PngOnly footnote end
								// markers \fe and \F as inline markers (even if they are not
								// marked so in AI_USFM.xml), so they won't get a new line
								// inserted before them, and be formatted as other inline
								// markers.
								if (gpApp->PngInLineMarkersStr.Find(wholeMkr + _T(' ')) != -1
									|| wholeMkr + _T(' ') == _T("\\fe ") || wholeMkr + _T(' ') == _T("\\F "))
									IsInLineMarker = TRUE;
								break;
							}
						case UsfmAndPng:
							{
								//if (gpApp->UsfmAndPngWrapMarkersStr.Find(wholeMkr + _T(' ')) != -1)
								//	IsWrapMarker = TRUE;
								if (gpApp->UsfmAndPngInLineMarkersStr.Find(wholeMkr + _T(' ')) != -1)
									IsInLineMarker = TRUE;
								break;
							}
						default:
							{
								//if (gpApp->UsfmWrapMarkersStr.Find(wholeMkr + _T(' ')) != -1)
								//	IsWrapMarker = TRUE;
								if (gpApp->UsfmInLineMarkersStr.Find(wholeMkr + _T(' ')) != -1)
									IsInLineMarker = TRUE;
							}
						}
						// insert any eol char(s) needed for current marker here. In this block
						// the marker cannot be an end marker, nor located at the beginning of
						// the file. We will always add eol unless the marker's attribute is
						// inLine; but if a [ precedes the marker, we'll tuck the marker up
						// against the [  and put the eol char(s) preceding the [ bracket
						// BEW 13Jul11: add code to check for "\v<sp>number<sp>" preceding the
						// pOld pointer, and set a flag if so, because we'll keep the final
						// space in such a circumstance - it's likely an export for Paratext
						// or Bibledit contentless usfm markup where we want the verse number
						// to be followed by a space (handle \vn similarly)
						bool bKeepLineFinalSpace = FALSE;
						bKeepLineFinalSpace = KeepSpaceBeforeEOLforVerseMkr(pOld);
						if (!IsInLineMarker)
						{
							if (*(pOld - 1) == _T('['))
							{
								// remove the [ we've already added to the new buffer
								--pNew;
								// now add the eol char(s) to the new buffer, lenEolStr is 2 for
								// windows, 1 for mac or linux
								int cteol;
								for (cteol = 0; cteol < lenEolStr; cteol++)
								{
									*pNew = gpApp->m_eolStr.GetChar(cteol);
									pNew++;
								}
								// put the [ bracket back after the eol char(s)
								*pNew = _T('[');
								pNew++;
								// now the marker will be immediately following the [ bracket

							}
							else
							{
								// Backslashes may initiate markers anywhere in the file - after
								// new lines, or in the midst of text lines. when inserting eolStr,
								// if preceding char is a space, first remove that space so there
								// won't be any spaces dangling at ends of lines preceding the
								// newly inserted eol char(s). I'll remove any spaces that managed
								// to creep in, as they are not needed.
								// BEW 13Jul11, wrapped it with a test for FALSE for the
								// bKeepLineFinalSpace which is set or cleared above; the idea
								// is that \v<space>versenum<space> keeps the final space and
								// eol is added after it - to support Paratext empty USFM
								// markup chapter or book files, exports thereof
								if (!bKeepLineFinalSpace)
								{
									while (*(pNew-1) == _T(' '))
									{
										--pNew;
									}
								}
								// now add the eol char(s) to the new buffer, lenEolStr is 2 for
								// windows, 1 for mac or linux
								int cteol;
								for (cteol = 0; cteol < lenEolStr; cteol++)
								{
									*pNew = gpApp->m_eolStr.GetChar(cteol);
									pNew++;
								}
							}
						}
						else if (IsInLineMarker && curMkrPos > 0 && text.GetChar(curMkrPos-1) != _T(' '))
						{
							// The marker is inline and there is no space preceding the marker.
							// We want to have a preceding space, (but not if punctuation
							// precedes, and not if an endmarker precedes) unless the marker is
							// \f or \fe, and not if fixed space ~ precedes. The following
							// ensures that there is a space separating non-end inLine markers
							// from what preceeds, providing the inLine marker is not the first
							// thing in the file, and providing the inLine marker is not a
							// footnote \f, or endnote \fe, and providing punctuation is not
							// preceding the space.
							// BEW 11Oct10, needed to remove \x from the test, because in
							// proper USFM markup style, \x follows a verse marker and its
							// trailing space, so we don't want \x to prevent putting on in if
							// it is absent
							if (spacelessPuncts.Find(text.GetChar(curMkrPos-1)) == wxNOT_FOUND &&
								text.GetChar(curMkrPos-1) != _T('~') &&
								text.GetChar(curMkrPos-1) != _T('*'))
							{
								// only do this attempt at space insertion prior to the marker
								// provided that the character preceding the marker is not one
								// of the punctuation characters nor ~
							if (wholeMkr != _T("\\f")
									&& wholeMkr != _T("\\fe"))
								{
									// only insert the space provided the marker is not \f or \fe
									*pNew = _T(' ');
									pNew++;
								}
							}
						}
						else if ((IsInLineMarker && curMkrPos > 0 && text.GetChar(curMkrPos-1) == _T(' '))
							&& (wholeMkr == _T("\\f") || wholeMkr == _T("\\fe"))
						&& (*(pOld-2) != _T('0') || *(pOld-2) != _T('1') || *(pOld-2) != _T('2') ||
							*(pOld-2) != _T('3') || *(pOld-2) != _T('4') || *(pOld-2) != _T('5') ||
							*(pOld-2) != _T('6') || *(pOld-2) != _T('7') || *(pOld-2) != _T('8') ||
								*(pOld-2) != _T('9')) )
						{
							// a space crept in preceding \f or \fe, so back up over it,
							// provided what precedes the space is not a digit (eg. verse number)
							pNew--;
						}
					}
					// The above sub-blocks have added any necessary end-of-line char(s) and/or
					// spaces. Now we parse the marker, copying it from pOld to pNew
					itemLen = pDoc->ParseMarker(pOld);
					for (ctmkr = 0; ctmkr < itemLen; ctmkr++)
					{
						*pNew++ = *pOld++;
					}
					curMkrPos = (int)(pOld - pBufStart); // update value
				}
			} // end of if (IsMarker  )
			else
			{
				// Process the non-marker text.
				// just copy whatever we are pointing at and then advance
				*pNew++ = *pOld++;
				curMkrPos = (int)(pOld - pBufStart); // update value
			}
		} // end of while (*pOld != (wxChar)0 && pOld < pEnd)
		*pNew = (wxChar)0; // terminate the new buffer string with null char
	} // end of special scoping block
	//text2.UngetWriteBuf(); // whm 8Jun12 removed - not used with wxStringBuffer
	text = text2; // replace old string with new one
}

void FormatUnstructuredTextBufferForOutput(wxString& text, bool bRTFOutput)
{
	int nTextLength = text.Length();
	const wxChar* pBuff = text.GetData();
	wxChar* pBuffStart = (wxChar*)pBuff;
	wxChar* pEnd = pBuffStart + nTextLength;
	
	wxString text2;
	// whm 8Jun12 modified to use wxStringBuffer
	// Create the wxStringBuffer in a specially scoped block. This is crucial here
	// in this function since the wxString textStr2 is accessed directly within
	// this function in the text = text2; statement at the end of the function.
	{ // begin special scoped block
		wxStringBuffer pBuff2(text2,nTextLength + 1);
		//wxChar* pBuff2 = text2.GetWriteBuf(nTextLength + 1); // pBuff2 must be writeable
		wxChar* pBuffStart2 = pBuff2;
		//wxChar* pEnd2;
		//pEnd2 = pBuffStart2 + nTextLength; // copy-to buffer can be same size

		CAdapt_ItDoc* pDoc = gpApp->GetDocument();

	// Since we have unstructured data, we have the task of removing the temporarily
		// inserted paragraph sfms before we output. The MFC version originally assumed
	// that because text is unstructured data, there would be no sfms in the original,
		// and so we wouldn't have to worry about contextually defined sfms, and every marker
		// would be just a paragraph marker which is to be removed, and it already
		// follows a newline (see AddParagraphMarkers() in the doc class). The MFC version
		// couldn't trust CString because it didn't see any embedded newlines, so it copied
		// character by character. The wx version doesn't have the same problem with wxString
		// but we'll work with a read only and copy text to a writable buffer for speed.
		wxChar* pOld = pBuffStart; // source
		wxChar* pNew = pBuffStart2; // destination
		int itemLen; // whm added 26Nov07
		while (*pOld != (wxChar)0)
		{
			if (*pOld == _T('\n')) // this \n is in the buffer
			{
				// remove gSFescapeChar if it follows the newline, and the p plus space which
				// follows it, by skipping these three chars and continuing the copy after that
				*pNew++ = *pOld; // copy the newline
				pOld++; // point at whatever follows it
				if (*pOld == gSFescapechar)
				{
				// MFC note: we have a standard format marker, which must be a paragraph
					// one followed by a single space, so remove it and the space by jumping it.
					//pOld += 3;
					// whm modified 26Nov07 The original code assumed that the only sfms used
					// in unstructured text would be \p and incremented the pointer by 3 to
					// parse over the \p and following space. However, some texts have things
					// like \note markers in unstructured text (for example the Hebrew to Kazakh
					// project. I think we should preserve whatever markers are used so I've
					// modified the code to parse over any marker encountered. It is quite possible
					// that users would want to export their "unstructured" documents with notes
					// attached to them. The original code mangles markers like \note so they
					// get the front part chopped off to "te" in the source text. Failure to
				// properly parse over additional markers other than \p encountered in
					// our "unstructured" text will almost certainly make for problems when the
					// user wants to export the text, especially to RTF.
					if (bRTFOutput)
					{
						if (IsMarkerRTF(pOld,pBuffStart))
						{
							itemLen = ParseMarkerRTF(pOld,pEnd);
							wxString wholeMkr(pOld,itemLen);
							// jump over the marker only if it is \p
							if (wholeMkr == _T("\\p"))
							{
								pOld += itemLen;
								itemLen = pDoc->ParseWhiteSpace(pOld);
								pOld += itemLen;
							}
							else
							{
								int ct;
								for (ct = 0; ct < itemLen; ct++)
								{
									*pNew++ = *pOld++; // copy the marker
								}
							}
						}
					}
					else // not RTF output
					{
						//if (pDoc->IsMarker(pOld,pBuffStart)) // use the non-RTF function
						if (pDoc->IsMarker(pOld)) // use the non-RTF function
						{
							itemLen = ParseMarkerRTF(pOld,pEnd);
							wxString wholeMkr(pOld,itemLen);
							// jump over the marker only if it is \p
							if (wholeMkr == _T("\\p"))
							{
								pOld += itemLen;
								itemLen = pDoc->ParseWhiteSpace(pOld);
								pOld += itemLen;

								// BEW added 13Dec10, since blank lines in the original file
								// turn up as additional \p markers in m_markers, any
								// additional \p after the first should be returned again to
								// being newlines
								while (*pOld == gSFescapechar && *(pOld + 1) == _T('p'))
								{
									// found another \p marker
									*pNew++ = _T('\n');
									pOld += 2;
									while (*pOld == _T(' '))
									{
										pOld++;
									}
								}
							}
							else
							{
								// preserve the marker since it is not \p
								int ct;
								for (ct = 0; ct < itemLen; ct++)
								{
									*pNew++ = *pOld++; // copy the marker
								}
							}
						}
					}
				}
			}
			else
			{
				// just copy whatever we are pointing at and then advance
				*pNew++ = *pOld++;
			}
		}
		*pNew = (wxChar)0; // add a null at the end of the string in pBuff2
	} // end of special scoping block
	//text2.UngetWriteBuf(); // whm 8Jun12 removed - not used with wxStringBuffer
	text = text2; // return the modified data
}

// BEW added 06Oct05 for support of free translation propagation across an export of the target text
// and subsequent import into a new project
////////////////////////////////////////////////////////////////////////////////////////
/// \return             a 1-based count of the number of words of either source text, 
///                     or target text, in the section
///	\param bCountInTargetText	->	TRUE if the count is to made in the target text;  
///	                                FALSE has it done in the source text instead
///	\param pList			    ->	pointer to the document's m_pSourcePhrases list 
///	                                of pSrcPhrase pointers
///	\param nAnchorSequNum		->	index to the CSourcePhrase in the pList list which
///	                                is the anchor for the (considered-to-be-filtered) 
///                                 \free ... \free* section - although in doc version 5
///                                 the free translation is stored without any wrapping
///                                 \free or \free* markers. Note: there is no
///                                 determinate relation between the number of words to
///                                 be counted, and the number of words in the content
///                                 delimited by \free and \free*)
/// \remarks  
/// Called only when exporting either the source text as (U)SFM plain text, or the target
/// text as the same file type. We need to store the returned count in the exported
/// material which occurs between the \free and \free* markers, so that if that file of
/// text is subsequently used as source text for creating a document in another project, we
/// will be able to extract the word counts and use them to set the m_bEndFreeTrans boolean
/// in pSrcPhrase to TRUE so as to define where the end of that particular section of free
/// translation occurs.
/// BEW 31Mar10, updated for support of doc version 5 (no changes needed)
/////////////////////////////////////////////////////////////////////////////////////////
int CountWordsInFreeTranslationSection(bool bCountInTargetText, SPList* pList, int nAnchorSequNum)
{
	int nCount = 0;
	int countFromPhrase = 0; // whm initialized to zero
	SPList::Node* anchorPos = pList->Item(nAnchorSequNum);
	wxASSERT(anchorPos);
	SPList::Node* pos = anchorPos;
	wxString phrase;
	CSourcePhrase* pSrcPhrase;
	pSrcPhrase = (CSourcePhrase*)anchorPos->GetData();
	// if the anchor location is also the end of the free translation section, we look only
	// at this one CSourcePhrase instance which stores it
	if (pSrcPhrase->m_bEndFreeTrans)
	{
		if (bCountInTargetText)
		{
			phrase = pSrcPhrase->m_targetStr;
			if (phrase.IsEmpty())
				return 0;
			countFromPhrase = GetWordCount(phrase,NULL); // NULL because we don't want 
														 // a word list returned
		}
		else
		{
			return pSrcPhrase->m_nSrcWords;
		}
	}
	// if we get to here, then the anchor location's pSrcPhrase is not the one and only
	// sourcephrase in the current section of free translation, so process all
	// sourcephrases in the section, but count only those which are not placeholders
	// because those won't be in a source text export
	while (pos != NULL)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase != NULL);
		if (bCountInTargetText)
		{
			// do the counting using the phrase or word in m_targetStr members
			phrase = pSrcPhrase->m_targetStr;
			if (phrase.IsEmpty())
				goto a;
			countFromPhrase = GetWordCount(phrase,NULL); // NULL because we don't want 
														 // a word list returned
		}
		else
		{
			// do the counting using the phrase or word in m_srcPhrase members (but not
			// placeholders) -- actually we will just use the m_nSrcWords member, since
			// this has the count already
			if (!pSrcPhrase->m_bNullSourcePhrase)
				countFromPhrase = pSrcPhrase->m_nSrcWords;
		}
		nCount += countFromPhrase;
		countFromPhrase = 0; // BEW added 8Apr10, otherwise the previous value is added 
							 // when at a placeholder

		// if we are at the m_bEndFreeTrans == TRUE location then break out
a:		if (pSrcPhrase->m_bEndFreeTrans)
			break;
	}
	return nCount;
}

// Note: This is a modification of Bruce's function of the same name, that can be employed
// in the DoExportInterlinearRTF function. This modified version differs from the original
// function in that it also composes and returns a source string (Sstr), a gloss string
// (Gstr), and a navigation text string (Nstr), [all by reference] in addition to the
// target text (Tstr) that the original non-overloaded version returned. Hence, with
// respect to the returned value of the target string Tstr, this function is identical to
// original function. The return of the additional string values is simply ignored in
// RebuildTargetText().
// whm Revised 10Nov05
// BEW note 31Mar10: despite the above comment, this function is only called in
// RebuildTargetText() and so the collection of Sstr, Gstr, Nstr currently is never used.
// 
// BEW 1Apr10, completely rewritten, with elimination of 5 globals, and encapsulation of
// the dialog for placement, which needs to be called if there were medial markers in the
// retranslation, and supporting doc version 5
// 
// BEW 11Oct10, more changes for doc version 5 to support m_follOuterPunct & inline
// markers
// 
// NOTE****************** there are comments in OnButtonRetranslation() before the call of
// BuildRetranslationSourcePhraseInstances() which explain the protocols to be used for
// supporting export of retranslation data & they impinge on what happens below.
// 
// Here is the text of that comment: do not delete it
// 
// **** Legacy comment -- don't delete, it documents how me need to make changes ****
// copy the retranslation's words, one per source phrase, to the constituted sequence of
// source phrases (including any null ones) which are to display it; but ignore any markers
// and punctuation if they were encountered when the retranslation was parsed, so that the
// original source text's punctuation settings in the document are preserved. Export will
// get the possibly new punctuation settings by copying m_targetStr, so we do not need to
// alter m_precPunct and m_follPunct on the document's CSourcePhrase instances.
// 
// *** New comment, 11Oct10, for support of new doc version 5 storage members,
// m_follOuterPunct and the four inline markers' wxString members. The legacy approach
// above fails to handle good punctuation handling because we need to support punctuation
// following endmarkers as well as before them. In the retranslation, we ask the user to
// type punctuation where it needs to be - that won't be in the same places as in the
// source text of the selection; but the punctuation typed will be parsed by
// TokenizeTextString() above into only two members, m_precPunct and m_follPunct, so
// m_follOuterPunct will be ignored. And since it is assured that the user won't type
// markers when doing the retranslation we then have the problem of how to get the markers
// into the right places when an SFM export of the target text is asked for. Our solution
// to this dilemma is the following:
// (1) When exporting pSrcPhrase instances NOT in a retranslation, the punctuation and
// markers are reconstituted from the pSrcPhrase members by algorithm.
// (2) When exporting pSrcPhrase instances in a retranslation, the m_targetStr member is
// taken "as is" and the pSrcPhrase members for storing puncts (those will be source text
// puncts, and not in the right places anyway) will be ignored, and so what is on
// m_targetStr is used - just as the user typed it in the retranslation. The code for
// building the output SFM target text will work out which markers are "medial" to
// the retranslation, and will present those to the user in a placement dialog - so he then
// can place each marker in the place where he deems it should be - in this way, he can
// re-establish, say, an inline endmarker between two consecutive 'following' punctuation
// characters.
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
// BEW 11Oct10, changed for support of additional docV5 storage in CSourcePhrase
SPList::Node* DoPlacementOfMarkersInRetranslation(SPList::Node* firstPos,
									SPList* pSrcPhrases, wxString& Tstr)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	SPList::Node* pos = firstPos;
	wxASSERT(pos != 0);
	bool bHasInternalMarkers = FALSE; // assume none for default
	SPList::Node* savePos = NULL; // whm initialized to NULL

	// undo what Bill did, they are not used
	wxString Sstr; // needed only for the placement dialog, not for returning to caller
	//wxString Gstr; // unused, so we can remove this
	//wxString Nstr; // ditto

	// markers needed, since doc version 5 doesn't store some filtered 
	// stuff using them
	wxString freeMkr(_T("\\free"));
	wxString freeEndMkr = freeMkr + _T("*");
	wxString noteMkr(_T("\\note"));
	wxString noteEndMkr = noteMkr + _T("*");
	wxString backTransMkr(_T("\\bt"));
	// prefixStr is a place where we'll temporarily store any filtered information from the
	// first CSourcePhrase of the retranslation, and withhold it from the placement dialog,
	// as the user doesn't need to see any of that filtered stuff when doing the medial
	// marker placements. Any retranslation-internal filtered stuff has to be put "in
	// place" within the target text being composed, however, and so will get shown in the
	// placement dialog (but in nearly all circumstances there will never be any such stuff
	// in a retranslation, so no big deal)
	wxString markersPrefix; // hide initial filtered and markers stuff in this
							// temporarily and prefix them after the dialog closes
	markersPrefix.Empty();
	wxArrayString markersToPlaceArray; // accumulate marker strings here, for transfer to dialog
	markersToPlaceArray.Empty(); 
	wxString finalSuffixStr; finalSuffixStr.Empty(); // put collected-string-final endmarkers here
	bool bFinalEndmarkers = FALSE; // set TRUE when finalSuffixStr has content to be added at loop end

	wxString retranstr = _("retranslation"); // make this localizable
	wxString aSpace = _T(" ");
	wxString markersStr; 
	wxString endMarkersStr;
	wxString freeTransStr;
	wxString noteStr;
	wxString collBackTransStr;
	wxString filteredInfoStr;
	wxString unfilteredStr; // any unfiltered medial stuff which has to be shown in the
							// Place... dialog can be accumulated in this local string on
							// a per-pSrcPhrase basis (emptied prior to each iteration)
	// loop over each CSourcePhrase instance in the retranslation
	bool bFirst = TRUE;
	wxString nBEMkrs; // for non-binding endmarkers
	wxString bEMkrs; // for binding endmarkers
	while (pos != 0)
	{
		savePos = pos; // savePos is what we return to the caller
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
/*
#ifdef __WXDEBUG__
		if (pSrcPhrase->m_nSequNumber == 367)
		{
			int halt_here = 1;
		}
#endif
*/
		pos = pos->GetNext();
		// break out of the loop if we reach the end of the retranslation, or if we reach
		// the beginning of an immediately following (but different) retranslation
		if (!pSrcPhrase->m_bRetranslation || ((pSrcPhrase->m_bRetranslation && 
				pSrcPhrase->m_bBeginRetranslation && savePos != firstPos)))
		{
			break;
		}
		else
		{
			// empty the scratch strings
			EmptyMarkersAndFilteredStrings(markersStr, endMarkersStr, freeTransStr, noteStr,
											collBackTransStr, filteredInfoStr);
			// get the other string information we want, putting it in the scratch strings
			GetMarkersAndFilteredStrings(pSrcPhrase, markersStr, endMarkersStr,
							freeTransStr, noteStr, collBackTransStr, filteredInfoStr);
			// remove any filter bracketing markers if filteredInfoStr has content
			if (!filteredInfoStr.IsEmpty())
			{
				filteredInfoStr = pDoc->RemoveAnyFilterBracketsFromString(filteredInfoStr);
			}

			// BEW added 22Feb11, \x ... \x* material should follow anything in m_markers,
			// so extract it from filteredInfoStr (if present) and put it in a separate
			// crossRefs string, for later placement (we append it to markersStr so that
			// when the latter is placed in location, the crossrefs go with it)
			wxString crossRefs; crossRefs.Empty();
			wxString tempStr1;
			wxString tempStr2;
			int anOffset = filteredInfoStr.Find(_T("\\x "));
			if (anOffset != wxNOT_FOUND)
			{
				tempStr1 = filteredInfoStr.Left(anOffset);
				crossRefs = filteredInfoStr.Mid(anOffset);
				int endOffset = crossRefs.Find(_T("\\x*"));
				if (endOffset != wxNOT_FOUND)
				{
					tempStr2 = crossRefs.Left(endOffset + 3);
					wxString remStr = crossRefs.Mid(endOffset + 3); // could be empty
					filteredInfoStr = tempStr1;
					if (!remStr.IsEmpty())
					{
						filteredInfoStr += remStr;
					}
					crossRefs = tempStr2;
				}
			}
			// attach crossRefs to markersStr now
			if (!crossRefs.IsEmpty())
			{
				markersStr += crossRefs;
			}

			// we compose the pre-user-edit form of the target string, and the source
			// string, and the gloss and nav strings, even though the app doesn't yet use
			// the latter two
			if (bFirst)
			{
				bFirst = FALSE; // prevent this block from being re-entered

				// for the first CSourcePhrase, we store any filtered info within the
				// prefix string, as we'll not show that stuff in the Place... dialog;
				// likewise for free translation, or a note, or collected back translation
				// on the first CSourcePhrase instance; and any content in m_markers -
				// this, if present, must come after all that, then, 
				// 11Oct10 additions, if m_inlineNonbindingMarkers has content, it is
				// added, (but m_inlinBindingMarkers content is added to the placement
				// list as it helps for the user to see all such & handle matched
				// beginmarker & endmarker pairs the same way)
				// remove LHS whitespace when done
				if (!filteredInfoStr.IsEmpty())
				{
					// this data has any markers and endmarkers already 'in place'
					markersPrefix.Trim();
					markersPrefix += filteredInfoStr;
				}
				if (!collBackTransStr.IsEmpty())
				{
					// add the marker too
					markersPrefix.Trim();
					markersPrefix += backTransMkr;
					markersPrefix += aSpace + collBackTransStr;
				}
				if (!freeTransStr.IsEmpty())
				{
					markersPrefix.Trim();
					markersPrefix += aSpace + freeMkr;

					// BEW addition 06Oct05; a \free .... \free* section pertains to a
					// certain number of consecutive sourcephrases starting at this one if
					// m_freeTrans has content, but the knowledge of how many
					// sourcephrases is marked in the latter instances by which ones have
					// the m_bStartFreeTrans == TRUE and m_bEndFreeTrans == TRUE, and if we
					// just export the filtered free translation content we will lose all
					// information about its extent in the document. So we have to compute
					// how many target words are involved in the section, and store that
					// count in the exported file -- and the obvious place to do it is
					// after the \free marker and its following space. We will store it as
					// follows: |@nnnn@|<space> so that we can search for the number and
					// find it quickly and remove it if we later import the exported file
					// into a project as source text. 
					// (Note: the following call has to do its word counting in the SPList,
					// because only there is the filtered information, if any, still hidden
					// and therefore unable to mess up the word count.)
					int nWordCount = CountWordsInFreeTranslationSection(TRUE,pSrcPhrases,
							pSrcPhrase->m_nSequNumber); // TRUE means 'count in tgt text'
					// construct an easily findable unique string containing the number
					wxString entry = _T("|@");
					entry << nWordCount; // converts int to string automatically
					entry << _T("@| ");
					// append it after a delimiting space
					markersPrefix += aSpace + entry;

					// now the free translation string itself & endmarker
					markersPrefix += aSpace + freeTransStr;
					markersPrefix += freeEndMkr; // don't need space too
				}
				if (!noteStr.IsEmpty())
				{
					markersPrefix.Trim();
					markersPrefix += aSpace + noteMkr;
/* 
					// BEW 15Jun11, removed OXES support until it is needed somewhere
					// BEW 17Sep10, provide oxes with the m_targetStr as the referenced
					// word for the note stored here
					// BEW 19May12, reinstated OXES support, but not with support of \note
					// so leave this commented out (until such time as we changes our minds)
					if (gpApp->m_bOxesExportInProgress)
					{
						// 'numberOfChars' is not the number of characters in the note itself, but
						// rather the number of characters in the words of the adaptation phrase in the
						// m_targetStr member of this merged CSourcePhrase (oxes needs this info)
						int numberOfChars = pSrcPhrase->m_targetStr.Len(); // no space at end
						wxString numStr;
						numStr = numStr.Format(_T("%d"),numberOfChars);
						numStr = _T("@#") + numStr;
						numStr += _T(':'); // divider
						numStr += pSrcPhrase->m_targetStr;
						numStr += _T("#@");
						noteStr = numStr + noteStr;
						// the oxes parser must detect this @#nnn#@ substring and remove it, convert
						// it to int, and use it to count the phrase's length to which the note applies
						// so that the endOffset in the relevant NoteDetails struct can be set
						// correctly, and put the word after the colon into its wordsInSpan member
					}
*/
					markersPrefix += aSpace + noteStr;
					markersPrefix += noteEndMkr; // don't need space too
				}
				// BEW 23Feb11 moved filtered into from here to be before coll back trans
				markersPrefix.Trim(FALSE); // finally, remove any LHS whitespace

				if (!markersStr.IsEmpty())
				{
					// this data has any markers and endmarkers already 'in place', and
					// we'll show this m_markers content to the user because it may
					// contain a marker for which there is a later 'medial' matching
					// endmarker which has to be placed by the Place... dialog, so it
					// helps to be able to see what the matching marker is and where it
					// is; this stuff will be the first, if it exists, in Sstr, Tstr
					// (except for any markersPrefix material which will precede it, but
					// we attach that at the very end of the function, if its non-empty)
					// 
					// BEW changed 22Feb11 -- we won't commence Sstr and Tstr with
					// markersStr here, because markersStr might have crossRefs appended;
					// and there is no real need to show markersStr anyway because it will
					// never contain a beginmarker which requires a matching later
					// endmarker, so we'll instead just store it in markersPrefix
					//Sstr = markersStr;
					//Tstr = markersStr;
					markersPrefix += markersStr;
				}	

				// USFM examples from UBS illustrate non-binding begin markers follow
				// begin markers that get stored in m_markers, so do this block first
				if (!pSrcPhrase->GetInlineNonbindingMarkers().IsEmpty())
				{
					// we store these markers with a delimiting space, so its already in place
					Sstr += pSrcPhrase->GetInlineNonbindingMarkers();
					Tstr += pSrcPhrase->GetInlineNonbindingMarkers();
				}

				// any endmarkers on the first CSourcePhrase are therefore "medial", and
				// any endmarkers on the last one are not medial and so can be added
				// without recourse to the Place... dialog; the last CSourcePhrase of the
				// retranslation will have the m_bEndRetranslation flag set; beware when
				// the retraslation is one word only and so it has m_bBeginRetranslation
				// and m_bEndRetranslation both set; note: the position of the endmarkers
				// is determinate for the Sstr accumulation, so we place them here for
				// that - but only for CSourcePhrase instances other than the end one, as
				// we handle case of endmarkers at the very end lower down in the code
				// after the loop ends
				// BEW 11Oct10 addition; inline markers have to be taken into account,
				// both non-binding and binding; we'll place an initial binding one via
				// the placement dialog as it will have an endmarker medial somewhere
				// almost certainly, so it's best to see both in the dialog
				bool bNonFinalEndmarkers = FALSE;
				if (!pSrcPhrase->GetInlineBindingMarkers().IsEmpty())
				{
					wxString iBMkrs = pSrcPhrase->GetInlineBindingMarkers();
					// there should always be a final space in m_inlineBindingMarkers, 
					// and we'll ensure it
					iBMkrs.Trim(FALSE);
					iBMkrs.Trim();
					iBMkrs += aSpace;
					markersToPlaceArray.Add(iBMkrs);
					bHasInternalMarkers = TRUE;
				}
				if (!pSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
				{
					bEMkrs = pSrcPhrase->GetInlineBindingEndMarkers();
					// there should never be a final space in m_inlineBindingMarkers, and
					// we'll ensure it (& such markers don't exist in the PNG 1998 SFM
					// standard)
					bEMkrs.Trim(FALSE);
					bEMkrs.Trim();
					markersToPlaceArray.Add(bEMkrs);
					bHasInternalMarkers = TRUE;
					bNonFinalEndmarkers = TRUE;
				}
				else
				{
					bEMkrs.Empty();
				}
				if (!endMarkersStr.IsEmpty())
				{
					// we've endmarkers we have to deal with 
					if (pSrcPhrase->m_bRetranslation && !pSrcPhrase->m_bEndRetranslation)
					{
						// these endmarkers become medial, so append to the list for
						// showing in the dialog; but place them at their known location in
						// Sstr further below
						markersToPlaceArray.Add(endMarkersStr);
						bHasInternalMarkers = TRUE;
						bNonFinalEndmarkers = TRUE;
					}
					else
					{
						// we've a one-instance retranslation with endmarkers at its end...
						// so we can place these automatically (don't need to use the
						// Place... dialog) (we are dealing with the first instance of a
						// retranslation remember)
						bFinalEndmarkers = TRUE; // use this below to do the final append
						finalSuffixStr = endMarkersStr;
					}
				}
				else
				{
					endMarkersStr.Empty();
				}
				if (!pSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
				{
					nBEMkrs = pSrcPhrase->GetInlineNonbindingEndMarkers();
					nBEMkrs.Trim(FALSE);
					nBEMkrs.Trim();
					if (pSrcPhrase->m_bRetranslation && !pSrcPhrase->m_bEndRetranslation)
					{
						// these are medial
						markersToPlaceArray.Add(nBEMkrs);
						bHasInternalMarkers = TRUE;
						bNonFinalEndmarkers = TRUE;
					}
					else
					{
						// we've a one-instance retranslation with inline nonbinding
						// endmarkers at its end... so we can place these automatically
						// (don't need to use the Place... dialog) (we are dealing with the
						// first instance of a retranslation remember)
						bFinalEndmarkers = TRUE; // use this below to do the final append
												 // of any nBEMkrs content
					}
				}
				else
				{
					nBEMkrs.Empty();
				}

				Tstr += pSrcPhrase->m_targetStr;

				// BEW 11Oct10, the legacy way to handle Sstr (needed for showing in the
				// placement dialog) was to just use m_strPhrase; but now that we have
				// inline markers (binding and non-binding possibilities), we have to
				// rebuild the inner part of Sstr here - that is, preceding puncts followed
				// by any inline binding markers followed by m_key followed by any inline
				// binding endmarkers followed by punctuation in m_follPunct. That much is
				// needed first. Then after that, any m_follOuterPunct will need to be
				// dealt with for Sstr, but not Tstr - the latter will have it (or what the
				// user typed) already in place and marker placement within the punctuation
				// string may be required in the Place... dialog; build inner part in s
				//Sstr += pSrcPhrase->m_srcPhrase; <<-- legacy code
				wxString s = pSrcPhrase->m_key;
				if (!pSrcPhrase->GetInlineBindingMarkers().IsEmpty())
				{
					wxString iBMkrs = pSrcPhrase->GetInlineBindingMarkers();
					// there should always be a final space in m_inlineBindingMarkers, 
					// and we'll ensure it
					iBMkrs.Trim(FALSE);
					iBMkrs.Trim();
					iBMkrs += aSpace;
					s = iBMkrs + s;
				}
				if (!bEMkrs.IsEmpty())
				{
					s += bEMkrs; // it was set further above
				}
				if (!pSrcPhrase->m_precPunct.IsEmpty())
				{
					s = pSrcPhrase->m_precPunct + s;
				}
				if (!pSrcPhrase->m_follPunct.IsEmpty())
				{
					s += pSrcPhrase->m_follPunct;
				}
				// Sstr may already have had markers and inline nonbinding mkr in it
				if (!s.IsEmpty())
				{
					Sstr.Trim();
					Sstr << aSpace << s;
				}
				// add the data which is determinate for position for the source text, Sstr
				if (bNonFinalEndmarkers)
				{
					Sstr += endMarkersStr;
					// m_follOuterPunct should only have content if there was preceding
					// content in m_endMarkers; so add any outer puncts next
					if (!pSrcPhrase->GetFollowingOuterPunct().IsEmpty())
					{
						Sstr += pSrcPhrase->GetFollowingOuterPunct();
					}
					// finally, and inline non-binding endmarkers, like \qt*, \wj* etc
					if (!nBEMkrs.IsEmpty())
					{
						Sstr += nBEMkrs;
					}
				}
			} // end TRUE block for test: if (bFirst)
			else
			{
				// We cannot automatically place any subsequent markers after those on the
				// first CSourcePhrase instance - they will be "medial" and so will have to
				// be placed manually using the dialog; this block handles non-first
				// CSourcePhrase instances, as we collect the Tstr, etc within the loop...
				unfilteredStr.Empty(); // empty our scratch string
				
				// BEW added revised comment 31MAR10: if there is medial filtered info (and
				// there might be because it is legal to select over filtered info and
				// create a retranslation - the medial filtered stuff that results gives no
				// problems except when doing an export. It will be considered to be at the
				// same CSourcePhrase for each of src text, adaptation text, and gloss
				// text, and so won't be presented as placeable in the dialog, but will be
				// shown in the text in the dialog which would allow the user to manually
				// shift its location if he wants. Info in m_markers, however, is
				// relocatable and so its markers (and any content following) will be
				// listed in the Place... dialog's list, and is placeable - but it doesn't
				// have to have its location resolved until export time - which is why we
				// only need to worry about it in this present function
				// 
				// BEW 11Oct10, additions to the code below to support m_follOuterPunct
				// and the inline markers from the 4 extra wxString members added to
				// CSourcePhrase in order to fully support USFM markup standards
				if (!filteredInfoStr.IsEmpty())
				{
					// this data has any markers and endmarkers already 'in place'
					unfilteredStr.Trim();
					unfilteredStr += aSpace + filteredInfoStr;
				}
				if (!collBackTransStr.IsEmpty())
				{
					// add the marker too
					unfilteredStr.Trim();
					unfilteredStr += backTransMkr;
					unfilteredStr += aSpace + collBackTransStr;
				}
				if (!freeTransStr.IsEmpty())
				{
					unfilteredStr.Trim();
					unfilteredStr += aSpace + freeMkr;

					// see comments in TRUE block above for what is happening here
					int nWordCount = CountWordsInFreeTranslationSection(TRUE,pSrcPhrases,
							pSrcPhrase->m_nSequNumber); // TRUE means 'count in tgt text'
					wxString entry = _T("|@");
					entry << nWordCount; // converts int to string automatically
					entry << _T("@| ");
					// append it after a delimiting space
					unfilteredStr += aSpace + entry;
					
					// now the free translation string itself & endmarker
					unfilteredStr += aSpace + freeTransStr;
					unfilteredStr += freeEndMkr; // don't need space too
				}
				if (!noteStr.IsEmpty())
				{
					unfilteredStr.Trim();
					unfilteredStr += aSpace + noteMkr;
/* 
					// BEW 15Jun11, removed OXES support until it is needed somewhere
					// BEW 17Sep10, provide oxes with the m_targetStr as the referenced
					// word for the note stored here
					// BEW 19May12, reinstated OXES support, but not support for \note,
					// so leave this commented out (unlesss we later change our minds)
					if (gpApp->m_bOxesExportInProgress)
					{
						// 'numberOfChars' is not the number of characters in the note itself, but
						// rather the number of characters in the word(s) of the adaptation phrase in the
						// m_targetStr member of this CSourcePhrase (oxes needs this info)
						int numberOfChars = pSrcPhrase->m_targetStr.Len(); // no space at end
						wxString numStr;
						numStr = numStr.Format(_T("%d"),numberOfChars);
						numStr = _T("@#") + numStr;
						numStr += _T(':'); // divider
						numStr += pSrcPhrase->m_targetStr;
						numStr += _T("#@");
						noteStr = numStr + noteStr;
						// the oxes parser must detect this @#nnn#@ substring and remove it, convert
						// it to int, and use it to count the phrase's length to which the note applies
						// so that the endOffset in the relevant NoteDetails struct can be set
						// correctly, and put the word after the colon into its wordsInSpan member
					}
*/
					unfilteredStr += aSpace + noteStr;
					unfilteredStr += noteEndMkr; // don't need space too
				}
				// BEW 23Feb11 moved filtered info to be first above
 				unfilteredStr.Trim(FALSE); // finally, remove any LHS whitespace
		   
				// insert any non-empty unfiltered material "in place" in the strings
				// which the user will see (source & target)
				if (!unfilteredStr.IsEmpty())
				{
					Tstr.Trim();
					Tstr << aSpace << unfilteredStr;
					Sstr.Trim();
					Sstr << aSpace << unfilteredStr;
				}

				// m_markers material, however, belongs in the list for later placement in
				// Tstr, but for Sstr we must place it automatically because its position
				// is determinate and not subject to relocation in the placement dialog
				if (!markersStr.IsEmpty())
				{
					markersToPlaceArray.Add(markersStr);
					bHasInternalMarkers = TRUE;
					Sstr += markersStr; 
				}

				// USFM examples from UBS illustrate non-binding begin markers follow
				// begin markers that get stored in m_markers, place it in Sstr, but add
				// to the list for Tstr
				if (!pSrcPhrase->GetInlineNonbindingMarkers().IsEmpty())
				{
					// we store these markers with a delimiting space, so its already in place
					Sstr += pSrcPhrase->GetInlineNonbindingMarkers();
					bHasInternalMarkers = TRUE;
					markersToPlaceArray.Add(pSrcPhrase->GetInlineNonbindingMarkers());
				}

				// at this point we can't add any more to Sstr until we've rebuilt the
				// word and its punctation and any inline binding markers & endmarkers below
				
				// inline binding beginmarker(s) are medial, so add them to Tstr
				if (!pSrcPhrase->GetInlineBindingMarkers().IsEmpty())
				{
					wxString iBMkrs = pSrcPhrase->GetInlineBindingMarkers();
					// there should always be a final space in m_inlineBindingMarkers, 
					// and we'll ensure it
					iBMkrs.Trim(FALSE);
					iBMkrs.Trim();
					iBMkrs += aSpace;
					markersToPlaceArray.Add(iBMkrs);
					bHasInternalMarkers = TRUE;
				}

				bool bNonFinalEndmarkers = FALSE;
				// if we are at m_bEndRetranslation == TRUE, we could automatically place
				// an inline binding endmarker, but if there were following punctuation on
				// the word, it wouldn't be possible without further analysis we'd want to
				// avoid, so we'll just add any such to the list and let the user Place them
				if (!pSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
				{
					bEMkrs = pSrcPhrase->GetInlineBindingEndMarkers();
					// there should never be a final space in m_inlineBindingMarkers, and
					// we'll ensure it (& such markers don't exist in the PNG 1998 SFM
					// standard)
					bEMkrs.Trim(FALSE);
					bEMkrs.Trim();
					markersToPlaceArray.Add(bEMkrs);
					bHasInternalMarkers = TRUE;
					bNonFinalEndmarkers = TRUE;
				}
				else
				{
					bEMkrs.Empty();
				}				
				// any m_endMarkers material will either be medial and so need to be put 
				// in the list, or final (if this is the last CSourcePhrase of the
				// retranslation, that is, if m_bEndRetranslation is TRUE)
				if (!endMarkersStr.IsEmpty())
				{
					// we've endmarkers we have to deal with 
					if (pSrcPhrase->m_bRetranslation && !pSrcPhrase->m_bEndRetranslation)
					{
						// these endmarkers become medial, so append to the list for
						// showing in the dialog; but place them at their known location in
						// Sstr further below
						markersToPlaceArray.Add(endMarkersStr);
						bHasInternalMarkers = TRUE;
						bNonFinalEndmarkers = TRUE;
					}
					else
					{
						// we've a retranslation with endmarkers at its end...
						// so we can place these automatically (don't need to use the
						// Place... dialog) (we are dealing with the first instance of a
						// retranslation remember)
						bFinalEndmarkers = TRUE; // use this below to do the final append
						finalSuffixStr = endMarkersStr; // for use by Sstr

						// for Tstr though, we'll still have to place the endmarker
						// material because there could be non-empty outer following
						// punctuation and so we can't assume endMarkersStr will be last
						markersToPlaceArray.Add(endMarkersStr);
						bHasInternalMarkers = TRUE;
						bNonFinalEndmarkers = TRUE;
					}
				}
				else
				{
					endMarkersStr.Empty();
				}
				if (!pSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
				{
					nBEMkrs = pSrcPhrase->GetInlineNonbindingEndMarkers();
					nBEMkrs.Trim(FALSE);
					nBEMkrs.Trim();
					if (pSrcPhrase->m_bRetranslation && !pSrcPhrase->m_bEndRetranslation)
					{
						// these are medial
						markersToPlaceArray.Add(nBEMkrs);
						bHasInternalMarkers = TRUE;
						bNonFinalEndmarkers = TRUE;
					}
					else
					{
						// we've a one-instance retranslation with inline nonbinding
						// endmarkers at its end... so we can place these automatically
						// (don't need to use the Place... dialog) (we are dealing with the
						// first instance of a retranslation remember)
						bFinalEndmarkers = TRUE; // use this below to do the final append
												 // of any nBEMkrs content, for Sstr

						// for Tstr though, we'll still have to place the inline nonbinding
						// endmarker, probably we could assume it will be last, but by
						// having the user place it we ensure it ends up where it should
						markersToPlaceArray.Add(endMarkersStr);
						bHasInternalMarkers = TRUE;
						bNonFinalEndmarkers = TRUE;
					}
				}
				else
				{
					nBEMkrs.Empty();
				}

				// add the sourcephrase's target text word or phrase, if it is not an empty
				// string; likewise for the source text word or phrase, except that we
				// must have built it from m_key in order to get markers placed correctly,
				// see above
				if (!pSrcPhrase->m_targetStr.IsEmpty())
				{
					Tstr.Trim();
					Tstr << aSpace << pSrcPhrase->m_targetStr;
				}
				// now build up the inner part of the source text word(s)
				wxString s = pSrcPhrase->m_key;
				if (!pSrcPhrase->GetInlineBindingMarkers().IsEmpty())
				{
					wxString iBMkrs = pSrcPhrase->GetInlineBindingMarkers();
					// there should always be a final space in m_inlineBindingMarkers, 
					// and we'll ensure it
					iBMkrs.Trim(FALSE);
					iBMkrs.Trim();
					iBMkrs += aSpace;
					s = iBMkrs + s;
				}
				if (!pSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
				{
					wxString iBEMkrs = pSrcPhrase->GetInlineBindingEndMarkers();
					// there should never be a final space in m_inlineBindingMarkers, and
					// we'll ensure it (& such markers don't exist in the PNG 1998 SFM
					// standard)
					iBEMkrs.Trim(FALSE);
					iBEMkrs.Trim();
					s += iBEMkrs;
				}
				if (!pSrcPhrase->m_precPunct.IsEmpty())
				{
					s = pSrcPhrase->m_precPunct + s;
				}
				if (!pSrcPhrase->m_follPunct.IsEmpty())
				{
					s += pSrcPhrase->m_follPunct;
				}
				if (!s.IsEmpty())
				{
					Sstr.Trim();
					Sstr << aSpace << s;
				}

				// add any needed endmarkers and other final stuff for Sstr, that
				// corresponds to the stuff added to the string array for placement into
				// Tstr
				if (bNonFinalEndmarkers)
				{
					if (!endMarkersStr.IsEmpty())
					{
						Sstr << endMarkersStr;
					}
					// m_follOuterPunct should only have content if there was preceding
					// content in m_endMarkers; so add any outer puncts next
					if (!pSrcPhrase->GetFollowingOuterPunct().IsEmpty())
					{
						Sstr += pSrcPhrase->GetFollowingOuterPunct();
					}
					// finally, and inline non-binding endmarkers, like \qt*, \wj* etc
					if (!nBEMkrs.IsEmpty())
					{
						Sstr += nBEMkrs;
					}
				}
			}
		} // end of else block for testing that we aren't at the end or start of another

		// finally, add any final endmarkers, when m_bEndRetranslation is TRUE, but only
		// for Sstr, because for Tstr any endmarkers may need the Placement dialog, since
		// there could be final punctuation and some of it may be outer punctuation
		if (bFinalEndmarkers && pSrcPhrase->m_bEndRetranslation)
		{
			if (!finalSuffixStr.IsEmpty())
			{
				Sstr += finalSuffixStr;
			}
			// m_follOuterPunct should only have content if there was preceding
			// content in m_endMarkers; so add any outer puncts next
			if (!pSrcPhrase->GetFollowingOuterPunct().IsEmpty())
			{
				Sstr += pSrcPhrase->GetFollowingOuterPunct();
			}
			// finally, and inline non-binding endmarkers, like \qt*, \wj* etc
			if (!nBEMkrs.IsEmpty())
			{
				Sstr += nBEMkrs;
			}
		}

		// if we got to the end of the file, pos will now be null, so we have to check and
		// if it is, set savePos to null because it is savePos that we return to the caller
		if (pos == 0)
			savePos = NULL;
	} // end of while loop

	// if there are internal markers, put up the dialog to place them
	if (bHasInternalMarkers)
	{
		// Note: because the setters are called before ShowModal() is called,
		// initialization of the internal controls' pointers etc has to be done in the
		// creator, rather than as is normally done (ie. in InitDialog()) because the
		// latter is called from ShowModal(, which is too late
		CPlaceRetranslationInternalMarkers dlg(gpApp->GetMainFrame());

		// set up the text controls and list box with their data; these setters enable the
		// data passing to be done without the use of globals
		dlg.SetNonEditableString(Sstr);
		dlg.SetUserEditableString(Tstr);
		dlg.SetPlaceableDataStrings(&markersToPlaceArray);

		// show the dialog
		dlg.ShowModal();

		// get the post-placement resulting string
		Tstr = dlg.GetPostPlacementString();

		// remove initial and final whitespace
		Tstr.Trim(FALSE);
		Tstr.Trim();
	} // end of TRUE block for test: if (bHasInternalMarkers)

	// now add the prefix string material not shown in the Place... dialog, 
	// if it is not empty
	if (!markersPrefix.IsEmpty())
	{
		markersPrefix.Trim(FALSE);
		markersPrefix.Trim();
		markersPrefix += aSpace; // ensure a trailing space
		Tstr = markersPrefix + Tstr;
	}
	markersToPlaceArray.Clear();

	// the caller will add a delimiting space where needed, so we can refrain from trying
	// to assume what the caller wants and return the string with ends Trim()-ed
	return savePos; // return position of first srcPhrase after retranslation, or null
}

// wx version addition: This is a version of the MFC function of the same name. The MFC function
// (see above) did the punctuation tuck leftward from within the text as a CString; this version does
// the tuck in the pNew buffer and so should be somewhat quicker.
// Note: The pOld and pNew pointers are returned by reference to provide their new updated positions in the caller.
bool DetachedNonQuotePunctuationFollows(wxChar* pOld, wxChar* pEnd, wxChar* pPosAfterMkr, wxString& spacelessPuncts)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	bool bTuckLeft = FALSE;
	if (pOld >= pEnd)
		return FALSE;

	wxChar* ptr = pPosAfterMkr;

	// get over the white space(s)
	int itemLen;
	itemLen = pDoc->ParseWhiteSpace(ptr);
	if (itemLen == 0)
	{
		// no tuck is needed because there is no white space following the endmarker
		return FALSE;
	}
	ptr += itemLen;
	if (ptr >= pEnd)
	{
		return FALSE;
	}

	wxString remainder = ptr; // we are pointing at something, so make a CString of what remains

	// first, we want to collect into a string as many punctuation characters as exist
	// at the ptr location, (there may be none, one, several, and if there are one or more
	// they may be at the start of the next word, or it/they may be detached from the next word
	wxString spannedStr;
	spannedStr.Empty();
	spannedStr = SpanIncluding(remainder,spacelessPuncts);
	if (spannedStr.IsEmpty())
	{
		return FALSE; // there was no punctuation at the ptr location, so no tuck left is needed
	}

	// get the length of the spanned string and find out if spanning stopped at white space
	int spannedLen = spannedStr.Length();
	int remainderLen = remainder.Length();
	// wx version note: Since we require a read-only buffer we use GetData which just returns
	// a const wxChar* to the data in the string.
	const wxChar* pBuff2 = remainder.GetData();
	wxChar* pBufStart2 = (wxChar*)pBuff2;
	wxChar* pEnd2;
	pEnd2 = pBufStart2 + remainderLen; // whm added
	wxASSERT(*pEnd2 == _T('\0'));
	pEnd2 = pEnd2; // avoid warning
	bool bIsWhite = pDoc->IsWhiteSpace(pBufStart2 + spannedLen) || (pBufStart2 + spannedLen >= pBufStart2 + remainderLen);

	if (!bIsWhite)
	{
		// the end of the punctuation was not buffer end or white space, so it must be initial
		// punctuation at the start of a following word - this is never a candidate for tucking
		// to the left, so return FALSE
		return FALSE;
	}

	// if we get here, then we know the punctuation spanned string is detached, and so is a
	// possible candidate for tucking to the left. However, detachment is not a sufficient
	// condition. The punctuation must not contain an opening quote or opening double quote,
	// because such quotes associate to the word or phrase to the right and we'd not tuck
	// left for those. On the other hand, if it is a vertical single or double quote, such
	// would be ambiguous between whether it is opening or closing - we'll assume any detached
	// straight quote is a closing one and so tuck it to the left; and any other punctuation
	// which is detached we'll assume it should tuck left also.
	// wx version note: Since we require a read-only buffer we use GetData which just returns
	// a const wxChar* to the data in the string.
	const wxChar* pSpanned = spannedStr.GetData();
	wxChar* pSpannedBufStart = (wxChar*)pSpanned;
	wxChar* pSpannedEnd = pSpannedBufStart + spannedLen;
	wxASSERT(*pSpannedEnd == _T('\0'));
	wxChar* ptr3 = pSpannedBufStart;
	bool bHasOpeningQuote = FALSE;
	while (ptr3 < pSpannedEnd)
	{
		bHasOpeningQuote = (pDoc->IsOpeningQuote(ptr3) && (*ptr3 != _T('\"')) && (*ptr3 != _T('\'')));
		if (bHasOpeningQuote)
			break;
		else
			ptr3++; // advance
	}
	if (bHasOpeningQuote)
		bTuckLeft = FALSE;
	else
		bTuckLeft = TRUE;
	return bTuckLeft;
}

// BEW added 06Oct05 for support of free translation propagation across an export of the
// target text and subsequent import into a new project; the function uses the
// wxStringTokenizer class and its GetNextToken() function to count the words in str and
// return the count; if pStrList is NULL, just the word count is returned, but if a
// wxArrayString pointer is passed in, then it is populated with the individual words 
// BEW 31Mar10, no changes needed for support of doc version 5
int GetWordCount(wxString& str, wxArrayString* pStrList)
{
	if (pStrList)
	{
		pStrList->Clear();
	}

	wxString aWord;
	int nCount = 0;

	wxStringTokenizer tkz(str,_T(" \n\r\t")); // use default " " whitespace here

	// whm note: The following test not needed but doesn't hurt
	if (tkz.CountTokens() == 0)
		return 0;

	// continue to break out the rest of the words and count them,
	// storing them if the caller wants them
	while (tkz.HasMoreTokens())
	{
		aWord = tkz.GetNextToken();
		nCount++;
		if (pStrList)
		{
			pStrList->Add(aWord);
		}
	}
	return nCount;
}

// The following ParseWordRTF() function is the same as the legacy ParseWord() function in the Doc before
// Bruce rewrote it for doc v 5 purposes. I've renamed it to ParseWordRTF and reclaimed it here for RTF output
// purposes.
int ParseWordRTF(wxChar *pChar, wxString& precedePunct, wxString& followPunct,
													wxString& nospacePuncts)
//													
// returns number of characters parsed over.
//
// From version 1.4.1 and onwards, we must choose which code we use according to the
// gbSfmOnlyAfterNewlines flag; when TRUE, any standard format marker escape characters
// which do not follow a newline are not assumed to belong to a sfm, and so we treat them
// in such cases as ordinary word-building characters (on the assumption we are dealing
// with a hacked legacy encoding in which the escape character is an alphabetic glyph in
// the font)
// BEW 17 March 2005 -- additions to the signature, and additional functions used...
// Accumulate preceding punctuation into precedPunct, following punctuation into
// followPunt, use the nospacePuncts string which contains the source set with all spaces
// removed to help do the parsing of any punctuation immediately attached to the word
// (either before or after) and IsOpeningQuote() and IsClosingQuote() to parse over any
// preceding or following detached quotation marks (of various kinds, including SFM < or >
// wedges)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	int len = 0;
	wxChar* ptr = pChar;
	// first, parse over any preceding punctuation, bearing in mind it may have sequences
	// of single and/or double opening quotation marks with one or more spaces between
	// each. We want to accumulate all such punctuation, and the spaces in-place, into the
	// precedePunct CString. We assume only left quotations and left wedges can be set off
	// by spaces from the actual word and whatever preceding punctuation is on it. We make
	// the same assumption for punctuation following the word - but in that case there
	// should be right wedges or right quotation marks. We'll allow ordinary (vertical)
	// double quotation, and single quotation if the latter is being considered to be
	// punctuation, even though this weakens the integrity of out algorithm - but it would
	// only be compromised if there were sequences of vertical quotes with spaces both at
	// the end of a word and at the start of the next word in the source text data, and
	// this would be highly unlikely to ever occur.
	bool bHasPrecPunct = FALSE;
	bool bHasOpeningQuote = FALSE;

	while (pDoc->IsOpeningQuote(ptr) || IsWhiteSpace(ptr))
	{
		// this block gets us over all detached preceding quotes and the spaces which
		// detach them; we exit this block either when the word proper has been reached, or
		// with ptr pointing at some non-quote punctuation attached to the start of the
		// word. In the latter case, the next block will parse across any such punctuation
		// until the word proper has been reached.
		if (IsWhiteSpace(ptr))
		{
			precedePunct += _T(' '); // normalize while we are at it
			ptr++;
		}
		else
		{
			bHasOpeningQuote = TRUE; // FALSE is used later to stop regular opening 
				// quote (when initial in a following word) from being interpretted as
				// belonging to the current sourcephrase in the circumstance where there is
				// detached non-quote punctuation being spanned in this current block. That
				// is, we want "... word1 ! "word2" word3 ..." to be handled that way,
				// instead of being parsed as "... word1 ! " word2" word3 ..." for example
			precedePunct += *ptr++;
		}
		len++;
	}
	int nFound = -1;
	while (!pDoc->IsEnd(ptr) && (nFound = nospacePuncts.Find(*ptr)) >= 0)
	{
		// the test checks to see if the character at the location of ptr belongs to the
		// set of source language punctuation characters (with space excluded from the
		// latter) - as long as the nFound value is positive we are parsing over
		// punctuation characters
		precedePunct += *ptr++;
		len++;
	}
	if (precedePunct.Length() > 0)
		bHasPrecPunct = TRUE;
	wxChar* pWordProper = ptr; // where the first character of the word starts
	// we've come to the word proper. We have to parse over it too, but be careful of the
	// fact that punctuation might be within it (eg. boy's) - so we parse to a space or
	// other determinate indicator of the end of the word, and then accumulate final
	// punctuation both preceding that space and following it - provided the latter is
	// right quotation marks or a right wedge (and we'll assume that ordinary vertical
	// double quote or apostrophe goes with the word which precedes, so long as there was
	// preceding punctuation found - otherwise we'll assume it belongs with the next word
	// to be parsed) We also don't card if there is a gFSescapechar in the next section -
	// we can assume it is being used as a word building character quite safely, because we
	// don't have to consider the possibility of such a character being the start of a
	// following (U)SFM until after the next white space character has been parsed over.

	// BEW changed 10Apr06, to remove the "&& *ptr != gSFescapechar" from the while's test,
	// and to put it instead in the code block with TRUE and FALSE code blocks, so as to
	// properly handle parsing across a backslash when the gbSfmOnlyAfterNewlines flag is
	// TRUE
	wxChar* pPunctStart = 0;
	wxChar* pPunctEnd = 0;
	bool bStarted = FALSE;
	while (!pDoc->IsEnd(ptr) && !IsWhiteSpace(ptr))
	{
		// BEW added 25May06; detecting a SF marker immediately following final punctuation
		// would cause return to the caller from within the loop, without the followPunct
		// CString having any chance to get final punctuation characters put in it. So now
		// we have to detect when final punctuation commences, set pPunctStart there, and
		// set pPunctEnd to where it ends, so that if we have to return to the caller
		// early, we can check for these pointers being different and copy what lies
		// between them into followPunct, so that the caller can properly remove the
		// following punctuation and set up m_key correctly. (Detached punctuation will not
		// break this algorithm because it will already have been put into precedePunct)
		if ((nFound = nospacePuncts.Find(*ptr)) >= 0)
		{
			// we found a (following) punctuation character
			if (bStarted)
			{
				// we've already found at least one, so set pPunctEnd to the current 
				// location
				pPunctEnd = ptr + 1;
			}
			else
			{
				// we've not found one yet, so set both pointers to this location & 
				// turn on the flag
				bStarted = TRUE;
				pPunctStart = ptr;
				pPunctEnd = ptr + 1;
			}
		}
		else
		{
			// we did not find (following) punctuation at this location - what we do here
			// depends on whether we've already found at least one such, or not; it could
			// be a SF escape char here, so we must leave bTurnedON TRUE,
			if (bStarted)
			{
				// we have found one earlier, so we must set the ending pointer here (tests
				// below will determine whether this section is word-internal and to be
				// ignored, or actually extends to the location at which word parsing ends
				// - in which case we don't want to ignore it)
				pPunctEnd = ptr;
			}
			else
			{
				// we've not started spanning (following) punctuation yet, so update both
				// pointers to this location (BEW 23Feb07 added +1; this block is not very
				// important as these values get overridden, but adding +1 makes the value
				// correct because ptr here is pointing at a non-punctuation character and
				// if there is a punctuation character it cannot be at ptr, it may or may
				// not be at ptr + 1, and the iteration of the parse will determine that or
				// not)
				pPunctStart = ptr + 1;
				pPunctEnd = ptr + 1;
			}
		}

		// advance over the next character, or if the user wants USFM fixed space ~
		// sequence retained as a conjoiner, then check for it and advance by one
		// if such a sequence is at ptr; but if the gbSfmOnlyAfterNewlines flag is TRUE and
		// we are pointing at a backslash, then parse over it too (ie. don't interpret it
		// as the beginning of a valid SFM)
		if (*ptr != gSFescapechar)
		{
			// we are not pointing at a backslash...
			if (!gpApp->m_bChangeFixedSpaceToRegularSpace && *ptr == _T('~'))
			{
				ptr += 1;
				len += 1;
			}
			else
			{
				ptr++;
				len++;
			}

			// if we are started and not pointing at white space either, then turn off and
			// reset the pointers for a following punctuation span
			if (bStarted && !IsWhiteSpace(ptr) && (*ptr != gSFescapechar))
			{
				// the punctuation span was word-internal, so we forget about it
				bStarted = FALSE;
				pPunctStart = ptr;
				pPunctEnd = ptr;
			}
		}
		else
		{
			// we are pointing at a backslash
			if (bStarted && (pPunctEnd - pPunctStart) > 0 && pPunctEnd == ptr)
			{
				// there is word-final punctuation content to be dealt with
				int numChars = (int)(pPunctEnd - pPunctStart);
				wxString finals(pPunctStart,numChars);
				followPunct = finals;
			}
			return len;
		}
	}


	// now, work backwards first - we may have stopped at a space and there could have been
	// several punctuation characters parsed over by the previous while loop; we don't have
	// to count these ones, so long as followPunct ends up containing all following
	// punctuation
	wxChar* pBack = ptr;
	do {
		--pBack; // point to the previous character
		if (pBack < pWordProper) break; // ptr did not advance in the previous while block, 
										// so break out
		if (pDoc->IsClosingQuote(pBack) || (nFound = nospacePuncts.Find(*pBack)) >= 0)
		{
			// it is a punctuation character - either one of the closing quote ones or
			// or apostrophe is being treated as punctuation and it is an apostrophe; OR
			// it is one of the spaceless source language set
			wxString s = *pBack;
			followPunct = s + followPunct; // accumulate in text order
		}
	} while ( pBack > pWordProper && (pDoc->IsClosingQuote(pBack) || nFound >= 0));
	// now parse forward from the location where we started parsing backwards - it's from
	// here on we have to be careful to allow for the possibility that the gSFescapechar
	// might or might not be an indicator of a new standard format marker being in the
	// source text stream; and we have to continue counting the characters we successfully
	// parse over. We parse over a small chunk until we determine we must halt. We don't
	// commit to the contents of the small chunk until we are sure we have parsed over at
	// least one genuine detached closing quote.

	// BEW note added on 10Apr06, the comment that it is here that gSFescapechar is
	// relevant is not correct; if ptr is pointing at a backslash, the stuff below does not
	// allow it to be parsed over, but only treated as an SFM onset - so I have to add the
	// checking for ignoring backslashes when gbSfmOnlyAfterNewlines is TRUE be done in the
	// loop above! The code below is therefore a bit more convoluted than it need be, but
	// I'll leave the sleeping dog to lie.

	if (pDoc->IsEnd(ptr))
		return len; // we are at the end of the source data, so can't parse further
	wxString smchunk;
	smchunk.Empty();
	int nChunkLen = 0;
	bool bFoundDetachedRightQuote = FALSE;
	// treat the escape character as indicating the presence of a (U)SFM, so test for
	// it as a loop ending criterion
	wxChar* ptr2;
	wxChar* ptr3;
a:	if (!pDoc->IsEnd(ptr) && *ptr != gSFescapechar)
	{
		if (IsWhiteSpace(ptr))
		{
			smchunk += _T(' '); // we may as well normalize to space 
								// while we are at it
			ptr++; // accumulate it and advance pointer then iterate
			goto a;
		}
		else
		{	
			// it's not white space, so what is it?
			if (pDoc->IsClosingQuote(ptr))
			{
				// it's one of the closing quote characters (but " or ' are ambiguous,
				// so test further because " or ' might be preceding punctuation on a
				// following word not yet parsed)
				if (pDoc->IsAmbiguousQuote(ptr))
				{
					// it's one of the two ambiguous ones; we'll assume this does not
					// belong with our word if there was no preceding punctuation, or
					// if there was preceding punctuation but we have already found at
					// least one closing curly quote, or if bHasOpeningQuote is FALSE,
					// otherwise we'll accept it as a detached following quote mark
					if (bHasPrecPunct)
					{
						if (!bHasOpeningQuote)
						{
							// there was no opening quote on this word, but the word
							// may be the end of a quoted section and so we must test
							// further
							if (bFoundDetachedRightQuote)
							{
								// a detached right quote was found earlier, so we
								// should stop the iteration right here, and not
								// accumulate the non-curly quote symbol because it is
								// unlikely it would associate to the left
								goto g;
							}
							else
							{
								// we only know where was opening punctuation and no
								// detached closing quote has yet been found, so we
								// need to apply the tests in the next block to decide
								// what to do with the ambiguous quote at ptr; OR
								// control got directed here from the bHasOpeningQuote
								// == FALSE block and we've not found a detached right
								// quote earlier, so we must make our final decision
								// based on the tests in the block below
								goto e;
							}
						}
						else // next block is where the 'final' decisions will be 
							 // made (only one option iterates from within the next
							 // battery of tests)
						{
							// there was an opening quote on this word, or control was
							// directed here from the block immediately above; so this
							// ambiguous quote may be a closing one, or it could belong
							// to the next word - so we must test further
e:							ptr2 = ptr;
							ptr2++; // point at the next character
							if (IsWhiteSpace(ptr2))
							{
								// *ptr is bracketed by white space either side, so it
								// could associate either to the left or two the right
								// - so we must make some assumptions: we assume it is
								// detached quote for the current (ie. to the left)
								// word if the next character past ptr2 is not
								// punctuation (if it's a white space we jump it and
								// test again), if it is punctuation we assume the
								// quote at ptr associates to the right, and if the
								// character at ptr2 is not white space we assume we
								// have moved into the preceding punctuation of a
								// following word and so associate the quote at ptr
								// rightwards
								ptr2++; // point beyond the white space

								// skip over any additional white spaces
								while (IsWhiteSpace(ptr2)) {ptr2++;} 

								// find out what the first non-whitespace character is
								if (nospacePuncts.Find(*ptr2) == -1)
								{
									// the character at ptr2 is not punctuation, so we
									// will assume the character at ptr associates to
									// the left; if ptr2 is actually at the end of the
									// data (eg, when rebuilding a sourcephrase for
									// document rebuild) then this is also accomodated
									// by the same decision
f:									bFoundDetachedRightQuote = TRUE;
									smchunk += *ptr++; // accumulate it, advance pointer
									goto a; // and iterate
								}
								else
								{
									// it is punctuation at ptr2, so we could have a
									// series of detached quotes which associate left,
									// or a series which associates right. To
									// distinguish these we will favour rightmost
									// association if there is a next word with initial
									// punctuation; otherwise we'll assume we should
									// associate leftwards
									ptr3 = ptr2;
									ptr3++; // point at next char (it could be space, etc)
									if (pDoc->IsEnd(ptr3)) goto f; // associate leftwards & iterate
									if (IsWhiteSpace(ptr3))
									{
										while (IsWhiteSpace(ptr3)) {ptr3++;} // skip any others
										if (pDoc->IsEnd(ptr3)) goto f;
										if (nospacePuncts.Find(*ptr3) == -1)
										{
											// it's not punctuation
											goto f; // iterate
										}
										else
										{
											// it's punctuation; so we'll limit the
											// nesting of tests to a max of two
											// detached ambiguous quotes, so we will
											// here examine what follows - if it is a
											// space or the end of the data we will
											// assume it is the last of detached
											// punctuation associating to the left;
											// anything else, we'll have the quote at
											// ptr associated rightwards
											ptr3++;
											if (pDoc->IsEnd(ptr3) || IsWhiteSpace(ptr3))
												goto f; // iterate
										}
									}
									// bale out (ie. associate right)
g:									followPunct += smchunk;
									nChunkLen = smchunk.Length();
									len += nChunkLen;
									return len;
								}
							}
							else
							{
								// it was not whitespace, so we assume the quote
								// character at ptr must associate to the right, so
								// bale out; however, if we are at the end of the text
								// (eg. when doing document rebuild) then associating
								// rightwards is impossible and we then associate to
								// the left
								if (pDoc->IsEnd(ptr2))
								{
									// associate it to the left, that is, it is part of
									// the currently being parsed word
									bFoundDetachedRightQuote = TRUE;
									smchunk += *ptr++; // accumulate it, advance pointer
									goto a; // and iterate
								}
								// if not at the end, then assume it belongs to the 
								// next word
								goto g;
							}
						} // end of the "final battery of tests" block
					}
					else // bHasPredPunct was FALSE
					{
						// there was no opening punctuation on our parsed word, but the
						// ambiguous quote at ptr could still be a closing quote, or it
						// could be a quote belonging to the next word - so additional
						// tests are required
						goto e;
					}
				}
				else
				{
					// it's a genuine curly closing quote or a right wedge, either way
					// this is detached punctuation belonging to the previous word, so
					// we must accumulate it & iterate
					bFoundDetachedRightQuote = TRUE;
					smchunk += *ptr++; // accumulate it, and advance to the 
									   // next character
					goto a; // iterate
				}
			}
			else
			{
				// it's not one of the possible closing quotes, so we have to stop 
				// iterating
b:				wxString spaceless = smchunk;
				while (spaceless.Find(_T(' ')) != -1)
				{
					spaceless.Remove(spaceless.Find(_T(' ')),1);
				}
				if (smchunk.Length() > 0 && bFoundDetachedRightQuote)
				{
					// there is something to accumulate
					followPunct += smchunk;
					nChunkLen = smchunk.Length();
					len += nChunkLen;
					return len;
				}
				else
				{
					// there is nothing worth accumulating
					return len;
				}
			}
		}
	}
	else
	{
		// we are at the end or at the start of a (U)SFM, so we cannot iterate further
		goto b;
	}
}
