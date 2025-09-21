#include "OffsetScanner.h"
#include "ProcessMemoryReader.h"
#include "ProcessUtils.h"
#include "Vector3.h"

#include <array>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Options {
    std::string processName;
    std::string moduleName;
    Vector3 primary{};
    Vector3 secondary{};
    bool hasPrimary{false};
    bool hasSecondary{false};
    float tolerance{0.01f};
    size_t maxResults{64};
};

void printUsage(const char *programName) {
    std::cout << "Usage: " << programName << " --process <name> --module <module> --primary <x,y,z> [options]\n"
              << "Options:\n"
              << "  --process <name>       Target process name as listed in /proc/<pid>/comm\n"
              << "  --module <module>     Module or binary name to constrain the scan\n"
              << "  --primary <x,y,z>     Known position sample (floats)\n"
              << "  --secondary <x,y,z>   Optional second sample for validation\n"
              << "  --tolerance <value>   Comparison tolerance (default 0.01)\n"
              << "  --max-results <n>     Maximum number of candidates to display (default 64)\n"
              << std::endl;
}

bool parseVectorArgument(const std::string &argument, Vector3 &out, std::string &error) {
    try {
        out = Vector3::parse(argument);
        return true;
    } catch (const std::exception &ex) {
        error = ex.what();
        return false;
    }
}

bool parseOptions(int argc, char **argv, Options &options, std::string &error) {
    if (argc < 2) {
        error = "not enough arguments";
        return false;
    }
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--process") {
            if (i + 1 >= argc) {
                error = "--process requires a value";
                return false;
            }
            options.processName = argv[++i];
        } else if (arg == "--module") {
            if (i + 1 >= argc) {
                error = "--module requires a value";
                return false;
            }
            options.moduleName = argv[++i];
        } else if (arg == "--primary") {
            if (i + 1 >= argc) {
                error = "--primary requires a value";
                return false;
            }
            if (!parseVectorArgument(argv[++i], options.primary, error)) {
                return false;
            }
            options.hasPrimary = true;
        } else if (arg == "--secondary") {
            if (i + 1 >= argc) {
                error = "--secondary requires a value";
                return false;
            }
            if (!parseVectorArgument(argv[++i], options.secondary, error)) {
                return false;
            }
            options.hasSecondary = true;
        } else if (arg == "--tolerance") {
            if (i + 1 >= argc) {
                error = "--tolerance requires a value";
                return false;
            }
            options.tolerance = std::strtof(argv[++i], nullptr);
        } else if (arg == "--max-results") {
            if (i + 1 >= argc) {
                error = "--max-results requires a value";
                return false;
            }
            options.maxResults = static_cast<size_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--help" || arg == "-h") {
            return false;
        } else {
            error = "unknown argument: " + arg;
            return false;
        }
    }
    if (options.processName.empty()) {
        error = "missing --process";
        return false;
    }
    if (options.moduleName.empty()) {
        error = "missing --module";
        return false;
    }
    if (!options.hasPrimary) {
        error = "missing --primary";
        return false;
    }
    return true;
}

std::string formatAddress(uintptr_t address) {
    std::ostringstream oss;
    oss << "0x" << std::hex << address;
    return oss.str();
}

void printCandidate(const CandidateOffset &candidate) {
    std::cout << "  address: " << formatAddress(candidate.address)
              << " | module offset: 0x" << std::hex << candidate.offsetFromModule
              << std::dec << " | value: " << candidate.value.toString(5)
              << std::endl;
}

} // namespace

int main(int argc, char **argv) {
    Options options;
    std::string error;
    if (!parseOptions(argc, argv, options, error)) {
        if (!error.empty()) {
            std::cerr << "Error: " << error << "\n";
        }
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    auto processInfo = ProcessUtils::findProcessByName(options.processName);
    if (!processInfo) {
        std::cerr << "Process '" << options.processName << "' not found.\n";
        return EXIT_FAILURE;
    }

    const auto moduleRegions = ProcessUtils::findModuleRegions(processInfo->pid, options.moduleName);
    if (moduleRegions.empty()) {
        std::cerr << "Module '" << options.moduleName << "' not found in process.\n";
        return EXIT_FAILURE;
    }

    ProcessMemoryReader reader(processInfo->pid);
    if (!reader.isValid()) {
        std::cerr << "Failed to open target process memory. Root privileges may be required.\n";
        return EXIT_FAILURE;
    }

    OffsetScanner scanner(processInfo->pid, moduleRegions, std::move(reader));

    std::cout << "Scanning process '" << processInfo->name << "' (pid " << processInfo->pid << ")\n";
    std::cout << "Module regions:" << std::endl;
    for (const auto &region : moduleRegions) {
        std::cout << "  " << ProcessUtils::describeMemoryRegion(region) << std::endl;
    }

    std::cout << "Searching for primary position " << options.primary.toString(5)
              << " with tolerance " << options.tolerance << std::endl;
    auto candidates = scanner.findCandidates(options.primary, options.tolerance, options.maxResults);
    if (candidates.empty()) {
        std::cout << "No matching candidates found in module." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Found " << candidates.size() << " candidate addresses." << std::endl;
    if (options.hasSecondary) {
        std::cout << "Validating using secondary sample " << options.secondary.toString(5) << std::endl;
        candidates = scanner.verifyCandidates(candidates, options.secondary, options.tolerance);
        if (candidates.empty()) {
            std::cout << "No candidates survived the secondary validation." << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "Remaining candidates after validation: " << candidates.size() << std::endl;
    } else {
        std::cout << "Provide --secondary to validate results after moving in-game." << std::endl;
    }

    std::cout << "Candidate offsets:" << std::endl;
    for (const auto &candidate : candidates) {
        printCandidate(candidate);
    }

    std::cout << "Use the module offset to access the position vector relative to the module base." << std::endl;
    return EXIT_SUCCESS;
}
