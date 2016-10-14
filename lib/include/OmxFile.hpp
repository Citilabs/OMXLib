#ifndef OMXLIB_OMX_FILE_HPP
#define OMXLIB_OMX_FILE_HPP

#include "OmxPlatform.hpp"
#include "OmxCommon.hpp"
#include "OmxAttributeCollection.hpp"

#include <string>
#include <vector>
#include <memory>

namespace omx {

class OmxMatrix;
class OmxZonalReference;

class OMXLib_API OmxFile {
public:
	OmxFile(const std::string& filename);
	OmxFile(const OmxFile&) = delete;
	OmxFile & operator=(const OmxFile&) = delete;
	~OmxFile();

	// opening methods
	void open();
	void openReadOnly();
	void openWithTruncate(OmxIndex zones);
	void openWithCreate(OmxIndex zones);

	// misc methods
	std::string getFilename() const;
	OmxVersion getVersion() const;
	OmxIndex getZones() const;
	OmxCompressionLevel getDefaultCompressionLevel() const;
	size_t getSize() const;

	// matrix methods
	bool isValidMatrixName(const std::string& name) const;
	OmxMatrix& addMatrix();
	OmxMatrix& addMatrix(const std::string& name);
	OmxMatrix& addMatrix(const std::string& name, OmxDataType dataType);
	OmxMatrix& addMatrix(const std::string& name, OmxDataType dataType, OmxCompressionLevel compressionLevel);
	void removeMatrix(const std::string& name);
	void removeMatrix(OmxIndex index);
	std::vector<std::string> getMatrixNames() const; 

	OmxMatrix& getMatrix(OmxIndex index) const;
	OmxMatrix& getMatrix(const std::string& name) const;
	bool matrixNameExists(const std::string& name) const;
	OmxIndex getMatrixCount() const;

	// zonal reference methods
	OmxZonalReference& addZonalReference(const std::string& name, OmxDataType dataType);
	OmxZonalReference& addZonalReference(const std::string& name, OmxDataType dataType, OmxCompressionLevel compressionLevel);
	void removeZonalReference(const std::string& name);
	OmxZonalReference& getZonalReference(const std::string& name) const;
	bool zonalReferenceExists(const std::string& name) const;
	std::vector<std::string> getZonalReferenceNames() const;
	OmxIndex getZonalReferenceCount() const;

	OmxAttributeCollection& attributes() const;

	void vacuum();

	void close();

private:
	class OmxFileImpl;
	std::unique_ptr<OmxFileImpl> _impl;
};
}
#endif