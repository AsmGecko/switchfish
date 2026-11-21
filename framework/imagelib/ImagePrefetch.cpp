#include "ImagePrefetch.h"
#include "ImageLib.h"

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>

#include "Common.h"

#ifdef __SWITCH__
#include <switch.h>
#endif

using namespace ImageLib;

namespace {

// how far the workers may run ahead of the loading thread (decoded images waiting in memory)
const size_t kAhead = 8;

struct Slot
{
	bool mDone = false;
	Image* mImage = nullptr;
};

std::mutex gLock;
std::condition_variable gChanged;
std::vector<PrefetchJob> gJobs;
std::map<std::string, size_t> gIndexOf;
std::vector<Slot> gSlots;
size_t gNext = 0;		// next job a worker takes
size_t gTaken = 0;		// jobs the loading thread is past
bool gStopping = false;
std::vector<std::thread> gWorkers;

void Worker(int theCore)
{
#ifdef __SWITCH__
	// cores 0 (the main thread only draws the loading screen) and 2; the loading thread has 1, the system 3
	// below the main thread (0x2C), so on core 0 it never holds up a frame
	if (R_FAILED(svcSetThreadCoreMask(CUR_THREAD_HANDLE, theCore, BIT(theCore))) ||
		R_FAILED(svcSetThreadPriority(CUR_THREAD_HANDLE, 0x2D)))
	{
		SDL_Log("image prefetch: couldn't move a worker to core %d", theCore);
	}
#else
	(void)theCore;
#endif
	std::unique_lock<std::mutex> aLock(gLock);
	for (;;)
	{
		gChanged.wait(aLock, [] { return gStopping || (gNext < gJobs.size() && gNext < gTaken + kAhead); });
		if (gStopping)
		{
			return;
		}
		size_t i = gNext++;
		if (gSlots[i].mDone)
		{
			continue;
		}
		PrefetchJob aJob = gJobs[i];
		aLock.unlock();
		Image* anImage = GetImage(aJob.mPath, true, aJob.mAlphaColor);
		aLock.lock();
		gSlots[i].mImage = anImage;
		gSlots[i].mDone = true;
		gChanged.notify_all();
	}
}

} // namespace

void ImageLib::StartPrefetch(std::vector<PrefetchJob> theJobs)
{
	StopPrefetch();
	std::lock_guard<std::mutex> aLock(gLock);
	gJobs.clear();
	gIndexOf.clear();
	// one job per file: two resources on the same file share the image
	for (PrefetchJob& aJob : theJobs)
	{
		if (gIndexOf.emplace(aJob.mPath, gJobs.size()).second)
		{
			gJobs.push_back(std::move(aJob));
		}
	}
	gSlots.assign(gJobs.size(), Slot());
	gNext = 0;
	gTaken = 0;
	gStopping = false;
	if (gJobs.empty())
	{
		return;
	}
	gWorkers.emplace_back(Worker, 0);
	gWorkers.emplace_back(Worker, 2);
}

Image* ImageLib::TakePrefetched(const std::string& thePath)
{
	std::unique_lock<std::mutex> aLock(gLock);
	auto anItr = gIndexOf.find(thePath);
	if (anItr == gIndexOf.end() || gStopping)
	{
		return nullptr;
	}
	size_t i = anItr->second;
	gIndexOf.erase(anItr);
	if (i + 1 > gTaken)
	{
		gTaken = i + 1;
		gChanged.notify_all();
	}
	// no worker has started it (they're behind): the loading thread decodes it itself, and the workers
	// carry on after it
	if (i >= gNext)
	{
		gSlots[i].mDone = true;
		return nullptr;
	}
	gChanged.wait(aLock, [i] { return gStopping || gSlots[i].mDone; });
	if (gStopping)
	{
		return nullptr;
	}
	Image* anImage = gSlots[i].mImage;
	gSlots[i].mImage = nullptr;
	return anImage;
}

void ImageLib::StopPrefetch()
{
	// gStopping stays set until the next start, so a waiting TakePrefetched gives up instead of reading
	// slots that are gone
	std::vector<std::thread> aWorkers;
	{
		std::lock_guard<std::mutex> aLock(gLock);
		gStopping = true;
		aWorkers.swap(gWorkers);
		gChanged.notify_all();
	}
	for (std::thread& aWorker : aWorkers)
	{
		aWorker.join();
	}
	std::lock_guard<std::mutex> aLock(gLock);
	for (Slot& aSlot : gSlots)
	{
		delete aSlot.mImage;
		aSlot.mImage = nullptr;
	}
	gSlots.clear();
	gJobs.clear();
	gIndexOf.clear();
}
