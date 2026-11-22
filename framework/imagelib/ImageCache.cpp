#include "ImageCache.h"
#include "ImageLib.h"
#include "Common.h"
#include "lz4.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

using namespace ImageLib;

namespace {

const char kMagic[4] = {'S', 'F', 'I', 'C'};
const uint32_t kVersion = 1;

struct Entry
{
	int mWidth = 0;
	int mHeight = 0;
	std::shared_ptr<const std::vector<char>> mBuffer;	// the whole file, or this entry's own data
	size_t mOffset = 0;
	size_t mSize = 0;
};

enum class State
{
	NotOpen,
	Open,
	Closed,
};

std::mutex gLock;
State gState = State::NotOpen;
std::map<std::string, Entry> gEntries;
bool gChanged = false;
uint64_t gSignature = 0;
Uint64 gUnpackTicks = 0;
int gHits = 0;

std::string Key(const std::string& thePath, int theAlphaColor)
{
	return thePath + "|" + std::to_string(theAlphaColor);
}

std::string CachePath()
{
	return Sexy::GetAppDataFolder() + "imagecache.bin";
}

// the names in the images folder and the size of resources.xml: a changed set of files rebuilds the cache
uint64_t Signature()
{
	uint64_t h = 1469598103934665603ull;
	auto mix = [&](const std::string& s)
	{
		for (unsigned char c : s)
		{
			h = (h ^ (unsigned char)tolower(c)) * 1099511628211ull;
		}
		h = (h ^ 0xff) * 1099511628211ull;
	};
	std::error_code ec;
	std::filesystem::path aRoot = Sexy::PathFromU8(Sexy::GetResourceFolder());
	std::vector<std::string> aNames;
	for (std::filesystem::directory_iterator it(aRoot / "images", ec), end; !ec && it != end; it.increment(ec))
	{
		aNames.push_back(it->path().filename().string());
	}
	std::sort(aNames.begin(), aNames.end());
	for (const std::string& aName : aNames)
	{
		mix(aName);
	}
	mix(std::to_string(std::filesystem::file_size(aRoot / "properties" / "resources.xml", ec)));
	mix(std::to_string(kVersion));
	return h;
}

template <typename T>
bool Take(const std::vector<char>& theData, size_t& thePos, T& theValue)
{
	if (thePos + sizeof(T) > theData.size())
	{
		return false;
	}
	memcpy(&theValue, theData.data() + thePos, sizeof(T));
	thePos += sizeof(T);
	return true;
}

void Load()
{
	std::ifstream aFile(Sexy::PathFromU8(CachePath()), std::ios::binary | std::ios::ate);
	if (!aFile)
	{
		return;
	}
	std::streamoff aLength = aFile.tellg();
	if (aLength <= 0)
	{
		return;
	}
	auto aBuffer = std::make_shared<std::vector<char>>((size_t)aLength);
	aFile.seekg(0);
	if (!aFile.read(aBuffer->data(), (std::streamsize)aBuffer->size()))
	{
		return;
	}
	const std::vector<char>& d = *aBuffer;
	size_t aPos = 0;
	uint64_t aSignature = 0;
	uint32_t aCount = 0;
	if (d.size() < 16 || memcmp(d.data(), kMagic, 4) != 0)
	{
		return;
	}
	aPos = 4;
	if (!Take(d, aPos, aSignature) || aSignature != gSignature || !Take(d, aPos, aCount))
	{
		SDL_Log("image cache: out of date, rebuilding it");
		return;
	}
	std::map<std::string, Entry> anEntries;
	for (uint32_t i = 0; i < aCount; i++)
	{
		uint16_t aKeyLen;
		int32_t aWidth, aHeight;
		uint32_t aSize;
		if (!Take(d, aPos, aKeyLen) || aPos + aKeyLen > d.size())
		{
			return;
		}
		std::string aKey(d.data() + aPos, aKeyLen);
		aPos += aKeyLen;
		// a damaged file is ignored (and rebuilt), never trusted with an allocation
		if (!Take(d, aPos, aWidth) || !Take(d, aPos, aHeight) || !Take(d, aPos, aSize) || aPos + aSize > d.size() ||
			aWidth <= 0 || aHeight <= 0 || aWidth > 8192 || aHeight > 8192)
		{
			return;
		}
		Entry& e = anEntries[aKey];
		e.mWidth = aWidth;
		e.mHeight = aHeight;
		e.mBuffer = aBuffer;
		e.mOffset = aPos;
		e.mSize = aSize;
		aPos += aSize;
	}
	gEntries.swap(anEntries);
	SDL_Log("image cache: %u images, %.1f MB", aCount, d.size() / 1e6);
}

} // namespace

void ImageLib::OpenImageCache()
{
	gSignature = Signature();
	std::lock_guard<std::mutex> aLock(gLock);
	Load();
	gState = State::Open;
}

Image* ImageLib::FindCachedImage(const std::string& thePath, int theAlphaColor)
{
	Entry e;
	{
		std::lock_guard<std::mutex> aLock(gLock);
		if (gState != State::Open)
		{
			return nullptr;
		}
		auto anItr = gEntries.find(Key(thePath, theAlphaColor));
		if (anItr == gEntries.end())
		{
			return nullptr;
		}
		e = anItr->second;
	}
	Uint64 aStart = SDL_GetPerformanceCounter();
	Image* anImage = new Image();
	anImage->mWidth = e.mWidth;
	anImage->mHeight = e.mHeight;
	anImage->mBits = new uint32_t[(size_t)e.mWidth * e.mHeight];
	int aBytes = e.mWidth * e.mHeight * 4;
	if (LZ4_decompress_safe(e.mBuffer->data() + e.mOffset, (char*)anImage->mBits, (int)e.mSize, aBytes) != aBytes)
	{
		delete anImage;
		return nullptr;
	}
	std::lock_guard<std::mutex> aLock(gLock);
	gUnpackTicks += SDL_GetPerformanceCounter() - aStart;
	gHits++;
	return anImage;
}

void ImageLib::StoreCachedImage(const std::string& thePath, int theAlphaColor, const Image* theImage)
{
	{
		std::lock_guard<std::mutex> aLock(gLock);
		if (gState != State::Open)
		{
			return;
		}
	}
	if (theImage->mWidth <= 0 || theImage->mHeight <= 0 || theImage->mWidth > 8192 || theImage->mHeight > 8192)
	{
		return;
	}
	int aBytes = theImage->mWidth * theImage->mHeight * 4;
	auto aData = std::make_shared<std::vector<char>>(LZ4_compressBound(aBytes));
	int aSize = LZ4_compress_default((const char*)theImage->mBits, aData->data(), aBytes, (int)aData->size());
	if (aSize <= 0)
	{
		return;
	}
	aData->resize(aSize);
	std::lock_guard<std::mutex> aLock(gLock);
	if (gState != State::Open)
	{
		return;
	}
	Entry& e = gEntries[Key(thePath, theAlphaColor)];
	e.mWidth = theImage->mWidth;
	e.mHeight = theImage->mHeight;
	e.mBuffer = aData;
	e.mOffset = 0;
	e.mSize = (size_t)aSize;
	gChanged = true;
}

void ImageLib::CloseImageCache(bool theWrite)
{
	std::lock_guard<std::mutex> aLock(gLock);
	if (gState != State::Open)
	{
		return;
	}
	gState = State::Closed;
	if (gHits > 0)
	{
		SDL_Log("image cache: %d images unpacked in %.2f s", gHits, (double)gUnpackTicks / SDL_GetPerformanceFrequency());
	}
	if (theWrite && gChanged)
	{
		std::string aPath = CachePath();
		std::ofstream aFile(Sexy::PathFromU8(aPath + ".tmp"), std::ios::binary | std::ios::trunc);
		uint32_t aCount = (uint32_t)gEntries.size();
		aFile.write(kMagic, 4);
		aFile.write((const char*)&gSignature, sizeof(gSignature));
		aFile.write((const char*)&aCount, sizeof(aCount));
		size_t aTotal = 0;
		for (const auto& [aKey, e] : gEntries)
		{
			uint16_t aKeyLen = (uint16_t)aKey.size();
			int32_t aWidth = e.mWidth, aHeight = e.mHeight;
			uint32_t aSize = (uint32_t)e.mSize;
			aFile.write((const char*)&aKeyLen, sizeof(aKeyLen));
			aFile.write(aKey.data(), aKeyLen);
			aFile.write((const char*)&aWidth, sizeof(aWidth));
			aFile.write((const char*)&aHeight, sizeof(aHeight));
			aFile.write((const char*)&aSize, sizeof(aSize));
			aFile.write(e.mBuffer->data() + e.mOffset, (std::streamsize)e.mSize);
			aTotal += e.mSize;
		}
		aFile.close();
		if (aFile && Sexy::CommitTempFile(aPath))
		{
			SDL_Log("image cache: wrote %u images, %.1f MB", aCount, aTotal / 1e6);
		}
		else
		{
			std::error_code ec;
			std::filesystem::remove(Sexy::PathFromU8(aPath + ".tmp"), ec);
			SDL_Log("image cache: couldn't write %s", aPath.c_str());
		}
	}
	gEntries.clear();
}
