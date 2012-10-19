/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Retranslation.h
/// \author			Erik Brommers
/// \date_created	10 March 2010
/// \date_revised	10 March 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the header file for the CRetranslation class. 
/// The CRetranslation class contains the retranslations-related methods
/// and event handlers that were in the CAdapt_ItView class.
/// \derivation		The CRetranslation class is derived from wxEvtHandler.
/////////////////////////////////////////////////////////////////////////////

#ifndef RETRANS_H
#define RETRANS_H

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "Retranslation.h"
#endif

//////////////////////////////////////////////////////////////////////////////////
/// The CRetranslation class contains retranslation-related functionality 
/// originally found in the CAdapt_ItView class.
/// \derivation		The CRetranslation class is derived from wxEvtHandler.
/////////////////////////////////////////////////////////////////////////////
class CRetranslation : public wxEvtHandler
{
	//friend class CLayout; //BEW I don't think we need to make friends with CLayout
public:
		
	CRetranslation(); // default constructor
	CRetranslation(CAdapt_ItApp* app); // use this one
		
	virtual ~CRetranslation();	// destructor
		
	// Items from Adapt_ItView
	bool		DoFindRetranslation(int nStartSequNum, int& nSequNum, int& nCount);
	void		DoRetranslation();
	void		DoRetranslationByUpArrow();
	void		GetSelectedSourcePhraseInstances(SPList*& pList,wxString& strSource,wxString& strAdapt);
	
	// getters/setters for deglobalified globals
	inline bool GetIsRetranslationCurrent() {return m_bIsRetranslationCurrent; }
	inline void SetIsRetranslationCurrent(bool bNewValue) {m_bIsRetranslationCurrent = bNewValue; }
	inline bool GetIsInsertingWithinFootnote() {return m_bInsertingWithinFootnote; }
	inline void SetIsInsertingWithinFootnote(bool bNewValue) {m_bInsertingWithinFootnote = bNewValue; }
	inline bool GetSuppressRemovalOfRefString() {return m_bSuppressRemovalOfRefString; }
	inline void SetSuppressRemovalOfRefString(bool bNewValue) {m_bSuppressRemovalOfRefString = bNewValue; }
	inline bool GetReplaceInTranslation() {return m_bReplaceInRetranslation; }
	inline void SetReplaceInTranslation(bool bNewValue) {m_bReplaceInRetranslation = bNewValue; }

	void		RemoveRetranslation(SPList* pList, int first, int last, wxString& oldAdaptation); // remove 
					// the retranslation which goes from indices first to last, from a passed in list
					// of CSourcePhrase instances (BEW added 11May11). Return its old adaptation too

	/* BEW deprecated 9Mar11
	// a function for use when converting a sublist of (modified) CSourcePhrase instances
	// generated when the user has changed punctuation settings, to a retranslation -
	// because the conversion of a merger resulted in more than MAX_WORDS instances, and
	// the only way to preserve the (modified) target text is to store it as a
	// retranslation (called only by ReconstituteAfterPunctuationChange() - in two places)
	void		ConvertSublistToARetranslation(SPList* pList, wxString& tgtText, wxString& gloss);
	*/
protected:
	void		AccumulateText(SPList* pList,wxString& strSource,wxString& strAdapt);
	void		BuildRetranslationSourcePhraseInstances(SPList* pRetransList,int nStartSequNum,
														int nNewLength,int nCount,int& nFinish);
	void		ClearSublistKBEntries(SPList* pSublist);
	void		CopySourcePhraseList(SPList*& pList,SPList*& pCopiedList,bool bDoDeepCopy = FALSE); // BEW modified 16Apr08
	void		DeleteSavedSrcPhraseSublist(SPList* pSaveList); // this list's members can have members in sublists 
	void		DoOneDocReport(wxString& name, SPList* pList, wxFile* pFile);
	void		DoRetranslationReport(CAdapt_ItDoc* pDoc, wxString& name,
									  wxArrayString* pFileList,SPList* pList, wxFile* pFile,
									  const wxString& progressTitle);
	void		GetRetranslationSourcePhrasesStartingAnywhere(CPile* pStartingPile,
															  CPile*& pFirstPile,SPList* pList);
	void		InsertSublistAfter(SPList* pSrcPhrases, SPList* pSublist, int nLocationSequNum);
	bool		IsConstantType(SPList* pList);
	bool		IsEndInCurrentSelection();
	void		PadWithNullSourcePhrasesAtEnd(CAdapt_ItDoc* pDoc, SPList* pSrcPhrases,int nEndSequNum,int nNewLength,int nCount);
	void		ReplaceMatchedSubstring(wxString strSearch, wxString& strReplace, wxString& strAdapt);
	void		RemoveUnwantedSourcePhraseInstancesInRestoredList(SPList* pSrcPhrases,int nCurCount,
																  int nStartingSequNum,SPList* pSublist);
	//void		RestoreOriginalPunctuation(CSourcePhrase* pSrcPhrase); // BEW deprecated 12May11
	void		RestoreTargetBoxText(CSourcePhrase* pSrcPhrase,wxString& str);
	void		SetNotInKBFlag(SPList* pList,bool bValue = TRUE);
	void		SetRetranslationFlag(SPList* pList,bool bValue = TRUE);
	void		UnmergeMergersInSublist(SPList*& pList,SPList*& pSrcPhrases,int& nCount,
										int& nEndSequNum,bool bActiveLocAfterSelection,int& nSaveActiveSequNum,
										bool bWantRetranslationFlagSet = TRUE,bool bAlsoUpdateSublist = FALSE);
	
protected: 
	// event handlers
	void OnRemoveRetranslation(wxCommandEvent& event);
	void OnUpdateRemoveRetranslation(wxUpdateUIEvent& event);
	void OnUpdateButtonEditRetranslation(wxUpdateUIEvent& event);
	void OnRetransReport(wxCommandEvent& WXUNUSED(event));
	void OnUpdateRetransReport(wxUpdateUIEvent& event);

public: 
	// BEW answer: Bill made them public as they are called by some access functions, in
	// the view -- eg. DoRetranslation(), and DoRetranslationByUpArrow(), call
	// OnButtonRetranslation(). So there will be some others... But those functions could/should
	// be make public accessors within the Retranslation class itself
	void OnButtonEditRetranslation(wxCommandEvent& event);
	void OnButtonRetranslation(wxCommandEvent& event); // whm moved to public in wx version
	void OnUpdateButtonRetranslation(wxUpdateUIEvent& event); // whm moved to public in wx version
	
private:
	CAdapt_ItApp*	m_pApp;	// The app owns this
	CLayout*		m_pLayout;
	CAdapt_ItView*	m_pView;
	
	// deglobalified globals
	bool m_bIsRetranslationCurrent;		// set TRUE when retranslating or editing same, used to 
										// suppress removing of KB entries during edit or creation
										// of the retranslation; default FALSE at other times
	bool m_bInsertingWithinFootnote;	// TRUE if inserting a null sourcephrase
										// within a footnote; eg. if a retranslation is within a
										// footnote and gets padded with null sourcephrases.
										// We need to know this so that we can propagate the
										// footnote TextType to the padding, or for any other insertion
										// within a footnote.
	bool m_bSuppressRemovalOfRefString;	// set TRUE in SetActivePilePointerSafely,
										// otherwise nested PlacePhraseBox call will result in a RemoveRefString
										// spurious call before the phrasebox is rebuilt, which could remove a
										// source to target translation association wrongly.	
	bool m_bReplaceInRetranslation;		// default FALSE, set TRUE only if a Find & Replace match has been
										// made and a replacement is being done within a
										// retranslation, used only once, in OnButtonEditRetranslation()
	
	DECLARE_EVENT_TABLE()
};

#endif // RETRANS_H