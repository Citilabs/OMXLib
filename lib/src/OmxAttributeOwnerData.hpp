#ifndef OMX_ATTRIBUTE_OWNER_DATA_HPP
#define OMX_ATTRIBUTE_OWNER_DATA_HPP

#include <string>

namespace omx {
struct OmxAttributeOwnerData {
	hid_t _handle;
	std::string path;
};
}

#endif