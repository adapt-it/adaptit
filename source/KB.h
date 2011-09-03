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
    #pragma interface "Adapt_ItDoc.h"
#endif

// forward declarations
class CTargetUnit;
class wxDataOutputStream;
class wxDataInputStream;

class TUList;	// This forward reference is needed because the macro 
// declaration below must be in general namespace, before CKB is declared.
// The macro below together with the macro list declaration in the .cpp file
// define a new list class called TUList. Its list elements are of type CKB.
class CKB; // needed for the macro below which must reside outside the class declaration

/// wxList declaration and partial implementation of the TUList class being
/// a list of pointers to CTargetUnit objects
WX_DECLARE_LIST(CTargetUnit, TUList); // see WX_DEFINE_LIST macro in the .cpp file

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
    DECLARE_DYNAMIC_CLASS(CKB)
	
	friend bool GetGlossingKBFlag(CKB* pKB);  // or the public accessor, IsThisAGlossingKB()
											  // can be used instead of this friend function
public:
	CKB();
	// CKB(const CKB& kb); copy constructor - doesn't work, because TUList can't be guaranteed to have
	// been defined before it gets used. Compiles fine actually, but m_pTargetUnits list pointer is
	// garbage when the app runs. As a work around, use a function approach, defining a Copy() function.
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
	// line to insure that it gets initialized first. See if it works properly in the wxWidgets
	// version and avoids the same problem Bruce encountered in the MFC version. cf similar notes
	// for CTargetUnit.

// implementation
public:
	// whm Note: I've changed the ordering of the members of CKB from what they
	// were in the MFC version. I've put the declaration of TUList before MapKeyStringToTgtUnit
	// since the compiler initializes the members in a class declaration IN THE ORDER they
	// are declared in the class declaration. This is an attempt to correct problems that
	// I've had (and possibly Bruce too) encountering NULL or bad m_pTargetUnits and m_pMap 
	// pointers.

	// The names of the languages need to be stored, because the startup wizard's <Back
	// button allows the user to create a project, then backtrack and choose an existing 
	// project. Doing this, if we did not store the language names in the KB, would would 
	// not easily be able to recover the existing project's language names, and the wrong 
	// names would then be profiled out on exit of the app.
	wxString			m_sourceLanguageName;
	wxString			m_targetLanguageName;

	int				m_nMaxWords; // current number of words in max length of src phrase

	TUList*			m_pTargetUnits; // stores translation equivalents for each source phrase

	MapKeyStringToTgtUnit*	m_pMap[10]; // stores associations of key and ptr to CTargetUnit instances
									   // where the key is a phrase with [index + 1] source words

	virtual ~CKB();
	
	// accessors for private members
	int				GetCurrentKBVersion();
	void			SetCurrentKBVersion(int kb_version);

private:
	CAdapt_ItApp*	m_pApp;
    // m_bGlossingKB will enable each CKB instantiation to know which kind of CKB class it
    // is, an (adapting) KB or a GlossingKB
	bool			m_bGlossingKB; // TRUE for a glossing KB, FALSE for an adapting KB
    int				m_kbVersionCurrent; // BEW added 3May10 for Save As... support


// Serialization
public:
	//virtual void	Serialize(CArchive& ar);
	// Serialize is replaced by LoadObject() and SaveObject() in the wxWidgets version
	//wxOutputStream& SaveObject(wxOutputStream& stream);
	//wxInputStream& LoadObject(wxInputStream& stream);

};
#endif // KB_h
