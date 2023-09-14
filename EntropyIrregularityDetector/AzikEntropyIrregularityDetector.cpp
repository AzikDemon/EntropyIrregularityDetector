#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <iomanip>
#include <windows.h>
#include <map>
#include <string>

// Define custom names and associated entropy values for client-specific detections
std::vector<std::string> customNames = {"VapeV4.11", "SkilledV3"};
std::vector<double> clientEntropy = {12.1883153487, 1.3415799831};

// Function to extract the file name from a given file path
std::string extractFileName(const std::string &filePath)
{
    size_t lastSlashIndex = filePath.find_last_of("/\\");
    if (lastSlashIndex != std::string::npos)
    {
        return filePath.substr(lastSlashIndex + 1);
    }
    return filePath;
}

// Structure to store information about a file
struct FileInfo
{
    std::string fileName;
    std::string customName; // For client-specific detections
    unsigned char maxMultiplicationIncreaseByte;
    double maxMultiplicationIncrease;
};

// Function to calculate entropy and identify file types
void calculateEntropy(const std::vector<unsigned char> &bytes, const std::string &fileName,
                      std::vector<FileInfo> &regularFiles, std::vector<FileInfo> &genericDetectionFiles,
                      std::vector<FileInfo> &clientDetectionFiles)
{
    std::vector<int> frequency(256, 0);
    int totalBytes = bytes.size();
    double previousFrequency = 0.0;
    double maxMultiplicationIncrease = 0.0;
    unsigned char maxMultiplicationIncreaseByte = 0;

    // Calculate byte frequency
    for (size_t i = 1; i < bytes.size() - 2; i++)
    {
        frequency[bytes[i]]++;
    }

    // Calculate entropy and identify max multiplication increase
    for (int i = 0; i < 255; i++)
    {
        if (frequency[i] > 0)
        {
            double probability = static_cast<double>(frequency[i]) / totalBytes;
            double entropy = -probability * log2(probability);

            double multiplicationIncrease = 1.0;
            if (previousFrequency > 0.0)
            {
                multiplicationIncrease = probability / previousFrequency;
            }

            // Update max multiplication increase and byte
            if (multiplicationIncrease > maxMultiplicationIncrease)
            {
                maxMultiplicationIncrease = multiplicationIncrease;
                maxMultiplicationIncreaseByte = i;
            }

            previousFrequency = probability;
        }
        // Debugging for Each Byte
        /*
        std::cout << "" << std::endl;
        std::cout << "File Name: " << fileName << std::endl;
        std::cout << "Byte: " << i << ", Size: " << frequency[i] << std::endl;
        */
    }

    // Create FileInfo structure to store information about the file
    FileInfo fileInfo;
    fileInfo.fileName = fileName;
    fileInfo.maxMultiplicationIncreaseByte = maxMultiplicationIncreaseByte;
    fileInfo.maxMultiplicationIncrease = maxMultiplicationIncrease;

    const double threshold = 10.0;

    // Identify generic detections based on the threshold
    if (maxMultiplicationIncrease > threshold)
    {
        genericDetectionFiles.push_back(fileInfo);
    }

    double epsilon = 1e-6;

    // Identify client-specific detections based on predefined entropy values
    for (size_t i = 0; i < clientEntropy.size(); i++)
    {
        if (fabs(maxMultiplicationIncrease - clientEntropy[i]) < epsilon)
        {
            fileInfo.customName = customNames[i];
            clientDetectionFiles.push_back(fileInfo);
        }
    }

    // Store the file in the regular files list
    regularFiles.push_back(fileInfo);
}

// Function to scan a directory for files
void scanDirectory(const std::string &directoryPath, std::vector<FileInfo> &regularFiles,
                   std::vector<FileInfo> &genericDetectionFiles, std::vector<FileInfo> &clientDetectionFiles)
{
    WIN32_FIND_DATAW findFileData;
    std::wstring directoryPathW = std::wstring(directoryPath.begin(), directoryPath.end());

    HANDLE hFind = FindFirstFileW((directoryPathW + L"\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        std::wcout << L"Error while scanning directory: " << GetLastError() << std::endl;
        return;
    }

    // Loop through files in the directory
    do
    {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            std::wstring fileNameW = findFileData.cFileName;
            std::string fileName(fileNameW.begin(), fileNameW.end());
            std::string filePath = directoryPath + "\\" + fileName;

            std::ifstream file(filePath, std::ios::binary);

            if (file)
            {
                // Read file contents into a vector of bytes and analyze it
                std::vector<unsigned char> bytes(std::istreambuf_iterator<char>(file), {});
                calculateEntropy(bytes, fileName, regularFiles, genericDetectionFiles, clientDetectionFiles);
            }
            else
            {
                std::cout << "Failed to open the file: " << filePath << std::endl;
            }
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);

    FindClose(hFind);
}

int main(int argc, char *argv[])
{
    // Check for correct command line arguments
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <directory_path>" << std::endl;
        return 1;
    }

    std::string directoryPath = argv[1];
    std::vector<FileInfo> regularFiles;
    std::vector<FileInfo> genericDetectionFiles;
    std::vector<FileInfo> clientDetectionFiles;

    // Scan the specified directory for files and analyze them
    scanDirectory(directoryPath, regularFiles, genericDetectionFiles, clientDetectionFiles);

    // Display results
    std::cout << "" << std::endl;
    std::cout << "------ All Files ------" << std::endl;
    std::cout << "" << std::endl;
    for (const auto &fileInfo : regularFiles)
    {
        std::cout << "File Name: " << fileInfo.fileName
                  << ", Multiplication Increase: " << std::fixed << std::setprecision(10) << fileInfo.maxMultiplicationIncrease << "]" << std::endl;
    }

    std::cout << "" << std::endl;
    std::cout << "------ Generic Detections ------" << std::endl;
    std::cout << "" << std::endl;

    for (const auto &fileInfo : genericDetectionFiles)
    {
        std::cout << "Generic Detection: " << fileInfo.fileName
                  << ", Multiplication Increase: " << std::fixed << std::setprecision(10) << fileInfo.maxMultiplicationIncrease << "]" << std::endl;
    }

    std::cout << "" << std::endl;
    std::cout << "------ Client Specific Detections ------" << std::endl;
    std::cout << "" << std::endl;
    for (const auto &fileInfo : clientDetectionFiles)
    {
        std::cout << fileInfo.customName << " Found: "
                  << ", File Name: " << fileInfo.fileName << "]" << std::endl;
    }
    return 0;
}
