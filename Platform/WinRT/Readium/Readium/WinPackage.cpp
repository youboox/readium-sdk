//
//  WinPackage.cpp
//  Readium
//
//  Created by Jim Dovey on 2013-09-26.
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

#include "WinPackage.h"
#include "CollectionBridges.h"
#include "WinSpine.h"
#include "WinManifest.h"
#include "WinMediaSupport.h"
#include "WinNavTable.h"
#include "WinContainer.h"
#include "WinCFI.h"
#include "PluginMaps.h"
#include "WinMediaHandler.h"
#include "WinFilterChain.h"
#include "WinSMILModel.h"

#include <iomanip>

BEGIN_READIUM_API

typedef BridgedStringKeyedObjectMapView<ManifestItem^, ::ePub3::ManifestItemPtr>					ManifestMapView;
typedef BridgedStringKeyedObjectMapView<NavigationTable^, ::ePub3::NavigationTablePtr>				NavTableMapView;
typedef BridgedObjectVectorView<ManifestItem^, ::ePub3::ManifestItemPtr>							ManifestList;
typedef BridgedContentHandlerVectorView																ContentHandlerList;
typedef BridgedObjectVectorView<MediaSupportInfo^, ::std::shared_ptr<::ePub3::MediaSupportInfo>>	MediaSupportList;

class winrt_clock
{
public:
	typedef std::chrono::duration<long long, std::milli>	duration;
	typedef duration::rep									rep;
	typedef duration::period								period;
	typedef std::chrono::time_point<winrt_clock>			time_point;
	static const bool is_steady = false;

	static time_point   now()                               _NOEXCEPT;
	static time_t       to_time_t(const time_point& __t)    _NOEXCEPT;
	static time_point   from_time_t(const time_t& __t)      _NOEXCEPT;

private:
#if EPUB_COMPILER_SUPPORTS(CXX_CONSTEXPR)
	static const time_point	unix_epoch = time_point(duration(116444736000000000LL));
#else
	static const time_point unix_epoch() {
		return time_point(duration(116444736000000000LL));
	}
#endif
};

//const winrt_clock::time_point winrt_clock::unix_epoch = time_point(duration(116444736000000000LL));

winrt_clock::time_point winrt_clock::now() _NOEXCEPT
{
	auto calendar = ref new ::Windows::Globalization::Calendar();
	auto datetime = calendar->GetDateTime();
	return time_point(duration(datetime.UniversalTime));
}
time_t winrt_clock::to_time_t(const time_point &__t) _NOEXCEPT
{
	return time_t(std::chrono::duration_cast<std::chrono::seconds>(__t.time_since_epoch() - unix_epoch().time_since_epoch()).count());
}
winrt_clock::time_point winrt_clock::from_time_t(const time_t &__t) _NOEXCEPT
{
	return time_point(duration(__t * std::milli::den - unix_epoch().time_since_epoch().count()));
}

_BRIDGE_API_IMPL_(::ePub3::PackagePtr, Package)

Package::Package(::ePub3::PackagePtr native) : _native(native)
{
	_native->SetBridge(this);
}

String^ Package::BasePath()
{
	auto nativeStr = _native->BasePath();
	DeclareFastPassString(nativeStr, result);
	return result;
}

IMapView<String^, ManifestItem^>^ Package::Manifest::get()
{
	auto nativeMap = _native->Manifest();
	return ref new ManifestMapView(nativeMap);
}
IMapView<String^, NavigationTable^>^ Package::NavigationTables::get()
{
	auto native = _native->NavigationTables();
	return ref new NavTableMapView(native);
}

SpineItem^ Package::FirstSpineItem::get()
{
	return SpineItem::Wrapper(_native->FirstSpineItem());
}

SpineItem^ Package::SpineItemAt(unsigned int idx)
{
	return SpineItem::Wrapper(_native->SpineItemAt(idx));
}
int Package::IndexOfSpineItemWithIDRef(String^ idref)
{
	return static_cast<int>(_native->IndexOfSpineItemWithIDRef(StringToNative(idref)));
}

ManifestItem^ Package::ManifestItemWithID(String^ id)
{
	return ManifestItem::Wrapper(_native->ManifestItemWithID(StringToNative(id)));
}

String^ Package::CFISubpathForManifestItemWithID(String^ id)
{
	return StringFromNative(_native->CFISubpathForManifestItemWithID(StringToNative(id)));
}

IVectorView<ManifestItem^>^ Package::ManifestItemsWithProperties(IVectorView<Uri^>^ iriList)
{
	size_t i = 0;
	std::vector<::ePub3::IRI> nativeList(iriList->Size);
	for (Uri^ uri : iriList)
	{
		nativeList[i++] = URIToIRI(uri);
	}
	return ref new ManifestList(_native->ManifestItemsWithProperties(nativeList));
}

ManifestItem^ Package::ManifestItemForRelativePath(String^ path)
{
	return ManifestItem::Wrapper(std::const_pointer_cast<::ePub3::ManifestItem>(_native->ManifestItemAtRelativePath(StringToNative(path))));
}

NavigationTable^ Package::GetNavigationTable(String^ type)
{
	return NavigationTable::Wrapper(_native->NavigationTable(StringToNative(type)));
}

IClosableStream^ Package::ReadStreamForItemAtPath(String^ path)
{
	return ref new Stream(_native->ReadStreamForItemAtPath(StringToNative(path)));
}

unsigned int Package::SpineCFIIndex::get()
{
	return static_cast<unsigned int>(_native->SpineCFIIndex());
}

Container^ Package::GetContainer()
{
	return Container::Wrapper(_native->GetContainer());
}

String^ Package::UniqueID::get()
{
	return StringFromNative(_native->UniqueID());
}
String^ Package::URLSafeUniqueID::get()
{
	return StringFromNative(_native->URLSafeUniqueID());
}
String^ Package::PackageID::get()
{
	return StringFromNative(_native->PackageID());
}
String^ Package::Type::get()
{
	return StringFromNative(_native->Type());
}
String^ Package::Version::get()
{
	return StringFromNative(_native->Version());
}

void Package::AddMediaHandler(IContentHandler^ handler)
{
	// TODO: implement content handler wrapper
}

SpineItem^ Package::SpineItemWithIDRef(String^ idref)
{
	return SpineItem::Wrapper(_native->SpineItemWithIDRef(StringToNative(idref)));
}

CFI^ Package::CFIForManifestItem(ManifestItem^ item)
{
	auto cfi = _native->CFIForManifestItem(item->NativeObject);
	return CFI::Wrapper(cfi);
}
CFI^ Package::CFIForSpineItem(SpineItem^ item)
{
	auto cfi = _native->CFIForSpineItem(item->NativeObject);
	return CFI::Wrapper(cfi);
}

ManifestItem^ Package::ManifestItemForCFI(CFI^ cfi, CFI^* remainingCFI)
{
	::ePub3::CFI remainder;
	auto item = ManifestItem::Wrapper(_native->ManifestItemForCFI(cfi->NativeObject, &remainder));
	if (remainingCFI != nullptr)
		*remainingCFI = CFI::Wrapper(remainder);
	return item;
}

MediaOverlaysSMILModel^ Package::SMILModel::get()
{
	return MediaOverlaysSMILModel::Wrapper(_native->MediaOverlaysSmilModel());
}

IClosableStream^ Package::ReadStreamForRelativePath(String^ relativePath)
{
	return ref new Stream(_native->ReadStreamForRelativePath(StringToNative(relativePath)));
}

IClosableStream^ Package::ContentStreamFor(SpineItem^ item)
{
	return ref new Stream(_native->ContentStreamForItem(item->NativeObject));
}
IClosableStream^ Package::ContentStreamFor(ManifestItem^ item)
{
	return ref new Stream(_native->ContentStreamForItem(item->NativeObject));
}

IClosableStream^ Package::SyncContentStreamFor(SpineItem^ item)
{
	return ref new Stream(_native->SyncContentStreamForItem(item->NativeObject));
}
IClosableStream^ Package::SyncContentStreamFor(ManifestItem^ item)
{
	return ref new Stream(_native->SyncContentStreamForItem(item->NativeObject));
}

NavigationTable^ Package::TableOfContents::get()
{
	return NavigationTable::Wrapper(_native->TableOfContents());
}
NavigationTable^ Package::ListOfFigures::get()
{
	return NavigationTable::Wrapper(_native->ListOfFigures());
}
NavigationTable^ Package::ListOfIllustrations::get()
{
	return NavigationTable::Wrapper(_native->ListOfIllustrations());
}
NavigationTable^ Package::ListOfTables::get()
{
	return NavigationTable::Wrapper(_native->ListOfTables());
}
NavigationTable^ Package::PageList::get()
{
	return NavigationTable::Wrapper(_native->PageList());
}

String^ Package::Title::get()
{
	return StringFromNative(_native->Title(_returnLocalized));
}
String^ Package::Subtitle::get()
{
	return StringFromNative(_native->Subtitle(_returnLocalized));
}
String^ Package::ShortTitle::get()
{
	return StringFromNative(_native->ShortTitle(_returnLocalized));
}
String^ Package::CollectionTitle::get()
{
	return StringFromNative(_native->CollectionTitle(_returnLocalized));
}
String^ Package::EditionTitle::get()
{
	return StringFromNative(_native->EditionTitle(_returnLocalized));
}
String^ Package::ExpandedTitle::get()
{
	return StringFromNative(_native->ExpandedTitle(_returnLocalized));
}
String^ Package::FullTitle::get()
{
	return StringFromNative(_native->FullTitle(_returnLocalized));
}

IVectorView<String^>^ Package::AuthorNames::get()
{
	return ref new BridgedStringVectorView(_native->AuthorNames(_returnLocalized));
}
IVectorView<String^>^ Package::AttributionNames::get()
{
	return ref new BridgedStringVectorView(_native->AttributionNames(_returnLocalized));
}
String^ Package::Authors::get()
{
	return StringFromNative(_native->Authors(_returnLocalized));
}
IVectorView<String^>^ Package::ContributorNames::get()
{
	return ref new BridgedStringVectorView(_native->ContributorNames(_returnLocalized));
}
String^ Package::Contributors::get()
{
	return StringFromNative(_native->Contributors(_returnLocalized));
}

String^ Package::Language::get()
{
	return StringFromNative(_native->Language());
}
String^ Package::Source::get()
{
	return StringFromNative(_native->Source(_returnLocalized));
}
String^ Package::CopyrightOwner::get()
{
	return StringFromNative(_native->CopyrightOwner(_returnLocalized));
}

DateTime Package::ModificationDate::get()
{
	std::tm tm;
	std::istringstream ss(_native->ModificationDate().stl_str());
	ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
	auto tp = winrt_clock::from_time_t(std::mktime(&tm));

	DateTime result;
	result.UniversalTime = tp.time_since_epoch().count();
	return result;
}

String^ Package::ISBN::get()
{
	return StringFromNative(_native->ISBN());
}

IVectorView<String^>^ Package::Subjects::get()
{
	return ref new BridgedStringVectorView(_native->Subjects(_returnLocalized));
}

PageProgression Package::PageProgressionDirection::get()
{
	return PageProgression(_native->PageProgressionDirection());
}

IVectorView<String^>^ Package::MediaTypesWithDHTMLHandlers()
{
	return ref new BridgedStringVectorView(_native->MediaTypesWithDHTMLHandlers());
}
IVectorView<IContentHandler^>^ Package::HandlersForMediaType(String^ mediaType)
{
	return ref new BridgedContentHandlerVectorView(_native->HandlersForMediaType(StringToNative(mediaType)));
}
MediaHandler^ Package::OPFHandlerForMediaType(String^ mediaType)
{
	return MediaHandler::Wrapper(_native->OPFHandlerForMediaType(StringToNative(mediaType)));
}

IVectorView<String^>^ Package::AllMediaTypes::get()
{
	return ref new BridgedStringVectorView(_native->AllMediaTypes());
}
IVectorView<String^>^ Package::UnsupportedMediaTypes::get()
{
	return ref new BridgedStringVectorView(_native->UnsupportedMediaTypes());
}

IMapView<String^, MediaSupportInfo^>^ Package::MediaSupport::get()
{
	return ref new BridgedStringKeyedObjectMapView<MediaSupportInfo^, ::ePub3::MediaSupportInfoPtr>(_native->MediaSupport());
}
void Package::MediaSupport::set(IMapView<String^, MediaSupportInfo^>^ newValue)
{
	::ePub3::Package::MediaSupportList nativeList;
	auto iterator = newValue->First();
	if (iterator->HasCurrent)
	{
		do
		{
			nativeList.emplace(StringToNative(iterator->Current->Key), iterator->Current->Value->NativeObject);

		} while (iterator->MoveNext());
	}

	_native->SetMediaSupport(nativeList);
}

void Package::SetFilterChain(FilterChain^ chain)
{
	_native->SetFilterChain(chain->NativeObject);
}

#define PropertyHolder Package
#include "PropertyHolderSubclassImpl.h"
#undef PropertyHolder

END_READIUM_API
