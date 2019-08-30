//
//  WinFilterManager.h
//  Readium
//
//  Created by Jim Dovey on 2013-10-30.
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

#include "WinFilterManager.h"
#include "WinPackage.h"
#include "WinManifest.h"
#include "PluginMaps.h"
#include <Windows.h>
#include <thread>

BEGIN_READIUM_API

IContentFilter^ FilterManager::GetFilterByName(String^ name, Package^ package)
{
	::ePub3::ContentFilterPtr native = ::ePub3::FilterManager::Instance()->GetFilterByName(StringToNative(name), package->NativeObject);
	if (!bool(native))
		return nullptr;

	auto wrapper = std::dynamic_pointer_cast<WinRTContentFilter>(native);
	if (bool(wrapper))
		return wrapper->GetBridge<IContentFilter>();

	return ContentFilterWrapper::Wrapper(native);
}

void FilterManager::RegisterFilter(String^ name, FilterPriority priority, ContentFilterFactory^ factory)
{
	auto instance = ::ePub3::FilterManager::Instance();
	instance->RegisterFilter(StringToNative(name), ::ePub3::ContentFilter::FilterPriority(priority), [factory](::ePub3::ConstPackagePtr pkg) -> std::shared_ptr<WinRTContentFilter> {
		auto rtPkg = Package::Wrapper(std::const_pointer_cast<::ePub3::Package>(pkg));
		IContentFilter^ filter = factory(rtPkg);
		if (filter == nullptr)
			return nullptr;
		return std::make_shared<WinRTContentFilter>(filter);
	});
}

FilterChain^ FilterManager::BuildFilterChain(Package^ package)
{
	return FilterChain::Wrapper(::ePub3::FilterManager::Instance()->BuildFilterChainForPackage(package->NativeObject));
}

END_READIUM_API
