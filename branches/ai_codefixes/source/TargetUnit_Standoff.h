/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			TargetUnit_Standoff.h
/// \author			Bruce Waters
/// \date_created	20 May 2010
/// \date_revised	
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CTargetUnit_Standoff class. 
/// The CTargetUnit_Standoff class functions as the in-memory storage class for metadata
/// associated with a unique CTargetUnit instance in memory. The two are linked by sharing
/// the same uuid value. Each CTargetUnit_Standoff object stores the linking uuid, the
/// wxString containing the source text word or phrase which is also stored on the
/// matching CTargetUnit instance, and datetime plus 'who created' information for each of
/// the stored CRefString_Standoff instances in the list within the CTargetUnit_Standoff
/// instance.
/// \derivation		The CTargetUnit class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef TargetUnit_Standoff_h
#define TargetUnit_Standoff_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "TargetUnit_Standoff.h"
#endif

// forward references
class CTargetUnit;
class CRefString_Standoff;
class CTargetUnit_Standoff;	// This forward reference is needed for the WX_DECLARE_LIST 
// macro below.

/// wxList declaration and partial implementation of the RefStrMetadataList class being
/// a list of pointers to CRefString_Standoff objects
WX_DECLARE_LIST(CRefString_Standoff, RefStrMetadataList); // see WX_DEFINE_LIST macro in .cpp file
// The RefStrMetadataList stores CRefString_Standoff* instances
// The above macro declares a "safe-pointer" list class using the name of 
// its second parameter here called "RefStrMetadataList". The parameters 
// must reside in the global namespace outside of class declarations 
// and methods.

/// The CTargetUnit_Standoff class functions as the in-memory storage class for metadata
/// associated with a unique CTargetUnit instance in memory. The two are linked by sharing
/// the same uuid value. Each CTargetUnit_Standoff object stores the linking uuid, the
/// wxString containing the source text word or phrase which is also stored on the
/// matching CTargetUnit instance, and datetime plus 'who created' information for each of
/// the stored CRefString_Standoff instances in the list within the CTargetUnit_Standoff
/// instance.
/// \derivation		The CTargetUnit class is derived from wxObject.
class CTargetUnit_Standoff : public wxObject
{
	friend class CRefString_Standoff;
	friend class CTargetUnit;
public:
	// whm Note: The compiler initializes the members in a class declaration 
	// IN THE ORDER they are declared in the class declaration.

	CTargetUnit_Standoff(void); // constructor
	CTargetUnit_Standoff(wxString sourceKey); // a more useful constructor

	//void Copy(const CTargetUnit_Standoff& tu);

	RefStrMetadataList*	m_pRefStrMetadataList; // stores CRefString_Standoff* instances

	CTargetUnit_Standoff(const CTargetUnit_Standoff& tu); // copy constructor -- check it works

	virtual ~CTargetUnit_Standoff(void); // destructor
	// other methods

private:
	wxString	m_uuid; // the primary linking mechanism to CKB contents
	wxString	m_fallbackKey; // used only for fallback linking if the uuid linking were to
                               // fail when the appropriate CTargetUnit_Standoff instance
                               // exists in the map; the link is to be reestablished if
                               // m_fallbackKey matches an m_sourceKey value in a
                               // CTargetUnit instance in the CKB instance

    // getters and setters -- should only need a setter for the uuid, otherwise, no classes
	// which are not friends should ever need to access the private members, and friends
	// can do it directly
public:
	void SetUuid(wxString uuid);

private:
	// class attributes
	DECLARE_DYNAMIC_CLASS(CTargetUnit_Standoff) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information.
};
#endif /* TargetUnit_Standoff_h */