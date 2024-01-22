// Check to see if we've received serial over USB
// Report status if ~ received, otherwise present config menu
#ifdef COMPILE_MENUS
void updateSerial()
{
    if (systemAvailable())
    {
        byte incoming = systemRead();

        if (incoming == '~')
        {
            // Output custom GNTXT message with all current system data
            // printCurrentConditionsNMEA();
        }
        else
            menuMain(); // Present user menu
    }
}

// Display the options
// If user doesn't respond within a few seconds, return to main loop
void menuMain()
{
    inMainMenu = true;

    while (1)
    {
        systemPrintln();
        char versionString[21];
        //getFirmwareVersion(versionString, sizeof(versionString), true);
        systemPrintf("SparkFun RTK %s %s\r\n", platformPrefix, versionString);

#ifdef COMPILE_BT
        systemPrint("** Bluetooth broadcasting as: ");
        systemPrint(deviceName);
        systemPrintln(" **");
#else  // COMPILE_BT
        systemPrintln("** Bluetooth Not Compiled **");
#endif // COMPILE_BT

        systemPrintln("Menu: Main");

        systemPrintln("1) Configure GNSS Receiver");

        systemPrintln("2) Configure GNSS Messages");

        //systemPrintln("4) Configure Ports");

#ifdef COMPILE_WIFI
        systemPrintln("6) Configure WiFi");
#else  // COMPILE_WIFI
        systemPrintln("6) **WiFi Not Compiled**");
#endif // COMPILE_WIFI

#ifdef COMPILE_ESPNOW
        systemPrintln("r) Configure Radios");
#else  // COMPILE_ESPNOW
        systemPrintln("r) **ESP-Now Not Compiled**");
#endif // COMPILE_ESPNOW

        systemPrintln("s) Configure System");


        if (btPrintEcho)
            systemPrintln("b) Exit Bluetooth Echo mode");

        systemPrintln("x) Exit");

        byte incoming = getCharacterNumber();

        if (incoming == 1)
            menuGNSS();
        else if (incoming == 4)
            menuWiFi();
        else if (incoming == 6)
            menuWiFi();
        else if (incoming == 's')
            menuSystem();

#ifdef COMPILE_ESPNOW
        else if (incoming == 'r')
            menuRadio();
#endif // COMPILE_ESPNOW

        else if (incoming == 'b')
        {
            printEndpoint = PRINT_ENDPOINT_SERIAL;
            systemPrintln("BT device has exited echo mode");
            btPrintEcho = false;
            break; // Exit config menu
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

    recordSystemSettings(); // Once all menus have exited, record the new settings to LittleFS and config file

    if (online.gnss == true)
        theGNSS.saveConfiguration(); // Save the current settings to flash and BBR on the ZED-F9P


    if (restartRover == true)
    {
        restartRover = false;
        requestChangeState(STATE_ROVER_NOT_STARTED); // Restart rover upon exit for latest changes to take effect
    }

    clearBuffer();           // Empty buffer of any newline chars
    btPrintEchoExit = false; // We are out of the menu system
    inMainMenu = false;
}
#endif //COMPILE_MENUS

// Change system wide settings based on current user profile
// Ways to change the ZED settings:
// Menus - we apply ZED changes at the exit of each sub menu
// Settings file - we detect differences between NVM and settings txt file and updateZEDSettings = true
// Profile - Before profile is changed, set updateZEDSettings = true
// AP - once new settings are parsed, set updateZEDSettings = true
// Setup button -
// Factory reset - updatesZEDSettings = true by default

// Erase all settings. Upon restart, unit will use defaults
void factoryReset(bool alreadyHasSemaphore)
{

    tasksStopUART2();

    if (online.gnss == true)
        theGNSS.factoryDefault(); // Reset everything: baud rate, I2C address, update rate, everything. And save to BBR.

    systemPrintln("Settings erased successfully. Rebooting. Goodbye!");
    delay(2000);
    ESP.restart();
}

#ifdef COMPILE_MENUS
// Configure the internal radio, if available
void menuRadio()
{
#ifdef COMPILE_ESPNOW
    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: Radios");

        // Pretty print the MAC of all radios
        systemPrint("  Radio MAC: ");
        for (int x = 0; x < 5; x++)
            systemPrintf("%02X:", wifiMACAddress[x]);
        systemPrintf("%02X\r\n", wifiMACAddress[5]);

        if (settings.espnowPeerCount > 0)
        {
            systemPrintln("  Paired Radios: ");
            for (int x = 0; x < settings.espnowPeerCount; x++)
            {
                systemPrint("    ");
                for (int y = 0; y < 5; y++)
                    systemPrintf("%02X:", settings.espnowPeers[x][y]);
                systemPrintf("%02X\r\n", settings.espnowPeers[x][5]);
            }
        }
        else
            systemPrintln("  No Paired Radios");

        systemPrintln("2) Pair radios");
        systemPrintln("3) Forget all radios");
        if (ENABLE_DEVELOPER)
        {
            systemPrintln("4) Add dummy radio");
            systemPrintln("5) Send dummy data");
            systemPrintln("6) Broadcast dummy data");
        }
      

        systemPrintln("x) Exit");

        int incoming = getNumber(); // Returns EXIT, TIMEOUT, or long

        if (settings.radioType == RADIO_ESPNOW && incoming == 2)
        {
            espnowStaticPairing();
        }
        else if (settings.radioType == RADIO_ESPNOW && incoming == 3)
        {
            systemPrintln("\r\nForgetting all paired radios. Press 'y' to confirm:");
            byte bContinue = getCharacterNumber();
            if (bContinue == 'y')
            {
                if (espnowState > ESPNOW_OFF)
                {
                    for (int x = 0; x < settings.espnowPeerCount; x++)
                        espnowRemovePeer(settings.espnowPeers[x]);
                }
                settings.espnowPeerCount = 0;
                systemPrintln("Radios forgotten");
            }
        }
        else if (ENABLE_DEVELOPER && settings.radioType == RADIO_ESPNOW && incoming == 4)
        {
            uint8_t peer1[] = {0xB8, 0xD6, 0x1A, 0x0D, 0x8F, 0x9C}; // Random MAC
            if (esp_now_is_peer_exist(peer1) == true)
                log_d("Peer already exists");
            else
            {
                // Add new peer to system
                espnowAddPeer(peer1);

                // Record this MAC to peer list
                memcpy(settings.espnowPeers[settings.espnowPeerCount], peer1, 6);
                settings.espnowPeerCount++;
                settings.espnowPeerCount %= ESPNOW_MAX_PEERS;
                recordSystemSettings();
            }

            espnowSetState(ESPNOW_PAIRED);
        }
        else if (ENABLE_DEVELOPER && settings.radioType == RADIO_ESPNOW && incoming == 5)
        {
            uint8_t espnowData[] =
                "This is the long string to test how quickly we can send one string to the other unit. I am going to "
                "need a much longer sentence if I want to get a long amount of data into one transmission. This is "
                "nearing 200 characters but needs to be near 250.";
            esp_now_send(0, (uint8_t *)&espnowData, sizeof(espnowData)); // Send packet to all peers
        }
        else if (ENABLE_DEVELOPER && settings.radioType == RADIO_ESPNOW && incoming == 6)
        {
            uint8_t espnowData[] =
                "This is the long string to test how quickly we can send one string to the other unit. I am going to "
                "need a much longer sentence if I want to get a long amount of data into one transmission. This is "
                "nearing 200 characters but needs to be near 250.";
            uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            esp_now_send(broadcastMac, (uint8_t *)&espnowData, sizeof(espnowData)); // Send packet to all peers
        }

        else if (incoming == INPUT_RESPONSE_GETNUMBER_EXIT)
            break;
        else if (incoming == INPUT_RESPONSE_GETNUMBER_TIMEOUT)
            break;
        else
            printUnknown(incoming);
    }

    radioStart();

    clearBuffer(); // Empty buffer of any newline chars
#endif             // COMPILE_ESPNOW
}
#endif //COMPILE_MENUS