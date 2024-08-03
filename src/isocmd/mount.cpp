// SPDX-License-Identifier: GNU General Public License v3.0 or later

#include "../headers.h"
#include "../threadpool.h"


// For storing isoFiles in RAM
std::vector<std::string> globalIsoFileList;

//	MOUNT STUFF

// Function to mount all ISOs indiscriminately
void mountAllIsoFiles(const std::vector<std::string>& isoFiles, std::set<std::string>& mountedFiles, std::set<std::string>& skippedMessages, std::set<std::string>& mountedFails) {
    size_t maxChunkSize = 50;  // Maximum number of files per chunk
    size_t totalIsos = isoFiles.size();
    std::atomic<size_t> completedIsos(0);
    std::atomic<bool> isComplete(false);
    // Determine the number of threads to use
    size_t numThreads = std::min(totalIsos, static_cast<size_t>(maxThreads));
    // Adjust chunk size based on the number of files
    size_t chunkSize = std::min(maxChunkSize, (totalIsos + numThreads - 1) / numThreads);
    // Ensure chunkSize is at least 1
    chunkSize = std::max(chunkSize, static_cast<size_t>(1));
    ThreadPool pool(numThreads);
    std::thread progressThread(displayProgressBar, std::ref(completedIsos), totalIsos, std::ref(isComplete));
    std::vector<std::future<void>> futures;
    futures.reserve((totalIsos + chunkSize - 1) / chunkSize);  // Reserve enough space for all future tasks

    for (size_t i = 0; i < totalIsos; i += chunkSize) {
        size_t end = std::min(i + chunkSize, totalIsos);
        futures.emplace_back(pool.enqueue([&, i, end]() {
            // Optimization: Replace loop with efficient vector construction
            std::vector<std::string> chunkFiles(isoFiles.begin() + i, isoFiles.begin() + end);
            mountIsoFiles(chunkFiles, mountedFiles, skippedMessages, mountedFails);
            completedIsos.fetch_add(chunkFiles.size(), std::memory_order_relaxed);
        }));
    }

    for (auto& future : futures) {
        future.wait();
    }

    isComplete.store(true, std::memory_order_release);
    progressThread.join();
}


// Function to select and mount ISO files by number
void select_and_mount_files_by_number() {
    std::set<std::string> mountedFiles, skippedMessages, mountedFails, uniqueErrorMessages;
    std::vector<std::string> isoFiles, filteredFiles;
    isoFiles.reserve(100);
    bool isFiltered = false;
    bool needsClrScrn = true;

    while (true) {
        removeNonExistentPathsFromCache();
        loadCache(isoFiles);

        if (needsClrScrn) clearScrollBuffer();

        if (isoFiles.empty()) {
            clearScrollBuffer();
            std::cout << "\n\033[1;93mISO Cache is empty. Choose 'ImportISO' from the Main Menu Options.\033[0;1m\n \n\033[1;32m↵ to continue...\033[0;1m";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }

        // Check if the loaded cache differs from the global list
        if (globalIsoFileList.size() != isoFiles.size() ||
            !std::equal(globalIsoFileList.begin(), globalIsoFileList.end(), isoFiles.begin())) {
            sortFilesCaseInsensitive(isoFiles);
            globalIsoFileList = isoFiles;
        } else {
            isoFiles = globalIsoFileList;
        }

        mountedFiles.clear();
        skippedMessages.clear();
        mountedFails.clear();
        uniqueErrorMessages.clear();

        if (needsClrScrn) {
			printIsoFileList(isFiltered ? filteredFiles : globalIsoFileList);
			std::cout << "\n\n\n";
		}
		// Move the cursor up 3 lines and clear them
        std::cout << "\033[1A\033[K";

        std::string prompt;
		if (isFiltered) {
			prompt = "\001\033[1;96m\002Filtered \001\033[1;92m\002ISO\001\033[1;94m\002 ↵ for \001\033[1;92m\002mount\001\033[1;94m\002 (e.g., 1-3,1 5,00=all), / ↵ filter, ↵ return:\001\033[0;1m\002 ";
		} else {
			prompt = "\001\033[1;92m\002ISO\001\033[1;94m\002 ↵ for \001\033[1;92m\002mount\001\033[1;94m\002 (e.g., 1-3,1 5,00=all), / ↵ filter, ↵ return:\001\033[0;1m\002 ";
		}
        std::unique_ptr<char[], decltype(&std::free)> input(readline(prompt.c_str()), &std::free);
        std::string inputString(input.get());

        if (inputString.empty()) {
            if (isFiltered) {
                isFiltered = false;
                continue;  // Return to the original list
            } else {
                return;  // Exit the function only if we're already on the original list
            }
        } else if (inputString == "/") {
            while (true) {
                mountedFiles.clear();
                skippedMessages.clear();
                mountedFails.clear();
                uniqueErrorMessages.clear();

                clear_history();
                historyPattern = true;
                loadHistory();
                // Move the cursor up 3 lines and clear them
                std::cout << "\033[1A\033[K";

                std::string filterPrompt = "\001\033[38;5;94m\002FilterTerms\001\033[1;94m\002 ↵ for \001\033[1;92m\002mount\001\033[1;94m\002 list (multi-term separator: \001\033[1;93m\002;\001\033[1;94m\002), ↵ return: \001\033[0;1m\002";

                std::unique_ptr<char, decltype(&std::free)> searchQuery(readline(filterPrompt.c_str()), &std::free);

                if (!searchQuery || searchQuery.get()[0] == '\0' || strcmp(searchQuery.get(), "/") == 0) {
                    historyPattern = false;
                    clear_history();
                    if (isFiltered) {
                        needsClrScrn = true;
                    } else {
                        needsClrScrn = false;
                    }
                    break;
                }

                std::string inputSearch(searchQuery.get());

                if (strcmp(searchQuery.get(), "/") != 0) {
                    add_history(searchQuery.get());
                    saveHistory();
                }
                historyPattern = false;
                clear_history();



                auto newFilteredFiles = filterFiles(globalIsoFileList, inputSearch);

                if (newFilteredFiles.size() == globalIsoFileList.size()) {
					isFiltered = false;
					break;
				}

                if (!newFilteredFiles.empty()) {
					needsClrScrn = true;
                    filteredFiles = std::move(newFilteredFiles);
                    isFiltered = true;
                    break;
                }

            }

        } else {
            std::vector<std::string>& currentFiles = isFiltered ? filteredFiles : globalIsoFileList;
            if (inputString == "00") {
				clearScrollBuffer();
				std::cout << "\033[1m\n";
				needsClrScrn = true;
                mountAllIsoFiles(currentFiles, mountedFiles, skippedMessages, mountedFails);
            } else {
				clearScrollBuffer();
				needsClrScrn = true;
				std::cout << "\033[1m\n";
                processAndMountIsoFiles(inputString, currentFiles, mountedFiles, skippedMessages, mountedFails, uniqueErrorMessages);
            }

            if (!uniqueErrorMessages.empty() && mountedFiles.empty() && skippedMessages.empty() && mountedFails.empty()) {
				clearScrollBuffer();
				needsClrScrn = true;
                std::cout << "\n\033[1;91mNo valid input provided for mount\033[0;1m\n\n\033[1;32m↵ to continue...\033[0;1m";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            } else if (verbose) {
				clearScrollBuffer();
				needsClrScrn = true;
                printMountedAndErrors(mountedFiles, skippedMessages, mountedFails, uniqueErrorMessages);
            }
        }
    }
}


// Function to print mount verbose messages
void printMountedAndErrors( std::set<std::string>& mountedFiles, std::set<std::string>& skippedMessages, std::set<std::string>& mountedFails, std::set<std::string>& uniqueErrorMessages) {

    // Print all mounted files
    for (const auto& mountedFile : mountedFiles) {
        std::cout << "\n" << mountedFile << "\033[0;1m";
    }

    if (!mountedFiles.empty()) {
        std::cout << "\n";
    }

    // Print all the stored skipped messages
    for (const auto& skippedMessage : skippedMessages) {
        std::cerr << "\n" << skippedMessage << "\033[0;1m";
    }

    if (!skippedMessages.empty()) {
        std::cout << "\n";
    }

    // Print all the stored error messages
    for (const auto& mountedFail : mountedFails) {
        std::cerr << "\n" << mountedFail << "\033[0;1m";
    }

    if (!mountedFails.empty()) {
        std::cout << "\n";
    }

    // Print all the stored error messages
    for (const auto& errorMessage : uniqueErrorMessages) {
        std::cerr << "\n" << errorMessage << "\033[0;1m";
    }

    if (!uniqueErrorMessages.empty()) {
        std::cout << "\n";
    }

    // Clear the vectors after each iteration
    mountedFiles.clear();
    skippedMessages.clear();
    mountedFails.clear();
    uniqueErrorMessages.clear();

	std::cout << "\n\033[1;32m↵ to continue...\033[0;1m";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}


// Function to check if a mountpoint isAlreadyMounted
bool isAlreadyMounted(const std::string& mountPoint) {
    struct libmnt_table *tb = mnt_new_table();
    struct libmnt_cache *cache = mnt_new_cache();

    if (!tb || !cache) {
        if (tb) mnt_free_table(tb);
        if (cache) mnt_free_cache(cache);
        throw std::runtime_error("Failed to allocate mnt_table or mnt_cache");
    }

    mnt_table_set_cache(tb, cache);
    int ret = mnt_table_parse_mtab(tb, NULL);
    if (ret != 0) {
        mnt_free_table(tb);
        mnt_free_cache(cache);
        throw std::runtime_error("Error parsing mtab");
    }

    struct libmnt_fs *fs = mnt_table_find_target(tb, mountPoint.c_str(), MNT_ITER_BACKWARD);

    bool isMounted = (fs != NULL);

    mnt_free_table(tb);
    mnt_free_cache(cache);

    return isMounted;
}


// Function to mount selected ISO files called from processAndMountIsoFiles
void mountIsoFiles(const std::vector<std::string>& isoFiles, std::set<std::string>& mountedFiles, std::set<std::string>& skippedMessages, std::set<std::string>& mountedFails) {
    for (const auto& isoFile : isoFiles) {
        namespace fs = std::filesystem;

        fs::path isoPath(isoFile);
        std::string isoFileName = isoPath.stem().string();

        std::hash<std::string> hasher;
        size_t hashValue = hasher(isoFile);

        const std::string base36Chars = "0123456789abcdefghijklmnopqrstuvwxyz";
        std::string shortHash;
        for (int i = 0; i < 5; ++i) {
            shortHash += base36Chars[hashValue % 36];
            hashValue /= 36;
        }

        std::string uniqueId = isoFileName + "\033[38;5;245m~" + shortHash + "\033[0;1m";
        std::string mountPoint = "/mnt/iso_" + uniqueId;

        auto [isoDirectory, isoFilename] = extractDirectoryAndFilename(isoFile);
        auto [mountisoDirectory, mountisoFilename] = extractDirectoryAndFilename(mountPoint);

        if (geteuid() != 0) {
            std::stringstream errorMessage;
            errorMessage << "\033[1;91mFailed to mnt: \033[1;93m'" << isoDirectory << "/" << isoFilename
                         << "'\033[0m\033[1;91m. Root privileges are required.\033[0m";
            {
                std::lock_guard<std::mutex> lowLock(Mutex4Low);
                mountedFails.insert(errorMessage.str());
            }
            continue;
        }

        if (isAlreadyMounted(mountPoint)) {
            std::stringstream skippedMessage;
            skippedMessage << "\033[1;93mISO: \033[1;92m'" << isoDirectory << "/" << isoFilename
                           << "'\033[1;93m already mnt@: \033[1;94m'" << mountisoDirectory
                           << "/" << mountisoFilename << "\033[1;94m'\033[1;93m.\033[0m";
            {
                std::lock_guard<std::mutex> lowLock(Mutex4Low);
                skippedMessages.insert(skippedMessage.str());
            }
            continue;
        }

        if (!fs::exists(mountPoint)) {
            try {
                fs::create_directory(mountPoint);
            } catch (const fs::filesystem_error& e) {
                std::stringstream errorMessage;
                errorMessage << "\033[1;91mFailed to create mount point: \033[1;93m'" << mountPoint
                             << "'\033[0m\033[1;91m. Error: " << e.what() << "\033[0m";
                {
                    std::lock_guard<std::mutex> lowLock(Mutex4Low);
                    mountedFails.insert(errorMessage.str());
                }
                continue;
            }
        }

        // Create a libmount context
        struct libmnt_context *ctx = mnt_new_context();
        if (!ctx) {
            std::stringstream errorMessage;
            errorMessage << "\033[1;91mFailed to create mount context for: \033[1;93m'" << isoFile << "'\033[0m";
            mountedFails.insert(errorMessage.str());
            continue;
        }

        // Set common mount options
        mnt_context_set_source(ctx, isoFile.c_str());
        mnt_context_set_target(ctx, mountPoint.c_str());
        mnt_context_set_options(ctx, "loop,ro");

        // Set filesystem types to try
        std::string fsTypeList = "iso9660,udf,hfsplus,rockridge,joliet,isofs";
        mnt_context_set_fstype(ctx, fsTypeList.c_str());

        // Attempt to mount
        int ret = mnt_context_mount(ctx);

        bool mountSuccess = (ret == 0);
        std::string successfulFsType;

        if (mountSuccess) {
            // If successful, we can get the used filesystem type
            const char* usedFsType = mnt_context_get_fstype(ctx);
            if (usedFsType) {
                successfulFsType = usedFsType;
            }
        }

        // Clean up the context
        mnt_free_context(ctx);

        if (mountSuccess) {
            std::string mountedFileInfo = "\033[1mISO: \033[1;92m'" + isoDirectory + "/" + isoFilename + "'\033[0m"
                                        + "\033[1m mnt@: \033[1;94m'" + mountisoDirectory + "/" + mountisoFilename
                                        + "\033[1;94m'\033[0;1m.";
            if (successfulFsType != "auto") {
                mountedFileInfo += " {" + successfulFsType + "}";
            }
            mountedFileInfo += "\033[0m";
            {
                std::lock_guard<std::mutex> lowLock(Mutex4Low);
                mountedFiles.insert(mountedFileInfo);
            }
        } else {
            std::stringstream errorMessage;
            errorMessage << "\033[1;91mFailed to mnt: \033[1;93m'" << isoDirectory << "/" << isoFilename
                         << "'\033[1;91m.\033[0;1m {badFS}";
            fs::remove(mountPoint);
            {
                std::lock_guard<std::mutex> lowLock(Mutex4Low);
                mountedFails.insert(errorMessage.str());
            }
        }
    }
}


// Function to process input and mount ISO files asynchronously
void processAndMountIsoFiles(const std::string& input, const std::vector<std::string>& isoFiles, std::set<std::string>& mountedFiles, std::set<std::string>& skippedMessages, std::set<std::string>& mountedFails, std::set<std::string>& uniqueErrorMessages) {
    std::istringstream iss(input);
    std::vector<int> indicesToProcess;
    indicesToProcess.reserve(isoFiles.size());  // Reserve maximum possible size

    std::atomic<bool> invalidInput(false);
    std::set<std::string> processedRanges;
    std::mutex errorMutex;

    auto addError = [&](const std::string& error) {
        std::lock_guard<std::mutex> lock(errorMutex);
        uniqueErrorMessages.insert(error);
        invalidInput.store(true, std::memory_order_release);
    };

    std::string token;
    while (iss >> token) {
        if (token == "/") break;

        if (token[0] == '0') {
            addError("\033[1;91mInvalid index: '0'.\033[0;1m");
            continue;
        }

        size_t dashPos = token.find('-');
        if (dashPos != std::string::npos) {
            if (dashPos == 0 || dashPos == token.size() - 1 || 
                !std::all_of(token.begin(), token.begin() + dashPos, ::isdigit) ||
                !std::all_of(token.begin() + dashPos + 1, token.end(), ::isdigit)) {
                addError("\033[1;91mInvalid input: '" + token + "'.\033[0;1m");
                continue;
            }

            int start, end;
            try {
                start = std::stoi(token.substr(0, dashPos));
                end = std::stoi(token.substr(dashPos + 1));
            } catch (const std::exception&) {
                addError("\033[1;91mInvalid input: '" + token + "'.\033[0;1m");
                continue;
            }

            if (start < 1 || static_cast<size_t>(start) > isoFiles.size() ||
                end < 1 || static_cast<size_t>(end) > isoFiles.size()) {
                addError("\033[1;91mInvalid range: '" + token + "'.\033[0;1m");
                continue;
            }

            if (processedRanges.insert(token).second) {
                int step = (start <= end) ? 1 : -1;
                for (int i = start; (step > 0) ? (i <= end) : (i >= end); i += step) {
                    indicesToProcess.push_back(i);
                }
            }
        } else if (std::all_of(token.begin(), token.end(), ::isdigit)) {
            int num = std::stoi(token);
            if (num >= 1 && static_cast<size_t>(num) <= isoFiles.size()) {
                indicesToProcess.push_back(num);
            } else {
                addError("\033[1;91mInvalid index: '" + token + "'.\033[0;1m");
            }
        } else {
            addError("\033[1;91mInvalid input: '" + token + "'.\033[0;1m");
        }
    }

    if (indicesToProcess.empty()) {
        addError("\033[1;91mNo valid input provided for mount.\033[0;1m");
        return;
    }

    std::atomic<size_t> completedTasks(0);
    std::atomic<bool> isProcessingComplete(false);

    unsigned int numThreads = std::min(static_cast<unsigned int>(indicesToProcess.size()), static_cast<unsigned int>(maxThreads));
    ThreadPool pool(numThreads);

    size_t totalTasks = indicesToProcess.size();
    size_t chunkSize = std::max(size_t(1), std::min(size_t(50), (totalTasks + numThreads - 1) / numThreads));

    std::mutex filesMutex;
    std::atomic<size_t> activeTaskCount(0);
    std::condition_variable taskCompletionCV;
    std::mutex taskCompletionMutex;

    for (size_t i = 0; i < totalTasks; i += chunkSize) {
        size_t end = std::min(i + chunkSize, totalTasks);
        activeTaskCount.fetch_add(1, std::memory_order_relaxed);
        pool.enqueue([&, i, end]() {
            std::vector<std::string> filesToMount;
            filesToMount.reserve(end - i);
            for (size_t j = i; j < end; ++j) {
                filesToMount.push_back(isoFiles[indicesToProcess[j] - 1]);
            }
            
            std::set<std::string> localMounted, localSkipped, localFails;
            mountIsoFiles(filesToMount, localMounted, localSkipped, localFails);
            
            {
                std::lock_guard<std::mutex> lock(filesMutex);
                mountedFiles.insert(localMounted.begin(), localMounted.end());
                skippedMessages.insert(localSkipped.begin(), localSkipped.end());
                mountedFails.insert(localFails.begin(), localFails.end());
            }

            completedTasks.fetch_add(end - i, std::memory_order_relaxed);

            if (activeTaskCount.fetch_sub(1, std::memory_order_release) == 1) {
                taskCompletionCV.notify_one();
            }
        });
    }

    std::thread progressThread(displayProgressBar, std::ref(completedTasks), totalTasks, std::ref(isProcessingComplete));

    // Wait for all tasks to complete
    {
        std::unique_lock<std::mutex> lock(taskCompletionMutex);
        taskCompletionCV.wait(lock, [&]() { return activeTaskCount.load(std::memory_order_acquire) == 0; });
    }

    isProcessingComplete.store(true, std::memory_order_release);
    progressThread.join();
}
