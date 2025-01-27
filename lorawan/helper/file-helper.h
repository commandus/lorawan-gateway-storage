/*
 * @file file-helper.h
 */
#ifndef FILE_HELPER_H
#define FILE_HELPER_H     1

#include <string>
#include <vector>

namespace file {
    bool mkDir(const std::string &path);
	bool rmDir(const std::string &path);
	bool rmFile(const std::string &fn);
#if defined(_MSC_VER) || defined(__MINGW32__)
	bool rmAllDir(const char* path);
#endif
	/**
	 * Return list of files in specified path
	 * @param path
	 * @param flags 0- as is, 1- full path, 2- relative (remove parent path)
	 * @param retval can be NULL
	 * @return count files
	 * FreeBSD fts.h fts_*()
	 */
	size_t filesInPath
	(
		const std::string &path,
		const std::string &suffix,
		int flags,
		std::vector<std::string> *retval
	);

    bool fileExists(const std::string &fileName);
    std::string expandFileName(const std::string &relativeName);
}

/**
 * @return last modification file time, seconds since unix epoch
 */
time_t fileModificationTime(
	const std::string &fileName
);

class URL {
private:
    void parse(const std::string &url);
    void clear();
public:
    std::string protocol;
    std::string host;
    std::string path;
    std::string query;
    URL(const std::string &url);
    // get query parameter value (first one)
    std::string get(const std::string &name);
    // get query parameter value as integer (first one)
    int getInt(const std::string &name);
};

std::string getCurrentDir();

#endif
