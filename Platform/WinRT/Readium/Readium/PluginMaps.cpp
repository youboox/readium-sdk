//
//  PluginMaps.cpp
//  Readium
//
//  Created by Jim Dovey on 2013-10-02.
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

#include "PluginMaps.h"

#include "WinPackage.h"
#include "WinManifest.h"
#include "IContentHandler.h"
#include "IContentFilter.h"
#include "CollectionBridges.h"
#include "BufferBridge.h"

#include <wrl.h>
#include <wrl/client.h>
#include <wrl/implements.h>

BEGIN_READIUM_API

typedef std::map<::Microsoft::WRL::WeakRef, std::weak_ptr<::ePub3::ContentHandler>>	ContentHandlerMap;
typedef std::map<::Microsoft::WRL::WeakRef, std::weak_ptr<::ePub3::ContentFilter>>	ContentFilterMap;

static ContentHandlerMap*	gContentHandlers;
static ContentFilterMap*	gContentFilters;

static void InitMapStorage()
{
	if (gContentHandlers == nullptr)
		gContentHandlers = new ContentHandlerMap;
	if (gContentFilters == nullptr)
		gContentFilters = new ContentFilterMap;
}

template <class _Tp>
static inline
_Tp^ LoadWeak(::Microsoft::WRL::WeakRef weakRef)
{
	ComPtr<IInspectable> raw(nullptr);
	weakRef.As(&raw);
	return reinterpret_cast<_Tp^>(raw.Get());
}

template <class _Tp>
static inline
WeakRef MakeWeak(_Tp^ obj)
{
	WeakRef ref;
	AsWeak(reinterpret_cast<IInspectable*>(obj), &ref);
	return ref;
}

::ePub3::ContentHandlerPtr GetNativeContentHandler(IContentHandler^ bridge)
{
	WeakRef ref = MakeWeak(bridge);
	InitMapStorage();
	auto pos = gContentHandlers->find(ref);
	if (pos == gContentHandlers->end())
		return nullptr;
	return pos->second.lock();
}
void SetNativeContentHandler(IContentHandler^ bridge, ::ePub3::ContentHandlerPtr native)
{
	InitMapStorage();
	WeakRef ref = MakeWeak(bridge);
	if (bool(native))
	{
		(*gContentHandlers)[ref] = native;
	}
	else
	{
		auto pos = gContentHandlers->find(ref);
		if (pos != gContentHandlers->end())
			gContentHandlers->erase(pos);
	}
}

::ePub3::ContentFilterPtr GetNativeContentFilter(IContentFilter^ bridge)
{
	WeakRef ref = MakeWeak(bridge);
	InitMapStorage();
	auto pos = gContentFilters->find(ref);
	if (pos == gContentFilters->end())
		return nullptr;
	return pos->second.lock();
}
void SetNativeContentFilter(IContentFilter^ bridge, ::ePub3::ContentFilterPtr native)
{
	InitMapStorage();
	WeakRef ref = MakeWeak(bridge);
	if (bool(native))
	{
		(*gContentFilters)[ref] = native;
	}
	else
	{
		auto pos = gContentFilters->find(ref);
		if (pos != gContentFilters->end())
			gContentFilters->erase(pos);
	}
}

class WinRTContentFilterContext : public ::ePub3::FilterContext
{
private:
	::Platform::Object^	_rtObj;

public:
	WinRTContentFilterContext(::Platform::Object^ obj) {
		_rtObj = obj;
	}
	virtual ~WinRTContentFilterContext() {}

	::Platform::Object^ WinRTContextObject() { return _rtObj; }

};

WinRTContentHandler::WinRTContentHandler(IContentHandler^ bridgeInstance) : ::ePub3::ContentHandler(bridgeInstance->Owner->NativeObject, StringToNative(bridgeInstance->MediaType)), __bridge_(bridgeInstance)
{
	SetBridge<IContentHandler>(bridgeInstance);
}
WinRTContentHandler::~WinRTContentHandler()
{
	IContentHandler^ bridge = GetBridge<IContentHandler>();
	SetNativeContentHandler(bridge, nullptr);
}
void WinRTContentHandler::operator()(const ::ePub3::string& src, const ParameterList& parameters)
{
	DeclareFastPassString(src, str);
	auto params = ref new BridgedStringToStringMapView(parameters);
	IContentHandler^ bridge = GetBridge<IContentHandler>();
	if (bridge != nullptr)
		bridge->Invoke(str, params);
}

WinRTContentFilter::WinRTContentFilter(IContentFilter^ bridgeInstance) : ::ePub3::ContentFilter(nullptr), __bridge_(bridgeInstance)
{
	SetTypeSniffer([bridgeInstance](::ePub3::ConstManifestItemPtr item) -> bool {
		ManifestItem^ bridgeItem = item->GetBridge<ManifestItem>();
		return bridgeInstance->TypeSniffer(bridgeItem);
	});
	SetBridge<IContentFilter>(bridgeInstance);
}
WinRTContentFilter::~WinRTContentFilter()
{
	IContentFilter^ filter = GetBridge<IContentFilter>();
	SetNativeContentFilter(filter, nullptr);
}
::ePub3::FilterContext* WinRTContentFilter::MakeFilterContext(::ePub3::ConstManifestItemPtr forItem) const
{
	IContentFilter^ filter = GetBridge<IContentFilter>();
	if (__bridge_ == nullptr)
		return nullptr;
	auto obj = __bridge_->MakeFilterContext(ManifestItem::Wrapper(std::const_pointer_cast<::ePub3::ManifestItem>(forItem)));
	if (obj == nullptr)
		return nullptr;
	return new WinRTContentFilterContext(obj);
}
bool WinRTContentFilter::RequiresCompleteData() const
{
	if (__bridge_ == nullptr)
		return ::ePub3::ContentFilter::RequiresCompleteData();
	return __bridge_->RequiresCompleteData;
}
void* WinRTContentFilter::FilterData(::ePub3::FilterContext* context, void* data, size_t len, size_t* outLen)
{
	::Platform::Object^ realContext = nullptr;
	auto rtContext = dynamic_cast<WinRTContentFilterContext*>(context);
	if (rtContext != nullptr)
		realContext = rtContext->WinRTContextObject();

	auto buf = BridgedBuffer::MakeBuffer((byte*)data, (UINT)len);
	IContentFilter^ filter = GetBridge<IContentFilter>();
	auto outBuf = __bridge_->FilterData(realContext, buf);

	size_t outBufLen = outBuf->Length;
	byte* outBytes = GetIBufferBytes(outBuf);
	if (outBytes != data)
	{
		// copy the bytes out into a new buffer which the callee can deallocate using delete[]
		byte* from = outBytes;
		outBytes = new byte[outBufLen];
		::memcpy_s(outBytes, outBufLen, from, outBufLen);
	}

	*outLen = outBufLen;
	return outBytes;
}

IContentHandler^ ContentHandlerWrapper::Wrapper(::ePub3::ContentHandlerPtr native)
{
	if (!bool(native))
		return nullptr;

	IContentHandler^ result = native->GetBridge<IContentHandler>();
	if (result == nullptr)
		result = ref new ContentHandlerWrapper(native);
	return result;
}
ContentHandlerWrapper::ContentHandlerWrapper(::ePub3::ContentHandlerPtr native) : _native(native)
{
	_native->SetBridge(this);
}
Package^ ContentHandlerWrapper::Owner::get()
{
	return Package::Wrapper(_native->Owner());
}
String^ ContentHandlerWrapper::MediaType::get()
{
	return StringFromNative(_native->MediaType());
}
void ContentHandlerWrapper::Invoke(String^ src, IMapView<String^, String^>^ params)
{
	::ePub3::ContentHandler::ParameterList nparms;
	for (auto pair : params)
	{
		nparms[StringToNative(pair->Key)] = StringToNative(pair->Value);
	}

	_native->operator()(StringToNative(src), nparms);
}

IContentFilter^ ContentFilterWrapper::Wrapper(::ePub3::ContentFilterPtr native)
{
	if (!bool(native))
		return nullptr;

	IContentFilter^ result = native->GetBridge<IContentFilter>();
	if (result == nullptr)
		result = ref new ContentFilterWrapper(native);
	return result;
}
ContentFilterWrapper::ContentFilterWrapper(::ePub3::ContentFilterPtr native) : _native(native)
{
	_native->SetBridge(this);
}
ContentFilterTypeSniffer^ ContentFilterWrapper::TypeSniffer::get()
{
	return ref new ContentFilterTypeSniffer([this](ManifestItem^ item) -> bool {
		return _native->TypeSniffer()(item->NativeObject);
	});
}
bool ContentFilterWrapper::RequiresCompleteData::get()
{
	return _native->RequiresCompleteData();
}
Object^ ContentFilterWrapper::MakeFilterContext(ManifestItem^ forItem)
{
	return ref new FilterContextWrapper(_native->MakeFilterContext(forItem->NativeObject));
}
IBuffer^ ContentFilterWrapper::FilterData(Object^ contextInfo, IBuffer^ inputData)
{
	FilterContextWrapper^ ctx = dynamic_cast<FilterContextWrapper^>(contextInfo);
	::ePub3::FilterContext* ctxPtr = (ctx ? ctx->NativeObject : nullptr);

	byte* inBytes = GetIBufferBytes(inputData);
	size_t inLen = inputData->Length;
	size_t outLen = 0;
	byte* outBytes = reinterpret_cast<byte*>(_native->FilterData(ctxPtr, inBytes, inLen, &outLen));

	IBuffer^ outBuffer = nullptr;
	if (outBytes == inBytes)
	{
		// same buffer, just ensure the length is now correct
		outBuffer = inputData;
		outBuffer->Length = outLen;
	}
	else
	{
		// need to create a wrapper object
		outBuffer = BridgedBuffer::MakeBuffer(outBytes, outLen);
	}

	return outBuffer;
}

END_READIUM_API
