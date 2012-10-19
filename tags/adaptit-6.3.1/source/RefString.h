/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			RefString.h
/// \author			Bill Martin
/// \date_created	21 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CRefString class. 
/// The CRefString class stores the target text adaptation typed
/// by the user for a given source word or phrase. It also keeps
/// track of the number of times this translation was previously
/// chosen.
/// \derivation		The CRefString class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef RefString_h
#define RefString_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "RefString.h"
#endif

// forward references
class CTargetUnit;
class CRefStringMetadata;

/// The CRefString class stores the target text adaptation typed
/// by the user for a given source word or phrase. It also keeps
/// track of the number of times this translation was previously
/// chosen.
/// \derivation		The CRefString class is derived from wxObject.
class CRefString : public wxObject
{
	friend class CKB;
	friend class CRefStringMetadata;
	friend class CTargetUnit;

public:
	CRefString(void); // constructor
	CRefString(CTargetUnit* pTargetUnit);
	CRefString(const CRefString& rs,CTargetUnit* pTargetUnit = NULL);

	// attributes
public:
	wxString		m_translation;
	int				m_refCount; // keep track of how many times it is referenced
								// the AddRef / Release mechanism looks too complex for my needs
	CTargetUnit*	m_pTgtUnit; // is this needed? Yes, currently used in two places to get
								// the owning CTargetUnit instance

	// helpers
	bool			operator==(const CRefString& rs); // equality operator overload

	virtual			~CRefString(void); // destructor // whm make all destructors virtual

	// other methods -- accessors, for doc class and the global functions in xml.cpp
	CRefStringMetadata* GetRefStringMetadata(); // needed so CAdapt_ItDoc::EraseKB() can delete
												// the instance on the heap
	bool			GetDeletedFlag();
	void			SetDeletedFlag(bool bValue);
	void			DeleteRefString(); // deletes both the CRefString instance and its
									   // pointed at CRefStringMetadata instance
protected:

private:
	// class attributes
	bool					m_bDeleted; // & the CRefStringMetadata instance will have the 
										// dateTime for the deletion
	CRefStringMetadata*		m_pRefStringMetadata;

	DECLARE_DYNAMIC_CLASS(CRefString) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CRefString)
};
#endif /* RefString_h */
