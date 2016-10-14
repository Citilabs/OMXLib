#ifndef OMXLIB_H5COMMON_HPP
#define OMXLIB_H5COMMON_HPP

#include <hdf5.h>
#include <hdf5_hl.h>

class H5FileScoped {
public:
	H5FileScoped(hid_t handle) : _handle{ handle } {

	}

	operator hid_t() { return _handle; }

	~H5FileScoped() {
		if (_handle >= 0) {
			H5Fclose(_handle);
			_handle = -1;
		}
	
	}
private:
	hid_t _handle;
};

class H5DatasetScoped {
public:
	H5DatasetScoped(hid_t handle) : _handle{ handle } {

	}

	operator hid_t() { return _handle; }

	~H5DatasetScoped() {
		if (_handle >= 0) {
			H5Dclose(_handle);
			_handle = -1;
		}
	}
private:
	hid_t _handle;
};

class H5DataspaceScoped {
public:
	H5DataspaceScoped(hid_t handle) : _handle{ handle } {

	}

	operator hid_t() { return _handle; }

	~H5DataspaceScoped() {
		if (_handle >= 0) {
			H5Sclose(_handle);
			_handle = -1;
		}
	}
private:
	hid_t _handle;
};

class H5PlistScoped {
public:
	H5PlistScoped(hid_t handle) : _handle{ handle } {

	}

	operator hid_t() { return _handle; }

	~H5PlistScoped() {
		if (_handle >= 0) {
			H5Pclose(_handle);
			_handle = -1;
		}
	}
private:
	hid_t _handle;
};

class H5GroupScoped {
public:
	H5GroupScoped(hid_t handle) : _handle{ handle } {

	}

	operator hid_t() { return _handle; }

	~H5GroupScoped() {
		if (_handle >= 0) {
			H5Gclose(_handle);
			_handle = -1;
		}
	}
private:
	hid_t _handle;
};

class H5TypeScoped {
public:
	H5TypeScoped(hid_t handle) : _handle{ handle } {

	}

	operator hid_t() { return _handle; }

	~H5TypeScoped() {
		if (_handle >= 0) {
			H5Tclose(_handle);
			_handle = -1;
		}
	}
private:
	hid_t _handle;
};

class H5AttributeScoped {
public:
	H5AttributeScoped(hid_t handle) : _handle{ handle } {

	}

	operator hid_t() { return _handle; }

	~H5AttributeScoped() {
		if (_handle >= 0) {
			H5Aclose(_handle);
			_handle = -1;
		}
	}
private:
	hid_t _handle;
};
#endif