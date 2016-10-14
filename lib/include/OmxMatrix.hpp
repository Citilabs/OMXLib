#ifndef OMXLIB_OMX_MATRIX_HPP
#define OMXLIB_OMX_MATRIX_HPP

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

class OMXLib_API OmxMatrix {
public:
	friend OmxFile;

	OmxMatrix(const OmxMatrix&) = delete;
	OmxMatrix & operator=(const OmxMatrix&) = delete;
	~OmxMatrix();

	std::string getName() const;

	void writeRow(OmxIndex row, void *rowBuffer);
	void writeRow(OmxIndex row, OmxIndex, void *rowBuffer, OmxDataType dataType);

	void readRow(OmxIndex row, void *rowBuffer);	
	void readRow(OmxIndex row, void *rowBuffer, OmxDataType dataType);

	void* createMatrixRowBuffer() const;
	void* createMatrixBuffer() const;

	OmxCompressionLevel getCompressionLevel() const;

	OmxIndex getZones() const;
	OmxDataType getDataType() const;
	size_t getDataSize() const;

	OmxAttributeCollection& attributes() const;

	void close();

private:
	OmxMatrix(OmxDataType dataType, OmxIndex zones, const std::string& name, OmxCompressionLevel compressionLevel, const OmxFileOwnerData *ownerData);
	class OmxMatrixImpl;
	std::unique_ptr<OmxMatrixImpl> _impl;
};
}
#endif
