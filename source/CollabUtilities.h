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
//
#ifndef collabUtilities_h
#define collabUtilities_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "CollabUtilities.h"
#endif

#ifndef _string_h_loaded
#define _string_h_loaded
#include "string.h"
#endif
#include "Adapt_It.h"

/// An enum for selecting which kind of text to generate in order to send it back to the external
/// editor (PT or BE), whether target text, or a free translation
enum SendBackTextType
{
	makeTargetText = 1,
	makeFreeTransText
};

// VChunkAndMap indexes into a wxArrayString of StructureAndExtents MD5 analysis, and also
// with offsets to wxChar in the wxString text buffer from which the MD5 analysis is done;
// it is based on MergeUpdatedSrc.cpp's SfmChunk struct, but without the int chapter &
// verse members, and the function which populates VChunkAndMap is a cut down version of
// the one, AnalyseChapterVerseRef(), which does the job for SfmChunk in MergeUpdatedSrc.cpp
struct VChunkAndMap {
	bool				bContainsText; // default TRUE, set FALSE if the information in the chunk is
									   // absent (such as a \v marker without any text
									   // following the verse number)
	bool				bIsComplex; // TRUE if verse num is a range, or something like 6b,
								// or is a gap in the chapter/verses; set to FALSE if 
								// just a simple number
	int					lineStart; // index of the chunk's first line in the extents array
	int					lineEnd; // index of the chunk's ending line in the extents array
	int					startOffset; // offset to the wxChar in the original text file, where this chunk starts
	int					endOffset; // offset to the wxChar in the original text file, which immediately
								   // follows where this chunk ends
	// the remaining members pertain to verseChunk type, and store info from the verse
	// reference in the extents array's line - but with chapter number string and a colon 
	// added so that the reference is equivalent to Adapt It's ch:verse style of reference
	wxString			strChapter; // chapter number as a string (always set, except for introduction material)
	wxString			strDelimiter; // delimiter string used in a range, eg. - in 3-5
	wxString			strStartingVerse; // string version of the verse number or first verse number of a range
	wxChar				charStartingVerseSuffix;  // for the a in something like 6a-8, or 9a
	wxString			strEndingVerse; // string version of the verse number or final verse number of a range
	wxChar				charEndingVerseSuffix; // for the a in something like 15-17a
};


class CBString;
class SPList;	// declared in SourcePhrase.h WX_DECLARE_LIST(CSourcePhrase, SPList); macro 
				// and defined in SourcePhrase.cpp WX_DEFINE_LIST(SPList); macro
class CSourcePhrase;

	// returns the absolute path to the folder being used as the Adapt It work folder, whether
	// in standard location or in a custom location - but for the custom location only
	// provided it is a "locked" custom location (if not locked, then the path to the standard
	// location is returned, i.e. m_workFolderPath, rather than m_customWorkFolderPath). The
	// "lock" condition ensures that a snooper can't set up a PT or BE collaboration and
	// the remote user not being aware of it.
	wxString SetWorkFolderPath_For_Collaboration();
	bool IsEthnologueCodeValid(wxString& code);
	// the next function is created from OnWizardPageChanging() in Projectpage.cpp, and
	// tweaked so as to remove support for the latter's context of a wizard dialog
	bool HookUpToExistingAIProject(CAdapt_ItApp* pApp, wxString* pProjectName, wxString* pProjectFolderPath);
	// a module for doing the layout and getting the view ready for the user to start
	// adapting;; it is not limited to being used in a Collaboration scenario
	void SetupLayoutAndView(CAdapt_ItApp* pApp, wxString& docTitle);
	// move the newSrc string of just-obtained (from PT or BE) source text, currently in the
	// .temp folder, to the __SOURCE_INPUTS folder, creating the latter folder if it doesn't
	// already exist, and storing in a file with filename constructed from fileTitle plus an
	// added .txt extension; if a file of that name already exists there, overwrite it.
	bool MoveTextToFolderAndSave(CAdapt_ItApp* pApp, wxString& folderPath, 
					wxString& pathCreationErrors, wxString& theText, wxString& fileTitle,
					bool bAddBOM = FALSE);
	wxString GetTextFromFileInFolder(CAdapt_ItApp* pApp, wxString folderPath, wxString& fileTitle);
	wxString GetTextFromFileInFolder(wxString folderPathAndName); // an override of above function
	wxString GetTextFromAbsolutePathAndRemoveBOM(wxString& absPath);
	bool OpenDocWithMerger(CAdapt_ItApp* pApp, wxString& pathToDoc, wxString& newSrcText, 
						   bool bDoMerger, bool bDoLayout, bool bCopySourceWanted);
	void UnloadKBs(CAdapt_ItApp* pApp);
	bool CreateNewAIProject(CAdapt_ItApp* pApp, wxString& srcLangName, wxString& tgtLangName,
							wxString& srcEthnologueCode, wxString& tgtEthnologueCode,
							bool bDisableBookMode);
	wxString ChangeFilenameExtension(wxString filenameOrPath, wxString extn);
	bool KeepSpaceBeforeEOLforVerseMkr(wxChar* pChar); //BEW added 13Jun11

	wxString GetPathToRdwrtp7(); // used in GetSourceTextFromEditor::OnInit()
	wxString GetBibleditInstallPath();  // used in GetSourceTextFromEditor::OnInit()

	wxString GetNumberFromChapterOrVerseStr(const wxString& verseStr);
	wxArrayString GetUsfmStructureAndExtent(wxString& sourceFileBuffer);
	wxString GetInitialUsfmMarkerFromStructExtentString(const wxString str);
	wxString GetStrictUsfmMarkerFromStructExtentString(const wxString str);
	wxString GetFinalMD5FromStructExtentString(const wxString str);
	enum CompareUsfmTexts CompareUsfmTextStructureAndExtent(const wxArrayString& usfmText1, const wxArrayString& usfmText2);
	bool GetNextVerseLine(const wxArrayString usfmText, int& index);
	bool IsTextOrPunctsChanged(wxString& oldText, wxString& newText); // text is usually src
	bool IsUsfmStructureChanged(wxString& oldText, wxString& newText); // text is usually src

	bool AnalyseChapterVerseRef_For_Collab(wxString& strChapVerse, wxString& strChapter, 
			wxString& strDelimiter, wxString& strStartingVerse, wxChar& charStartingVerseSuffix, 
			wxString& strEndingVerse, wxChar& charEndingVerseSuffix);
	wxString MakeAnalysableChapterVerseRef(wxString strChapter, wxString strVerseOrRange);
	bool AnalyseChVs_For_Collab(wxArrayString& md5Array, int chapterLine, int verseLine, 
		VChunkAndMap*& pVChMap, bool bVerseMarkerSeenAlready);
	void InitializeVChunkAndMap_ChapterVerseInfoOnly(VChunkAndMap*& pVChMap);

	// the md5Array chunking and mapping by offsets into originalText string are done by
	// the following chunking function
	void GetNextVerse_ForChunking(const wxArrayString& md5Array, const wxString& originalText, 
				const int& curLineVerseInArr, const int& curOffsetVerseInText, 
				int& endLineVerseInArr, int& endOffsetVerseInText, int& chapterLineIndex);


	// BEW added 11July, to get changes to the adaptation and free translation back to the
	// respective PT or BE projects
	wxString MakePostEditTextForExternalEditor(SPList* pDocList, 
					enum SendBackTextType makeTextType, bool bWholeBook = FALSE);

#endif

