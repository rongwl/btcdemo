#ifndef BTCLITE_BLOB_H
#define BTCLITE_BLOB_H

#include <array>
#include <cstring>
#include <iterator>

#include "utilstrencodings.h"

template <std::size_t size>
using Bytes = std::array<uint8_t, size>;

/** Template base class for fixed-sized opaque blobs. (Little endian) */
template <uint32_t nBITS>
class Blob : public Bytes<nBITS/8>{
public:
	bool IsNull() const
    {
		for (auto it : *this) {
			if (it != 0)
				return false;
		}
		return true;
    }	
	void SetNull()
	{
		std::memset(&this->front(), 0, WIDTH);
	}
	
	//-------------------------------------------------------------------------
	template <typename SType>
	void Serialize(SType& s) const
	{
		s.write(reinterpret_cast<const char*>(&this->front()), this->size());
	}
	template <typename SType>
	void UnSerialize(SType& s)
	{
		s.read(reinterpret_cast<char*>(&this->front()), this->size());
	}
	
	//-------------------------------------------------------------------------
	int Compare(const Blob& b) const
	{
		return std::memcmp(&this->front(), &b.front(), WIDTH);
	}
	
	std::string ToString() const
	{
		return Hex();
	}

    uint64_t Uint64(int pos) const
    {
        auto it = this->begin() + pos * 8;
        return *(it) | 
			   (*(it+1) << 8) | 
               (*(it+2) << 16) | 
               (*(it+3) << 24) | 
               (*(it+4) << 32) | 
               (*(it+5) << 40) | 
               (*(it+6) << 48) | 
               (*(it+7) << 56);
    }
	
	std::string Hex() const
	{
		return HexEncode(this->rbegin(), this->rend());
	}
    void SetHex(const std::string& str)
	{
		SetNull();
		HexDecode(str, this->begin(), this->end());
	}
	
protected:
	Blob()
	{
		SetNull();
	}

	static constexpr int WIDTH = nBITS / 8;
};
/*
template <unsigned int BITS>
void Blob<BITS>::SetHex(const std::string& psz)
{
	SetNull();

	auto first = std::find_if_not(psz.begin(), psz.end(), isspace);
	if (first == psz.end())
		first = psz.begin();
	if (*first == '0' && std::tolower(*(first+1)) == 'x')
		first += 2;
	auto last = std::find_if_not(first, psz.end(), isxdigit);
	std::string substr(first, last);
	
	for (auto it = this->begin(), rit = substr.rbegin(); rit < substr.rend(); ++it, rit += 2) {
		std::string low(rit, rit+1), hi(rit+1, rit+2);
		*it = std::stoi(hi) << 4 | std::stoi(low);
	}
}
*/

// modified from boost.iostreams example
// boost.org/doc/libs/1_55_0/libs/iostreams/doc/tutorial/container_source.html
template <typename Container, typename SinkType, typename CharType>
class ContainerSink {
public:
    typedef CharType char_type;

    ContainerSink(Container& container)
      : container_(container)
    {
        static_assert(sizeof(SinkType) == sizeof(CharType), "invalid size");
    }

    std::streamsize write(const char_type* buffer, std::streamsize size)
    {
        const auto safe_sink = reinterpret_cast<const SinkType*>(buffer);
        container_.insert(container_.end(), safe_sink, safe_sink + size);
        return size;
    }

private:
    Container& container_;
};

template <typename Container>
using ByteSink = ContainerSink<Container, uint8_t, char>;

#endif // BTCLITE_BLOB_H