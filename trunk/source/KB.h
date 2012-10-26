/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KB.h
/// \author			Bill Martin
/// \date_created	21 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CKB class.
/// The CKB class defines the knowledge base class for the App's 
/// data storage structures. The CKB class is completely independent of 
/// the list of CSourcePhrase instances stored on the App. This independence 
/// allows modifying the latter, or any of these CKB structures, independently 
/// of the other with no side effects. Links between the document structures and the 
/// CKB structures are established dynamically by the lookup engine on a 
/// "need to know" basis.
/// \derivation		The CKB class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef KB_h
#define KB_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KB.h"
#endif

// forward declarations
class CTargetUnit;
class CKB; // needed for the macro below which must reside outside the class declaration
class CRefString;

enum UseForLookup
{
     useGlossOrAdaptationForLookup,
     useTargetPhraseForLookup
};

enum KB_Entry {
	absent,
	present_but_deleted,
	really_present
};

// BEW removed 29May10, as TUList is redundant * now removed
// wxList declaration and partial implementation of the TUList class being
// a list of pointers to CTargetUnit objects
//WX_DECLARE_LIST(CTargetUnit, TUList); // see WX_DEFINE_LIST macro in the .cpp file

// wxHashMap uses this macro for its declaration
WX_DECLARE_HASH_MAP( wxString,		// the map key is the source text word or phrase string
                    CTargetUnit*,	// the map value is the pointer to the class instance
                    wxStringHash,
                    wxStringEqual,
                    MapKeyStringToTgtUnit ); // the name of the map class declared by this macro pMap

/// The CKB class defines the knowledge base class for the App's 
/// data storage structures. The CKB class is completely independent of 
/// the list of CSourcePhrase instances stored on the App. This independence 
/// allows modifying the latter, or any of these CKB structures, independently 
/// of the other with no side effects. Links between the document structures and the 
/// CKB structures are established dynamically by the lookup engine on a 
/// "need to know" basis.
/// \derivation		The CKB class is derived from wxObject.
class CKB : public wxObject  
{
	friend class CTargetUnit;
	friend class CRefString;
	friend class CRefStringMetadata;
	friend bool GetGlossingKBFlag(CKB* pKB);  // or the public accessor, IsThisAGlossingKB()
											  // can be used instead of this friend function
public:
	CKB();
	CKB(bool bGlossingKB); // BEW 12May10, use this constructor everywhere from 5.3.0 onwards
	// CKB(const CKB& kb); copy constructor - doesn't work, because TUList can't be guaranteed to have
	// been defined before it gets used. Compiles fine actually, but m_pTargetUnits list pointer is
	// garbage when the app runs. As a work around, use a function approach, defining a Copy() function.
	void Copy(const CKB& kb); // this separates the copy code from creation of the object's owned lists
	// Note: Could the problem be the result of the ordering of the members in this CKB class 
	// declaration below??? According to B. Milewski (p. 17) "Data members
	// [including embedded objects] are inititalized in the order in which they appear in the class 
	// definition...Embedded objects are destroyed in the reverse order of their creation." Hence 
	// the compiler will initialize the MapKeyStringToTgtUnit, before TUList.
	// TODO: Implement the commented out copy constructor, change the ordering of the class members
	// in the declaration below, putting TUList* m_pTargetUnits before the MapKeyStringToTgtUnit* m_pMap[10]
	// line to ensure that it gets initialized first. See if it works properly in the wxWidgets
	// version and avoids the same problem Bruce encountered in the MFC version. cf similar notes
	// for CTargetUnit.

// implementation
public:
	// whm Note: I've changed the ordering of the members of CKB from what they
	// were in the MFC version. I've put the declaration of TUList before MapKeyStringToTgtUnit
	// since the compiler initializes the members in a class declaration IN THE ORDER they
	// are declared in the class declaration. This is an attempt to correct problems that
	// I've had (and possibly Bruce too) encountering NULL or bad m_pTargetUnits and m_pMap 
	// pointers. BEW  note 29May10 -- this is no longer relevant, as TUList is removed now

	// The names of the languages need to be stored, because the startup wizard's <Back
	// button allows the user to create a project, then backtrack and choose an existing 
	// project. Doing this, if we did not store the language names in the KB, would would 
	// not easily be able to recover the existing project's language names, and the wrong 
	// names would then be profiled out on exit of the app.
	wxString			m_sourceLanguageName;
	wxString			m_targetLanguageName;

	int				m_nMaxWords; // current number of words in max length of src phrase

	MapKeyStringToTgtUnit*	m_pMap[MAX_WORDS]; // stores associations of key and ptr to CTargetUnit instances
									   // where the key is a phrase with [index + 1] source words

	virtual ~CKB();

	// Public implementation functions
	bool			AutoCapsLookup(MapKeyStringToTgtUnit* pMap,CTargetUnit*& pTU,wxString keyStr);
	wxString		AutoCapsMakeStorageString(wxString str, bool bIsSrc = TRUE);
	void			DoKBExport(wxFile* pFile, enum KBExportSaveAsType kbExportSaveAsType);
	void			DoKBImport(wxString pathName,enum KBImportFileOfType kbImportFileOfType);
	void			DoKBSaveAsXML(wxFile& f, const wxString& progressTitle, int nTotal);
	void			DoNotInKB(CSourcePhrase* pSrcPhrase, bool bChoice = TRUE);
	bool			FindMatchInKB(int numWords, wxString srcPhrase, CTargetUnit*& pTargetUnit);
	void			Fix_NotInKB_WronglyEditedOut(CPile* pCurPile); // BEW added 24Mar09, to simplify MoveToNextPile()
	void			GetAndRemoveRefString(CSourcePhrase* pSrcPhrase,
								wxString& targetPhrase, enum UseForLookup useThis); // BEW created 11May10
	void			GetForceAskList(KPlusCList* pKeys);

	//CRefString*	    GetRefString(int nSrcWords, wxString keyStr, wxString valueStr);
	enum KB_Entry	GetRefString(int nSrcWords, wxString keyStr, wxString valueStr, CRefString*& pRefStr);

	//CRefString*		GetRefString(CTargetUnit* pTU, wxString valueStr);
					// an overload useful for LIFT imports
	enum KB_Entry	GetRefString(CTargetUnit* pTU, wxString valueStr, CRefString*& pRefStr);

	CTargetUnit*	GetTargetUnit(int nSrcWords, wxString keyStr);
	bool			IsAlreadyInKB(int nWords, wxString key, wxString adaptation);
	// overloaded version below, for use when Consistency Check is being done
	bool			IsAlreadyInKB(int nWords, wxString key, wxString adaptation, 
						CTargetUnit*& pTU, CRefString*& pRefStr, bool& bDeleted);
	bool			IsItNotInKB(CSourcePhrase* pSrcPhrase);
	bool			IsNot_In_KB_inThisTargetUnit(CTargetUnit* pTU);
	bool			IsDeleted_Not_In_KB_inThisTargetUnit(CTargetUnit* pTU);
	bool			IsThisAGlossingKB(); // accessor for private bool m_bGlossingKB
	CBString		MakeKBElementXML(wxString& src,CTargetUnit* pTU,int nTabLevel);
	void			RedoStorage(CSourcePhrase* pSrcPhrase, wxString& errorStr); // BEW 15Nov10 moved from view
	void			RemoveRefString(CRefString* pRefString, CSourcePhrase* pSrcPhrase, int nWordsInPhrase);
	void			RestoreForceAskSettings(KPlusCList* pKeys);
	bool			StoreText(CSourcePhrase* pSrcPhrase, wxString& tgtPhrase, bool bSupportNoAdaptationButton = FALSE);
	bool			StoreTextGoingBack(CSourcePhrase *pSrcPhrase, wxString &tgtPhrase);
	void			DoKBRestore(int& nCount, int& nCumulativeTotal);
	bool			UndeleteNormalEntryAndDeleteNotInKB(CSourcePhrase* pSrcPhrase, CTargetUnit* pTU, wxString& str);

	// accessors for private members
	int				GetCurrentKBVersion();
	void			SetCurrentKBVersion();

	// KbServer support
	// Note: the pointers to the KbServer instances are stored in the app class; this is
	// deliberate, because CKB operations sometimes are done manually, or automatically,
	// quite often and may involve loading and unloading of CKB instances - and we want to
	// keep such actions from forcing unwanted KbServer instances being created and
	// destroyed on the fly at such times
#if defined (_KBSERVER)

	// use next for phrasebox typed adaptations or glosses, and for KBEditor's Add button
	bool			HandleNewPairCreated(int kbServerType, wxString srcKey, wxString translation);
	// use the next for phrasebox typed adaptation or gloss which is to be a normal entry
	// when the local KB has the same pair as a pseudo-deleted entry
	bool			HandleUndelete(int kbServerType, wxString srcKey, wxString translation);
	// use the next when in the KB Editor for the local KB, the user selects an adaptation
	// or gloss and clicks the Remove button to pseudo-delete it
	bool			HandlePseudoDelete(int kbServerType, wxString srcKey, wxString translation);
	// Use the next when in the KB Editor for the local KB, the user corrects an
	// incorrectly spelled adaptation or gloss. Internally this is implemented as a
	// pseudo-delete of the old incorrectly spelled entry, together with creation of a new
	// entry with the corrected src/tgt or src/gloss pair. Hence, the kbserver support can
	// simply do HandlePseudoDelete() using the old pair, followed by
	// HandleNewPairCreated() using the new src/tgt or src/gloss pair.
	bool			HandlePseudoDeleteAndNewPair(int kbServerType, wxString srcKey, 
									wxString oldTranslation, wxString newTranslation);
														  





#endif // for _KBSERVER

  private:

	CAdapt_ItApp*	m_pApp;

    // m_bGlossingKB will enable each CKB instantiation to know which kind of CKB class it
    // is, an (adapting) KB or a GlossingKB
	bool			m_bGlossingKB; // TRUE for a glossing KB, FALSE for an adapting KB
    int				m_kbVersionCurrent; // BEW added 3May10 for Save As... support

	//CRefString*		AutoCapsFindRefString(CTargetUnit* pTgtUnit,wxString adaptation);
	enum KB_Entry	AutoCapsFindRefString(CTargetUnit* pTgtUnit,wxString adaptation, CRefString*& pRefStr);
	//int			CountSourceWords(wxString& rStr); // use helpers.cpp TrimAndCountWordsInString() instead
	bool			IsMember(wxString& rLine, wxString& rMarker, int& rOffset);

    DECLARE_DYNAMIC_CLASS(CKB)
};
#endif // KB_h
