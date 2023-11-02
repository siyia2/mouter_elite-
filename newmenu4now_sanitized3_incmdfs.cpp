#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <condition_variable>
#include <readline/readline.h>
#include <algorithm>
#include <filesystem>
#include <future>
#include <sys/types.h>
#include <sys/stat.h>
#include <future>
#include <unistd.h>
#include <dirent.h>
#include <sstream>
#include <thread>
#include <queue>
#include <map>
#include <mutex>
#include <algorithm>
#include <cctype>
#include <regex>

//167 opoydhpote trexw thn entolh system needs escaping with replace " with  ' and ' with ("'", "'\\''", string) 
//santitization example system(("sudo mkdir -p '" + sanitizePath(mountPoint) + "'").c_str()) != 0)
std::string sanitizeForShell(const std::string& input) {
    // If the input contains only spaces, don't escape it
    if (input.find_first_not_of(" ()") == std::string::npos) {
        return input;
    }

    return input;
}

// Function to read a line of input using readline
std::string readInputLine(const std::string& prompt) {
    char* input = readline(prompt.c_str());

    if (input) {
        std::string inputStr(input);
        free(input); // Free the allocated memory for the input
        return inputStr;
    }

    return ""; // Return an empty string if readline fails
}

std::mutex mtx;

int numThreads = 4;
// Define the default cache directory
const std::string cacheDirectory = "/tmp/";

namespace fs = std::filesystem;


// Function prototypes
std::vector<std::string> findMdsMdfFiles(const std::string& directory);
bool isMdf2IsoInstalled();
void convertMDFToISO(const std::string& inputPath);
void convertMDFsToISOs(const std::vector<std::string>& inputPaths, int numThreads);
void processMDFFilesInRange(int start, int end);
void select_and_convert_files_to_iso_mdf();

void processMdfMdsFilesInRange(const std::vector<std::string>& mdfMdsFiles, int start, int end);


bool directoryExists(const std::string& path);
bool hasIsoExtension(const std::string& filePath);
void traverseDirectory(const std::filesystem::path& path, std::vector<std::string>& isoFiles);
void mountISO(const std::vector<std::string>& isoFiles);
void listAndMountISOs();
void unmountISOs();
void cleanAndUnmountAllISOs();
void convertBINsToISOs();
void listMountedISOs();
void listMode();
void manualMode_isos();
void select_and_mount_files_by_number();
void select_and_convert_files_to_iso();
void manualMode_imgs();
void print_ascii();
void screen_clear();

std::vector<std::string> findBinImgFiles(const std::string& directory);
std::mutex mountMutex; // Mutex for thread safety

std::string directoryPath;
std::vector<std::string> binImgFiles;  // Declare binImgFiles here
std::vector<std::string> mdfImgFiles;

// Function to list and prompt the user to choose a file for conversion
std::string chooseFileToConvert(const std::vector<std::string>& files) {
    std::cout << "Found the following .bin and .img files:\n";
    for (size_t i = 0; i < files.size(); ++i) {
        std::cout << i + 1 << ": " << files[i] << "\n";
    }

    int choice;
    std::cout << "Enter the number of the file you want to convert: ";
    std::cin >> choice;

    if (choice >= 1 && choice <= static_cast<int>(files.size())) {
        return files[choice - 1];
    } else {
        std::cout << "Invalid choice. Please choose a valid file.\n";
        return "";
    }
}

int main() {
    std::string choice;
    int numThreads = 4;

    while (true) {
        // Display the menu options
        std::cout << "Menu Options:" << std::endl;
        std::cout << "1. List and Mount ISOs" << std::endl;
        std::cout << "2. Unmount ISOs" << std::endl;
        std::cout << "3. Clean and Unmount All ISOs" << std::endl;
        std::cout << "4. Scan for .bin and .img Files" << std::endl;
        std::cout << "5. List Mounted ISOs" << std::endl;
        std::cout << "6. Exit the Program" << std::endl;

        // Prompt for choice
        std::cout << "Enter your choice: ";
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // Consume the newline character

        if (choice == "1") {
            select_and_mount_files_by_number();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::system("clear");
        } else if (choice == "2") {
            // Call your unmountISOs function
            unmountISOs();
            std::cout << "Press Enter to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::system("clear");
        } else if (choice == "3") {
            // Call your cleanAndUnmountAllISOs function
            cleanAndUnmountAllISOs();
            std::cout << "Press Enter to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::system("clear");
            } else if (choice == "4") {			
            select_and_convert_files_to_iso();
        } else if (choice == "5") {
            // Call your listMountedISOs function
            listMountedISOs();
            std::cout << "Press Enter to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::system("clear");
        } else if (choice == "6") {
            std::cout << "Exiting the program..." << std::endl;
            return 0;
        } else {
            std::cout << "Invalid choice. Please enter 1, 2, 3, 4, 5, or 6." << std::endl;
        }
    }

    return 0;
}

// ... Function definitions ...


void print_ascii() {
    /// Display ASCII art

std::cout << "\033[32m  _____          ___ _____ _____   ___ __   __  ___  _____ _____   __   __  ___ __   __ _   _ _____ _____ ____          _   ___   ___             \033[0m" << std::endl;
std::cout << "\033[32m |  ___)   /\\   (   |_   _)  ___) (   )  \\ /  |/ _ \\|  ___)  ___) |  \\ /  |/ _ (_ \\ / _) \\ | (_   _)  ___)  _ \\        / | /   \\ / _ \\  \033[0m" << std::endl;
std::cout << "\033[32m | |_     /  \\   | |  | | | |_     | ||   v   | |_| | |   | |_    |   v   | | | |\\ v / |  \\| | | | | |_  | |_) )  _  __- | \\ O /| | | |      \033[0m" << std::endl;
std::cout << "\033[32m |  _)   / /\\ \\  | |  | | |  _)    | || |\\_/| |  _  | |   |  _)   | |\\_/| | | | | | |  |     | | | |  _) |  __/  | |/ /| | / _ \\| | | |     \033[0m" << std::endl;
std::cout << "\033[32m | |___ / /  \\ \\ | |  | | | |___   | || |   | | | | | |   | |___  | |   | | |_| | | |  | |\\  | | | | |___| |     | / / | |( (_) ) |_| |       \033[0m" << std::endl;
std::cout << "\033[32m |_____)_/    \\_(___) |_| |_____) (___)_|   |_|_| |_|_|   |_____) |_|   |_|\\___/  |_|  |_| \\_| |_| |_____)_|     |__/  |_(_)___/ \\___/       \033[0m" << std::endl;
		
std::cout << " " << std::endl;
}


bool directoryExists(const std::string& path) {
    return fs::is_directory(path);
}

bool mountIsoFile(const std::string& isoFile, std::map<std::string, std::string>& mountedIsos) {
    // Check if the ISO file is already mounted
    std::lock_guard<std::mutex> lock(mountMutex); // Lock to protect access to mountedIsos
    if (mountedIsos.find(isoFile) != mountedIsos.end()) {
        std::cout << "\e[1;31mALREADY MOUNTED\e[0m: ISO file '" << isoFile << "' is already mounted at '" << mountedIsos[isoFile] << "'." << std::endl;
        return false;
    }

    // Use the filesystem library to extract the ISO file name
    fs::path isoPath(isoFile);
    std::string isoFileName = isoPath.stem().string(); // Remove the .iso extension

    std::string mountPoint = "/mnt/iso_" + isoFileName; // Use the modified ISO file name in the mount point with "iso_" prefix

    // Check if the mount point directory doesn't exist, create it
    if (!directoryExists(mountPoint)) {
        // Sanitize the mountPoint before using it in a shell command
        std::string sanitizedMountPoint = sanitizeForShell(mountPoint);

        // Replace any spaces in the sanitized mount point with underscores
        std::replace(sanitizedMountPoint.begin(), sanitizedMountPoint.end(), ' ', '_');

        // Create the mount point directory
        if (system(("sudo mkdir -p " + sanitizedMountPoint).c_str()) != 0) {
            std::perror("Failed to create mount point directory");
            return false;
        }

        // Use absolute paths for the ISO file and the mount point
        std::string absoluteIsoFile = fs::absolute(isoFile).string();

        // Mount the ISO file to the mount point
        int result = system(("sudo mount -o loop '" + absoluteIsoFile + "' '" + sanitizedMountPoint + "'").c_str());

        if (result != 0) {
            std::perror("Failed to mount ISO file");
            return false;
        } else {
            std::cout << "ISO file '" << isoFile << "' mounted at '" << sanitizedMountPoint << "'." << std::endl;
            // Store the mount point in the map
            mountedIsos[isoFile] = sanitizedMountPoint;
            return true;
        }
    } else {
        // The mount point directory already exists, so the ISO is considered mounted
        std::cout << "ISO file '" << isoFile << "' is already mounted at '" << mountPoint << "'." << std::endl;
        mountedIsos[isoFile] = mountPoint;
        std::cout << "Press Enter to continue...";
        std::cin.get(); // Wait for the user to press Enter
        return true;
    }
}

void mountISO(const std::vector<std::string>& isoFiles) {
    std::map<std::string, std::string> mountedIsos;
    int count = 1;
    std::vector<std::thread> threads;

    for (const std::string& isoFile : isoFiles) {
        // Limit the number of threads to a maximum of 4
        if (threads.size() >= 4) {
            for (auto& thread : threads) {
                thread.join();
            }
            threads.clear();
        }

        threads.emplace_back(mountIsoFile, isoFile, std::ref(mountedIsos));
    }

    // Join any remaining threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Print a message indicating that all ISO files have been mounted
    std::cout << "\e[1;32mPreviously Selected ISO files have been mounted.\e[0m" << std::endl;
}


void select_and_mount_files_by_number() {
    std::string directoryPath = readInputLine("Enter the directory path to search for .iso files: ");
    
    if (directoryPath.empty()) {
        std::cout << "Path input is empty. Exiting." << std::endl;
        return;
    }

    // Sanitize the directory path before using it
    directoryPath = sanitizeForShell(directoryPath);

    std::vector<std::string> isoFiles;
    traverseDirectory(directoryPath, isoFiles);

    std::vector<std::string> mountedISOs;

    while (true) {
        if (isoFiles.empty()) {
            std::cout << "No .iso files found in the specified directory and its subdirectories." << std::endl;
            break;
        }

        // Remove already mounted files from the selection list
        for (const auto& mountedISO : mountedISOs) {
            auto it = std::remove(isoFiles.begin(), isoFiles.end(), mountedISO);
            isoFiles.erase(it, isoFiles.end());
        }

        if (isoFiles.empty()) {
            std::cout << "No more unmounted .iso files in the directory." << std::endl;
            break;
        }

        for (int i = 0; i < isoFiles.size(); i++) {
            std::cout << i + 1 << ". " << isoFiles[i] << std::endl;
        }

        std::string input;
        std::cout << "Choose .iso files to mount (enter numbers separated by spaces or ranges like '1-3', or press Enter to exit): ";
        std::getline(std::cin, input);

        if (input.empty()) {
            std::cout << "Press Enter to Return" << std::endl;
            break;
        }

        std::istringstream iss(input);
        std::string token;
        
        while (std::getline(iss, token, ' ')) {
            if (token.find('-') != std::string::npos) {
                // Handle a range
                size_t dashPos = token.find('-');
                int startRange = std::stoi(token.substr(0, dashPos));
                int endRange = std::stoi(token.substr(dashPos + 1));

                if (startRange >= 1 && startRange <= isoFiles.size() && endRange >= startRange && endRange <= isoFiles.size()) {
                    for (int i = startRange; i <= endRange; i++) {
                        // Sanitize the selected ISO file path
                        std::string selectedISO = isoFiles[i - 1];
                        selectedISO = sanitizeForShell(selectedISO);

                        // Check if the selected ISO is not already mounted
                        if (std::find(mountedISOs.begin(), mountedISOs.end(), selectedISO) == mountedISOs.end()) {
                            // Mount the selected ISO file
                            mountISO({selectedISO});
                            // Add the sanitized mounted ISO to the list
                            mountedISOs.push_back(selectedISO);
                        } else {
                            std::cout << "ISO file '" << selectedISO << "' is already mounted." << std::endl;
                        }
                    }
                } else {
                    std::cout << "Invalid range: " << token << ". Please try again." << std::endl;
                }
            } else {
                int selectedNumber = std::stoi(token);
                if (selectedNumber >= 1 && selectedNumber <= isoFiles.size()) {
                    // Sanitize the selected ISO file path
                    std::string selectedISO = isoFiles[selectedNumber - 1];
                    selectedISO = sanitizeForShell(selectedISO);

                    // Check if the selected ISO is not already mounted
                    if (std::find(mountedISOs.begin(), mountedISOs.end(), selectedISO) == mountedISOs.end()) {
                        // Mount the selected ISO file
                        mountISO({selectedISO});
                        // Add the sanitized mounted ISO to the list
                        mountedISOs.push_back(selectedISO);
                    } else {
                        std::cout << "ISO file '" << selectedISO << "' is already mounted." << std::endl;
                    }
                } else {
                    std::cout << "Invalid number: " << selectedNumber << ". Please try again." << std::endl;
                }
            }
        }
    }
}

bool iequals(const std::string& a, const std::string& b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char lhs, char rhs) {
        return std::tolower(lhs) == std::tolower(rhs);
    });
}

void traverseDirectory(const std::filesystem::path& path, std::vector<std::string>& isoFiles) {
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::string filePath = entry.path().string();

                // Check if the file has an ".iso" extension (case-insensitive)
                if (iequals(std::filesystem::path(filePath).extension(), ".iso")) {
                    isoFiles.push_back(filePath);
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}


// Function to check if a file has the .iso extension
bool hasIsoExtension(const std::string& filePath) {
    // Extract the file extension and check if it's ".iso"
    size_t pos = filePath.find_last_of('.');
    if (pos != std::string::npos) {
        std::string extension = filePath.substr(pos);
        return extension == ".iso";
    }
    return false;
}

bool hasIsoExtensionInParallel(const std::vector<std::string>& filePaths) {
    std::vector<bool> results(filePaths.size(), false);

    // Define a function to be executed by each thread
    auto checkIsoExtension = [&](int start, int end) {
        for (int i = start; i < end; i++) {
            results[i] = hasIsoExtension(filePaths[i]);
        }
    };

    // Create four threads to perform the checks in parallel (for a maximum of 4 cores)
    int numThreads = std::min(4, static_cast<int>(filePaths.size()));
    std::vector<std::thread> threads;

    int batchSize = filePaths.size() / numThreads;
    for (int i = 0; i < numThreads; i++) {
        int start = i * batchSize;
        int end = (i == numThreads - 1) ? filePaths.size() : (i + 1) * batchSize;
        threads.emplace_back(checkIsoExtension, start, end);
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    // Check if any of the threads found an ".iso" file
    for (bool result : results) {
        if (result) {
            return true;
        }
    }

    return false;
}


void unmountISOs() {
    const std::string isoPath = "/mnt";

    while (true) {
        std::vector<std::string> isoDirs;

        // Find and store directories with the name "iso_*" in /mnt
        DIR* dir;
        struct dirent* entry;

        if ((dir = opendir(isoPath.c_str())) != NULL) {
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_DIR && std::string(entry->d_name).find("iso_") == 0) {
                    isoDirs.push_back(isoPath + "/" + entry->d_name);
                }
            }
            closedir(dir);
        }

        if (isoDirs.empty()) {
            std::cout << "\033[31mNO ISOS MOUNTED, NOTHING TO DO.\n\033[0m";
            return;
        }

        // Display a list of mounted ISOs with indices
        std::cout << "List of mounted ISOs:" << std::endl;
        for (size_t i = 0; i < isoDirs.size(); ++i) {
            std::cout << i + 1 << ". " << isoDirs[i] << std::endl;
        }

        // Prompt for unmounting input
        std::cout << "\033[33mEnter the range of ISOs to unmount (e.g., 1, 1-3, 1 to 3) or type 'exit' to cancel:\033[0m ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "exit") {
            std::cout << "Exiting the unmounting tool." << std::endl;
            break;  // Exit the loop
        }

        // Continue with the rest of the logic to process user input
        std::istringstream iss(input);
        int startRange, endRange;
        char hyphen;

        if ((iss >> startRange) && (iss >> hyphen) && (iss >> endRange)) {
            if (hyphen == '-' && startRange >= 1 && endRange >= startRange && static_cast<size_t>(endRange) <= isoDirs.size()) {
                // Valid range input
            } else {
                std::cerr << "\033[31mInvalid range. Please try again.\n\033[0m" << std::endl;
                continue;  // Restart the loop
            }
        } else {
            // If no hyphen is present or parsing fails, treat it as a single choice
            endRange = startRange;
        }
        if (startRange < 1 || endRange > static_cast<int>(isoDirs.size()) || startRange > endRange) {
            std::cerr << "\033[31mInvalid range or choice. Please try again.\n\033[0m" << std::endl;
            continue;  // Restart the loop
        }

        // Unmount and attempt to remove the selected range of ISOs, suppressing output
        for (int i = startRange - 1; i < endRange; ++i) {
            const std::string& isoDir = isoDirs[i];

            // Sanitize the isoDir path before using it in shell commands
            std::string sanitizedIsoDir = sanitizeForShell(isoDir);

            // Unmount the ISO and suppress logs
            std::string unmountCommand = "sudo umount -l " + sanitizedIsoDir + " > /dev/null 2>&1";
            int result = system(unmountCommand.c_str());

            if (result == 0) {
                // Omitted log for unmounting
            } else {
                // Omitted log for unmounting
            }

            // Remove the directory, regardless of unmount success, and suppress error message
            std::string removeDirCommand = "sudo rmdir -p " + sanitizedIsoDir + " 2>/dev/null";
            int removeDirResult = system(removeDirCommand.c_str());

            if (removeDirResult == 0) {
                // Omitted log for directory removal
            }
        }
    }
}



void unmountAndCleanISO(const std::string& isoDir) {
    // Sanitize the isoDir path before using it in shell commands
    std::string sanitizedIsoDir = sanitizeForShell(isoDir);

    std::string unmountCommand = "sudo umount -l " + sanitizedIsoDir + " 2>/dev/null";
    int result = std::system(unmountCommand.c_str());

    // if (result != 0) {
       // std::cerr << "Failed to unmount " << isoDir << " with sudo." << std::endl;
    // }

    // Remove the directory after unmounting
    std::string removeDirCommand = "sudo rmdir " + sanitizedIsoDir;
    int removeDirResult = std::system(removeDirCommand.c_str());

    if (removeDirResult != 0) {
        std::cerr << "Failed to remove directory " << sanitizedIsoDir << std::endl;
    }
}

void cleanAndUnmountISO(const std::string& isoDir) {
    std::lock_guard<std::mutex> lock(mtx);
    unmountAndCleanISO(isoDir);
}

void cleanAndUnmountAllISOs() {
	std::cout << "\n";
    std::cout << "Clean and Unmount All ISOs function." << std::endl;
    const std::string isoPath = "/mnt";
    std::vector<std::string> isoDirs;

    DIR* dir;
    struct dirent* entry;

    if ((dir = opendir(isoPath.c_str())) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR && std::string(entry->d_name).find("iso_") == 0) {
                isoDirs.push_back(isoPath + "/" + entry->d_name);
            }
        }
        closedir(dir);
    }

    if (isoDirs.empty()) {
        std::cout << "\033[31mNO ISOS TO BE CLEANED\n\033[0m" << std::endl;
        return;
    }

    std::vector<std::thread> threads;

    for (const std::string& isoDir : isoDirs) {
        threads.emplace_back(cleanAndUnmountISO, isoDir);
        if (threads.size() >= 4) {
            for (std::thread& thread : threads) {
                thread.join();
            }
            threads.clear();
        }
    }

    for (std::thread& thread : threads) {
        thread.join();
    }

    std::cout << "\033[32mALL ISOS CLEANED\n\033[0m" << std::endl;
}


void listMountedISOs() {
    std::string path = "/mnt";
    int isoCount = 0;

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_directory() && entry.path().filename().string().find("iso") == 0) {
            // Print the directory number, name, and color.
            std::cout << "\033[1;35m" << ++isoCount << ". " << entry.path().filename().string() << "\033[0m" << std::endl;
        }
    }

    if (isoCount == 0) {
        std::cout << "\033[31mNo ISO(s) mounted.\n\033[0m" << std::endl;
    }
}


std::vector<std::string> findBinImgFiles(const std::string& directory) {
    std::vector<std::string> fileNames;

    try {
        std::vector<std::future<void>> futures;
        std::mutex mutex; // Mutex for protecting the shared data
        const int maxThreads = 4; // Maximum number of worker threads

        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) { return std::tolower(c); }); // Convert extension to lowercase

		if ((ext == ".bin" || ext == ".img") && (entry.path().filename().string().find("data") == std::string::npos) && (entry.path().filename().string() != "terrain.bin")&& (entry.path().filename().string() != "blocklist.bin")) {
                    if (fs::file_size(entry) >= 10'000'000) {
                        // Ensure the number of active threads doesn't exceed maxThreads
                        while (futures.size() >= maxThreads) {
                            // Wait for at least one thread to complete
                            auto it = std::find_if(futures.begin(), futures.end(),
                                [](const std::future<void>& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; });
                            if (it != futures.end()) {
                                it->get();
                                futures.erase(it);
                            }
                        }

                        // Create a task to process the file
                        futures.push_back(std::async(std::launch::async, [entry, &fileNames, &mutex] {
                            std::string fileName = entry.path().string();

                            // Lock the mutex before modifying the shared data
                            std::lock_guard<std::mutex> lock(mutex);

                            // Add the file name to the shared vector
                            fileNames.push_back(fileName);
                        }));
                    }
                }
            }
        }

        // Wait for the remaining tasks to complete
        for (auto& future : futures) {
            future.get();
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }

    return fileNames;
}




bool isCcd2IsoInstalled() {
    if (std::system("which ccd2iso > /dev/null 2>&1") == 0) {
        return true;
    } else {
        return false;
    }
}


void convertBINToISO(const std::string& inputPath) {
    // Check if the input file exists
    if (!std::ifstream(inputPath)) {
        std::cout << "The specified input file '" << inputPath << "' does not exist." << std::endl;
        return;
    }

    // Define the output path for the ISO file with only the .iso extension
    std::string outputPath = inputPath.substr(0, inputPath.find_last_of(".")) + ".iso";

    // Check if the output ISO file already exists
    if (std::ifstream(outputPath)) {
        std::cout << "The output ISO file '" << outputPath << "' already exists. Skipping conversion." << std::endl;
    } else {
        // Execute the conversion using ccd2iso
        std::string conversionCommand = "ccd2iso \"" + inputPath + "\" \"" + outputPath + "\"";
        int conversionStatus = std::system(conversionCommand.c_str());
        if (conversionStatus == 0) {
            std::cout << "Image file converted to ISO: " << outputPath << std::endl;
        } else {
            std::cout << "Conversion of " << inputPath << " failed." << std::endl;
        }
    }
}

void convertBINsToISOs(const std::vector<std::string>& inputPaths, int numThreads) {
    if (!isCcd2IsoInstalled()) {
        std::cout << "ccd2iso is not installed. Please install it before using this option." << std::endl;
        return;
    }

    // Create a thread pool with a limited number of threads
    std::vector<std::thread> threads;
    int numCores = std::min(numThreads, static_cast<int>(std::thread::hardware_concurrency()));

    for (const std::string& inputPath : inputPaths) {
        if (inputPath == "}") {
            break; // Exit the loop if a closing brace is encountered
        } else {
            // Create a new thread for each conversion
            threads.emplace_back(convertBINToISO, inputPath);
            if (threads.size() >= numCores) {
                // Limit the number of concurrent threads to the number of available cores
                for (auto& thread : threads) {
                    thread.join();
                }
                threads.clear();
            }
        }
    }

    // Join any remaining threads
    for (auto& thread : threads) {
        thread.join();
    }
}



void processFilesInRange(int start, int end) {
    std::vector<std::string> selectedFiles;
    for (int i = start; i <= end; i++) {
        selectedFiles.push_back(binImgFiles[i - 1]);
    }
    convertBINsToISOs(selectedFiles, numThreads);
}

void select_and_convert_files_to_iso() {
    std::string directoryPath = readInputLine("Enter the directory path to search for .bin .img files: ");
    
    if (directoryPath.empty()) {
        std::cout << "Path input is empty. Exiting." << std::endl;
        return;
    }

    // Sanitize the directory path before using it
    directoryPath = sanitizeForShell(directoryPath);

    binImgFiles = findBinImgFiles(directoryPath); // You need to define findBinImgFiles function.
    if (binImgFiles.empty()) {
        std::cout << "No .bin or .img files found in the specified directory and its subdirectories or all files are under 10MB." << std::endl;
    } else {
        for (int i = 0; i < binImgFiles.size(); i++) {
            std::cout << i + 1 << ". " << binImgFiles[i] << std::endl;
        }

        std::string input;

        while (true) {
            std::cout << "Choose a file to process (enter the number or range e.g., 1-5 or press Enter to exit): ";
            std::getline(std::cin, input);

            if (input.empty()) {
                std::cout << "Exiting..." << std::endl;
                break;
            }

            std::istringstream iss(input);
            int start, end;
            char dash;

            if (iss >> start) {
                if (iss >> dash && dash == '-' && iss >> end) {
                    // Range input (e.g., 1-5)
                    if (start >= 1 && start <= binImgFiles.size() && end >= start && end <= binImgFiles.size()) {
                        // Divide the work among threads (up to 4 cores)
                        std::thread threads[4];
                        int range = (end - start + 1) / 4;

                        for (int i = 0; i < 4; i++) {
                            int threadStart = start + i * range;
                            int threadEnd = i == 3 ? end : threadStart + range - 1;
                            threads[i] = std::thread(processFilesInRange, threadStart, threadEnd);
                        }

                        for (int i = 0; i < 4; i++) {
                            threads[i].join();
                        }
                    } else {
                        std::cout << "Invalid range. Please try again." << std::endl;
                    }
                } else if (start >= 1 && start <= binImgFiles.size()) {
                    // Single number input
                    std::vector<std::string> selectedFiles;
                    selectedFiles.push_back(binImgFiles[start - 1]);
                    convertBINsToISOs(selectedFiles, numThreads); // Convert the selected file immediately
                } else {
                    std::cout << "Invalid number. Please try again." << std::endl;
                }
            } else {
                std::cout << "Invalid input format. Please try again." << std::endl;
            }
        }
    }
}


// MDF/MDS CONVERSION STUFF

std::vector<std::string> findMdsMdfFiles(const std::string& directory) {
    std::vector<std::string> fileNames;

    try {
        std::vector<std::future<void>> futures;
        std::mutex mutex; // Mutex for protecting the shared data
        const int maxThreads = 4; // Maximum number of worker threads

        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) { return std::tolower(c); }); // Convert extension to lowercase

                if ((ext == ".mds" || ext == ".mdf") && (entry.path().filename().string().find("data") == std::string::npos) && (entry.path().filename().string() != "terrain.mdf") && (entry.path().filename().string() != "blocklist.mdf")) {
                    if (fs::file_size(entry) >= 10'000'000) {
                        // Ensure the number of active threads doesn't exceed maxThreads
                        while (futures.size() >= maxThreads) {
                            // Wait for at least one thread to complete
                            auto it = std::find_if(futures.begin(), futures.end(),
                                [](const std::future<void>& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; });
                            if (it != futures.end()) {
                                it->get();
                                futures.erase(it);
                            }
                        }

                        // Create a task to process the file
                        futures.push_back(std::async(std::launch::async, [entry, &fileNames, &mutex] {
                            std::string fileName = entry.path().string();

                            // Lock the mutex before modifying the shared data
                            std::lock_guard<std::mutex> lock(mutex);

                            // Add the file name to the shared vector
                            fileNames.push_back(fileName);
                        }));
                    }
                }
            }
        }

        // Wait for the remaining tasks to complete
        for (auto& future : futures) {
            future.get();
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }

    return fileNames;
}

bool isMdf2IsoInstalled() {
    if (std::system("which mdf2iso > /dev/null 2>&1") == 0) {
        return true;
    } else {
        return false;
    }
}

void convertMDFToISO(const std::string& inputPath) {
    // Check if the input file exists
    if (!std::ifstream(inputPath)) {
        std::cout << "The specified input file '" << inputPath << "' does not exist." << std::endl;
        return;
    }

    // Define the output path for the ISO file with only the .iso extension
    std::string outputPath = inputPath.substr(0, inputPath.find_last_of(".")) + ".iso";

    // Check if the output ISO file already exists
    if (std::ifstream(outputPath)) {
        std::cout << "The output ISO file '" << outputPath << "' already exists. Skipping conversion." << std::endl;
    } else {
        // Execute the conversion using mdf2iso
        std::string conversionCommand = "mdf2iso \"" + inputPath + "\" \"" + outputPath + "\"";
        int conversionStatus = std::system(conversionCommand.c_str());
        if (conversionStatus == 0) {
            std::cout << "Image file converted to ISO: " << outputPath << std::endl;
        } else {
            std::cout << "Conversion of " << inputPath << " failed." << std::endl;
        }
    }
}

void convertMDFsToISOs(const std::vector<std::string>& inputPaths, int numThreads) {
    if (!isMdf2IsoInstalled()) {
        std::cout << "mdf2iso is not installed. Please install it before using this option." << std::endl;
        return;
    }

    // Create a thread pool with a limited number of threads
    std::vector<std::thread> threads;
    int numCores = std::min(numThreads, static_cast<int>(std::thread::hardware_concurrency()));

    for (const std::string& inputPath : inputPaths) {
        if (inputPath == "}") {
            break; // Exit the loop if a closing brace is encountered
        } else {
            // Create a new thread for each conversion
            threads.emplace_back(convertMDFToISO, inputPath);
            if (threads.size() >= numCores) {
                // Limit the number of concurrent threads to the number of available cores
                for (auto& thread : threads) {
                    thread.join();
                }
                threads.clear();
            }
        }
    }

    // Join any remaining threads
    for (auto& thread : threads) {
        thread.join();
    }
}

void processMDFFilesInRange(int start, int end) {
    std::vector<std::string> selectedFiles;
    for (int i = start; i <= end; i++) {
        selectedFiles.push_back(mdfImgFiles[i - 1]);
    }
    convertMDFsToISOs(selectedFiles, numThreads);
}


void select_and_convert_files_to_iso_mdf() {
    std::string directoryPath = readInputLine("Enter the directory path to search for .mdf .mds files: ");
    
    if (directoryPath.empty()) {
        std::cout << "Path input is empty. Exiting." << std::endl;
        return;
    }

    // Sanitize the directory path before using it
    directoryPath = sanitizeForShell(directoryPath);

    std::vector<std::string> mdfMdsFiles = findMdsMdfFiles(directoryPath);  // You need to define findMdfMdsFiles function.
    if (mdfMdsFiles.empty()) {
        std::cout << "No .mdf or .mds files found in the specified directory and its subdirectories or all files are under 10MB." << std::endl;
    } else {
        for (int i = 0; i < mdfMdsFiles.size(); i++) {
            std::cout << i + 1 << ". " << mdfMdsFiles[i] << std::endl;
        }

        std::string input;

        while (true) {
            std::cout << "Choose a file to process (enter the number or range e.g., 1-5 or press Enter to exit): ";
            std::getline(std::cin, input);

            if (input.empty()) {
                std::cout << "Exiting..." << std::endl;
                break;
            }

            std::istringstream iss(input);
            int start, end;
            char dash;

            if (iss >> start) {
                if (iss >> dash && dash == '-' && iss >> end) {
                    // Range input (e.g., 1-5)
                    if (start >= 1 && start <= mdfMdsFiles.size() && end >= start && end <= mdfMdsFiles.size()) {
                        // Divide the work among threads (up to 4 cores)
                        std::thread threads[4];
                        int range = (end - start + 1) / 4;

                        for (int i = 0; i < 4; i++) {
                            int threadStart = start + i * range;
                            int threadEnd = i == 3 ? end : threadStart + range - 1;
                            threads[i] = std::thread(processMdfMdsFilesInRange, mdfMdsFiles, threadStart, threadEnd);
                        }

                        for (int i = 0; i < 4; i++) {
                            threads[i].join();
                        }
                    } else {
                        std::cout << "Invalid range. Please try again." << std::endl;
                    }
                } else if (start >= 1 && start <= mdfMdsFiles.size()) {
                    // Single number input
                    std::vector<std::string> selectedFiles;
                    selectedFiles.push_back(mdfMdsFiles[start - 1]);
                    convertMDFsToISOs(selectedFiles, numThreads); // Convert the selected file immediately
                } else {
                    std::cout << "Invalid number. Please try again." << std::endl;
                }
            } else {
                std::cout << "Invalid input format. Please try again." << std::endl;
            }
        }
    }
}

void processMdfMdsFilesInRange(const std::vector<std::string>& mdfMdsFiles, int start, int end) {
    for (int i = start - 1; i < end; i++) {
        std::vector<std::string> selectedFiles;
        selectedFiles.push_back(mdfMdsFiles[i]);
        convertMDFsToISOs(selectedFiles, numThreads);
    }
}


