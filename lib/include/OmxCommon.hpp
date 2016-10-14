#ifndef OMXLIB_OMX_COMMON_HPP
#define OMXLIB_OMX_COMMON_HPP

#include "OmxPlatform.hpp"
#include "OmxException.hpp"

#include <string>
#include <cstdint>


namespace omx {
enum class OMXLib_API OmxVersion {
	v0_3_0
};

enum class OMXLib_API OmxDataType {
	Int8, UInt8, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double, String, Unknown
};

size_t OMXLib_API getDataTypeSize(OmxDataType dataType);

std::string OMXLib_API getVersionString(OmxVersion version);

typedef int8_t OmxInt8;
typedef uint8_t OmxUInt8;

typedef int16_t OmxInt16;
typedef uint16_t OmxUInt16;

typedef int32_t OmxInt32;
typedef uint32_t OmxUInt32;

typedef int64_t OmxInt64;
typedef uint64_t OmxUInt64;

typedef float OmxFloat;
typedef double OmxDouble;

typedef std::string OmxString;

typedef uint64_t OmxIndex;

enum class OMXLib_API OmxCompressionLevel {
	NoCompression, Level_1, Level_2, Level_3, Level_4, Level_5, Level_6, Level_7, Level_8, Level_9
};

}
#endif