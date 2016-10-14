#ifndef OMX_ZONAL_REFERENCE_HPP
#define OMX_ZONAL_REFERENCE_HPP

#include "OmxPlatform.hpp"
#include "OmxCommon.hpp"
#include "OmxAttributeCollection.hpp"

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdint>

namespace omx {
class OmxFile;
struct OmxFileOwnerData;

class OMXLib_API OmxZonalReference {
public:
	friend OmxFile;

	OmxZonalReference(const OmxZonalReference&) = delete;
	OmxZonalReference & operator=(const OmxZonalReference&) = delete;
	~OmxZonalReference();

	std::string getName() const;
	OmxCompressionLevel getCompressionLevel() const;
	OmxDataType getDataType() const;
	OmxIndex getZones() const;
	size_t getDataSize() const;
	void readReference(void *buffer) const;
	std::vector<std::string> readStringReference() const;
	void writeReference(const void *buffer);
	void writeStringReference(const std::vector<std::string> &values);
	void* createReferenceBuffer() const;

	OmxAttributeCollection& attributes() const;
private:
	OmxZonalReference(OmxDataType dataType, OmxIndex zones, const std::string& name, OmxCompressionLevel compressionLevel, const OmxFileOwnerData *ownerData);
	class OmxZonalReferenceImpl;
	std::unique_ptr<OmxZonalReferenceImpl> _impl;
};
}

#endif
