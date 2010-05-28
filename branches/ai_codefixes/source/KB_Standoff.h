/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KB_Standoff.h
/// \author			Bruce Waters
/// \date_created	20 May 2010
/// \date_revised	
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CKB_Standoff class.
/// The CKB_Standoff class partly defines the knowledge base class for the Adapt It data
/// storage structures; it is the in-memory class (a single instance only) which stores
/// metadata for the CKB in-memory class (which also is a single instance). (The app,
/// however, maintains one CKB class for adaptation mode, and one for glossing mode; hence
/// each of these will have a companion CKB_Standoff class instance to store metadata for
/// each knowledge base type.) The CKB class has an array of ten maps. However, since the
/// standoff does not have to have knowledge of how many source text words are involved,
/// the companion CKB_Standoff class has only one map which is a conflation for all ten of
/// the maps in CKB. The map in CKB_Standoff maps wxString keys to CTargetUnit_Standoff*
/// instances. (And the latter store a list of CRefString_Standoff instances.) The metadata
/// is datetime stamping, and a string specifying the source (IP address or host machine
/// name)of the adaptation or gloss when entered into the knowledge base.
/// \derivation		The CKB_Standoff class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef KB_Standoff_h
#define KB_Standoff_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Adapt_ItDoc.h"
#endif

// forward declarations
class CTargetUnit_Standoff;
class CKB_Standoff; // needed for the macro below which must reside outside the class declaration
class CRefString_Standoff;
class CSourcePhrase;
class CTargetUnit;
class CRefString;

// wxHashMap uses this macro for its declaration
WX_DECLARE_HASH_MAP( wxString,		// the map key is an uuid
                    CTargetUnit_Standoff*,	// the map value is the pointer to the class instance
                    wxStringHash,
                    wxStringEqual,
                    MapStrToTgtUnit_Standoff ); // the name of the map class declared by this macro

/// The CKB_Standoff class partly defines the knowledge base class for the Adapt It data
/// storage structures; it is the in-memory class (a single instance only) which stores
/// metadata for the CKB in-memory class (which also is a single instance). (The app,
/// however, maintains one CKB class for adaptation mode, and one for glossing mode; hence
/// each of these will have a companion CKB_Standoff class instance to store metadata for
/// each knowledge base type.) The CKB class has an array of ten maps. However, since the
/// standoff does not have to have knowledge of how many source text words are involved,
/// the companion CKB_Standoff class has only one map which is a conflation for all ten of
/// the maps in CKB. The map in CKB_Standoff maps wxString keys to CTargetUnit_Standoff*
/// instances. (And the latter store a list of CRefString_Standoff instances.) The metadata
/// is datetime stamping, and a string specifying the source (IP address or host machine
/// name)of the adaptation or gloss when entered into the knowledge base.
/// \derivation		The CKB_Standoff class is derived from wxObject.
class CKB_Standoff : public wxObject  
{
	friend class CKB;
	friend class CTargetUnit;
	friend class CRefString;
	friend class CTargetUnit_Standoff;
	friend class CRefString_Standoff;

public:
	CKB_Standoff();

	// BEW 12May10, use this constructor everywhere from 5.3.0 onwards
	CKB_Standoff(bool bGlossingKB, int kbVersion, CKB* pOwningKB); 

	CKB_Standoff(const CKB_Standoff& kb); // copy constructor
	
	void			SetOwningKB(CKB* pOwningKB);

	CKB*			GetOwningKB();

	MapStrToTgtUnit_Standoff*	m_pMap_Standoff; // stores associations of uuid and 
						// ptr to CTargetUnit_Standoff instances where the key 
						// is a string which is an uuid (universal unique ID)

	virtual		~CKB_Standoff();

	// Public implementation functions
private:

	CAdapt_ItApp*	m_pApp;
    // m_bGlossingKB will enable each CKB instantiation to know which kind of CKB class it
    // is, an (adapting) KB or a GlossingKB
	bool			m_bGlossingKB; 
    int				m_kbVersionCurrent; // BEW added 3May10 for Save As... support
	CKB*			m_pOwningKB; // pointer to the owning CKB instance


private:
    DECLARE_DYNAMIC_CLASS(CKB_Standoff)

};
#endif // KB_Standoff_h
