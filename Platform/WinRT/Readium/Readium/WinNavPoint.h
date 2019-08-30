//
//  WinNavPoint.h
//  Readium
//
//  Created by Jim Dovey on 2013-10-11.
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

#ifndef __Readium_NavPoint_h__
#define __Readium_NavPoint_h__

#include "Readium.h"
#include "INavigationElement.h"
#include <ePub3/nav_point.h>

BEGIN_READIUM_API

public ref class NavigationPoint sealed : INavigationElement
{
	_DECLARE_BRIDGE_API_(::ePub3::NavigationPointPtr, NavigationPoint^);

internal:
	NavigationPoint(::ePub3::NavigationPointPtr native);

public:
	virtual ~NavigationPoint() {}

	property String^ Title { virtual String^ get(); virtual void set(String^); }
	property IVectorView<INavigationElement^>^ Children { virtual IVectorView<INavigationElement^>^ get(); }
	virtual void AppendChild(INavigationElement^ child);

	property String^ Content { String^ get(); void set(String^); }

	property INavigationElement^ Parent { INavigationElement^ get(); }

};

END_READIUM_API

#endif	/* __Readium_NavPoint_h__ */
