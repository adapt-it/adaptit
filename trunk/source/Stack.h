/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			stack.h
/// \author			Bruce Waters; modified by Bill Martin for the WX version
/// \date_created	October 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CStack class.
/// Stack, built with a fixed length array of MAX elements (set to 100) 
/// (which should be plenty for the simple XML parsing use to which this
/// stack will be put - the MAX stack depth only needs to be in excess of
/// the maximum nesting level for elements within elements), each element
/// being a c-string of length 31 char or less plus a null byte at the end
/// Note: my CStack class, the XML parser, and my string class CBString
/// will all be single-byte characters. When used with the Uncode supporting
/// app, these single-byte characters will be interpretted as UTF-8, and the
/// callback functions which the XML parser uses will internally convert the
/// byte string to UTF-16; and so the signature of these callbacks has to have
/// a MFC CString so as to accept the UTF-16 for a unicode build. Elsewhere
/// in the XML module, where strings are required, I will use my CBString class.
/// \derivation		The CStack class is not a derived class.
/////////////////////////////////////////////////////////////////////////////

// #pragma once makes the following unnecessary, but no harm leaving it there
#ifndef STACK_H_
#define STACK_H_

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Stack.h"
#endif

typedef char  Item[32]; // we'll store 32 char  arrays, each of
					   // which is an xml element tag

// the following typedefs allow my Palm OS integer types to be used unchanged
typedef short unsigned int UInt16;
typedef unsigned int UInt32;
typedef short int Int16;
typedef int Int32;

/// Stack, built with a fixed length array of MAX elements (set to 100) 
/// (which should be plenty for the simple XML parsing use to which this
/// stack will be put - the MAX stack depth only needs to be in excess of
/// the maximum nesting level for elements within elements), each element
/// being a c-string of length 31 char or less plus a null byte at the end
/// Note: my CStack class, the XML parser, and my string class CBString
/// will all be single-byte characters. When used with the Uncode supporting
/// app, these single-byte characters will be interpretted as UTF-8, and the
/// callback functions which the XML parser uses will internally convert the
/// byte string to UTF-16; and so the signature of these callbacks has to have
/// a MFC CString so as to accept the UTF-16 for a unicode build. Elsewhere
/// in the XML module, where strings are required, I will use my CBString class.
/// \derivation		The CStack class is not a derived class.
class CStack
{
private:
	enum {MAX = 100};
	Item items[MAX];
	UInt16 top; // index of top item in the stack
	
public:
	CStack(); // construct an empty stack
	bool IsEmpty() const; // true if index top is zero
	bool IsFull() const; // true if index top equals MAX
	bool Push(Item item); // push a Char array onto the stack
	bool Pop(Item item); // pop the top element into item
	// whm added below 24May10
	bool Contains(const Item item); // true if item is contained in the stack
	// BEW added - probably in 2010
	bool MyParentsAre(int nThisMany, const Item item_up1, const Item item_up2,
						const Item item_up3);
	// BEW added 28May12
	void GetTop(Item item); // puts the top element's string into item, but 
							// doesn't pop; returns empty char array (all nulls) 
							// if stack is empty
};

#endif
