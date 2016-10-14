#include "../include/OmxZonalReference.hpp"

#include "OmxH5Common.hpp"
#include "H5Scoped.hpp"
#include "OmxFileOwnerData.hpp"
#include "OmxAttributeOwnerData.hpp"

#include <cstring>

namespace omx {

class OmxZonalReference::OmxZonalReferenceImpl {
public:

	OmxZonalReferenceImpl(OmxDataType dataType, OmxIndex zones, const std::string& name, OmxCompressionLevel compressionLevel, hid_t dataset)
		: _dataType( dataType ), _zones( zones ), _name( name ), _dataset( dataset ), _memspace( -1 ), _compressionLevel( compressionLevel ),_dataspace( -1 ), _attributes(nullptr) {

		OmxAttributeOwnerData attributeOwnerData{ _dataset, "." };
		_attributes.reset(new OmxAttributeCollection(&attributeOwnerData));

	}

	~OmxZonalReferenceImpl() {
		close();
	}

	void close() {
		if (_dataspace >= 0)
			H5Sclose(_dataspace);

		if (_memspace >= 0)
			H5Sclose(_memspace);
	}

	void writeReference(const void *buffer)  {
		herr_t status;
		hsize_t count[1], offset[1];

		count[0] = _zones;

		offset[0] = 0;

		if (_memspace < 0)
			_memspace = H5Screate_simple(1, count, NULL);

		if (_dataspace < 0) {
			_dataspace = H5Dget_space(_dataset);
		}

		status = H5Sselect_hyperslab(_dataspace, H5S_SELECT_SET, offset, NULL, count, NULL);
		if (status < 0)
			throw OmxZonalReferenceException("Unable to prepare data for writing zonal reference.");


		status = H5Dwrite(_dataset,
			getH5DataType(_dataType),
			_memspace,
			_dataspace,
			H5P_DEFAULT,
			buffer);

		if (status < 0) {
			throw OmxZonalReferenceException("Unable to write data for zonal reference.");
		}

		if (status < 0) {
			throw OmxZonalReferenceException("Unable to write reference data to storage.");
		}
	}

	void* createZonalReferenceBuffer() const {
		if (_dataType == OmxDataType::String)
			throw OmxZonalReferenceException("createZonalReferenceBuffer() is not valid for string data types.");

		auto dataTypeSize = getDataTypeSize(_dataType);
		auto size = dataTypeSize * _zones;
		return (void *)new uint8_t[size];
	}

	OmxCompressionLevel _compressionLevel;
	OmxDataType _dataType;
	OmxIndex _zones;
	std::string _name;

	hid_t _memspace;
	hid_t _dataspace;
	hid_t _dataset;

	std::unique_ptr<OmxAttributeCollection> _attributes;
};

OmxZonalReference::OmxZonalReference(OmxDataType dataType, OmxIndex zones, const std::string& name, OmxCompressionLevel compressionLevel, const OmxFileOwnerData *ownerData)
	: _impl{ new OmxZonalReferenceImpl(dataType, zones, name, compressionLevel, ownerData->_dataset) } {

}

OmxZonalReference::~OmxZonalReference() = default;

std::string OmxZonalReference::getName() const {
	return _impl->_name;
}

OmxCompressionLevel OmxZonalReference::getCompressionLevel() const {
	return _impl->_compressionLevel;
}

OmxDataType OmxZonalReference::getDataType() const {
	return _impl->_dataType;
}

OmxIndex OmxZonalReference::getZones() const {
	return _impl->_zones;
}

size_t OmxZonalReference::getDataSize() const {
	auto dataType = getDataType(); 
	size_t dataSize = dataType != OmxDataType::String ? getDataTypeSize(dataType) : 0;
	return dataSize * getZones();
}

void OmxZonalReference::readReference(void *buffer) const {
	if (_impl->_dataType == OmxDataType::String)
		throw OmxZonalReferenceException("Cannot use readReference() for string data, must use readStringReference().");

	hsize_t data_count[1], data_offset[1];

	data_count[0] = _impl->_zones;
	data_offset[0] = 0;

	if (_impl->_dataspace < 0) {
		_impl->_dataspace = H5Dget_space(_impl->_dataset);
	}

	if (_impl->_memspace < 0) {
		_impl->_memspace = H5Screate_simple(1, data_count, NULL);
	}

	if (H5Sselect_hyperslab(_impl->_dataspace, H5S_SELECT_SET, data_offset, NULL, data_count, NULL) < 0) {
		throw OmxZonalReferenceException("Unable to prepare for reading the zonal reference.");
	}

	if (H5Dread(_impl->_dataset, getH5DataType(_impl->_dataType), _impl->_memspace, _impl->_dataspace,
		H5P_DEFAULT, buffer) < 0) {
		throw OmxZonalReferenceException("Unable to read zonal reference.");
	} 
}

std::vector<std::string> OmxZonalReference::readStringReference() const {
	std::vector<std::string> values;

	if (_impl->_dataType != OmxDataType::String)
		throw OmxZonalReferenceException("Zonal reference is not of type string.");

	hid_t memtype = H5Tcopy(H5T_C_S1);
	herr_t status = H5Tset_size(memtype, H5T_VARIABLE);

	char **buffer = (char **)malloc(_impl->_zones * sizeof(char*));


	if (!buffer)
		throw OmxZonalReferenceException("Unable to allocate memory for buffer.");

	H5DataspaceScoped space(H5Dget_space(_impl->_dataset));
	status = H5Dread(_impl->_dataset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer);



	for (OmxIndex i = 0; i < _impl->_zones; i++) {
		values.push_back(buffer[i] != nullptr ? buffer[i] : "");
	}

	status = H5Dvlen_reclaim(memtype, space, H5P_DEFAULT, buffer);
	free(buffer);
	return values;
}

void OmxZonalReference::writeStringReference(const std::vector<std::string> &values) {

	if (values.size() != _impl->_zones)
		throw OmxZonalReferenceException("Incorrect number of zonal references specified.");

	hsize_t tempDims[1] = { _impl->_zones };
	herr_t status;

	H5TypeScoped memtype(H5Tcopy(H5T_C_S1));
	status = H5Tset_size(memtype, H5T_VARIABLE);

	std::vector<char*> cstrings;

	for (size_t i = 0; i < values.size(); ++i)
		cstrings.push_back(const_cast<char*>(values[i].c_str()));


	status = H5Dwrite(_impl->_dataset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &cstrings[0]);
}

void OmxZonalReference::writeReference(const void *buffer) {
	if (_impl->_dataType == OmxDataType::String)
		throw OmxZonalReferenceException("Cannot use writeReference() for string data, must use writeStringReference().");

	_impl->writeReference(buffer);
}

void* OmxZonalReference::createReferenceBuffer() const {
	return _impl->createZonalReferenceBuffer();
}

OmxAttributeCollection& OmxZonalReference::attributes() const {
	return *_impl->_attributes;
}

}
