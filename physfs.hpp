#pragma once

#include <physfs/physfs.h>
#include <string>
#include <vector>
#include <iostream>

namespace physfs {
    typedef enum {
        READ,
        WRITE,
        APPEND
    } mode;

    using std::string;

    typedef std::vector<string> StringList;

    typedef PHYSFS_uint8 uint8;

    typedef PHYSFS_sint8 sint8;

    typedef PHYSFS_uint16 uint16;

    typedef PHYSFS_sint16 sint16;

    typedef PHYSFS_uint32 uint32;

    typedef PHYSFS_sint32 sint32;

    typedef PHYSFS_uint64 uint64;

    typedef PHYSFS_sint64 sint64;

    typedef PHYSFS_StringCallback StringCallback;

    typedef PHYSFS_EnumFilesCallback EnumFilesCallback;

    typedef PHYSFS_Version Version;

    typedef PHYSFS_Allocator Allocator;

    typedef PHYSFS_ArchiveInfo ArchiveInfo;

    typedef PHYSFS_ErrorCode ErrorCode;

    typedef std::vector<ArchiveInfo> ArchiveInfoList;

    typedef uint64 size_t;

    class base_fstream {
    protected:
        PHYSFS_File * const file;
    public:
        base_fstream(PHYSFS_File * file);
        virtual ~base_fstream();
        size_t length();
    };

    class ifstream : public base_fstream, public std::istream {
    public:
        ifstream(string const & filename);
        virtual ~ifstream();
    };

    class ofstream : public base_fstream, public std::ostream {
    public:
        ofstream(string const & filename, mode writeMode = WRITE);
        virtual ~ofstream();
    };

    class fstream : public base_fstream, public std::iostream {
    public:
        fstream(string const & filename, mode openMode = READ);
        virtual ~fstream();
    };

    Version getLinkedVersion();

    int init(char const * argv0);

    int deinit();

    ArchiveInfoList supportedArchiveTypes();

    string getDirSeparator();

    void permitSymbolicLinks(bool allow);

    StringList getCdRomDirs();

    void getCdRomDirs(StringCallback callback, void * extra);

    string getBaseDir();

    string getUserDir();

    string getWriteDir();

    void setWriteDir(string const & newDir);

    void removeFromSearchPath(string const & oldDir);

    StringList getSearchPath();

    void getSearchPath(StringCallback callback, void * extra);

    void setSaneConfig(string const & orgName, string const & appName, string const & archiveExt, bool includeCdRoms, bool archivesFirst);

    void mkdir(string const & dirName);

    void deleteFile(string const & filename);

    string getRealDir(string const & filename);

    StringList enumerateFiles(string const & directory);

    void enumerateFiles(string const & directory, EnumFilesCallback callback, void * extra);

    bool exists(string const & filename);

    bool isDirectory(string const & filename);

    bool isSymbolicLink(string const & filename);

    sint64 getLastModTime(string const & filename);

    bool isInit();

    bool symbolicLinksPermitted();

    void setAllocator(Allocator const * allocator);

    void mount(string const & newDir, string const & mountPoint, bool appendToPath);

    string getMountPoint(string const & dir);

    namespace utility {

        sint16 swapSLE16(sint16 value);

        uint16 swapULE16(uint16 value);

        sint32 swapSLE32(sint32 value);

        uint32 swapULE32(uint32 value);

        sint64 swapSLE64(sint64 value);

        uint64 swapULE64(uint64 value);

        sint16 swapSBE16(sint16 value);

        uint16 swapUBE16(uint16 value);

        sint32 swapSBE32(sint32 value);

        uint32 swapUBE32(uint32 value);

        sint64 swapSBE64(sint64 value);

        uint64 swapUBE64(uint64 value);

        string utf8FromUcs4(uint32 const * src);

        string utf8ToUcs4(char const * src);

        string utf8FromUcs2(uint16 const * src);

        string utf8ToUcs2(char const * src);

        string utf8FromLatin1(char const * src);

    }



#ifdef PHYFSPP_IMPL

#include <streambuf>
#include <stdexcept>
#include <string.h>

using std::streambuf;
using std::ios_base;

class fbuf : public streambuf {
private:
	fbuf(const fbuf & other);
	fbuf& operator=(const fbuf& other);

	int_type underflow() {
		if (PHYSFS_eof(file)) {
			return traits_type::eof();
		}
		size_t bytesRead = PHYSFS_read(file, buffer, 1, bufferSize);
		if (bytesRead < 1) {
			return traits_type::eof();
		}
		setg(buffer, buffer, buffer + bytesRead);
		return (unsigned char) *gptr();
	}

	pos_type seekoff(off_type pos, ios_base::seekdir dir, ios_base::openmode mode) {
		switch (dir) {
		case std::ios_base::beg:
			PHYSFS_seek(file, pos);
			break;
		case std::ios_base::cur:
			// subtract characters currently in buffer from seek position
			PHYSFS_seek(file, (PHYSFS_tell(file) + pos) - (egptr() - gptr()));
			break;
		case std::ios_base::end:
			PHYSFS_seek(file, PHYSFS_fileLength(file) + pos);
			break;
		}
		if (mode & std::ios_base::in) {
			setg(egptr(), egptr(), egptr());
		}
		if (mode & std::ios_base::out) {
			setp(buffer, buffer);
		}
		return PHYSFS_tell(file);
	}

	pos_type seekpos(pos_type pos, std::ios_base::openmode mode) {
		PHYSFS_seek(file, pos);
		if (mode & std::ios_base::in) {
			setg(egptr(), egptr(), egptr());
		}
		if (mode & std::ios_base::out) {
			setp(buffer, buffer);
		}
		return PHYSFS_tell(file);
	}

	int_type overflow( int_type c = traits_type::eof() ) {
		if (pptr() == pbase() && c == traits_type::eof()) {
			return 0; // no-op
		}
		if (PHYSFS_write(file, pbase(), pptr() - pbase(), 1) < 1) {
			return traits_type::eof();
		}
		if (c != traits_type::eof()) {
			if (PHYSFS_write(file, &c, 1, 1) < 1) {
				return traits_type::eof();
			}
		}

		return 0;
	}

	int sync() {
		return overflow();
	}

	char * buffer;
	size_t const bufferSize;
protected:
	PHYSFS_File * const file;
public:
	fbuf(PHYSFS_File * file, std::size_t bufferSize = 2048) : file(file), bufferSize(bufferSize) {
		buffer = new char[bufferSize];
		char * end = buffer + bufferSize;
		setg(end, end, end);
		setp(buffer, end);
	}

	~fbuf() {
		sync();
		delete [] buffer;
	}
};

base_fstream::base_fstream(PHYSFS_File* file) : file(file) {
    if (file == NULL) {
        throw std::invalid_argument("attempted to construct fstream with NULL ptr");
    }
}

base_fstream::~base_fstream() {
	PHYSFS_close(file);
}

physfs::size_t base_fstream::length() {
	return PHYSFS_fileLength(file);
}

PHYSFS_File* openWithMode(char const * filename, mode openMode) {
    PHYSFS_File* file = NULL;
	switch (openMode) {
	case WRITE:
		file = PHYSFS_openWrite(filename);
        break;
	case APPEND:
		file = PHYSFS_openAppend(filename);
        break;
	case READ:
		file = PHYSFS_openRead(filename);
	}
    if (file == NULL) {
        throw std::invalid_argument("file not found: " + std::string(filename));
    }
    return file;
}

ifstream::ifstream(const string& filename)
	: base_fstream(openWithMode(filename.c_str(), READ)), std::istream(new fbuf(file)) {}

ifstream::~ifstream() {
	delete rdbuf();
}

ofstream::ofstream(const string& filename, mode writeMode)
	: base_fstream(openWithMode(filename.c_str(), writeMode)), std::ostream(new fbuf(file)) {}

ofstream::~ofstream() {
	delete rdbuf();
}

fstream::fstream(const string& filename, mode openMode)
	: base_fstream(openWithMode(filename.c_str(), openMode)), std::iostream(new fbuf(file)) {}

fstream::~fstream() {
	delete rdbuf();
}

Version getLinkedVersion() {
	Version version;
	PHYSFS_getLinkedVersion(&version);
	return version;
}

int init(const char* argv0) {
	return PHYSFS_init(argv0);
}

int deinit() {
    return PHYSFS_deinit();
}

ArchiveInfoList supportedArchiveTypes() {
	ArchiveInfoList list;
	for (const ArchiveInfo** archiveType = PHYSFS_supportedArchiveTypes(); *archiveType != NULL; archiveType++) {
		list.push_back(**archiveType);
	}
	return list;
}

ErrorCode getLastErrorCode() {
    return PHYSFS_getLastErrorCode();
}

string getErrorByCode(ErrorCode code) {
    return PHYSFS_getErrorByCode(code);
}

string getDirSeparator() {
	return PHYSFS_getDirSeparator();
}

void permitSymbolicLinks(bool allow) {
	PHYSFS_permitSymbolicLinks(allow);
}

StringList getCdRomDirs() {
	StringList dirs;
	char ** dirBegin = PHYSFS_getCdRomDirs();
	for (char ** dir = dirBegin; *dir != NULL; dir++) {
		dirs.push_back(*dir);
	}
	PHYSFS_freeList(dirBegin);
	return dirs;
}

void getCdRomDirs(StringCallback callback, void * extra) {
	PHYSFS_getCdRomDirsCallback(callback, extra);
}

string getBaseDir() {
	return PHYSFS_getBaseDir();
}

string getUserDir() {
	return PHYSFS_getUserDir();
}

string getWriteDir() {
	return PHYSFS_getWriteDir();
}

void setWriteDir(const string& newDir) {
	PHYSFS_setWriteDir(newDir.c_str());
}

void removeFromSearchPath(const string& oldDir) {
	PHYSFS_removeFromSearchPath(oldDir.c_str());
}

StringList getSearchPath() {
	StringList pathList;
	char ** pathBegin = PHYSFS_getSearchPath();
	for (char ** path = pathBegin; *path != NULL; path++) {
		pathList.push_back(*path);
	}
	PHYSFS_freeList(pathBegin);
	return pathList;
}

void getSearchPath(StringCallback callback, void * extra) {
	PHYSFS_getSearchPathCallback(callback, extra);
}

void setSaneConfig(const string& orgName, const string& appName,
		const string& archiveExt, bool includeCdRoms, bool archivesFirst) {
	PHYSFS_setSaneConfig(orgName.c_str(), appName.c_str(), archiveExt.c_str(), includeCdRoms, archivesFirst);
}

void mkdir(const string& dirName) {
	PHYSFS_mkdir(dirName.c_str());
}

void deleteFile(const string& filename) {
	PHYSFS_delete(filename.c_str());
}

string getRealDir(const string& filename) {
	return PHYSFS_getRealDir(filename.c_str());
}

StringList enumerateFiles(const string& directory) {
	StringList files;
	char ** listBegin = PHYSFS_enumerateFiles(directory.c_str());
	for (char ** file = listBegin; *file != NULL; file++) {
		files.push_back(*file);
	}
	PHYSFS_freeList(listBegin);
	return files;
}

void enumerateFiles(const string& directory, EnumFilesCallback callback, void * extra) {
	PHYSFS_enumerateFilesCallback(directory.c_str(), callback, extra);
}

bool exists(const string& filename) {
	return PHYSFS_exists(filename.c_str());
}

bool isDirectory(const string& filename) {
	return PHYSFS_isDirectory(filename.c_str());
}

bool isSymbolicLink(const string& filename) {
	return PHYSFS_isSymbolicLink(filename.c_str());
}

sint64 getLastModTime(const string& filename) {
	return PHYSFS_getLastModTime(filename.c_str());
}

bool isInit() {
	return PHYSFS_isInit();
}

bool symbolicLinksPermitted() {
	return PHYSFS_symbolicLinksPermitted();
}

void setAllocator(const Allocator* allocator) {
	PHYSFS_setAllocator(allocator);
}

void mount(const string& newDir, const string& mountPoint, bool appendToPath) {
	PHYSFS_mount(newDir.c_str(), mountPoint.c_str(), appendToPath);
}

string getMountPoint(const string& dir) {
	return PHYSFS_getMountPoint(dir.c_str());
}

sint16 utility::swapSLE16(sint16 value) {
	return PHYSFS_swapSLE16(value);
}

uint16 utility::swapULE16(uint16 value) {
	return PHYSFS_swapULE16(value);
}

sint32 utility::swapSLE32(sint32 value) {
	return PHYSFS_swapSLE32(value);
}

uint32 utility::swapULE32(uint32 value) {
	return PHYSFS_swapULE32(value);
}

sint64 utility::swapSLE64(sint64 value) {
	return PHYSFS_swapSLE64(value);
}

uint64 utility::swapULE64(uint64 value) {
	return PHYSFS_swapULE64(value);
}

sint16 utility::swapSBE16(sint16 value) {
	return PHYSFS_swapSBE16(value);
}

uint16 utility::swapUBE16(uint16 value) {
	return PHYSFS_swapUBE16(value);
}

sint32 utility::swapSBE32(sint32 value) {
	return PHYSFS_swapSBE32(value);
}

uint32 utility::swapUBE32(uint32 value) {
	return PHYSFS_swapUBE32(value);
}

sint64 utility::swapSBE64(sint64 value) {
	return PHYSFS_swapSBE64(value);
}

uint64 utility::swapUBE64(uint64 value) {
	return PHYSFS_swapUBE64(value);
}

string utility::utf8FromUcs4(const uint32* src) {
	string value;
	std::size_t length = strlen((char*) src);
	char * buffer = new char[length]; // will be smaller than len
	PHYSFS_utf8FromUcs4(src, buffer, length);
	value.append(buffer);
	return value;
}

string utility::utf8ToUcs4(const char* src) {
	string value;
	std::size_t length = strlen(src) * 4;
	char * buffer = new char[length]; // will be smaller than len
	PHYSFS_utf8ToUcs4(src, (uint32*) buffer, length);
	value.append(buffer);
	return value;
}

string utility::utf8FromUcs2(const uint16* src) {
	string value;
	std::size_t length = strlen((char*) src);
	char * buffer = new char[length]; // will be smaller than len
	PHYSFS_utf8FromUcs2(src, buffer, length);
	value.append(buffer);
	return value;
}

string utility::utf8ToUcs2(const char* src) {
	string value;
	std::size_t length = strlen(src) * 2;
	char * buffer = new char[length]; // will be smaller than len
	PHYSFS_utf8ToUcs2(src, (uint16*) buffer, length);
	value.append(buffer);
	return value;
}

string utility::utf8FromLatin1(const char* src) {
	string value;
	std::size_t length = strlen((char*) src) * 2;
	char * buffer = new char[length]; // will be smaller than len
	PHYSFS_utf8FromLatin1(src, buffer, length);
	value.append(buffer);
	return value;
}

#endif

} // end namespace physfs
