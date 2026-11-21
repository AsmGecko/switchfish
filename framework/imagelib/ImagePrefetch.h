#ifndef __IMAGEPREFETCH_H__
#define __IMAGEPREFETCH_H__

// switchfish: decodes the images of a resource group ahead of the loading thread, on the cores it leaves
// idle. The loading thread still loads everything in the original order; it just finds most images
// already decoded. On the Switch decoding (mostly GIF) was most of the boot.

#include <string>
#include <vector>

namespace ImageLib
{
	class Image;

	struct PrefetchJob
	{
		std::string mPath;
		int mAlphaColor;
	};

	// starts decoding theJobs in order (stops what was running)
	void StartPrefetch(std::vector<PrefetchJob> theJobs);
	// the decoded image for thePath (waits if a worker is on it), or nullptr if it wasn't prefetched
	Image* TakePrefetched(const std::string& thePath);
	// stops the workers and frees whatever wasn't taken
	void StopPrefetch();
}

#endif
