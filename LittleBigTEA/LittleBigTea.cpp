// LittleBigTea.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#include <algorithm>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <cstdarg>
#include <ctime>

#include <stdint.h>

#define uint unsigned int
#define longlong long long
#define ulonglong unsigned long long

#define DELTA 0x9e3779b9
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z)))
#define SWAP_INT16(x) ((x >> 8 | x << 8))
#define SWAP_INT32(x) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24)) 

struct XXTeaKeys {
	uint key1 = 0;
	uint key2 = 0;
	uint key3 = 0;
	uint key4 = 0;
};

static XXTeaKeys keys{ 0x1b70cbd, 0x149607d6, 0x7f94dd5, 0x10db8ca0 };

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[90];
	localtime_s(&tstruct, &now);
	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
	// for more information about date/time format
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

	return buf;
}

class CLog {
	public:
		enum { All = 0, Spam, Debug, Info, Warning, Error, Fatal, None };
		static void Write(int nLevel, const char *szFormat, ...);
		static void SetLevel(int nLevel);

	protected:
		static void CheckInit();
		static void Init();

	private:
		CLog();
		static bool m_bInitialised;
		static int  m_nLevel;

#ifdef _DEBUG
		static bool m_bIsSavingLogs;
		static bool m_bWantLevelLogging;
		static std::string logName;
#endif
};

bool CLog::m_bInitialised;
int  CLog::m_nLevel;

#ifdef _DEBUG
bool CLog::m_bIsSavingLogs = false;
bool CLog::m_bWantLevelLogging = true;
std::string CLog::logName;
#endif

void CLog::Write(int nLevel, const char *szFormat, ...) {
	CheckInit();
	va_list args;
	va_start(args, szFormat);

#ifdef _DEBUG
	if (m_bIsSavingLogs) {
		char buffer[0x1000];
		std::fstream logFile;

		if (m_bWantLevelLogging && nLevel >= m_nLevel) {
			vsnprintf(buffer, sizeof(buffer), szFormat, args);
			logFile.open(logName, std::ios::out | std::ios::binary | std::ios::app);
			if (logFile.is_open()) {
				logFile << buffer;
				logFile.close();
			} else {
				std::cout << "Failed to open log file " << logName.c_str() << "!\n";
				m_bIsSavingLogs = false;
			}
		} else if (!m_bWantLevelLogging) {
			vsnprintf(buffer, sizeof(buffer), szFormat, args);
			logFile.open(logName, std::ios::out | std::ios::binary | std::ios::app);
			if (logFile.is_open()) {
				logFile << buffer;
				logFile.close();
			} else {
				std::cout << "Failed to open log file " << logName.c_str() << "!\n";
				m_bIsSavingLogs = false;
			}
		}
	}
#endif 
	if (nLevel >= m_nLevel) {
		vprintf(szFormat, args);
	}
	va_end(args);
}

void CLog::SetLevel(int nLevel) {
	m_nLevel = nLevel;
	m_bInitialised = true;
}

void CLog::CheckInit() {
	if (!m_bInitialised) {
		Init();
	}
}

void CLog::Init() {
#ifdef _DEBUG
	int nDfltLevel(CLog::Debug);
	m_bIsSavingLogs = true;

	std::string time = currentDateTime();
	logName = "littlebigtea-log-" + time + ".txt";
	std::replace(logName.begin(), logName.end(), ':', '.');
#else
	int nDfltLevel(CLog::Info);
#endif
	SetLevel(nDfltLevel);
}

void SwapStringU16(std::string &str) {
	size_t strIntSize = str.size() / 2;
	uint16_t *strInt = (uint16_t *)str.data();

	// Reverse the endian for later use.
	for (size_t a = 0; a < strIntSize; ++a) {
		strInt[a] = SWAP_INT16(strInt[a]);
	}
}

void SwapStringU32(std::string &str) {
	size_t strIntSize = str.size() / 4;
	uint32_t *strInt = (uint32_t *)str.data();

	// Reverse the endian for later use.
	for (size_t a = 0; a < strIntSize; ++a) {
		strInt[a] = SWAP_INT32(strInt[a]);
	}
}

void btea(uint32_t *v, int n, uint32_t const key[4]) {
	uint32_t y, z, sum;
	unsigned p, rounds, e;
	if (n > 1) {          /* Coding Part */
		rounds = 6 + 52 / n;
		sum = 0;
		z = v[n - 1];
		do {
			sum += DELTA;
			e = (sum >> 2) & 3;
			for (p = 0; p < n - 1; p++) {
				y = v[p + 1];
				z = v[p] += MX;
			}
			y = v[0];
			z = v[n - 1] += MX;
		} while (--rounds);
	}
	else if (n < -1) {  /* Decoding Part */
		n = -n;
		rounds = 6 + 52 / n;
		sum = rounds * DELTA;
		y = v[0];
		do {
			e = (sum >> 2) & 3;
			for (p = n - 1; p > 0; p--) {
				z = v[p - 1];
				y = v[p] -= MX;
			}
			z = v[n - 1];
			y = v[0] -= MX;
			sum -= DELTA;
		} while (--rounds);
	}
}

inline size_t get_fstream_size(std::fstream &stream) {
	std::streampos begin, end;
	size_t size = 0;
	begin = stream.tellg();
	stream.seekg(0, std::ios::end);
	end = stream.tellg();
	stream.seekg(0, std::ios::beg);
	size = end - begin;
	return size;
}

template <typename ...Args>
std::string stringWithFormat(const std::string& format, Args && ...args) {
	auto size = std::snprintf(nullptr, 0, format.c_str(), std::forward<Args>(args)...);
	std::string output(size + 1, '\0');
	sprintf_s(&output[0], output.size(), format.c_str(), std::forward<Args>(args)...);
	return output;
}

std::string process_tea_file(std::string filename, bool isEncrypted = true) {
	std::fstream file;

	file.open(filename, std::ios::in | std::ios::binary);
	if (file.is_open()) {
		size_t size = get_fstream_size(file);
		CLog::Write(CLog::Debug, "File '%s' has a size of 0x%X bytes.\n", filename.data(), size);

		if (size < 4) {
			CLog::Write(CLog::Error, "File '%s' is too small to be processed!\n", filename.data());
			file.close();
			return "BAD_DATA";
		}

		std::string memblockHolder((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

		// Reverse our endian for processing.
		SwapStringU32(memblockHolder);

		int n = size / 4;
		if (isEncrypted) {
			CLog::Write(CLog::Info, "File '%s' is encrypted! Decrypting file.\n", filename.data());
			btea((uint *)memblockHolder.data(), -n, (uint *)&keys);
		} else {
			CLog::Write(CLog::Info, "File '%s' is decrypted! Encrypting file.\n", filename.data());
			btea((uint *)memblockHolder.data(), n, (uint *)&keys);
		}

		// Reverse the endian for writing.
		SwapStringU32(memblockHolder);

		// Close our reading file.
		file.close();

		return memblockHolder;
	} else {
		CLog::Write(CLog::Warning, "Unable to open file '%s'!\n", filename.data());
		return "BAD_DATA";
	}
}

void process_big_fart_profile(std::string filename, int bigFartLen = 0) {
	std::fstream file;
	std::string chunkData("");
	bool isEncrypted = true;


	file.open(filename, std::ios::in | std::ios::binary);
	if (file.is_open()) {
		size_t size = get_fstream_size(file);
		CLog::Write(CLog::Debug, "File '%s' has a size of 0x%X bytes.\n", filename.data(), size);
		size = size - 4;

		if (size < 4) {
			CLog::Write(CLog::Error, "File '%s' is too small to be processed!\n", filename.data());
			file.close();
			return;
		}

		std::string memblockHolder((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
	    isEncrypted = memblockHolder.find("BPRb") == std::string::npos && memblockHolder.find("PLNb") == std::string::npos && memblockHolder.find("LVLb") == std::string::npos;

		if (bigFartLen > 1) {
			for (int a = 0; a < (bigFartLen - 1); ++a) {
				std::string newPath = filename.substr(0, filename.find_last_of("\\/"));
				newPath += "/BIG%03d";
				newPath = stringWithFormat(newPath, a);
				std::string teaData = process_tea_file(newPath, isEncrypted);
				chunkData.append(teaData);
			}
		}

		// Reverse the endian for processing.
		SwapStringU32(memblockHolder);

		int n = size / 4;
		if (isEncrypted) {
			CLog::Write(CLog::Info, "File '%s' is encrypted! Decrypting file.\n", filename.data());
			btea((uint *)memblockHolder.data(), -n, (uint *)&keys);
		} else {
			CLog::Write(CLog::Info, "File '%s' is decrypted! Encrypting file.\n", filename.data());
			btea((uint *)memblockHolder.data(), n, (uint *)&keys);
		}

		// Reverse the endian for writing.
		SwapStringU32(memblockHolder);

		// Close our reading file.
		file.close();

		if (bigFartLen > 1) {
			chunkData.append(memblockHolder);
			memblockHolder = chunkData;
		}

		file.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
		file.write(memblockHolder.data(), memblockHolder.size() - 4);
		file << "FAR4";
		file.close();
	} else {
		CLog::Write(CLog::Warning, "Unable to open file '%s'!\n", filename.data());
	}
}

void process_ipr_file(std::string filename) {
	std::fstream file;
	bool isEncrypted = false;
	std::stringstream fileDatastream;
	std::string fileData;

	// Open our file.
	file.open(filename, std::ios::in | std::ios::binary);

	// Check if our file is opened.
	if (file.is_open()) {
		size_t size = get_fstream_size(file);
		CLog::Write(CLog::Debug, "File '%s' has a size of 0x%X bytes.\n", filename.data(), size);

		// If the file is too small. There's no way something is in it.
		if (size < 4) {
			CLog::Write(CLog::Error, "File '%s' is too small to be processed!\n", filename.data());
			file.close();
		}

		// We need a buffer for our header.
		char *header = new char[5];
		header[4] = '\0';

		// Read our files header.
		file.read(header, 4);

		// If the header matches an encrpyted IPR header, We will decrypt the IPR.
		isEncrypted = strcmp(header, "IPRe") == 0;

		if (!isEncrypted) {
			CLog::Write(CLog::Error, "Re-encryption of IPR files is currently not supported. Aborting.\n");
			file.close();
			return;
		}

		// We no longer need the header. Free it.
		delete[] header;

		// Setup our retainer for the data.
		std::string memblockHolder((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

		// Get the timestamp data from the IPR.
		std::string timestampData = memblockHolder.substr(0, 0x8);
		memblockHolder.erase(0, 0x8);

		// In the future we might know what the flags actually are.
		std::string flags = memblockHolder.substr(0, 0x6);
		memblockHolder.erase(0, 0x6);

		// Reverse the endian for decryption or encryption.
		long long memblockHolderSize = memblockHolder.size() / 4;
		uint *memblockHolderInt = (uint *)memblockHolder.data();
		for (int a = 0; a < memblockHolderSize; ++a) {
			memblockHolderInt[a] = SWAP_INT32(memblockHolderInt[a]);
		}

		// Get the size of the data we will perform TEA on.
		uint32_t teaDataSize = memblockHolderInt[0] + 4;
		std::string teaDataSizeString = memblockHolder.substr(0, 4);
		memblockHolder.erase(0, 4);
		CLog::Write(CLog::Debug, "Read TEA data size 0x%X!\n", teaDataSize);

		// Extract the data to perform TEA on. 
		std::string teaData = memblockHolder.substr(0, teaDataSize);
		memblockHolder.erase(0, teaDataSize);
		CLog::Write(CLog::Debug, "Working on TEA data with size %X for IPR!\n", teaData.size());

		int n = teaData.size() / 4;
		if (isEncrypted) {
			CLog::Write(CLog::Info, "File '%s' is encrypted! Decrypting file.\n", filename.data());
			btea((uint *)teaData.data(), -n, (uint *)&keys);
		} else {
			CLog::Write(CLog::Info, "File '%s' is decrypted! Encrypting file.\n", filename.data());
			btea((uint *)teaData.data(), n, (uint *)&keys);
		}

		if (isEncrypted) {
			fileDatastream << "IPRb";
		} else {
			fileDatastream << "IPRe";
		}
		
		// Add the timestamp data back.
		fileDatastream << timestampData;

		// Add in our flags.
		// Until we know the purpose of all the flags. We just use preset stuff.
		fileDatastream << flags;

		// Add back our TEA data size.
		std::stringstream processedDatastream;
		std::string processedData;
		
		// iF we're encrypting. We need to include the size.
		if (!isEncrypted) {
			processedDatastream << teaDataSizeString;
		}
		
		// Add our data which went through TEA.
		processedDatastream << teaData;

		// Finally add back all the data that comes after.
		processedDatastream << memblockHolder;

		processedData = processedDatastream.str();

		// Reverse the endian for writing.
		SwapStringU32(processedData);

		// Add our processed data.
		fileDatastream << processedData;

		// Close our reading file.
		file.close();

		fileData = fileDatastream.str();

		// Re-open the and write to it.
		file.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
		file.write(fileData.data(), fileData.size());
		file.close();
	} else {
		// We failed to open our file.
		CLog::Write(CLog::Warning, "Unable to open file '%s'!\n", filename.data());
	}
}

void process_raw_blob(std::string filename, bool isEncrypted = true) {
	std::fstream file;

	std::string fileData = process_tea_file(filename, isEncrypted);

	// Re-open the and write to it.
	file.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
	file.write(fileData.data(), fileData.size());
	file.close();
}

int main(int argc, char *argv[]) {
	bool isBigFart = false;
    bool isRawBlobEncrypted = false;
	int bigFartLen = 0;

	CLog::Write(CLog::Info, "Little Big TEA Tool created and written by Prince Frizzy also known as theclashingfritz.\n\n");

	std::string sFilename = "littlefartencrypted";
	std::cout << "What filepath do you wish to process? (If you want to process a big fart and there is more then 1, Use the last one) [Include the file's extension]: ";
	getline(std::cin, sFilename);

	std::string sBigFarc = "0";
	std::cout << "Is the file a big fart? [File Type] (0 for No, 1 for Yes): ";
	getline(std::cin, sBigFarc);
	if (std::stoi(sBigFarc) >= 1) {
		isBigFart = 1;

		std::string sBigFarcSpilt = "0";
		std::cout << "Number of Big Fart Files (If only 1 file put 0, Otherwise put the number of files): ";
		getline(std::cin, sBigFarcSpilt);
		bigFartLen = std::stoi(sBigFarcSpilt);
		if (bigFartLen < 0) {
			bigFartLen = 0;
		}
        
        process_big_fart_profile(sFilename, bigFartLen);
		return 0;
	}

	std::string sIPR = "0";
	std::cout << "Is the file a IPR file? (0 for No, 1 for Yes): ";
	getline(std::cin, sIPR);
	if (std::stoi(sIPR) >= 1) {
		process_ipr_file(sFilename);
		return 0;
	}
    
    std::string sRawBlob = "0";
    std::cout << "Is the file a raw blob? (0 for No, 1 for Yes): ";
    getline(std::cin, sRawBlob);
    if (std::stoi(sRawBlob) >= 1) {
        std::string sRawBlobEncrypted = "0";
        std::cout << "Is the file encrypted? (0 for No, 1 for Yes): ";
        getline(std::cin, sRawBlobEncrypted);
        if (std::stoi(sRawBlobEncrypted) >= 1) {
            isRawBlobEncrypted = 1;
        }
        
        process_raw_blob(sFilename, isRawBlobEncrypted);
        return 0;
    }
    
    CLog::Write(CLog::Info, "No other modes are supported. Exiting.\n");
	return 1;
}