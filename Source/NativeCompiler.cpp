#include "NativeCompiler.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace ruuna {
namespace {

class TemporaryFile {
public:
    TemporaryFile() {
#ifdef _WIN32
        char directory[MAX_PATH];
        const DWORD length = GetTempPathA(MAX_PATH, directory);
        if (length == 0 || length >= MAX_PATH) {
            throw std::runtime_error("cannot get temporary directory");
        }
        char path[MAX_PATH];
        if (GetTempFileNameA(directory, "run", 0, path) == 0) {
            throw std::runtime_error("cannot create temporary C file");
        }
        path_ = path;
#else
        char pattern[] = "/tmp/ruuna-XXXXXX.c";
        const int descriptor = mkstemps(pattern, 2);
        if (descriptor == -1) {
            throw std::runtime_error("cannot create temporary C file");
        }
        if (close(descriptor) == -1) {
            unlink(pattern);
            throw std::runtime_error("cannot close temporary C file");
        }
        path_ = pattern;
#endif
    }

    ~TemporaryFile() {
        if (!path_.empty()) {
#ifdef _WIN32
            DeleteFileA(path_.c_str());
#else
            unlink(path_.c_str());
#endif
        }
    }

    const std::string& path() const {
        return path_;
    }

private:
    std::string path_;
};

bool runSystemCompiler(const std::string& cFile, const std::string& output) {
#ifdef _WIN32
    const char* const arguments[] = {
        "gcc",
        "-std=c11",
        "-O2",
        "-x",
        "c",
        cFile.c_str(),
        "-o",
        output.c_str(),
        nullptr
    };
    const intptr_t result = _spawnvp(_P_WAIT, arguments[0], arguments);
    if (result == -1) {
        std::perror("ruumake: cannot execute gcc");
        return false;
    }
    return result == 0;
#else
    const pid_t child = fork();
    if (child == -1) {
        throw std::runtime_error("cannot start the system C compiler");
    }
    if (child == 0) {
        char* const arguments[] = {
            const_cast<char*>("cc"),
            const_cast<char*>("-std=c11"),
            const_cast<char*>("-O2"),
            const_cast<char*>(cFile.c_str()),
            const_cast<char*>("-o"),
            const_cast<char*>(output.c_str()),
            nullptr
        };
        execvp(arguments[0], arguments);
        std::perror("ruumake: cannot execute cc");
        _exit(127);
    }

    int status = 0;
    while (waitpid(child, &status, 0) == -1) {
        if (errno != EINTR) {
            throw std::runtime_error("failed while waiting for the C compiler");
        }
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif
}

}  // namespace

bool compileNative(const std::string& generatedC, const std::string& output) {
    TemporaryFile temporaryC;
    {
        std::ofstream file(temporaryC.path(), std::ios::binary);
        if (!file) {
            throw std::runtime_error("cannot write temporary C file");
        }
        file << generatedC;
        file.close();
        if (!file) {
            throw std::runtime_error("cannot write temporary C file");
        }
    }
    return runSystemCompiler(temporaryC.path(), output);
}

}  // namespace ruuna
