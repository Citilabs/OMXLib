#include "../include/OmxMatrix.hpp"

#include "OmxH5Common.hpp"
#include "OmxFileOwnerData.hpp"
#include "OmxAttributeOwnerData.hpp"

#include <stdexcept>
#include <map>

#include <cstring>

#include <hdf5.h>
#include <hdf5_hl.h>

namespace omx {

class OmxMatrix::OmxMatrixImpl {
public:
	OmxMatrixImpl(OmxIndex zones, OmxDataType dataType, const std::string& name, OmxCompressionLevel compressionLevel, hid_t dataset, size_t sizeOfDataType) :
								_zones( zones ),
								_dataType( dataType ),
								_name(name),
								_compressionLevel( compressionLevel ),
								_dataset( dataset ),
								_sizeOfDataType{ sizeOfDataType },
								_attributes(nullptr) {
		_memspace = -1;
		_dataspace = -1;

		OmxAttributeOwnerData attributeOwnerData{ _dataset, "." };
		_attributes.reset(new OmxAttributeCollection(&attributeOwnerData));

		_isClosed = false;
	}

	~OmxMatrixImpl() {
		close();
	}

	void writeRowH5(OmxIndex row, void *rowBuffer) {
		hsize_t dims[2], start[2];

		dims[0] = 1;
		dims[1] = _zones;

		start[0] = row;
		start[1] = 0;

		if (_memspace < 0)
			_memspace = H5Screate_simple(2, dims, NULL);

		if (_dataspace < 0) {
			_dataspace = H5Dget_space(_dataset);
		}

		H5Sselect_hyperslab(_dataspace, H5S_SELECT_SET, start, NULL, dims, NULL);

		herr_t status = H5Dwrite(_dataset,
			getH5DataType(_dataType),
			_memspace,
			_dataspace,
			H5P_DEFAULT,
			rowBuffer);

		if (status < 0) {
			throw OmxMatrixException("Unable to write row of matrix to storage.");
		}
	}

	void writeRow(OmxIndex row, void *rowBuffer) {
		writeRowH5(row, rowBuffer);
	}

	void close() {
		if (_dataspace >= 0)
			H5Sclose(_dataspace);

		if (_memspace >= 0)
			H5Sclose(_memspace);

		_attributes.reset(nullptr);

		_isClosed = true;
	}

	OmxIndex _zones;

	hid_t _memspace;
	hid_t _dataspace;
	hid_t _dataset;

	bool _isClosed;

	OmxDataType _dataType;
	OmxCompressionLevel _compressionLevel;

	// cache the dataType size
	size_t _sizeOfDataType;

	std::unique_ptr<OmxAttributeCollection> _attributes;
	std::string _name;
};

OmxMatrix::OmxMatrix(OmxDataType dataType, OmxIndex zones, const std::string& name, OmxCompressionLevel compressionLevel, const OmxFileOwnerData *ownerData)
	: _impl{ new OmxMatrixImpl{ zones, dataType, name, compressionLevel, ownerData->_dataset, getDataTypeSize(dataType) } } {

}

OmxMatrix::~OmxMatrix() = default;

std::string OmxMatrix::getName() const {
	return _impl->_name;
}

void OmxMatrix::writeRow(OmxIndex row, OmxIndex, void *rowBuffer, OmxDataType dataType) {
	if (dataType != _impl->_dataType)
		throw OmxMatrixException("Data type mismatch.");

	_impl->writeRow(row, rowBuffer);
}

void OmxMatrix::writeRow(OmxIndex row, void *rowBuffer) {
    if (row >= _impl->_zones)
        throw OmxMatrixException("Out of range.");

    _impl->writeRow(row, rowBuffer);
}

void OmxMatrix::readRow(OmxIndex row, void *rowBuffer, OmxDataType dataType) {
	if (dataType != _impl->_dataType)
		throw std::invalid_argument("Data type mismatch.");

	readRow(row, rowBuffer);
}

void OmxMatrix::readRow(OmxIndex row, void *rowBuffer) {
	if (row >= _impl->_zones)
		throw std::out_of_range("Row index " + std::to_string(row) + " was out of the acceptable range.");

	hsize_t dims[2],start[2];

	dims[0] = 1;
	dims[1] = _impl->_zones;
	start[0] = row;
	start[1] = 0;

	if (_impl->_dataspace < 0) {
		_impl->_dataspace = H5Dget_space(_impl->_dataset);
	}

	if (_impl->_memspace < 0) {
		_impl->_memspace = H5Screate_simple(2, dims, NULL);
	}

	if (H5Sselect_hyperslab(_impl->_dataspace, H5S_SELECT_SET, start, NULL, dims, NULL) < 0) {
		throw OmxMatrixException("Unable to prepare for reading the matrix.");
	}

	if (H5Dread(_impl->_dataset, getH5DataType(_impl->_dataType), _impl->_memspace, _impl->_dataspace,
		H5P_DEFAULT, rowBuffer) < 0) {
		throw OmxMatrixException("Unable to read matrix.");
	}
}

void* OmxMatrix::createMatrixRowBuffer() const {
	auto size = getDataTypeSize(_impl->_dataType) *  _impl->_zones;
	return (void *)new uint8_t[size];
}

void* OmxMatrix::createMatrixBuffer() const {
	auto size = getDataTypeSize(_impl->_dataType) *  _impl->_zones * _impl->_zones;
	return (void *)new uint8_t[size];
}

OmxCompressionLevel OmxMatrix::getCompressionLevel() const {
	return _impl->_compressionLevel;
}

OmxIndex OmxMatrix::getZones() const {
	return _impl->_zones;
}

OmxDataType OmxMatrix::getDataType() const {
	return _impl->_dataType;
}

size_t OmxMatrix::getDataSize() const {
	return getDataTypeSize(_impl->_dataType) * getZones();
}

OmxAttributeCollection& OmxMatrix::attributes() const {
	return *_impl->_attributes;
}

void OmxMatrix::close() {

}

}
