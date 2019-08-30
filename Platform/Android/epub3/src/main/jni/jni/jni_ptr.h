//
//  jni_ptr.h
//  ePub3
//
//  Created by Pedro Reis Colaco (txtr) on 2013-08-02.
//  Copyright (c) 2014 Readium Foundation and/or its licensees. All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without modification, 
//  are permitted provided that the following conditions are met:
//  1. Redistributions of source code must retain the above copyright notice, this 
//  list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice, 
//  this list of conditions and the following disclaimer in the documentation and/or 
//  other materials provided with the distribution.
//  3. Neither the name of the organization nor the names of its contributors may be 
//  used to endorse or promote products derived from this software without specific 
//  prior written permission.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
//  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
//  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
//  OF THE POSSIBILITY OF SUCH DAMAGE.


#ifndef _JNI_JNIPTR_H_
#define _JNI_JNIPTR_H_


#include <memory>
#include <map>

#include <jni.h>

#include "jni_log.h"


namespace jni {


class PointerPool;


/**
 * Macro for Pointer name so that it keeps a record of file and
 * line where it came from.
 */
#define POINTER_GPS(name) name " [" __FILE__ ":" STRINGIZE(__LINE__) "]"

/**
 * Pointer helper class to ease the manipulation of the Pointer
 * pool. When creating an object of this class it's pointer is
 * added automatically to the pool. It also can recycle a previous
 * object by constructing it with the id. Or release it and the
 * pointer is taken out of the pool to be freed by C++ smart pointer
 * architecture when refcount is 0.
 */
class Pointer {

private:
	/**
	 * Generic void smart pointer
	 */
	typedef std::shared_ptr<void> sptr;

	jlong _id;
	sptr _ptr;
	std::string _name;

public:
	/**
	 * Default constructor (to nullptr).
	 */
	Pointer() : _id(0), _ptr(nullptr), _name("") { }

	/**
	 * Copy constructor.
	 */
	Pointer(const Pointer& o) : _id(o._id), _ptr(o._ptr), _name(o._name) { }

	/**
	 * Move constructor.
	 */
	Pointer(Pointer&& o) : _id(o._id), _ptr(std::move(o._ptr)), _name(std::move(o._name)) { }

	/**
	 * Construct a pointer object and add it to the pointer pool.
	 * This prevents the pointer form being deleted when passed to
	 * Java layer.
	 * The name argument is optional and should be used mainly to
	 * resolve memory leak issues. Just pass to it the macro
	 * POINTER_GPS(name) to have a complete dump of the leaks at
	 * the end of your program execution, or when you call the
	 * PointerPool::dumpLeaks() function.
	 */
	Pointer(sptr ptr, std::string name = "");

	/**
	 * Recall a pointer previously added to the pointer pool.
	 * If the id passed is not valid, this pointer will be pointing
	 * to nullptr.
	 */
	Pointer(jlong id);

	/**
	 * Nothing to be done here. This shouldn't release the pointer
	 * from the pointer pool. For that, an explicit call to the
	 * release() function needs to be done.
	 */
	~Pointer() { }

	/**
	 * Returns true if this pointer is nullptr
	 */
	bool isNull();

	/**
	 * Returns true if this pointer is unique.
	 */
	bool isUnique();

	/**
	 * Returns the current use_count of this pointer.
	 */
	long useCount();

	/**
	 * Gets the id of this pointer.
	 */
	jlong getId();

	/**
	 * Gets a copy of the smart pointer of this pointer.
	 */
	sptr getPtr();

	/**
	 * Gets the name of the smart pointer, if any.
	 */
	std::string getName();

	/**
	 * Releases this pointer from the pointer pool.
	 */
	void release();

	/**
	 * Copy assignment operator.
	 */
	Pointer& operator=(const Pointer& o);

	/**
	 * Move assignment operator.
	 */
	Pointer& operator=(Pointer&& o);

};


/**
 * Pointer pool to hold the C/C++ smart pointers when they are
 * passed to Android Java side through JNI. This prevents them to
 * get deleted in the meanwhile and gives us control of their life
 * cycle. Every pointer added to this pool has to be removed when
 * it is not needed anymore.
 */
class PointerPool {

private:
	/**
	 * Internal static pointer pool.
	 */
	typedef std::map<jlong, Pointer> PointerPoolMap;
	static PointerPoolMap _pool;

public:
	/**
	 * Construct a pointer pool.
	 */
	PointerPool();

	/**
	 * Destruct a pointer pool. Log error if there are still some
	 * pointers left when destructed.
	 */
	~PointerPool();

	/**
	 * Adder.
	 */
	static jlong add(Pointer ptr);

	/**
	 * Getter.
	 */
	static Pointer get(jlong id);

	/**
	 * Deleters.
	 */
	static void del(jlong id);
	static void del(Pointer ptr);

	/**
	 * Dumps the pointers in the pool when needed.
	 */
	static std::string dump();

};


} //namespace JNI


#endif //ifndef _JNI_JNIPTR_H_
