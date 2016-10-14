#ifndef OMX_H5_COMMON_H
#define OMX_H5_COMMON_H

#include "../include/OmxCommon.hpp"

#include <hdf5.h>
#include <hdf5_hl.h>



namespace omx {

uint32_t getH5CompressionLevelFromOmx(OmxCompressionLevel compressionLevel);

OmxCompressionLevel getOmxCompressionLevelFromH5(uint32_t compressionLevel);

hid_t getH5DataType(OmxDataType dataType);

bool h5TypesEqual(hid_t a, hid_t b);

OmxDataType getOmxDataType(hid_t dataType);

hid_t getH5DirectDataType(hid_t dataType);

}
#endif