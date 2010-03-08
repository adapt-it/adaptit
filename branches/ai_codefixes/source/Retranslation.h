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

#ifdef	_RETRANS

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
		
	// Utility functions
	CLayout*		CRetranslation::GetLayout();
	CAdapt_ItView*	CRetranslation::GetView();
	CAdapt_ItApp*	CRetranslation::GetApp();
		
	// Items from Adapt_ItView
	bool		DoFindRetranslation(int nStartSequNum, int& nSequNum, int& nCount);
	void		DoRetranslation();
	void		DoRetranslationByUpArrow();
	void		GetSelectedSourcePhraseInstances(SPList*& pList,wxString& strSource,wxString& strAdapt);
	void		NewRetranslation();
	bool		IsContainedByRetranslation(int nFirstSequNum, int nCount, int& nSequNumFirst,
										   int& nSequNumLast);
	bool		IsNullSrcPhraseInSelection(SPList* pList);
	bool		IsRetranslationInSelection(SPList* pList);
	void		RemoveNullSourcePhrase(CPile* pInsertLocPile, const int nCount);

protected:
	void		AccumulateText(SPList* pList,wxString& strSource,wxString& strAdapt);
	void		BuildRetranslationSourcePhraseInstances(SPList* pRetransList,int nStartSequNum,
														int nNewLength,int nCount,int& nFinish);
	void		ClearSublistKBEntries(SPList* pSublist);
	void		CopySourcePhraseList(SPList*& pList,SPList*& pCopiedList,bool bDoDeepCopy = FALSE); // BEW modified 16Apr08
	void		DeleteSavedSrcPhraseSublist(SPList* pSaveList); // this list's members can have members in sublists 
	void		DoOneDocReport(wxString& name, SPList* pList, wxFile* pFile);
	void		DoRetranslationReport(CAdapt_ItApp* pApp, CAdapt_ItDoc* pDoc, wxString& name,
									  wxArrayString* pFileList,SPList* pList, wxFile* pFile);
	void		GetRetranslationSourcePhrasesStartingAnywhere(CPile* pStartingPile,
															  CPile*& pFirstPile,SPList* pList);
	void		InsertSublistAfter(SPList* pSrcPhrases, SPList* pSublist, int nLocationSequNum);
	bool		IsConstantType(SPList* pList);
	bool		IsEndInCurrentSelection();
	void		PadWithNullSourcePhrasesAtEnd(CAdapt_ItDoc* pDoc,CAdapt_ItApp* pApp,
											  SPList* pSrcPhrases,int nEndSequNum,int nNewLength,int nCount);
	void		ReplaceMatchedSubstring(wxString strSearch, wxString& strReplace, wxString& strAdapt);
	void		RemoveNullSrcPhraseFromLists(SPList*& pList,SPList*& pSrcPhrases,int& nCount,
											 int& nEndSequNum,bool bActiveLocAfterSelection,int& nSaveActiveSequNum);
	void		RemoveUnwantedSourcePhraseInstancesInRestoredList(SPList* pSrcPhrases,int nCurCount,
																  int nStartingSequNum,SPList* pSublist);
	void		RestoreOriginalPunctuation(CSourcePhrase* pSrcPhrase);
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
	// TODO: these are public, but don't appear to be called from outside the class. Do we still need them public?
	void OnButtonEditRetranslation(wxCommandEvent& event);
	void OnButtonRetranslation(wxCommandEvent& event); // whm moved to public in wx version
	void OnUpdateButtonRetranslation(wxUpdateUIEvent& event); // whm moved to public in wx version
	
private:
	CAdapt_ItApp*	m_pApp;	// The app owns this
	CLayout*		m_pLayout;
	CAdapt_ItView*	m_pView;
		
	DECLARE_EVENT_TABLE()
};

#endif // _RETRANS

#endif // RETRANS_H