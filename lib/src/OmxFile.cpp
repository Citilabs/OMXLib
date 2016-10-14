#include "../include/OmxFile.hpp"

#include "../include/OmxAttributeCollection.hpp"
#include "../include/OmxMatrix.hpp"
#include "../include/OmxZonalReference.hpp"

#include "OmxH5Common.hpp"
#include "H5Scoped.hpp"
#include "OmxAttributeOwnerData.hpp"
#include "OmxFileOwnerData.hpp"

#include <map>
#include <algorithm>
#include <functional>
#include <fstream>

#include <hdf5.h>
#include <hdf5_hl.h>

#define HDF5_PATH_MATRICES			"/matrices"
#define HDF5_PATH_ZONAL_REFS		"/zonalReferences"
#define HDF5_ATTR_OMX_VERSION		"OMX_VERSION"
#define HDF5_ATTR_VALUE_OMX_VERSION "0.3"
#define HDF5_ATTR_OMX_ZONES			"OMX_ZONES"

namespace omx {

static const OmxIndex MAX_CHUNK_DIMS = 2;
static const size_t IDEAL_CHUNK_SIZE_NO_COMPRESSION = 256000;
static const size_t IDEAL_CHUNK_SIZE_WITH_COMPRESSION = 320000;
static const OmxDataType DEFAULT_DATA_TYPE = OmxDataType::Double;


herr_t datasetNameIterator(hid_t loc_id, const char *name, const H5L_info_t *info, void *opdata)
{
	((std::vector<std::string> *)opdata)->push_back(name);

	return 0;
}

typedef std::function<bool(OmxDataType)> ValidDatasetDataTypeFn_t;
template <typename T>
struct NamedDatasetObjectCollection {
	NamedDatasetObjectCollection(const std::string& typeName, const std::string& typeNamePlural, const std::string& parentPath, ValidDatasetDataTypeFn_t validDataTypeFn)
		: _typeName(typeName), _parentPath(parentPath), _typeNamePlural(typeNamePlural), _validDataTypeFn(validDataTypeFn), _datasets(), _entries() {
		
	}

	NamedDatasetObjectCollection(const NamedDatasetObjectCollection&) = delete;

	NamedDatasetObjectCollection & operator=(const NamedDatasetObjectCollection&) = delete;

	~NamedDatasetObjectCollection() {
		clear();
	}

	void add(const std::string& name, hid_t dataset, T *t) {
		_datasets[name] = std::make_unique<H5DatasetScoped>(dataset);
		_entries.push_back(std::unique_ptr<T>(t));
	}

	bool exists(const std::string& name) const {

		return std::find_if(_entries.cbegin(), _entries.cend(),
			[&name](const std::unique_ptr<T> &e) { return e->getName() == name; }) != _entries.cend();
	}

	std::vector<std::string> getNames() const {
		std::vector<std::string> names;

		for (const auto &e : _entries) {
			names.push_back(e->getName());
		}

		return names;
	}

	T* getEntry(const std::string& name) const {
		for (const auto& e : _entries) {
			if (e->getName() == name)
				return e.get();
		}

		throw OmxFileException("Couldn't find " + _typeName + " with name '" + name + "'.");
	}

	T* getEntry(OmxIndex index) const {
		if (index >= size())
			throw OmxFileException("Index out of range.");

		return _entries[index].get();
	}

	inline size_t size() const {
		return _entries.size();
	}

	void clear() {
		_entries.clear();
		_datasets.clear();
	}

	std::string _typeName;
	std::string _typeNamePlural;
	std::string _parentPath;
	ValidDatasetDataTypeFn_t _validDataTypeFn;
	std::map<std::string, std::unique_ptr<H5DatasetScoped>> _datasets;
	std::vector<std::unique_ptr<T>> _entries;
};

static bool isValidMatrixDataType(OmxDataType dataType) {
	return dataType != OmxDataType::String && dataType != OmxDataType::Unknown;
}

static bool isValidZonalReferenceDataType(OmxDataType dataType) {
	return dataType != OmxDataType::Unknown;
}

// parameters (name, zones, compressionLevel, h5Type, ownerData)
typedef std::function<void*(const std::string&, OmxIndex, OmxCompressionLevel, hid_t, OmxFileOwnerData *)> DatasetObjectFactory_t;

class OmxFile::OmxFileImpl {
public:

	OmxFileImpl(const std::string& filename)
		: _filename( filename ),
		_mats("matrix", "matrices", HDF5_PATH_MATRICES, &isValidMatrixDataType),
		_zonals("zonal reference", "zonal references", HDF5_PATH_ZONAL_REFS, &isValidZonalReferenceDataType),
		_isInitialized( false ),
		_compressionLevel( OmxCompressionLevel::NoCompression ),
		_attributes( nullptr ),
		_version( OmxVersion::v0_3_0 ),
		_handle( nullptr ) {
	}

	~OmxFileImpl() {
		close();
	}

	inline bool hasValidHandle() const { return _handle != nullptr && *_handle >= 0; }

	inline void requireValidHandle() const {
		if (!hasValidHandle())
			throw OmxFileException("File was not properly opened.");
	}
	
	hid_t openDataset(const std::string& parentPath, const std::string& name, const std::string& typeName) {
		std::string path = parentPath + "/" + name;

		hid_t dataset = H5Dopen(*_handle, path.c_str(), H5P_DEFAULT);
		if (dataset < 0) {
			throw OmxFileException("Could not open " + typeName + " '" + name + "'.");
		}

		return dataset;
	}

	void readZones() {
		OmxUInt64 zones;

		try {
			_attributes->getAttribute(HDF5_ATTR_OMX_ZONES, &zones);
		}
		catch (OmxAttributexException)
		{
			throw OmxFileException("Unable to read zone information.");
		}
		
		_zones = zones;
	}

	void readAndVerifyFile() {
		requireValidHandle();

		if (!_attributes->hasAttribute(HDF5_ATTR_OMX_VERSION)) {
			throw OmxFileException("No version specified.");
		}

		if (_attributes->getAttributeString(HDF5_ATTR_OMX_VERSION) != std::string(HDF5_ATTR_VALUE_OMX_VERSION)) {
			throw OmxFileException("Unsupported version.");
		}

		readZones();
		readMatrices();
		readZonalReferences();
	}
	
	void readMatrices() {
		readDatasets<OmxMatrix, OmxMatrixException>(&_mats, true, nullptr, &matrixFactory);
	}

	void readZonalReferences() {
		readDatasets<OmxZonalReference, OmxZonalReferenceException>(&_zonals, true, nullptr, &zonalReferenceFactory);
	}
	
	// TODO: this checking of the dataset types to only have one type for certain sorts of datatypes may be removed
	template <typename T, typename E>
	void readDatasets(NamedDatasetObjectCollection<T> *collection, bool allowMultipleDataTypes, hid_t *firstH5Type, DatasetObjectFactory_t factoryFn) {
		requireValidHandle();
	
		std::string typeName = collection->_typeName;
	
		uint32_t flags = 0;
	
		H5GroupScoped group(H5Gopen(*_handle, collection->_parentPath.c_str(), H5P_DEFAULT));
	
		if (group < 0) {
			throw E("Error opening " + typeName + " metadata.");
		}
	
		H5PlistScoped info(H5Gget_create_plist(group));
		if (H5Pget_link_creation_order(info, &flags) < 0) {
			throw E("Couldn't determine " + typeName + " ordering.");
		}
	
		std::vector<std::string> datasetNames;
		auto indexType = flags & H5P_CRT_ORDER_TRACKED ? H5_INDEX_CRT_ORDER : H5_INDEX_NAME;
		H5Literate(group, indexType, H5_ITER_INC, NULL, datasetNameIterator, &datasetNames);
	
		hid_t datasetType = -1;
	
		for (auto name : datasetNames) {
			// filters
			hid_t dataset = openDataset(collection->_parentPath, name, typeName);
			if (dataset < 0)
				E("Couldn't open " + typeName + " '" + name + ".");
	
			H5PlistScoped plist{ H5Dget_create_plist(dataset) };
	
			if (plist < 0)
				E("Couldn't read metadata for " + typeName + " '" + name + ".");
	
			H5TypeScoped datasetType2(H5Dget_type(dataset));
			if (datasetType2 < 0)
				throw E("Couldn't determine type information for " + typeName + " '" + name + "'.");
	
			hid_t directType = getH5DirectDataType(datasetType2);

			if (!collection->_validDataTypeFn(getOmxDataType(directType)))
				throw E("Invalid data type detected for " + typeName + " '" + name + ".");
			
			if (datasetType == -1) {
				datasetType = directType;

				if (firstH5Type)
					*firstH5Type = directType;
			}
			else if (datasetType != directType && !allowMultipleDataTypes) {
				if (dataset >= 0)
					H5Dclose(dataset);
	
				throw E("Contains " + collection->_typeNamePlural + " of more than one of the supported data types, which is not allowed.");
			}

			// all checks passed, add to list
			OmxFileOwnerData ownerData{ *_handle, dataset };
			collection->add(name, dataset, static_cast<T*>(factoryFn(name, _zones, _compressionLevel, directType, &ownerData)));
		}
	}

	static OmxMatrix *matrixFactory(const std::string& name, OmxIndex zones, OmxCompressionLevel compressionLevel, hid_t h5Type, const OmxFileOwnerData *ownerData)
	{ 
		return new OmxMatrix(getOmxDataType(h5Type), zones, name, compressionLevel, ownerData); 
	}

	static OmxZonalReference *zonalReferenceFactory(const std::string& name, OmxIndex zones, OmxCompressionLevel compressionLevel, hid_t h5Type, const OmxFileOwnerData *ownerData)
	{
		return new OmxZonalReference(getOmxDataType(h5Type), zones, name, compressionLevel, ownerData);
	}

	void initialize(bool isCreating) {
		requireValidHandle();

		if (_isInitialized) {
			throw OmxFileException("OmxFile is already initialized.");
		}

		OmxAttributeOwnerData attributeOwnerData{ *_handle, "/" };
		_attributes.reset(new OmxAttributeCollection(&attributeOwnerData));

		if (isCreating) {
			H5LTset_attribute_string(*_handle, "/", HDF5_ATTR_OMX_VERSION, HDF5_ATTR_VALUE_OMX_VERSION);

			_attributes->setAttribute("OMX_ZONES", &_zones);

			H5PlistScoped plist(H5Pcreate(H5P_GROUP_CREATE));
			H5Pset_link_creation_order(plist, H5P_CRT_ORDER_TRACKED);
			
			H5Gclose(H5Gcreate(*_handle, HDF5_PATH_MATRICES, H5P_DEFAULT, plist, H5P_DEFAULT));
			H5Gclose(H5Gcreate(*_handle, HDF5_PATH_ZONAL_REFS, H5P_DEFAULT, plist, H5P_DEFAULT));

		}
		else {
			readAndVerifyFile();
		}

		_isInitialized = true;
	}

	void setChunkSize2D(hsize_t *chunkData, OmxIndex zones, size_t dataTypeSize, OmxCompressionLevel compressionLevel) {
		auto rowSize = dataTypeSize * zones;
		size_t idealChunkDataSize = compressionLevel == OmxCompressionLevel::NoCompression 
			? IDEAL_CHUNK_SIZE_NO_COMPRESSION : IDEAL_CHUNK_SIZE_WITH_COMPRESSION;

		size_t maxZonesPerRowFit = idealChunkDataSize / dataTypeSize;

		if (zones == maxZonesPerRowFit) {
			chunkData[0] = 1;
			chunkData[1] = zones;
		}
		else if (zones < maxZonesPerRowFit) {
			auto numRows = idealChunkDataSize / rowSize;
			chunkData[0] = std::min(zones, numRows);
			chunkData[1] = zones;
		}
		else {
			chunkData[0] = 1;
			chunkData[1] = idealChunkDataSize / dataTypeSize;
		}
	}

	void setChunkSize1D(hsize_t *chunkData, OmxIndex zones, size_t dataTypeSize) {
		size_t maxEntriesPerChunk = IDEAL_CHUNK_SIZE_NO_COMPRESSION / dataTypeSize;
		chunkData[0] = zones > maxEntriesPerChunk ? maxEntriesPerChunk : zones;
	}

	template <typename T, typename E>
	hid_t addH5Dataset(NamedDatasetObjectCollection<T> *collection,
					   const std::string& name,
					   OmxCompressionLevel compressionLevel,
					   int numDims,
					   const hsize_t *dims,
					   hid_t h5Type,
					   std::function<void(hid_t)> *additionalPlistFn,
					   DatasetObjectFactory_t factoryFn) {

		requireValidHandle();

		const auto datasetTypeName = collection->_typeName;

		if (!collection->_validDataTypeFn(getOmxDataType(h5Type)))
			throw E("Attempted to create a " + datasetTypeName + " with an invalid data type.");

		if (!isValidDatasetName(name))
			throw E("The name '" + name + "' is not a valid " + datasetTypeName + " name.");

		if (collection->exists(name))
			throw E("A " + datasetTypeName + " with the name '" + name + "' already exists.");

		
		hsize_t chunkSize[2];
		if (numDims == 2) {
			setChunkSize2D(chunkSize, _zones, getDataTypeSize(getOmxDataType(h5Type)), compressionLevel);
		}
		else if(numDims == 1) {
			setChunkSize1D(chunkSize, _zones, getDataTypeSize(getOmxDataType(h5Type)));
		}
		else {
			throw E("Incompatible dimensions.");
		}

		H5DataspaceScoped dataspace(H5Screate_simple(numDims, dims, NULL));

		if (dataspace < 0)
			throw E("Couldn't prepare for new " + datasetTypeName + ".");

		H5PlistScoped plist(H5Pcreate(H5P_DATASET_CREATE));

		if (plist < 0)
			throw E("Couldn't prepare metadata parameters for new " + datasetTypeName + ".");

		if (H5Pset_chunk(plist, numDims, chunkSize) < 0)
			throw E("Couldn't set data parameters for new " + datasetTypeName + ".");

		if (compressionLevel != OmxCompressionLevel::NoCompression) {
			if (H5Pset_deflate(plist, getH5CompressionLevelFromOmx(compressionLevel)) < 0)
				throw E("Couldn't set compression level for new " + datasetTypeName + ".");
		}

		if (additionalPlistFn)
			(*additionalPlistFn)(plist);

		std::string path = collection->_parentPath + "/" + name;
		hid_t dataset = H5Dcreate2(*_handle, path.c_str(), h5Type, dataspace, H5P_DEFAULT, plist, H5P_DEFAULT);
		if (dataset < 0) {
			throw E("Error creating " + datasetTypeName + " '" + name + "'.");
		}

		OmxFileOwnerData ownerData{ *_handle, dataset };
		collection->add(name, dataset, static_cast<T*>(factoryFn(name, _zones, compressionLevel, h5Type, &ownerData)));

		return dataset;
	}

	template <typename T, typename E>
	void removeDataset(NamedDatasetObjectCollection<T> *collection, const std::string& name) {

		if (!collection->exists(name))
			throw E("No " + collection->_typeName + " found with name '" + name + ".");

		if (H5Ldelete(*_handle, (collection->_parentPath + "/" + name).c_str(), H5P_DEFAULT) < 0) {
			throw E("Couldn't remove " + collection->_typeName + " '" + name + "'.");
		}

		collection->_datasets.erase(collection->_datasets.find(name));
		collection->_entries.erase(std::remove_if(collection->_entries.begin(), collection->_entries.end(),
			[&name](const std::unique_ptr<T> &m) { return m.get()->getName() == name; }),
			collection->_entries.end());
	}

	bool matrixNameExists(const std::string& name) const {
		requireValidHandle();

		return _mats.exists(name);
	}

	bool zonalReferenceExists(const std::string& name) const {
		requireValidHandle();

		return _zonals.exists(name);
	}

	bool isValidDatasetName(const std::string& name) const {
		// TODO: add additional rules
		return name.length() > 0;
	}

	void close() {
		if (hasValidHandle()) {
			_handle.reset(nullptr);
			_attributes.reset(nullptr);

			_mats.clear();
			_zonals.clear();
			_isInitialized = false;
		}
	}

	std::string _filename;
	std::unique_ptr<H5FileScoped> _handle;
	OmxIndex _zones;
	bool _isInitialized;
	OmxVersion _version;

	OmxCompressionLevel _compressionLevel;

	NamedDatasetObjectCollection<OmxMatrix> _mats;
	NamedDatasetObjectCollection<OmxZonalReference> _zonals;

	std::unique_ptr<OmxAttributeCollection> _attributes;

};

OmxFile::OmxFile(const std::string& filename) : _impl{ new OmxFileImpl{ filename } } {
}

OmxFile::~OmxFile() = default;

void OmxFile::open() {
	if (_impl->hasValidHandle())
		throw OmxFileException("File already open.");

	auto file = H5Fopen(_impl->_filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);

	if (file < 0) {
		throw OmxFileException("Could not open file.");
	}

	_impl->_handle = std::make_unique<H5FileScoped>(file);

	_impl->initialize(false);
}

void OmxFile::openReadOnly() {
	if (_impl->hasValidHandle())
		throw OmxFileException("File already open.");

	auto file = H5Fopen(_impl->_filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

	if (file < 0) {
		throw OmxFileException("Could not open file in readonly mode.");
	}

	_impl->_handle = std::make_unique<H5FileScoped>(file);

	_impl->initialize(false);
}

void OmxFile::openWithTruncate(OmxIndex zones) {
	if (_impl->hasValidHandle())
		throw OmxFileException("File already open.");

	auto file = H5Fcreate(_impl->_filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

	if (file < 0) {
		throw OmxFileException("Could not create file.");
	}

	_impl->_handle = std::make_unique<H5FileScoped>(file);

	_impl->_zones = zones;
	_impl->initialize(true);
}

void OmxFile::openWithCreate(OmxIndex zones) {
	bool exists = false;

	if (std::ifstream(_impl->_filename.c_str())) {
		exists = true;
	}

	if (!exists) {
		openWithTruncate(zones);
	}
	else {
		auto file = H5Fopen(_impl->_filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
		if (file < 0) {
			throw OmxFileException("Could not open file.");
		}
		_impl->_handle = std::make_unique<H5FileScoped>(file);
		_impl->initialize(false);
	}
}

OmxVersion OmxFile::getVersion() const {
	_impl->requireValidHandle();

	return _impl->_version;
}

std::string OmxFile::getFilename() const {
	_impl->requireValidHandle();

	return _impl->_filename;
}

size_t OmxFile::getSize() const {
	_impl->requireValidHandle();

	hsize_t size;
	if (H5Fget_filesize(*_impl->_handle, &size) < 0)
		throw OmxFileException("Couldn't determine the file size.");

	return size;
}

bool OmxFile::isValidMatrixName(const std::string& name) const {
	return _impl->isValidDatasetName(name);
}

OmxMatrix& OmxFile::addMatrix() {
	auto next = _impl->_mats.size() + 1;
	return addMatrix(std::string("matrix") + std::to_string(next));
}
 
OmxMatrix& OmxFile::addMatrix(const std::string& name) {
	return addMatrix(name, DEFAULT_DATA_TYPE, _impl->_compressionLevel);
}

OmxMatrix& OmxFile::addMatrix(const std::string& name, OmxDataType dataType) {
	return addMatrix(name, dataType, _impl->_compressionLevel);
}

OmxMatrix& OmxFile::addMatrix(const std::string& name, OmxDataType dataType, OmxCompressionLevel compressionLevel) {
	if (dataType == OmxDataType::String || dataType == OmxDataType::Unknown)
		throw OmxMatrixException("Unsupported data type for matrices.");

	hsize_t  dims[2] = { _impl->_zones, _impl->_zones };

	hid_t dataset = _impl->addH5Dataset<OmxMatrix, OmxFileException>(&_impl->_mats, name, compressionLevel, 2, dims,
		getH5DataType(dataType),
		nullptr,
		&OmxFileImpl::matrixFactory);

    (void)dataset;

	return getMatrix(name);
}

void OmxFile::removeMatrix(const std::string& name) {
	_impl->removeDataset<OmxMatrix, OmxFileException>(&_impl->_mats, name);
}

void OmxFile::removeMatrix(OmxIndex index) {
	auto& m = getMatrix(index);
	_impl->removeDataset<OmxMatrix, OmxFileException>(&_impl->_mats, m.getName());
}

OmxIndex OmxFile::getZones() const {
	_impl->requireValidHandle();

	return _impl->_zones;
}

OmxCompressionLevel OmxFile::getDefaultCompressionLevel() const {
	_impl->requireValidHandle();

	return _impl->_compressionLevel;
}

std::vector<std::string> OmxFile::getMatrixNames() const {
	return _impl->_mats.getNames();
}

OmxMatrix& OmxFile::getMatrix(OmxIndex index) const {
	_impl->requireValidHandle();

	return *_impl->_mats.getEntry(index);
}

OmxMatrix& OmxFile::getMatrix(const std::string& name) const {
	_impl->requireValidHandle();

	return *_impl->_mats.getEntry(name);
}

bool OmxFile::matrixNameExists(const std::string& name) const {
	return _impl->matrixNameExists(name);
}

OmxIndex OmxFile::getMatrixCount() const {
	_impl->requireValidHandle();

	return _impl->_mats.size();
}

OmxZonalReference& OmxFile::addZonalReference(const std::string& name, OmxDataType dataType) {
	return addZonalReference(name, dataType, OmxCompressionLevel::NoCompression);
}

OmxZonalReference& OmxFile::addZonalReference(const std::string& name, OmxDataType dataType, OmxCompressionLevel compressionLevel) {
	hsize_t  dims[1] = { _impl->_zones };
														  
	hid_t h5Type;
	bool mustClose = false;

	if (dataType != OmxDataType::String) {
		h5Type = getH5DataType(dataType);
	}
	else {
		h5Type = H5Tcopy(H5T_C_S1);
		if ((h5Type = H5Tcopy(H5T_C_S1)) < 0) {
			throw OmxZonalReferenceException("Cannot create data type for zonal reference.");
		}

		if (H5Tset_size(h5Type, H5T_VARIABLE) < 0) {
			throw OmxZonalReferenceException("Cannot initialize size of data type for zonal reference.");
		}

		mustClose = true;

	}

	hid_t dataset = _impl->addH5Dataset<OmxZonalReference, OmxZonalReferenceException>(&_impl->_zonals, name, compressionLevel, 1, dims,
        h5Type,
        nullptr,
        &OmxFileImpl::zonalReferenceFactory);

    (void)dataset;

	if (mustClose) {
		H5Tclose(h5Type);
	}

	return getZonalReference(name);
}

void OmxFile::removeZonalReference(const std::string& name) {
	_impl->removeDataset<OmxZonalReference, OmxZonalReferenceException>(&_impl->_zonals, name);
}

OmxZonalReference& OmxFile::getZonalReference(const std::string& name) const {
	_impl->requireValidHandle();

	return *_impl->_zonals.getEntry(name);
}

bool OmxFile::zonalReferenceExists(const std::string& name) const {
	return _impl->zonalReferenceExists(name);
}

std::vector<std::string> OmxFile::getZonalReferenceNames() const {
	return _impl->_zonals.getNames();
}

OmxIndex OmxFile::getZonalReferenceCount() const {
	return _impl->_zonals.size();
}

OmxAttributeCollection& OmxFile::attributes() const {
	return *_impl->_attributes;
}

void OmxFile::vacuum() {
	std::runtime_error("Not implemented.");
}

void OmxFile::close() {
	_impl->close();
}

}
