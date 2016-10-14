#ifndef OMXLIB_OMX_ATTRIBUTE_COLLECTION_HPP
#define OMXLIB_OMX_ATTRIBUTE_COLLECTION_HPP

#include "OmxPlatform.hpp"
#include "OmxCommon.hpp"

#include <memory>
#include <string>
#include <vector>

namespace omx {
class OmxFile;
class OmxMatrix;
class OmxZonalReference;
struct OmxAttributeOwnerData;


class OMXLib_API OmxAttributeCollection {
public:
	friend OmxFile;
	friend OmxMatrix;
	friend OmxZonalReference;

	OmxAttributeCollection(const OmxAttributeCollection&) = delete;
	OmxAttributeCollection & operator=(const OmxAttributeCollection&) = delete;
	~OmxAttributeCollection();

	std::vector<std::string> getAttributeNames() const;

	bool hasAttribute(const std::string& name) const;
	bool hasAttribute(const std::string& name, OmxDataType dataType) const;
	size_t getAttributeStringLength(const std::string& name) const;
	OmxDataType getAttributeDataType(const std::string& name) const;
	OmxIndex count() const;

    void setAttribute(const std::string& name, OmxInt8 *value);
    void setAttribute(const std::string& name, OmxUInt8 *value);
    void setAttribute(const std::string& name, OmxInt16 *value);
    void setAttribute(const std::string& name, OmxUInt16 *value);
    void setAttribute(const std::string& name, OmxInt32 *value);
    void setAttribute(const std::string& name, OmxUInt32 *value);
    void setAttribute(const std::string& name, OmxInt64 *value);
    void setAttribute(const std::string& name, OmxUInt64 *value);
    void setAttribute(const std::string& name, OmxFloat *value);
    void setAttribute(const std::string& name, OmxDouble *value);
    void setAttribute(const std::string& name, OmxString *value);

	void setAttributeInt8  (const std::string& name, OmxInt8    value);
	void setAttributeUInt8 (const std::string& name, OmxUInt8   value);
	void setAttributeInt16 (const std::string& name, OmxInt16   value);
	void setAttributeUInt16(const std::string& name, OmxUInt16  value);
	void setAttributeInt32 (const std::string& name, OmxInt32   value);
	void setAttributeUInt32(const std::string& name, OmxUInt32  value);
	void setAttributeInt64 (const std::string& name, OmxInt64   value);
	void setAttributeUInt64(const std::string& name, OmxUInt64  value);
	void setAttributeFloat (const std::string& name, OmxFloat   value);
	void setAttributeDouble(const std::string& name, OmxDouble  value);
	void setAttributeString(const std::string& name, OmxString  value);

	// getAttribute methods
    void getAttribute(const std::string& name, OmxInt8 *value) const;
    void getAttribute(const std::string& name, OmxUInt8 *value) const;
    void getAttribute(const std::string& name, OmxInt16 *value) const;
    void getAttribute(const std::string& name, OmxUInt16 *value) const;
    void getAttribute(const std::string& name, OmxInt32 *value) const;
    void getAttribute(const std::string& name, OmxUInt32 *value) const;
    void getAttribute(const std::string& name, OmxInt64 *value) const;
    void getAttribute(const std::string& name, OmxUInt64 *value) const;
    void getAttribute(const std::string& name, OmxFloat *value) const;
    void getAttribute(const std::string& name, OmxDouble *value) const;
    void getAttribute(const std::string& name, OmxString *value) const;

	OmxInt8   getAttributeInt8	(const std::string& name) const;
	OmxUInt8  getAttributeUInt8	(const std::string& name) const;
	OmxInt16  getAttributeInt16	(const std::string& name) const;
	OmxUInt16 getAttributeUInt16(const std::string& name) const;
	OmxInt32  getAttributeInt32	(const std::string& name) const;
	OmxUInt32 getAttributeUInt32(const std::string& name) const;
	OmxInt64  getAttributeInt64	(const std::string& name) const;
	OmxUInt64 getAttributeUInt64(const std::string& name) const;
	OmxFloat  getAttributeFloat	(const std::string& name) const;
	OmxDouble getAttributeDouble(const std::string& name) const;
	OmxString getAttributeString(const std::string& name) const;

	
private:
	OmxAttributeCollection(const OmxAttributeOwnerData *ownerData);
	class OmxAttributeCollectionImpl;
	std::unique_ptr<OmxAttributeCollectionImpl> _impl;
};
}
#endif
