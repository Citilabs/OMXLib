#include "../include/OmxAttributeCollection.hpp"

#include "../include/OmxCommon.hpp"
#include "OmxH5Common.hpp"
#include "H5Scoped.hpp"
#include "OmxAttributeOwnerData.hpp"

#include <string>
#include <functional>
#include <cstring>

#include <hdf5.h>
#include <hdf5_hl.h>

namespace omx {

class OmxAttributeCollection::OmxAttributeCollectionImpl {
public:
	OmxAttributeCollectionImpl(const OmxAttributeOwnerData *ownerData) : _handle( ownerData->_handle ), _path( ownerData->path ) {

	}


	~OmxAttributeCollectionImpl() {

	}

	void removeAttribute(const std::string& name) {
		if (H5Adelete_by_name(_handle, _path.c_str(), name.c_str(), H5P_DEFAULT) < 0)
			throw OmxAttributexException("Couldn't remove attribute '" + name + ".");
	}

	template <typename T>
	void setAttribute(const std::string& name,OmxDataType dataType, T *value) {
		if (hasAttribute(name))
			removeAttribute(name);

		const void *writeValue = value;
		hsize_t dims = 1;
		H5DataspaceScoped dataspace(H5Screate(H5S_SCALAR));
		if (dataspace < 0)
			throw OmxAttributexException("Unable to allocate resources for attribute '" + name + "'.");

		H5TypeScoped type(H5Tcopy(getH5DataType(dataType)));
		if (type < 0)
			throw OmxAttributexException("Couldn't set type information for attribute '" + name + "'.");

		size_t size = 1;
		if (dataType == OmxDataType::String) {
			size = H5T_VARIABLE;
			H5Tset_size(type, size);
			writeValue = (*(OmxString *)value).c_str();

			H5TypeScoped ftype(H5Tcopy(H5T_C_S1));
			H5Tset_strpad(ftype, H5T_STR_NULLTERM);
			herr_t s;
			s = H5Tset_size(ftype, strlen((char *)writeValue)+1);

			H5AttributeScoped attribute(H5Acreate_by_name(_handle, _path.c_str(), name.c_str(), ftype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
			if (attribute < 0)
				throw OmxAttributexException("Unable to create attribute '" + name + "'.");

			if (H5Awrite(attribute, ftype, writeValue) < 0)
				throw OmxAttributexException("Unable to write data for attribute '" + name + "'.");

			return;
		}

		H5AttributeScoped attribute(H5Acreate_by_name(_handle, _path.c_str(), name.c_str(), type, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
		if (attribute < 0)
			throw OmxAttributexException("Unable to create attribute '" + name + "'.");

		if (H5Awrite(attribute, type, writeValue) < 0)
			throw OmxAttributexException("Unable to write data for attribute '" + name + "'.");
	}

	void getAttribute(const std::string& name, void *value, OmxDataType dataType) const {
		verifyAttributeDataType(name, dataType);

		getAttribute(name, value);
	}

	void getAttribute(const std::string& name, void *value) const {
		H5AttributeScoped attribute(H5Aopen_by_name(_handle, _path.c_str(), name.c_str(), H5P_DEFAULT, H5P_DEFAULT));

		if (attribute < 0)
			throw OmxAttributexException("Couldn't open attribute '" + name + "'.");

		H5TypeScoped type(H5Aget_type(attribute));

		H5A_info_t info;
		if (H5Aget_info_by_name(_handle, _path.c_str(), name.c_str(), &info, H5P_DEFAULT) < 0)
			throw OmxAttributexException("Couldn't determine size information of attribute '" + name + "'.");

		size_t dataTypeSize = H5Tget_size(type);
		std::unique_ptr<uint8_t[]> buffer(new uint8_t[info.data_size]);

		if (type < 0)
			throw OmxAttributexException("Couldn't determine the type of attribute '" + name + "'.");

		if (H5Aread(attribute, type, buffer.get()) < 0)
			throw OmxAttributexException("Couldn't read attribute '" + name + "'.");

		std::memcpy(value, buffer.get(), dataTypeSize);
	}

	OmxDataType getAttributeDataType(const std::string& name) const {
		size_t size;
		return getAttributeDataType(name, &size);
	}

	OmxDataType getAttributeDataType(const std::string& name, size_t *size) const {
		// TODO: finish implementation
		hsize_t dims;
		H5T_class_t typeClass;
		OmxDataType omxType;

		if (H5LTget_attribute_info(_handle, _path.c_str(), name.c_str(), &dims, &typeClass, size) < 0)
			throw OmxAttributexException("Couldn't get attribute '" + name + "'.");

		if (typeClass == H5T_STRING) {
			omxType = OmxDataType::String;
		}
		else {
			H5AttributeScoped attribute(H5Aopen_by_name(_handle, _path.c_str(), name.c_str(), H5P_DEFAULT, H5P_DEFAULT));
			if (attribute < 0)
				throw OmxAttributexException("Couldn't get attribute '" + name + ".");

			H5A_info_t info;
			if (H5Aget_info(attribute, &info) < 0)
				throw OmxAttributexException("Couldn't get information about attribute '" + name + ".");

			H5TypeScoped type(H5Aget_type(attribute));

			omxType = getOmxDataType(type);
		}

		return omxType;
	}

	bool isAttributeDataType(const std::string& name, OmxDataType otherDataType) const {
		auto dataType = getAttributeDataType(name);
		return dataType == otherDataType;

	}

	void  verifyAttributeDataType(const std::string& name, OmxDataType dataType) const {
		if (!isAttributeDataType(name, dataType))
			throw OmxAttributexException("Incorrect attribut data type.");
	}

	bool hasAttribute(const std::string& name) const {
		htri_t status = H5Aexists(_handle, name.c_str());

		if (status > 0) {
			return true;
		}
		else if (status == 0) {
			return false;
		}
		else {
			throw OmxAttributexException("Couldn't retrieve data about attribute '" + name + "'.");
		}
	}

	template <typename T>
	std::string getAttributeString(const OmxAttributeCollection *collection, const std::string& name) const {
		std::string s;
		T v;

		collection->getAttribute(name, &v);
		return std::to_string(v);
	}

	hid_t _handle;
	std::string _path;
};

// template specialization
template <>
std::string OmxAttributeCollection::OmxAttributeCollectionImpl::getAttributeString<OmxString>(const OmxAttributeCollection *collection, const std::string& name) const {
	std::string v;
	collection->getAttribute(name, &v);
	return v;
}

OmxAttributeCollection::OmxAttributeCollection(const OmxAttributeOwnerData *ownerData) : _impl{ new OmxAttributeCollectionImpl(ownerData) }   {

}

OmxAttributeCollection::~OmxAttributeCollection() = default;

OmxIndex OmxAttributeCollection::count() const {
	return getAttributeNames().size();
}

herr_t attributeNameIterator(hid_t loc_id, const char *name, const H5A_info_t *ainfo, void *opdata)
{
	((std::vector<std::string> *)opdata)->push_back(name);
	return 0;
}

std::vector<std::string> OmxAttributeCollection::getAttributeNames() const {
	std::vector<std::string> v;

	H5Aiterate_by_name(_impl->_handle, _impl->_path.c_str(), H5_INDEX_CRT_ORDER, H5_ITER_INC, NULL, attributeNameIterator, &v, H5P_DEFAULT);

	return v;
}

// setters
void OmxAttributeCollection::setAttribute(const std::string& name, OmxInt8 *value) {
    _impl->setAttribute<OmxInt8>(name, OmxDataType::Int8, value);
}

void OmxAttributeCollection::setAttribute(const std::string& name, OmxUInt8 *value) {
    _impl->setAttribute<OmxUInt8>(name, OmxDataType::UInt8, value);
}
void OmxAttributeCollection::setAttribute(const std::string& name, OmxInt16 *value) {
    _impl->setAttribute<OmxInt16>(name, OmxDataType::Int16, value);
}
void OmxAttributeCollection::setAttribute(const std::string& name, OmxUInt16 *value) {
    _impl->setAttribute<OmxUInt16>(name, OmxDataType::UInt16, value);
}
void OmxAttributeCollection::setAttribute(const std::string& name, OmxInt32 *value) {
    _impl->setAttribute<OmxInt32>(name, OmxDataType::Int32, value);
}
void OmxAttributeCollection::setAttribute(const std::string& name, OmxUInt32 *value) {
    _impl->setAttribute<OmxUInt32>(name, OmxDataType::UInt32, value);
}

void OmxAttributeCollection::setAttribute(const std::string& name, OmxInt64 *value) {
    _impl->setAttribute<OmxInt64>(name, OmxDataType::Int32, value);
}

void OmxAttributeCollection::setAttribute(const std::string& name, OmxUInt64 *value) {
    _impl->setAttribute<OmxUInt64>(name, OmxDataType::UInt64, value);
}

void OmxAttributeCollection::setAttribute(const std::string& name, OmxFloat *value) {
    _impl->setAttribute<OmxFloat>(name, OmxDataType::Float, value);
}

void OmxAttributeCollection::setAttribute(const std::string& name, OmxDouble *value) {
    _impl->setAttribute<OmxDouble>(name, OmxDataType::Double, value);
}

void OmxAttributeCollection::setAttribute(const std::string& name, OmxString *value) {
    _impl->setAttribute<OmxString>(name, OmxDataType::String, value);
}


void OmxAttributeCollection::setAttributeInt8(const std::string& name, OmxInt8 value) {
	_impl->setAttribute<OmxInt8>(name, OmxDataType::Int8, &value);
}

void OmxAttributeCollection::setAttributeUInt8(const std::string& name, OmxUInt8 value) {
	_impl->setAttribute<OmxUInt8>(name, OmxDataType::UInt8, &value);
}

void OmxAttributeCollection::setAttributeInt16(const std::string& name, OmxInt16 value) {
	_impl->setAttribute<OmxInt16>(name, OmxDataType::Int16, &value);
}

void OmxAttributeCollection::setAttributeUInt16(const std::string& name, OmxUInt16 value) {
	_impl->setAttribute<OmxUInt16>(name, OmxDataType::UInt16, &value);
}

void OmxAttributeCollection::setAttributeInt32(const std::string& name, OmxInt32 value) {
	_impl->setAttribute<OmxInt32>(name, OmxDataType::Int32, &value);
}

void OmxAttributeCollection::setAttributeUInt32(const std::string& name, OmxUInt32 value) {
	_impl->setAttribute<OmxUInt32>(name, OmxDataType::UInt32, &value);
}

void OmxAttributeCollection::setAttributeInt64(const std::string& name, OmxInt64 value) {
	_impl->setAttribute<OmxInt64>(name, OmxDataType::Int32, &value);
}

void OmxAttributeCollection::setAttributeUInt64(const std::string& name, OmxUInt64 value) {
	_impl->setAttribute<OmxUInt64>(name, OmxDataType::UInt64, &value);
}

void OmxAttributeCollection::setAttributeFloat(const std::string& name, OmxFloat value) {
	_impl->setAttribute<OmxFloat>(name, OmxDataType::Float, &value);
}

void OmxAttributeCollection::setAttributeDouble(const std::string& name, OmxDouble value) {
	_impl->setAttribute<OmxDouble>(name, OmxDataType::Double, &value);
}

void OmxAttributeCollection::setAttributeString(const std::string& name, OmxString value) {
	_impl->setAttribute<OmxString>(name, OmxDataType::String, &value);
}

// getters
size_t OmxAttributeCollection::getAttributeStringLength(const std::string& name) const {
	size_t len;
	_impl->verifyAttributeDataType(name, OmxDataType::String);
	_impl->getAttributeDataType(name, &len);

	return len;
}

void OmxAttributeCollection::getAttribute(const std::string& name, OmxString *value) const {
    size_t len;
    char * buf;

    _impl->verifyAttributeDataType(name, OmxDataType::String);
    if (_impl->getAttributeDataType(name, &len) == OmxDataType::Unknown)
        throw OmxAttributexException("Couldn't get value for attribute '" + name + "' because it has unknown type.");

    buf = new char[len];
    H5LTget_attribute_string(_impl->_handle, _impl->_path.c_str(), name.c_str(), buf);
    *value = buf;
    delete buf;
}

void OmxAttributeCollection::getAttribute(const std::string& name, OmxInt8 *value) const {
    _impl->getAttribute(name, value, OmxDataType::Int8);
}

void OmxAttributeCollection::getAttribute(const std::string& name, OmxUInt8 *value) const {
    _impl->getAttribute(name, value, OmxDataType::UInt8);
}

void OmxAttributeCollection::getAttribute(const std::string& name, OmxInt16 *value) const {
    _impl->getAttribute(name, value, OmxDataType::Int16);
}

void OmxAttributeCollection::getAttribute(const std::string& name, OmxUInt16 *value) const {
    _impl->getAttribute(name, value, OmxDataType::UInt16);
}

void OmxAttributeCollection::getAttribute(const std::string& name, OmxInt32 *value) const {
    _impl->getAttribute(name, value, OmxDataType::Int32);
}

void OmxAttributeCollection::getAttribute(const std::string& name, OmxUInt32 *value) const {
    _impl->getAttribute(name, value, OmxDataType::UInt32);
}

void OmxAttributeCollection::getAttribute(const std::string& name, OmxInt64 *value) const {
    _impl->getAttribute(name, value, OmxDataType::Int64);
}

void OmxAttributeCollection::getAttribute(const std::string& name, OmxUInt64 *value) const {
    _impl->getAttribute(name, value, OmxDataType::UInt64);
}

void OmxAttributeCollection::getAttribute(const std::string& name, OmxFloat *value) const {
    _impl->getAttribute(name, value, OmxDataType::Float);
}

void OmxAttributeCollection::getAttribute(const std::string& name, OmxDouble *value) const {
    _impl->getAttribute(name, value, OmxDataType::Double);
}

OmxInt8 OmxAttributeCollection::getAttributeInt8(const std::string& name) const {
    OmxInt8 value;
	_impl->getAttribute(name, &value, OmxDataType::Int8);
    return value;
}

OmxUInt8 OmxAttributeCollection::getAttributeUInt8(const std::string& name) const {
    OmxUInt8 value;
	_impl->getAttribute(name, &value, OmxDataType::UInt8);
    return value;
}

OmxInt16 OmxAttributeCollection::getAttributeInt16(const std::string& name) const {
    OmxInt16 value;
	_impl->getAttribute(name, &value, OmxDataType::Int16);
    return value;
}

OmxUInt16 OmxAttributeCollection::getAttributeUInt16(const std::string& name) const {
    OmxUInt16 value;
	_impl->getAttribute(name, &value, OmxDataType::UInt16);
    return value;
}

OmxInt32 OmxAttributeCollection::getAttributeInt32(const std::string& name) const {
    OmxInt32 value;
	_impl->getAttribute(name, &value, OmxDataType::Int32);
    return value;
}

OmxUInt32 OmxAttributeCollection::getAttributeUInt32(const std::string& name) const {
    OmxUInt32 value;
	_impl->getAttribute(name, &value, OmxDataType::UInt32);
    return value;
}

OmxInt64 OmxAttributeCollection::getAttributeInt64(const std::string& name) const {
    OmxInt64 value;
	_impl->getAttribute(name, &value, OmxDataType::Int64);
    return value;
}

OmxUInt64 OmxAttributeCollection::getAttributeUInt64(const std::string& name) const {
    OmxUInt64 value;
	_impl->getAttribute(name, &value, OmxDataType::UInt64);
    return value;
}

OmxFloat OmxAttributeCollection::getAttributeFloat(const std::string& name) const {
    OmxFloat value;
	_impl->getAttribute(name, &value, OmxDataType::Float);
    return value;
}

OmxDouble OmxAttributeCollection::getAttributeDouble(const std::string& name) const {
    OmxDouble value;
	_impl->getAttribute(name, &value, OmxDataType::Double);
    return value;
}

std::string OmxAttributeCollection::getAttributeString(const std::string& name) const {
	size_t size;
	auto type = _impl->getAttributeDataType(name, &size);

	switch (type) {
	case OmxDataType::Int8:		return _impl->getAttributeString<OmxInt8>(this, name);
	case OmxDataType::UInt8:	return _impl->getAttributeString<OmxUInt8>(this, name);
	case OmxDataType::Int16:	return _impl->getAttributeString<OmxInt16>(this, name);
	case OmxDataType::UInt16:	return _impl->getAttributeString<OmxUInt16>(this, name);
	case OmxDataType::Int32:	return _impl->getAttributeString<OmxInt32>(this, name);
	case OmxDataType::UInt32:	return _impl->getAttributeString<OmxUInt32>(this, name);
	case OmxDataType::Int64:	return _impl->getAttributeString<OmxInt64>(this, name);
	case OmxDataType::UInt64:	return _impl->getAttributeString<OmxUInt64>(this, name);
	case OmxDataType::Float:	return _impl->getAttributeString<OmxFloat>(this, name);
	case OmxDataType::Double:	return _impl->getAttributeString<OmxDouble>(this, name);
	case OmxDataType::String:	return _impl->getAttributeString<OmxString>(this, name);
	case OmxDataType::Unknown: throw OmxAttributexException("Cannot convert unknown attribute type to string.");
	default:				   throw OmxAttributexException("Cannot determine attribute type.");
	}	
}

OmxDataType OmxAttributeCollection::getAttributeDataType(const std::string& name) const {
	size_t size;
	return _impl->getAttributeDataType(name, &size);
}

bool OmxAttributeCollection::hasAttribute(const std::string& name) const {
	return _impl->hasAttribute(name);
}

bool OmxAttributeCollection::hasAttribute(const std::string& name, OmxDataType dataType) const {
	return _impl->hasAttribute(name) && getAttributeDataType(name) == dataType;
}

}
