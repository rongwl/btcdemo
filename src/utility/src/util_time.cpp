#include "util_time.h"

#include <cassert>
#include <ctime>
#include <sys/time.h>

#include "constants.h"


namespace btclite {
namespace util {

static TimeOffset& time_offset()
{
    static TimeOffset time_offset;
    return time_offset;
}

int64_t GetTimeSeconds()
{
    std::time_t now = std::time(nullptr);
    assert(now > 0);
    return now;
}

int64_t GetTimeMillis()
{
    struct timeval tv;
    assert(0 == gettimeofday(&tv, NULL));
    return tv.tv_sec*1000 + tv.tv_usec/1000;
}

int64_t GetTimeMicros()
{
    struct timeval tv;
    assert(0 == gettimeofday(&tv, NULL));
    return tv.tv_sec*1000000+tv.tv_usec;
}

int64_t GetAdjustedTime()
{
    return GetTimeSeconds() + time_offset().offset();
}

void AddTimeData(const network::NetAddr& addr, int64_t offset_sample)
{
    // Ignore duplicates
    static std::set<std::string> set_known;
    if (set_known.size() == kMaxTimedataSamples)
        return;
    if (!set_known.insert(addr.ToString()).second)
        return;

    // Add data
    static MedianFilter<int64_t> time_offsets(kMaxTimedataSamples, 0);
    time_offsets.Input(offset_sample);
    BTCLOG(LOG_LEVEL_VERBOSE) << "Added time data, samples " << time_offsets.Size() 
                              << ", offset " << offset_sample;
    
    if (time_offsets.Size() >= 5 && time_offsets.Size() % 2 == 1)
    {
        int64_t median = time_offsets.Median();
        // Only let other nodes change our time by so much
        if (abs(median) <= 70*60)
            time_offset().set_offset(median);
        else
            time_offset().set_offset(0);
        
        BTCLOG(LOG_LEVEL_VERBOSE) << "Set new btc time offset:" 
                                  << time_offset().offset();
    }    
}

int64_t TimeOffset::offset() const
{
    LOCK(cs_);
    return offset_;
}

void TimeOffset::set_offset(int64_t offset)
{
    LOCK(cs_);
    offset_ = offset;
}

} // namespace util
} // namespace btclite
