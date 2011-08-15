/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CollabUtilities.h
/// \author			Bruce Waters, from code taken from SetupEditorCollaboration.h by Bill Martin
/// \date_created	27 July 2011
/// \date_revised	
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is a header file containing some utility functions used by Adapt It's
///                 collaboration feature - collaborating with either Paratext or Bibledit. 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "CollabUtilities.h"
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

#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/colour.h>
#include <wx/dir.h>
#include <wx/textfile.h>

#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "SourcePhrase.h"
#include "MainFrm.h"
#include "BString.h"
#include "WaitDlg.h"
#include "XML.h"
#include "SplitDialog.h"
#include "ExportFunctions.h"
#include "PlaceInternalMarkers.h"
#include "Uuid_AI.h" // for uuid support
// the following includes support friend functions for various classes
#include "TargetUnit.h"
#include "KB.h"
#include "Pile.h"
#include "Strip.h"
#include "Layout.h"
#include "KBEditor.h"
#include "RefString.h"
#include "helpers.h"
#include "tellenc.h"
#include "md5.h"
#include "CollabUtilities.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;
extern wxString szProjectConfiguration;
extern wxArrayString m_exportBareMarkers;
extern wxArrayInt m_exportFilterFlags;

extern wxChar gSFescapechar; // the escape char used for start of a standard format marker
extern const wxChar* filterMkr; // defined in the Doc
extern const wxChar* filterMkrEnd; // defined in the Doc
const int filterMkrLen = 8;
const int filterMkrEndLen = 9;

extern bool gbIsGlossing;
extern bool gbGlossingUsesNavFont;
extern bool gbForceUTF8;
extern int  gnOldSequNum;
extern int  gnBeginInsertionsSequNum;
extern int  gnEndInsertionsSequNum;
extern bool gbTryingMRUOpen;
extern bool gbConsistencyCheckCurrent;
extern bool gbDoingInitialSetup;


/// Length of the byte-order-mark (BOM) which consists of the three bytes 0xEF, 0xBB and 0xBF
/// in UTF-8 encoding.
#define nBOMLen 3

/// Length of the byte-order-mark (BOM) which consists of the two bytes 0xFF and 0xFE in
/// in UTF-16 encoding.
#define nU16BOMLen 2

#ifdef _UNICODE

/// The UTF-8 byte-order-mark (BOM) consists of the three bytes 0xEF, 0xBB and 0xBF
/// in UTF-8 encoding. Some applications like Notepad prefix UTF-8 files with
/// this BOM.
static wxUint8 szBOM[nBOMLen] = {0xEF, 0xBB, 0xBF}; // MFC uses BYTE

/// The UTF-16 byte-order-mark (BOM) which consists of the two bytes 0xFF and 0xFE in
/// in UTF-16 encoding.
static wxUint8 szU16BOM[nU16BOMLen] = {0xFF, 0xFE}; // MFC uses BYTE

#endif


//  CollabUtilities functions

wxString SetWorkFolderPath_For_Collaboration()
{
	wxString workPath;
	// get the absolute path to "Adapt It Unicode Work" or "Adapt It Work" as the case may be
	// NOTE: m_bLockedCustomWorkFolderPath == TRUE is included in the test deliberately,
	// because without it, an (administrator) snooper might be tempted to access someone
	// else's remote shared Adapt It project and set up a PT or BE collaboration on his
	// behalf - we want to snip that possibility in the bud!! The snooper won't have this
	// boolean set TRUE, and so he'll be locked in to only being to collaborate from
	// what's on his own machine
	if ((gpApp->m_customWorkFolderPath != gpApp->m_workFolderPath) && gpApp->m_bUseCustomWorkFolderPath
		&& gpApp->m_bLockedCustomWorkFolderPath)
	{
		workPath = gpApp->m_customWorkFolderPath;
	}
	else
	{
		workPath = gpApp->m_workFolderPath;
	}
	return workPath;
}

// If textKind is collab_source_text, returns path in .temp folder to source text file
// from the external editor (it could be a chapter, or whole book, and the function works
// out which and it's filename using the m_bCollabByChapterOnly flag. Likewise, if the
// enum value passed in is collab_target_text, it makes the path to the target text, and
// if the value passed in is collab_freeTrans_text if makes the path to the free
// translation file there, or an empty path string if m_bCollaborationExpectsFreeTrans is
// FALSE.
wxString MakePathToFileInTempFolder_For_Collab(enum DoFor textKind)
{
	wxString chStrForFilename;
	if (gpApp->m_bCollabByChapterOnly)
		chStrForFilename = gpApp->m_CollabChapterSelected;
	else
		chStrForFilename = _T("");

	wxString bookCode = gpApp->GetBookCodeFromBookName(gpApp->m_CollabBookSelected);

	wxString tempFolder;
	wxString path;
	wxString shortProjName;
	tempFolder = gpApp->m_workFolderPath + gpApp->PathSeparator + _T(".temp");
	path = tempFolder + gpApp->PathSeparator;
	switch (textKind)
	{
	case collab_source_text:
		shortProjName = GetShortNameFromProjectName(gpApp->m_CollabProjectForSourceInputs);
		path += gpApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, shortProjName, chStrForFilename, _T(".tmp"));
		break;
	case collab_target_text:
		shortProjName = GetShortNameFromProjectName(gpApp->m_CollabProjectForTargetExports);
		path += gpApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, shortProjName, chStrForFilename, _T(".tmp"));
		break;
	case collab_freeTrans_text:
		if (gpApp->m_bCollaborationExpectsFreeTrans)
		{
			shortProjName = GetShortNameFromProjectName(gpApp->m_CollabProjectForFreeTransExports);
			path += gpApp->GetFileNameForCollaboration(_T("_Collab"), bookCode, shortProjName, chStrForFilename, _T(".tmp"));
		}
		else
		{
			path.Empty();
		}
		break;
	}
	return path;
}

// Build the command lines for reading the PT/BE projects using rdwrtp7.exe/bibledit-gtk.
// whm modified 27Jul11 to use _T("0") for whole book retrieval on the command-line
// BEW 1Aug11, removed code from GetSourceTextFromEditor.h&.cpp to put it here
// For param 1 pass in 'reading' for a read command line, or writing for a write command
// line, to be generated. For param 2, pass in one of collab_source_text,
// collab_targetText, or collab_freeTrans_text. The function internally works out if
// Paratext or Bible edit is being supported, and whether or not free translation is
// expected, the bookCode required, and the shortName to be used for the project which
// pertains to the textKind passed in
wxString BuildCommandLineFor(enum CommandLineFor lineFor, enum DoFor textKind)
{
	wxString cmdLine; cmdLine.Empty();

	wxString chStrForCommandLine;
	if (gpApp->m_bCollabByChapterOnly)
		chStrForCommandLine = gpApp->m_CollabChapterSelected;
	else
		chStrForCommandLine = _T("0");

	wxString readwriteChoiceStr;
	if (lineFor == reading)
		readwriteChoiceStr = _T("-r"); // reading
	else
		readwriteChoiceStr = _T("-w"); // writing

	wxString cmdLineAppPath;
	if (gpApp->m_bCollaboratingWithParatext)
	{
		cmdLineAppPath = GetPathToRdwrtp7();
	}
	else
	{
		cmdLineAppPath = GetBibleditInstallPath();
	}

	wxString shortProjName;
	wxString pathToFile;
	switch (textKind)
	{
		case collab_source_text:
			shortProjName = GetShortNameFromProjectName(gpApp->m_CollabProjectForSourceInputs);
			pathToFile = MakePathToFileInTempFolder_For_Collab(textKind);
			break;
		case collab_target_text:
			shortProjName = GetShortNameFromProjectName(gpApp->m_CollabProjectForTargetExports);
			pathToFile = MakePathToFileInTempFolder_For_Collab(textKind);
			break;
		case collab_freeTrans_text:
			shortProjName = GetShortNameFromProjectName(gpApp->m_CollabProjectForFreeTransExports);
			pathToFile = MakePathToFileInTempFolder_For_Collab(textKind); // will  be returned empty if
							// m_bCollaborationExpectsFreeTrans is FALSE
			break;
	}

	wxString bookCode = gpApp->GetBookCodeFromBookName(gpApp->m_CollabBookSelected);

	// build the command line
	if ((textKind == collab_freeTrans_text && gpApp->m_bCollaborationExpectsFreeTrans) ||
		textKind != collab_freeTrans_text)
	{
		cmdLine = _T("\"") + cmdLineAppPath + _T("\"") + _T(" ") + readwriteChoiceStr + _T(" ") + 
					shortProjName + _T(" ") + bookCode + _T(" ") + chStrForCommandLine + _T(" ") + 
					_T("\"") + pathToFile + _T("\"");
	}
	/* the following is Bill's old code from GetSourceTextFromEditor's OnOK() function, later we can delete it
	wxString commandLineSrc, commandLineTgt, commandLineFreeTrans;
	commandLineFreeTrans.Empty();
	if (m_pApp->m_bCollaboratingWithParatext)
	{
		commandLineSrc = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + shortProjNameSrc + _T(" ") + bookCode + _T(" ") + chStrForCommandLine + _T(" ") + _T("\"") + sourceTempFileName + _T("\"");
		commandLineTgt = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + shortProjNameTgt + _T(" ") + bookCode + _T(" ") + chStrForCommandLine + _T(" ") + _T("\"") + targetTempFileName + _T("\"");
	}
	else if (m_pApp->m_bCollaboratingWithBibledit)
	{
		commandLineSrc = _T("\"") + m_bibledit_gtkPathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + shortProjNameSrc + _T(" ") + bookCode + _T(" ") + chStrForCommandLine + _T(" ") + _T("\"") + sourceTempFileName + _T("\"");
		commandLineTgt = _T("\"") + m_bibledit_gtkPathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + shortProjNameTgt + _T(" ") + bookCode + _T(" ") + chStrForCommandLine + _T(" ") + _T("\"") + targetTempFileName + _T("\"");
	}
	if (m_pApp->m_bCollaborationExpectsFreeTrans)
	{
		if (m_pApp->m_bCollaboratingWithParatext)
		{
			commandLineFreeTrans = _T("\"") + m_rdwrtp7PathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + shortProjNameFreeTrans + _T(" ") + bookCode + _T(" ") + chStrForCommandLine + _T(" ") + _T("\"") + freeTransTempFileName + _T("\"");
		}
		else if (m_pApp->m_bCollaboratingWithBibledit)
		{
			commandLineFreeTrans = _T("\"") + m_bibledit_gtkPathAndFileName + _T("\"") + _T(" ") + _T("-r") + _T(" ") + shortProjNameFreeTrans + _T(" ") + bookCode + _T(" ") + chStrForCommandLine + _T(" ") + _T("\"") + freeTransTempFileName + _T("\"");
		}
	}
	*/
	return cmdLine;
}

wxString GetShortNameFromProjectName(wxString projName)
{
	// the short name is the first field in the projName string
	wxString collabProjShortName;
	collabProjShortName.Empty();
	int posColon;
	posColon = projName.Find(_T(':'));
	// Under Bibledit, the projName may not have any ':' chars. In such cases
	// just return the incoming projName unchanged
	if (!projName.IsEmpty() && posColon == wxNOT_FOUND)
		return projName;
	collabProjShortName = projName.Mid(0,posColon);
	collabProjShortName.Trim(FALSE);
	collabProjShortName.Trim(TRUE);
	return collabProjShortName;
}

// whm modified 27Jul11 to handle whole book filename creation (which does not
// have the _CHnn chapter part in it.
// BEW 1Aug11 moved the code from OnOK() in GetSourceTextFromEditor.cpp to here
void TransferTextBetweenAdaptItAndExternalEditor(enum CommandLineFor lineFor, enum DoFor textKind,
							wxArrayString& textIOArray, wxArrayString& errorsIOArray, long& resultCode)
{
	// Note: _EXCHANGE_DATA_DIRECTLY_WITH_BIBLEDIT is defined near beginning of Adapt_It.h
	// Defined to 0 to use Bibledit's command-line interface to fetch text and write text 
	// from/to its project data files.
	// Defined to 1 to fetch text and write text directly from/to Bibledit's project data 
	// files (not using command-line interface.
	wxString bareChapterSelectedStr;
	if (gpApp->m_bCollabByChapterOnly)
	{
		bareChapterSelectedStr = gpApp->m_CollabChapterSelected;
	}
	else
	{
		bareChapterSelectedStr = _T("0");
	}
	int chNumForBEDirect = wxAtoi(bareChapterSelectedStr); //actual chapter number to get chapter
	if (chNumForBEDirect == 0)
	{
		// 0 is the Paratext parameter value for whole book, but we change this to -1
		// for Bibledit whole book collection in the CopyTextFromBibleditDataToTempFolder()
		// function call below.
		chNumForBEDirect = -1;
	}

	wxString beProjPath = gpApp->GetBibleditProjectsDirPath();
	wxString shortProjName;
	switch (textKind)
	{
	case collab_source_text:
		shortProjName = GetShortNameFromProjectName(gpApp->m_CollabProjectForSourceInputs);
		break;
	case  collab_target_text:
		shortProjName = GetShortNameFromProjectName(gpApp->m_CollabProjectForTargetExports);
		break;
	case collab_freeTrans_text:
		shortProjName = GetShortNameFromProjectName(gpApp->m_CollabProjectForFreeTransExports);
		break;
	}
	beProjPath += gpApp->PathSeparator + shortProjName;
	wxString fullBookName = gpApp->m_CollabBookSelected;
	wxString theFileName = MakePathToFileInTempFolder_For_Collab(textKind);
	
	if (lineFor == reading)
	{
		// we are transferring data from Paratext or Bibledit, to Adapt It
		// whm Note: The feedback from textIOarray and errorsIOArray is handled
		// in the calling function MakeUpdatedTextForExternalEditor().
		if (gpApp->m_bCollaboratingWithParatext || _EXCHANGE_DATA_DIRECTLY_WITH_BIBLEDIT == 0)
		{
			// Use the wxExecute() override that takes the two wxStringArray parameters. This
			// also redirects the output and suppresses the dos console window during execution.
			wxString commandLine = BuildCommandLineFor(lineFor, textKind);
			resultCode = ::wxExecute(commandLine,textIOArray,errorsIOArray);
		}
		else if (gpApp->m_bCollaboratingWithBibledit)
		{
			bool bWriteOK;
			bWriteOK = CopyTextFromBibleditDataToTempFolder(beProjPath, fullBookName, chNumForBEDirect, 
																	theFileName, errorsIOArray);
			if (bWriteOK)
				resultCode = 0; // 0 means same as wxExecute() success
			else // bWriteOK was FALSE
				resultCode = 1; // 1 means same as wxExecute() ERROR, errorsIOArray will contain error message(s)
		}
	} // end of TRUE block for test:  if (lineFor == reading)
	else // lineFor == writing
	{
		// we are transferring data from Adapt It, to either Paratext or Bibledit
		// whm 13Aug11 added this code for transferring data from Adapt It to Paratext or Bibledit
		// whm Note: The feedback from textIOarray and errorsIOArray is handled
		// in the calling function MakeUpdatedTextForExternalEditor().
		if (gpApp->m_bCollaboratingWithParatext || _EXCHANGE_DATA_DIRECTLY_WITH_BIBLEDIT == 0)
		{
			// Use the wxExecute() override that takes the two wxStringArray parameters. This
			// also redirects the output and suppresses the dos console window during execution.
			wxString commandLine = BuildCommandLineFor(lineFor, textKind);
			resultCode = ::wxExecute(commandLine,textIOArray,errorsIOArray);
		}
		else if (gpApp->m_bCollaboratingWithBibledit)
		{
			bool bWriteOK;
			bWriteOK = CopyTextFromTempFolderToBibleditData(beProjPath, fullBookName, chNumForBEDirect,
																	theFileName,errorsIOArray);
			if (bWriteOK)
				resultCode = 0; // 0 means same as wxExecute() success
			else // bWriteOK was FALSE
				resultCode = 1; // 1 means same as wxExecute() ERROR, errorsIOArray will contain error message(s)
		}
	}
}

bool CopyTextFromBibleditDataToTempFolder(wxString projectPath, wxString bookName, int chapterNumber, wxString tempFilePathName, wxArrayString& errors)
{
	// construct the path to the Bibledit chapter data files
	wxString pathToBookFolder;
	wxString dataFolder = _T("data");
	pathToBookFolder = projectPath + gpApp->PathSeparator + dataFolder + gpApp->PathSeparator + bookName;
	wxString dataBuffer = _T("");
	bool bGetWholeBook = FALSE;
	if (chapterNumber == -1)
	{
		// get the whole book
		bGetWholeBook = TRUE;
	}
	wxFile* pTempFile;
	// We need to ensure it doesn't exist because we are concatenating in the pTempFile->Write() call below and
	// we want to start afresh in the file.
	if (::wxFileExists(tempFilePathName))
	{
		bool bRemoved = FALSE;
		bRemoved = ::wxRemoveFile(tempFilePathName); 
		if (!bRemoved)
		{
			// Not likely to happen, so an English message will suffice.
			wxString msg = _T("Unable to remove existing temporary file at:\n%s");
			msg = msg.Format(msg,tempFilePathName.c_str());
			errors.Add(msg);
			return FALSE;
		}
	}
	pTempFile = new wxFile(tempFilePathName,wxFile::write_append); // just append new data to end of the temp file;
	if (pTempFile == NULL)
	{
		// Not likely to happen, so an English message will suffice.
		wxString msg = _T("Unable to create temporary file at:\n%s");
		msg = msg.Format(msg,tempFilePathName.c_str());
		errors.Add(msg);
		// no need to delete pTempFile here
		return FALSE;
	}
	if (bGetWholeBook)
	{
		// Get the whole book for bookName. We read the data file contents located within 
		// each chapter folder, and concatenate them into a single string buffer
		wxArrayString chNumArray;
		wxDir finder(pathToBookFolder);
		if (finder.Open(pathToBookFolder))
		{
			wxString str = _T("");
			bool bWorking = finder.GetFirst(&str,wxEmptyString,wxDIR_DIRS); // only get directories
			while (bWorking)
			{
				// str should be in the form of numbers "0", "1", "2" ... for as many chapters as are
				// contained in the book.
				// whm Note: The folders representing chapter numbers won't necessarily be traversed
				// in numerical order, therefore we must put the numbers into an array and sort the
				// array, before we concatenate the text chapters into a whole book
				if (str.Length() == 1)
					str = _T("00") + str;
				else if (str.Length() == 2)
					str = _T("0") + str;
				chNumArray.Add(str);
				bWorking = finder.GetNext(&str);
			}
			// now sort the array.
			chNumArray.Sort();
			int ct;
			for (ct = 0; ct < (int)chNumArray.GetCount(); ct++)
			{
				wxString chNumStr;
				chNumStr.Empty();
				chNumStr << chNumArray.Item(ct);
				while (chNumStr.GetChar(0) == _T('0') && chNumStr.Length() > 1)
					chNumStr.Remove(0,1);
				wxString pathToChapterDataFolder = pathToBookFolder + gpApp->PathSeparator + chNumStr + gpApp->PathSeparator + dataFolder;
				bool bOK;
				if (!::wxFileExists(pathToChapterDataFolder))
				{
					// Not likely to happen, so an English message will suffice.
					wxString msg = _T("A Bibledit data folder was not found at:\n%s");
					msg = msg.Format(msg,pathToChapterDataFolder.c_str());
					errors.Add(msg);
					delete pTempFile;
					pTempFile = (wxFile*)NULL;
					return FALSE;
				}
				dataBuffer = GetTextFromFileInFolder(pathToChapterDataFolder);
				bOK = pTempFile->Write(dataBuffer);
				if (!bOK)
				{
					// Not likely to happen, so an English message will suffice.
					wxString msg = _T("Unable to write to temporary file at:\n%s");
					msg = msg.Format(msg,tempFilePathName.c_str());
					errors.Add(msg);
					delete pTempFile;
					pTempFile = (wxFile*)NULL;
					return FALSE;
				}
			}

		}
		else
		{
			// Not likely to happen, so an English message will suffice.
			wxString msg = _T("Unable to open book directory at:\n%s");
			msg = msg.Format(msg,pathToBookFolder.c_str());
			errors.Add(msg);
			delete pTempFile;
			pTempFile = (wxFile*)NULL;
			return FALSE;
		}
	}
	else
	{
		// Get only a chapter. This amounts to reading the data file content of the given
		// chapter folder
		wxString chNumStr = _T("");
		chNumStr << chapterNumber;
		wxString pathToChapterDataFolder = pathToBookFolder + gpApp->PathSeparator + chNumStr + gpApp->PathSeparator + dataFolder;
		bool bOK;
		if (!::wxFileExists(pathToChapterDataFolder))
		{
			// Not likely to happen, so an English message will suffice.
			wxString msg = _T("A Bibledit data folder was not found at:\n%s");
			msg = msg.Format(msg,pathToChapterDataFolder.c_str());
			errors.Add(msg);
			delete pTempFile;
			pTempFile = (wxFile*)NULL;
			return FALSE;
		}
		dataBuffer = GetTextFromFileInFolder(pathToChapterDataFolder);
		bOK = pTempFile->Write(dataBuffer);
		if (!bOK)
		{
			// Not likely to happen, so an English message will suffice.
			wxString msg = _T("Unable to write to temporary file at:\n%s");
			msg = msg.Format(msg,tempFilePathName.c_str());
			errors.Add(msg);
			delete pTempFile;
			pTempFile = (wxFile*)NULL;
			return FALSE;
		}
	}
	delete pTempFile;
	pTempFile = (wxFile*)NULL;
	return TRUE;
}

bool CopyTextFromTempFolderToBibleditData(wxString projectPath, wxString bookName, int chapterNumber, wxString tempFilePathName, wxArrayString& errors)
{
	// The objective is to split up the returning whole-book text 
	// into chapters 0, 1, 2, 3, etc and copy them back to the 
	// Bibledit's individual chapter data folders.

	// construct the path to the Bibledit chapter data files
	wxString pathToBookFolder;
	wxString dataFolder = _T("data");
	pathToBookFolder = projectPath + gpApp->PathSeparator + dataFolder + gpApp->PathSeparator + bookName;
	wxString dataBuffer = _T("");
	bool bDoWholeBook = FALSE;
	if (chapterNumber == -1)
	{
		// get the whole book
		bDoWholeBook = TRUE;
	}
	// We need to ensure it exists otherwise we've nothing to do.
	if (!::wxFileExists(tempFilePathName))
	{
		// Not likely to happen, so an English message will suffice.
		wxString msg = _T("Unable to locate temporary file at:\n%s");
		msg = msg.Format(msg,tempFilePathName.c_str());
		errors.Add(msg);
		return FALSE;
	}

	wxString strBuffer;
	strBuffer = GetTextFromFileInFolder(tempFilePathName);
	wxArrayString chapterStringsForBE;
	chapterStringsForBE = BreakStringBufIntoChapters(strBuffer);

	// The chapterStringsForBE array now contains all parts of the
	// Scripture book partitioned into intro material (0) and chapters
	// (1, 2, 3, 4, etc).
	// Now write the data back to the Bibledit data files
	wxFFile ff;
	int chCount;
	wxString dataPath;
	for (chCount = 0;chCount < (int)chapterStringsForBE.GetCount(); chCount++)
	{
		// Create the path to the "data" folder for the indicated chapter.
		// Note: Except for element zero of the chapterStringsForBE array, we 
		// need to look at the actual chapter number used in the \c n in the
		// array element - we can't assume that all chapters are purely 
		// in sequential order. Some chapters may be missing or skipped in
		// the book text.
		wxString tempChStr;
		tempChStr = chapterStringsForBE.Item(chCount);
		int nTheLen = tempChStr.Length();
		// wx version the pBuffer is read-only so use GetData()
		const wxChar* pBuffer = tempChStr.GetData();
		wxChar* ptr = (wxChar*)pBuffer;		// save start address of Buffer
		wxChar* pEnd;
		pEnd = ptr + nTheLen;// bound past which we must not go
		wxASSERT(*pEnd == _T('\0')); // ensure there is a null at end of Buffer
		int itemLen;
		wxString chapterNumStr = _T("");
		bool bFoundFirstVerse = FALSE;
		bool bOK;
		while (ptr < pEnd)
		{
			int nMkrLen = -1;
			if (Is_VerseMarker(ptr,nMkrLen))
			{
				itemLen = Parse_Marker(ptr,pEnd);
				ptr += itemLen; // point past the \c marker

				itemLen = Parse_NonEol_WhiteSpace(ptr);
				ptr += itemLen; // point past the space

				itemLen = Parse_Number(ptr, pEnd); // ParseNumber doesn't parse over eol chars
				chapterNumStr = GetStringFromBuffer(ptr,itemLen); // get the number
				bFoundFirstVerse = TRUE;
			}
			ptr++;
		}
		if (chCount == 0)
		{
			// This is pre-chapter 1 material that goes into data for the "0" folder
			dataPath = pathToBookFolder + gpApp->PathSeparator + _T("0") + gpApp->PathSeparator + dataFolder;
			
		}
		else if (bFoundFirstVerse)
		{
			// verseNumStr contains the chapter number that followed the \c marker
			dataPath = pathToBookFolder + gpApp->PathSeparator + chapterNumStr + gpApp->PathSeparator + dataFolder;
			
		}
		// Verify that Bibledit actually has a "data" file at the path specified in dataPath
		if (!::wxFileExists(dataPath))
		{
			// Not likely to happen, so an English message will suffice.
			wxString msg = _T("A Bibledit data folder for chapter (%s) was not found at:\n%s");
			msg = msg.Format(msg,chapterNumStr.c_str(),dataPath.c_str());
			errors.Add(msg);
			return FALSE;
			// Any action to take here deferred to caller
		}

		const wxChar writemode[] = _T("w");
		if (ff.Open(dataPath, writemode))
		{
			// no error  when opening
			ff.Write(tempChStr, wxConvUTF8);
			bOK = ff.Close(); // ignore bOK, we don't expect any error for such a basic function
		}
		else
		{
			wxString msg = _T("Unable to create a Bibledit data folder for chapter (%s) at:\n%s");
			msg = msg.Format(msg,chapterNumStr.c_str(),dataPath.c_str());
			errors.Add(msg);
			return FALSE;
			// Any action to take here deferred to caller
		}
	}
	return TRUE;
}

// The next function is created from OnWizardPageChanging() in Projectpage.cpp, and
// tweaked so as to remove support for the latter's context of a wizard dialog; it should
// be called only after an existing active project has been closed off. It will fail with
// an assert tripped in the debug version if the app's m_pKB and/or m_pGlossingKB pointers
// are not NULL on entry. 
// It activates the AI project specified by the second parameter - the last bit of the
// pProjectFolderPath has to be a folder with the name form "XXX to YYY adaptations"
// without a following path separator, which is the standard name form for Adapt It project
// folders, where XXX and YYY are source and target language names, respectively; and
// pProjectName has to be that particular name itself. The pProjectFolderPath needs to have
// been constructed in the caller having taken the possibility of custom or non-custom work
// folder location into account, before the resulting correct path is passed in here.
// 
// Note: calling this function will reset m_curProjectName and m_curProjectPath and
// m_curAdaptionsPath and m_sourceInputsFolderPath, and other version 6 folder's paths (app
// variables) without making any checks related to what these variables may happen to be
// pointing at; this is safe provided any previous active project has been closed.
// Return TRUE if all went well, FALSE if the hookup was unsuccessful for any reason.
// Called in OnOK() of GetSourceTextFromEditor.h and .cpp 
bool HookUpToExistingAIProject(CAdapt_ItApp* pApp, wxString* pProjectName, wxString* pProjectFolderPath)
{
	// ensure there is no document currently open (it also calls UnloadKBs() & sets their
	// pointers to NULL)
	pApp->GetView()->ClobberDocument();
	
	// ensure the adapting KB isn't active. If it is, assert in the debug build, in the
	// release build return FALSE without doing any changes to the current project
	if (pApp->m_pKB != NULL)
	{
		// an English message will do here - it's a development error
		wxMessageBox(_T("HookUpToExistingAIProject() failed. There is an adaptation KB still open."), _T("Error"), wxICON_ERROR);
		wxASSERT(pApp->m_pKB == NULL);
		return FALSE;
	}
	pApp->m_pKB = NULL;

	// Ensure the glossing KB isn't active. If it is, assert in the debug build, in the
	// release build return FALSE
	wxASSERT(pApp->m_pGlossingKB == NULL);
	if (pApp->m_pGlossingKB != NULL)
	{
		// an English message will do here - it's a development error
		wxMessageBox(_T("HookUpToExistingAIProject() failed. There is a glossing KB still open."), _T("Error"), wxICON_ERROR);
		wxASSERT(pApp->m_pGlossingKB == NULL);
		return FALSE;
	}
	pApp->m_pGlossingKB = NULL;

	// we are good to go, as far as KBs are concerned -- neither is loaded yet

	// fill out the app's member variables for the paths etc.
	pApp->m_curProjectName = *pProjectName;
	pApp->m_curProjectPath = *pProjectFolderPath;
	pApp->m_sourceInputsFolderPath = pApp->m_curProjectPath + pApp->PathSeparator + 
									pApp->m_sourceInputsFolderName; 
    // make sure the path to the Adaptations folder is correct
	pApp->m_curAdaptionsPath = pApp->m_curProjectPath + pApp->PathSeparator 
									+ pApp->m_adaptionsFolder;
	pApp->GetProjectConfiguration(pApp->m_curProjectPath); // get the project's configuration settings
	pApp->SetupKBPathsEtc(); //  get the project's(adapting, and glossing) KB paths set
	// get the colours from the project config file's settings just read in
	wxColour sourceColor = pApp->m_sourceColor;
	wxColour targetColor = pApp->m_targetColor;
	wxColour navTextColor = pApp->m_navTextColor;
	// for debugging
	//wxString navColorStr = navTextColor.GetAsString(wxC2S_CSS_SYNTAX);
	// colourData items have to be kept in sync, to avoid crashes if Prefs opened
	pApp->m_pSrcFontData->SetColour(sourceColor);
	pApp->m_pTgtFontData->SetColour(targetColor);
	pApp->m_pNavFontData->SetColour(navTextColor);

	// when not using the wizard, we don't keep track of whether we are choosing an
	// earlier project or not, nor what the folder was for the last document opened, since
	// this function may be used to hook up to different projects at each call!
	pApp->m_bEarlierProjectChosen = FALSE;
	//pApp->m_lastDocPath.Empty(); // whm removed 6Aug11

	// must have this off, if it is left TRUE and we get to end of doc, RecalcLayout() may
	// fail if the phrase box is not in existence
	gbDoingInitialSetup = FALSE; // turn it back off, the pApp->m_targetBox now exists, etc

	// open the two knowledge bases and load their contents;
	pApp->m_pKB = new CKB(FALSE);
	wxASSERT(pApp->m_pKB != NULL);
	{ // this block defines the existence of the wait dialog for loading the regular KB
	CWaitDlg waitDlg(pApp->GetMainFrame());
	// indicate we want the reading file wait message
	waitDlg.m_nWaitMsgNum = 8;	// 8 "Please wait while Adapt It loads the KB..."
	waitDlg.Centre();
	waitDlg.Show(TRUE);
	waitDlg.Update();
	// the wait dialog is automatically destroyed when it goes out of scope below.
	bool bOK = pApp->LoadKB();
	if (bOK)
	{
		pApp->m_bKBReady = TRUE;
		pApp->LoadGuesser(pApp->m_pKB); // whm added 20Oct10

		// now do it for the glossing KB
		pApp->m_pGlossingKB = new CKB(TRUE);
		wxASSERT(pApp->m_pGlossingKB != NULL);
		bOK = pApp->LoadGlossingKB();
		if (bOK)
		{
			pApp->m_bGlossingKBReady = TRUE;
			pApp->LoadGuesser(pApp->m_pGlossingKB); // whm added 20Oct10
		}
		else
		{
			// failure to load the glossing KB is unlikely, an English message will
			// suffice  & an assert in debug build, in release build also return FALSE
			if (pApp->m_pKB != NULL)
			{
				// delete the adapting one we successfully loaded
				pApp->GetDocument()->EraseKB(pApp->m_pKB); // calls delete
				pApp->m_pKB = NULL; //
			}
			pApp->m_bKBReady = FALSE;
			wxMessageBox(_T("HookUpToExistingAIProject(): loading the glossing knowledge base failed"), _T("Error"), wxICON_ERROR);
			wxASSERT(FALSE);
			return FALSE;
		}

		// inform the user if KB backup is currently turned off
		if (pApp->m_bAutoBackupKB)
		{
			// It should not be called when a project is first opened when no 
			// changes have been made; and since on, no message is required
			;
		}
		else
		{
			// show the message only if not a snooper
			if ( (pApp->m_bUseCustomWorkFolderPath && pApp->m_bLockedCustomWorkFolderPath) 
				|| !pApp->m_bUseCustomWorkFolderPath)
			{
				wxMessageBox(
_("A reminder: backing up of the knowledge base is currently turned off.\nTo turn it on again, see the Knowledge Base tab within the Preferences dialog."),
				_T(""), wxICON_INFORMATION);
			}
		}
	}
	else
	{
		// the load of the normal adaptation KB didn't work and the substitute empty KB 
		// was not created successfully, so delete the adaptation CKB & advise the user 
		// to Recreate the KB using the menu item for that purpose. Loading of the glossing
		// KB will not have been attempted if we get here.
		// This is unlikely to have happened, so a simple English message will suffice &
		// and assert in the debug build, for release build return FALSE
		if (pApp->m_pKB != NULL)
		{
			pApp->GetDocument()->EraseKB(pApp->m_pKB); // calls delete
			pApp->m_pKB = NULL;
		}
		pApp->m_bKBReady = FALSE;
		pApp->m_pKB = (CKB*)NULL;
		wxMessageBox(_T("HookUpToExistingAIProject(): loading the adapting knowledge base failed"), _T("Error"), wxICON_ERROR);
		wxASSERT(FALSE);
		return FALSE;
	}
	} // end of CWaitDlg scope, closing the progress dialog

    // whm added 12Jun11. Ensure the inputs and outputs directories are created.
    // SetupDirectories() normally takes care of this for a new project, but we also want
    // existing projects created before version 6 to have these directories too.
	wxString pathCreationErrors = _T("");
	pApp->CreateInputsAndOutputsDirectories(pApp->m_curProjectPath, pathCreationErrors);
	// ignore dealing with any unlikely pathCreationErrors at this point
	
	return TRUE;
}

// SetupLayoutAndView()encapsulates the minimal actions needed after tokenizing
// a string of source text and having identified the project and set up its paths and KBs,
// for laying out the CSourcePhrase instances resulting from the tokenization, and
// displaying the results in the view ready for adapting to take place. Phrase box is put
// at sequence number 0. Returns nothing. Assumes that app's m_curOutputPath has already
// been set correctly.
// Called in GetSourceTextFromEditor.cpp. (It is not limited to use in collaboration
// situations.)
void SetupLayoutAndView(CAdapt_ItApp* pApp, wxString& docTitle)
{
	CAdapt_ItView* pView = pApp->GetView();
	CAdapt_ItDoc* pDoc = pApp->GetDocument();

	// get the title bar, and output path set up right...
	wxString typeName = _T(" - Adapt It");
	#ifdef _UNICODE
	typeName += _T(" Unicode");
	#endif
	pDoc->SetFilename(pApp->m_curOutputPath, TRUE);
	pDoc->SetTitle(docTitle + typeName); // do it also on the frame (see below)
	
	// mark document as modified
	pDoc->Modify(TRUE);

	gbDoingInitialSetup = FALSE; // ensure it's off, otherwise RecalcLayout() may
			// fail after phrase box gets past end of doc

	// do this too... (from DocPage.cpp line 839)
	CMainFrame *pFrame = (CMainFrame*)pView->GetFrame();
	// whm added: In collaboration, we are probably bypassing some of the doc-view
	// black box functions, so we should add a wxFrame::SetTitle() call as was done
	// later in DocPage.cpp about line 966
	pFrame->SetTitle(docTitle + typeName);

	// get the nav text display updated, layout the document and place the
	// phrase box
	int unusedInt = 0;
	TextType dummyType = verse;
	bool bPropagationRequired = FALSE;
	pApp->GetDocument()->DoMarkerHousekeeping(pApp->m_pSourcePhrases, unusedInt, 
												dummyType, bPropagationRequired);
	pApp->GetDocument()->GetUnknownMarkersFromDoc(pApp->gCurrentSfmSet, 
							&pApp->m_unknownMarkers, 
							&pApp->m_filterFlagsUnkMkrs, 
							pApp->m_currentUnknownMarkersStr, 
							useCurrentUnkMkrFilterStatus);

	// calculate the layout in the view
	CLayout* pLayout = pApp->GetLayout();
	pLayout->SetLayoutParameters(); // calls InitializeCLayout() and 
				// UpdateTextHeights() and calls other relevant setters
#ifdef _NEW_LAYOUT
	bool bIsOK = pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_and_piles);
#else
	bool bIsOK = pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_and_piles);
#endif
	if (!bIsOK)
	{
		// unlikely to fail, so just have something for the developer here
		wxMessageBox(_T("Error. RecalcLayout(TRUE) failed in OnImportEditedSourceText()"),
		_T(""), wxICON_STOP);
		wxASSERT(FALSE);
		wxExit();
	}

	// show the initial phraseBox - place it at the first empty target slot
	pApp->m_pActivePile = pLayout->GetPile(0);
	pApp->m_nActiveSequNum = 0;
	if (gbIsGlossing && gbGlossingUsesNavFont)
	{
		pApp->m_pTargetBox->SetOwnForegroundColour(pLayout->GetNavTextColor());
	}
	else
	{
		pApp->m_pTargetBox->SetOwnForegroundColour(pLayout->GetTgtColor());
	}

	// set initial location of the targetBox
	pApp->m_targetPhrase = pView->CopySourceKey(pApp->m_pActivePile->GetSrcPhrase(),FALSE);
	// we must place the box at the first pile
	pApp->m_pTargetBox->m_textColor = pApp->m_targetColor;
	// delay the Place...() call until next OnIdle() if the flag is TRUE
	if (!pApp->bDelay_PlacePhraseBox_Call_Until_Next_OnIdle)
	{ 
		pView->PlacePhraseBox(pApp->m_pActivePile->GetCell(1));
	}
	pView->Invalidate();
	gnOldSequNum = -1; // no previous location exists yet
}

// Saves a wxString, theText (which in the Unicode build should be UTF16 text, and in the
// non-Unicode build, ANSI or ASCII or UTF-8 text), to a file with filename given by
// fileTitle with .txt added internally, and saves the file in the folder with absolute
// path folderPath. Any creation errors are passed back in pathCreationErrors. Return TRUE
// if all went well, else return FALSE. This function can be used to store a text string in
// any of the set of folders predefined for various kinds of storage in a version 6 or
// later project folder; and if by chance the folder isn't already created, it will first
// create it before doing the file save. For the Unicode app, the text is saved converted
// to UTF8 before saving, for the Regular app, it is saved as a series of single bytes.
// The bAddBOM input param is default FALSE, to have a uft16 BOM (0xFFFE)inserted (but
// only if the build is a unicode one, supply TRUE explicitly. A check is made that it is
// not already present, and it is added if absent, not added if already present.)
// (The function which goes the other way is called GetTextFromFileInFolder().)
bool MoveTextToFolderAndSave(CAdapt_ItApp* pApp, wxString& folderPath, 
				wxString& pathCreationErrors, wxString& theText, wxString& fileTitle,
				bool bAddBOM)
{
	// next bit of code taken from app's CreateInputsAndOutputsDirectories()
	wxASSERT(!folderPath.IsEmpty());
	bool bCreatedOK = TRUE;
	if (!folderPath.IsEmpty())
	{
		if (!::wxDirExists(folderPath))
		{
			bool bOK = ::wxMkdir(folderPath);
			if (!bOK)
			{
				if (!pathCreationErrors.IsEmpty())
				{
					pathCreationErrors += _T("\n   ");
					pathCreationErrors += folderPath;
				}
				else
				{
					pathCreationErrors += _T("   ");
					pathCreationErrors += folderPath;
				}
				bCreatedOK = FALSE;
				wxBell(); // a bell will suffice
				return bCreatedOK;
			}
		}
	}
	else
	{
		wxBell();
		return FALSE; // path to the project is empty (we don't expect this
			// so won't even bother to give a message, a bell will do
	}
#ifdef _UNICODE
	bool bBOM_PRESENT = FALSE;
	wxUint32 littleENDIANutf16BOM = 0xFFFE;
	// next line gives us the UTF16 BOM on a machine of either endianness
	wxChar utf16BOM = (wxChar)wxUINT32_SWAP_ON_BE(littleENDIANutf16BOM);
	wxChar firstChar = theText.GetChar(0);
	if (firstChar == utf16BOM)
	{
		bBOM_PRESENT = TRUE;
	}
	if (!bBOM_PRESENT && bAddBOM)
	{
		wxString theBOM = utf16BOM;
		theText = theBOM + theText;
	}
#else
	bAddBOM = bAddBOM; // to prevent compiler warning
#endif
	// create the path to the file
	wxString filePath = folderPath + pApp->PathSeparator + fileTitle + _T(".txt");
	// an earlier version of the file may already be present, if so, just overwrite it
	wxFFile ff;
	bool bOK;
	// NOTE: according to the wxWidgets documentation, both Unicode and non-Unicode builds
	// should use char, and so the next line should be const char writemode[] = "w";, but
	// in the Unicode build, this gives a compile error - wide characters are expected
	const wxChar writemode[] = _T("w");
	if (ff.Open(filePath, writemode))
	{
		// no error  when opening
		ff.Write(theText, wxConvUTF8);
		bOK = ff.Close(); // ignore bOK, we don't expect any error for such a basic function
	}
	else
	{
		bCreatedOK = FALSE;
	}
	return bCreatedOK;
}

// Pass in a fileTitle, that is, filename without a dot or extension, (".txt will be added
// internally) and it will look for a file of that name in a folder with absolute path
// folderPath. The file (if using the Unicode app build) will expect the file's data to be
// UTF8, and it will convert it to UTF16 - no check for UTF8 is done so don't use this on a
// file which doesn't comply). It will return the UTF16 text if it finds such a file, or an
// empty string if it cannot find the file or if it failed to open it's file -- in the
// latter circumstance, since the file failing to be opened is extremely unlikely, we'll
// not distinguish that error from the lack of a such a file being present.
wxString GetTextFromFileInFolder(CAdapt_ItApp* pApp, wxString folderPath, wxString& fileTitle)
{
	wxString fileName = fileTitle + _T(".txt");
	wxASSERT(!folderPath.IsEmpty());
	wxString filePath = folderPath + pApp->PathSeparator + fileName;
	wxString theText; theText.Empty();
	bool bFileExists = ::wxFileExists(filePath);
	if (bFileExists)
	{
		wxFFile ff;
		bool bOK;
		if (ff.Open(filePath)) // default is read mode "r"
		{
			// no error when opening
			bOK = ff.ReadAll(&theText, wxConvUTF8);
			if (bOK)
				ff.Close(); // ignore return value
			else
			{
				theText.Empty();
				return theText;
			}
		}
		else
		{
			return theText;
		}
	}
	return theText;
}

// Pass in a full path and file name (folderPathAndName) and it will look for a file of 
// that name. The file (if using the Unicode app build) will expect the file's data to be
// UTF8, and it will convert it to UTF16 - no check for UTF8 is done so don't use this on a
// file which doesn't comply). It will return the UTF16 text if it finds such a file, or an
// empty string if it cannot find the file or if it failed to open it's file -- in the
// latter circumstance, since the file failing to be opened is extremely unlikely, we'll
// not distinguish that error from the lack of a such a file being present.
// Created by whm 18Jul11 so it can be used for text files other than those with .txt 
// extension.
wxString GetTextFromFileInFolder(wxString folderPathAndName) // an override of above function
{
	wxString theText; theText.Empty();
	bool bFileExists = ::wxFileExists(folderPathAndName);
	if (bFileExists)
	{
		wxFFile ff;
		bool bOK;
		if (ff.Open(folderPathAndName)) // default is read mode "r"
		{
			// no error when opening
			bOK = ff.ReadAll(&theText, wxConvUTF8);
			if (bOK)
				ff.Close(); // ignore return value
			else
			{
				theText.Empty();
				return theText;
			}
		}
		else
		{
			return theText;
		}
	}
	return theText;
}

// Breaks a usfm formatted Scripture text into chapters, storing each chapter
// in an element of the wxArrayString. The material preceding chapter 1 is
// stored in element 0 of the array, the contents of chapter 1 in element 1,
// the contents of chapter 2 in element 2, etc. This is used for 
wxArrayString BreakStringBufIntoChapters(const wxString& bookString)
{
	wxArrayString tempArrayString;
	const wxChar* pBuffer = bookString.GetData();
	int nBufLen = bookString.Len();
	wxChar* ptr = (wxChar*)pBuffer;	// point to the first wxChar (start) of the buffer text
	wxChar* pEnd = (wxChar*)pBuffer + nBufLen;	// point past the last wxChar of the buffer text
	wxASSERT(*pEnd == '\0');

	wxChar* pFirstCharOfSubString = ptr;
	int chapterCharCount = 0;
	bool bHitFirstChapter = FALSE;
	while (ptr < pEnd)
	{
		if (Is_ChapterMarker(ptr))
		{
			if (!bHitFirstChapter)
			{
				// Store the text preceding the first chapter \c marker
				wxString chStr = wxString(pFirstCharOfSubString,chapterCharCount);
				tempArrayString.Add(chStr);
				chapterCharCount = 0;
				pFirstCharOfSubString = ptr;
				bHitFirstChapter = TRUE;
			}
			else
			{
				// store the previous chapter's text
				wxString chStr = wxString(pFirstCharOfSubString,chapterCharCount);
				tempArrayString.Add(chStr);
				chapterCharCount = 0;
				pFirstCharOfSubString = ptr;
			}
		}
		chapterCharCount++;
		ptr++;
	}
	// get the last chapter
	if (chapterCharCount != 0)
	{
		wxString chStr = wxString(pFirstCharOfSubString,chapterCharCount);
		tempArrayString.Add(chStr);
		chapterCharCount = 0;
	}
	return tempArrayString;
}

// Pass in the absolute path to the file, which should be UTF-8. The function opens and
// reads the file into a temporary byte buffer internally which is destroyed when the
// function exits. Internally just before exit the byte buffer is used to create a UTF-16
// text, converting the UTF-8 to UTF-16 in the process. In the Unicode build, if there is
// an initial UTF8 byte order mark (BOM), it gets converted to the UTF-16 BOM. Code at the
// end of the function then tests for the presence of the UTF-16 BOM, and removes it from
// the wxString before the latter is returned to the caller by value The function is used,
// so far, in GetSourceTextFromEditor.cpp, in the OnOK() function. Code initially created
// by Bill Martin, BEW pulled it out into this utility function on 3Jul11, & adding the BOM
// removal code.
// Initially created for reading a chapter of text from a file containing the UTF-8 text,
// but there is nothing which limits this function to just a chapter of data, nor does it
// test for any USFM markup - it's generic.
// BEW 26Jul11, added call of IsOpened() which, if failed to open, allows us to return an
// empty string as the indicator to the caller that the open failed, or the file didn't
// exist at the supplied path
wxString GetTextFromAbsolutePathAndRemoveBOM(wxString& absPath)
{
	wxString emptyStr = _T("");
	wxFile f_txt(absPath,wxFile::read);
	bool bOpenedOK = f_txt.IsOpened();
	if (!bOpenedOK)
	{
		return emptyStr;
	}
	wxFileOffset fileLenTxt;
	fileLenTxt= f_txt.Length();
	// read the raw byte data into pByteBuf (char buffer on the heap)
	char* pTextByteBuf = (char*)malloc(fileLenTxt + 1);
	memset(pTextByteBuf,0,fileLenTxt + 1); // fill with nulls
	f_txt.Read(pTextByteBuf,fileLenTxt);
	wxASSERT(pTextByteBuf[fileLenTxt] == '\0'); // should end in NULL
	f_txt.Close();
	wxString textBuffer = wxString(pTextByteBuf,wxConvUTF8,fileLenTxt);
	free((void*)pTextByteBuf);
	// Now we have to check for textBuffer having an initial UTF-16 BOM, and if so, remove
	// it. No such test nor removal is needed for an ANSI/ASCII build.

	// remove any initial UFT16 BOM (it's 0xFF 0xFE), but on a big-endian machine would be
	// opposite order, so try both
#ifdef _UNICODE
	wxChar litEnd = (wxChar)0xFFFE; // even if I get the endian value reversed, since I try
	wxChar bigEnd = (wxChar)0xFEFF; // both, it doesn't matter
	int offset = textBuffer.Find(litEnd);
	if (offset == 0)
	{
		textBuffer = textBuffer.Mid(1);
	}
	offset = textBuffer.Find(bigEnd);
	if (offset == 0)
	{
		textBuffer = textBuffer.Mid(1);
	}
#endif
    // the file may exist, but be empty except for the BOM - as when using rdwrtp7.exe to
    // get, say, a chapter of free translation from a PT project designated for such data,
    // but which as yet has no books defined for that project -- the call gets a file with
    // nothing in it except the BOM, so having removed the BOM, textBuffer will either be
    // empty or not - nothing else needs to be done except return it now
	return textBuffer;
}

////////////////////////////////////////////////////////////////////////////////
/// \return                 TRUE if all went well, TRUE (yes, TRUE) even if there
///                         was an error -- see the Note below for why
/// \param pApp         ->  ptr to the running instance of the application
/// \param pathToDoc    ->  ref to absolute path to the existing document file
/// \param newSrcText   ->  ref to (possibly newly edited) source text for this doc
/// \param bDoMerger    ->  TRUE if a recursive merger of newSrcText is to be done,
///                         FALSE if not (in which case newSrcText can be empty) 
/// \param bDoLayout    ->  TRUE if RecalcLayout() and placement of phrase box etc
///                         is wanted, FALSE if just app's m_pSourcePhrases is to
///                         be calculated with no layout done
/// \remarks
/// This function provides the minimum for opening an existing document pointed at 
/// by the parameter pathToDoc. It optionally can, prior to laying out etc, merge
/// recursively using MergeUpdatedSrcText() the passed in newSrcText -- the latter
/// may have been edited externally to Adapt It (for example, in Paratext or Bibledit)
/// and so requires the recursive merger be done. The merger is only done provided
/// bDoMerger is TRUE.
/// The app's m_pSourcePhrases list must be empty on entry, otherwise an assert trips
/// in the debug version, or FALSE is returned without anything being done (in the
/// release version). Book mode should be OFF on entry, and this function does not support
/// it's use. The KBs must be loaded already, for the current project in which this
/// document resides.
/// If the bDoMerger flag is TRUE, then after the merge is done, the function will try to
/// find the sequence number of the first adaptable "hole" in the document, and locate
/// the phrase box there (and Copy Source in view menu should have been turned off too,
/// otherwise a possibly bogus copy would be in the phrase box and the user may not notice
/// it is bogus); otherwise it will use 0 as the active sequence number.
/// Used in GetSourceTextFromEditor.cpp in OnOK().
/// Note: never return FALSE, it messes with the wxWidgets implementation of the doc/view 
/// framework, so just always return TRUE even if there was an error
/// Created BEW 3Jul11
////////////////////////////////////////////////////////////////////////////////
bool OpenDocWithMerger(CAdapt_ItApp* pApp, wxString& pathToDoc, wxString& newSrcText, 
		 bool bDoMerger, bool bDoLayout, bool bCopySourceWanted)
{
	wxASSERT(pApp->m_pSourcePhrases->IsEmpty());
	int nActiveSequNum = 0; // default
	if (!pApp->m_pSourcePhrases->IsEmpty())
	{
		wxBell();
		return TRUE;
	}
	gbDoingInitialSetup = FALSE; // ensure it's off, otherwise RecalcLayout() may
			// fail after phrase box gets past end of doc

	gnBeginInsertionsSequNum = -1; // reset for "no current insertions"
	gnEndInsertionsSequNum = -1; // reset for "no current insertions"
	bool bBookMode;
	bBookMode = pApp->m_bBookMode;
	wxASSERT(!bBookMode);
	if (bBookMode)
	{
		wxBell();
		return TRUE;
	}
	wxASSERT(!gbTryingMRUOpen); // must not be trying to open from MRU list
	wxASSERT(!gbConsistencyCheckCurrent); // must not be doing a consistency check
	wxASSERT(pApp->m_pKB != NULL); // KBs must be loaded
	// set the path for saving
	pApp->m_curOutputPath = pathToDoc;

	// much of the code below is from OnOpenDocument() in Adapt_ItDoc.cpp, and
	// much of the rest is from MergeUpdatedSrc.cpp and SetupLayoutAndView()
	// here in helpers.cpp
	wxString extensionlessName; extensionlessName.Empty();
	wxString thePath = pathToDoc;
	wxString extension = thePath.Right(4);
	extension.MakeLower();
	wxASSERT(extension[0] == _T('.')); // check it really is an extension
	bool bWasXMLReadIn = TRUE;

	// get the filename
	wxString fname = pathToDoc;
	fname = MakeReverse(fname);
	int curPos = fname.Find(pApp->PathSeparator);
	if (curPos != -1)
	{
		fname = fname.Left(curPos);
	}
	fname = MakeReverse(fname);
	int length = fname.Len();
	extensionlessName = fname.Left(length - 4);

	CAdapt_ItDoc* pDoc = pApp->GetDocument();
	CAdapt_ItView* pView = pApp->GetView();

	if (extension == _T(".xml"))
	{
		// we have to input an xml document
		wxString thePath = pathToDoc;
		wxFileName fn(thePath);
		wxString fullFileName;
		fullFileName = fn.GetFullName();
		bool bReadOK = ReadDoc_XML(thePath, pDoc); // defined in XML.cpp
		if (!bReadOK)
		{
			wxString s;
				// allow the user to continue
				s = _(
"There was an error parsing in the XML file.\nIf you edited the XML file earlier, you may have introduced an error.\nEdit it in a word processor then try again.");
				wxMessageBox(s, fullFileName, wxICON_INFORMATION);
			return TRUE; // return TRUE to allow the user another go at it
		}
		// app's m_pSourcePhrases list has been populated with CSourcePhrase instances
	}
	// exit here if we only wanted m_pSourcePhrases populated and no recursive merger done
	if (!bDoLayout && !bDoMerger)
	{
		return TRUE;
	}

	if (bDoMerger)
	{
		// first task is to tokenize the (possibly edited) source text just obtained from
		// PT or BE

        // choose a spanlimit int value, (a restricted range of CSourcePhrase instances),
        // use the AdaptitConstant.h value SPAN_LIMIT, set currently to 60. This should be
        // large enough to guarantee some "in common" text which wasn't user-edited, within
        // a span of that size.
		int nSpanLimit = SPAN_LIMIT;	

		// get an input buffer for the new source text & set its content (not really
		// necessary but it allows the copied code below to be used unmodified)
		wxString buffer; buffer.Empty();
		wxString* pBuffer = &buffer;
		buffer += newSrcText; // NOTE: I used += here deliberately, earlier I used =
							// instead, but wxWidgets did not use operator=() properly
							// and it created a buffer full of unknown char symbols
							// and no final null! Weird. But += works fine.
		// The code below is copied from CAdapt_ItView::OnImportEditedSourceText(),
		// comments have been removed to save space, the original code is fully commented
		// so anything not clear can be looked up there
		ChangeParatextPrivatesToCustomMarkers(*pBuffer);
		if (pApp->m_bChangeFixedSpaceToRegularSpace)
			pDoc->OverwriteUSFMFixedSpaces(pBuffer);
		pDoc->OverwriteUSFMDiscretionaryLineBreaks(pBuffer);
#ifndef __WXMSW__
#ifndef _UNICODE
		// whm added 12Apr2007
		OverwriteSmartQuotesWithRegularQuotes(pBuffer);
#endif
#endif
		// parse the new source text data into a list of CSourcePhrase instances
		int nHowMany;
		SPList* pSourcePhrases = new SPList; // for storing the new tokenizations
		nHowMany = pView->TokenizeTextString(pSourcePhrases, *pBuffer, 0); // 0 = initial sequ number value
		SPList* pMergedList = new SPList; // store the results of the merging here
		if (nHowMany > 0)
		{
			MergeUpdatedSourceText(*pApp->m_pSourcePhrases, *pSourcePhrases, pMergedList, nSpanLimit);
            // take the pMergedList list, delete the app's m_pSourcePhrases list's
            // contents, & move to m_pSourcePhrases the pointers in pMergedList...
 			SPList::Node* posCur = pApp->m_pSourcePhrases->GetFirst();
			while (posCur != NULL)
			{
				CSourcePhrase* pSrcPhrase = posCur->GetData();
				posCur = posCur->GetNext();
				pDoc->DeleteSingleSrcPhrase(pSrcPhrase); // also delete partner piles
						// (strictly speaking not necessary since there shouldn't be
						// any yet, but no harm in allowing the attempts)
			}
			// now clear the pointers from the list
			pApp->m_pSourcePhrases->Clear();
			wxASSERT(pApp->m_pSourcePhrases->IsEmpty());

			// make deep copies of the pMergedList instances and add them to the emptied
			// m_pSourcePhrases list, and delete the instance in pMergedList each time
			SPList::Node* posnew = pMergedList->GetFirst();
			CSourcePhrase* pSP = NULL;
			while (posnew != NULL)
			{
				CSourcePhrase* pSrcPhrase = posnew->GetData();
				posnew = posnew->GetNext();
				pSP = new CSourcePhrase(*pSrcPhrase); // shallow copy
				pSP->DeepCopy(); // make shallow copy into a deep one
				pApp->m_pSourcePhrases->Append(pSP); // ignore the Node* returned
				// we no longer need the one in pMergedList
				pDoc->DeleteSingleSrcPhrase(pSrcPhrase, FALSE);
			}
			pMergedList->Clear(); // doesn't try to delete the pointers' memory because
								  // DeleteContents(TRUE) was not called beforehand
			delete pMergedList; // don't leak memory

			// now delete the list formed from the imported new version of the source text
			SPList::Node* pos = pSourcePhrases->GetFirst();
			while (pos != NULL)
			{
				CSourcePhrase* pSrcPhrase = pos->GetData();
				pos = pos->GetNext();
				pDoc->DeleteSingleSrcPhrase(pSrcPhrase, FALSE);
			}
			pSourcePhrases->Clear();
			delete pSourcePhrases; // don't leak memory
		} // end of TRUE block for test: if (nHowMany > 0)
	} // end of TRUE block for test: if (bDoMerger)

	// exit here if we only wanted m_pSourcePhrases populated with the merged new source
	// text 
	if (!bDoLayout)
	{
		return TRUE;
	}
	// get the layout built, view window set up, phrase box placed etc, if wanted
	if (bDoLayout)
	{
		// update the window title
		wxString typeName = _T(" - Adapt It");
		#ifdef _UNICODE
		typeName += _T(" Unicode");
		#endif
		pDoc->SetFilename(pApp->m_curOutputPath, TRUE);
		pDoc->SetTitle(extensionlessName + typeName); // do it also on the frame (see below)
		
		// mark document as modified
		pDoc->Modify(TRUE);

		// do this too... (from DocPage.cpp line 839)
		CMainFrame *pFrame = (CMainFrame*)pView->GetFrame();
		// we should add a wxFrame::SetTitle() call as was done
		// later in DocPage.cpp about line 966
		pFrame->SetTitle(extensionlessName + typeName);
		//pDoc->SetDocumentWindowTitle(fname, extensionlessName);

		// get the backup filename and path produced
		wxFileName fn(pathToDoc);
		wxString filenameStr = fn.GetFullName();
		if (bWasXMLReadIn)
		{
			// it was an *.xml file the user opened
			pApp->m_curOutputFilename = filenameStr;

			// construct the backup's filename
			filenameStr = MakeReverse(filenameStr);
			filenameStr.Remove(0,4); //filenameStr.Delete(0,4); // remove "lmx."
			filenameStr = MakeReverse(filenameStr);
			filenameStr += _T(".BAK");
		}
		pApp->m_curOutputBackupFilename = filenameStr;
		// I haven't defined the backup doc's path, but I don't think I need to, having
		// the filename set correctly should be enough, and other code will get the backup
		// doc file saved in the right place automatically

		// get the nav text display updated, layout the document and place the
		// phrase box
		int unusedInt = 0;
		TextType dummyType = verse;
		bool bPropagationRequired = FALSE;
		pDoc->DoMarkerHousekeeping(pApp->m_pSourcePhrases, unusedInt, 
									dummyType, bPropagationRequired);
		pDoc->GetUnknownMarkersFromDoc(pApp->gCurrentSfmSet, 
								&pApp->m_unknownMarkers, 
								&pApp->m_filterFlagsUnkMkrs, 
								pApp->m_currentUnknownMarkersStr, 
								useCurrentUnkMkrFilterStatus);

		// calculate the layout in the view
		CLayout* pLayout = pApp->GetLayout();
		pLayout->SetLayoutParameters(); // calls InitializeCLayout() and 
					// UpdateTextHeights() and calls other relevant setters
#ifdef _NEW_LAYOUT
		bool bIsOK = pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_and_piles);
#else
		bool bIsOK = pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_and_piles);
#endif
		if (!bIsOK)
		{
			// unlikely to fail, so just have something for the developer here
			wxMessageBox(_T("Error. RecalcLayout(TRUE) failed in OpenDocWithMerger()"),
			_T(""), wxICON_STOP);
			wxASSERT(FALSE);
			wxExit();
		}

		// show the initial phraseBox - place it at nActiveSequNum
		pApp->m_pActivePile = pLayout->GetPile(nActiveSequNum);
		pApp->m_nActiveSequNum = nActiveSequNum; // currently is 0
		if (gbIsGlossing && gbGlossingUsesNavFont)
		{
			pApp->m_pTargetBox->SetOwnForegroundColour(pLayout->GetNavTextColor());
		}
		else
		{
			pApp->m_pTargetBox->SetOwnForegroundColour(pLayout->GetTgtColor());
		}

		// set initial location of the targetBox
		CPile* pPile = pApp->m_pActivePile;
		pPile = pView->GetNextEmptyPile(pPile);
		if (pPile == NULL)
		{
			pApp->m_pActivePile = pLayout->GetPile(nActiveSequNum); // put it back at 0
		}
		else
		{
			pApp->m_pActivePile = pPile;
			pApp->m_nActiveSequNum = pPile->GetSrcPhrase()->m_nSequNumber;
		}
		// if Copy Source wanted, and it is actually turned on, then copy the source text
		// to the active location's box, but otherwise leave the phrase box empty
		if (bCopySourceWanted && pApp->m_bCopySource)
		{
			pApp->m_targetPhrase = pView->CopySourceKey(pApp->m_pActivePile->GetSrcPhrase(),FALSE);
		}
		else
		{
			pApp->m_targetPhrase.Empty();
		}
		// we must place the box at the active pile's location
		pApp->m_pTargetBox->m_textColor = pApp->m_targetColor;
		// delay the Place...() call until next OnIdle() if the flag is TRUE
		if (!pApp->bDelay_PlacePhraseBox_Call_Until_Next_OnIdle)
		{
			pView->PlacePhraseBox(pApp->m_pActivePile->GetCell(1));
		}
		pView->Invalidate();
		gnOldSequNum = -1; // no previous location exists yet
	}
	return TRUE;
}

void UnloadKBs(CAdapt_ItApp* pApp)
{
	if (pApp->m_pKB != NULL)
	{
		pApp->GetDocument()->EraseKB(pApp->m_pKB); // calls delete on CKB pointer
		pApp->m_bKBReady = FALSE;
		pApp->m_pKB = (CKB*)NULL;
	}
	if (pApp->m_pGlossingKB != NULL)
	{
		pApp->GetDocument()->EraseKB(pApp->m_pGlossingKB); // calls delete on CKB pointer
		pApp->m_bGlossingKBReady = FALSE;
		pApp->m_pGlossingKB = (CKB*)NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////
/// \return                       TRUE if all went well, FALSE if project could
///                               not be created
/// \param pApp               ->  ptr to the running instance of the application
/// \param srcLangName        ->  the language name for source text to be used for
///                               creating the project folder name
/// \param tgtLangName        ->  the language name for target text to be used for
///                               creating the project folder name
/// \param srcEthnologueCode  ->  3-letter unique language code from iso639-3,
///                               (for PT or BE collaboration, only pass a value if
///                               the external editor has the right code) 
/// \param tgtEthnologueCode  ->  3-letter unique language code from iso639-3,
///                               (for PT or BE collaboration, only pass a value if
///                               the external editor has the right code)
/// \param bDisableBookMode   ->  TRUE to make it be disabled (sets m_bDisableBookMode)
///                               FALSE to allow it to be user-turn-on-able (for 
///                               collaboration with an external editor, it should be
///                               kept off - hence disabled)
/// \remarks
/// A minimalist utility function (suitable for use when collaborating with Paratext or
/// Bibledit) for creating a new Adapt It project, from a pair of language names plus a
/// pointer to the application class's instance. It pulls out bits and pieces of the
/// ProjectPage.cpp and LanguagesPage.cpp wizard page classes - from their
/// OnWizardPageChanging() functions. It does not try to replicate the wizard, that is, no
/// GUI interface is shown to the user. Instead, where the wizard would ask the user for
/// some manual setup steps, this function takes takes whatever was the currently open
/// project' settings (or app defaults if no project is open). So it takes over it's font
/// settings, colours, etc. It only forms the minimal necessary set of new parameters -
/// mostly paths, the project name, the KB names, their paths, and sets up the set of child
/// folders for various kinds of data storage, such as __SOURCE_INPUTS, etc; and creates
/// empty adapting and glossing KBs and opens them ready for work. Setting of the status
/// bar message is not done here, because what is put there depends on when this function
/// is used, and so that should be done externally once this function returns.
/// It also writes the new project's settings out to the project configuration file just
/// before returning to the caller.
/// Created BEW 5Jul11
////////////////////////////////////////////////////////////////////////////////
bool CreateNewAIProject(CAdapt_ItApp* pApp, wxString& srcLangName, wxString& tgtLangName,
						wxString& srcEthnologueCode, wxString& tgtEthnologueCode,
						bool bDisableBookMode)
{
	// ensure there is no document currently open (it also calls UnloadKBs() & sets their
	// pointers to NULL) -- note, if the doc is dirty, too bad, recent changes will be lost
	pApp->GetView()->ClobberDocument();

    // ensure app does not try to restore last saved doc and active location (these two
    // lines pertain to wizard use, so not relevant here, but just in case I've forgotten
    // something it is good protection to have them)
	pApp->m_bEarlierProjectChosen = FALSE;
	pApp->nLastActiveSequNum = 0;

	// set default character case equivalences for a new project
	pApp->SetDefaultCaseEquivalences();

    // A new project will not yet have a project config file, so set the new project's
    // filter markers list to be equal to the current value for pApp->gCurrentFilterMarkers
	pApp->gProjectFilterMarkersForConfig = pApp->gCurrentFilterMarkers;
	// the above ends material plagiarized from ProjectPage.cpp, what follows comes from
	// LanguagesPage.cpp, & tweaked somewhat

	// If the ethnologue codes are passed in (from PT or BE - both are unlikely at this
	// point in time), then store them in Adapt It's app members for this purpose; but we
	// won't force their use (for collaboration mode it's pointless, since PT and BE don't
	// require them yet); also, set the language names for source and target from what was
	// passed in (typically, coming from PT's or BE's language names)
	pApp->m_sourceName = srcLangName;
	pApp->m_targetName = tgtLangName;
	pApp->m_sourceLanguageCode = srcEthnologueCode; // likely to be empty string
	pApp->m_targetLanguageCode = tgtEthnologueCode; // likely to be empty string

	// ensure Bible book folder mode is off and disabled - if disabling is wanted (need to
	// do this before SetupDirectories() is called)
	if (bDisableBookMode)
	{
		pApp->m_bBookMode = FALSE;
		pApp->m_pCurrBookNamePair = NULL;
		pApp->m_nBookIndex = -1;
		pApp->m_bDisableBookMode = TRUE;
		wxASSERT(pApp->m_bDisableBookMode);
	}

    // Build the Adapt It project's name, and store it in m_curProjectName, and the path to
    // the folder in m_curProjectPath, and all the auxiliary folders for various kinds of
    // data storage, and the other critical paths, m_adaptationsFolder, etc. The
    // SetupDirectories() function does all this, and it takes account of the current value
    // for m_bUseCustomWorkFolderPath so that the setup is done in the custom location
    // (m_customWorkFolderPath) if the latter is TRUE, otherwise done in m_workFolderPath's
    // folder if FALSE.
	pApp->SetupDirectories(); // also sets KB paths and loads KBs & Guesser

	// Now setup the KB paths and get the KBs loaded
	wxASSERT(!pApp->m_curProjectPath.IsEmpty());

	wxColour sourceColor = pApp->m_sourceColor;
	wxColour targetColor = pApp->m_targetColor;
	wxColour navTextColor = pApp->m_navTextColor;
	// for debugging
	//wxString navColorStr = navTextColor.GetAsString(wxC2S_CSS_SYNTAX);
	// colourData items have to be kept in sync, to avoid crashes if Prefs opened
	pApp->m_pSrcFontData->SetColour(sourceColor);
	pApp->m_pTgtFontData->SetColour(targetColor);
	pApp->m_pNavFontData->SetColour(navTextColor);

	// if we need to do more, the facenames etc can be obtained from the 3 structs Bill
	// defined for storing the font information as sent out to the config files, these are
	// SrcFInfo, TgtFInfo and NavFInfo, and are defined as global structs at start of
	// Adapt_It.cpp file
    // Clobbering the old project doesn't do anything to their contents, and so the old
    // project's font settings are available there (need extern defns to be added to
    // helpers.cpp first), and if necessary can be grabbed here and used (if some setting
    // somehow got mucked up)

	// save the config file for the newly made project
	bool bOK = pApp->WriteConfigurationFile(szProjectConfiguration, 
							pApp->m_curProjectPath, projectConfigFile);
	bOK = bOK; // ignore errors (shouldn't fail anyway), 
			   // and avoid compiler warning
	gbDoingInitialSetup = FALSE; // ensure it's off, otherwise RecalcLayout() may
			// fail after phrase box gets past end of doc
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////
/// \return                   a wxString representing the file name or path 
///                                with the changed extension
/// \param filenameOrPath     ->  the filename or filename+path to be changed
/// \param extn               ->  the new extension to be used
/// \remarks
/// 
/// Called from:
/// This function uses wxFileName methods to change any legitimate file extension
/// in filenameOrPath to the value in extn. The incoming extn value may, or may not, 
/// have an initial period - the function strips out any initial period before using
/// the wxFileName methods. Path normalization takes place if needed. Accounts for
/// platform differences (initial dot meaning hidden files in Linux/Mac). 
/// Note: Double extensions can be passed in. This function makes adjustments for 
/// the fact that, if filenameOrPath does not contain a path (but just a filename) 
/// we cannot use the wxFileName::GetPath() and wxFileName::GetFullPath() methods, 
/// because both return a path to the executing program's path rather than an empty 
/// string.
////////////////////////////////////////////////////////////////////////////////
wxString ChangeFilenameExtension(wxString filenameOrPath, wxString extn)
{
	extn.Replace(_T("."),_T(""));
	wxString pathOnly,nameOnly,extOnly,buildStr;
	wxFileName fn(filenameOrPath);
	bool bHasPath;
	if (filenameOrPath.Find(fn.GetPathSeparator()) == wxNOT_FOUND)
		bHasPath = FALSE;
	else
		bHasPath = TRUE;
	fn.Normalize();
	if (fn.HasExt())
		fn.SetExt(extn);
	if (fn.GetDirCount() != 0)
		pathOnly = fn.GetPath();
	nameOnly = fn.GetName();
	extOnly = fn.GetExt();
	fn.Assign(pathOnly,nameOnly,extOnly);
	if (!fn.IsOk())
	{
		return filenameOrPath;
	}
	else
	{
		if (bHasPath)
			return fn.GetFullPath();
		else
			return fn.GetFullName();
	}
}

wxString GetPathToRdwrtp7()
{
	// determine the path and name to rdwrtp7.exe
	wxString rdwrtp7PathAndFileName;
	// Note: Nathan M says that when we've tweaked rdwrtp7.exe to our satisfaction that he will
	// ensure that it gets distributed with future versions of Paratext 7.x. Since AI version 6
	// is likely to get released before that happens, and in case some Paratext users haven't
	// upgraded their PT version 7.x to the distribution that has rdwrtp7.exe installed along-side
	// Paratext.exe, we check for its existence here and use it if it is located in the PT
	// installation folder. If not present, we use our own copy in AI's m_appInstallPathOnly 
	// location (and copy the other dll files if necessary)

	if (::wxFileExists(gpApp->m_ParatextInstallDirPath + gpApp->PathSeparator + _T("rdwrtp7.exe")))
	{
		// rdwrtp7.exe exists in the Paratext installation so use it
		rdwrtp7PathAndFileName = gpApp->m_ParatextInstallDirPath + gpApp->PathSeparator + _T("rdwrtp7.exe");
	}
	else
	{
		// rdwrtp7.exe does not exist in the Paratext installation, so use our copy in AI's install folder
		rdwrtp7PathAndFileName = gpApp->m_appInstallPathOnly + gpApp->PathSeparator + _T("rdwrtp7.exe");
		wxASSERT(::wxFileExists(rdwrtp7PathAndFileName));
		// Note: The rdwrtp7.exe console app has the following dependencies located in the Paratext install 
		// folder (C:\Program Files\Paratext\):
		//    a. ParatextShared.dll
		//    b. ICSharpCode.SharpZipLib.dll
		//    c. Interop.XceedZipLib.dll
		//    d. NetLoc.dll
		//    e. Utilities.dll
		// I've not been able to get the build of rdwrtp7.exe to reference these by setting
		// either using: References > Add References... in Solution Explorer or the rdwrtp7
		// > Properties > Reference Paths to the "c:\Program Files\Paratext\" folder.
		// Until Nathan can show me how (if possible to do it in the actual build), I will
		// here check to see if these dependencies exist in the Adapt It install folder in
		// Program Files, and if not copy them there from the Paratext install folder in
		// Program Files (if the system will let me do it programmatically).
		wxString AI_appPath = gpApp->m_appInstallPathOnly;
		wxString PT_appPath = gpApp->m_ParatextInstallDirPath;
		// Check for any newer versions of the dlls (comparing to previously copied ones) 
		// and copy the newer ones if older ones were previously copied
		wxString fileName = _T("ParatextShared.dll");
		wxString ai_Path;
		wxString pt_Path;
		ai_Path = AI_appPath + gpApp->PathSeparator + fileName;
		pt_Path = PT_appPath + gpApp->PathSeparator + fileName;
		if (!::wxFileExists(ai_Path))
		{
			::wxCopyFile(pt_Path,ai_Path);
		}
		else
		{
			if (FileHasNewerModTime(pt_Path,ai_Path))
				::wxCopyFile(PT_appPath,ai_Path);
		}
		fileName = _T("ICSharpCode.SharpZipLib.dll");
		ai_Path = AI_appPath + gpApp->PathSeparator + fileName;
		pt_Path = PT_appPath + gpApp->PathSeparator + fileName;
		if (!::wxFileExists(ai_Path))
		{
			::wxCopyFile(pt_Path,ai_Path);
		}
		else
		{
			if (FileHasNewerModTime(pt_Path,ai_Path))
				::wxCopyFile(pt_Path,ai_Path);
		}
		fileName = _T("Interop.XceedZipLib.dll");
		ai_Path = AI_appPath + gpApp->PathSeparator + fileName;
		pt_Path = PT_appPath + gpApp->PathSeparator + fileName;
		if (!::wxFileExists(ai_Path))
		{
			::wxCopyFile(pt_Path,ai_Path);
		}
		else
		{
			if (FileHasNewerModTime(pt_Path,ai_Path))
				::wxCopyFile(pt_Path,ai_Path);
		}
		fileName = _T("NetLoc.dll");
		ai_Path = AI_appPath + gpApp->PathSeparator + fileName;
		pt_Path = PT_appPath + gpApp->PathSeparator + fileName;
		if (!::wxFileExists(ai_Path))
		{
			::wxCopyFile(pt_Path,ai_Path);
		}
		else
		{
			if (FileHasNewerModTime(pt_Path,ai_Path))
				::wxCopyFile(pt_Path,ai_Path);
		}
		fileName = _T("Utilities.dll");
		ai_Path = AI_appPath + gpApp->PathSeparator + fileName;
		pt_Path = PT_appPath + gpApp->PathSeparator + fileName;
		if (!::wxFileExists(ai_Path))
		{
			::wxCopyFile(pt_Path,ai_Path);
		}
		else
		{
			if (FileHasNewerModTime(pt_Path,ai_Path))
				::wxCopyFile(pt_Path,ai_Path);
		}
	}
	return rdwrtp7PathAndFileName;
}

wxString GetBibleditInstallPath()
{
	wxString bibledit_gtkPathAndFileName;
	if (::wxFileExists(gpApp->m_BibleditInstallDirPath + gpApp->PathSeparator + _T("bibledit-gtk")))
	{
		// bibledit-gtk exists on the machine so use it
		bibledit_gtkPathAndFileName = gpApp->m_BibleditInstallDirPath + gpApp->PathSeparator + _T("bibledit-gtk");
	}
	return bibledit_gtkPathAndFileName;
}


wxString GetNumberFromChapterOrVerseStr(const wxString& verseStr)
{
	// Parse the string which is of the form: \v n:nnnn:MD5 or \c n:nnnn:MD5 with
	// an :MD5 suffix; or of the form: \v n:nnnn or \c n:nnnn without the :MD5 suffix.
	// This function gets the chapter or verse number as a string and works for
	// either the \c or \v marker.
	// Since the string is of the form: \v n:nnnn:MD5, \c n:nnnn:MD5, (or without :MD5)
	// we cannot use the Parse_Number() to get the number itself since, with the :nnnn... 
	// suffixed to it, it does not end in whitespace or CR LF as would a normal verse 
	// number, so we get the number differently using ordinary wxString methods.
	wxString numStr = verseStr;
	wxASSERT(numStr.Find(_T('\\')) == 0);
	wxASSERT(numStr.Find(_T('c')) == 1 || numStr.Find(_T('v')) == 1); // it should be a chapter or verse marker
	int posSpace = numStr.Find(_T(' '));
	wxASSERT(posSpace != wxNOT_FOUND);
	// get the number and any following part
	numStr = numStr.Mid(posSpace);
	int posColon = numStr.Find(_T(':'));
	if (posColon != wxNOT_FOUND)
	{
		// remove ':' and following part(s)
		numStr = numStr.Mid(0,posColon);
	}
	numStr.Trim(FALSE);
	numStr.Trim(TRUE);
	return numStr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// \return	a wxArrayString representing the usfm structure and extent (character count and MD5
///                         checksum) of the input text buffer
/// \param	fileBuffer -> a wxString buffer containing the usfm text to analyze
/// \remarks
/// Processes the fileBuffer text extracting an wxArrayString representing the 
/// general usfm structure of the fileBuffer (usually the source or target file).
/// whm Modified 5Jun11 to also include a unique MD5 checksum as an additional field so that
/// the form of the representation would be \mkr:nnnn:MD5, where the \mkr field is a usfm
/// marker (such as \p or \v 3); the second field nnnn is a character count of text 
/// directly associated with the marker; and the third field MD5 is a 32 byte hex
/// string, or 0 when no text is associated with the marker, i.e. the MD5 checksum is 
/// used where there are text characters associated with a usfm marker; when the 
/// character count is zero, the MD5 checksum is also 0 in such cases.
///////////////////////////////////////////////////////////////////////////////////////////////////
wxArrayString GetUsfmStructureAndExtent(wxString& fileBuffer)
{
	// process the buffer extracting an wxArrayString representing the general usfm structure
	// from of the fileBuffer (i.e., the source and/or target file buffers)
	// whm Modified 5Jun11 to also include a unique MD5 checksum as an additional field so that
	// the form of the representation would be \mkr:1:nnnn:MD5, where the MD5 is a 32 byte hex
	// string. The MD5 checksum is used where there are text characters associated with a usfm
	// marker; when the character count is zero, the MD5 checksum is also 0 in such cases.
	// Here is an example of what a returned array might look like:
	//	\id:49:2f17e081efa1f7789bac5d6e500fc3d5
	//	\mt:6:010d9fd8a87bb248453b361e9d4b3f38
	//	\c 1:0:0
	//	\s:16:e9f7476ed5087739a673a236ee810b4c
	//	\p:0:0
	//	\v 1:138:ef64c033263f39fdf95b7fe307e2b776
	//	\v 2:152:ec5330b7cb7df48628facff3e9ce2e25
	//	\v 3:246:9ebe9d27b30764602c2030ba5d9f4c8a
	//	\v 4:241:ecc7fb3dfb9b5ffeda9c440db0d856fb
	//	\v 5:94:aea4ba44f438993ca3f44e2ccf5bdcaf
	//	\p:0:0
	//	\v 6:119:639858cb1fc6b009ee55e554cb575352
	//	\f:322:a810d7d923fbedd4ef3a7120e6c4af93
	//	\f*:0:0
	//	\p:0:0
	//	\v 7:121:e17f476eb16c0589fc3f9cc293f26531
	//	\v 8:173:4e3a18eb839a4a57024dba5a40e72536
	//	\p:0:0
	//	\v 9:124:ad649962feeeb2715faad0cc78274227
	//	\p:0:0
	//	\v 10:133:3171aeb32d39e2da92f3d67da9038cf6
	//	\v 11:262:fca59249fe26ee4881d7fe558b24ea49
	//	\s:29:3f7fcd20336ae26083574803b7dddf7c
	//	\p:0:0
	//	\v 12:143:5f71299ac7c347b7db074e3c327cef7e
	//	\v 13:211:6df92d40632c1de539fa3eeb7ba2bc0f
	//	\v 14:157:5383e5a5dcd6976877b4bc82abaf4fee
	//	\p:0:0
	//	\v 15:97:47e16e4ae8bfd313deb6d9aef4e33ca7
	//	\v 16:197:ce14cd0dd77155fa23ae0326bac17bdd
	//	\v 17:51:b313ee0ee83a10c25309c7059f9f46b3
	//	\v 18:143:85b88e5d3e841e1eb3b629adf1345e7b
	//	\v 19:101:2f30dec5b8e3fa7f6a0d433d65a9aa1d
	//	\p:0:0
	//	\v 20:64:b0d7a2fc799e6dc9f35a44ce18755529
	//	\p:0:0
	//	\v 21:90:e96d4a1637d901d225438517975ad7c8
	//	\v 22:165:36f37b24e0685939616a04ae7fc3b56d
	//	\p:0:0
	//	\v 23:96:53b6c4c5180c462c1c06b257cb7c33f8
	//	\f:23:317f2a231b8f9bcfd13a66f45f0c8c72
	//	\fk:19:db64e9160c4329440bed9161411f0354
	//	\fk*:1:5d0b26628424c6194136ac39aec25e55
	//	\f*:7:86221a2454f5a28121e44c26d3adf05c
	//	\v 24-25:192:4fede1302a4a55a4f0973f5957dc4bdd
	//	\v 26:97:664ca3f0e110efe790a5e6876ffea6fc
	//	\c 2:0:0
	//	\s:37:6843aea2433b54de3c2dad45e638aea0
	//	\p:0:0
	//	\v 1:19:47a1f2d8786060aece66eb8709f171dc
	//	\v 2:137:78d2e04d80f7150d8c9a7c123c7bcb80
	//	\v 3:68:8db3a04ff54277c792e21851d91d93e7
	//	\v 4:100:9f3cff2884e88ceff63eb8507e3622d2
	//	\p:0:0
	//	\v 5:82:8d32aba9d78664e51cbbf8eab29fcdc7
	// 	\v 6:151:4d6d314459a65318352266d9757567f1
	//	\v 7:95:73a88b1d087bc4c5ad01dd423f3e45d0
	//	\v 8:71:aaeb79b24bdd055275e94957c9fc13c2
	// Note: The first number field after the usfm (delimited by ':') is a character count 
	// for any text associated with that usfm. The last number field represents the MD5 checksum,
	// except that only usfm markers that are associated with actual text have the 32 byte MD5
	// checksums. Other markers, i.e., \c, \p, have 0 in the MD5 checksum field.
	
	wxArrayString UsfmStructureAndExtentArray;
	const wxChar* pBuffer = fileBuffer.GetData();
	int nBufLen = fileBuffer.Length();
	int itemLen = 0;
	wxChar* ptrSrc = (wxChar*)pBuffer;	// point to the first char (start) of the buffer text
	wxChar* pEnd = ptrSrc + nBufLen;	// point to one char past the end of the buffer text
	wxASSERT(*pEnd == '\0');

	// Note: the wxConvUTF8 parameter of the caller's fileBuffer constructor also
	// removes the initial BOM from the string when converting to a wxString
	// but we'll check here to make sure and skip it if present. Curiously, the string's
	// buffer after conversion also contains the FEFF UTF-16 BOM as its first char in the
	// buffer! The wxString's converted buffer is indeed UTF-16, but why add the BOM to a 
	// memory buffer in converting from UTF8 to UTF16?!
	const int bomLen = 3;
	wxUint8 szBOM[bomLen] = {0xEF, 0xBB, 0xBF};
	bool bufferHasUtf16BOM = FALSE;
	if (!memcmp(pBuffer,szBOM,nBOMLen))
	{
		ptrSrc = ptrSrc + nBOMLen;
	}
	else if (*pBuffer == 0xFEFF)
	{
		bufferHasUtf16BOM = TRUE;
		// skip over the UTF16 BOM in the buffer
		ptrSrc = ptrSrc + 1;
	}
	
	wxString temp;
	temp.Empty();
	int nMkrLen = 0;

	// charCount includes all chars in file except for eol chars
	int charCountSinceLastMarker = 0;
	int eolCount = 0;
	int charCount = 0;
	int charCountMarkersOnly = 0; // includes any white space within and after markers, but not eol chars
	wxString lastMarker;
	wxString lastMarkerNumericAugment;
	wxString stringForMD5Hash;
	wxString md5Hash;
	// Scan the buffer and extract the chapter:verse:count information
	while (ptrSrc < pEnd)
	{
		while (ptrSrc < pEnd && !Is_Marker(ptrSrc,pEnd))
		{
			// This loop handles the parsing and counting of all characters not directly 
			// associated with sfm markers, including eol characters.
			if (*ptrSrc == _T('\n') || *ptrSrc == _T('\r'))
			{
				// its an eol char
				ptrSrc++;
				eolCount++;
			}
			else
			{
				// its a text char other than sfm marker chars and other than eol chars
				stringForMD5Hash += *ptrSrc;
				ptrSrc++;
				charCount++;
				charCountSinceLastMarker++;			
			}
		}
		if (ptrSrc < pEnd)
		{
			// This loop handles the parsing and counting of all sfm markers themselves.
			if (Is_Marker(ptrSrc,pEnd))
			{
				if (!lastMarker.IsEmpty())
				{
					// output the data for the last marker
					wxString usfmDataStr;
					// construct data string for the lastMarker
					usfmDataStr = lastMarker;
					usfmDataStr += lastMarkerNumericAugment;
					usfmDataStr += _T(':');
					usfmDataStr << charCountSinceLastMarker;
					if (charCountSinceLastMarker == 0)
					{
						// no point in storing an 32 byte MD5 hash when the string is empty (i.e., 
						// charCountSinceLastMarker is 0).
						md5Hash = _T('0');
					}
					else
					{
						// Calc md5Hash here, and below
						md5Hash = wxMD5::GetMD5(stringForMD5Hash);
					}
					usfmDataStr += _T(':');
					usfmDataStr += md5Hash;
					charCountSinceLastMarker = 0;
					stringForMD5Hash.Empty();
					lastMarker.Empty();
					UsfmStructureAndExtentArray.Add(usfmDataStr);
				
					lastMarkerNumericAugment.Empty();
				}
			}
			
			if (Is_ChapterMarker(ptrSrc))
			{
				// its a chapter marker
				// parse the chapter marker and following white space

				itemLen = Parse_Marker(ptrSrc, pEnd); // does not parse through eol chars
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the marker
				lastMarker = temp;
				
				ptrSrc += itemLen; // point past the \c marker
				charCountMarkersOnly += itemLen;

				itemLen = Parse_NonEol_WhiteSpace(ptrSrc);
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get non-eol whitespace
				lastMarkerNumericAugment += temp;

				ptrSrc += itemLen; // point at chapter number
				charCountMarkersOnly += itemLen;

				itemLen = Parse_Number(ptrSrc, pEnd); // ParseNumber doesn't parse over eol chars
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the number
				lastMarkerNumericAugment += temp;
				
				ptrSrc += itemLen; // point past chapter number
				charCountMarkersOnly += itemLen;

				itemLen = Parse_NonEol_WhiteSpace(ptrSrc); // parse the non-eol white space following 
													     // the number
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the non-eol white space
				lastMarkerNumericAugment += temp;
				ptrSrc += itemLen; // point past it
				charCountMarkersOnly += itemLen;
				// we've parsed the chapter marker and number and non-eol white space
			}
			else if (Is_VerseMarker(ptrSrc,nMkrLen))
			{
				// Its a verse marker

				// Parse the verse number and following white space
				itemLen = Parse_Marker(ptrSrc, pEnd); // does not parse through eol chars
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the verse marker
				lastMarker = temp;
				
				if (nMkrLen == 2)
				{
					// its a verse marker
					ptrSrc += 2; // point past the \v marker
					charCountMarkersOnly += itemLen;
				}
				else
				{
					// its an Indonesia branch verse marker \vn
					ptrSrc += 3; // point past the \vn marker
					charCountMarkersOnly += itemLen;
				}

				itemLen = Parse_NonEol_WhiteSpace(ptrSrc);
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the non-eol white space
				lastMarkerNumericAugment += temp;
				ptrSrc += itemLen; // point at verse number
				charCountMarkersOnly += itemLen;

				itemLen = Parse_Number(ptrSrc, pEnd); // ParseNumber doesn't parse over eol chars
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the verse number
				lastMarkerNumericAugment += temp;
				ptrSrc += itemLen; // point past verse number
				charCountMarkersOnly += itemLen;

				itemLen = Parse_NonEol_WhiteSpace(ptrSrc); // parse white space which is 
														 // after the marker
				// we don't need the white space on the lastMarkerNumericAugment which we
				// would have to remove during parsing of fields on the : delimiters
				ptrSrc += itemLen; // point past the white space
				charCountMarkersOnly += itemLen; // count following white space with markers
			}
			else if (Is_Marker(ptrSrc,pEnd))
			{
				// Its some other marker.

				itemLen = Parse_Marker(ptrSrc, pEnd); // does not parse through eol chars
				temp = GetStringFromBuffer(ptrSrc,itemLen); // get the marker
				lastMarker = temp;
				ptrSrc += itemLen; // point past the marker
				charCountMarkersOnly += itemLen;

				itemLen = Parse_NonEol_WhiteSpace(ptrSrc);
				// we don't need the white space but we count it
				ptrSrc += itemLen; // point past the white space
				charCountMarkersOnly += itemLen; // count following white space with markers
			}
		} // end of TRUE block for test: if (ptrSrc < pEnd)
		else
		{
			// ptrSrc is pointing at or past the pEnd
			break;
		}
	} // end of loop: while (ptrSrc < pEnd)
	
	// output data for any lastMarker that wasn't output in above main while 
	// loop (at the end of the file)
	if (!lastMarker.IsEmpty())
	{
		wxString usfmDataStr;
		// construct data string for the lastMarker
		usfmDataStr = lastMarker;
		usfmDataStr += lastMarkerNumericAugment;
		usfmDataStr += _T(':');
		usfmDataStr << charCountSinceLastMarker;
		if (charCountSinceLastMarker == 0)
		{
			// no point in storing an 32 byte MD5 hash when the string is empty (i.e., 
			// charCountSinceLastMarker is 0).
			md5Hash = _T('0');
		}
		else
		{
			// Calc md5Hash here, and below
			md5Hash = wxMD5::GetMD5(stringForMD5Hash);
		}
		usfmDataStr += _T(':');
		usfmDataStr += md5Hash;
		charCountSinceLastMarker = 0;
		stringForMD5Hash.Empty();
		lastMarker.Empty();
		UsfmStructureAndExtentArray.Add(usfmDataStr);
	
		lastMarkerNumericAugment.Empty();
	}
	//int ct;
	//for (ct = 0; ct < (int)UsfmStructureAndExtentArray.GetCount(); ct++)
	//{
	//	wxLogDebug(UsfmStructureAndExtentArray.Item(ct));
	//}
	
	// Note: Our pointer is always incremented to pEnd at the end of the file which is one char beyond
	// the actual last character so it represents the total number of characters in the buffer. 
	// Thus the Total Count below doesn't include the beginning UTF-16 BOM character, which is also the length
	// of the wxString buffer as reported in nBufLen by the fileBuffer.Length() call.
	// Actual size on disk will be 2 characters larger than charCount's final value here because
	// the file on disk will have the 3-char UTF8 BOM, and the fileBuffer here has the 
	// 1-char UTF-16 BOM.
	int utf16BomLen;
	if (bufferHasUtf16BOM)
		utf16BomLen = 1;
	else
		utf16BomLen = 0;
	wxLogDebug(_T("Total Count = %d [charCount (%d) + eolCount (%d) + charCountMarkersOnly (%d)] Compare to nBufLen = %d"),
		charCount+eolCount+charCountMarkersOnly,charCount,eolCount,charCountMarkersOnly,nBufLen - utf16BomLen);
	return UsfmStructureAndExtentArray;
}

// Gets the initial Usfm marker from a string element of an array produced by 
// GetUsfmStructureAndExtent(). Assumes the incoming str begins with a back
// slash char and that a ':' delimits the end of the marker field
// 
// Note: for a \c or \v line, it gets both the marker, the following space, and whatever
// chapter or verse, or verse range, follows the space. If the strict marker and nothing
// else is wanted, use GetStrictUsfmMarkerFromStructExtentString() instead.
wxString GetInitialUsfmMarkerFromStructExtentString(const wxString str)
{
	wxASSERT(str.GetChar(0) == gSFescapechar);
	int posColon = str.Find(_T(':'),FALSE);
	wxASSERT(posColon != wxNOT_FOUND);
	wxString tempStr = str.Mid(0,posColon);
	return tempStr;
}

wxString GetStrictUsfmMarkerFromStructExtentString(const wxString str)
{
	wxASSERT(str.GetChar(0) == gSFescapechar);
	int posColon = str.Find(_T(':'),FALSE);
	wxASSERT(posColon != wxNOT_FOUND);
	wxString tempStr = str.Mid(0,posColon);
	wxString aSpace = _T(' ');
	int offset = tempStr.Find(aSpace);
	if (offset == wxNOT_FOUND)
	{
		return tempStr;
	}
	else
	{
		tempStr = tempStr.Left(offset);
	}
	return tempStr;
}

// get the substring for thee character count, and return it's base-10 value
int GetCharCountFromStructExtentString(const wxString str)
{
	wxASSERT(str.GetChar(0) == gSFescapechar);
	int posColon = str.Find(_T(':'),FALSE);
	wxASSERT(posColon != wxNOT_FOUND);
	wxString astr = str.Mid(posColon + 1);
	posColon = astr.Find(_T(':'),FALSE);
	wxASSERT(posColon != wxNOT_FOUND);
	wxString numStr = astr(0,posColon);
	return wxAtoi(numStr);
}


wxString GetFinalMD5FromStructExtentString(const wxString str)
{
	wxASSERT(str.GetChar(0) == gSFescapechar);
	int posColon = str.Find(_T(':'),TRUE); // TRUE - find from right end
	wxASSERT(posColon != wxNOT_FOUND);
	wxString tempStr = str.Mid(posColon+1);
	return tempStr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// \return	an enum value of one of the following: noDifferences, usfmOnlyDiffers, textOnlyDiffers,
///     or usfmAndTextDiffer.
/// \param	usfmText1	-> a structure and extent wxArrayString created by GetUsfmStructureAndExtent()
/// \param	usfmText2	-> a second structure and extent wxArrayString created by GetUsfmStructureAndExtent()
/// \remarks
/// Compares the two incoming array strings and determines if they have no differences, or if only the 
/// usfm structure differs (but text parts are the same), or if only the text parts differ (but the 
/// usfm structure is the same), or if both the usfm structure and the text parts differ. The results of
/// the comparison are returned to the caller via one of the CompareUsfmTexts enum values.
///////////////////////////////////////////////////////////////////////////////////////////////////
enum CompareUsfmTexts CompareUsfmTextStructureAndExtent(const wxArrayString& usfmText1, const wxArrayString& usfmText2)
{
	int countText1 = (int)usfmText1.GetCount();
	int countText2 = (int)usfmText2.GetCount();
	bool bTextsHaveDifferentUSFMMarkers = FALSE;
	bool bTextsHaveDifferentTextParts = FALSE;
	if (countText2 == countText1)
	{
		// the two text arrays have the same number of lines, i.e. the same number of usfm markers
		// so we can easily compare those markers in this case by scanning through each array
		// line-by-line and compare their markers, and their MD5 checksums 
		int ct;
		for (ct = 0; ct < countText1; ct++)
		{
			wxString mkr1 = GetInitialUsfmMarkerFromStructExtentString(usfmText1.Item(ct));
			wxString mkr2 = GetInitialUsfmMarkerFromStructExtentString(usfmText2.Item(ct));
			if (mkr1 != mkr2)
			{
				bTextsHaveDifferentUSFMMarkers = TRUE;
			}
			wxString md5_1 = GetFinalMD5FromStructExtentString(usfmText1.Item(ct));
			wxString md5_2 = GetFinalMD5FromStructExtentString(usfmText2.Item(ct));
			if (md5_1 != md5_2)
			{
				bTextsHaveDifferentTextParts = TRUE;
			}
		}
	}
	else
	{
		// The array counts differ, which means that at least, the usfm structure 
		// differs too since each line of the array is defined by the presence of a 
		// usfm.
		bTextsHaveDifferentUSFMMarkers = TRUE;
		// But, what about the Scripture text itself? Although the usfm markers 
		// differ at least in number, we need to see if the text itself differs 
		// for those array lines which the two arrays share in common. For 
		// Scripture texts the parts that are shared in common are represented 
		// in the structure-and-extent arrays primarily as the lines with \v n 
		// as their initial marker. We may want to also account for embedded verse
		// markers such as \f footnotes, \fe endnotes, and other medial markers 
		// that can occur within the sacred text. We'll focus first on the \v n
		// lines, and posibly handle footnotes as they are encountered. The MD5
		// checksum is the most important part to deal with, as comparing them
		// across corresponding verses will tell us whether there has been a
		// change in the text between the two versions.
		
		// Scan through the texts comparing the MD5 checksums of their \v n
		// lines
		int ct1 = 0;
		int ct2 = 0;
		// use a while loop here since we may be scanning and comparing different
		// lines in the arrays - in this block the array counts differ.
		wxString mkr1;
		wxString mkr2;
		while (ct1 < countText1 && ct2 < countText2)
		{
			mkr1 = _T("");
			mkr2 = _T("");
			if (GetNextVerseLine(usfmText1,ct1))
			{
				mkr1 = GetInitialUsfmMarkerFromStructExtentString(usfmText1.Item(ct1));
				if (GetNextVerseLine(usfmText2,ct2))
				{
					mkr2 = GetInitialUsfmMarkerFromStructExtentString(usfmText2.Item(ct2));
				}
				if (!mkr2.IsEmpty())
				{
					// Check if we are looking at the same verse in both arrays
					if (mkr1 == mkr2)
					{
						// The verse markers are the same - we are looking at the same verse
						// in both arrays.
						// Check the MD5 sums associated with the verses for any changes.
						wxString md5_1 = GetFinalMD5FromStructExtentString(usfmText1.Item(ct1));
						wxString md5_2 = GetFinalMD5FromStructExtentString(usfmText2.Item(ct2));
						if (md5_1 == md5_2)
						{
							// the verses have not changed, leave the flag at its default of FALSE;
							;
						}
						else
						{
							// there has been a change in the texts, so set the flag to TRUE
							bTextsHaveDifferentTextParts = TRUE;
						}
					}
					else
					{
						// the verse markers are different. Perhaps the user edited out a verse
						// or combined the verses into a bridged verse.
						// We have no way to deal with checking for text changes in making a 
						// bridged verse so we will assume there were changes made in such cases.
						bTextsHaveDifferentTextParts = TRUE;
						break;
						
					}
				}
				else
				{
					// mkr2 is empty indicating we've prematurely reached the end of verses in
					// usfmText2. This amounts to a change in both the text and usfm structure.
					// We've set the flag for change in usfm structure above, so now we set the
					// bTextsHaveDifferentTextParts flag and break out.
					bTextsHaveDifferentTextParts = TRUE;
					break;
				}
			}
			if (mkr1.IsEmpty())
			{
				// We've reached the end of verses for usfmText1. It is possible that there 
				// could be more lines with additional verse(s) in usfmText2 - as in the case 
				// that a verse bridge was unbridged into individual verses. 
				if (GetNextVerseLine(usfmText2,ct2))
				{
					mkr2 = GetInitialUsfmMarkerFromStructExtentString(usfmText2.Item(ct2));
				}
				if (!mkr2.IsEmpty())
				{
					// We have no way to deal with checking for text changes in un-making a 
					// bridged verse so we will assume there were changes made in such cases.
					bTextsHaveDifferentTextParts = TRUE;
					break;
				}
			}
			ct1++;
			ct2++;
		}
	}

	if (bTextsHaveDifferentUSFMMarkers && bTextsHaveDifferentTextParts)
		return usfmAndTextDiffer;
	if (bTextsHaveDifferentUSFMMarkers)
		return usfmOnlyDiffers;
	if (bTextsHaveDifferentTextParts)
		return textOnlyDiffers;
	// The only case left is that there are no differences
	return noDifferences;
}

// Use the MD5 checksums approach to compare two text files for differences: the MD5
// approach can detect differences in the usfm structure, or just in either punctuation or
// words changes or both (it can't distinguish puncts changes from word changes). This
// function checks for puncts and/or word changes only.
// Return TRUE if there are such changes, FALSE if there are none of that kind
bool IsTextOrPunctsChanged(wxString& oldText, wxString& newText)
{
	wxArrayString oldArr;
	wxArrayString newArr;
	oldArr = GetUsfmStructureAndExtent(oldText);
	newArr = GetUsfmStructureAndExtent(newText);
	enum CompareUsfmTexts value = CompareUsfmTextStructureAndExtent(oldArr, newArr);
	switch(value)
	{
	case noDifferences:
		return FALSE;
	case usfmOnlyDiffers:
		return FALSE;
	case textOnlyDiffers:
		return TRUE;
	case usfmAndTextDiffer:
		return TRUE;
	default:
		return FALSE;
	}
}

// overload of the above, taking arrays and start & finish item indices as parameters;
// this saves having to reconstitute the text in order to compare parts of two different
// versions of the same text; do it instead from the original md5 arrays for those texts
bool IsTextOrPunctsChanged(wxArrayString& oldMd5Arr, int oldStart, int oldEnd,
							wxArrayString& newMd5Arr, int newStart, int newEnd)
{
	wxArrayString oldArr;
	wxArrayString newArr;
	oldArr = ObtainSubarray(oldMd5Arr, oldStart, oldEnd);
	newArr = ObtainSubarray(newMd5Arr, newStart, newEnd);
	enum CompareUsfmTexts value = CompareUsfmTextStructureAndExtent(oldArr, newArr);
	switch(value)
	{
	case noDifferences:
		return FALSE;
	case usfmOnlyDiffers:
		return FALSE;
	case textOnlyDiffers:
		return TRUE;
	case usfmAndTextDiffer:
		return TRUE;
	default:
		return FALSE;
	}
}

// Use the MD5 checksums approach to compare two text files for differences: the MD5
// approach can detect differences in the usfm structure, or just in either punctuation or
// words changes or both (it can't distinguish puncts changes from word changes). This
// function checks for usfm changes only.
// Return TRUE if there are such changes, FALSE if there are none of that kind
bool IsUsfmStructureChanged(wxString& oldText, wxString& newText)
{
	wxArrayString oldArr;
	wxArrayString newArr;
	oldArr = GetUsfmStructureAndExtent(oldText);
	newArr = GetUsfmStructureAndExtent(newText);
	enum CompareUsfmTexts value = CompareUsfmTextStructureAndExtent(oldArr, newArr);
	switch(value)
	{
	case noDifferences:
		return FALSE;
	case usfmOnlyDiffers:
		return TRUE;
	case textOnlyDiffers:
		return FALSE;
	case usfmAndTextDiffer:
		return TRUE;
	default:
		return FALSE;
	}
}

// Advances index until usfmText array at that index points to a \v n line.
// If there are no more \v n lines, then the function returns FALSE. The
// index is returned to the caller via the index reference parameter.
// Assumes that index is not already at a line with \v when called; that is, the line
// pointed at on entry is a candidate for being the next verse line
bool GetNextVerseLine(const wxArrayString& usfmText, int& index)
{
	int totLines = (int)usfmText.GetCount();
	while (index < totLines)
	{
		wxString testingStr = usfmText.Item(index);
		if (testingStr.Find(_T("\\v")) != wxNOT_FOUND)
		{
			return TRUE;
		}
		else
		{
			index++;
		}
	}
	return FALSE;
}

// Like GetNextVerseLine(), except that it assumes that index is at a line with \v when
// called. Advances index past the line with \v and then searches successive lines of
// usfmText array until index points to a later \v n line. If there are no more \v n lines,
// then the function returns FALSE. The index is returned to the caller via the index
// reference parameter.
bool GetAnotherVerseLine(const wxArrayString& usfmText, int& index)
{
	int totLines = (int)usfmText.GetCount();
	index++;
	while (index < totLines)
	{
		wxString testingStr = usfmText.Item(index);
		if (testingStr.Find(_T("\\v")) != wxNOT_FOUND)
		{
			return TRUE;
		}
		else
		{
			index++;
		}
	}
	return FALSE;
}


bool IsVerseLine(const wxArrayString& usfmText, int index)
{
	int totLines = (int)usfmText.GetCount();
	wxASSERT(index < totLines);
	if (index < totLines)
	{
		return ((usfmText.Item(index)).Find(_T("\\v")) != wxNOT_FOUND);
	}
	else
	{
		return FALSE;
	}
}

/// returns                 TRUE for a successful analysis, FALSE if unsuccessful or an
///                         empty string was passed in
/// 
/// \param  strChapVerse        ->  ref to a chapter:verse string, or chapter:verse_range string which
///                                 is passed in to be analysed into its parts
/// \param  strChapter          <-  ref to the chapter number as a string
/// \param  strDelimiter        <-  if present, whatever separates the parts of a verse range
/// \param  strStartingVerse    <-  the starting verse of a range, as a string
/// \param  strStartingVerseSuffix <- usually a single lower case letter such as a or b
///                                   after strStartingVerse
/// \param  strEndingVerse      <-  the ending verse of a range, as a string
/// \param  strEndingVerseSuffix <- usually a single lower case letter such as a or b,
///                                 after strEndingVerse
/// \remarks
/// This is similar to the MergeUpdatedSrc.cpp file's  AnalyseChapterVerseRef() , but is
/// simpler. It can handle Arabic digits - but as strings, since no attempt is made here to
/// convert chapter or verse strings into their numeric values. This function is used for
/// populating the relevant members of a VChunkAndMap struct, used in the chunking and
/// comparisons and replacements of data into the final wxString text to be sent back to
/// whichever external editor we are currently collaborating with.
/// Note: input is a "chap:verse" string, where "verse" might be a verse range, or a part
/// verse with a suffix such as a b or c. The StuctureAndExtents array which we obtain the
/// verse information and chapter information lines from, will have those two bits of
/// information on separate lines - and so the caller will form the "chap:verse" string
/// from those lines before passing it in to AnalyseChapterVerseRef_For_Collab() for
/// analysis.
/* not needed, BEW removed 4Aug11
bool AnalyseChapterVerseRef_For_Collab(wxString& strChapVerse, wxString& strChapter, 
		wxString& strDelimiter, wxString& strStartingVerse, wxChar& charStartingVerseSuffix, 
		wxString& strEndingVerse, wxChar& charEndingVerseSuffix)
{
    // The Adapt It chapterVerse reference string is always of the form ch:vs or
    // ch:vsrange, the colon is always there except for single chapter books. Single
    // chapter books with no chapter marker will return 1 as the chapter number
	strStartingVerse.Empty();
	strEndingVerse.Empty();
	charStartingVerseSuffix = _T('\0');
	charEndingVerseSuffix = _T('\0');
	strDelimiter.Empty();
	if (strChapVerse.IsEmpty())
		return FALSE; // reference passed in was empty
	wxString range;
	range.Empty();

	// first determine if there is a chapter number present; handle Arabic digits if on a
	// Mac machine and Arabic digits were input
	int nFound = strChapVerse.Find(_T(':'));
	if (nFound == wxNOT_FOUND)
	{
		// no chapter number, so set chapter to 1
		range = strChapVerse;
		strChapter = _T("1");
	}
	else
	{
		// chapter number exists, extract it and put the remainder after the colon into range
		strChapter = SpanExcluding(strChapVerse,_T(":"));
		nFound++; // index the first char after the colon
		range = strChapVerse.Mid(nFound);
	}

	// now deal with the verse range, or single verse
	int numChars = range.Len();
	int index;
	wxChar aChar = _T('\0');
	// get the verse number, or the first verse number of the range
	int count = 0;
	for (index = 0; index < numChars; index++)
	{
		aChar = range.GetChar(index);
#ifdef __WXMAC__
// Kludge because the atoi() function in the MacOS X standard library can't handle Arabic digits
		if (aChar >= (wchar_t)0x6f0 && aChar <= (wchar_t)0x6f9)
		{
			aChar = aChar & (wchar_t)0x3f; // zero out the higher bits of this Arabic digit
		}
#endif
		int isDigit = wxIsdigit(aChar);
		if (isDigit != 0)
		{
			// it's a digit
			strStartingVerse += aChar;
			count++;
		}
		else
		{
			// it's not a digit, so exit with what we've collected so far
			break;
		}
	}
	if (count == numChars)
	{
		// all there was in the range variable was the digits of a verse number, so set
		// the return parameter values and return TRUE
		strEndingVerse = strStartingVerse;
		return TRUE;
	}
	else
	{
		// there's more, but get what we've got so far and trim that stuff off of range
		range = range.Mid(count);
		numChars = range.Len();
		// if a part-verse marker (assume one of a or b or c only), get it
		aChar = range.GetChar(0);
		if ( aChar == _T('a') || aChar == _T('b') || aChar == _T('c'))
		{
			charStartingVerseSuffix = aChar;
			range = range.Mid(1); // remove the suffix character
			numChars = range.Len();
		}
		if (numChars == 0)
		{
			// we've exhausted the range string, fill params and return TRUE
			strEndingVerse = strStartingVerse;
			charEndingVerseSuffix = charStartingVerseSuffix;
			return TRUE;
		}
		else 
		{
			// there is more still, what follows must be the delimiter, we'll assume it is
			// possible to have more than a single character (exotic scripts might need
			// more than one) so search for a following digit as the end point, or string
			// end; and handle Arabic digits too (ie. convert them)
			count = 0;
			for (index = 0; index < numChars; index++)
			{
				aChar = range.GetChar(index);
#ifdef __WXMAC__
// Kludge because the atoi() function in the MacOS X standard library can't handle Arabic digits
				if (aChar >= (wchar_t)0x6f0 && aChar <= (wchar_t)0x6f9)
				{
					aChar = aChar & (wchar_t)0x3f; // zero out the higher bits of this Arabic digit
				}
#endif 
				int isDigit = wxIsdigit(aChar);
				if (isDigit != 0)
				{
					// it's a digit, so we've reached the end of the delimiter
					break;
				}
				else
				{
					// it's not a digit, so it's part of the delimiter string
					strDelimiter += aChar;
					count++;
				}
			}
			if (count == numChars)
			{
				// it was "all delimiter" - a formatting error, as there is not verse
				// number to indicate the end of the range - so just take the starting
				// verse number (and any suffix) and return FALSE
				strEndingVerse = strStartingVerse;
				charEndingVerseSuffix = charStartingVerseSuffix;
				return FALSE;
			}
			else
			{
                // we stopped earlier than the end of the range string, and so presumably
                // we stopped at the first digit of the verse which ends the range
				range = range.Mid(count); // now just the final verse and possibly an a, b or c suffix
				numChars = range.Len();
				// get the final verse...
				count = 0;
				for (index = 0; index < numChars; index++)
				{
					aChar = range.GetChar(index);
#ifdef __WXMAC__
// Kludge because the atoi() function in the MacOS X standard library can't handle Arabic digits
					if (aChar >= (wchar_t)0x6f0 && aChar <= (wchar_t)0x6f9)
					{
						aChar = aChar & (wchar_t)0x3f; // zero out the higher bits of this Arabic digit
					}
#endif
					int isDigit = wxIsdigit(aChar);
					if (isDigit != 0)
					{
						// it's a digit
						strEndingVerse += aChar;
						count++;
					}
					else
					{
						// it's not a digit, so exit with what we've collected so far
						break;
					}
				}
				if (count == numChars)
				{
                    // all there was in the range variable was the digits of the ending
                    // verse number, so set the return parameter values and return TRUE
					return TRUE;
				}
				else
				{
					// there's more, but get what we've got so far and trim that stuff 
					// off of range
					range = range.Mid(count);
					numChars = range.Len();
					// if a part-verse marker (assume one of a or b or c only), get it
					if (numChars > 0)
					{
						// what remains should just be a final a or b or c
						aChar = range.GetChar(0);
						if ( aChar == _T('a') || aChar == _T('b') || aChar == _T('c'))
						{
							charEndingVerseSuffix = aChar;
							range = range.Mid(1); // remove the suffix character
							numChars = range.Len(); // numChars should now be 0
						}
						if (numChars != 0)
						{
							// rhere's still something remaining, so just ignore it, but
							// alert the developer, not with a localizable string
							wxString suffix1;
							wxString suffix2;
							if (charStartingVerseSuffix != _T('\0'))
							{
								suffix1 = charStartingVerseSuffix;
							}
							if (charEndingVerseSuffix != _T('\0'))
							{
								suffix2 = charEndingVerseSuffix;
							}
							wxString msg;
							msg = msg.Format(
_T("The verse range was parsed, and the following remains unparsed: %s\nfrom the specification %s:%s%s%s%s%s%s"),
							range.c_str(),strChapter.c_str(),strStartingVerse.c_str(),suffix1.c_str(),
							strDelimiter.c_str(),strEndingVerse.c_str(),suffix2.c_str(),range.c_str());
							wxMessageBox(msg,_T("Verse range specification error (ignored)"),wxICON_WARNING);
						}
					} //end of TRUE block for test: if (numChars > 0)
				} // end of else block for test: if (count == numChars)
			} // end of else block for test: if (count == numChars)
		} // end of else block for test: if (count == numChars)
	} // end of else block for test: if (numChars == 0)
	return TRUE;
}
*/

// Pass in a VerseAnalysis struct by pointer, and fill out it's members internally by
// analysing what verseNum contains, handling bridged verses, part verses, simple verses,
// etc.
bool DoVerseAnalysis(const wxString& verseNum, VerseAnalysis& rVerseAnal)
{
	InitializeVerseAnalysis(rVerseAnal);
	wxString range = verseNum;

	// deal with the verse range, or it might just be a simple verse number
	int numChars = range.Len();
	int index;
	wxChar aChar = _T('\0');
	// get the verse number, or the first verse number of the range
	int count = 0;
	for (index = 0; index < numChars; index++)
	{
		aChar = range.GetChar(index);
#ifdef __WXMAC__
// Kludge because the atoi() function in the MacOS X standard library can't handle Arabic digits
		if (aChar >= (wchar_t)0x6f0 && aChar <= (wchar_t)0x6f9)
		{
			aChar = aChar & (wchar_t)0x3f; // zero out the higher bits of this Arabic digit
		}
#endif /* __WXMAC__ */
		int isDigit = wxIsdigit(aChar); // returns non-zero number when TRUE, 0 when FALSE
		if (isDigit != 0)
		{
			// it's a digit
			rVerseAnal.strStartingVerse += aChar;
			count++;
		}
		else
		{
			// it's not a digit, so exit with what we've collected so far
			break;
		}
	}
	if (count == numChars)
	{
		// all there was in the range variable was the digits of a verse number, so set
		// the relevant members in pVerseAnal & return
		rVerseAnal.strEndingVerse = verseNum;
		return rVerseAnal.bIsComplex;
	}
	else
	{
		// there's more, but get what we've got so far and trim that stuff off of range
		range = range.Mid(count);
		numChars = range.Len();
		// if a part-verse marker (assume one of a or b or c only), get it
		aChar = range.GetChar(0);
		if ( aChar == _T('a') || aChar == _T('b') || aChar == _T('c'))
		{
			rVerseAnal.bIsComplex = TRUE;
			rVerseAnal.charStartingVerseSuffix = aChar;
			range = range.Mid(1); // remove the suffix character
			numChars = range.Len();
		}
		if (numChars == 0)
		{
			// we've exhausted the range string, fill params and return TRUE
			rVerseAnal.strEndingVerse = rVerseAnal.strStartingVerse;
			rVerseAnal.charEndingVerseSuffix = rVerseAnal.charStartingVerseSuffix;
			return rVerseAnal.bIsComplex;
		}
		else 
		{
			// there is more still, what follows must be the delimiter, we'll assume it is
			// possible to have more than a single character (exotic scripts might need
			// more than one) so search for a following digit as the end point, or string
			// end; and handle Arabic digits too (ie. convert them)
			count = 0;
			rVerseAnal.bIsComplex = TRUE;
			for (index = 0; index < numChars; index++)
			{
				aChar = range.GetChar(index);
#ifdef __WXMAC__
// Kludge because the atoi() function in the MacOS X standard library can't handle Arabic digits
				if (aChar >= (wchar_t)0x6f0 && aChar <= (wchar_t)0x6f9)
				{
					aChar = aChar & (wchar_t)0x3f; // zero out the higher bits of this Arabic digit
				}
#endif /* __WXMAC__ */
				int isDigit = wxIsdigit(aChar);
				if (isDigit != 0)
				{
					// it's a digit, so we've reached the end of the delimiter
					break;
				}
				else
				{
					// it's not a digit, so it's part of the delimiter string
					rVerseAnal.strDelimiter += aChar;
					count++;
				}
			}
			if (count == numChars)
			{
				// it was "all delimiter" - a formatting error, as there is not verse
				// number to indicate the end of the range - so just take the starting
				// verse number (and any suffix) and return FALSE
				rVerseAnal.strEndingVerse = rVerseAnal.strStartingVerse;
				rVerseAnal.charEndingVerseSuffix = rVerseAnal.charStartingVerseSuffix;
				return rVerseAnal.bIsComplex;
			}
			else
			{
                // we stopped earlier than the end of the range string, and so presumably
                // we stopped at the first digit of the verse which ends the range
				range = range.Mid(count); // now just the final verse and possibly an a, b or c suffix
				numChars = range.Len();
				// get the final verse...
				count = 0;
				for (index = 0; index < numChars; index++)
				{
					aChar = range.GetChar(index);
#ifdef __WXMAC__
// Kludge because the atoi() function in the MacOS X standard library can't handle Arabic digits
					if (aChar >= (wchar_t)0x6f0 && aChar <= (wchar_t)0x6f9)
					{
						aChar = aChar & (wchar_t)0x3f; // zero out the higher bits of this Arabic digit
					}
#endif /* __WXMAC__ */
					int isDigit = wxIsdigit(aChar);
					if (isDigit != 0)
					{
						// it's a digit
						rVerseAnal.strEndingVerse += aChar;
						count++;
					}
					else
					{
						// it's not a digit, so exit with what we've collected so far
						break;
					}
				}
				if (count == numChars)
				{
                    // all there was in the range variable was the digits of the ending
                    // verse number, so set the return parameter values and return TRUE
					return rVerseAnal.bIsComplex;
				}
				else
				{
					// there's more, but get what we've got so far and trim that stuff 
					// off of range
					range = range.Mid(count);
					numChars = range.Len();
					// if a part-verse marker (assume one of a or b or c only), get it
					if (numChars > 0)
					{
						// what remains should just be a final a or b or c
						aChar = range.GetChar(0);
						if ( aChar == _T('a') || aChar == _T('b') || aChar == _T('c'))
						{
							rVerseAnal.charEndingVerseSuffix = aChar;
							range = range.Mid(1); // remove the suffix character
							numChars = range.Len(); // numChars should now be 0
						}
						if (numChars != 0)
						{
							// rhere's still something remaining, so just ignore it, but
							// alert the developer, not with a localizable string
							wxString suffix1;
							wxString suffix2;
							if (rVerseAnal.charStartingVerseSuffix != _T('\0'))
							{
								suffix1 = rVerseAnal.charStartingVerseSuffix;
							}
							if (rVerseAnal.charEndingVerseSuffix != _T('\0'))
							{
								suffix2 = rVerseAnal.charEndingVerseSuffix;
							}
							wxString msg;
							msg = msg.Format(
_T("The verse range was parsed, and the following remains unparsed: %s\nfrom the specification %s:%s%s%s%s%s"),
							range.c_str(),rVerseAnal.strStartingVerse.c_str(),suffix1.c_str(),
							rVerseAnal.strDelimiter.c_str(),rVerseAnal.strEndingVerse.c_str(),suffix2.c_str(),range.c_str());
							wxMessageBox(msg,_T("Verse range specification error (ignored)"),wxICON_WARNING);
						}
					} //end of TRUE block for test: if (numChars > 0)
				} // end of else block for test: if (count == numChars)
			} // end of else block for test: if (count == numChars)
		} // end of else block for test: if (count == numChars)
	} // end of else block for test: if (numChars == 0)
	return rVerseAnal.bIsComplex;
}

// overload for the one above (had to change param order, and make a ref param in order to
// get the compiler to recognise this as different - it was failing to see the lineIndex param
// when refVAnal was a ptr ref and was param 3 and lineIndex was param2)
bool DoVerseAnalysis(VerseAnalysis& refVAnal, const wxArrayString& md5Array, size_t lineIndex)
{
	size_t count = md5Array.GetCount();
	wxASSERT(count > 0); // it's not an empty array
	wxASSERT( lineIndex < count); // no bounds error
	wxString lineStr = md5Array.Item(lineIndex);
	// test we really do have a line beginning with a verse marker
	wxString mkr = GetStrictUsfmMarkerFromStructExtentString(lineStr);
	if (mkr != _T("\\v") || mkr != _T("\\vn"))
	{
		// don't expect the error, a message to developer will do
		wxString msg = _T("DoVerseAnalysis(), for the passed in lineIndex, the string returned does not start with a verse marker.\nFALSE will be returned - the logic will be wrong hereafter so exit without saving as soon as possible.");
		wxMessageBox(msg,_T(""),wxICON_ERROR);
		InitializeVerseAnalysis(refVAnal);
		return FALSE;

	}
	wxString verseNum = GetNumberFromChapterOrVerseStr(lineStr);
	bool bIsComplex = DoVerseAnalysis(verseNum, refVAnal);
	return bIsComplex;
}

void InitializeVerseAnalysis(VerseAnalysis& rVerseAnal)
{
	rVerseAnal.bIsComplex = FALSE; // assume it's a simple verse number (ie. no bridge, etc)
	rVerseAnal.strDelimiter = _T("-"); // it's usually a hyphen, but it doesn't matter what we put here
	rVerseAnal.strStartingVerse = _T("0"); // this is an invalid verse number, we can test
										 // for this to determine if the actual starting
										 // verse was not set
	rVerseAnal.strEndingVerse = _T("0");   // ditto 
	rVerseAnal.charStartingVerseSuffix = _T('\0'); // a null wxChar
	rVerseAnal.charEndingVerseSuffix = _T('\0'); // a null wxChar
}

// return TRUE if the versification at lineIndex's wxString from md5Arr is complex, that
// is, not just a simple verse number. FALSE if it is not complex.
bool IsComplexVersificationLine(const wxArrayString& md5Arr, size_t lineIndex)
{
	size_t count = md5Arr.GetCount();
	wxASSERT(lineIndex < count);
	VerseAnalysis vAnal;
	if (IsVerseLine(md5Arr, lineIndex))
	{
		return DoVerseAnalysis(vAnal, md5Arr, lineIndex);
	}
	return FALSE;
}

// Return the index in md5Arr at which there is an exact match of verseNum string in a
// verse line at nStart or beyond; else wxNOT_FOUND (-1) if no match was made. verseNum can
// be complex, such as 16b-18 or a part, like 7b, or whatever else scripture versification
// allows. We must match it exactly, not match a substring within the verseNum stored in
// the line being tested.
int FindExactVerseNum(const wxArrayString& md5Arr, int nStart, const wxString& verseNum)
{
	int count = md5Arr.GetCount();
	wxASSERT(nStart < count);
	int index;
	wxString lineStr;
	wxString strVsRange;
	for (index = nStart; index < count; index++)
	{
		if (IsVerseLine(md5Arr, index))
		{
			lineStr = md5Arr.Item(index);
			strVsRange = GetNumberFromChapterOrVerseStr(lineStr);
			if (strVsRange == verseNum)
			{
				return index;
			}
		}
	}
	return wxNOT_FOUND;
}

// Return the top end of the two matched complex chunks, as indices into postEditMd5Arr and
// fromEditorMd5Arr - namely, postEditEnd, and fromEditorEnd. postEditEnd, and
// fromEditorEnd will point at either the last line string in the two md5 arrays, or at the
// lines in those arrays which immediately precede lines with simple and matched to each
// other versification lines, or at lines which immediately precede complex and matched to
// each other versification lines in which the verseNum substring, although complex, is an
// exact match for the verseNum substring for the matching verse line of the other array.
void DelineateComplexChunksAssociation(const wxArrayString& postEditMd5Arr, 
				const wxArrayString& fromEditorMd5Arr, int postEditStart, int& postEditEnd, 
				int fromEditorStart, int& fromEditorEnd)
{
	int postEditArrCount = postEditMd5Arr.GetCount();
	int fromEditorArrCount = fromEditorMd5Arr.GetCount();
	wxASSERT(postEditStart < postEditArrCount);
	wxASSERT(fromEditorStart < fromEditorArrCount);

	// generate the verseArrays needed for making comparisons and searches for matching
	// verseNum strings to define where the end of the chunk is
	wxArrayPtrVoid postEditVerseArr;
	wxArrayPtrVoid fromEditorVerseArr;
	// populate the two arrays of verse information - stored in VerseInf structs storing 
	// (verseStr, index, bIsComplex flag) for each verse line -- generated from the passed
	// in postEditMd5Arr and fromEditorMd5Arr arrays
	// Note: material prior to chapter 1 is not verse-milestoned, but could contain
	// non-matchable fields between the two text versions being compare. If this is the
	// case, then we process this pre-chapter-one material here in an initial loop to
	// handle this situation as a special case, and return to the caller with the indices
	// appropriately advanced. On the next iteration in the caller, the indices passed in
	// for the starting indices will then point at chapter 1's md5 line in the arrays, and
	// then only the second loop (the main one) will be used from that point on.
	// However, if the fields in pre-chapter-one material are the same, we don't need to
	// do anything special because the per-field processing done in the main loop will
	// handle it there just fine
	GetRemainingMd5VerseLinesInChapter(postEditMd5Arr, postEditStart, postEditVerseArr);
	GetRemainingMd5VerseLinesInChapter(fromEditorMd5Arr, fromEditorStart, fromEditorVerseArr);

	// we need the following for both loops
    int postEditArr_Count = postEditVerseArr.GetCount(); // a smaller array than postEditMd5Arr
    int fromEditorArr_Count = fromEditorVerseArr.GetCount(); // a smaller array than fromEditorMd5Arr
	int postEditArr_Index = 0;
	int fromEditorArr_Index = 0;
	VerseInf* postEditVInf = NULL;
	VerseInf* fromEditorVInf = NULL;
	wxString postEditArr_VerseStr;
	wxString fromEditorArr_VerseStr;
	int postEditFwdsIndex = wxNOT_FOUND;
	int fromEditorFwdsIndex = wxNOT_FOUND;
	bool bSuccessful = FALSE;

    // Check if we are processing lines which precede the "\c 1: ..." line. If we are, then
    // one or both of the two VerseInf pointer arrays will have just one VerseInf struct, and the
    // struct's member values will have the string _T("0") as the verseNumStr, and
    // indexIntoArray value which points to the md5Array's \c 1 line, and TRUE as the
    // bIsComplex value. Deal with the various possibilities here in this special loop
    // before tackling the verse-milestoned material in the main loop further below.
	if (postEditVerseArr.GetCount() == 1 || fromEditorVerseArr.GetCount() == 1)
	{
		// at least one, or maybe both, of these arrays have fields which precede chapter
		// 1 and we need to deal with that material here
		bool postEditArrHasPre_Chapter_One_Material = FALSE;
		bool fromEditorArrHasPre_Chapter_One_Material = FALSE;
		wxString postEditVerseNumStr = _T("");
		wxString fromEditorVerseNumStr = _T("");
		bool postEdit_ItsComplexFlag = FALSE;
		bool fromEditor_ItsComplexFlag = FALSE;
		if (!postEditVerseArr.IsEmpty())
		{
			postEditVerseNumStr = ((VerseInf*)postEditVerseArr.Item(0))->verseNumStr;
			postEdit_ItsComplexFlag = ((VerseInf*)postEditVerseArr.Item(0))->bIsComplex;
		}
		if (!fromEditorVerseArr.IsEmpty())
		{
			fromEditorVerseNumStr = ((VerseInf*)fromEditorVerseArr.Item(0))->verseNumStr;
			fromEditor_ItsComplexFlag = ((VerseInf*)fromEditorVerseArr.Item(0))->bIsComplex;
		}
		if (postEditVerseNumStr == _T("0") && postEdit_ItsComplexFlag == TRUE)
		{
			// first test is the important one, if TRUE, then 2nd will always be TRUE; but
			// if the first is FALSE, that would mean there were no lines in the array
			// prior to the one which is the \c 1 line
			postEditArrHasPre_Chapter_One_Material = TRUE;
		}
		if (fromEditorVerseNumStr == _T("0") && fromEditor_ItsComplexFlag == TRUE)
		{
			// first test is the important one, if TRUE, then 2nd will always be TRUE; but
			// if the first is FALSE, that would mean there were no lines in the array
			// prior to the one which is the \c 1 line
			fromEditorArrHasPre_Chapter_One_Material = TRUE;
		}
		// What we do here depends on the flag values. We only get into this block in the
		// first place when there are mismatched md5 lines (i.e. not matching USFM markers
		// at matching indices) for indices in the material prior to the line which is the
		// chapter-one line; so like elsewhere, the from-editor material is to be retained
		// if the user didn't do any editing in Adapt It in the pre-chapter-one material,
		// but overwritten by the edited stuff if he did. Other complications are that one
		// of the text versions may have pre-chapter-one material and the other not. So
		// deal with all these possibilites: get the indices we process as far as,
		// and then use the two booleans above etc
		int postEdit_EndAt = postEditStart;
		int fromEditor_EndAt = fromEditorStart;
		if (postEditArrHasPre_Chapter_One_Material && fromEditorArrHasPre_Chapter_One_Material)
		{
			// both arrays have pre-chapter-one line lines, and they are not in sync
			postEdit_EndAt = ((VerseInf*)postEditVerseArr.Item(0))->indexIntoArray;
			fromEditor_EndAt = ((VerseInf*)fromEditorVerseArr.Item(0))->indexIntoArray;
		}
		else 
		{
			// one of the arrays has pre-chapter-one lines, the other has none (the
			// possibility that neither has any can't happen because we'd not have had
			// control enter this function for material prior to chapter one's line, if
			// that was the case)
			if (postEditArrHasPre_Chapter_One_Material)
			{
				// the postEditArr has the pre-chapter-one material
				postEdit_EndAt = ((VerseInf*)postEditVerseArr.Item(0))->indexIntoArray;
			}
			else if (fromEditorArrHasPre_Chapter_One_Material)
			{
				// the fromEditorArr is the one with the pre-chapter-one material
				fromEditor_EndAt = ((VerseInf*)fromEditorVerseArr.Item(0))->indexIntoArray;
			}
		}
		// processing not-in-sync USFM and md5 sum lines which lie preceding chapter 1's
		// line is finished, return for the caller to advance the line indices to process
		// what follows from postEditEnd and fromEditorEnd, in their respective arrays
		// because these indices now point at chapter 1 line for each array
		postEditEnd = postEdit_EndAt;
		fromEditorEnd = fromEditor_EndAt;
		return;
	} // end of block for test:
	  // if (postEditVerseArr.GetCount() == 1 || fromEditorVerseArr.GetCount() == 1)

	// This block is where the rubber hits the road -- we are processing in this loop
	// within a chapter, and any pre-chapter material which is complex has already been handled
	// above on the preceding iteration in the caller. Now we deal with milestoned stuff.
	
    // We start at the beginning of the two verseline inventories, finding a verseStr in
    // one array, and searching from the start of the other array for a match (identical
    // bridges like 17-19a & 17-19a, identical part verses like 6b & 6b etc, not just
    // identical simple verses like 7 & 7, are all valid potential matches - we don't
    // distinguish, so long as we have a matchup we have come to where the "mismatched
    // chunk" ends)
	while (postEditArr_Index < postEditArr_Count && fromEditorArr_Index < fromEditorArr_Count)
	{
		postEditArr_VerseStr   = ((VerseInf*)postEditVerseArr.Item(postEditArr_Index))->verseNumStr;
		fromEditorArr_VerseStr = ((VerseInf*)fromEditorVerseArr.Item(fromEditorArr_Index))->verseNumStr;
		if (postEditArr_VerseStr == fromEditorArr_VerseStr)
		{
			// successful matchup
			postEditVInf = (VerseInf*)postEditVerseArr.Item(postEditArr_Index);
			fromEditorVInf = (VerseInf*)fromEditorVerseArr.Item(fromEditorArr_Index);
			bSuccessful = TRUE;
			break;
		}
		else
		{
			// look for a matchup of postEditArr_VerseStr, in the other array at a forwards
			// location within fromEditorVerseArr
			fromEditorFwdsIndex = 
				FindMatchingVerseNumInOtherChapterArray(fromEditorVerseArr, postEditArr_VerseStr);
			if (fromEditorFwdsIndex != wxNOT_FOUND)
			{
				// successful matchup
				postEditVInf = (VerseInf*)postEditVerseArr.Item(postEditArr_Index);
				fromEditorVInf = (VerseInf*)fromEditorVerseArr.Item(fromEditorFwdsIndex);
				bSuccessful = TRUE;
				break;
			}
			else
			{
				// that failed, so try searching for fromEditorArr_VerseStr matchup
				// somewhere forwards in postEditVerseArr
				postEditFwdsIndex = 
					FindMatchingVerseNumInOtherChapterArray(postEditVerseArr, fromEditorArr_VerseStr);
				if (postEditFwdsIndex != wxNOT_FOUND)
				{
					// successful matchup
					postEditVInf = (VerseInf*)postEditVerseArr.Item(postEditFwdsIndex);
					fromEditorVInf = (VerseInf*)fromEditorVerseArr.Item(fromEditorArr_Index);
					bSuccessful = TRUE;
					break;
				}
				else
				{
					// all possibilities for matching either of postEditArr_VerseStr or
					// fromEditorArr_VerseStr have been exhausted, so advance the iterators and
					// loop
					postEditArr_Index++;
					fromEditorArr_Index++;
				}
			}
		} // end of else block for test: if (postEditArr_VerseStr == fromEditorArr_VerseStr)
	} // end of loop: while (postEditArr_Index < postEditArr_Count && fromEditorArr_Index < fromEditorArr_Count)
	if (bSuccessful)
	{
		postEditEnd = postEditVInf->indexIntoArray; // the index into the verseline within postEditMd5Arr
		fromEditorEnd = fromEditorVInf->indexIntoArray; // the index into the verseline within fromEditorMd5Arr
	}
	else
	{
		// if we didn't make any match, then the mismatched chunk is one or more USFM
		// fields, each set of which is at the end of the passed in pair of arrays, and
		// extends to the ends of those arrays; so return the array counts to indicate
		// this
		postEditEnd = postEditArr_Count;
		fromEditorEnd = fromEditorArr_Count;
	}
	DeleteAllVerseInfStructs(postEditVerseArr); // don't leak memory
	DeleteAllVerseInfStructs(fromEditorVerseArr); // ditto
}

// Search for a match of verseNum in the passed in verseInfArr, searching the whole array
// from start for the first match. Return the index if found, wxNOT_FOUND if no matchup is made
int	FindMatchingVerseNumInOtherChapterArray(const wxArrayPtrVoid& verseInfArr, wxString& verseNum)
{
	int count = verseInfArr.GetCount();
	int index;
	wxString aVerseNum;
	VerseInf* viPtr = NULL;
	for (index = 0; index < count; index++)
	{
		viPtr = (VerseInf*)verseInfArr.Item(index);
		aVerseNum = viPtr->verseNumStr;
		// must be an exact match
		if (aVerseNum == verseNum)
		{
			// matched, it could be two simple verses, or two identical bridges, etc
			return index;
		}
	}
	return wxNOT_FOUND;
}

// When processing comes, when comparing lines in two md5Array instances (one for
// postEditText the other for fromEditorText) to a line pair which don't match in the
// initial USFM markers, we have to delineate (from that point onwards) how many markers
// from each array belong to this "mismatch chunk". To do this we have to look for the
// next match of either two verse lines which have the same (simple) verse numbers, or the
// same complex verse numbers -- the important point is they be the same, as that defines
// where the "mismatch chunk" ends (at the md5Arr lines which immediately precede these
// two matched verse lines). We match verse lines simply because other markers have no
// milestone information that can tell us if they are equivalent or not. This function
// helps us out by gathering all the verse lines from the start of the mismatch, up to the
// end of the chapter (but not including a line belonging to the next chapter); and
// putting the information relevant to determining where the mismatch chunk ends into a
// struct, VerseInf, which has 3 members, one points back to the line's index in the 
// original md5Array, another is the verseNumStr (which may be simple or complex, the
// latter being things like 15b-17, whereas a simple one would be something like 10), and
// the third is a boolean which is TRUE if the verseNumSt is complex, FALSE if not. The
// caller will then use this information in its algorithm for working out where the
// mismatch chunk ends - after using two calls of this function, one for each of the
// original md5 arrays being compared. 
void GetRemainingMd5VerseLinesInChapter(const wxArrayString& md5Arr, int nStart, 
										wxArrayPtrVoid& verseLinesArr)
{
	int count = md5Arr.GetCount();
	// set nBoundingIndex to the index value immediately beyond the last index of the
	// current chapter (at the end of the array, this would be equal to the array count)
	bool bBeforeChapterOne = FALSE;
	int nBoundingIndex = FindNextChapterLine(md5Arr, nStart, bBeforeChapterOne);
	if (nBoundingIndex == wxNOT_FOUND)
	{
		nBoundingIndex = count;
	}
	wxString lineStr;
	verseLinesArr.Clear();
	if (nStart == nBoundingIndex)
	{
		return;
	}
	VerseAnalysis va;
	VerseInf* pVInf = NULL;
	int index = nStart;

	// What we do depends on whether the nStart index is within pre-chapter-one material,
	// or somewhere after that. In the latter case, we want to use collect verse lines,
	// but in the former situation we only want to collect non-verse lines up to and
	// including the md5 line immediately prior to chapter 1
	if (bBeforeChapterOne)
	{
		// Just pass back a single VInf struct in the verseLinesArr with special values as
		// follows: 
		// for bIsComplex, TRUE (we want to chunk all the pre-chapter-one material)
		// for verseNumStr, the special diagnostic value of "0" so caller can know how 
		//     to interpret the next member
		// for indexIntoArray, the nBoundingIndex value determined above (that is, the
		//     index to the line which is the md5 line for the \c 1 field)
		pVInf = new VerseInf;
		pVInf->bIsComplex = TRUE;
		pVInf->indexIntoArray = nBoundingIndex;
		pVInf->verseNumStr = _T("0");
		verseLinesArr.Add(pVInf);
		return;
	}
	else
	{
		bool bIsVerseLine = IsVerseLine(md5Arr, index);
		if (bIsVerseLine)
		{
			lineStr = md5Arr.Item(index);
			pVInf = new VerseInf;
			pVInf->indexIntoArray = nStart;
			pVInf->verseNumStr = GetNumberFromChapterOrVerseStr(lineStr);
			pVInf->bIsComplex = DoVerseAnalysis(pVInf->verseNumStr, va);
			verseLinesArr.Add(pVInf);
		}
			bool bGotAnother = FALSE;
			do {
				// next call starts looking at line after index's line, and if finds a next
				// verse line, then it returns its index in the index param
				bGotAnother = GetAnotherVerseLine(md5Arr, index); 
				if (bGotAnother && index < nBoundingIndex)
				{
					// it's within the current chapter
					lineStr = md5Arr.Item(index);
					pVInf = new VerseInf;
					pVInf->indexIntoArray = index;
					pVInf->verseNumStr = GetNumberFromChapterOrVerseStr(lineStr);
					pVInf->bIsComplex = DoVerseAnalysis(pVInf->verseNumStr, va);
					verseLinesArr.Add(pVInf);
				}
				else
					break;
			} while (TRUE);
	}
}

// frees from the heap the passed in array's VerseInf structs
void DeleteAllVerseInfStructs(wxArrayPtrVoid& arr)
{
	int count = arr.GetCount();
	int i;
	for (i=0; i<count; i++)
	{
		VerseInf* pVI = (VerseInf*)arr.Item(i);
		delete pVI;
	}
}

// Starting at nStartAt, look for the next line of form  "\\c nn:mm:md5sum" and return its
// index in md5Arr, or wxNOT_FOUND if there is no later chapter line
// The param bBeforeChapterOne is default FALSE; it the nStart index value is a field
// which lies before the \c 1 field, then bBeforeChapterOne will be returned TRUE,
// otherwise it is returned FALSE (for chapters 1 and beyond we always process right up to
// the next chapter's \c line, or buffer end, so there won't be a "before" set of fields
// prior to subsequent chapter lines than for \c 1)
int  FindNextChapterLine(const wxArrayString& md5Arr, int nStartAt, bool& bBeforeChapterOne)
{
	int count = md5Arr.GetCount();
	int i;
	wxString chapterMkrPlusSpace = _T("\\c ");
	wxString lineStr;
	for (i=nStartAt; i<count; i++)
	{
		lineStr = md5Arr.Item(i);
		if (lineStr.Find(chapterMkrPlusSpace) != wxNOT_FOUND)
		{
			if (lineStr.Find(_T(" 1:")) != wxNOT_FOUND)
			{
				// we are at a \c 1 line -- it could have things like \id \h \mt1 \mt2 and
				// maybe more before it, so we will handle that as a pseudo-chapter in its
				// own right - so tell the caller that nStartAt started out with a line
				// index prior to that for the \c 1 line
				bBeforeChapterOne = TRUE;
			}
			else
			{
				bBeforeChapterOne = FALSE;
			}
			return i;
		}
	}
	bBeforeChapterOne = FALSE; // if we can't find one, caller will want everything, so this
							   // is the appropiate return for this boolean in this situation
	return wxNOT_FOUND;
}


// this function exports the target text from the document, and programmatically causes
// \free \note and \bt (and any \bt-initial markers) and the contents of those markers to
// be excluded from the export
wxString ExportTargetText_For_Collab(SPList* pDocList)
{
	wxASSERT(pDocList != NULL);
	int textLen = 0;
	wxString text;
	textLen = RebuildTargetText(text, pDocList); // from ExportFunctions.cpp
	// set \note, \bt, and \free as to be programmatically excluded from the export
	ExcludeCustomMarkersFromExport(); // defined in ExportFunctions.cpp
	// cause the markers set for exclusion, plus their contents, to be actually removed
	// from the exported text
	bool bRTFOutput = FALSE; // we are working with USFM marked up text
	text = ApplyOutputFilterToText(text, m_exportBareMarkers, m_exportFilterFlags, bRTFOutput);
	// in next call, param 2 is from enum ExportType in Adapt_It.h
	FormatMarkerBufferForOutput(text, targetTextExport);
	text = RemoveMultipleSpaces(text);
	return text;
}

// this function exports the free translation text from the document; any
// un-free-translated sections are output as just empty USFM markers
wxString ExportFreeTransText_For_Collab(SPList* pDocList)
{
	wxASSERT(pDocList != NULL);
	int textLen = 0;
	wxString text;
	textLen = RebuildFreeTransText(text, pDocList); // from ExportFunctions.cpp
	// in next call, param 2 is from enum ExportType in Adapt_It.h
	FormatMarkerBufferForOutput(text, freeTransTextExport);
	text = RemoveMultipleSpaces(text);
	return text;
}


////////////////////////////////////////////////////////////////////////////////////////
/// \return                 The updated target text, or free translation text, (depending)
///                         on the makeTextType value passed in, which is ready for
///                         transferring back to the external editor using ::wxExecute();
///                         or an empty string if adaptation is wanted but the doc is
///                         empty, or free translation is wanted but none are available
///                         from the document.
/// \param pDocList     ->  ptr to the list of CSourcePhrase ptrs which comprise the document,
///                         or some similar list (the function does not rely in any way on
///                         this list being the m_pSourcePhrases list from the app class, but
///                         normally it will be that list)
/// \param              ->  Either makeTargetText (value is 1) or makeFreeTransText (value is
///                         2); our algorithms work almost the same for either kind of text.
/// \param postEditText <-  the text to replace the pre-edit target text or free translation
/// \remarks
/// 
/// Comments below are a bit out of date -- there is only a 3-text situation, shouldn't be
/// two unless the from-external-editor text is an empty string
/// 
/// The document contains, of course, the adaptation and, if the user has done any free
/// translation, also the latter. Sending either back to PT or BE after editing work in
/// either or both is problematic, because the user may have done some work in PT or BE
/// on either the adaptation or the free translation as stored in the external editor,
/// and so we don't want changes made there wiped out unnecessarily. So we have to
/// potentially do a lot of checks so as to send to the external editor only the minimum
/// necessary. So we must store the adaptation and free translation in AI's native storage
/// after each save, so we can compare subsequent work with that "preEdit" version of the
/// adaptation and / or free translation; and each time we re-setup the same document in
/// AI, and at the commencement of each File / Save, we must grab from PT or BE the
/// adaptation and free translation (if the latter is expected) as they currently are, just
/// in case the user did editing of both or either in the external editor before switching
/// to AI to work on the same doc there. So we store the text as at last Save, this is the
/// "preEdit" version we compare with on next Save, and we store the just-grabbed PT or BE
/// version of the text (in case the user did edits in the external editor beforehand
/// which AI would not otherwise know about) - and we do our insertions of the AI edits
/// into this just-grabbed text, otherwise, the older text from PT or BE wouldn't have
/// those edits done externally and what we send back would overwrite the user's edits
/// done in PT or BE before switching back to AI for the editing work done for the
/// currently-being-done Save. So there are potentially up to 3 versions of the same
/// chapter or book that we have to juggle, and minimize unwanted changes within the final
/// version being transferred back to the external editor on this current Save.
/// 
/// For comparisons, the idea is that the user will have done some editing of the document,
/// and we want to find out by doing verse-based chunking and then various comparisons
/// using MD5 checksums, to work out which parts of the document have changed in either (1)
/// wording, or, (2) in punctuation only. Testing for this means comparing the pre-edit
/// state of the text (as from param 1) with the post-edit state of the text (as generated
/// from param 2 -- but actually, it's more likely to be "at this File / Save" operation,
/// rather than post-edit, because the user may do several File / Save operations before
/// he's finished editing the document to it's final state). Only the parts which have
/// changed are then candidates for producing changes to the text in pLatestTextFromEditor
/// - if the user didn't work on chapter 5 verse 4 when doing his changes, then
/// pLatestTextFromEditor's version of chapter 5 verse 4 is returned to PT or BE unchanged,
/// whether or not it differs from what the document's version of chapter 5 verse 4 happens
/// to be. (In this way, we don't destroy any edits made in PT or BE prior to the document
/// being edited a second or third etc time in Adapt It, with edits being done in the
/// external editor between-times.) The enum parameter allows us to choose which kind of
/// text to update from the user's editing in Adapt It.
/// 
/// Note 1: If there are word-changes when the function checks the pre-edit and post-edit
/// state of the document's verse-based chunks, these chunks have their edited text
/// inserted into the appropriate places in pLatestTextFromEditor, overwriting what was in
/// the latter at the relevant places. But in the case of the user only making punctuation
/// changes to one or more chunks, then the Adapt It version of the text for such a chunk
/// only replaces what is in the relevant part of pLatestTextFromEditor provided that the
/// latter's words in the chunk don't differ from the Adapt It document's words for the
/// chunk. If the words do differ, then we don't risk replacing edits done externally with
/// text from the AI document which may be inferior at the place where the punctuation was
/// edited, and we just refrain from honouring such punctuation-only edits in that case
/// (that is, the relevant part of pLatestTextFromEditor is NOT updated in that case. The
/// user can do the relevant punctuation changes in the external editor at a later time,
/// such as when doing a pre-publication check of the text.)
/// 
/// Note 2: in case you are wondering why we do things this way... the problem is we can't
/// merge the user's edits of target text and/or free translation text back into the Adapt
/// It document which produced the earlier versions of such text. Such a merger would be
/// complicated, and need to be interactive and require a smart GUI widget to make it
/// doable in an easy to manage way - and we are a long way from figuring that out and
/// implementing it (though BEW does have ideas about how to go about it). Without being
/// able to do such mergers, the best we can do is what this function currently does. We
/// attribute higher value to something which runs without needing the user to do anything
/// with a GUI widget, than something smart but which requires tedious and repetitive
/// decision-making in a GUI widget.
/// 
/// This function is complex. It uses (a) verse-based chunking code as used previously in
/// the Import Edited Source Text feature, (b) various wxArrayPtrVoid and wxArrayString to
/// store constructed space-delimited word strings, and coalesced punctuation strings,
/// (c) MD5 checksums of the last two kinds of things, stored in wxArrayString, tokenizing
/// of the passed in strings - and in the case of the first and third, the target text
/// punctuation must be used for the tokenizing, because m_srcPhrase and m_key members
/// will be holding target text, not source text; and various structs, comparisons and
/// mapping of chunks to starting and ending offsets into pLatestTextFromEditor, and code
/// for doing such things... you get the picture. However, the solution is generic and
/// self-contained, and so can potentially be used elsewhere.
/// BEW added 11July, to get changes to the adaptation and free translation ready, as a
/// wxString, for transferring back to the respective PT or BE projects - which would be
/// done in the caller once the returned string is assigned there.
/// Note: internally we do our processing using SPArray rather than SPList, as it is
/// easier and we can use functions from MergeUpdatedSrc.h & .cpp unchanged.
///  
/// Note 3, 16Jul11, culled from my from email to Bill:
/// Two new bits of the puzzle were 
/// (a) get the two or three text variants which need to be compared to USFM text format,
/// then I can tokenize each with the tokenizing function that uses target text
/// punctuation, to have the data from each text in just m_srcPhrase and m_key to work with
/// - that way simplifies things a lot and ensures consistency in the parsing; and
/// (b) on top of the existing versed based chunking, I need to do a higher level chunking
/// based on the verse-based chunks, the higher level chunks are "equivalence chunks" -
/// which boils down to verse based chunks if there are no part verses or bridged verses,
/// but when there are the latter, the complexities of disparate chunking at the lower
/// level for a certain point in the texts can be subsumed into a single equivalence chunk.
/// Equivalence chunking needs to be done in 2-way associations, and also in 3-way
/// associations -- the latter when there is a previously sent-to-PT variant of the text
/// retained in AI from a previous File / Save, plus the edited document as current it is
/// at the current File / Save, plus the (possibly independently edited beforehand in PT)
/// just-grabbed from PT text. Once I have the equivalence associations set up, I can step
/// through them, comparing MD5 checksums, and determining where the user made edits, and
/// then getting the new AI edits into just the corresponding PT text's chunks (the third
/// text mentioned above). The two-way equivalences are when there is no previous File /
/// Save done, so all there is is the current doc in AI and the just-grabbed from PT
/// (possibly edited) text. It's all a bit complex, but once the algorithm is clear in its
/// parts, which it now is, it should just be a week or two's work to have it working. Of
/// course, the simplest situation is when there has been no previous File / Save for the
/// data, and PT doesn't have any corresponding data to grab yet -- that boils down to just
/// a simple export of the adaptation or free translation, as the case may be, using the
/// existing RebuildTargetText() or RebuildFreeTransText() functions, as the case may be,
/// and then transferring the resulting USFM text to the relevant PT project using
/// rdwrtp7.exe.
/// 
/// Note 4: since the one document SPList contains both adaptation text and associated free
/// translation text (if any), returning adaptation text to PT or BE, and returning free
/// translation text to PT or BE, will require two calls to this function. Both, however,
/// are done at the one File / Save -- first the adaptation is sent, and then if free
/// translation is expected to be be sent, it is sent after the adaptation is sent,
/// automatically.
///////////////////////////////////////////////////////////////////////////////////////
wxString MakeUpdatedTextForExternalEditor(SPList* pDocList, enum SendBackTextType makeTextType, 
										   wxString& postEditText)
{
	wxString emptyStr = _T("");
	//CAdapt_ItView* pView = gpApp->GetView();
	//CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	
	wxString text; text.Empty();
	wxString preEditText; // the adaptation or free translation text prior to the editing session
	wxString fromEditorText; // the adaptation or free translation text just grabbed from PT or BE
	fromEditorText.Empty();
	postEditText.Empty(); // the exported adaptation or free translation text at File / Save time

	/* app member variables
	bool m_bCollaboratingWithParatext;
	bool m_bCollaboratingWithBibledit;
	bool m_bCollaborationExpectsFreeTrans;
	bool m_bCollaborationDocHasFreeTrans;
	wxString m_collaborationEditor;
	*/
    // If collaborating with Paratext, check if Paratext is running, if it is, warn user to
    // close it now and then try again; it not running, the data transfer can take place
    // safely (i.e. it won't cause VCS conflicts which otherwise would happen when the user
    // in Paratext next did a Save there)
	if (gpApp->m_bCollaboratingWithParatext)
	{
		if (gpApp->ParatextIsRunning())
		{
			// don't let the function proceed further, give message, and return empty string
			wxString msg;
			msg = msg.Format(_T("Adapt It has detected that %s is still running.\nTransferring your work to %s while it is running would probably cause data conflicts later on; so it is not allowed.\nLeave Adapt It running, switch to Paratext and shut it down, saving any unsaved work.\nThen switch back to Adapt It and your next File / Save attempt will succeed."),
				gpApp->m_collaborationEditor.c_str(), gpApp->m_collaborationEditor.c_str());
			wxMessageBox(msg, _T(""), wxICON_WARNING);
			return text; // text is currently an empty string
		}
	}
    // If collaborating with Bibledit, check if Bibledit is running, if it is, warn user to
    // close it now and then try again; it not running, the data transfer can take place
    // safely (i.e. it won't cause VCS conflicts which otherwise would happen when the user
    // in Bibledit next did a Save there)
	if (gpApp->m_bCollaboratingWithBibledit)
	{
		if (gpApp->BibleditIsRunning())
		{
			// don't let the function proceed further, give message, and return empty string
			wxString msg;
			msg = msg.Format(_T("Adapt It has detected that %s is still running.\nTransferring your work to %s while it is running would probably cause data conflicts later on; so it is not allowed.\nLeave Adapt It running, switch to Paratext and shut it down, saving any unsaved work.\nThen switch back to Adapt It and your next File / Save attempt will succeed."),
				gpApp->m_collaborationEditor.c_str(), gpApp->m_collaborationEditor.c_str());
			wxMessageBox(msg, _T(""), wxICON_WARNING);
			return text; // text is currently an empty string
		}
	}

	// get the pre-edit saved text, and the from-PT or from-BE version of the same text
	wxArrayString textIOArray; textIOArray.Clear(); // needed for the internal ::wxExecute() call
	wxArrayString errorsIOArray; errorsIOArray.Clear(); // ditto
	long resultCode = -1; // ditto
	// if there was an error, a message will have been seen already
	switch (makeTextType)
	{
	case makeFreeTransText:
		{
			preEditText = gpApp->GetStoredFreeTransText_PreEdit();
			// next call gets the file of data into the .temp folder, with appropriate filename
			TransferTextBetweenAdaptItAndExternalEditor(reading, collab_freeTrans_text,
							textIOArray, errorsIOArray, resultCode);
			if (resultCode > 0)
			{
				// we don't expect this to fail, so a beep (plus the error message
				// generated internally) should be enough, and just don't send anything to
				// the external editor
				wxBell();
				return emptyStr;
			}
			// remove the BOM and get the data into wxString textFromEditor
			wxString absPath = MakePathToFileInTempFolder_For_Collab(collab_freeTrans_text);
			fromEditorText = GetTextFromAbsolutePathAndRemoveBOM(absPath);
		}
		break;
	default:
	case makeTargetText:
		{
			preEditText = gpApp->GetStoredTargetText_PreEdit();
			// next call gets the file of data into the .temp folder, with appropriate filename
			TransferTextBetweenAdaptItAndExternalEditor(reading, collab_target_text,
							textIOArray, errorsIOArray, resultCode);
			if (resultCode > 0)
			{
				// we don't expect this to fail, so a beep (plus the error message
				// generated internally) should be enough, and just don't send anything to
				// the external editor
				wxBell();
				return emptyStr;
			}
			// remove the BOM and get the data into wxString textFromEditor
			wxString absPath = MakePathToFileInTempFolder_For_Collab(collab_target_text);
			fromEditorText = GetTextFromAbsolutePathAndRemoveBOM(absPath);
		}
		break;
	};

	// if the document has no content, just return an empty wxString to the caller
	if (pDocList->IsEmpty())
	{
		return text;
	}
	// pDocList is not an empty list of CSourcePhrase instances, so build as much of the
	// wanted data type as is done so far in the document & return it to caller
	if (fromEditorText.IsEmpty())
	{
		// if no text was received from the external editor, then the document is being,
		// or has just been, adapted for the first time - this simplifies our task to
		// become just a simple export of the appropriate type...
		switch (makeTextType)
		{
		case makeFreeTransText:
			// rebuild the free translation USFM marked-up text
			text = ExportFreeTransText_For_Collab(pDocList);
			break;
		default:
		case makeTargetText:
			// rebuild the adaptation USFM marked-up text
			text = ExportTargetText_For_Collab(pDocList);
			break;
		}
		return text;
	}

	// if control gets here, then we've 3 text variants to manage & compare; pre-edit,
	// post-edit, and fromEditor; we've not yet got the post-edit text from the document,
	// so do it now
	switch (makeTextType)
	{
	case makeFreeTransText:
		postEditText = ExportFreeTransText_For_Collab(pDocList);
			break;
	default:
	case makeTargetText:
		postEditText = ExportTargetText_For_Collab(pDocList);
		break;
	}

	// abandon any wxChars which precede first marker in text, for each of the 3 texts, so
	// that we make sure each text we compare starts with a marker
	int offset = postEditText.Find(_T('\\'));
	if (offset != wxNOT_FOUND && offset > 0)
	{
		postEditText = postEditText.Mid(offset); // guarantees postEditText starts with a marker
	}
	offset = preEditText.Find(_T('\\'));
	if (offset != wxNOT_FOUND && offset > 0)
	{
		preEditText = preEditText.Mid(offset); // guarantees preEditText starts with a marker
	}
	offset = fromEditorText.Find(_T('\\'));
	if (offset != wxNOT_FOUND && offset > 0)
	{
		fromEditorText = fromEditorText.Mid(offset); // guarantees fromEditorText starts with a marker
	}

	// If the user changes the USFM structure for the text within Paratext or Bibledit,
	// such as to bridge or unbridge verses, and/or make part verses, and/or add new
	// markers such as poetry markers, etc - such changes, even if no words or punctuation
	// are altered, change the data which follows the marker and that will lead to MD5
	// changes based on the structure changes. If the USFM in the text coming from the
	// external editor is different, we must use more complex algorithms - chunking
	// strategies and wrapping the corresponding sections of differing USFM structure text
	// parts in a bigger chunk and doing, within that bigger chunk, more text transfer
	// (and possibly rubbing out better text in the external editor as a result). The
	// situation where USFM hasn't changed is much better - in that case, every marker is
	// the same in the same part of every one of the 3 texts: the pre-edit text, the
	// post-edit text, and the grabbed-text (ie. just grabbed from the external editor).
	// This fact allows for a much simpler processing algorithm. We'll tackle this case
	// first. Fortunately, it is the overwhelmingly most common situation. To determine
	// whether or not the USFM structure has changed we must now calculate the
	// UsfmStructure&Extents arrays for each of the 3 text variants, then test if the USFM
	// in the post-edit text is any different from the USFM in the grabbed text. (The
	// pre-edit text is ALWAYS the same in USFM structure as the post-edit text, because
	// editing of the source text is not allowed when in collaboration mode.
	wxArrayString preEditMd5Arr = GetUsfmStructureAndExtent(preEditText);
	wxArrayString postEditMd5Arr = GetUsfmStructureAndExtent(postEditText);;
	wxArrayString fromEditorMd5Arr = GetUsfmStructureAndExtent(fromEditorText);

    // Now use MD5Map stucts to map the individual lines of each UsfmStructureAndExtent
    // array to the associated subspans within text variant which gave rise to the
    // UsfmStructureAndExtent array. In this way, for a given index value into the
    // UsfmStructureAndExtent array, we can quickly get the offsets for start and end of
    // the substring in the text which is associated with that md5 line.
	size_t count = preEditMd5Arr.GetCount();
	wxArrayPtrVoid preEditOffsetsArr;
	preEditOffsetsArr.Alloc(count); // pre-allocate sufficient space
	MapMd5ArrayToItsText(preEditText, preEditOffsetsArr, preEditMd5Arr);

	count = postEditMd5Arr.GetCount();
	wxArrayPtrVoid postEditOffsetsArr;
	postEditOffsetsArr.Alloc(count); // pre-allocate sufficient space
	MapMd5ArrayToItsText(postEditText, postEditOffsetsArr, postEditMd5Arr);

	count = fromEditorMd5Arr.GetCount();
	wxArrayPtrVoid fromEditorOffsetsArr;
	fromEditorOffsetsArr.Alloc(count); // pre-allocate sufficient space
	MapMd5ArrayToItsText(fromEditorText, fromEditorOffsetsArr, fromEditorMd5Arr);



	bool bUsfmIsTheSame = TRUE;
	if (IsUsfmStructureChanged(postEditText, fromEditorText))
	{
		bUsfmIsTheSame = FALSE;
	}
	if (bUsfmIsTheSame)
	{
		text = GetUpdatedText_UsfmsUnchanged(postEditText, fromEditorText,
					preEditMd5Arr, postEditMd5Arr, fromEditorMd5Arr,
					postEditOffsetsArr, fromEditorOffsetsArr);
	}
	else
	{
#ifdef __WXDEBUG__
	//int ct;
	//for (ct = 0; ct < (int)UsfmStructureAndExtentArray.GetCount(); ct++)
	//{
	//	wxLogDebug(UsfmStructureAndExtentArray.Item(ct));
	//}
	int count1, count2;
	count1 = (int)postEditMd5Arr.GetCount();
	count2 = (int)fromEditorMd5Arr.GetCount();
	int ctmin = wxMin(count1,count2);
	int ctmax = wxMax(count1,count2);
	if (ctmin == ctmax)
	{
		wxLogDebug(_T("***SAME LENGTH ARRAYS: numItems = %d"), ctmin);
	}else
	{
		wxLogDebug(_T("***DIFFERENT LENGTH ARRAYS: postEditArr numItems = %d  fromEditorNumItems = %d"), count1, count2);
	}
	int ct;
	for (ct = 0; ct < ctmin; ct++)
	{
		wxLogDebug(_T("Same indices part:   %s          %s"),postEditMd5Arr.Item(ct).c_str(), fromEditorMd5Arr.Item(ct).c_str());
	}
#endif
		// the USFM structure has changed in at least one location in the text
		text = GetUpdatedText_UsfmsChanged(preEditText, postEditText, fromEditorText,
					preEditMd5Arr, postEditMd5Arr, fromEditorMd5Arr,
					postEditOffsetsArr, fromEditorOffsetsArr);
	}

	// delete from the heap all the MD5Map structs we created
	DeleteMD5MapStructs(preEditOffsetsArr);
	DeleteMD5MapStructs(postEditOffsetsArr);
	DeleteMD5MapStructs(fromEditorOffsetsArr);

	return text;
}

void DeleteMD5MapStructs(wxArrayPtrVoid& structsArr)
{
	int count = structsArr.GetCount();
	int index;
	MD5Map* pStruct = NULL;
	for (index = 0; index < count; index++)
	{	
		pStruct = (MD5Map*)structsArr.Item(index);
		delete pStruct;
	}
	structsArr.Clear();
}

// GetUpdatedText_UsfmsUnchanged() is the simple-case (i.e. USFM markers have not been
// changed, added or deleted) function which generates the USFM marked up text, either
// target text, or free translation text, which the caller will pass back to the relevant
// Paratext or Bibledit project, it's book or chapter as the case may be.
// 
// Note, we don't need to pass in a preEditOffsetsArr, nor preEditText, because we don't
// need to map with indices into the preEditText itself; instead, we use the passed in MD5
// checksums for preEditText to tell us all we need about that text in relation to the
// same marker and content in postEditText.
wxString GetUpdatedText_UsfmsUnchanged(wxString& postEditText, wxString& fromEditorText,
			wxArrayString& preEditMd5Arr, wxArrayString& postEditMd5Arr, wxArrayString& fromEditorMd5Arr,
			wxArrayPtrVoid& postEditOffsetsArr, wxArrayPtrVoid& fromEditorOffsetsArr)
{
	wxString newText; newText.Empty();
	wxString zeroStr = _T("0");
    // each text variant's MD5 structure&extents array has exactly the same USFMs in the
    // same order -- verify before beginning
	size_t preEditMd5Arr_Count = preEditMd5Arr.GetCount();					
	size_t postEditMd5Arr_Count = postEditMd5Arr.GetCount();					
	size_t fromEditorMd5Arr_Count = fromEditorMd5Arr.GetCount();					
	wxASSERT( preEditMd5Arr_Count == postEditMd5Arr_Count && 
			  preEditMd5Arr_Count == fromEditorMd5Arr_Count);
	MD5Map* pPostEditOffsets = NULL; // stores ptr of MD5Map from postEditOffsetsArr
	MD5Map* pFromEditorOffsets = NULL; // stores ptr of MD5Map from fromEditorOffsetsArr
	wxString preEditMd5Line;
	wxString postEditMd5Line;
	wxString fromEditorMd5Line;
	wxString preEditMD5Sum;
	wxString postEditMD5Sum;
	wxString fromEditorMD5Sum;

	// work with wxChar pointers for each of the postEditText and fromEditorText texts (we
	// don't need a block like this for the preEditText because we don't access it's data
	// directly in this function)
	const wxChar* pPostEditBuffer = postEditText.GetData();
	int nPostEditBufLen = postEditText.Len();
	wxChar* pPostEditStart = (wxChar*)pPostEditBuffer;
	wxChar* pPostEditEnd = pPostEditStart + nPostEditBufLen;
	wxASSERT(*pPostEditEnd == '\0');

	const wxChar* pFromEditorBuffer = fromEditorText.GetData();
	int nFromEditorBufLen = fromEditorText.Len();
	wxChar* pFromEditorStart = (wxChar*)pFromEditorBuffer;
	wxChar* pFromEditorEnd = pFromEditorStart + nFromEditorBufLen;
	wxASSERT(*pFromEditorEnd == '\0');

	size_t index;
	for (index = 0; index < postEditMd5Arr_Count; index++)
	{
		// get the next line from each of the MD5 structure&extents arrays
		preEditMd5Line = preEditMd5Arr.Item(index);
		postEditMd5Line = postEditMd5Arr.Item(index);
		fromEditorMd5Line = fromEditorMd5Arr.Item(index);
		// get the MD5 checksums from each line
		preEditMD5Sum = GetFinalMD5FromStructExtentString(preEditMd5Line);
		postEditMD5Sum = GetFinalMD5FromStructExtentString(postEditMd5Line);
		fromEditorMD5Sum = GetFinalMD5FromStructExtentString(fromEditorMd5Line);
		
		// start testing - first test: check for MD5 checksum of "0" in fromEditorMD5Sum,
		// and if so, the copy the span over from postEditText unilaterally (marker and
		// text, or marker an no text, as the case may be - doesn't matter since the
		// fromEditorText's marker had no content anyway)
		if (fromEditorMD5Sum == zeroStr)
		{
			// text from Paratext or Bibledit for this marker is absent so far, or the
			// marker is a contentless one anyway (we have to transfer them too)
			pPostEditOffsets = (MD5Map*)postEditOffsetsArr.Item(index);
			wxString fragmentStr = ExtractSubstring(pPostEditBuffer, pPostEditEnd, 
							pPostEditOffsets->startOffset, pPostEditOffsets->endOffset);
			newText += fragmentStr;
		}
		else
		{
			// the MD5 checksum for the external editor's marker contents for this
			// particular marker is non-empty; so, if the preEditText and postEditText at
			// the matching marker have the same checksum, then copy the fromEditorText's
			// text (and marker) 'as is'; otherwise, if they have different checksums,
			// then the user has done some adapting (or free translating if the text we
			// are dealing with is free translation text) and so the from-AI-document text
			// has to instead be copied to newText
			if (preEditMD5Sum == postEditMD5Sum)
			{
				// no user edits, so keep the from-external-editor version for this marker
				pFromEditorOffsets = (MD5Map*)fromEditorOffsetsArr.Item(index);
				wxString fragmentStr = ExtractSubstring(pFromEditorBuffer, pFromEditorEnd, 
								pFromEditorOffsets->startOffset, pFromEditorOffsets->endOffset);
				newText += fragmentStr;
			}
			else
			{
				// must be user edits done, so send them to newText along with the marker
				pPostEditOffsets = (MD5Map*)postEditOffsetsArr.Item(index);
				wxString fragmentStr = ExtractSubstring(pPostEditBuffer, pPostEditEnd, 
								pPostEditOffsets->startOffset, pPostEditOffsets->endOffset);
				newText += fragmentStr;
			}
		}
		// advance index
		index++;
	} // end of loop: for (index = 0; index < postEditMd5Arr_Count; index++)
	return newText;
}

// on invocation, the caller will have ensured that a USFM marker is the first character of
// the text parameter
void MapMd5ArrayToItsText(wxString& text, wxArrayPtrVoid& mappingsArr, wxArrayString& md5Arr)
{
#ifdef __WXDEBUG__
	wxString twofifty = text.Left(250);
	wxLogDebug(_T("MapMd5ArrayToItsText(), first 250 wxChars:\n%s"),twofifty.c_str());
#endif
	// work with wxChar pointers for the text
	const wxChar* pBuffer = text.GetData();
	int nBufLen = text.Len();
	wxChar* pStart = (wxChar*)pBuffer;	// point to the first wxChar (start) of the buffer text
	wxChar* pEnd = pStart + nBufLen;	// point past the last wxChar of the buffer text
	wxASSERT(*pEnd == '\0');
	// use  helpers.cpp versions of: bool Is_Marker(wxChar *pChar, wxChar *pEnd), 
	// bool IsWhiteSpace(const wxChar *pChar) and
	// int Parse_Marker(wxChar *pChar, wxChar *pEnd)
	int md5ArrayCount = md5Arr.GetCount();
	MD5Map* pMapStruct = NULL; // we create instances on the heap, store ptrs in mappingsArr
	wxChar* ptr = pStart; // initialize iterator
						   
	int lineIndex;
	wxString lineStr;
	wxString mkrFromMD5Line;
	size_t charCount = 0;
	wxString md5Str;
	int mkrCount = 0;
	size_t charOffset = 0;
#ifdef __WXDEBUG__
	wxChar* pStrBegin = NULL;
#endif
	for (lineIndex = 0; lineIndex < md5ArrayCount; lineIndex++)
	{
		// get next line from md5Arr
		lineStr = md5Arr.Item(lineIndex);
		mkrFromMD5Line = GetStrictUsfmMarkerFromStructExtentString(lineStr);
		charCount = (size_t)GetCharCountFromStructExtentString(lineStr);
		//md5Str = GetFinalMD5FromStructExtentString(lineStr); // not needed here

		// get the same marker in the passed in text
		wxASSERT(*ptr == gSFescapechar); // ptr must be pointing at a USFM
		// get the marker's character count
		mkrCount = Parse_Marker(ptr, pEnd);
		wxString wholeMkr = wxString(ptr, mkrCount);
		// it must match the one in lineStr
		wxASSERT(wholeMkr == mkrFromMD5Line);
		charOffset = (size_t)(ptr - pStart);
#ifdef __WXDEBUG__
		pStrBegin = ptr;
#endif

		// create an MD5Map instance & store it & start populating it
		pMapStruct = new MD5Map;
		pMapStruct->md5Index = lineIndex;
		pMapStruct->startOffset = charOffset;
		mappingsArr.Add(pMapStruct);

		// make ptr point charCount wxChars further along, since what this count leaves
		// out is the number of chars after the marker and before the first alphabetic
		// character (if any), and any eol characters - so adding this value now decreases
		// the number of iterations needed in the loop below in order to get to the next
		// marker
		if (charCount == 0)
		{
			// must advance ptr by at least one wxChar
			ptr++;
		}
		else
		{
			ptr = ptr + charCount;
		}
		wxASSERT(ptr <= pEnd);
		bool bReachedEnd = TRUE;
		while (ptr < pEnd)
		{
			if (Is_Marker(ptr, pEnd))
			{
				bReachedEnd = FALSE;
				break;
			}
			else
			{
				ptr++;
			}
		}
		charOffset = (size_t)(ptr - pStart);
		pMapStruct->endOffset = charOffset;
#ifdef __WXDEBUG__
		// show what the subspan of text is
		unsigned int nSpan = (unsigned int)((pStart + pMapStruct->endOffset) - pStrBegin);
		wxString str = wxString(pStrBegin, nSpan);
		wxLogDebug(_T("MapMd5ArrayToItsText(), map index = %d: nSpan = %d, textSpan = %s"),lineIndex, nSpan, str.c_str());
#endif
		// the next test should be superfluous, but no harm in it
		if (bReachedEnd)
			break;
	} // end of loop: for (lineIndex = 0; lineIndex < md5ArrayCount; lineIndex++)
	// whm 12Aug11 removed this UngetWriteBuf() call, which should NEVER be done on a READ-ONLY
	// buffer established with ::GetData().
	//text.UngetWriteBuf();
}

wxArrayString ObtainSubarray(const wxArrayString arr, size_t nStart, size_t nFinish)
{
	wxArrayString newArray; newArray.Clear();
	wxASSERT(nFinish >= nStart);
	if (nStart == nFinish)
	{
		return newArray; // return the empty array
	}
	newArray.Alloc(nFinish + 1 - nStart);
	size_t i;
	wxString s;
	for (i = nStart; i <= nFinish; i++)
	{
		s = arr.Item(i);
		newArray.Add(s);
	}
	return newArray;
}

// GetUpdatedText_UsfmsChanged() is the complex case (i.e. USFM markers have been
// changed - perhaps versification changes such as verse bridging, or markers added or
// deleted, or edited because misspelled). This function generates the USFM marked up text, either
// target text, or free translation text, which the caller will pass back to the relevant
// Paratext or Bibledit project, it's book or chapter as the case may be, when parts of
// the texts are not in sync with respect to USFM marker structure.
// 
// Our approach is to proceed on a marker by marker basis, as is done in
// GetUpdatedText_UsfmsUnchanged(), and vary from that only when we detect a mismatch of
// USFM markers. Priority is given first and foremost to editing work in the actual words
// - the editing may have been done in AI (in which case we flow the edits back to PT, and
// they will carry their markup with them, overwriting any markup changes in the same
// portion of the document in Paratext, if any, and that is something we can't do anything
// about - we consider it vital to give precedence to meaning over markup); or the editing
// may have only been done by the user in PT (in which case we detect that the AI document
// doesn't have any meaning and/or punctuation changes in that subsection, and if so, we
// leave the PT equivalent text subsection unchanged). So meaning has priority. This
// applies whether the chunk association is a minimal one - between the same marker and
// content in each of the two texts (postEditText and fromEditorText), or a much larger
// chunk of mismatched marker lines which have to be deal with as a whole (e.g. user may
// have added or removed markers, bridged some verses, etc and such changes appear in only
// one of the texts).
// 
// That should help explain the complexities in the code below, and why they are there.
wxString GetUpdatedText_UsfmsChanged(wxString& preEditText, wxString& postEditText, 
			wxString& fromEditorText, wxArrayString& preEditMd5Arr, wxArrayString& postEditMd5Arr,
			wxArrayString& fromEditorMd5Arr, wxArrayPtrVoid& postEditOffsetsArr, 
			wxArrayPtrVoid& fromEditorOffsetsArr)
{
	wxString newText; newText.Empty();
	wxString zeroStr = _T("0");
	// each text variant's MD5 structure&extents array has exactly the same USFMs, but
	// only for preEdit and postEdit varieties; the fromEditor variant will be different
	// otherwise this function won't be entered
	size_t preEditMd5Arr_Count = preEditMd5Arr.GetCount();					
	size_t postEditMd5Arr_Count = postEditMd5Arr.GetCount();					
	size_t fromEditorMd5Arr_Count = fromEditorMd5Arr.GetCount();
	// in debug build verify our assumptions are correct
	wxASSERT( preEditMd5Arr_Count == postEditMd5Arr_Count && 
			  preEditMd5Arr_Count != fromEditorMd5Arr_Count);
	MD5Map* pPostEditOffsets = NULL; // stores ptr of MD5Map from postEditOffsetsArr
	MD5Map* pFromEditorOffsets = NULL; // stores ptr of MD5Map from fromEditorOffsetsArr
	wxString preEditMd5Line;
	wxString postEditMd5Line;
	wxString fromEditorMd5Line;
	wxString preEditMD5Sum;
	wxString postEditMD5Sum;
	wxString fromEditorMD5Sum;

	// work with wxChar pointers for each of the postEditText and fromEditorText texts
	const wxChar* pPreEditBuffer = preEditText.GetData();
	int nPreEditBufLen = preEditText.Len();
	wxChar* pPreEditStart = (wxChar*)pPreEditBuffer;
	wxChar* pPreEditEnd = pPreEditStart + nPreEditBufLen;
	wxASSERT(*pPreEditEnd == '\0');

	const wxChar* pPostEditBuffer = postEditText.GetData();
	int nPostEditBufLen = postEditText.Len();
	wxChar* pPostEditStart = (wxChar*)pPostEditBuffer;
	wxChar* pPostEditEnd = pPostEditStart + nPostEditBufLen;
	wxASSERT(*pPostEditEnd == '\0');

	const wxChar* pFromEditorBuffer = fromEditorText.GetData();
	int nFromEditorBufLen = fromEditorText.Len();
	wxChar* pFromEditorStart = (wxChar*)pFromEditorBuffer;
	wxChar* pFromEditorEnd = pFromEditorStart + nFromEditorBufLen;
	wxASSERT(*pFromEditorEnd == '\0');

	// The following are the array iterators; if there were no differences in USFM
	// structure, then they'd stay in sync. However as soon as a mismatch of markers
	// happens, they will take different values and will likely be different from each
	// other on each iteration thereafter until the loop is done (the postEditArr_Index
	// value can be used for indexing into preEditMd5Arr as well, since the preEdit and
	// postEdit arrays are guaranteed to have the same USFM structure - since both come
	// from the same AI document and editing of source text in collaboration mode is
	// disallowed - giving no opportunity to the user to alter the document's USFM
	// structure from within AI itself)
	int postEditArr_Index = 0;
	int fromEditorArr_Index = 0;
    // The next two store the indices of the matched verse lines following a "mismatched
    // chunk" (or array counts if the mismatched chunk is at their ends) when
    // DelineateComplexChunksAssociation() has done it's work. These indices are where
    // normal line by line processing kicks off from, and so are not within the mismatched
    // chunk - but are one greater than the indices for the last marker's line in each of
    // the md5 arrays for the associated chunks which make up the mismatched chunk
	int	postEditArr_AfterChunkIndex = wxNOT_FOUND;
	int	fromEditorArr_AfterChunkIndex = wxNOT_FOUND;

	while (postEditArr_Index < (int)postEditMd5Arr_Count && fromEditorArr_Index < (int)fromEditorMd5Arr_Count)
	{
		// get the next line from each of the MD5 structure&extents arrays
		preEditMd5Line = preEditMd5Arr.Item(postEditArr_Index);
		postEditMd5Line = postEditMd5Arr.Item(postEditArr_Index);
		fromEditorMd5Line = fromEditorMd5Arr.Item(fromEditorArr_Index);
		// get the MD5 checksums from each line
		preEditMD5Sum = GetFinalMD5FromStructExtentString(preEditMd5Line);
		postEditMD5Sum = GetFinalMD5FromStructExtentString(postEditMd5Line);
		fromEditorMD5Sum = GetFinalMD5FromStructExtentString(fromEditorMd5Line);
		
        // start testing: check that the markers match; if they don't, then the first thing
        // to do is to delineate the span in each array which wraps the mismatched subset
        // of md5 lines - and then work out how to process the data within the "mismatched
        // chunk" so delineated - the md5Array lines which are immediately following each
        // such chunk are either beyond the end of the arrays (ie. equal to the array
        // counts) in which case we are done, or they are guaranteed to be verse lines in
        // which the verseNum string from each is an exact match, and so from that point on
        // linebyline processing can occur again until another mismatched chunk is
        // encountered, delineated, and processed, etc until done.
        // Note, in this test for mismatched markers, we don't stipulate what kind of
        // markers they should be - in particular, we don't stipulate that they be, say,
        // only verse markers. We want to catch things like new marker insertions - such as
        // poetry markers introduced either into AI via the user editing source text in
        // Paratext or Bibledit, or poetry markers introduced into the PT or BE target text
        // (which AI wouldn't know about and so they wouldn't be in the postEditText passed
        // in), similarly for other marker types, or editing out of markers done outside of
        // AI, or editing misspelled markers done in the external editor (such editing
        // can't be done within AI in collaboration mode, since Edit Source Text
        // functionality is not enabled in that mode), and the like.
		wxString postEditLineMkr = GetStrictUsfmMarkerFromStructExtentString(postEditMd5Line);  
		wxString fromEditorLineMkr = GetStrictUsfmMarkerFromStructExtentString(fromEditorMd5Line);
		if (postEditLineMkr != fromEditorLineMkr)
		{
#ifdef __WXDEBUG__
            // get the first 10 MD5Map structs and display their offsets and the text
            // delineated, for the fromEditorOffsetsArr (stores MD5Map struct ptrs) and its
            // fromEditorText
			wxLogDebug(_T("\n  *** First 10 MD5Map structs & extracted substrings ***"));
			MD5Map* pMap = NULL;
			int i;
			for (i = 0; i<10; i++)
			{
				pMap = (MD5Map*)fromEditorOffsetsArr.Item(i);
				wxString strSpan = ExtractSubstring(pFromEditorBuffer, pFromEditorEnd, 
								pMap->startOffset, pMap->endOffset);
				size_t numCharsInSpan = (size_t)(pMap->endOffset - pMap->startOffset);
				wxLogDebug(_T("map index %d   start wxChar offset  %d  end offset  %d , span size = %d   textSpanned =  %s"),
					i, pMap->startOffset, pMap->endOffset, numCharsInSpan, strSpan.c_str());
			}
#endif

			// first task is to delineate the extent of the mismatched sets of marker lines
			DelineateComplexChunksAssociation(postEditMd5Arr, fromEditorMd5Arr,
				postEditArr_Index,    // starting index of mismatched chunk within postEditMd5Arr
				postEditArr_AfterChunkIndex,    // the "after the end" index of mismatched chunk within postEditMd5Arr
				fromEditorArr_Index,  // starting index of mismatched chunk within fromEditorMd5Arr
				fromEditorArr_AfterChunkIndex); // the "after the end" index of mismatched chunk within fromEditorMd5Arr

			// Get the potential value for the last md5 line in the mismatched chunk for each
			// array - either or both might turn out to be an incorrect value, but we'll
			// correct further below. What we want to handle here is any information that
			// lies before chapter 1
			int postEditArr_LastLineIndex = postEditArr_AfterChunkIndex - 1;
			int fromEditorArr_LastLineIndex = fromEditorArr_AfterChunkIndex - 1;

            // When control gets here, we may have just found a mismatched section of lines
            // in material which is prior to chapter 1's line in one or both arrays - so
            // check and deal with the special cases where one array has no pre-chapter-one
            // material and the other does. Detected by: postEditArr_LastLineIndex will
            // have been set to -1 if postEditArr has no pre-chapter one material, and it
            // will be fromEditorArr_LastLineIndex that would have a value of -1 if
            // fromEditorArr has no pre-chapter-one material. But if neither has any such,
            // nothing special needs to be done; likewise, if both have none processing of
            // lines starts in each array in the milestoned data (ie. there are verse
            // number lines) and nothing special needs to be done here. So deal with the
            // two special situation mentioned above
            if (postEditArr_LastLineIndex == -1 && fromEditorArr_LastLineIndex != -1)
			{
				// postEditArr has no pre-chapter one material, so keep external editor's text
				MD5Map* pFromEditorArr_StartMap = (MD5Map*)fromEditorOffsetsArr.Item(fromEditorArr_Index);
				MD5Map* pFromEditorArr_LastMap = (MD5Map*)fromEditorOffsetsArr.Item(fromEditorArr_LastLineIndex);
				wxString fromEditorTextSubstring = ExtractSubstring(pFromEditorBuffer, pFromEditorEnd, 
							pFromEditorArr_StartMap->startOffset, pFromEditorArr_LastMap->endOffset);
				newText += fromEditorTextSubstring;
			}
			else if (postEditArr_LastLineIndex != -1 && fromEditorArr_LastLineIndex == -1)
			{
				MD5Map* pPostEditArr_StartMap = (MD5Map*)postEditOffsetsArr.Item(postEditArr_Index);
				MD5Map* pPostEditArr_LastMap = (MD5Map*)postEditOffsetsArr.Item(postEditArr_LastLineIndex);
				wxString postEditTextSubstring = ExtractSubstring(pPostEditBuffer, pPostEditEnd, 
								pPostEditArr_StartMap->startOffset, pPostEditArr_LastMap->endOffset);
				newText += postEditTextSubstring;
			}
			else
			{
                // This is probably a normal situation (that is, a mismatched section in
                // the milestoned area of the texts), or pre-chapter-one material which
                // exists in both texts and is not in sync. We handle these alike. However,
                // there is the possibility that one text has a field absent in the other -
                // in that situation, one index will not have advanced when the
                // DelineateComplexChunksAssociation() call returns - and if we compute the
                // *_LastLineIndex value by subtracting 1 from the *_AfterChunkIndex as we 
                // did above, then the *_LastLineIndex would end up smaller than the
                // *_Index (starting) value, which would be an error. So we must check here
                // for either index not advancing and handle the updating for those as a
                // special case
				if (postEditArr_Index == postEditArr_AfterChunkIndex)
				{
					// the from-editor text has one or more fields not in the postEdit
					// text at this point, so transfer the from-editor material unchanged
					wxASSERT(fromEditorArr_Index <= fromEditorArr_LastLineIndex);
					MD5Map* pFromEditorArr_StartMap = (MD5Map*)fromEditorOffsetsArr.Item(fromEditorArr_Index);
					MD5Map* pFromEditorArr_LastMap = (MD5Map*)fromEditorOffsetsArr.Item(fromEditorArr_LastLineIndex);
					wxString fromEditorTextSubstring = ExtractSubstring(pFromEditorBuffer, pFromEditorEnd, 
								pFromEditorArr_StartMap->startOffset, pFromEditorArr_LastMap->endOffset);
					newText += fromEditorTextSubstring;
				}
				else if (fromEditorArr_Index == fromEditorArr_AfterChunkIndex)
				{
					// the postEditText has one or more extra fields at this point, so
					// since we give priority to what is in AI, transfer these markers and
					// their text contents to newText (in effect, 'inserting' it into the
					// from-editor text at this location)
					wxASSERT(postEditArr_Index <= postEditArr_LastLineIndex);
					MD5Map* pPostEditArr_StartMap = (MD5Map*)postEditOffsetsArr.Item(postEditArr_Index);
					MD5Map* pPostEditArr_LastMap = (MD5Map*)postEditOffsetsArr.Item(postEditArr_LastLineIndex);
					wxString postEditTextSubstring = ExtractSubstring(pPostEditBuffer, pPostEditEnd, 
								pPostEditArr_StartMap->startOffset, pPostEditArr_LastMap->endOffset);
					newText += postEditTextSubstring;
				}
				else
				{
					// both indices have advanced, so it's a normal situation

					/* 
                    // the approach in this commented out section requires reconstituting
                    // text and reprocessing for md5 checksums a second time - I think a
                    // better way is to use the original checkums, and compare subarrays
                    // constructed from them - this will avoid the possibility of
                    // introducing a spurious text difference due to round tripping part of
                    // text through the md5 checksum process once more

					// now get the first and last lineStrings for each array from the array of
					// MD5Map structs stored, one for each line, from these we can get character
					// offsets into postEditText and fromEditorText for this whole mismatch chunk
					// (remember, postEditArr_Index and postEditArr_LastLineIndex are valid for
					// both preEditOffsetsArr and postEditOffsetsArr, and so the parameters in the
					// signatures on RHS of the first two lines below are not mistakes)
					MD5Map* pPreEditArr_StartMap = (MD5Map*)preEditOffsetsArr.Item(postEditArr_Index); // param is not a mistake
					MD5Map* pPreEditArr_LastMap = (MD5Map*)preEditOffsetsArr.Item(postEditArr_LastLineIndex); // param is not a mistake
					MD5Map* pPostEditArr_StartMap = (MD5Map*)postEditOffsetsArr.Item(postEditArr_Index);
					MD5Map* pPostEditArr_LastMap = (MD5Map*)postEditOffsetsArr.Item(postEditArr_LastLineIndex);
					MD5Map* pFromEditorArr_StartMap = (MD5Map*)fromEditorOffsetsArr.Item(fromEditorArr_Index);
					MD5Map* pFromEditorArr_LastMap = (MD5Map*)fromEditorOffsetsArr.Item(fromEditorArr_LastLineIndex);

					// work out whether or not there are meaning differences between the
					// preEditText and the postEditText for the mismatched range; we can do this
					// by pulling out from preEditMd5Arr and postEditMd5Arr the relevant subset of
					// lines into separate smaller wxArrayString variables, and then use Bill's
					// comparison functions to evaluate whether or not the meaning has been
					// changed within this mismatched section (by the user's work done in AI since
					// last File / Save)
					wxString preEditTextSubstring = ExtractSubstring(pPreEditBuffer, pPreEditEnd, 
									pPreEditArr_StartMap->startOffset, pPreEditArr_LastMap->endOffset);
					wxString postEditTextSubstring = ExtractSubstring(pPostEditBuffer, pPostEditEnd, 
									pPostEditArr_StartMap->startOffset, pPostEditArr_LastMap->endOffset);
					bool bTextAndOrPunctsChanged = IsTextOrPunctsChanged(preEditTextSubstring, postEditTextSubstring);
					*/
					// Recall that postEditArr_Index and postEditArr_LastLineIndex are the same for
					// both preEditOffsetsArr and postEditOffsetsArr (because markers cannot be
					// accessed and changed when in collaboration mode), and so param2 equals
					// param5, and param3 equals param6 in the following call
					bool bTextAndOrPunctsChanged = IsTextOrPunctsChanged(preEditMd5Arr, postEditArr_Index,
											postEditArr_LastLineIndex, postEditMd5Arr, postEditArr_Index, 
											postEditArr_LastLineIndex);
					if (bTextAndOrPunctsChanged)
					{
						// The user has edited something in this "mismatch section" within the
						// Adapt It document, and so we must honour that an pass this whole
						// section back to the external edit's text - so get the offsets to the
						// start and end of the whole section within postEditText, and then
						// transfer it to the external editor's text updated text - in newText
						MD5Map* pPostEditArr_StartMap = (MD5Map*)postEditOffsetsArr.Item(postEditArr_Index);
						MD5Map* pPostEditArr_LastMap = (MD5Map*)postEditOffsetsArr.Item(postEditArr_LastLineIndex);
						wxString postEditTextSubstring = ExtractSubstring(pPostEditBuffer, pPostEditEnd, 
									pPostEditArr_StartMap->startOffset, pPostEditArr_LastMap->endOffset);
						newText += postEditTextSubstring;
					}
					else
					{
						// The user, since the last File / Save, has not edited either the punctuation or words within
						// Adapt It within this mismatch section, so the external editor's current
						// version of the text for this section is to be preserved unchanged.
						// Extract it and add it to newText
						MD5Map* pFromEditorArr_StartMap = (MD5Map*)fromEditorOffsetsArr.Item(fromEditorArr_Index);
						MD5Map* pFromEditorArr_LastMap = (MD5Map*)fromEditorOffsetsArr.Item(fromEditorArr_LastLineIndex);
						wxString fromEditorTextSubstring = ExtractSubstring(pFromEditorBuffer, pFromEditorEnd, 
									pFromEditorArr_StartMap->startOffset, pFromEditorArr_LastMap->endOffset);
						newText += fromEditorTextSubstring;
					}
				} // end of else block for test: 
				  // else if (fromEditorArr_Index == fromEditorArr_AfterChunkIndex)
			} // end of else block for test: 
			  // else if (postEditArr_LastLineIndex != -1 && fromEditorArr_LastLineIndex == -1)
			
            // update the loop indices -- on the RHS use the variables which already have
            // the kick-off index value for the next iteration
			postEditArr_Index = postEditArr_AfterChunkIndex;
			fromEditorArr_Index = fromEditorArr_AfterChunkIndex;
		}  // end of TRUE block for test: if (postEditLineMkr != fromEditorLineMkr)
		else // the postEdit and fromEditor line's stored markers are identical
		{
			// simple-case protocols apply: so check for MD5 checksum of "0" in
			// fromEditorMD5Sum, and if so, the copy the span over from postEditText
			// unilaterally (marker and text, or marker an no text, as the case may be -
			// doesn't matter since the fromEditorText's marker had no content anyway)
			if (fromEditorMD5Sum == zeroStr)
			{
				// text from Paratext or Bibledit for this marker is absent so far, or the
				// marker is a contentless one anyway (we have to transfer them too)
				pPostEditOffsets = (MD5Map*)postEditOffsetsArr.Item(postEditArr_Index);
				wxString fragmentStr = ExtractSubstring(pPostEditBuffer, pPostEditEnd, 
								pPostEditOffsets->startOffset, pPostEditOffsets->endOffset);
				newText += fragmentStr;
			}
			else
			{
				// the MD5 checksum for the external editor's marker contents for this
				// particular marker is non-empty; so, if the preEditText and postEditText at
				// the matching marker have the same checksum, then copy the fromEditorText's
				// text (and marker) 'as is'; otherwise, if they have different checksums,
				// then the user has done some adapting (or free translating if the text we
				// are dealing with is free translation text) and so the from-AI-document text
				// has to instead be copied to newText
				if (preEditMD5Sum == postEditMD5Sum)
				{
					// no user edits, so keep the from-external-editor version for this marker
					pFromEditorOffsets = (MD5Map*)fromEditorOffsetsArr.Item(fromEditorArr_Index);
					wxString fragmentStr = ExtractSubstring(pFromEditorBuffer, pFromEditorEnd, 
									pFromEditorOffsets->startOffset, pFromEditorOffsets->endOffset);
					newText += fragmentStr;
				}
				else
				{
					// must be user edits done, so send them to newText along with the marker
					pPostEditOffsets = (MD5Map*)postEditOffsetsArr.Item(postEditArr_Index);
					wxString fragmentStr = ExtractSubstring(pPostEditBuffer, pPostEditEnd, 
									pPostEditOffsets->startOffset, pPostEditOffsets->endOffset);
					newText += fragmentStr;
				}
			}
			// advance indices
			postEditArr_Index++;
			fromEditorArr_Index++;
		} // end of else block for test: if (postEditLineMkr != fromEditorLineMkr)
	} // end of loop: for (index = 0; index < postEditMd5Arr_Count; index++)

	// handle the possibility that one of the arrays might have unprocessed material in it
	// after the above loop has finished
	if (postEditArr_Index < (int)postEditMd5Arr_Count - 1)
	{
		// there is unprocessed material at the end of postEditArr, so it must be transferred
		MD5Map* pPostEditArr_StartMap = (MD5Map*)postEditOffsetsArr.Item(postEditArr_Index);
		MD5Map* pPostEditArr_LastMap = (MD5Map*)postEditOffsetsArr.Item((int)postEditMd5Arr_Count - 1);
		wxString postEditTextSubstring = ExtractSubstring(pPostEditBuffer, pPostEditEnd, 
					pPostEditArr_StartMap->startOffset, pPostEditArr_LastMap->endOffset);
		newText += postEditTextSubstring;
	}
	if (fromEditorArr_Index < (int)fromEditorMd5Arr_Count - 1)
	{
		// there is unprocessed material at the end of fromEditorArr, get it and add it to
		// newText
		MD5Map* pFromEditorArr_StartMap = (MD5Map*)fromEditorOffsetsArr.Item(fromEditorArr_Index);
		MD5Map* pFromEditorArr_LastMap = (MD5Map*)fromEditorOffsetsArr.Item((int)fromEditorMd5Arr_Count - 1);
		wxString fromEditorTextSubstring = ExtractSubstring(pFromEditorBuffer, pFromEditorEnd, 
					pFromEditorArr_StartMap->startOffset, pFromEditorArr_LastMap->endOffset);
		newText += fromEditorTextSubstring;
	}
	return newText;
}





