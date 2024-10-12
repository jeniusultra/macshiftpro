#include <Windows.h>
#include <Iphlpapi.h>
#include <Netcon.h>
#include <Objbase.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "netcon.lib")
#pragma comment(lib, "ole32.lib")

// Function prototypes
static void submain(const std::vector<std::string> &argVec);
static void showHelp(const std::string &exePath);
static bool isValidMac(const std::string &str);
static std::string randomMac();
static std::string findAdapterId(const std::string &adapterName);
static void setMac(const std::string &adapterId, const std::string &newMac);
static void resetAdapter(const std::string &adapterName);
static bool isMacLoggedIn(const std::string &macAddress);

int main(int argc, char **argv) {
    std::cerr << "Macshift v2.0 - the simple Windows MAC address changing utility" << std::endl;

    std::vector<std::string> argVec;
    for (int i = 0; i < argc; i++)
        argVec.push_back(std::string(argv[i]));

    srand(static_cast<unsigned int>(GetTickCount64()));

    try {
        submain(argVec);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void submain(const std::vector<std::string> &argVec) {
    std::string adapterName = "";
    bool isMacModeSet = false;
    std::string newMac = randomMac();

    // Parse command-line arguments
    for (std::size_t i = 1; i < argVec.size(); i++) {
        const std::string &arg = argVec.at(i);
        if (arg.find("-") == 0) {  // A flag
            if (arg == "-h")
                showHelp(argVec.at(0));
            else if (arg == "-d" || arg == "-r" || arg == "-a") {
                if (isMacModeSet)
                    throw std::invalid_argument("Command-line arguments contain more than one MAC address mode");
                isMacModeSet = true;
                if (arg == "-d")
                    newMac = "";
                else if (arg == "-r")
                    ;  // Do nothing else because newMac is already random
                else if (arg == "-a") {
                    if (argVec.size() - i <= 1)
                        throw std::invalid_argument("Missing MAC address argument");
                    i++;
                    const std::string &val = argVec.at(i);
                    if (!isValidMac(val))
                        throw std::invalid_argument("Invalid MAC address, must match pattern /[0-9a-fA-F]{12}/");
                    newMac = val;
                } else
                    throw std::logic_error("Unreachable");
            } else
                throw std::invalid_argument("Unrecognized command-line flag");
        } else {  // Not a flag
            if (!adapterName.empty())
                throw std::invalid_argument("Command-line arguments contain more than one network adapter name");
            adapterName = arg;
        }
    }

    if (adapterName == "")
        showHelp(argVec.at(0));

    std::cerr << "New MAC address: ";
    if (newMac == "")
        std::cerr << "(restore)";
    else {
        for (std::size_t i = 0; i < 12; i += 2) {
            if (i > 0)
                std::cerr << "-";
            std::cerr << newMac.substr(i, 2);
        }
    }
    std::cerr << std::endl;

    std::string adapterId = findAdapterId(adapterName);
    std::cerr << "Network adapter ID: " << adapterId << std::endl;

    // Scan for MAC addresses on the network
    ULONG ulOutLen = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutLen);
    if (GetAdaptersInfo(pAdapterInfo, &ulOutLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutLen);
        if (GetAdaptersInfo(pAdapterInfo, &ulOutLen) == ERROR_SUCCESS) {
            PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
            while (pAdapter) {
                std::string macAddress = pAdapter->AdapterAddress;
                if (isMacLoggedIn(macAddress)) {
                    setMac
