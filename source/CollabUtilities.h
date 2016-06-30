/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CollabUtilities.h
/// \author			Bruce Waters, from code taken from SetupEditorCollaboration.h by Bill Martin
/// \date_created	27 July 2011
/// \rcs_id $Id$
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
#include "helpers.h"

// mrh 11Jun14 - removed the const wxString createNewProjectInstead since on OSX localization is set up after const initialization, and in any case
// this string is only used on one place, in GetAIProjectFolderForCollab().

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
// enum SendBackType has been moved to Adapt_It.h, because it is not needed in ExportFunctions.cpp
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

// This struct is used for finding the next sync position when the main loop comes to
// mismatched markers (which not necessarily are \v markers, we need to deal with things
// like \q mismatched with \c etc). We also want to handle \c like we do for \v, so that
// for purposes of the matching algorithm when we come to a "complex" part of the array
// matchups, we can halt at the \c which precedes the \v 1, and treat the \c as equivalent
// to a verse marker with verse number 0. So for a halt at \c, we put verseNumStr =
// _T("0") and set chapterStr to the actual number string for the chapter, and bIsComplex
// to FALSE; but for a halt at a verse, chapterStr will be empty, and verseNumStr will
// have something different than _T("0"), and bIsComplex could be TRUE or FALSE depending
// on what is in verseNumStr)
struct VerseInf {
	wxString verseNumStr;
	wxString chapterStr;
	int indexIntoArray;
	bool bIsComplex;
};

enum TypeOfMatch {
	insertPostEditChunk,
	removeFromEditorChunk,
	replaceTheFromEditorChunk,
	transferAllThatRemains
};

enum SearchWhere {
	aiData,
	editorData
};

enum Complexity {
	bothAreNotComplex,
	onlyPostEditIsComplex,
	onlyFromEditorIsComplex,
	bothAreComplex
};

/// a struct for use in collaboration conflict resolution dialog
struct CollabAction  {
	wxString	preVerseOneText;      // the \c and any other pre-verse-1 information
	wxString	sourceText;           // empty except for a conflicted verse instance
	wxString	AI_verse_version;     // this or the next may be empty, depending on action
	wxString	PTorBE_verse_version; // to be taken; for a conflict, both are non-empty
	bool		bIsPreVerseOne;		  // TRUE when this struct just has the preVerseOneText for obligatory transfer
	bool		bPTorBE_verse_empty;  // TRUE if the md5sum(s) for the PTorBE verse are all empty
	bool		bAI_verse_empty;      // TRUE if the md5sum(s) for the AI verse are all empty
	bool		bUserEditsDetected;   // TRUE if preEdit and postEdit versions differ
	bool		bConflictedVerses;	  // TRUE if the Conflict Resolution dlg needs to be used
	wxString	bookCode; // the 3-letter code for the currently open book or chapter of a book
	wxString	chapter_ref; // the string form of the chapter eg. "3"
	wxString	verse_ref; // the string form of the verse (e.g. "23" or "15-16" or whatever)
};

/// another struct for use in collaboration conflict resolution dialog
struct ConflictRes  {
	int			collabActionsArrIndex; // which struct in the collabActionsArr generated this one
	bool		bUserWantsAIverse; // user's response for which verse version to send
	wxString	srcText;
	wxString	AIText;
	wxString	PTorBEText_original; // Restore button will use this for resetting RHS text box
	wxString	PTorBEText_edited; // the adaptation or free translation text to send
	wxString	bookCodeStr;
	wxString	chapterRefStr;
	wxString	verseRefStr;
};

class CBString;
class SPList;	// declared in SourcePhrase.h WX_DECLARE_LIST(CSourcePhrase, SPList); macro 
				// and defined in SourcePhrase.cpp WX_DEFINE_LIST(SPList); macro
class CSourcePhrase;

///////////////////////////////////////////////////////////////////////////////////
/// This group of functions are used for analysis of texts in order to get an updated text
/// to return to the external editor
///////////////////////////////////////////////////////////////////////////////////
	enum			EditorProjectVerseContent DoProjectAnalysis(enum CollabTextType textType,
						wxString compositeProjName,wxString editor,wxString ptVersion,
						wxString& emptyBooks,wxString& booksWithContent,wxString& errorMsg);
	wxArrayString	BreakStringBufIntoChapters(const wxString& bookString);
	bool			DoVerseAnalysis(const wxString& verseNum, VerseAnalysis& rVerseAnal); // return TRUE if is complex
	// this overload takes an array of structure& extent checksums, gets the
	// wxString at the 0-based index given by lineIndex, and fills out the passed in
	// VerseAnalysis struct; returns TRUE if the verse structure is complex, FALSE if it
	// is simple
	bool			DoVerseAnalysis(VerseAnalysis& refVAnal, const wxArrayString& md5Array, size_t lineIndex);
	void			DeleteAllVerseInfStructs(wxArrayPtrVoid& arr);
	bool			GetNextVerseLine(const wxArrayString& usfmText, int& index, bool bOrTestForChapterLine = FALSE);
	int				EndOfVerse(const wxArrayString& usfmMd5LinesArr, int nVerseStart); // deprecated 20Jul15
	bool			GetAnotherVerseOrChapterLine(const wxArrayString& usfmText, int& index, wxString& chapterStr);
	wxString		GetInitialUsfmMarkerFromStructExtentString(const wxString str);
	wxString		GetStrictUsfmMarkerFromStructExtentString(const wxString str);
	wxString		GetFinalMD5FromStructExtentString(const wxString str);
	int				GetCharCountFromStructExtentString(const wxString str);
	wxString		GetNumberFromChapterOrVerseStr(const wxString& verseStr);
	void			GetRemainingMd5VerseLines(const wxArrayString& md5Arr, int nStart, 
												wxArrayPtrVoid& verseLinesArr);
	void GetChapterListAndVerseStatusFromBook(enum CollabTextType textType, 
								wxArrayString& usfmStructureAndExtentArray,
								wxString collabCompositeProjectName,
								wxString targetBookFullName, 
								wxArrayString& staticBoxDescriptionArray,
								wxArrayString& chapterList,
								wxArrayString& statusList,
								bool& bBookIsEmpty);
	wxString GetStatusOfChapter(enum CollabTextType cTextType, wxString collabCompositeProjectName,
			const wxArrayString &usfmStructureAndExtentArray, int indexOfChItem, wxString bookFullName,
			bool& bChapterIsEmpty, wxString& nonDraftedVerses);

	bool FindMatchingVerseInf(SearchWhere whichArray,
		VerseInf*   matchThisOne, // the struct instance for what a matchup is being tried in the being-scanned array
		int&		atIndex, // the index into the being-scanned VerseInf array at which the matchup succeeded
		const wxArrayPtrVoid& postEditVerseArr, // the temporary array of VerseInf structs for postEdit data
		const wxArrayPtrVoid& fromEditorVerseArr); // the temporary array of VerseInf structs for fromEditor data

	// BEW added 22Jun15 to support preEdit data being structurally different due to things like filtering
	// BEW 10Jul15 changed parameter names a bit so it can be reused with different text's array
	bool FindAMatchingVerseInf(
		VerseInf*   matchThisOne, // the struct instance for what a matchup is being tried in the being-scanned preEditVerseArr
		int&		atIndex, // the index into the being-scanned VerseInf array at which the matchup succeeded
		const wxArrayPtrVoid& inWhichVerseArr); // the temporary array of VerseInf structs, 
												// will be either preEdit data or sourceText data
	bool GetMatchedChunksUsingVerseInfArrays(int postEditStart, // index of non-matched verse md5 line in postEditMd5Arr
		int   fromEditorStart, // index of non-matched verse md5 line in fromEditorMd5Arr
		int   preEditStart, // index of equivalent-to-postEdit verse md5 line in preEditMd5Arr
		int   sourceTextStart, // index of equivalent-to-postEdit verse md5 line in sourceTextMd5Arr
		const wxArrayString& postEditMd5Arr, // full md5 lines array for AI postEdit text
		const wxArrayString& fromEditorMd5Arr, // full md5 lines array for PT fromEditor text
		const wxArrayString& preEditMd5Arr, // full md5 lines array for AI preEdit text
		const wxArrayString& sourceTextMd5Arr, // full md5 lines array for sourceText AS IN ADAPT IT (it may
		// have had changes done due to filterings, making it permanently different
		// in USFM structure than the source text which is in Paratext or BE;
		// there is no feedback mechanism for filterings to change the PT or BE
		// structure of the source text in either of those editors
		int&  postEditEnd,     // points at index in the md5 array of last postEdit field in the matched chunk
		int&  fromEditorEnd,   // points at index into the md5 array of last fromEditor field in the matched chunk
		int&  preEditEnd,      // points at index in the md5 array of last preEdit field in the matched chunk
		int&  sourceTextEnd);  // points at index in the md5 array of last sourceText field in the matched chunk

	bool HasInfoChanged(
		int preEditIndex,    // index of current preEdit text verse md5 line in preEditMd5Arr
		int postEditIndex,   // index of current postEdit text verse md5 line in postEditMd5Arr
		int fromEditorIndex, // index of current fromEditor verse md5 line in fromEditorMd5Arr
		int sourceTextIndex, // index of current sourceText verse md5 line in sourceTextMd5Arr
		const wxArrayString& preEditMd5Arr, // full one-chapter md5 lines array for AI preEdit text
		const wxArrayString& postEditMd5Arr, // full one-chapter md5 lines array for AI postEdit text
		const wxArrayString& fromEditorMd5Arr, // full one-Chapter md5 lines array for PT or BE fromEditor text
		const wxArrayString& sourceTextMd5Arr, // full one-Chapter md5 lines array for PT or BE fromEditor text
		int  preEditEnd, // index of the last md5 line of preEdit text immediately prior to next verse
						  // or if no next verse, the last md5 line (it may not be a verse line) in the array
		int  postEditEnd, // index of the last md5 line of preEdit text immediately prior to next verse
						   // or if no next verse, the last md5 line (it may not be a verse line) in the array
		int  fromEditorEnd, // index of the last md5 line of preEdit text immediately prior to next verse
							 // or if no next verse, the last md5 line (it may not be a verse line) in the array
		int  sourceTextEnd, // index of the last md5 line of sourceText immediately prior to next verse
							 // or if no next verse, the last md5 line (it may not be a verse line) in the array
		bool&	bTheTwoTextsDiffer	// return FALSE if there was no detected punctuation difference AND
									// no detected overall text difference; but TRUE if either kind of
									// difference was detected (BEW added 10Jul15, for conflict res support)
		);        

	int				FindExactVerseNum(const wxArrayString& md5Arr, int nStart, const wxString& verseNum);
	int				FindNextChapterLine(const wxArrayString& md5Arr, int nStartAt, bool& bBeforeChapterOne);
	void			InitializeVerseAnalysis(VerseAnalysis& rVerseAnal);
	bool			IsChapterLine(const wxArrayString& usfmText, int index, wxString& chapterStr);
	bool			IsVerseLine(const wxArrayString& usfmText, int index);

	// The next subset are the main workhorses for the creation of the updated text...
	
	// Next function includes the nStart & nFinish items within the subarray
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
	bool			OpenDocWithMerger(CAdapt_ItApp* pApp, wxString& pathToDoc, wxString& newSrcText, 
						    bool bDoMerger, bool bDoLayout, bool bCopySourceWanted);
	wxString		GetUpdatedText_UsfmsUnchanged(wxString& postEditText, wxString& fromEditorText,
							wxString& sourceText, wxArrayString& sourceTextMd5Arr,
							wxArrayString& preEditMd5Arr, wxArrayString& postEditMd5Arr, 
							wxArrayString& fromEditorMd5Arr,wxArrayPtrVoid& postEditOffsetsArr, 
							wxArrayPtrVoid& fromEditorOffsetsArr, wxArrayPtrVoid& sourceTextOffsetsArr);
	wxString		GetUpdatedText_UsfmsChanged(wxString& postEditText, wxString& fromEditorText,
							wxString& sourceText, wxArrayString& sourceTextMd5Arr,
							wxArrayString& preEditMd5Arr, wxArrayString& postEditMd5Arr, 
							wxArrayString& fromEditorMd5Arr, wxArrayPtrVoid& postEditOffsetsArr, 
							wxArrayPtrVoid& fromEditorOffsetsArr, wxArrayPtrVoid& sourceTextOffsetsArr);
	// BEW 10Jul15, next ones needed for conflict resolution dialog, and imposing verse-based advances
    // on the former marker-based advancing in the loop in GetUpdatedText_UsfmsUnchanged() function
	wxString		MakeSourceTextForCollabConflictResDlg();
	bool            DelineateThisVerse(const wxArrayString& md5Arr, int& nStart, int& nEnd, 
		                            wxArrayInt* lineIndicesArr, wxString& chapNumReached);
	bool			IsPTorBEverseEmpty(const wxArrayString& md5Arr, wxArrayInt& lineIndices);
	bool			IsAIverseEmpty(const wxArrayString& md5Arr, wxArrayInt& lineIndices);
	bool			AreTheseTwoTextVersionsDifferent(const wxArrayString& preEditMd5Arr, 
						const wxArrayString& postEditMd5Arr, wxArrayInt& lineIndices);
	void			SetCollabActionDefaults(CollabAction* p);
	// Next one used in GetUpdatedText_UsfmsChanged(), 'chunk' here could be a chunk defined
	// by a group of md5 lines which have a structural change within, or by a verse matchup
	// which has no structural change but potentially may have other verse-internal usfms; we
	// pass in starting and ending indices into the md5 array because we'll have calculated
	// those parameters prior to making these calls; nEnd refers to the last md5 line within
	// the chunk being examined. We use this function for testing postEditMd5Arr or
	// fromEditorMd5Arr since these govern our data transfer protocol
	bool			IsThisChunkEmpty(const wxArrayString& md5Arr, int nStart, int nEnd);

	////////////////// end of those for analysis of texts //////////////////////////


	/////////////////////////////////////////////////////////////////////////////////////
	///   Functions for connecting  the externally obtained data into a new or existing
	///   Adapt It project, and getting data out of the Adapt It document
	/////////////////////////////////////////////////////////////////////////////////////
	bool			CollabProjectIsEditable(wxString projShortName);
	bool			CreateNewAIProject(CAdapt_ItApp* pApp, wxString& srcLangName, 
							wxString& tgtLangName, wxString& srcEthnologueCode, 
							wxString& tgtEthnologueCode, bool bDisableBookMode);
	wxString		ExportTargetText_For_Collab(SPList* pDocList);
	wxString		ExportFreeTransText_For_Collab(SPList* pDocList);
	wxString		GetAIProjectFolderForCollab(wxString& aiProjName, wxString& aiSrcLangName, 
						wxString& aiTgtLangName, wxString editorSrcProjStr, wxString editorTgtProjStr);
	// The next function is created from OnWizardPageChanging() in Projectpage.cpp, and
	// tweaked so as to remove support for the latter's context of a wizard dialog
	bool			HookUpToExistingAIProject(CAdapt_ItApp* pApp, wxString* pProjectName, 
							wxString* pProjectFolderPath);
	bool			IsEthnologueCodeValid(wxString& code);
	bool			CollabProjectFoundInListOfEditorProjects(wxString projName, wxArrayString projList,wxString& composedProjStr); // whm added 19Apr12
	bool			KeepSpaceBeforeEOLforVerseMkr(wxChar* pChar); //BEW added 13Jun11
	// a module for doing the layout and getting the view ready for the user to start
	// adapting;; it is not limited to being used in a Collaboration scenario
	void			SetupLayoutAndView(CAdapt_ItApp* pApp, wxString& docTitle);
	void			UnloadKBs(CAdapt_ItApp* pApp);

	/////////////////////////////////////////////////////////////////////////////////////
	///     Functions for MD5 checksums and their manipulation
	/////////////////////////////////////////////////////////////////////////////////////
	enum			CompareUsfmTexts CompareUsfmTextStructureAndExtent(const wxArrayString& usfmText1, 
							const wxArrayString& usfmText2);
	void			DeleteMD5MapStructs(wxArrayPtrVoid& structsArr);
	wxArrayString	GetUsfmStructureAndExtent(wxString& sourceFileBuffer);
	bool			IsTextOrPunctsChanged(wxString& oldText, wxString& newText);
	// overload of the above, taking arrays and start & finish item indices as parameters
	bool			IsTextOrPunctsChanged(wxArrayString& oldMd5Arr, int oldStart, int oldEnd,
							wxArrayString& newMd5Arr, int newStart, int newEnd);
	bool			IsUsfmStructureChanged(wxString& oldText, wxString& newText);
	void			MapMd5ArrayToItsText(wxString& itsText, wxArrayPtrVoid& mappingsArr, 
							wxArrayString& md5Arr);


	/////////////////////////////////////////////////////////////////////////////////////
	///     Functions for filename creation, path creation, file moving, and data transfer
	///     to the external editor, when doing collaboration
	/////////////////////////////////////////////////////////////////////////////////////
	bool			BookExistsInCollabProject(wxString projCompositeName, wxString bookFullName);
	bool			CollabProjectHasAtLeastOneBook(wxString projCompositeName,wxString collabEditor,wxString ptEditorVersion);
	bool			CollabProjectsAreValid(wxString srcCompositeProjName, wxString tgtCompositeProjName, 
							wxString frtrCompositeProjName, wxString collabEditor, wxString ptEditorVersion,
							wxString& errorStr, wxString& errorProjects);
	wxString		GetPathToRdwrtp7(); // used in GetSourceTextFromEditor::OnInit() and CollabUtilities.cpp
	wxString		GetPathToBeRdwrt(); // used in GetSourceTextFromEditor::OnInit() and CollabUtilities.cpp
	wxString		GetBibleditInstallPath();  // used in GetSourceTextFromEditor::OnInit()
	wxString		GetTextFromAbsolutePathAndRemoveBOM(wxString& absPath, wxString bookCodeForID);
	wxString		GetTextFromFileInFolder(CAdapt_ItApp* pApp, wxString folderPath, wxString& fileTitle);
	wxString		GetTextFromFileInFolder(wxString folderPathAndName); // an override of above function
	wxString		GetShortNameFromProjectName(wxString projName);
	wxString		GetLanguageNameFromProjectName(wxString projName);
	void			GetAILangNamesFromAIProjectNames(const wxString aiProjectName, wxString& sourceLang, wxString& targetLang);
	// The next three are the meaty ones, which together get the updated text back to the
	// external editor, via the .temp folder (child of the work folder) as an
	// intermediatory location for storage of the file holding the data being transferred
	// in or out as the case may be - determined by the lineFor value, either 'reading' or
	// 'writing'
	wxString		MakePathToFileInTempFolder_For_Collab(enum DoFor textKind);
	wxString		BuildCommandLineFor(enum CommandLineFor lineFor, enum DoFor textKind);
	void			TransferTextBetweenAdaptItAndExternalEditor(enum CommandLineFor lineFor, enum DoFor textKind,
							wxArrayString& textIOArray, wxArrayString& errorsIOArray, long& resultCode);
	// Move the newSrc string of just-obtained (from PT or BE) source text, currently in the
	// .temp folder, to the __SOURCE_INPUTS folder, creating the latter folder if it doesn't
	// already exist, and storing in a file with filename constructed from fileTitle plus an
	// added .txt extension; if a file of that name already exists there, overwrite it.
	bool			MoveTextToFolderAndSave(CAdapt_ItApp* pApp, wxString& folderPath, 
							wxString& pathCreationErrors, wxString& theText, wxString& fileTitle,
							bool bAddBOM = FALSE);
	// The following is similar to MoveTextToFolderAndSave() except that it computes
	// internally the path to the .temp folder, and adds the required filename to it (also
	// internally) and so fewer parameters are needed in the signature
	bool			MoveTextToTempFolderAndSave(enum DoFor textKind, wxString& theText, bool bAddBOM = FALSE);
	// The next two move text to or from Bibledit's native storage structures
	bool			CopyTextFromBibleditDataToTempFolder(wxString projectPath, wxString bookName, 
							int chapterNumber, wxString tempFilePathName, wxArrayString& errors);
	bool			CopyTextFromTempFolderToBibleditData(wxString projectPath, wxString bookName, 
							int chapterNumber, wxString tempFilePathName, wxArrayString& errors);
	// Returns the absolute path to the folder being used as the Adapt It work folder, whether
	// in standard location or in a custom location - but for the custom location only
	// provided it is a "locked" custom location (if not locked, then the path to the standard
	// location is returned, i.e. m_workFolderPath, rather than m_customWorkFolderPath). The
	// "lock" condition ensures that a snooper can't set up a PT or BE collaboration and
	// the remote user not being aware of it.
	wxString		SetWorkFolderPath_For_Collaboration();
	wxString		ChangeFilenameExtension(wxString filenameOrPath, wxString extn);
	wxString		RemoveIDMarkerAndCode(wxString textForSending);

	/////////////////////////////////////////////////////////////////////////////////////
	///     Function for cleaning out the files aggregated in .temp folder in AI Unicode Work folder
	/////////////////////////////////////////////////////////////////////////////////////

	// Without this, the .temp folder will fill with potentially thousands of chapter files, or
	// scores of whole-book files, wasting disk space needlessly. Call it from OnExit(). If
	// OnExit() isn't called (eg. an abnormal shutdown) it won't matter, they can be removed
	// at the next normal shutdown
	void	EmptyCollaborationTempFolder();
	long	OK_btn_delayedHandler_GetSourceTextFromEditor(CAdapt_ItApp* pApp);
	void	OnVerseConflictDlg(wxCommandEvent& WXUNUSED(event));


	/////////////////////////////////////////////////////////////////////////////////////
	///     Functions for support of the conflict resolution process
	/////////////////////////////////////////////////////////////////////////////////////
	
	void CollectConflicts(wxArrayPtrVoid& collabActionsArr, wxArrayPtrVoid& conflictsArr); 
	void DestroyConflictResStructs(wxArrayPtrVoid& arr);
	void MeldConflictResolutionsBackIntoActionsArray(wxArrayPtrVoid& collabActionsArr, 
					wxArrayPtrVoid& conflictsArr);
	void MeldConflictsUserCancelBackIntoActionsArray(wxArrayPtrVoid& collabActionsArr, 
					wxArrayPtrVoid& conflictsArr);

#endif

