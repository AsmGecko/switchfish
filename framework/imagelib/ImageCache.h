#ifndef __IMAGECACHE_H__
#define __IMAGECACHE_H__

// switchfish: the decoded images, LZ4 compressed, in one file in the save folder (imagecache.bin), made
// from the player's own files on the first boot and read back on the next ones. On the Switch decoding the
// GIFs was most of the loading time; reading and unpacking this is several times faster. It is never
// shipped. Delete the file to rebuild it; it also rebuilds itself when files are added to or removed from
// the images folder (a file replaced under the same name isn't noticed).

#include <string>

namespace ImageLib
{
	class Image;

	// read the cache (at the start of the loading thread); before this nothing is looked up or stored
	void OpenImageCache();
	// the image from the cache, or nullptr
	Image* FindCachedImage(const std::string& thePath, int theAlphaColor);
	// a freshly decoded image, to be written with the others by CloseImageCache
	void StoreCachedImage(const std::string& thePath, int theAlphaColor, const Image* theImage);
	// writes the cache if anything was added, frees it; lookups stop
	void CloseImageCache(bool theWrite);
}

#endif
