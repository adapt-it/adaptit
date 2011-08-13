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


struct VerseAnalysis {
	bool				bIsComplex; // TRUE if verse num is a range, or something like 6b,
								// or is a gap in the chapter/verses; set to FALSE if 
								// just a simple number
	wxString			strDelimiter; // delimiter string used in a range, eg. - in 3-5
	wxString			strStartingVerse; // string version of the verse number or first verse number of a range
	wxChar				charStartingVerseSuffix;  // for the a in something like 6a-8, or 9a
	wxString			strEndingVerse; // string version of the verse number or final verse number of a range
	wxChar				charEndingVerseSuffix; // for the a in something like 15-17a
};
/// An enum for selecting which kind of text to generate in order to send it back to the external
/// editor (PT or BE), whether target text, or a free translation
enum SendBackTextType
{
	makeTargetText = 1,
	makeFreeTransText
};
enum DoFor 
{
	collab_source_text,
	collab_target_text,
	collab_freeTrans_text
};
enum CommandLineFor
{
	reading,
	writing
};
// this struct stores an index to a "\\mkr :nnn:md5sum" line in a structure&extents array
// and also stores, in startOffset and endOffset, indices to the first and last wxChar
// instances of the span of text within the wxString which gave rise to the
// structure&extents array
struct MD5Map {
	int md5Index;
	size_t startOffset;
	size_t endOffset;
};
struct VerseInf {
	wxString verseNumStr;
	int indexIntoArray;
	bool bIsComplex;
};




class CBString;
class SPList;	// declared in SourcePhrase.h WX_DECLARE_LIST(CSourcePhrase, SPList); macro 
				// and defined in SourcePhrase.cpp WX_DEFINE_LIST(SPList); macro
class CSourcePhrase;

///////////////////////////////////////////////////////////////////////////////////
/// This group of functions are used for analysis of texts in order to get an updated text
/// to return to the external editor
///////////////////////////////////////////////////////////////////////////////////
	bool	DoVerseAnalysis(const wxString& verseNum, VerseAnalysis& rVerseAnal); // return TRUE if is complex
	// this overload takes an array of structure& extent checksums, gets the
	// wxString at the 0-based index given by lineIndex, and fills out the passed in
	// VerseAnalysis struct; returns TRUE if the verse structure is complex, FALSE if it
	// is simple
	bool		DoVerseAnalysis(VerseAnalysis& refVAnal, const wxArrayString& md5Array, size_t lineIndex);
	void		InitializeVerseAnalysis(VerseAnalysis& rVerseAnal);
	bool		GetNextVerseLine(const wxArrayString& usfmText, int& index);
	bool		GetAnotherVerseLine(const wxArrayString& usfmText, int& index);
	bool		IsVerseLine(const wxArrayString& usfmText, int index);
	bool		IsComplexVersificationLine(const wxArrayString& md5Arr, size_t lineIndex);
	bool		IsComplexVersificationLine(const wxString& verseLineStr);
	wxString	GetInitialUsfmMarkerFromStructExtentString(const wxString str);
	wxString	GetStrictUsfmMarkerFromStructExtentString(const wxString str);
	wxString	GetFinalMD5FromStructExtentString(const wxString str);
	int			GetCharCountFromStructExtentString(const wxString str);
	int			FindExactVerseNum(const wxArrayString& md5Arr, int nStart, const wxString& verseNum);
	int			FindMatchingVerseNumInOtherChapterArray(const wxArrayPtrVoid& verseInfArr, wxString& verseNum);
	void		DelineateComplexChunksAssociation(const wxArrayString& postEditMd5Arr, 
						const wxArrayString& fromEditorMd5Arr, int postEditStart, 
						int& postEditEnd, int fromEditorStart, int& fromEditorEnd);
	void		GetRemainingMd5VerseLinesInChapter(const wxArrayString& md5Arr, int nStart, wxArrayPtrVoid& verseLinesArr);
	void		DeleteAllVerseInfStructs(wxArrayPtrVoid& arr);
	int			FindNextChapterLine(const wxArrayString& md5Arr, int nStartAt, bool& bBeforeChapterOne);
	wxString	GetNumberFromChapterOrVerseStr(const wxString& verseStr);

////////////////////// end of those for analysis of texts //////////////////////////


	// returns the absolute path to the folder being used as the Adapt It work folder, whether
	// in standard location or in a custom location - but for the custom location only
	// provided it is a "locked" custom location (if not locked, then the path to the standard
	// location is returned, i.e. m_workFolderPath, rather than m_customWorkFolderPath). The
	// "lock" condition ensures that a snooper can't set up a PT or BE collaboration and
	// the remote user not being aware of it.
	wxString		SetWorkFolderPath_For_Collaboration();
	bool			IsEthnologueCodeValid(wxString& code);
	// the next function is created from OnWizardPageChanging() in Projectpage.cpp, and
	// tweaked so as to remove support for the latter's context of a wizard dialog
	bool			HookUpToExistingAIProject(CAdapt_ItApp* pApp, wxString* pProjectName, 
							wxString* pProjectFolderPath);
	// a module for doing the layout and getting the view ready for the user to start
	// adapting;; it is not limited to being used in a Collaboration scenario
	void			SetupLayoutAndView(CAdapt_ItApp* pApp, wxString& docTitle);
	// move the newSrc string of just-obtained (from PT or BE) source text, currently in the
	// .temp folder, to the __SOURCE_INPUTS folder, creating the latter folder if it doesn't
	// already exist, and storing in a file with filename constructed from fileTitle plus an
	// added .txt extension; if a file of that name already exists there, overwrite it.
	bool			MoveTextToFolderAndSave(CAdapt_ItApp* pApp, wxString& folderPath, 
							wxString& pathCreationErrors, wxString& theText, wxString& fileTitle,
							bool bAddBOM = FALSE);
	wxString		ExportTargetText_For_Collab(SPList* pDocList);
	wxString		ExportFreeTransText_For_Collab(SPList* pDocList);
	wxString		GetTextFromFileInFolder(CAdapt_ItApp* pApp, wxString folderPath, wxString& fileTitle);
	wxString		GetTextFromFileInFolder(wxString folderPathAndName); // an override of above function
	wxArrayString	BreakStringBufIntoChapters(const wxString& bookString);
	wxString		GetTextFromAbsolutePathAndRemoveBOM(wxString& absPath);
	bool			OpenDocWithMerger(CAdapt_ItApp* pApp, wxString& pathToDoc, wxString& newSrcText, 
						   bool bDoMerger, bool bDoLayout, bool bCopySourceWanted);
	void			UnloadKBs(CAdapt_ItApp* pApp);
	bool			CreateNewAIProject(CAdapt_ItApp* pApp, wxString& srcLangName, wxString& tgtLangName,
							wxString& srcEthnologueCode, wxString& tgtEthnologueCode,
							bool bDisableBookMode);
	wxString		ChangeFilenameExtension(wxString filenameOrPath, wxString extn);
	bool			KeepSpaceBeforeEOLforVerseMkr(wxChar* pChar); //BEW added 13Jun11

	wxString		GetPathToRdwrtp7(); // used in GetSourceTextFromEditor::OnInit()
	wxString		GetBibleditInstallPath();  // used in GetSourceTextFromEditor::OnInit()

	wxArrayString	GetUsfmStructureAndExtent(wxString& sourceFileBuffer);
	enum			CompareUsfmTexts CompareUsfmTextStructureAndExtent(const wxArrayString& usfmText1, 
							const wxArrayString& usfmText2);

	bool			IsTextOrPunctsChanged(wxString& oldText, wxString& newText);
	// overload of the above, taking arrays and start & finish item indices as parameters
	bool			IsTextOrPunctsChanged(wxArrayString& oldMd5Arr, int oldStart, int oldEnd,
							wxArrayString& newMd5Arr, int newStart, int newEnd);
	bool			IsUsfmStructureChanged(wxString& oldText, wxString& newText);
	wxString		GetShortNameFromProjectName(wxString projName);

	wxString		MakePathToFileInTempFolder_For_Collab(enum DoFor textKind);
	wxString		BuildCommandLineFor(enum CommandLineFor lineFor, enum DoFor textKind);
	void			TransferTextBetweenAdaptItAndExternalEditor(enum CommandLineFor lineFor, enum DoFor textKind,
							wxArrayString& textIOArray, wxArrayString& errorsIOArray, long& resultCode);
	
	bool			CopyTextFromBibleditDataToTempFolder(wxString projectPath, wxString bookName, 
							int chapterNumber, wxString tempFilePathName, wxArrayString& errors);
	bool			CopyTextFromTempFolderToBibleditData(wxString projectPath, wxString bookName, 
							int chapterNumber, wxString tempFilePathName, wxArrayString& errors);

	wxString		GetUpdatedText_UsfmsUnchanged(wxString& postEditText, wxString& fromEditorText,
							wxArrayString& preEditMd5Arr, wxArrayString& postEditMd5Arr, 
							wxArrayString& fromEditorMd5Arr,wxArrayPtrVoid& postEditOffsetsArr, 
							wxArrayPtrVoid& fromEditorOffsetsArr);

	wxString		GetUpdatedText_UsfmsChanged(wxString& preEditText, wxString& postEditText, 
							wxString& fromEditorText, wxArrayString& preEditMd5Arr, 
							wxArrayString& postEditMd5Arr, wxArrayString& fromEditorMd5Arr, 
							wxArrayPtrVoid& postEditOffsetsArr, wxArrayPtrVoid& fromEditorOffsetsArr);
	// next function includes nStart & nFinish items within the subarray
	wxArrayString	ObtainSubarray(const wxArrayString arr, size_t nStart, size_t nFinish); 

	// BEW added 11July, to get changes to the adaptation and free translation back to the
	// respective PT or BE projects; the post-edit text from the document at the time of
	// File / Save is returned in the postEditText parameter - it will be either target
	// text, or freeTrans text, and the caller will use it to replace what is stored in
	// the relevant app member for the pre-edit tgt or freeTrans text. The returned
	// wxString is the updated USFM text string to be sent to PT or BE - or an empty
	// string if there was an error.
	wxString		MakeUpdatedTextForExternalEditor(SPList* pDocList, 
							enum SendBackTextType makeTextType, wxString& postEditText);

	void			MapMd5ArrayToItsText(wxString& itsText, wxArrayPtrVoid& mappingsArr, 
							wxArrayString& md5Arr);
	void			DeleteMD5MapStructs(wxArrayPtrVoid& structsArr);
#endif

