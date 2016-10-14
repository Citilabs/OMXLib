#ifndef OMX_EXCEPTION_HPP
#define OMX_EXCEPTION_HPP

#include <stdexcept>

namespace omx {

struct OmxException : public std::runtime_error {
	OmxException(const std::string& msg) : std::runtime_error(msg){};
};

struct OmxFileException : public OmxException {
	OmxFileException(const std::string& msg) : OmxException(msg){};
};
struct OmxMatrixException : public OmxException {
	OmxMatrixException(const std::string& msg) : OmxException(msg){};
};

struct OmxZonalReferenceException : public OmxException {
	OmxZonalReferenceException(const std::string& msg) : OmxException(msg){};
};

struct OmxAttributexException : public OmxException {
	OmxAttributexException(const std::string& msg) : OmxException(msg){};
};

}
#endif