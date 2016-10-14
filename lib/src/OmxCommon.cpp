#include "../include/OmxCommon.hpp"

#include <stdexcept>

namespace omx {

size_t getDataTypeSize(OmxDataType dataType) {

	switch (dataType) {
	case OmxDataType::Int8:		return sizeof(int8_t);
	case OmxDataType::UInt8:	return sizeof(int8_t);
	case OmxDataType::Int16:	return sizeof(int16_t);
	case OmxDataType::UInt16:	return sizeof(int16_t);
	case OmxDataType::Int32:	return sizeof(int32_t);
	case OmxDataType::UInt32:	return sizeof(int32_t);
	case OmxDataType::Int64:	return sizeof(int64_t);
	case OmxDataType::UInt64:	return sizeof(int64_t);
	case OmxDataType::Float:	return sizeof(float);
	case OmxDataType::Double:	return sizeof(double);
	case OmxDataType::String:	return sizeof(char);
	case OmxDataType::Unknown:	throw OmxException("Cannot retrieve size of unknown type."); break;
	}

	throw OmxException("Unable to determine data type.");
}

std::string getVersionString(OmxVersion version) {
	switch (version) {
	case OmxVersion::v0_3_0: return std::string("0.3");
	}

	throw OmxException("Unable to determine version.");
}

}