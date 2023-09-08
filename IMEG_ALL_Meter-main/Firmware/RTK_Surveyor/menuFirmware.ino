// Update firmware if bin files found
void menuFirmware()
{
    bool newOTAFirmwareAvailable = false;
    char reportedVersion[50] = {'\0'};

    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: Update Firmware");

        if (btPrintEcho == true)
            systemPrintln("Firmware update not available while configuration over Bluetooth is active");

        char currentVersion[21];
        getFirmwareVersion(currentVersion, sizeof(currentVersion), enableRCFirmware);
        systemPrintf("Current firmware: %s\r\n", currentVersion);

        if (strlen(reportedVersion) > 0)
        {
            if (newOTAFirmwareAvailable == false)
                systemPrintf("c) Check SparkFun for device firmware: Up to date\r\n");
        }
        else
            systemPrintln("c) Check SparkFun for device firmware");

        systemPrintf("e) Allow Beta Firmware: %s\r\n", enableRCFirmware ? "Enabled" : "Disabled");

        if (newOTAFirmwareAvailable)
            systemPrintf("u) Update to new firmware: v%s\r\n", reportedVersion);

        for (int x = 0; x < binCount; x++)
            systemPrintf("%d) Load SD file: %s\r\n", x + 1, binFileNames[x]);

        systemPrintln("x) Exit");

        byte incoming = getCharacterNumber();

        if (incoming == 'c' && btPrintEcho == false)
        {
            bool previouslyConnected = wifiIsConnected();

            bluetoothStop(); // Stop Bluetooth to allow for SSL on the heap

            // Attempt to connect to local WiFi
            if (wifiConnect(10000) == true)
            {
                // Get firmware version from server
                if (otaCheckVersion(reportedVersion, sizeof(reportedVersion)))
                {
                    // We got a version number, now determine if it's newer or not
                    char currentVersion[21];
                    getFirmwareVersion(currentVersion, sizeof(currentVersion), enableRCFirmware);
                    if (isReportedVersionNewer(reportedVersion, &currentVersion[1]) == true)
                    {
                        log_d("New version detected");
                        newOTAFirmwareAvailable = true;
                    }
                    else
                    {
                        log_d("No new firmware available");
                    }
                }
                else
                {
                    // Failed to get version number
                    systemPrintln("Failed to get version number from server.");
                }
            }
            else if (incoming == 'c' && btPrintEcho == false)
            {
                bool previouslyConnected = wifiIsConnected();

                bluetoothStop(); // Stop Bluetooth to allow for SSL on the heap

                // Attempt to connect to local WiFi
                if (wifiConnect(10000) == true)
                {
                    // Get firmware version from server
                    if (otaCheckVersion(reportedVersion, sizeof(reportedVersion)))
                    {
                        // We got a version number, now determine if it's newer or not
                        char currentVersion[21];
                        getFirmwareVersion(currentVersion, sizeof(currentVersion), enableRCFirmware);
                        if (isReportedVersionNewer(reportedVersion, &currentVersion[1]) == true)
                        {
                            log_d("New version detected");
                            newOTAFirmwareAvailable = true;
                        }
                        else
                        {
                            log_d("No new firmware available");
                        }
                    }
                    else
                    {
                        // Failed to get version number
                        systemPrintln("Failed to get version number from server.");
                    }
                }
                else
                    systemPrintln("Firmware update failed to connect to WiFi.");

                if (previouslyConnected == false)
                    wifiStop();

                bluetoothStart(); // Restart BT according to settings
            }
        }
        else if (incoming == 'c' && btPrintEcho == true)
        {
            systemPrintln("Firmware update not available while configuration over Bluetooth is active");
            delay(2000);
        }
        else if (newOTAFirmwareAvailable && incoming == 'u')
        {
            bool previouslyConnected = wifiIsConnected();

            otaUpdate();

            // We get here if WiFi failed or the server was not available

            if (previouslyConnected == false)
                wifiStop();
        }

        else if (incoming == 'e')
        {
            enableRCFirmware ^= 1;
            strncpy(reportedVersion, "", sizeof(reportedVersion) - 1); // Reset to force c) menu
        }
        else if (incoming == 'x')
            break;
        else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_EMPTY)
            break;
        else if (incoming == INPUT_RESPONSE_GETCHARACTERNUMBER_TIMEOUT)
            break;
        else
            printUnknown(incoming);
    }

    clearBuffer(); // Empty buffer of any newline chars
}





// Format the firmware version
void formatFirmwareVersion(uint8_t major, uint8_t minor, char * buffer, int bufferLength, bool includeDate)
{
    char prefix;

    // Construct the full or release candidate version number
    prefix = ENABLE_DEVELOPER ? 'd' : 'v';
    if (enableRCFirmware && (bufferLength >= 21))
        // 123456789012345678901
        // pxxx.yyy-dd-mmm-yyyy0
        snprintf(buffer, bufferLength, "%c%d.%d-%s", prefix, major, minor, __DATE__);

    // Construct a truncated version number
    else if (bufferLength >= 9)
        // 123456789
        // pxxx.yyy0
        snprintf(buffer, bufferLength, "%c%d.%d", prefix, major, minor);

    // The buffer is too small for the version number
    else
    {
        systemPrintf("ERROR: Buffer too small for version number!\r\n");
        if (bufferLength > 0)
            *buffer = 0;
    }
}

// Get the current firmware version
void getFirmwareVersion(char * buffer, int bufferLength, bool includeDate)
{
    formatFirmwareVersion(FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, buffer, bufferLength, includeDate);
}

const char * otaGetUrl()
{
    // Select the URL for the over-the-air (OTA) updates
    return enableRCFirmware ? OTA_RC_FIRMWARE_JSON_URL : OTA_FIRMWARE_JSON_URL;
}

// Returns true if we successfully got the versionAvailable
// Modifies versionAvailable with OTA getVersion response
// Connects to WiFi as needed
bool otaCheckVersion(char *versionAvailable, uint8_t versionAvailableLength)
{
    bool gotVersion = false;
#ifdef COMPILE_WIFI
    bool previouslyConnected = wifiIsConnected();

    if (wifiConnect(10000) == true)
    {
        char versionString[21];
        getFirmwareVersion(versionString, sizeof(versionString), enableRCFirmware);
        systemPrintf("Current firmware version: v%s\r\n", versionString);

        const char * url = otaGetUrl();
        systemPrintf("Checking to see if an update is available from %s\r\n", url);

        ESP32OTAPull ota;

        int response = ota.CheckForOTAUpdate(url, versionString, ESP32OTAPull::DONT_DO_UPDATE);

        // We don't care if the library thinks the available firmware is newer, we just need a successful JSON parse
        if (response == ESP32OTAPull::UPDATE_AVAILABLE || response == ESP32OTAPull::NO_UPDATE_AVAILABLE)
        {
            gotVersion = true;

            // Call getVersion after original inquiry
            String otaVersion = ota.GetVersion();
            otaVersion.toCharArray(versionAvailable, versionAvailableLength);
        }
        else if (response == ESP32OTAPull::HTTP_FAILED)
        {
            systemPrintln("Firmware server not available");
        }
        else
        {
            systemPrintln("OTA failed");
        }
    }
    else
    {
        systemPrintln("WiFi not available.");
    }

    if (systemState != STATE_WIFI_CONFIG)
    {
        // wifiStop() turns off the entire radio including the webserver. We need to turn off Station mode only.
        // For now, unit exits AP mode via reset so if we are in AP config mode, leave WiFi Station running.

        // If WiFi was originally off, turn it off again
        if (previouslyConnected == false)
            wifiStop();
    }

    if (gotVersion == true)
        log_d("Available OTA firmware version: %s\r\n", versionAvailable);

#endif  // COMPILE_WIFI
    return (gotVersion);
}

// Force updates firmware using OTA pull
// Exits by either updating firmware and resetting, or failing to connect
void otaUpdate()
{
#ifdef COMPILE_WIFI
    bool previouslyConnected = wifiIsConnected();

    if (wifiConnect(10000) == true)
    {
        char versionString[9];
        formatFirmwareVersion(0, 0, versionString, sizeof(versionString), false);

        ESP32OTAPull ota;

        int response;
        const char * url = otaGetUrl();
        response = ota.CheckForOTAUpdate(url, &versionString[1], ESP32OTAPull::DONT_DO_UPDATE);

        if (response == ESP32OTAPull::UPDATE_AVAILABLE)
        {
            systemPrintln("Installing new firmware");
            ota.SetCallback(otaPullCallback);
            if (enableRCFirmware == false)
            ota.CheckForOTAUpdate(url, versionString); // Install new firmware, no reset

            if (apConfigFirmwareUpdateInProcess)
            {
#ifdef COMPILE_AP
                // Tell AP page to display reset info
                websocket->textAll("confirmReset,1,");
#endif  // COMPILE_AP
            }
            ESP.restart();
        }

        else if (response == ESP32OTAPull::NO_UPDATE_AVAILABLE)
        {
            systemPrintln("OTA Update: Current firmware is up to date");
        }
        else if (response == ESP32OTAPull::HTTP_FAILED)
        {
            systemPrintln("OTA Update: Firmware server not available");
        }
        else
        {
            systemPrintln("OTA Update: OTA failed");
        }
    }
    else
    {
        systemPrintln("WiFi not available.");
    }

    // Update failed. If WiFi was originally off, turn it off again
    if (previouslyConnected == false)
        wifiStop();

#endif  // COMPILE_WIFI
}

// Called while the OTA Pull update is happening
void otaPullCallback(int bytesWritten, int totalLength)
{
    static int previousPercent = -1;
    int percent = 100 * bytesWritten / totalLength;
    if (percent != previousPercent)
    {
        // Indicate progress
        int barWidthInCharacters = 20; // Width of progress bar, ie [###### % complete
        long portionSize = totalLength / barWidthInCharacters;

        // Indicate progress
        systemPrint("\r\n[");
        int barWidth = bytesWritten / portionSize;
        for (int x = 0; x < barWidth; x++)
            systemPrint("=");
        systemPrintf(" %d%%", percent);
        if (bytesWritten == totalLength)
            systemPrintln("]");


        if (apConfigFirmwareUpdateInProcess == true)
        {
#ifdef COMPILE_AP
            char myProgress[50];
            snprintf(myProgress, sizeof(myProgress), "otaFirmwareStatus,%d,", percent);
            websocket->textAll(myProgress);
#endif  // COMPILE_AP
        }

        previousPercent = percent;
    }
}

const char *otaPullErrorText(int code)
{
#ifdef COMPILE_WIFI
    switch (code)
    {
    case ESP32OTAPull::UPDATE_AVAILABLE:
        return "An update is available but wasn't installed";
    case ESP32OTAPull::NO_UPDATE_PROFILE_FOUND:
        return "No profile matches";
    case ESP32OTAPull::NO_UPDATE_AVAILABLE:
        return "Profile matched, but update not applicable";
    case ESP32OTAPull::UPDATE_OK:
        return "An update was done, but no reboot";
    case ESP32OTAPull::HTTP_FAILED:
        return "HTTP GET failure";
    case ESP32OTAPull::WRITE_ERROR:
        return "Write error";
    case ESP32OTAPull::JSON_PROBLEM:
        return "Invalid JSON";
    case ESP32OTAPull::OTA_UPDATE_FAIL:
        return "Update fail (no OTA partition?)";
    default:
        if (code > 0)
            return "Unexpected HTTP response code";
        break;
    }
#endif  // COMPILE_WIFI
    return "Unknown error";
}

// Returns true if reportedVersion is newer than currentVersion
// Version number comes in as v2.7-Jan 5 2023
// 2.7-Jan 5 2023 is newer than v2.7-Jan 1 2023
bool isReportedVersionNewer(char *reportedVersion, char *currentVersion)
{
    float currentVersionNumber = 0.0;
    int currentDay = 0;
    int currentMonth = 0;
    int currentYear = 0;

    float reportedVersionNumber = 0.0;
    int reportedDay = 0;
    int reportedMonth = 0;
    int reportedYear = 0;

    breakVersionIntoParts(currentVersion, &currentVersionNumber, &currentYear, &currentMonth, &currentDay);
    breakVersionIntoParts(reportedVersion, &reportedVersionNumber, &reportedYear, &reportedMonth, &reportedDay);

    log_d("currentVersion: %f %d %d %d", currentVersionNumber, currentYear, currentMonth, currentDay);
    log_d("reportedVersion: %f %d %d %d", reportedVersionNumber, reportedYear, reportedMonth, reportedDay);
    if (enableRCFirmware)
        log_d("RC firmware enabled");

    // Production firmware is named "2.6"
    // Release Candidate firmware is named "2.6-Dec 5 2022"

    // If the user is not using Release Candidate firmware, then check only the version number
    if (enableRCFirmware == false)
    {
        // Check only the version number
        if (reportedVersionNumber > currentVersionNumber)
            return (true);
        return (false);
    }

    // For RC firmware, compare firmware date as well
    // Check version number
    if (reportedVersionNumber > currentVersionNumber)
        return (true);

    // Check which date is more recent
    // https://stackoverflow.com/questions/5283120/date-comparison-to-find-which-is-bigger-in-c
    int reportedVersionScore = reportedDay + reportedMonth * 100 + reportedYear * 2000;
    int currentVersionScore = currentDay + currentMonth * 100 + currentYear * 2000;

    if (reportedVersionScore > currentVersionScore)
    {
        log_d("Reported version is greater");
        return (true);
    }

    return (false);
}

// Version number comes in as v2.7-Jan 5 2023
// Given a char string, break into version number, year, month, day
// Returns false if parsing failed
bool breakVersionIntoParts(char *version, float *versionNumber, int *year, int *month, int *day)
{
    char monthStr[20];
    int placed = 0;

    if (enableRCFirmware == false)
    {
        placed = sscanf(version, "%f", versionNumber);
        if (placed != 1)
        {
            log_d("Failed to sscanf basic");
            return (false); // Something went wrong
        }
    }
    else
    {
        placed = sscanf(version, "%f-%s %d %d", versionNumber, monthStr, day, year);

        if (placed != 4)
        {
            log_d("Failed to sscanf RC");
            return (false); // Something went wrong
        }

        (*month) = mapMonthName(monthStr);
        if (*month == -1)
            return (false); // Something went wrong
    }

    return (true);
}

// https://stackoverflow.com/questions/21210319/assign-month-name-and-integer-values-from-string-using-sscanf
int mapMonthName(char *mmm)
{
    static char const *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (size_t i = 0; i < sizeof(months) / sizeof(months[0]); i++)
    {
        if (strcmp(mmm, months[i]) == 0)
            return i + 1;
    }
    return -1;
}
