#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <iomanip>
#include <windows.h>
#include <map>
#include <string>

bool endsWithIgnoreCase(const std::string &str, const std::string &suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin(), [](char a, char b) {
        return tolower(a) == tolower(b);
    });
}

std::vector<std::string> customNames = {"Itami", "Drip Lite V2.91", "VapeV4.11", "SkilledV3", "VapeV4.12", "crimDLL", "crimEXE", "decaDLL", "EntropyV3.12", "EntropyV4.35", "Isolation", "KarmaV0.15", "nullDLL", "Performs Cane Mod", "RavenB2", "Sapphire.LITE", "Tense Clicker", "Wisp", "Akira"};
std::vector<double> clientEntropy = {28.1000000000, 3.4246777562, 12.1883153487, 1.3415799831, 12.9593982392, 5.6545028457, 54.0000000000, 1.8610687023, 2.1479197235, 1.7928388747, 24.8558139535, 2.5128729456, 1.4154397689, 1.2592190889, 1.4904903876, 1.4813348416, 17.3666666667, 1.7757780650, 1.5264465744};

std::string extractFileName(const std::string &filePath) {
    size_t lastSlashIndex = filePath.find_last_of("/\\");
    if (lastSlashIndex != std::string::npos) {
        return filePath.substr(lastSlashIndex + 1);
    }
    return filePath;
}

struct FileInfo {
    std::string fileName;
    std::string customName;
    unsigned char maxMultiplicationIncreaseByte;
    double maxMultiplicationIncrease;
    double entropy;
};

void calculateEntropy(const std::vector<unsigned char> &bytes, const std::string &fileName,
                      std::vector<FileInfo> &regularFiles, std::vector<FileInfo> &genericDetectionFiles,
                      std::vector<FileInfo> &clientDetectionFiles) {
    std::vector<int> frequency(256, 0);
    int totalBytes = bytes.size();
    double previousFrequency = 0.0;
    double maxMultiplicationIncrease = 0.0;
    unsigned char maxMultiplicationIncreaseByte = 0;
    double entropy = 0.0;

    for (size_t i = 1; i < bytes.size() - 2; i++) {
        frequency[bytes[i]]++;
    }

    for (int i = 0; i < 256; i++) {
        if (frequency[i] > 0) {
            double probability = static_cast<double>(frequency[i]) / totalBytes;
            entropy += -probability * log2(probability);

            double multiplicationIncrease = 1.0;
            if (previousFrequency > 0.0) {
                multiplicationIncrease = probability / previousFrequency;
            }

            if (multiplicationIncrease > maxMultiplicationIncrease) {
                maxMultiplicationIncrease = multiplicationIncrease;
                maxMultiplicationIncreaseByte = i;
            }

            previousFrequency = probability;
        }
    }

    FileInfo fileInfo;
    fileInfo.fileName = fileName;
    fileInfo.maxMultiplicationIncreaseByte = maxMultiplicationIncreaseByte;
    fileInfo.maxMultiplicationIncrease = maxMultiplicationIncrease;
    fileInfo.entropy = entropy;

    const double threshold = 10.0;
    const double entropyAmount = 7.0;
    if (!endsWithIgnoreCase(fileName, ".jar")) {
        if (maxMultiplicationIncrease > threshold && fileInfo.entropy > entropyAmount) {
            genericDetectionFiles.push_back(fileInfo);
        }
        if (fileInfo.entropy > 7.8) {
            genericDetectionFiles.push_back(fileInfo);
        }
        if (maxMultiplicationIncrease > 20.0) {
            genericDetectionFiles.push_back(fileInfo);
        }
    }
    double epsilon = 1e-6;

    for (size_t i = 0; i < clientEntropy.size(); i++) {
        if (fabs(maxMultiplicationIncrease - clientEntropy[i]) < epsilon) {
            fileInfo.customName = customNames[i];
            clientDetectionFiles.push_back(fileInfo);
        }
    }

    regularFiles.push_back(fileInfo);
}

void scanDirectory(const std::string &directoryPath, std::vector<FileInfo> &regularFiles,
                   std::vector<FileInfo> &genericDetectionFiles, std::vector<FileInfo> &clientDetectionFiles) {
    WIN32_FIND_DATAW findFileData;
    std::wstring directoryPathW = std::wstring(directoryPath.begin(), directoryPath.end());
    HANDLE hFind = FindFirstFileW((directoryPathW + L"\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::wcout << L"Error while scanning directory: " << GetLastError() << std::endl;
        return;
    }

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring fileNameW = findFileData.cFileName;
            std::string fileName(fileNameW.begin(), fileNameW.end());
            std::string filePath = directoryPath + "\\" + fileName;
            std::ifstream file(filePath, std::ios::binary);

            if (file) {
                std::vector<unsigned char> bytes(std::istreambuf_iterator<char>(file), {});
                calculateEntropy(bytes, fileName, regularFiles, genericDetectionFiles, clientDetectionFiles);
            } else {
                std::cout << "Failed to open the file: " << filePath << std::endl;
            }
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);

    FindClose(hFind);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <directory_path>" << std::endl;
        return 1;
    }

    std::string directoryPath = argv[1];
    std::vector<FileInfo> regularFiles;
    std::vector<FileInfo> genericDetectionFiles;
    std::vector<FileInfo> clientDetectionFiles;

    scanDirectory(directoryPath, regularFiles, genericDetectionFiles, clientDetectionFiles);

    std::cout << "" << std::endl;
    std::cout << "------ All Files ------" << std::endl;
    std::cout << "" << std::endl;
    for (const auto &fileInfo : regularFiles) {
        std::cout << std::left << "File Name: " << std::setw(33) << fileInfo.fileName
                  << " | Multiplication Increase: " << std::setw(15) << std::fixed << std::setprecision(10) << fileInfo.maxMultiplicationIncrease
                  << " | Entropy: " << fileInfo.entropy << std::endl;
    }

    std::cout << "" << std::endl;
    std::cout << "------ Generic Detections ------" << std::endl;
    std::cout << "" << std::endl;

    for (const auto &fileInfo : genericDetectionFiles) {
        std::cout << std::left << "Generic Detection: " << std::setw(25) << fileInfo.fileName
                  << " | Multiplication Increase: " << std::setw(15) << std::fixed << std::setprecision(10) << fileInfo.maxMultiplicationIncrease
                  << " | Entropy: " << fileInfo.entropy << std::endl;
    }

    std::cout << "" << std::endl;
    std::cout << "------ Client Specific Detections ------" << std::endl;
    std::cout << "" << std::endl;
    for (const auto &fileInfo : clientDetectionFiles) {
        std::cout << std::left << "Found: " << std::setw(37) << fileInfo.customName
                  << " | File Name: " << std::setw(29) << fileInfo.fileName << " | Entropy: " << fileInfo.entropy << std::endl;
    }
    return 0;
}
