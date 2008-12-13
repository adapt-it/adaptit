/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Adapt_ItDoc.h
/// \author			Bill Martin
/// \date_created	05 January 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CAdapt_ItDoc class. 
/// Adapt It Document (CAdapt_ItDoc) class. 
/// The CAdapt_ItDoc class implements the storage structures and methods 
/// for the Adapt It application's persistent data. Adapt It's document 
/// consists mainly of a list of CSourcePhrases stored in order of occurrence 
/// of source text words. The document's data structures are kept logically 
/// separate from and independent of the view class's in-memory data structures. 
/// This schema is an implementation of the document/view framework. 
/// \derivation		The CAdapt_ItDoc class is derived from wxDocument.
/////////////////////////////////////////////////////////////////////////////

#ifndef Adapt_ItDoc_h
#define Adapt_ItDoc_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Adapt_ItDoc.h"
#endif

#include "SourcePhrase.h" // needed for enum TextType

// Forward declarations
class wxDataOutputStream;
class wxDataInputStream;
class wxFile;
class CAdapt_ItApp;
class CAdapt_ItDoc;


// If we need another list based on CSourcePhrase, we don't declare it
// with another macro like the one above, but instead we simply use 
// the SrcPList class that is declared by the macro to declare 
// additional instance list object memebers within the class we want to
// use them. For example: SrcPList* m_pSaveList;

/// The CAdapt_ItDoc class implements the storage structures and methods 
/// for the Adapt It application's persistent data. Adapt It's document 
/// consists mainly of a list of CSourcePhrases stored in order of occurrence 
/// of source text words. The document's data structures are kept logically 
/// separate from and independent of the view class's in-memory data structures. 
/// This schema is an implementation of the document/view framework. 
/// \derivation		The CAdapt_ItDoc class is derived from wxDocument.
class CAdapt_ItDoc : public wxDocument
{
  protected: // MFC version comment: create from serialization only
	//CAdapt_ItDoc();
	// Note: In the MFC version the Doc's constructor can apparently have
	// "protected" access even with the DECLARE_DYNCREATE(CAdapt_ItDoc) 
	// and IMPLEMENT_DYNCREATE(CAdapt_ItDoc, CDocument) macros attached
	// to the Doc class. Under wxWidgets we must implement serialization 
	// manually, and so cannot have a protected constructor for the document

    DECLARE_DYNAMIC_CLASS(CAdapt_ItDoc)
	// DECLARE_DYNAMIC_CLASS() is used inside a class declaration to 
	// declare that the objects of this class should be dynamically 
	// creatable from run-time type information. 
	// MFC uses DECLARE_DYNCREATE(CAdapt_ItDoc)

  // Attributes
  public:
	CAdapt_ItDoc(); // moved constructor to public access for wxWidgets version (see note above)

	// ////////////////////////////////////////////////////////////////////////////
	// All of the Doc's public data members were moved to the App class' public area
	// SINCE POINTERS TO THE CURRENT DOCUMENT CAN CHANGE, DO NOT PUT DATA MEMBERS
	// HERE, BUT RATHER PLACE THEM IN THE APP CLASS' PUBLIC AREA.
	// ////////////////////////////////////////////////////////////////////////////

	// Operations
public:

	// Overrides
	public:

	void OnFileNew(wxCommandEvent& event);
	virtual bool OnNewDocument();
	//virtual void Serialize(CArchive& ar); // Serialize not used in wxWidgets version
	virtual bool DeleteContents(); // virtual void in MFC and virtual bool in wx but not documented in wx
	virtual bool OnCloseDocument(); // virtual void type in MFC
    //virtual bool OnSaveDocument(const wxString& filename);// OnSaveDocument not used in the MFC version
    virtual bool OnOpenDocument(const wxString& filename);
    virtual bool IsModified() const;
    virtual void Modify(bool mod);

	virtual bool OnSaveModified(); // in protected area of MFC app

	// Implementation
protected:
	void			AddParagraphMarkers(wxString& rText, int& rTextLength);
	bool			AnalyseMarker(CSourcePhrase* pSrcPhrase, CSourcePhrase* pLastSrcPhrase,
									wxChar* pChar, int len, USFMAnalysis* pUsfmAnalysis);
	//void			AppendNull(wxChar* pChar, int length); // unused
	bool			BackupDocument(CAdapt_ItApp* WXUNUSED(pApp));
	//void			CleanOutWork(SPList* pList); // unused
	int				ClearBuffer();
	CBString		ConstructSettingsInfoAsXML(int nTabLevel); // BEW added 05Aug05 for XML doc output support
	int				ContainsMarkerToBeFiltered(enum SfmSet sfmSet, wxString markers, wxString filterList,
						wxString& wholeMkr, wxString& wholeShortMkr, wxString& endMkr, bool& bHasEndmarker,
						bool& bUnknownMarker, int startAt);
	void			CopyFlags(CSourcePhrase* dest, CSourcePhrase* src);
	void			DeleteListContentsOnly(SPList*& pList);
	bool			DoUnpackDocument(wxFile* pFile);
	bool			IsPreviousTextTypeWanted(wxChar* pChar,USFMAnalysis* pAnalysis); // BEW added 06June05 to replace
																					// IsInlineOrFootnote()
	void			GetMarkerMapFromString(MapWholeMkrToFilterStatus*& pMkrMap, wxString str); // used in SetupForSFMSetChange
	wxString		GetNextFilteredMarker(wxString& markers, int offset, int& nStart, int& nEnd);
	//bool			GetPrecedingPunctuation(CSourcePhrase* pSrcPhrase, wxString& charSet, wxString& strStripped); // unused
	bool			IsEndingSrcPhrase(enum SfmSet sfmSet, wxString& markers);
	bool			IsEndMarkerForTextTypeNone(wxChar* pChar);
	bool			IsUnstructuredPlainText(wxString& rText);
	void			MakeOutputBackupFilenames(wxString& curOutputFilename, bool bSaveAsXML);
	bool			NotAtEnd(wxString& rText, const int nTextLength, int nFound);
	void			OverwriteUSFMFixedSpaces(wxString*& pstr);
	void			OverwriteUSFMDiscretionaryLineBreaks(wxString*& pstr);
#ifndef __WXMSW__
#ifndef _UNICODE
	void			OverwriteSmartQuotesWithRegularQuotes(wxString*& pstr);
#endif
#endif
	void			RemoveVenturaOptionalHyphens(wxString*& pstr);

	// ReconstitutDoc was replaced by more robust functions below
	//void			ReconstituteDoc(SPList* pOldList, SPList* pNewList, int nHowMany, int nExtras = 0);
	
	bool			ReconstituteAfterPunctuationChange(CAdapt_ItView* pView, SPList*& pList, 
								SPList::Node* pos, CSourcePhrase*& pSrcPhrase, wxString& fixesStr);
	bool			ReconstituteOneAfterPunctuationChange(CAdapt_ItView* pView, SPList*& pList, SPList::Node* pos, 
								 CSourcePhrase*& pSrcPhrase, wxString& WXUNUSED(fixesStr), SPList*& pNewList, bool bIsOwned);
	bool			ReconstituteAfterFilteringChange(CAdapt_ItView* pView, SPList*& pList, wxString& fixesStr);
	void			SetupForSFMSetChange(enum SfmSet oldSet, enum SfmSet newSet, wxString oldFilterMarkers,
							wxString newFilterMarkers, wxString& secondPassFilterStr, enum WhichPass pass);
	void			SmartDeleteSingleSrcPhrase(CSourcePhrase* pSrcPhrase, SPList* pOtherList);
	void			ConditionallyDeleteSrcPhrase(CSourcePhrase* pSrcPhrase, SPList* pOtherList);

public:
	void			AdjustSequNumbers(int nValueForFirst, SPList* pList);
	wxString&		AppendFilteredItem(wxString& dest,wxString& src);
	wxString&		AppendItem(wxString& dest,wxString& src, const wxChar* ptr, int itemLen);
	void			DeleteSingleSrcPhrase(CSourcePhrase* pSrcPhrase);
	void			DeleteSourcePhrases();
	void			DeleteSourcePhrases(SPList* pList);
	bool			DoFileSave();
	void			DoMarkerHousekeeping(SPList* pNewSrcPhrasesList,int WXUNUSED(nNewCount), 
							TextType& propagationType, bool& bTypePropagationRequired);
	bool			DoTransformedDocFileSave(wxString path);
	//void			EnsureSpaceDelimited(wxString& s); // unused
	void			EraseKB(CKB* pKB);
	bool			FilenameClash(wxString& typedName); // BEW added 22July08 to 
														// prevent 2nd creation destroying work
	CAdapt_ItApp*	GetApp();
	wxString		GetCurrentDirectory();	// BEW added 4Jan07 for saving & restoring the full path
											// to the current doc's directory across the writing of
											// the project configuration file to the project's directory
	//wxString		GetFileName(const wxString fullPath); // not used in wx version
	wxString		GetFilteredItemBracketed(const wxChar* ptr, int itemLen);
	bool			GetNewFile(wxString*& pstrBuffer, wxUint32& nLength, wxString titleID, wxString filter,
					wxString* fileTitle);
	void			GetProjectConfiguration(wxString sourceFolderPath);
	wxString		GetUnFilteredMarkers(wxString& src);
	wxString		GetWholeMarker(wxChar *pChar);
	wxString		GetWholeMarker(wxString str);
	wxString		GetMarkerWithoutBackslash(wxChar *pChar);
	wxString		GetBareMarkerForLookup(wxChar *pChar);
	void			GetMarkersAndTextFromString(wxArrayString* pMkrList, wxString str);
	void			GetUnknownMarkersFromDoc(enum SfmSet useSfmSet,
											wxArrayString* pUnkMarkers,
											wxArrayInt* pUnkMkrsFlags,
											wxString & unkMkrsStr,
											enum SetInitialFilterStatus mkrInitStatus);
	wxString		GetUnknownMarkerStrFromArrays(wxArrayString* pUnkMarkers, wxArrayInt* pUnkMkrsFlags);
	bool			IsEnd(wxChar* pChar);
	bool			IsWhiteSpace(wxChar *pChar);
	int				ParseNumber(wxChar* pChar);
	bool			IsVerseMarker(wxChar* pChar, int& nCount);
	bool			IsFilteredBracketMarker(wxChar *pChar, wxChar* pEnd);
	bool			IsFilteredBracketEndMarker(wxChar *pChar, wxChar* pEnd);
	bool			IsChapterMarker(wxChar* pChar);
	//bool			IsVerseNumber(wxChar *pChar); // unused
	bool			IsAmbiguousQuote(wxChar* pChar);
	bool			IsOpeningQuote(wxChar* pChar);
	bool			IsClosingQuote(wxChar* pChar);
	bool			IsAFilteringSFM(USFMAnalysis* pUsfmAnalysis);
	bool			IsAFilteringUnknownSFM(wxString unkMkr);
	bool			IsMarker(wxChar* pChar, wxChar* pBuffStart);
	bool			IsPrevCharANewline(wxChar* ptr, wxChar* pBuffStart);
	bool			IsEndMarker(wxChar *pChar, wxChar* pEnd);
	bool			IsInLineMarker(wxChar *pChar, wxChar* WXUNUSED(pEnd));
	bool			IsCorresEndMarker(wxString wholeMkr, wxChar *pChar, wxChar* pEnd); // whm added 10Feb05
	static SPList *LoadSourcePhraseListFromFile(wxString FilePath);
	USFMAnalysis*	LookupSFM(wxChar *pChar);
	USFMAnalysis*	LookupSFM(wxString bareMkr);
	bool			MarkerExistsInArrayString(wxArrayString* pUnkMarkers, wxString unkMkr, int& MkrIndex);
	bool			MarkerExistsInString(wxString MarkerStr, wxString wholeMkr, int& markerPos);
	//wxString		MakeFilteredTextIntoPlaceHolders(wxString str); // unused
	wxString		MarkerAtBufPtr(wxChar *pChar, wxChar *pEnd);
	//void			NormalizeToSpaces(wxChar* p, wxChar* WXUNUSED(pEnd)); // unused in wx version
	wxString		NormalizeToSpaces(wxString str); // whm added override of above function for wxString input
	bool			OpenDocumentInAnotherProject(wxString lpszPathName);
	int				ParseMarker(wxChar* pChar);
	int				ParseWhiteSpace(wxChar *pChar);
	int				ParseFilteringSFM(const wxString wholeMkr, wxChar *pChar, 
							wxChar *pBuffStart, wxChar *pEnd);
	int				ParseFilteredMarkerText(const wxString wholeMkr, wxChar *pChar, 
							wxChar *pBuffStart, wxChar *pEnd);
	int				ParseWord(wxChar *pChar, wxString& precedePunct, wxString& followPunct,wxString& SpacelessSrcPunct);
	wxString		RedoNavigationText(CSourcePhrase* pSrcPhrase);
	bool			RemoveMarkerFromBoth(wxString& mkr, wxString& str1, wxString& str2);
	wxString		RemoveAnyFilterBracketsFromString(wxString str);
	//wxString		RemoveAllFilterMaterialFromString(wxString str); // unused
	wxString		RemoveMultipleSpaces(wxString& rString);
	void			ResetUSFMFilterStructs(enum SfmSet useSfmSet, wxString filterMkrs, wxString unfilterMkrs);
	void			ResetUSFMFilterStructs(enum SfmSet useSfmSet, wxString filterMkrs, enum resetMarkers resetMkrs);
	void			RestoreDocParamsOnInput(wxString buffer); // BEW added 08Aug05 to facilitate XML doc input
															  // as well as binary (legacy) doc input & its
															  // output equivalent function is SetupBufferForOutput
	int				RetokenizeText(	bool bChangedPunctuation,
									bool bChangedFiltering, bool bChangedSfmSet);
	void			SetDocumentWindowTitle(wxString title, wxString& nameMinusExtension);
	wxString		SetupBufferForOutput(wxString* pCString);
	//bool			StringsContainSameMarkers(wxString str1, wxString str2); // unused
	int				TokenizeText(int nStartingSequNum, SPList* pList, wxString& rBuffer,int nTextLength);
	void			UpdateFilenamesAndPaths(bool bKBFilename,bool bKBPath,bool bKBBackupPath,
										   bool bGlossingKBPath, bool bGlossingKBBackupPath);
	void			UpdateSequNumbers(int nFirstSequNum);

	void			SetFilename(const wxString& filename, bool notifyViews);

public:
	// Destructor from the MFC version
	virtual ~CAdapt_ItDoc();

protected:

// Generated message map functions (from MFC version)
public:
	void OnFileSave(wxCommandEvent& WXUNUSED(event));
	void OnUpdateFileSave(wxUpdateUIEvent& event);
	void OnFileOpen(wxCommandEvent& WXUNUSED(event));
	void OnFileClose(wxCommandEvent& event);
	void OnUpdateFileClose(wxUpdateUIEvent& event);
	void OnSplitDocument(wxCommandEvent& WXUNUSED(event));
	void OnUpdateSplitDocument(wxUpdateUIEvent& event);
	void OnJoinDocuments(wxCommandEvent& WXUNUSED(event));
	void OnUpdateJoinDocuments(wxUpdateUIEvent& event);
	void OnMoveDocument(wxCommandEvent& WXUNUSED(event));
	void OnUpdateMoveDocument(wxUpdateUIEvent& event);
	void OnFilePackDoc(wxCommandEvent& WXUNUSED(event));
	void OnFileUnpackDoc(wxCommandEvent& WXUNUSED(event));
	void OnUpdateFilePackDoc(wxUpdateUIEvent& event);
	void OnUpdateFileUnpackDoc(wxUpdateUIEvent& event);
	void OnUpdateAdvancedReceiveSynchronizedScrollingMessages(wxUpdateUIEvent& event);
	void OnAdvancedReceiveSynchronizedScrollingMessages(wxCommandEvent& WXUNUSED(event));
	void OnAdvancedSendSynchronizedScrollingMessages(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedSendSynchronizedScrollingMessages(wxUpdateUIEvent& event);

	//virtual void Serialize(CArchive& ar);
	// wxDocument has no virtual Serialize function so we create our own
	// utilizing wxDocument's virtual SaveOjbect() and LoadObject() 
	// Stream methods.
	// We did away with these binary serialization routines when xml i/o was implemented
	//wxOutputStream& SaveObject(wxOutputStream& stream); // = MFC Serialize IsStoring()
	//wxInputStream& LoadObject(wxInputStream& stream); // = MFC Serialize !IsStoring

  private:

	DECLARE_EVENT_TABLE()
};

#endif /* Adapt_ItDoc_h */
