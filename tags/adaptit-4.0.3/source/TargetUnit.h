/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			TargetUnit.h
/// \author			Bill Martin
/// \date_created	12 February 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CTargetUnit class. 
/// The CTargetUnit class functions as a repository of translations. Each
/// CTargetUnit object stores all the known translations for a given
/// source text word or phrase.
/// \derivation		The CTargetUnit class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef TargetUnit_h
#define TargetUnit_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "TargetUnit.h"
#endif

// forward references
class wxDataOutputStream;
class wxDataInputStream;

class CTargetUnit;	// This forward reference is needed for the WX_DECLARE_LIST 
// macro below. Without it the compiler will generate at least 6 cryptic error messages!
// wxList uses this macro for its declaration

/// wxList declaration and partial implementation of the TranslationsList class being
/// a list of pointers to CRefString objects
WX_DECLARE_LIST(CRefString, TranslationsList); // see WX_DEFINE_LIST macro in .cpp file
// The TranslationsList stores CRefString* instances
// The above macro declares a "safe-pointer" list class using the name of 
// its second parameter here called "TranslationsList". The parameters 
// must reside in the global namespace outside of class declarations 
// and methods.

/// The CTargetUnit class functions as a repository of translations. Each
/// CTargetUnit object stores all the known translations for a given
/// source text word or phrase.
/// \derivation		The CTargetUnit class is derived from wxObject.
class CTargetUnit : public wxObject
{
public:
	// whm Note: I've changed the ordering of the members of CTargetUnit from what they
	// were in the MFC version. The compiler initializes the members in a class declaration 
	// IN THE ORDER they are declared in the class declaration. This is an attempt to correct 
	// problems that I've had (and possibly Bruce too) encountering NULL pointers.

	CTargetUnit(void); // constructor
	void Copy(const CTargetUnit& tu);

	// attributes
	bool				m_bAlwaysAsk; // TRUE when more than one translation, can be set TRUE by the user
								  // even when there is only one translation currently
	TranslationsList*	m_pTranslations; // list of all translations of a key, stores CRefString* instances

	CTargetUnit(const CTargetUnit& tu); // MFC note: copy constructor -- it doesn't work, 
	// see .cpp file for reason // whm moved it here after declaration of m_pTranslations

	//wxOutputStream& SaveObject(wxOutputStream& stream);
	//wxInputStream& LoadObject(wxInputStream& stream);

	virtual ~CTargetUnit(void); // destructor // whm make all destructors virtual
	// other methods

//public:
// MFC"s Serialize replaced by LoadObject() and SaveObject() above
//	virtual void	Serialize(CArchive& ar);


protected:

private:
	// class attributes
	DECLARE_DYNAMIC_CLASS(CTargetUnit) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CTargetUnit)
};
#endif /* TargetUnit_h */
