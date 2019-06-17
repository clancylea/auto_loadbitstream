/*
 * Empty C++ Application
 */
#include <iostream>

using namespace std;
#include <string>
#include <limits.h>
#include <stdlib.h>
#include <stdexcept>
#include <chrono>
#include <vector>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <thread>


class GeneralUtils {
    public:
        typedef std::chrono::high_resolution_clock::time_point chrono_t;
        static bool dirExists(std::string const &path);
        static bool fileExists(std::string const &path);
        static std::string dirname(std::string const &);
        static std::string filename(std::string const &);
        static std::string abspath(std::string const &);
        static std::string abspathReference(std::string const &, std::string const &);
        static std::string readStringFile(std::string const &);
        static void readStringFile(std::string &, std::string const &);
        static std::vector<char> readBinaryFile(std::string const &);
        static void readBinaryFile(std::vector<char> &, std::string const &);
        static void configureFabric(std::vector<char> const &buffer);
        static void loadBitstreamFile(std::string const &);
        static unsigned int padTo(unsigned int , unsigned int );
        static signed long long getTime(chrono_t &);
        static chrono_t getTimer();
    private:
        GeneralUtils() {};
        ~GeneralUtils() {};
};


GeneralUtils::chrono_t GeneralUtils::getTimer() {
    return std::chrono::high_resolution_clock::now();
}

signed long long GeneralUtils::getTime(GeneralUtils::chrono_t &timer) {
    chrono_t now =  std::chrono::high_resolution_clock::now();
    return (signed long long) std::chrono::duration_cast<std::chrono::microseconds>(now - timer).count();
}

std::string GeneralUtils::abspathReference(std::string const &path, std::string const &ref) {
    std::string result;
    std::size_t slash = path.find_first_of("\\/");
    if (slash != std::string::npos && slash == 0) {
        return GeneralUtils::abspath(path);
    } else {
        return GeneralUtils::abspath(ref + "/" + path);
    }
}

std::string GeneralUtils::abspath(std::string const &path) {
    char *p = (char *) malloc(PATH_MAX * sizeof(char));
    if (realpath(path.c_str(), p) == NULL) {
        free(p);
        throw std::runtime_error("Could not find " + path+ "!");
    }
    std::string result(p);
    free(p);
    return result;
}

std::string GeneralUtils::dirname(std::string const &path) {
    std::size_t slash = path.find_last_of("\\/");
    if (slash != std::string::npos) {
        return path.substr(0, slash);
    } else {
        return std::string("./");
    }
}

std::string GeneralUtils::filename(std::string const &path) {
    std::size_t slash = path.find_last_of("\\/");
    if (slash != std::string::npos) {
        return path.substr(slash+1, std::string::npos);
    } else {
        return std::string("");
    }
}

std::string GeneralUtils::readStringFile(std::string const &filename) {
    std::string buf;
    GeneralUtils::readStringFile(buf, filename);
    return buf;
}

void GeneralUtils::readStringFile(std::string &buffer, std::string const &filename) {
    std::vector<char> buf;
    GeneralUtils::readBinaryFile(buf, filename);
    buffer = std::string(buf.begin(), buf.end());
}

std::vector<char> GeneralUtils::readBinaryFile(std::string const &filename) {
    std::vector<char> buf;
    GeneralUtils::readBinaryFile(buf, filename);
    return buf;
}

void GeneralUtils::readBinaryFile(std::vector<char> &buffer, std::string const &filename) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + filename);
    } else {
        file.seekg(0, std::ios::end);
        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        buffer.resize(fileSize);
        file.read(buffer.data(), fileSize);
        if (file.gcount() != fileSize) {
            throw std::runtime_error("Could not read all file characters!");
        }
        file.close();
    }
}

unsigned int GeneralUtils::padTo(unsigned int num, unsigned int padTo) {
    if(num % padTo == 0) {
        return num;
    } else {
        return num + padTo - (num % padTo);
    }
}

bool GeneralUtils::fileExists(std::string const &path) {
    struct stat info;
    if ( stat(path.c_str(), &info) != 0 ) {
        return false;
    } else if( info.st_mode & S_IFDIR ) {
        return false;
    }
    return true;
}

bool GeneralUtils::dirExists(std::string const &path) {
    struct stat info;
    if ( stat(path.c_str(), &info) != 0 ) {
        return false;
    } else if (info.st_mode & S_IFDIR) {
        return true;
    }
    return false;
}




void GeneralUtils::configureFabric(std::vector<char> const &buffer) {
    if (GeneralUtils::fileExists("/dev/xdevcfg")) {
        //We are on Kernel 4.6 or lower
        FILE *fd = fopen("/dev/xdevcfg", "w");
        if (fd == NULL) {
            throw std::runtime_error("Could not open /dev/xdevcfg device");
        }
        size_t written = fwrite(buffer.data(), sizeof(char), buffer.size(), fd);
        if (written != buffer.size()) {
            throw std::runtime_error("Could not write complete bitstream to /dev/xdevcfg");
        }
        fclose(fd);
    } else if (GeneralUtils::fileExists("/sys/class/fpga_manager/fpga0/firmware")) {
        const char bitstreamFile[] = "fabric_bitstream.bin";

        std::ofstream bitstream(std::string("/lib/firmware/") + bitstreamFile, std::ios::out | std::ios::binary | std::ios::trunc);
        bitstream.write(buffer.data(), buffer.size());
        bitstream.close();

        std::ofstream firmware("/sys/class/fpga_manager/fpga0/firmware");
        firmware << bitstreamFile;
        firmware.close();
        if (!firmware.good()) {
            throw std::runtime_error("Fabric configuration through fpga_manager failed!");
        }
        // Loading was successful, wait for half a second before using the fpga!
        // or else the board is gone... praise the fpga_manager
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    } else {
        // TODO load AWS bitstream
    }
}

void GeneralUtils::loadBitstreamFile(std::string const &bitstreamPath) {
    GeneralUtils::configureFabric(GeneralUtils::readBinaryFile(bitstreamPath));
}


int main()
{
    // load test images from binary files
	std::string const &bitstreamPath = "design_1_wrapper.bin";
	GeneralUtils::loadBitstreamFile(bitstreamPath);
	printf("loadBitstreamFile ok");

}
