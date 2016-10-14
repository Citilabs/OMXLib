#include "OmxH5Common.hpp"

#include <stdexcept>

uint32_t omx::getH5CompressionLevelFromOmx(OmxCompressionLevel compressionLevel) {
	uint32_t level = 0;

	switch (compressionLevel) {
	case OmxCompressionLevel::NoCompression:	level = 0; break;
	case OmxCompressionLevel::Level_1:			level = 1; break;
	case OmxCompressionLevel::Level_2:			level = 2; break;
	case OmxCompressionLevel::Level_3:			level = 3; break;
	case OmxCompressionLevel::Level_4:			level = 4; break;
	case OmxCompressionLevel::Level_5:			level = 5; break;
	case OmxCompressionLevel::Level_6:			level = 6; break;
	case OmxCompressionLevel::Level_7:			level = 7; break;
	case OmxCompressionLevel::Level_8:			level = 8; break;
	case OmxCompressionLevel::Level_9:			level = 9; break;
	}

	return level;
}

omx::OmxCompressionLevel omx::getOmxCompressionLevelFromH5(uint32_t compressionLevel) {

	switch (compressionLevel) {
	case 0: return OmxCompressionLevel::NoCompression; break;
	case 1: return OmxCompressionLevel::Level_1;		break;
	case 2: return OmxCompressionLevel::Level_2;		break;
	case 3: return OmxCompressionLevel::Level_3;		break;
	case 4: return OmxCompressionLevel::Level_4;		break;
	case 5: return OmxCompressionLevel::Level_5;		break;
	case 6: return OmxCompressionLevel::Level_6;		break;
	case 7: return OmxCompressionLevel::Level_7;		break;
	case 8: return OmxCompressionLevel::Level_8;		break;
	case 9: return OmxCompressionLevel::Level_9;		break;
	default: throw OmxException("Unknown compression level.");
	}
}

hid_t omx::getH5DataType(omx::OmxDataType dataType) {
	hid_t h5Type = -1;

	switch (dataType) {  
	case omx::OmxDataType::Int8:	h5Type = H5T_STD_I8LE;		break;
	case omx::OmxDataType::UInt8:	h5Type = H5T_STD_U8LE;		break;
	case omx::OmxDataType::Int16:	h5Type = H5T_STD_I16LE;		break;
	case omx::OmxDataType::UInt16:	h5Type = H5T_STD_U16LE;		break;
	case omx::OmxDataType::Int32:	h5Type = H5T_STD_I32LE;		break;
	case omx::OmxDataType::UInt32:	h5Type = H5T_STD_U32LE;		break;
	case omx::OmxDataType::Int64:	h5Type = H5T_STD_I64LE;		break;
	case omx::OmxDataType::UInt64:	h5Type = H5T_STD_U64LE;		break;
	case omx::OmxDataType::Float:	h5Type = H5T_IEEE_F32LE;	break;
	case omx::OmxDataType::Double:	h5Type = H5T_IEEE_F64LE;	break;
	case omx::OmxDataType::String:	h5Type = H5T_C_S1;			break;
	case omx::OmxDataType::Unknown:	OmxException("Unknown data type.");
	}

	return h5Type;
}

bool omx::h5TypesEqual(hid_t a, hid_t b) {
	htri_t status = H5Tequal(a, b);

	if (status < 0)
		throw OmxException("An error occurred while examaining data types.");

	return status > 0;
}

omx::OmxDataType omx::getOmxDataType(hid_t dataType) {
	if		(h5TypesEqual(dataType, H5T_STD_I8LE))		return omx::OmxDataType::Int8;
	else if (h5TypesEqual(dataType, H5T_STD_U8LE))		return omx::OmxDataType::UInt8;
	else if (h5TypesEqual(dataType, H5T_STD_I16LE))		return omx::OmxDataType::Int16;
	else if (h5TypesEqual(dataType, H5T_STD_U16LE))		return omx::OmxDataType::UInt16;
	else if (h5TypesEqual(dataType, H5T_STD_I32LE))		return omx::OmxDataType::Int32;
	else if (h5TypesEqual(dataType, H5T_STD_U32LE))		return omx::OmxDataType::UInt32;
	else if (h5TypesEqual(dataType, H5T_STD_I64LE))		return omx::OmxDataType::Int64;
	else if (h5TypesEqual(dataType, H5T_STD_U64LE))		return omx::OmxDataType::UInt64;
	else if (h5TypesEqual(dataType, H5T_IEEE_F32LE))	return omx::OmxDataType::Float;
	else if (h5TypesEqual(dataType, H5T_IEEE_F64LE))	return omx::OmxDataType::Double;
	else if (h5TypesEqual(dataType, H5T_C_S1))			return omx::OmxDataType::String;
	// we deal with variable length strings (for zonal references) separately for now
	else if (H5Tget_class(dataType) == H5T_STRING)		return omx::OmxDataType::String;
	else return OmxDataType::Unknown;
}

hid_t omx::getH5DirectDataType(hid_t dataType) {
	if		(h5TypesEqual(dataType, H5T_STD_I8LE))		return H5T_STD_I8LE;
	else if (h5TypesEqual(dataType, H5T_STD_U8LE))		return H5T_STD_U8LE;
	else if (h5TypesEqual(dataType, H5T_STD_I16LE))		return H5T_STD_I16LE;
	else if (h5TypesEqual(dataType, H5T_STD_U16LE))		return H5T_STD_U16LE;
	else if (h5TypesEqual(dataType, H5T_STD_I32LE))		return H5T_STD_I32LE;
	else if (h5TypesEqual(dataType, H5T_STD_U32LE))		return H5T_STD_U32LE;
	else if (h5TypesEqual(dataType, H5T_STD_I64LE))		return H5T_STD_I64LE;
	else if (h5TypesEqual(dataType, H5T_STD_U64LE))		return H5T_STD_U64LE;
	else if (h5TypesEqual(dataType, H5T_IEEE_F32LE))	return H5T_IEEE_F32LE;
	else if (h5TypesEqual(dataType, H5T_IEEE_F64LE))	return H5T_IEEE_F64LE;
	else if (h5TypesEqual(dataType, H5T_C_S1))			return H5T_C_S1;
	// we deal with variable length strings (for zonal references) separately for now
	else if (H5Tget_class(dataType) == H5T_STRING)		return H5T_C_S1;
	
	throw OmxException("Invalid data type detected.");
}
