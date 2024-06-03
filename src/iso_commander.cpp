#include "headers.h"

 
// Get max available CPU cores for global use, fallback is 2 cores
unsigned int maxThreads = std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 2;

// Cache Variables

const std::string cacheDirectory = std::string(std::getenv("HOME")) + "/.cache"; // Construct the full path to the cache directory
const std::string cacheFileName = "iso_commander_cache.txt";;
const uintmax_t maxCacheSize = 10 * 1024 * 1024; // 10MB

std::mutex Mutex4High; // Mutex for high level functions
std::mutex Mutex4Med; // Mutex for mid level functions
std::mutex Mutex4Low; // Mutex for low level functions

// For cache directory creation
bool gapPrinted = false; // for cache refresh for directory function
bool promptFlag = true; // for cache refresh for directory function
bool gapPrintedtraverse = false; // for traverse function

// For saving history to a differrent cache for FilterPatterns
bool historyPattern = false;

// Global variables for cleanup
int lockFileDescriptor = -1;


// Vector to store ISO mounts
std::vector<std::string> mountedFiles;
// Vector to store skipped ISO mounts
std::vector<std::string> skippedMessages;

// Vector to store ISO unique input errors
std::unordered_set<std::string> uniqueErrorMessages;

// Vector to store ISO unmounts
std::vector<std::string> unmountedFiles;
// Vector to store ISO unmount errors
std::vector<std::string> unmountedErrors;


// Main function
int main(int argc, char *argv[]) {
	
	if (argc == 2 && (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v")) {
        printVersionNumber("3.2.8");
        return 0;
    }
	
    const char* lockFile = "/tmp/isocmd.lock";
    
    lockFileDescriptor = open(lockFile, O_CREAT | O_RDWR, 0666);
    
    if (lockFileDescriptor == -1) {
        std::cerr << "\033[93mAnother instance of isocmd is already running. If not run \"rm /tmp/isocmd.lock\".\n\033[0m" << std::endl;
        return 1;
    }

    struct flock fl;
    fl.l_type = F_WRLCK;  // Write lock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // Lock the whole file

    if (fcntl(lockFileDescriptor, F_SETLK, &fl) == -1) {
        std::cerr << "\033[93mAnother instance of isocmd is already running.\n\033[0m" << std::endl;
        close(lockFileDescriptor);
        return 1;
    }

    // Register signal handlers
    signal(SIGINT, signalHandler);  // Handle Ctrl+C
    signal(SIGTERM, signalHandler); // Handle termination signals
    
    bool exitProgram = false;
    std::string choice;

    while (!exitProgram) {
        clearScrollBuffer();
        print_ascii();
        // Display the main menu options
        printMenu();

        // Clear history
        clear_history();

        // Prompt for the main menu choice
        char* input = readline("\001\033[1;94m\002Choose an option:\001\033[0;1m\002 ");
        if (!input) {
            break; // Exit the program if readline returns NULL (e.g., on EOF or Ctrl+D)
        }

        std::string choice(input);
        free(input); // Free the allocated memory by readline

        if (choice == "1") {
            submenu1();
        } else {
            // Check if the input length is exactly 1
            if (choice.length() == 1) {
                switch (choice[0]) {
                    case '2':
                        submenu2();
                        break;
                    case '3':
                        manualRefreshCache();
                        clearScrollBuffer();
                        break;
                    case '4':
                        exitProgram = true; // Exit the program
                        clearScrollBuffer();
                        break;
                    default:
                        break;
                }
            }
        }
    }

    close(lockFileDescriptor); // Close the file descriptor, releasing the lock
    unlink(lockFile); // Remove the lock file
    return 0;
}


// ... Function definitions ...

// ART

// Print the version number of the program
void printVersionNumber(const std::string& version) {
    
    std::cout << "\x1B[32mIso Commander v" << version << "\x1B[0m\n" << std::endl; // Output the version number in green color
}


// Function to print ascii
void print_ascii() {
    // Display ASCII art

    const char* Color = "\x1B[1;38;5;214m";
    const char* resetColor = "\x1B[0m"; // Reset color to default
	                                                                                                                           
std::cout << Color << R"( (   (       )            )    *      *              ) (         (    
 )\ ))\ ) ( /(      (  ( /(  (  `   (  `    (     ( /( )\ )      )\ ) 
(()/(()/( )\())     )\ )\()) )\))(  )\))(   )\    )\()(()/(  (  (()/( 
 /(_)/(_)((_)\    (((_((_)\ ((_)()\((_)()((((_)( ((_)\ /(_)) )\  /(_))
(_))(_))   ((_)   )\___ ((_)(_()((_(_()((_)\ _ )\ _((_(_))_ ((_)(_))
|_ _/ __| / _ \  ((/ __/ _ \|  \/  |  \/  (_)_\(_| \| ||   \| __| _ \
 | |\__ \| (_) |  | (_| (_) | |\/| | |\/| |/ _ \ | .` || |) | _||   /
|___|___/ \___/    \___\___/|_|  |_|_|  |_/_/ \_\|_|\_||___/|___|_|_\

)" << resetColor;

}


// Function to print submenu1
void submenu1() {

    while (true) {
        clearScrollBuffer();
        std::cout << "\033[1;32m+-------------------------+" << std::endl;
        std::cout << "\033[1;32m|↵ Manage ISO              |" << std::endl;
        std::cout << "\033[1;32m+-------------------------+" << std::endl;
        std::cout << "\033[1;32m|1. Mount                 |" << std::endl;
        std::cout << "\033[1;32m+-------------------------+" << std::endl;
        std::cout << "\033[1;32m|2. Unmount               |" << std::endl;
        std::cout << "\033[1;32m+-------------------------+" << std::endl;
        std::cout << "\033[1;32m|3. Delete                |" << std::endl;
        std::cout << "\033[1;32m+-------------------------+" << std::endl;
        std::cout << "\033[1;32m|4. Move                  |" << std::endl;
        std::cout << "\033[1;32m+-------------------------+" << std::endl;
        std::cout << "\033[1;32m|5. Copy                  |" << std::endl;
        std::cout << "\033[1;32m+-------------------------+" << std::endl;
        std::cout << " " << std::endl;
        char* submenu_input = readline("\001\033[1;94m\002Choose an option:\001\033[0;1m\002 ");

        if (!submenu_input || std::strlen(submenu_input) == 0) {
			free(submenu_input);
			break; // Exit the submenu if input is empty or NULL
		}
					
          std::string submenu_choice(submenu_input);
          free(submenu_input);
         // Check if the input length is exactly 1
        if (submenu_choice.empty() || submenu_choice.length() == 1) {
		std::string operation;
		switch (submenu_choice[0]) {
        case '1':
			clearScrollBuffer();
            select_and_mount_files_by_number();
            clearScrollBuffer();
            break;
        case '2':
			clearScrollBuffer();
            unmountISOs();
            clearScrollBuffer();
            break;
        case '3':
			clearScrollBuffer();
            operation = "rm";
            select_and_operate_files_by_number(operation);
            clearScrollBuffer();
            break;
        case '4':
			clearScrollBuffer();
            operation = "mv";
            select_and_operate_files_by_number(operation);
            clearScrollBuffer();
            break;
        case '5':
			clearScrollBuffer();
            operation = "cp";
            select_and_operate_files_by_number(operation);
            clearScrollBuffer();
            break;
			}
		}
    }
}


// Function to print submenu2
void submenu2() {
	while (true) {
		clearScrollBuffer();
		std::cout << "\033[1;32m+-------------------------+" << std::endl;
		std::cout << "\033[1;32m|↵ Convert2ISO             |" << std::endl;
		std::cout << "\033[1;32m+-------------------------+" << std::endl;
        std::cout << "\033[1;32m|1. CCD2ISO               |" << std::endl;
        std::cout << "\033[1;32m+-------------------------+" << std::endl;
        std::cout << "\033[1;32m|2. MDF2ISO               |" << std::endl;
        std::cout << "\033[1;32m+-------------------------+" << std::endl;
        std::cout << " " << std::endl;
        char* submenu_input = readline("\001\033[1;94m\002Choose an option:\001\033[0;1m\002 ");
        

        if (!submenu_input || std::strlen(submenu_input) == 0) {
			free(submenu_input);
			break; // Exit the submenu if input is empty or NULL
		}
					
          std::string submenu_choice(submenu_input);
          free(submenu_input);
         // Check if the input length is exactly 1
		 if (submenu_choice.empty() || submenu_choice.length() == 1){
         switch (submenu_choice[0]) {		
             case '1':
				clearScrollBuffer();
                select_and_convert_files_to_iso("bin");
                clearScrollBuffer();
                break;
             case '2':
				clearScrollBuffer();
                select_and_convert_files_to_iso("mdf");
                clearScrollBuffer();
                break;
			}
		}
	}	
}


// Function to print menu
void printMenu() {
    std::cout << "\033[1;32m+-------------------------+" << std::endl;
    std::cout << "\033[1;32m|       Menu Options       |" << std::endl;
    std::cout << "\033[1;32m+-------------------------+" << std::endl;
    std::cout << "\033[1;32m|1. Manage ISO            | " << std::endl;
    std::cout << "\033[1;32m+-------------------------+" << std::endl;
    std::cout << "\033[1;32m|2. Convert2ISO           |" << std::endl;
    std::cout << "\033[1;32m+-------------------------+" << std::endl;
    std::cout << "\033[1;32m|3. Refresh ISO Cache     |" << std::endl;
    std::cout << "\033[1;32m+-------------------------+" << std::endl;
    std::cout << "\033[1;32m|4. Exit Program          |" << std::endl;
    std::cout << "\033[1;32m+-------------------------+" << std::endl;
    std::cout << std::endl;
}


// GENERAL STUFF

// Function to filter files based on search query (case-insensitive)
std::vector<std::string> filterFiles(const std::vector<std::string>& files, const std::string& query) {
    // Vector to store filtered file names
    std::vector<std::string> filteredFiles;

    // Set to store query tokens (lowercased)
    std::unordered_set<std::string> queryTokens;

    // Split the query string into tokens using the delimiter ';' and store them in a set
    std::stringstream ss(query);
    std::string token;
    while (std::getline(ss, token, ';')) {
        // Convert token to lowercase
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        // Insert token into the set
        queryTokens.insert(token);
    }

    // Create a thread pool with the desired number of threads
    ThreadPool pool(maxThreads);

    // Vector to store futures for tracking tasks' completion
    std::vector<std::future<void>> futures;

    // Function to filter files
    auto filterTask = [&](size_t start, size_t end) {
        for (size_t i = start; i < end; ++i) {
            const std::string& file = files[i];

            // Find the position of the last '/' character to extract file name
            size_t lastSlashPos = file.find_last_of('/');
            // Extract file name (excluding path) or use the full path if no '/' is found
            std::string fileName = (lastSlashPos != std::string::npos) ? file.substr(lastSlashPos + 1) : file;
            // Convert file name to lowercase
            std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);

            // Flag to track if a match is found for the file
            bool matchFound = false;
            // Iterate through each query token
            for (const std::string& queryToken : queryTokens) {
                // If the file name contains the current query token
                if (fileName.find(queryToken) != std::string::npos) {
                    // Set matchFound flag to true and break out of the loop
                    matchFound = true;
                    break;
                }
            }

            // If a match is found, add the file to the filtered list
            if (matchFound) {
                // Lock access to the shared vector
                std::lock_guard<std::mutex> lock(Mutex4Med);
                filteredFiles.push_back(file);
            }
        }
    };

    // Calculate the number of files per thread
    size_t numFiles = files.size();
    size_t numThreads = maxThreads;
    size_t filesPerThread = numFiles / numThreads;

    // Enqueue filter tasks into the thread pool
    for (size_t i = 0; i < numThreads - 1; ++i) {
        size_t start = i * filesPerThread;
        size_t end = start + filesPerThread;
        futures.emplace_back(pool.enqueue(filterTask, start, end));
    }

    // Handle the remaining files in the main thread
    filterTask((numThreads - 1) * filesPerThread, numFiles);

    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.wait();
    }

    // Return the vector of filtered file names
    return filteredFiles;
}


// Function to clear scrollbuffer
void clearScrollBuffer() {
    std::cout << "\033[3J";  // Clear the scrollback buffer
    std::cout << "\033[2J";  // Clear the screen
    std::cout << "\033[H";   // Move the cursor to the top-left corner
    std::cout.flush();       // Ensure the output is flushed
}


// Function to handle termination signals
void signalHandler(int signum) {
    
    // Perform cleanup before exiting
    if (lockFileDescriptor != -1) {
        close(lockFileDescriptor);
    }
    
    exit(signum);
}


// Function to check if a string consists only of zeros
bool isAllZeros(const std::string& str) {
    return str.find_first_not_of('0') == std::string::npos;
}


// Function to check if a string is numeric
bool isNumeric(const std::string& str) {
    return std::all_of(str.begin(), str.end(), [](char c) {
        return std::isdigit(c);
    });
}


// Function to print ISO files with alternating colors for sequence numbers
void printIsoFileList(const std::vector<std::string>& isoFiles) {
    // ANSI escape codes for text formatting
    const std::string defaultColor = "\033[0m";
    const std::string bold = "\033[1m";
    const std::string red = "\033[31;1m";   // Red color
    const std::string green = "\033[32;1m"; // Green color
    const std::string magenta = "\033[95m";

    bool useRedColor = true; // Start with red color

    for (size_t i = 0; i < isoFiles.size(); ++i) {
        // Determine sequence number
        int sequenceNumber = i + 1;

        // Determine color based on alternating pattern
        std::string sequenceColor = (useRedColor) ? red : green;
        useRedColor = !useRedColor; // Toggle between red and green

        // Print sequence number with the determined color
        std::cout << sequenceColor << std::right << std::setw(2) << sequenceNumber <<". ";

        // Extract directory and filename
        auto [directory, filename] = extractDirectoryAndFilename(isoFiles[i]);

        // Print the directory part in the default color
        std::cout << defaultColor << bold << directory << defaultColor;

        // Print the filename part in bold
        std::cout << bold << "/" << magenta << filename << defaultColor << std::endl;
    }
}


//	CACHE STUFF

// Function to check if a file exists asynchronously
std::future<std::vector<std::string>> FileExistsAsync(const std::vector<std::string>& paths) {
    return std::async(std::launch::async, [paths]() {
        std::vector<std::string> result;
        for (const auto& path : paths) {
            if (std::filesystem::exists(path)) {
                result.push_back(path);
            }
        }
        return result;
    });
}


// Function to remove non-existent paths from cache asynchronously with basic thread control
void removeNonExistentPathsFromCache() {
    // Define the path to the cache file
    const std::string cacheFilePath = std::string(getenv("HOME")) + "/.cache/iso_commander_cache.txt";

    // Open the cache file for reading
    std::ifstream cacheFile(cacheFilePath, std::ios::in | std::ios::binary);
    if (!cacheFile.is_open()) {
        // Handle error if unable to open cache file
        return;
    }

    // Get the file size
    const auto fileSize = cacheFile.seekg(0, std::ios::end).tellg();
    cacheFile.seekg(0, std::ios::beg);

    // Open the file for memory mapping
    int fd = open(cacheFilePath.c_str(), O_RDONLY);
    if (fd == -1) {
        // Handle error if unable to open the file
        return;
    }

    // Memory map the file
    char* mappedFile = static_cast<char*>(mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0));
    if (mappedFile == MAP_FAILED) {
        // Handle error if unable to map the file
        close(fd);
        return;
    }

    // Process the memory-mapped file
    std::vector<std::string> cache;
    char* start = mappedFile;
    char* end = mappedFile + fileSize;
    while (start < end) {
        char* lineEnd = std::find(start, end, '\n');
        cache.emplace_back(start, lineEnd);
        start = lineEnd + 1;
    }

    // Unmap the file
    munmap(mappedFile, fileSize);
    close(fd);

    // Calculate dynamic batch size based on the number of available processor cores
    const std::size_t maxThreads = std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 2;
    const size_t batchSize = std::max(cache.size() / maxThreads + 1, static_cast<std::size_t>(2));

    // Create a vector to hold futures for asynchronous tasks
    std::vector<std::future<std::vector<std::string>>> futures;
    futures.reserve(cache.size() / batchSize + 1); // Reserve memory for futures

    // Process paths in dynamic batches
    for (size_t i = 0; i < cache.size(); i += batchSize) {
        auto begin = cache.begin() + i;
        auto end = std::min(begin + batchSize, cache.end());
        futures.push_back(std::async(std::launch::async, [begin, end]() {
            // Process batch
            std::future<std::vector<std::string>> futureResult = FileExistsAsync({begin, end});
            return futureResult.get();
        }));
    }

    // Wait for all asynchronous tasks to complete and collect the results
    std::vector<std::string> retainedPaths;
    retainedPaths.reserve(cache.size()); // Reserve memory for retained paths
    for (auto& future : futures) {
        auto result = future.get();
        // Protect the critical section with a mutex
        {
            std::lock_guard<std::mutex> highLock(Mutex4High);
            retainedPaths.insert(retainedPaths.end(), std::make_move_iterator(result.begin()), std::make_move_iterator(result.end()));
        }
    }

    // Open the cache file for writing
    std::ofstream updatedCacheFile(cacheFilePath);
    if (!updatedCacheFile.is_open()) {
        // Handle error if unable to open cache file for writing
        return;
    }

    // Write the retained paths to the updated cache file
    for (const std::string& path : retainedPaths) {
        updatedCacheFile << path << '\n';
    }

    // RAII: Close the updated cache file automatically when it goes out of scope
}


// Set default cache dir
std::string getHomeDirectory() {
    const char* homeDir = getenv("HOME");
    if (homeDir) {
        return std::string(homeDir);
    }
    return "";
}


// Load cache
std::vector<std::string> loadCache() {
    std::vector<std::string> isoFiles;

    std::string cacheFilePath = getHomeDirectory() + "/.cache/iso_commander_cache.txt";

    // Open the file for memory mapping
    int fd = open(cacheFilePath.c_str(), O_RDONLY);
    if (fd == -1) {
        // Handle error if unable to open the file
        return isoFiles;
    }

    // Get the file size
    struct stat fileStat;
    if (fstat(fd, &fileStat) == -1) {
        // Handle error if unable to get file statistics
        close(fd);
        return isoFiles;
    }
    const auto fileSize = fileStat.st_size;

    // Memory map the file
    char* mappedFile = static_cast<char*>(mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0));
    if (mappedFile == MAP_FAILED) {
        // Handle error if unable to map the file
        close(fd);
        return isoFiles;
    }

    // Process the memory-mapped file
    char* start = mappedFile;
    char* end = mappedFile + fileSize;
    while (start < end) {
        char* lineEnd = std::find(start, end, '\n');
        std::string line(start, lineEnd);
        if (!line.empty()) {
            isoFiles.push_back(std::move(line));
        }
        start = lineEnd + 1;
    }

    // Unmap the file
    munmap(mappedFile, fileSize);
    close(fd);

    // Remove duplicates from the loaded cache
    std::sort(isoFiles.begin(), isoFiles.end());
    isoFiles.erase(std::unique(isoFiles.begin(), isoFiles.end()), isoFiles.end());

    return isoFiles;
}


// Function to check if filepath exists
bool exists(const std::filesystem::path& path) {
    return std::filesystem::exists(path);
}


// Save cache
bool saveCache(const std::vector<std::string>& isoFiles, std::size_t maxCacheSize) {
    std::filesystem::path cachePath = cacheDirectory;
    cachePath /= cacheFileName;

    // Check if cache directory exists
    if (!exists(cacheDirectory) || !std::filesystem::is_directory(cacheDirectory)) {
		std::cout << " " << std::endl;
        std::cerr << "\033[1;91mInvalid cache directory.\033[0;1m" << std::endl;
        return false;  // Cache save failed
    }

    // Load the existing cache
    std::vector<std::string> existingCache = loadCache();

    // Combine new and existing entries and remove duplicates
    std::set<std::string> combinedCache(existingCache.begin(), existingCache.end());
    for (const std::string& iso : isoFiles) {
        combinedCache.insert(iso);
    }

    // Limit the cache size to the maximum allowed size
    while (combinedCache.size() > maxCacheSize) {
        combinedCache.erase(combinedCache.begin());
    }

    // Open the cache file in write mode (truncating it)
    std::ofstream cacheFile(cachePath, std::ios::out | std::ios::trunc);
    if (cacheFile.is_open()) {
        for (const std::string& iso : combinedCache) {
            cacheFile << iso << "\n";
        }

        // Check if writing to the file was successful
        if (cacheFile.good()) {
            cacheFile.close();
            return true;  // Cache save successful
        } else {
			std::cout << " " << std::endl;
            std::cerr << "\033[1;91mFailed to write to cache file.\033[0;1m" << std::endl;
            cacheFile.close();
            return false;  // Cache save failed
        }
    } else {
		std::cout << " " << std::endl;
        std::cerr << "\033[1;91mFailed to open ISO cache file: \033[1;93m'"<< cacheDirectory + "/" + cacheFileName <<"'\033[1;91m. Check read/write permissions.\033[0;1m" << std::endl;
        return false;  // Cache save failed
    }
}


// Function to check if a directory input is valid
bool isValidDirectory(const std::string& path) {
    return std::filesystem::is_directory(path);
}


// Function to refresh the cache for a single directory
void refreshCacheForDirectory(const std::string& path, std::vector<std::string>& allIsoFiles) {
    if (promptFlag) {
    std::cout << "\033[1;93mProcessing directory path: '" << path << "'.\033[0m" << std::endl;
	}
    std::vector<std::string> newIsoFiles;

    // Perform the cache refresh for the directory (e.g., using parallelTraverse)
    parallelTraverse(path, newIsoFiles, Mutex4Low);

    // Check if the gap has been printed, if not, print it
    if (!gapPrinted && promptFlag) {
        std::cout << " " << std::endl;
        gapPrinted = true; // Set the flag to true to indicate that the gap has been printed
    }
    std::lock_guard<std::mutex> MedLock(Mutex4Med);
    // Append the new entries to the shared vector
    allIsoFiles.insert(allIsoFiles.end(), newIsoFiles.begin(), newIsoFiles.end());
	if (promptFlag) {
    std::cout << "\033[1;92mProcessed directory path: '" << path << "'.\033[0m" << std::endl;
	}
}


// Function for manual cache refresh
void manualRefreshCache(const std::string& initialDir) {
    if (promptFlag){
    clearScrollBuffer();
    gapPrinted = false;
	}
    // Load history from file
    loadHistory();

    std::string inputLine;

    // Append the initial directory if provided
    if (!initialDir.empty()) {
        inputLine = initialDir;
    } else {
        // Prompt the user to enter directory paths for manual cache refresh
        inputLine = readInputLine("\033[1;94mDirectory path(s) ↵ to build/refresh the \033[1m\033[1;92mISO Cache\033[94m (multi-path separator: \033[1m\033[1;93m;\033[0m\033[1;94m), or ↵ to return:\n\033[0;1m");
    }

    if (!inputLine.empty()) {
        // Save history to file
        saveHistory();
    }

    // Check if the user canceled the cache refresh
    if (inputLine.empty()) {
        return;
    }

    // Create an input string stream to parse directory paths
    std::istringstream iss(inputLine);
    std::string path;

    // Vector to store all ISO files from multiple directories
    std::vector<std::string> allIsoFiles;

    // Vector to store valid directory paths
    std::vector<std::string> validPaths;

    // Vector to store invalid paths
    std::vector<std::string> invalidPaths;

    // Set to store processed invalid paths
    std::set<std::string> processedInvalidPaths;
    
    // Set to store processed valid paths
    std::set<std::string> processedValidPaths;

    std::vector<std::future<void>> futures;

    // Iterate through the entered directory paths and print invalid paths
    while (std::getline(iss, path, ';')) {
        // Check if the directory path is valid
        if (isValidDirectory(path)) {
            validPaths.push_back(path); // Store valid paths
        } else {
            // Check if the path has already been processed
            if (processedInvalidPaths.find(path) == processedInvalidPaths.end()) {
                // Print the error message and mark the path as processed
                invalidPaths.push_back("\033[1;91mInvalid directory path(s): '" + path + "'. Skipped from processing.\033[0m");
                processedInvalidPaths.insert(path);
            }
        }
    }

    // Check if any invalid paths were encountered and add a gap
    if ((!invalidPaths.empty() || !validPaths.empty()) && promptFlag) {
        std::cout << " " << std::endl;
    }

    // Print invalid paths
    for (const auto& invalidPath : invalidPaths) {
        std::cout << invalidPath << std::endl;
    }

    if (!invalidPaths.empty() && !validPaths.empty() && promptFlag) {
        std::cout << " " << std::endl;
    }

    // Start the timer
    auto start_time = std::chrono::high_resolution_clock::now();

    // Create a task for each valid directory to refresh the cache and pass the vector by reference
    std::istringstream iss2(inputLine); // Reset the string stream
    std::size_t runningTasks = 0;  // Track the number of running tasks
    
    while (std::getline(iss2, path, ';')) {
        // Check if the directory path is valid
        if (!isValidDirectory(path)) {
            continue; // Skip invalid paths
        }

        // Check if the path has already been processed
        if (processedValidPaths.find(path) != processedValidPaths.end()) {
            continue; // Skip already processed valid paths
        }

        // Add a task to the thread pool for refreshing the cache for each directory
        futures.emplace_back(std::async(std::launch::async, refreshCacheForDirectory, path, std::ref(allIsoFiles)));

        ++runningTasks;

        // Mark the path as processed
        processedValidPaths.insert(path);

        // Check if the number of running tasks has reached the maximum allowed
        if (runningTasks >= maxThreads) {
            // Wait for the tasks to complete
            for (auto& future : futures) {
                future.wait();
            }
            // Clear completed tasks from the vector
            futures.clear();
            runningTasks = 0;  // Reset the count of running tasks
            std::cout << " " << std::endl;
            gapPrinted = false;
        }
    }

    // Wait for the remaining tasks to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    // Save the combined cache to disk
    bool saveSuccess = saveCache(allIsoFiles, maxCacheSize);

    // Stop the timer after completing the cache refresh and removal of non-existent paths
    auto end_time = std::chrono::high_resolution_clock::now();
    
    if (promptFlag) {

    // Calculate and print the elapsed time
    std::cout << " " << std::endl;
    auto total_elapsed_time = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();

    // Print the time taken for the entire process in bold with one decimal place
    std::cout << "\033[1mTotal time taken: " << std::fixed << std::setprecision(1) << total_elapsed_time << " seconds\033[0m" << std::endl;

    // Inform the user about the cache refresh status
    if (saveSuccess && !validPaths.empty() && invalidPaths.empty()) {
        std::cout << " " << std::endl;
        std::cout << "\033[1;92mCache refreshed successfully.\033[0m" << std::endl;
        std::cout << " " << std::endl;
    } 
    if (saveSuccess && !validPaths.empty() && !invalidPaths.empty()) {
        std::cout << " " << std::endl;
        std::cout << "\033[1;93mCache refreshed with errors from invalid path(s).\033[0m" << std::endl;
        std::cout << " " << std::endl;
    }
    if (saveSuccess && validPaths.empty() && !invalidPaths.empty()) {
        std::cout << " " << std::endl;
        std::cout << "\033[1;91mCache refresh failed due to missing valid path(s).\033[0m" << std::endl;
        std::cout << " " << std::endl;
    } 
    if (!saveSuccess) {
        std::cout << " " << std::endl;
        std::cout << "\033[1;91mCache refresh failed.\033[0m" << std::endl;
        std::cout << " " << std::endl;
    }
    std::cout << "\033[1;32m↵ to continue...\033[0;1m";
    std::cin.get();
	}
	promptFlag = true;
}


//Function to perform case-insensitive string comparison using std::string_view asynchronously
std::future<bool> iequals(std::string_view a, std::string_view b) {
    // Using std::async to perform the comparison asynchronously
    return std::async(std::launch::async, [a, b]() {
        // Check if the string views have different sizes, if so, they can't be equal
        if (a.size() != b.size()) {
            return false;
        }

        // Iterate through each character of the string views and compare them
        for (std::size_t i = 0; i < a.size(); ++i) {
            // Convert characters to lowercase using std::tolower and compare them
            if (std::tolower(a[i]) != std::tolower(b[i])) {
                // If characters are not equal, strings are not equal
                return false;
            }
        }

        // If all characters are equal, the strings are case-insensitively equal
        return true;
    });
}


// Function to check if a string ends with ".iso" (case-insensitive)
bool ends_with_iso(const std::string& str) {
    // Convert the string to lowercase
    std::string lowercase = str;
    std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), ::tolower);
    // Check if the string ends with ".iso" by comparing the last 4 characters
    return (lowercase.size() >= 4) && (lowercase.compare(lowercase.size() - 4, 4, ".iso") == 0);
}


// Function to parallel traverse a directory and find ISO files
void parallelTraverse(const std::filesystem::path& path, std::vector<std::string>& isoFiles, std::mutex& Mutex4Low) {
    try {
        // Vector to store futures for asynchronous tasks
        std::vector<std::future<void>> futures;

        // Iterate over entries in the specified directory and its subdirectories
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            // Check if the entry is a regular file
            if (entry.is_regular_file()) {
                const std::filesystem::path& filePath = entry.path();

                // Check file size and skip if less than 5MB or empty, or if it has a ".bin" extension
                const auto fileSize = std::filesystem::file_size(filePath);
                if (fileSize < 5 * 1024 * 1024 || fileSize == 0 || iequals(filePath.stem().string(), ".bin").get()) {
                    continue;
                }

                // Extract the file extension
                std::string_view extension = filePath.extension().string();

                // Check if the file has a ".iso" extension
                if (iequals(extension, ".iso").get()) {
                    // Asynchronously push the file path to the isoFiles vector while protecting access with a mutex
                    futures.push_back(std::async(std::launch::async, [filePath, &isoFiles, &Mutex4Low]() {
                        std::lock_guard<std::mutex> lowLock(Mutex4Low);
                        isoFiles.push_back(filePath.string());
                    }));
                }
            }
        }

        // Wait for all asynchronous tasks to complete
        for (auto& future : futures) {
            future.get();
        }
    } catch (const std::filesystem::filesystem_error& e) {
        // Handle filesystem errors, print a message, and introduce a 2-second delay
        std::cerr << "\033[1;91m" << e.what() << ".\033[0;1m" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}


//	MOUNT STUFF

// Function to mount all ISO files
void mountAllIsoFiles(const std::vector<std::string>& isoFiles, std::unordered_set<std::string>& mountedSet, std::vector<std::string>& isoFilesToMount) {
    
    // Detect and use the minimum of available threads and ISOs to ensure efficient parallelism
    unsigned int numThreads = std::min(static_cast<int>(isoFiles.size()), static_cast<int>(maxThreads));

    // Create a ThreadPool with maxThreads
    ThreadPool pool(numThreads);
      
    // Process all ISO files asynchronously
	for (size_t i = 0; i < isoFiles.size(); ++i) {
        // Capture isoFiles and mountedSet by reference, and i by value
        pool.enqueue([i, &isoFiles, &mountedSet, &isoFilesToMount]() {
        // Create a vector containing the single ISO file to mount
        std::vector<std::string> isoFilesToMountLocal = { isoFiles[i] }; // Assuming isoFiles is 1-based indexed

        // Call mountIsoFile with the vector of ISO files to mount and the mounted set
		mountIsoFile(isoFilesToMountLocal, mountedSet);
		});
	}
}


// Function to select and mount ISO files by number
void select_and_mount_files_by_number() {
    // Remove non-existent paths from the cache
    removeNonExistentPathsFromCache();

    // Load ISO files from cache
    std::vector<std::string> isoFiles = loadCache();

    // Check if the cache is empty
    if (isoFiles.empty()) {
        clearScrollBuffer();
        std::cout << "\033[1;93mISO Cache is empty. Please refresh it from the main Menu Options.\033[0;1m" << std::endl;
        std::cout << " " << std::endl;
        std::cout << "\033[1;32m↵ to continue...\033[0;1m";
        std::cin.get();
        return;
    }

    // Set to track mounted ISO files
    std::unordered_set<std::string> mountedSet;
    std::vector<std::string> isoFilesToMount;

    // Main loop for selecting and mounting ISO files
    while (true) {
		std::vector<std::string> isoFilesToMount;
		bool verboseFiltered = false;
        clearScrollBuffer();
        std::cout << "\033[1;93m ! IF EXPECTED ISO FILE(S) NOT ON THE LIST REFRESH ISO CACHE FROM THE MAIN MENU OPTIONS !\033[0;1m" << std::endl;
        std::cout << "\033[1;93m         	! ROOT ACCESS IS PARAMOUNT FOR SUCCESSFUL MOUNTS !\n\033[0;1m" << std::endl;

        // Remove non-existent paths from the cache after selection
        removeNonExistentPathsFromCache();

        // Load ISO files from cache
        isoFiles = loadCache();
        std::string searchQuery;
        std::vector<std::string> filteredFiles;
        printIsoFileList(isoFiles);
		
        // Prompt user for input
        char* input = readline(
        "\n\001\033[1;92m\002ISO(s)\001\033[1;94m\002 ↵ for \001\033[1;92m\002mount\001\033[1;94m\002 (e.g., '1-3', '1 5', '00' for all), / ↵ to filter, or ↵ to return:\001\033[0;1m\002 "
    );
        clearScrollBuffer();
        
        if (strcmp(input, "/") != 0 || (!(std::isspace(input[0]) || input[0] == '\0'))) {
			std::cout << "\033[1mPlease wait...\033[1m" << std::endl;
		}

        // Check if the user wants to return
        if (std::isspace(input[0]) || input[0] == '\0') {
			free(input);
            break;
        }

		if (strcmp(input, "/") == 0) {
			free(input);
			verboseFiltered = true;
			
			while (true) {
			
			clearScrollBuffer();
			historyPattern = true;
			loadHistory();
			
			// User pressed '/', start the filtering process
			std::string prompt = "\n\001\033[1;92m\002SearchQuery\001\033[1;94m\002 ↵ to filter \001\033[1;92m\002mount\001\033[1;94m\002 list (case-insensitive, multi-term separator: \001\033[1;93m\002;\001\033[1;94m\002), or ↵ to return: \001\033[0;1m\002";
			
			char* searchQuery = readline(prompt.c_str());
			clearScrollBuffer();
			
			
			if (searchQuery && searchQuery[0] != '\0') {
				std::cout << "\033[1mPlease wait...\033[1m" << std::endl;
				add_history(searchQuery); // Add the search query to the history
				saveHistory();
			}
			clear_history();
			
			// Store the original isoFiles vector
			std::vector<std::string> originalIsoFiles = isoFiles;
			// Check if the user wants to return
        if (!(std::isspace(searchQuery[0]) || searchQuery[0] == '\0')) {
        

			if (searchQuery != nullptr) {
				std::vector<std::string> filteredFiles = filterFiles(isoFiles, searchQuery);
				free(searchQuery);

				if (filteredFiles.empty()) {
					clearScrollBuffer();
					std::cout << "\033[1;91mNo ISO(s) match the search query.\033[0;1m\n";
					std::cout << "\n\033[1;32m↵ to continue...\033[0;1m";
					std::cin.get();
				} else {
					while (true) {
						clearScrollBuffer();
						std::cout << "\033[1mFiltered results:\n\033[0;1m" << std::endl;
						printIsoFileList(filteredFiles); // Print the filtered list of ISO files
					
						// Prompt user for input again with the filtered list
						char* input = readline("\n\001\033[1;92m\002ISO(s)\001\033[1;94m\002 ↵ for \001\033[1;92m\002mount\001\033[1;94m\002 (e.g., '1-3', '1 5', '00' for all), or ↵ to return:\001\033[0;1m\002 ");
					
						// Check if the user wants to return
						if (std::isspace(input[0]) || input[0] == '\0') {
							free(input);
							historyPattern = false;
							break;
						}
					
						if (std::strcmp(input, "00") == 0) {
							clearScrollBuffer();
							std::cout << "\033[1mPlease wait...\033[1m" << std::endl;
							// Restore the original list of ISO files
							isoFiles = filteredFiles;
							verboseFiltered = false;
							mountAllIsoFiles(isoFiles, mountedSet, isoFilesToMount);
						}

						// Check if the user provided input
						if (input[0] != '\0' && (strcmp(input, "/") != 0)) {
							clearScrollBuffer();
							std::cout << "\033[1mPlease wait...\033[1m" << std::endl;

							// Process the user input with the filtered list
							processAndMountIsoFiles(input, filteredFiles, mountedSet);
							free(input);
						
							clearScrollBuffer();

							verbose(mountedFiles, skippedMessages, uniqueErrorMessages);
						}
					}
				}	
			} 
				} else {
					free(searchQuery);
					historyPattern = false;
					verboseFiltered = true;
					isoFiles = originalIsoFiles; // Revert to the original cache list
					break;
			}
		}
	}

        // Check if the user wants to mount all ISO files
		if (std::strcmp(input, "00") == 0) {
			mountAllIsoFiles(isoFiles, mountedSet, isoFilesToMount);
		}
        if (input[0] != '\0' && (strcmp(input, "/") != 0) && !verboseFiltered) {
            // Process user input to select and mount specific ISO files
            processAndMountIsoFiles(input, isoFiles, mountedSet);
            clearScrollBuffer();
            verbose(mountedFiles, skippedMessages, uniqueErrorMessages);
            free(input);
        }          
    }
}


// Function to print mount verbose messages
void verbose(std::vector<std::string>& mountedFiles,std::vector<std::string>& skippedMessages,std::unordered_set<std::string>& uniqueErrorMessages) {
    if (!mountedFiles.empty()) {
        std::cout << " " << std::endl;
    }

    // Print all mounted files
    for (const auto& mountedFile : mountedFiles) {
        std::cout << mountedFile << std::endl;
    }

    if (!skippedMessages.empty()) {
        std::cout << " " << std::endl;
    }

    // Print all the stored skipped messages
    for (const auto& skippedMessage : skippedMessages) {
        std::cerr << skippedMessage;
    }

    if (!uniqueErrorMessages.empty()) {
        std::cout << " " << std::endl;
    }

    // Print all the stored error messages
    for (const auto& errorMessage : uniqueErrorMessages) {
        std::cerr << errorMessage;
    }

    // Clear the vectors after each iteration
    mountedFiles.clear();
    skippedMessages.clear();
    uniqueErrorMessages.clear();
    
    std::cout << " " << std::endl;
	std::cout << "\033[1;32m↵ to continue...\033[0;1m";
	std::cin.get();
}


// Function to mount selected ISO files called from processAndMountIsoFiles
void mountIsoFile(const std::vector<std::string>& isoFilesToMount, std::unordered_set<std::string>& mountedSet) {
    // Lock the global mutex for synchronization
    std::lock_guard<std::mutex> lowLock(Mutex4Low);

    namespace fs = std::filesystem;

    for (const auto& isoFile : isoFilesToMount) {
        // Use the filesystem library to extract the ISO file name
        fs::path isoPath(isoFile);
        std::string isoFileName = isoPath.stem().string(); // Remove the .iso extension

        // Use the modified ISO file name in the mount point with "iso_" prefix
        std::string mountPoint = "/mnt/iso_" + isoFileName;

        auto [mountisoDirectory, mountisoFilename] = extractDirectoryAndFilename(mountPoint);
        auto [isoDirectory, isoFilename] = extractDirectoryAndFilename(isoFile);

            // Asynchronously check and create the mount point directory
            auto future = std::async(std::launch::async, [&mountPoint]() {
                if (!fs::exists(mountPoint)) {
                    fs::create_directory(mountPoint);
                }
            });

            // Wait for the asynchronous operation to complete
            future.wait();

            // Check if the mount point is already mounted
            if (isAlreadyMounted(mountPoint)) {
                // If already mounted, print a message and continue
                std::stringstream skippedMessage;
                skippedMessage << "\033[1;93mISO: \033[1;92m'" << isoDirectory << "/" << isoFilename << "'\033[1;93m already mounted at: \033[1;94m'" << mountisoDirectory << "/" << mountisoFilename << "'\033[1;93m.\033[0;1m" << std::endl;

                // Create the unordered set after populating skippedMessages
                std::unordered_set<std::string> skippedSet(skippedMessages.begin(), skippedMessages.end());

                // Check for duplicates
                if (skippedSet.find(skippedMessage.str()) == skippedSet.end()) {
                    // Error message not found, add it to the vector
                    skippedMessages.push_back(skippedMessage.str());
                }

                continue; // Skip mounting this ISO file
            }

            // Initialize libmount context
            struct libmnt_context *cxt = mnt_new_context();

            // Initialize the source and target paths
            struct libmnt_cache *cache = mnt_new_cache();
            struct libmnt_fs *fs = mnt_new_fs();
            mnt_fs_set_source(fs, isoFile.c_str());
            mnt_fs_set_target(fs, mountPoint.c_str());

            // Set the mount options
            mnt_fs_set_fstype(fs, "iso9660");
            mnt_fs_set_options(fs, "loop");

            // Associate the libmnt_fs object with the libmnt_context
            mnt_context_set_fs(cxt, fs);

            // Mount the ISO file
            int ret = mnt_context_mount(cxt);
            if (ret != 0) {
                // Handle mount error
                std::stringstream errorMessage;
                errorMessage << "\033[1;91mFailed to mount: \033[1;93m'" << isoDirectory << "/" << isoFilename << "'\033[0;1m\033[1;91m.\033[0;1m" << std::endl;
                fs::remove(mountPoint);
                std::unordered_set<std::string> errorSet(uniqueErrorMessages.begin(), uniqueErrorMessages.end());
                    if (errorSet.find(errorMessage.str()) == errorSet.end()) {
                        // Error message not found, add it to the vector
                        uniqueErrorMessages.insert(errorMessage.str());
                    }
            } else {
                // Mount successful
                mountedSet.insert(mountPoint);
                std::string mountedFileInfo = "\033[1mISO: \033[1;92m'" + isoDirectory + "/" + isoFilename + "'\033[0;1m"
                                              + "\033[1m mounted at: \033[1;94m'" + mountisoDirectory + "/" + mountisoFilename + "'\033[0;1m\033[1m.\033[0;1m";
                mountedFiles.push_back(mountedFileInfo);
            }

            // Clean up libmount resources
            mnt_free_fs(fs);
            mnt_free_cache(cache);
            mnt_free_context(cxt);
    }
}


// Function to process input and mount ISO files asynchronously
void processAndMountIsoFiles(const std::string& input, const std::vector<std::string>& isoFiles, std::unordered_set<std::string>& mountedSet) {
    std::istringstream iss(input);  // Create an input string stream from the input string

    // Determine the number of threads to use, based on the number of ISO files and hardware concurrency
    unsigned int numThreads = std::min(static_cast<int>(isoFiles.size()), static_cast<int>(std::thread::hardware_concurrency()));

    bool invalidInput = false;  // Flag to indicate invalid input
    std::unordered_set<int> processedIndices;  // Set to track processed indices
    std::unordered_set<int> validIndices;      // Set to track valid indices
    std::set<std::pair<int, int>> processedRanges;  // Set to track processed ranges

    ThreadPool pool(numThreads);  // Thread pool with the determined number of threads
    std::mutex MutexForProcessedIndices;  // Mutex for protecting access to processedIndices
    std::mutex MutexForValidIndices;      // Mutex for protecting access to validIndices

    std::string token;
    while (iss >> token) {  // Iterate through each token in the input
        if (token == "/") {  // Break the loop if a '/' is encountered
            break;
        }

        // Handle special case for input "00"
        if (token != "00" && isAllZeros(token)) {
            if (!invalidInput) {
                invalidInput = true;
                uniqueErrorMessages.insert("\033[1;91mFile index '0' does not exist.\033[0;1m\n");
            }
            continue;
        }

        // Check if the token contains a dash ('-'), indicating a range
        size_t dashPos = token.find('-');
        if (dashPos != std::string::npos) {
            // Check for invalid range format
            if (token.find('-', dashPos + 1) != std::string::npos || 
                (dashPos == 0 || dashPos == token.size() - 1 || !std::isdigit(token[dashPos - 1]) || !std::isdigit(token[dashPos + 1]))) {
                invalidInput = true;
                uniqueErrorMessages.insert("\033[1;91mInvalid input: '" + token + "'.\033[0;1m\n");
                continue;
            }

            int start, end;
            try {
                // Parse the start and end of the range
                start = std::stoi(token.substr(0, dashPos));
                end = std::stoi(token.substr(dashPos + 1));
            } catch (const std::invalid_argument&) {
                invalidInput = true;
                uniqueErrorMessages.insert("\033[1;91mInvalid input: '" + token + "'.\033[0;1m\n");
                continue;
            } catch (const std::out_of_range&) {
                invalidInput = true;
                uniqueErrorMessages.insert("\033[1;91mInvalid range: '" + token + "'. Ensure that numbers align with the list.\033[0;1m\n");
                continue;
            }

            // Validate range
            if (start < 1 || static_cast<size_t>(start) > isoFiles.size() || end < 1 || static_cast<size_t>(end) > isoFiles.size()) {
                invalidInput = true;
                uniqueErrorMessages.insert("\033[1;91mInvalid range: '" + std::to_string(start) + "-" + std::to_string(end) + "'. Ensure that numbers align with the list.\033[0;1m\n");
                continue;
            }

            std::pair<int, int> range(start, end);
            if (processedRanges.find(range) == processedRanges.end()) {
                processedRanges.insert(range);

                // Determine step direction and iterate through the range
                int step = (start <= end) ? 1 : -1;
                for (int i = start; (start <= end) ? (i <= end) : (i >= end); i += step) {
                    if (processedIndices.find(i) == processedIndices.end()) {
                        processedIndices.insert(i);

                        pool.enqueue([&, i]() {  // Enqueue a task to the thread pool
                            std::lock_guard<std::mutex> validLock(MutexForValidIndices);  // Protect access to validIndices
                            if (validIndices.find(i) == validIndices.end()) {
                                validIndices.insert(i);
                                std::vector<std::string> isoFilesToMount;
                                isoFilesToMount.push_back(isoFiles[i - 1]);
                                mountIsoFile(isoFilesToMount, mountedSet);  // Mount the ISO file
                            }
                        });
                    }
                }
            }
        } else if (isNumeric(token)) {  // Handle single numeric token
            int num = std::stoi(token);
            if (num >= 1 && static_cast<size_t>(num) <= isoFiles.size() && processedIndices.find(num) == processedIndices.end()) {
                processedIndices.insert(num);

                pool.enqueue([&, num]() {  // Enqueue a task to the thread pool
                    std::lock_guard<std::mutex> validLock(MutexForValidIndices);  // Protect access to validIndices
                    if (validIndices.find(num) == validIndices.end()) {
                        validIndices.insert(num);
                        std::vector<std::string> isoFilesToMount;
                        isoFilesToMount.push_back(isoFiles[num - 1]);
                        mountIsoFile(isoFilesToMount, mountedSet);  // Mount the ISO file
                    }
                });
            } else if (static_cast<std::vector<std::string>::size_type>(num) > isoFiles.size()) {
                invalidInput = true;
                uniqueErrorMessages.insert("\033[1;91mFile index '" + std::to_string(num) + "' does not exist.\033[0;1m\n");
            }
        } else {  // Handle invalid token
            invalidInput = true;
            uniqueErrorMessages.insert("\033[1;91mInvalid input: '" + token + "'.\033[0;1m\n");
        }
    }
}


// Function to check if an ISO is already mounted
bool isAlreadyMounted(const std::string& mountPoint) {
    FILE* mountTable = setmntent("/proc/mounts", "r");
    if (!mountTable) {
        // Failed to open mount table
        return false;
    }

    mntent* entry;
    while ((entry = getmntent(mountTable)) != nullptr) {
        if (std::strcmp(entry->mnt_dir, mountPoint.c_str()) == 0) {
            // Found the mount point in the mount table
            endmntent(mountTable);
            return true;
        }
    }

    endmntent(mountTable);
    return false;
}


// UMOUNT STUFF


// Function to list mounted ISOs in the /mnt directory
void listMountedISOs() {
    // Path where ISO directories are expected to be mounted
    const std::string isoPath = "/mnt";

    // Vector to store names of mounted ISOs
    static std::mutex mtx;
    std::vector<std::string> isoDirs;

    // Lock mutex for accessing shared resource
    std::lock_guard<std::mutex> lock(mtx);

    // Open the /mnt directory and find directories with names starting with "iso_"
    DIR* dir;
    struct dirent* entry;

    if ((dir = opendir(isoPath.c_str())) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            // Check if the entry is a directory and has a name starting with "iso_"
            if (entry->d_type == DT_DIR && std::string(entry->d_name).find("iso_") == 0) {
                // Build the full path and extract the ISO name
                std::string fullDirPath = isoPath + "/" + entry->d_name;
                std::string isoName = entry->d_name + 4; // Remove "/mnt/iso_" part
                isoDirs.push_back(isoName);
            }
        }
        closedir(dir);
    } else {
        // Print an error message if there is an issue opening the /mnt directory
        std::cerr << "\033[1;91mError opening the /mnt directory.\033[0;1m" << std::endl;
        return;
    }

    // Sort ISO directory names alphabetically in a case-insensitive manner
    std::sort(isoDirs.begin(), isoDirs.end(), [](const std::string& a, const std::string& b) {
        // Convert both strings to lowercase before comparison
        std::string lowerA = a;
        std::transform(lowerA.begin(), lowerA.end(), lowerA.begin(), [](unsigned char c) { return std::tolower(c); });

        std::string lowerB = b;
        std::transform(lowerB.begin(), lowerB.end(), lowerB.begin(), [](unsigned char c) { return std::tolower(c); });

        return lowerA < lowerB;
    });

    // Display a list of mounted ISOs with ISO names in bold and alternating colors
    if (!isoDirs.empty()) {
        std::cout << "\033[0;1mList of mounted ISO(s):\033[0;1m" << std::endl; // White and bold
        std::cout << " " << std::endl;

        bool useRedColor = true; // Start with red color for sequence numbers

        for (size_t i = 0; i < isoDirs.size(); ++i) {
            // Determine color based on alternating pattern
            std::string sequenceColor = (useRedColor) ? "\033[31;1m" : "\033[32;1m";
            useRedColor = !useRedColor; // Toggle between red and green

            // Print sequence number with the determined color
            std::cout << sequenceColor << std::setw(2) << i + 1 << ". ";

            // Print ISO directory path in bold and magenta
            std::cout << "\033[0;1m/mnt/iso_\033[1m\033[95m" << isoDirs[i] << "\033[0;1m" << std::endl;
        }
    }
}


//function to check if directory is empty for unmountISO
bool isDirectoryEmpty(const std::string& path) {
    namespace fs = std::filesystem;

    // Create a path object from the given string
    fs::path dirPath(path);

    // Check if the path exists and is a directory
    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
        // Handle the case when the path is invalid or not a directory
        return false;
    }

    // Iterate over the directory entries at the surface level
    auto dirIter = fs::directory_iterator(dirPath);
    auto end = fs::directory_iterator(); // Default constructor gives the "end" iterator

    // Check if there are any entries in the directory
    if (dirIter != end) {
        // If we find any entry (file or subdirectory) in the directory, it's not empty
        return false;
    }

    // If we reach here, it means the directory is empty
    return true;
}


// Function to unmount ISO files asynchronously
void unmountISO(const std::vector<std::string>& isoDirs) {
    // Determine batch size based on the number of isoDirs
    size_t batchSize = 1;
    if (isoDirs.size() > 100000 && isoDirs.size() > maxThreads) {
        batchSize = 100;
    } else if (isoDirs.size() > 10000 && isoDirs.size() > maxThreads) {
        batchSize = 50;
    } else if (isoDirs.size() > 1000 && isoDirs.size() > maxThreads) {
        batchSize = 25;
    } else if (isoDirs.size() > 100 && isoDirs.size() > maxThreads) {
        batchSize = 10;
    } else if (isoDirs.size() > 50 && isoDirs.size() > maxThreads) {
        batchSize = 5;
    } else if (isoDirs.size() > maxThreads) {
        batchSize = 2;
    }

    // Use std::async to unmount and remove the directories asynchronously
    auto unmountFuture = std::async(std::launch::async, [&isoDirs, batchSize]() {
        // Unmount directories in batches
        for (size_t i = 0; i < isoDirs.size(); i += batchSize) {
            std::string unmountBatchCommand = "umount -l";
            size_t batchEnd = std::min(i + batchSize, isoDirs.size());

            for (size_t j = i; j < batchEnd; ++j) {
                unmountBatchCommand += " " + shell_escape(isoDirs[j]) + " 2>/dev/null";
            }

            std::future<int> unmountFuture = std::async(std::launch::async, [&unmountBatchCommand]() {
                std::array<char, 128> buffer;
                std::string result;
                auto pclose_deleter = [](FILE* fp) { return pclose(fp); };
                std::unique_ptr<FILE, decltype(pclose_deleter)> pipe(popen(unmountBatchCommand.c_str(), "r"), pclose_deleter);
                if (!pipe) {
                    throw std::runtime_error("popen() failed!");
                }
                while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                    result += buffer.data();
                }
                return 0; // Return value doesn't matter for umount
            });

            unmountFuture.get();
        }

        // Check and remove empty directories
        std::vector<std::string> emptyDirs;
        for (const auto& isoDir : isoDirs) {
            if (isDirectoryEmpty(isoDir)) {
                emptyDirs.push_back(isoDir);
            } else {
                // Handle non-empty directory error
                std::stringstream errorMessage;
                errorMessage << "\033[1;91mFailed to unmount: \033[1;93m'" << isoDir << "'\033[1;91m.\033[0;1m";

                if (std::find(unmountedErrors.begin(), unmountedErrors.end(), errorMessage.str()) == unmountedErrors.end()) {
                    unmountedErrors.push_back(errorMessage.str());
                }
            }
        }

        // Remove empty directories in batches
        while (!emptyDirs.empty()) {
            std::string removeDirCommand = "rmdir ";
            for (size_t i = 0; i < std::min(batchSize, emptyDirs.size()); ++i) {
                removeDirCommand += shell_escape(emptyDirs[i]) + " ";
            }

            std::future<int> removeDirFuture = std::async(std::launch::async, [&removeDirCommand]() {
                std::array<char, 128> buffer;
                std::string result;
                auto pclose_deleter = [](FILE* fp) { return pclose(fp); };
                std::unique_ptr<FILE, decltype(pclose_deleter)> pipe(popen(removeDirCommand.c_str(), "r"), pclose_deleter);
                if (!pipe) {
                    throw std::runtime_error("popen() failed!");
                }
                while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                    result += buffer.data();
                }
                return pipe.get() ? 0 : -1;
            });

            int removeDirResult = removeDirFuture.get();

            for (size_t i = 0; i < std::min(batchSize, emptyDirs.size()); ++i) {
                if (removeDirResult == 0) {
                    auto [isoDirectory, isoFilename] = extractDirectoryAndFilename(emptyDirs[i]);
                    std::string unmountedFileInfo = "\033[1mUnmounted: \033[1;92m'" + isoDirectory + "/" + isoFilename + "'\033[0;1m.";
                    unmountedFiles.push_back(unmountedFileInfo);
                } else {
                    auto [isoDirectory, isoFilename] = extractDirectoryAndFilename(emptyDirs[i]);
                    std::stringstream errorMessage;
                    errorMessage << "\033[1;91mFailed to remove directory: \033[1;93m'" << isoDirectory << "/" << isoFilename << "'\033[1;91m ...Please check it out manually.\033[0;1m";

                    if (std::find(unmountedErrors.begin(), unmountedErrors.end(), errorMessage.str()) == unmountedErrors.end()) {
                        unmountedErrors.push_back(errorMessage.str());
                    }
                }
            }
            emptyDirs.erase(emptyDirs.begin(), emptyDirs.begin() + std::min(batchSize, emptyDirs.size()));
        }
    });

    unmountFuture.get();
}


// Function to print unmounted ISOs and errors
void printUnmountedAndErrors(bool invalidInput) {
	clearScrollBuffer();
	
	if (!unmountedFiles.empty()) {
		std::cout << " " << std::endl; // Print a blank line before unmounted files
	}
	
    // Print unmounted files
    for (const auto& unmountedFile : unmountedFiles) {
        std::cout << unmountedFile << std::endl;
    }

    if (!unmountedErrors.empty()) {
        std::cout << " " << std::endl; // Print a blank line before deleted folders
    }
    // Print unmounted errors
    for (const auto& unmountedError : unmountedErrors) {
        std::cout << unmountedError << std::endl;
    }

    // Clear vectors
    unmountedFiles.clear();
    unmountedErrors.clear();
    
    if (invalidInput) {
		std::cout << " " << std::endl;
	}

    // Print unique error messages
    if (!uniqueErrorMessages.empty()) {
        for (const std::string& uniqueErrorMessage : uniqueErrorMessages) {
            std::cerr << "\033[1;91m" << uniqueErrorMessage << "\033[0;1m" <<std::endl;
        }
        uniqueErrorMessages.clear();
    }
}


// Function to parse user input for selecting ISOs to unmount
std::vector<std::string> parseUserInput(const std::string& input, const std::vector<std::string>& isoDirs, bool& invalidInput, bool& noValid, bool& isFiltered) {
    // Vector to store selected ISO directories
    std::vector<std::string> selectedIsoDirs;

    // Vector to store selected indices
    std::vector<size_t> selectedIndices;

    // Set to keep track of processed indices
    std::unordered_set<size_t> processedIndices;

    // Create a stringstream to tokenize the input
    std::istringstream iss(input);

    // ThreadPool and mutexes for synchronization
    ThreadPool pool(maxThreads);
    std::vector<std::future<void>> futures;
    std::mutex indicesMutex;
    std::mutex processedMutex;
    std::mutex dirsMutex;

    // Lambda function to parse each token
    auto parseToken = [&](const std::string& token) {
        try {
            size_t dashPos = token.find('-');
			if (dashPos != std::string::npos) {
				// Token contains a range (e.g., "1-5")
				size_t start = std::stoi(token.substr(0, dashPos)) - 1;
				size_t end = std::stoi(token.substr(dashPos + 1)) - 1;

				// Process the range
				if (start < isoDirs.size() && end < isoDirs.size()) {
					if (start < end) {
						for (size_t i = start; i <= end; ++i) {
							std::lock_guard<std::mutex> lock(processedMutex);
							if (processedIndices.find(i) == processedIndices.end()) {
								std::lock_guard<std::mutex> lock(indicesMutex);
								selectedIndices.push_back(i);
								processedIndices.insert(i);
							}
						}
					} else if (start > end) { // Changed condition to start > end
						for (size_t i = start; i >= end; --i) {
							std::lock_guard<std::mutex> lock(processedMutex);
							if (processedIndices.find(i) == processedIndices.end()) {
								std::lock_guard<std::mutex> lock(indicesMutex);
								selectedIndices.push_back(i);
								processedIndices.insert(i);
								if (i == end) break;
							}
						}
					} else { // Added else branch for equal start and end
						uniqueErrorMessages.insert("\033[1;91mInvalid range: '" + std::to_string(start + 1) + "-" + std::to_string(end + 1) + "'. Ensure that numbers align with the list.\033[0;1m");
						invalidInput = true;
					}
				} else {
					uniqueErrorMessages.insert("\033[1;91mInvalid range: '" + std::to_string(start + 1) + "-" + std::to_string(end + 1) + "'. Ensure that numbers align with the list.\033[0;1m");
					invalidInput = true;
				}
						
            } else {
                // Token is a single index
                size_t index = std::stoi(token) - 1;
                
                // Process single index
                if (index < isoDirs.size()) {
                    std::lock_guard<std::mutex> lock(processedMutex);
                    if (processedIndices.find(index) == processedIndices.end()) {
                        std::lock_guard<std::mutex> lock(indicesMutex);
                        selectedIndices.push_back(index);
                        processedIndices.insert(index);
                    }
                } else {
                    // Single index is out of bounds
                    uniqueErrorMessages.insert("\033[1;91mFile index '" + std::to_string(index) + "' does not exist.\033[0;1m");
                    invalidInput = true;
                }
            }
        } catch (const std::invalid_argument&) {
            // Token is not a valid integer
            uniqueErrorMessages.insert("\033[1;91mInvalid input: '" + token + "'.\033[0;1m");
            invalidInput = true;
        }
    };

    // Tokenize input and enqueue tasks to thread pool
    for (std::string token; iss >> token;) {
        futures.emplace_back(pool.enqueue(parseToken, token));
    }

    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.wait();
    }

    // Check if any directories were selected
    if (!selectedIndices.empty()) {
        // Lock the mutex and retrieve selected directories
        std::lock_guard<std::mutex> lock(dirsMutex);
        for (size_t index : selectedIndices) {
            selectedIsoDirs.push_back(isoDirs[index]);
        }
    } else {
        // No valid directories were selected
        if (noValid && !isFiltered) {
            // Clear the console buffer and display an error message
            clearScrollBuffer();
            std::cerr << "\n\033[1;91mNo valid input provided for umount.\n";
            std::cout << "\n\033[1;32m↵ to continue...";
            std::cin.get();
        }
        noValid = true;
    }

    return selectedIsoDirs;
}


// Function to filter mounted isoDirs
void filterIsoDirs(const std::vector<std::string>& isoDirs, const std::vector<std::string>& filterPatterns, std::vector<std::string>& filteredIsoDirs, std::mutex& resultMutex, size_t start, size_t end) {
    // Iterate through the chunk of ISO directories
    for (size_t i = start; i < end; ++i) {
        const std::string& dir = isoDirs[i];
        std::string dirLower = dir;
        std::transform(dirLower.begin(), dirLower.end(), dirLower.begin(), ::tolower);

        // Flag to track if a match is found for the directory
        bool matchFound = false;
        // Iterate through each filter pattern
        for (const std::string& pattern : filterPatterns) {
            // If the directory matches the current filter pattern
            if (dirLower.find(pattern) != std::string::npos) {
                matchFound = true;
                break;
            }
        }

        // If a match is found, add the directory to the filtered list
        if (matchFound) {
            // Lock access to the shared vector
            std::lock_guard<std::mutex> lock(resultMutex);
            filteredIsoDirs.push_back(dir);
        }
    }
}


// Main function for unmounting ISOs
void unmountISOs() {
    // Initialize necessary variables
    std::vector<std::string> isoDirs;
    std::mutex isoDirsMutex;
    const std::string isoPath = "/mnt";
    bool invalidInput = false, skipEnter = false, isFiltered = false, noValid = true;

    while (true) {
        // Initialize variables for each loop iteration
        std::vector<std::string> filteredIsoDirs, selectedIsoDirs, selectedIsoDirsFiltered;
        clearScrollBuffer();
        listMountedISOs();
        isoDirs.clear();
        uniqueErrorMessages.clear();
        invalidInput = false;

        {
            // Lock the mutex to protect isoDirs from concurrent access
            std::lock_guard<std::mutex> lock(isoDirsMutex);

            // Populate isoDirs with directories that match the "iso_" prefix
            for (const auto& entry : std::filesystem::directory_iterator(isoPath)) {
                if (entry.is_directory() && entry.path().filename().string().find("iso_") == 0) {
                    isoDirs.push_back(entry.path().string());
                }
            }

            // Sort ISO directory names alphabetically in a case-insensitive manner
            std::sort(isoDirs.begin(), isoDirs.end(), [](const std::string& a, const std::string& b) {
                // Convert both strings to lowercase before comparison
                std::string lowerA = a;
                std::transform(lowerA.begin(), lowerA.end(), lowerA.begin(), [](unsigned char c) { return std::tolower(c); });

                std::string lowerB = b;
                std::transform(lowerB.begin(), lowerB.end(), lowerB.begin(), [](unsigned char c) { return std::tolower(c); });

                return lowerA < lowerB;
            });
        }

        // Check if there are no matching directories
        if (isoDirs.empty()) {
            std::cerr << "\033[1;93mNo path(s) matching the '/mnt/iso_*' pattern found.\033[0;1m" << std::endl;
            std::cout << "\n\033[1;32m↵ to continue...";
            std::cin.get();
            return;
        }

        // Prompt the user for input
        char* input = readline("\n\001\033[1;92m\002ISO(s)\001\033[1;94m\002 ↵ for \001\033[1;93m\002umount\001\033[1;94m\002 (e.g., '1-3', '1 5', '00' for all), / ↵ to filter\001\033[1;94m\002 , or ↵ to return:\001\033[0;1m\002 ");
        clearScrollBuffer();


        if (input[0] != '/' || (!(std::isspace(input[0]) || input[0] == '\0'))) {
            std::cout << "Please wait...\n";
        }

        // Check if the user wants to return to the main menu
        if (std::isspace(input[0]) || input[0] == '\0') {
            free(input);
            break;
        }

        // Check if the user wants to filter the list of ISOs
        if (strcmp(input, "/") == 0) {
            bool breakOuterLoop = false;
            while (true) {
                if (breakOuterLoop) {
                    historyPattern = false;
                    break;
                }
                clearScrollBuffer();
                isFiltered = true;
                historyPattern = true;
                loadHistory();
                std::string prompt;
                prompt = "\n\001\033[1;92m\002SearchQuery\001\033[1;94m\002 ↵ to filter \001\033[1;93m\002umount\001\033[1;94m\002 list (case-insensitive, multi-term separator: \001\033[1;93m\002;\001\033[1;94m\002), or ↵ to return: \001\033[0;1m\002";

                char* filterPattern = readline(prompt.c_str());
                clearScrollBuffer();

                if (filterPattern && filterPattern[0] != '\0') {
                    std::cout << "\\033\[1mPlease wait...\\033\[1m" << std::endl;
                    add_history(filterPattern); // Add the search query to the history
                    saveHistory();
                }

                clear_history();

                if (std::isspace(filterPattern[0]) || filterPattern[0] == '\0') {
                    free(filterPattern);
                    skipEnter = false;
                    isFiltered = false;
                    noValid = false;
                    historyPattern = false;
                    break;
                }

                // Split the filterPattern string into tokens using the delimiter ';'
                std::vector<std::string> filterPatterns;
                std::stringstream ss(filterPattern);
                std::string token;
                while (std::getline(ss, token, ';')) {
                    filterPatterns.push_back(token);
                    std::transform(filterPatterns.back().begin(), filterPatterns.back().end(), filterPatterns.back().begin(), ::tolower);
                }
                free(filterPattern);

                // Filter the list of ISO directories based on the filter pattern
                filteredIsoDirs.clear();

                // Create a thread pool with the desired number of threads
                ThreadPool pool(maxThreads);

                // Vector to store futures for tracking tasks' completion
                std::vector<std::future<void>> futures;

                // Calculate the number of directories per thread
                size_t numDirs = isoDirs.size();
				unsigned int numThreads = std::min(static_cast<unsigned int>(numDirs), maxThreads);                
				size_t dirsPerThread = numDirs / numThreads;
                // Enqueue filter tasks into the thread pool
                for (size_t i = 0; i < numThreads; ++i) {
                    size_t start = i * dirsPerThread;
                    size_t end = (i == numThreads - 1) ? numDirs : start + dirsPerThread;
                    futures.push_back(pool.enqueue(filterIsoDirs, std::ref(isoDirs), std::ref(filterPatterns), std::ref(filteredIsoDirs), std::ref(isoDirsMutex), start, end));
                }

                // Wait for all filter tasks to complete
                for (auto& future : futures) {
                    future.wait();
                }

                // Check if any directories matched the filter
                if (filteredIsoDirs.empty()) {
                    clearScrollBuffer();
                    std::cout << "\n\033[1;91mNo ISO mountpoint(s) match the filter pattern.\033[0;1m" << std::endl;
                    std::cout << "\n\033[1;32m↵ to continue...";
                    std::cin.get();
                    clearScrollBuffer();
                } else {
                    // Display the filtered list and prompt the user for input
                    while (true) {
                        // Display filtered results
                        clearScrollBuffer();
                        std::cout << "\033[1mFiltered results:\n\033[0;1m" << std::endl;
                        size_t maxIndex = filteredIsoDirs.size();
                        size_t numDigits = std::to_string(maxIndex).length();

                        for (size_t i = 0; i < filteredIsoDirs.size(); ++i) {
                            std::string afterSlash = filteredIsoDirs[i].substr(filteredIsoDirs[i].find_last_of("/") + 1);
                            std::string afterUnderscore = afterSlash.substr(afterSlash.find("_") + 1);
                            std::string color = (i % 2 == 0) ? "\033[1;31m" : "\033[1;32m"; // Red if even, Green if odd

                            std::cout << color << "\033[1m"
                                      << std::setw(numDigits) << std::setfill(' ') << (i + 1) << ".\033[0;1m /mnt/iso_"
                                      << "\033[1;95m" << afterUnderscore << "\033[0;1m"
                                      << std::endl;
                        }

                        // Prompt the user for the list of ISOs to unmount
                        char* chosenNumbers = readline("\n\001\033[1;92m\002ISO(s)\001\033[1;94m\002 ↵ for \001\033[1;93m\002umount\001\033[1;94m\002 (e.g., '1-3', '1 5', '00' for all), or ↵ to return:\001\033[0;1m\002 ");

                        if (std::isspace(chosenNumbers[0]) || chosenNumbers[0] == '\0') {
                            free(chosenNumbers);
                            noValid = false;
                            skipEnter = true;
                            historyPattern = false;
                            break;
                        }

                        if (std::strcmp(chosenNumbers, "00") == 0) {
                            free(chosenNumbers);
                            clearScrollBuffer();
                            std::cout << "\033[1mPlease wait...\033[1m" << std::endl;
                            selectedIsoDirs = filteredIsoDirs;
                            skipEnter = false;
                            isFiltered = true;
                            breakOuterLoop = true;
                            historyPattern = false;
                            break;
                        }

                        // Parse the user input to determine which ISOs to unmount
                        selectedIsoDirsFiltered = parseUserInput(chosenNumbers, filteredIsoDirs, invalidInput, noValid, isFiltered);

                        if (!selectedIsoDirsFiltered.empty()) {
                            selectedIsoDirs = selectedIsoDirsFiltered;
                            skipEnter = false;
                            isFiltered = true;
                            historyPattern = false;
                            break; // Exit filter loop to process unmount
                        } else {
                            clearScrollBuffer();
                            invalidInput = false;
                            uniqueErrorMessages.clear();
                            std::cerr << "\n\033[1;91mNo valid input provided for umount.\n";
                            std::cout << "\n\033[1;32m↵ to continue...";
                            std::cin.get();
                        }
                    }

                    if (!selectedIsoDirsFiltered.empty() && isFiltered) {
                        clearScrollBuffer();
                        std::cout << "\033[1mPlease wait...\033[1m" << std::endl;
                        isFiltered = true;
                        historyPattern = false;
                        break;
                    }
                }
            }
        }

        // Check if the user wants to unmount all ISOs
        if (std::strcmp(input, "00") == 0) {
            free(input);
            selectedIsoDirs = isoDirs;
        } else if (!isFiltered) {
            selectedIsoDirs = parseUserInput(input, isoDirs, invalidInput, noValid, isFiltered);
        }

        // If there are selected ISOs, proceed to unmount them
        if (!selectedIsoDirs.empty()) {
            std::vector<std::future<void>> futures;
            unsigned int numThreads = std::min(static_cast<int>(selectedIsoDirs.size()), static_cast<int>(maxThreads));
            ThreadPool pool(numThreads);
            std::lock_guard<std::mutex> isoDirsLock(isoDirsMutex);

            // Divide the selected ISOs into batches for parallel processing
            size_t batchSize = (selectedIsoDirs.size() + maxThreads - 1) / maxThreads;
            std::vector<std::vector<std::string>> batches;
            for (size_t i = 0; i < selectedIsoDirs.size(); i += batchSize) {
                batches.emplace_back(selectedIsoDirs.begin() + i, std::min(selectedIsoDirs.begin() + i + batchSize, selectedIsoDirs.end()));
            }

            // Enqueue unmount tasks for each batch of ISOs
            for (const auto& batch : batches) {
                futures.emplace_back(pool.enqueue([batch]() {
                    std::lock_guard<std::mutex> highLock(Mutex4High);
                    unmountISO(batch);
                }));
            }

            // Wait for all unmount tasks to complete
            for (auto& future : futures) {
                future.wait();
            }

            printUnmountedAndErrors(invalidInput);

            // Prompt the user to press Enter to continue
            if (!skipEnter) {
                std::cout << "\n\033[1;32m↵ to continue...";
                std::cin.get();
            }
            clearScrollBuffer();
            skipEnter = false;
            isFiltered = false;
        }
    }
}

