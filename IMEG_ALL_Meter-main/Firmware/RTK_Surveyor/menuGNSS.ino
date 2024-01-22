#ifdef COMPILE_MENUS
// Configure the basic GNSS reception settings
// Update rate, constellations, etc
void menuGNSS()
{
    restartRover = false; // If user modifies any NTRIP settings, we need to restart the rover

    while (1)
    {
        int minCNO = settings.minCNO_F9P;
        if (zedModuleType == PLATFORM_F9R)
            minCNO = settings.minCNO_F9R;

        systemPrintln();
        systemPrintln("Menu: GNSS Receiver");

        // Because we may be in base mode, do not get freq from module, use settings instead
        float measurementFrequency = (1000.0 / settings.measurementRate) / settings.navigationRate;

        systemPrint("1) Set measurement rate in Hz: ");
        systemPrintln(measurementFrequency, 5);

        systemPrint("2) Set measurement rate in seconds between measurements: ");
        systemPrintln(1 / measurementFrequency, 5);

        systemPrintln("\tNote: The measurement rate is overridden to 1Hz when in Base mode.");

        systemPrintln();

        systemPrintln("4) Set Constellations");


        systemPrintln("x) Exit");

        int incoming = getNumber(); // Returns EXIT, TIMEOUT, or long

        if (incoming == 1)
        {
            systemPrint("Enter GNSS measurement rate in Hz: ");
            double rate = getDouble();
            if (rate < 0.00012 || rate > 20.0) // 20Hz limit with all constellations enabled
            {
                systemPrintln("Error: Measurement rate out of range");
            }
            else
            {
                //setRate(1.0 / rate); // Convert Hz to seconds. This will set settings.measurementRate,
                                     // settings.navigationRate, and GSV message
                // Settings recorded to NVM and file at main menu exit
            }
        }
        else if (incoming == 2)
        {
            systemPrint("Enter GNSS measurement rate in seconds between measurements: ");
            float rate = getDouble();
            if (rate < 0.0 || rate > 8255.0) // Limit of 127 (navRate) * 65000ms (measRate) = 137 minute limit.
            {
                systemPrintln("Error: Measurement rate out of range");
            }
            else
            {
                //setRate(rate); // This will set settings.measurementRate, settings.navigationRate, and GSV message
                // Settings recorded to NVM and file at main menu exit
            }
        }
        else if (incoming == 4)
        {
            menuConstellations();
        }
        else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
            break;
        else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
            break;
        else
            printUnknown(incoming);
    }


    clearBuffer(); // Empty buffer of any newline chars
}

// Controls the constellations that are used to generate a fix and logged
void menuConstellations()
{
    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: Constellations");

        for (int x = 0; x < MAX_CONSTELLATIONS; x++)
        {
            systemPrintf("%d) Constellation %s: ", x + 1, settings.ubxConstellations[x].textName);
            if (settings.ubxConstellations[x].enabled == true)
                systemPrint("Enabled");
            else
                systemPrint("Disabled");
            systemPrintln();
        }

        systemPrintln("x) Exit");

        int incoming = getNumber(); // Returns EXIT, TIMEOUT, or long

        if (incoming >= 1 && incoming <= MAX_CONSTELLATIONS)
        {
            incoming--; // Align choice to constallation array of 0 to 5

            settings.ubxConstellations[incoming].enabled ^= 1;

            // 3.10.6: To avoid cross-correlation issues, it is recommended that GPS and QZSS are always both enabled or
            // both disabled.
            if (incoming == SFE_UBLOX_GNSS_ID_GPS || incoming == 4) // QZSS ID is 5 but array location is 4
            {
                settings.ubxConstellations[SFE_UBLOX_GNSS_ID_GPS].enabled =
                    settings.ubxConstellations[incoming].enabled; // GPS ID is 0 and array location is 0
                settings.ubxConstellations[4].enabled =
                    settings.ubxConstellations[incoming].enabled; // QZSS ID is 5 but array location is 4
            }
        }
        else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
            break;
        else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
            break;
        else
            printUnknown(incoming);
    }

    // Apply current settings to module
    setConstellations(true); // Apply newCfg and sendCfg values to batch

    clearBuffer(); // Empty buffer of any newline chars
}

#endif //COMPILE_MENUS

// Given the number of seconds between desired solution reports, determine measurementRate and navigationRate
// measurementRate > 25 & <= 65535
// navigationRate >= 1 && <= 127
// We give preference to limiting a measurementRate to 30s or below due to reported problems with measRates above 30.
/*
bool setRate(double secondsBetweenSolutions)
{
    uint16_t measRate = 0; // Calculate these locally and then attempt to apply them to ZED at completion
    uint16_t navRate = 0;

    // If we have more than an hour between readings, increase mesaurementRate to near max of 65,535
    if (secondsBetweenSolutions > 3600.0)
    {
        measRate = 65000;
    }

    // If we have more than 30s, but less than 3600s between readings, use 30s measurement rate
    else if (secondsBetweenSolutions > 30.0)
    {
        measRate = 30000;
    }

    // User wants measurements less than 30s (most common), set measRate to match user request
    // This will make navRate = 1.
    else
    {
        measRate = secondsBetweenSolutions * 1000.0;
    }

    navRate = secondsBetweenSolutions * 1000.0 / measRate; // Set navRate to nearest int value
    measRate = secondsBetweenSolutions * 1000.0 / navRate; // Adjust measurement rate to match actual navRate

    // systemPrintf("measurementRate / navRate: %d / %d\r\n", measRate, navRate);

    bool response = true;
    response &= theGNSS.newCfgValset();
    response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_MEAS, measRate);
    response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_NAV, navRate);

    int gsvRecordNumber = getMessageNumberByName("UBX_NMEA_GSV");

    // If enabled, adjust GSV NMEA to be reported at 1Hz to avoid swamping SPP connection
    if (settings.ubxMessageRates[gsvRecordNumber] > 0)
    {
        float measurementFrequency = (1000.0 / measRate) / navRate;
        if (measurementFrequency < 1.0)
            measurementFrequency = 1.0;

        log_d("Adjusting GSV setting to %f", measurementFrequency);

        setMessageRateByName("UBX_NMEA_GSV", measurementFrequency); // Update GSV setting in file
        response &= theGNSS.addCfgValset(ubxMessages[gsvRecordNumber].msgConfigKey,
                                         settings.ubxMessageRates[gsvRecordNumber]); // Update rate on module
    }

    response &= theGNSS.sendCfgValset(); // Closing value - max 4 pairs

    // If we successfully set rates, only then record to settings
    if (response == true)
    {
        settings.measurementRate = measRate;
        settings.navigationRate = navRate;
    }
    else
    {
        systemPrintln("Failed to set measurement and navigation rates");
        return (false);
    }

    return (true);
}
*/

// Print the module type and firmware version
void printZEDInfo()
{
    if (zedModuleType == PLATFORM_F9P)
        systemPrintf("ZED-F9P firmware: %s\r\n", zedFirmwareVersion);
    else if (zedModuleType == PLATFORM_F9R)
        systemPrintf("ZED-F9R firmware: %s\r\n", zedFirmwareVersion);
    else
        systemPrintf("Unknown module with firmware: %s\r\n", zedFirmwareVersion);
}


