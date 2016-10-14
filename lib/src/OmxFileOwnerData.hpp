#ifndef OMX_FILE_OWNER_DATA_HPP
#define OMX_FILE_OWNER_DATA_HPP

#include <string>

namespace omx {
struct OmxFileOwnerData {
	hid_t _handle;
	hid_t _dataset;
};
}

#endif