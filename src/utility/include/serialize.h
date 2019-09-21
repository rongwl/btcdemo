#ifndef BTCLITE_SERIALIZE_H
#define BTCLITE_SERIALIZE_H

#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>

#include "constants.h"
#include "Endian.h"


size_t VarIntSize(size_t vec_size);

inline uint64_t DoubleToBinary(double d)
{
    union {
        double d;
        uint64_t i;
    } tmp;
    tmp.d = d;
    return tmp.i;
}

inline uint32_t FloatToBinary(float f)
{
    union {
        float f;
        uint32_t i;
    } tmp;
    tmp.f = f;
    return tmp.i;
}

inline double BinaryToDouble(uint64_t i)
{
    union {
        double d;
        uint64_t i;
    } tmp;
    tmp.i = i;
    return tmp.d;
}

inline float BinaryToFloat(uint32_t i)
{
    union {
        float f;
        uint32_t i;
    } tmp;
    tmp.i = i;
    return tmp.f;
}


template <typename Stream>
class Serializer {
public:
    Serializer(Stream& s)
        : stream_(s) {}
    
    // interface for serialize
    template <typename T>
    void SerialWrite(const T& obj)
    {
        Serialize(obj);
    }
    // interface for unserialize
    template <typename T>
    void SerialRead(T *obj)
    {
        Unserialize(obj);
    }
    
private:
    Stream& stream_;
    
    //-------------------------------------------------------------------------
    // for double type
    void Serialize(const double& in) 
    {
        SerWriteData(DoubleToBinary(in));
    }
    
    // for float type
    void Serialize(const float& in)
    {
        SerWriteData(FloatToBinary(in));
    }
    
    //for char type array
    template <size_t N>
    void Serialize(const std::array<char, N>& in)
    {
        stream_.write(reinterpret_cast<const char*>(in.data()), N);
    }
    
    // for arithmetic type vector
    template <typename T>
    std::enable_if_t<std::is_arithmetic<T>::value> Serialize(const std::vector<T>& in)
    {
        SerWriteVarInt(in.size());
        if (!in.empty())
            stream_.write(reinterpret_cast<const char*>(in.data()), in.size()*sizeof(T));
    }
    
    // for class type vector
    template <typename T>
    std::enable_if_t<std::is_class<T>::value> Serialize(const std::vector<T>& in)
    {
        SerWriteVarInt(in.size());
        for (auto it = in.begin(); it != in.end(); it++)
            Serialize(*it);
    }
    
    // for integral type
    template <typename T>
    std::enable_if_t<std::is_integral<T>::value> Serialize(const T& in) 
    {
        SerWriteData(in);
    }
    
    // default to calling member function 
    template <typename T>
    std::enable_if_t<std::is_class<T>::value> Serialize(const T& obj) 
    {
        obj.Serialize(stream_);
    }
    
    // for double type
    void UnSerialize(double *out) 
    {
        uint64_t i;
        SerReadData(&i);
        *out = BinaryToDouble(i);        
    }
    
    // for float type
    void UnSerialize(float *out) 
    {
        uint32_t i;
        SerReadData(&i);
        *out = BinaryToFloat(i);
    }
    
    // for char type array
    template <size_t N>
    void UnSerialize(std::array<char, N> *out)
    {
        stream_.read(reinterpret_cast<char*>(out->data()), N);
    }
    
    // for arithmetic type vector
    template <typename T> 
    std::enable_if_t<std::is_arithmetic<T>::value> UnSerialize(std::vector<T>*); 
    
    // for class type vector
    template <typename T> 
    std::enable_if_t<std::is_class<T>::value> UnSerialize(std::vector<T>*); 
    
    // for integral type
    template <typename T>
    std::enable_if_t<std::is_integral<T>::value> UnSerialize(T *out) 
    {
        SerReadData(out);
    }
    
    // default to calling member function
    template <typename T>
    std::enable_if_t<std::is_class<T>::value> UnSerialize(T *obj) 
    {
        obj->UnSerialize(stream_);
    }
    
    // serialize variable length integer
    //-------------------------------------------------------------------------
    void SerWriteVarInt(const uint64_t);
    uint64_t SerReadVarInt();
    
    // Lowest-level serialization and conversion.
    //-------------------------------------------------------------------------
    template <typename T> void SerWriteData(const T&);
    template <typename T> void SerReadData(T*);        
};

template <typename Stream>
template <typename T>
std::enable_if_t<std::is_arithmetic<T>::value> Serializer<Stream>::UnSerialize(std::vector<T> *out)
{
    // Limit size per read so bogus size value won't cause out of memory
    uint64_t count = SerReadVarInt();
    if (count*sizeof(T) > kMaxBlockSize)
        throw std::ios_base::failure("vector size larger than max block size");
    out->clear();
    out->resize(count);
    stream_.read(reinterpret_cast<char*>(out->data()), count*sizeof(T));
}

template <typename Stream>
template <typename T> 
std::enable_if_t<std::is_class<T>::value> Serializer<Stream>::UnSerialize(std::vector<T> *out)
{
    uint64_t count = SerReadVarInt();
    size_t size = 0;    
    out->clear();
    out->resize(count);
    for (uint64_t i = 0; i < count; i++) {
        UnSerialize(&(out->at(i)));
        size += out->at(i).Size(true);
        if (size > kMaxBlockSize)
            throw std::ios_base::failure("vector size larger than max block size");
    }
}

template <typename Stream>
void Serializer<Stream>::SerWriteVarInt(const uint64_t count)
{
    if (count < kVarint16bits) {
        SerWriteData(static_cast<uint8_t>(count));
    }
    else if (count <= std::numeric_limits<uint16_t>::max()) {
        SerWriteData(kVarint16bits);
        SerWriteData(static_cast<uint16_t>(count));
    }
    else if (count <= std::numeric_limits<uint32_t>::max()) {
        SerWriteData(kVarint32bits);
        SerWriteData(static_cast<uint32_t>(count));
    }
    else {
        SerWriteData(kVarint64bits);
        SerWriteData(count);
    }
}

template <typename Stream>
uint64_t Serializer<Stream>::SerReadVarInt()
{
    uint8_t count;
    uint64_t varint;
    
    SerReadData(&count);
    if (count < kVarint16bits) {
        varint = count;
    }
    else if (count == kVarint16bits) {
        SerReadData(reinterpret_cast<uint16_t*>(&varint));
        if (varint < kVarint16bits)
            throw std::ios_base::failure("non-canonical SerReadVarInt()");
    }
    else if (count == kVarint32bits) {
        SerReadData(reinterpret_cast<uint32_t*>(&varint));
        if (varint <= std::numeric_limits<uint16_t>::max())
            throw std::ios_base::failure("non-canonical SerReadVarInt()");
    }
    else {
        SerReadData(&varint);
        if (varint <= std::numeric_limits<uint32_t>::max())
            throw std::ios_base::failure("non-canonical SerReadVarInt()");
    }
    if (varint > kMaxVardataSize)
        throw std::ios_base::failure("SerReadVarInt(): size too large");
    
    return varint;
}

template <typename Stream>
template <typename T>
void Serializer<Stream>::SerWriteData(const T& obj)
{
    Bytes<sizeof(T)> data;
    ToLittleEndian(obj, &data);
    stream_.write(reinterpret_cast<const char*>(&data[0]), sizeof data);
}

template <typename Stream>
template <typename T>
void Serializer<Stream>::SerReadData(T *obj)
{
    Bytes<sizeof(T)> data;
    stream_.read(reinterpret_cast<char*>(&data[0]), sizeof data);
    FromLittleEndian(data.begin(), data.end(), obj);
}


#endif // BTCLITE_SERIALIZE_H
