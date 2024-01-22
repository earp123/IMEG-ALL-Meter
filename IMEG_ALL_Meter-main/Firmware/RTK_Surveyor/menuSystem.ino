#ifdef COMPILE_MENUS
// Display current system status
void menuSystem()
{
    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: System");

        beginI2C();
        if (online.i2c == false)
            systemPrintln("I2C: Offline - Something is causing bus problems");

        systemPrint("GNSS: ");
        if (online.gnss == true)
        {
            systemPrint("Online - ");

            printZEDInfo();

            systemPrintf("Module unique chip ID: %s\r\n", zedUniqueId);

            printCurrentConditions();
        }
        else
            systemPrintln("Offline");

        systemPrint("Display: ");
        if (online.display == true)
            systemPrintln("Online");
        else
            systemPrintln("Offline");

        if (online.accelerometer == true)
            systemPrintln("Accelerometer: Online");

        systemPrint("Fuel Gauge: ");
        if (online.battery == true)
        {
            systemPrint("Online - ");

            battLevel = lipo.getSOC();
            battVoltage = lipo.getVoltage();

            systemPrintf("Batt (%d%%) / Voltage: %0.02fV", battLevel, battVoltage);
            systemPrintln();
        }
        else
            systemPrintln("Offline");

        systemPrint("microSD: ");
        if (online.microSD == true)
            systemPrintln("Online");
        else
            systemPrintln("Offline");

        // Display the Bluetooth status
        bluetoothTest(false);

#ifdef COMPILE_WIFI
        systemPrint("WiFi MAC Address: ");
        systemPrintf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", wifiMACAddress[0], wifiMACAddress[1], wifiMACAddress[2],
                     wifiMACAddress[3], wifiMACAddress[4], wifiMACAddress[5]);
        if (wifiState == WIFI_CONNECTED)
            wifiDisplayIpAddress();
#endif  // COMPILE_WIFI

        // Display the uptime
        uint64_t uptimeMilliseconds = millis();
        uint32_t uptimeDays = 0;
        byte uptimeHours = 0;
        byte uptimeMinutes = 0;
        byte uptimeSeconds = 0;

        uptimeDays = uptimeMilliseconds / MILLISECONDS_IN_A_DAY;
        uptimeMilliseconds %= MILLISECONDS_IN_A_DAY;

        uptimeHours = uptimeMilliseconds / MILLISECONDS_IN_AN_HOUR;
        uptimeMilliseconds %= MILLISECONDS_IN_AN_HOUR;

        uptimeMinutes = uptimeMilliseconds / MILLISECONDS_IN_A_MINUTE;
        uptimeMilliseconds %= MILLISECONDS_IN_A_MINUTE;

        uptimeSeconds = uptimeMilliseconds / MILLISECONDS_IN_A_SECOND;
        uptimeMilliseconds %= MILLISECONDS_IN_A_SECOND;

        systemPrint("System Uptime: ");
        systemPrintf("%d %02d:%02d:%02d.%03lld (Resets: %d)\r\n", uptimeDays, uptimeHours, uptimeMinutes, uptimeSeconds,
                     uptimeMilliseconds, settings.resetCount);

        

        systemPrint("e) Echo User Input: ");
        if (settings.echoUserInput == true)
            systemPrintln("On");
        else
            systemPrintln("Off");

        systemPrintln("d) Configure Debug");

        systemPrintf("z) Set time zone offset: %02d:%02d:%02d\r\n", settings.timeZoneHours, settings.timeZoneMinutes,
                     settings.timeZoneSeconds);

        systemPrint("b) Set Bluetooth Mode: ");
        if (settings.bluetoothRadioType == BLUETOOTH_RADIO_SPP)
            systemPrintln("Classic");
        else if (settings.bluetoothRadioType == BLUETOOTH_RADIO_BLE)
            systemPrintln("BLE");
        else
            systemPrintln("Off");

        systemPrintln("r) Reset all settings to default");

        // Support mode switching
        
        systemPrintln("R) Switch to Rover mode");
        systemPrintln("W) Switch to WiFi Config mode");
        systemPrintln("S) Shut down");

        systemPrintln("x) Exit");

        byte incoming = getCharacterNumber();

        if (incoming == 'd')
            menuDebug();
        else if (incoming == 'z')
        {
            systemPrint("Enter time zone hour offset (-23 <= offset <= 23): ");
            int value = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((value != INPUT_RESPONSE_GETNUMBER_EXIT) && (value != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (value < -23 || value > 23)
                    systemPrintln("Error: -24 < hours < 24");
                else
                {
                    settings.timeZoneHours = value;

                    systemPrint("Enter time zone minute offset (-59 <= offset <= 59): ");
                    int value = getNumber(); // Returns EXIT, TIMEOUT, or long
                    if ((value != INPUT_RESPONSE_GETNUMBER_EXIT) && (value != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
                    {
                        if (value < -59 || value > 59)
                            systemPrintln("Error: -60 < minutes < 60");
                        else
                        {
                            settings.timeZoneMinutes = value;

                            systemPrint("Enter time zone second offset (-59 <= offset <= 59): ");
                            int value = getNumber(); // Returns EXIT, TIMEOUT, or long
                            if ((value != INPUT_RESPONSE_GETNUMBER_EXIT) && (value != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
                            {
                                if (value < -59 || value > 59)
                                    systemPrintln("Error: -60 < seconds < 60");
                                else
                                {
                                    settings.timeZoneSeconds = value;
                                    online.rtc = false;
                                    syncRTCInterval =
                                        1000; // Reset syncRTCInterval to 1000ms (tpISR could have set it to 59000)
                                    rtcSyncd = false;
                                    updateRTC();
                                } // Succesful seconds
                            }
                        } // Succesful minute
                    }
                } // Succesful hours
            }
        }
        else if (incoming == 'e')
        {
            settings.echoUserInput ^= 1;
        }
        else if (incoming == 'b')
        {
            // Restart Bluetooth
            bluetoothStop();
            if (settings.bluetoothRadioType == BLUETOOTH_RADIO_OFF)
                settings.bluetoothRadioType = BLUETOOTH_RADIO_BLE;
            else if (settings.bluetoothRadioType == BLUETOOTH_RADIO_BLE)
                settings.bluetoothRadioType = BLUETOOTH_RADIO_OFF;
            
            bluetoothStart();
        }
        else if (incoming == 'r')
        {
            systemPrintln("\r\nResetting to factory defaults. Press 'y' to confirm:");
            byte bContinue = getCharacterNumber();
            if (bContinue == 'y')
            {
                factoryReset(false); //We do not have the SD semaphore
            }
            else
                systemPrintln("Reset aborted");
        }

        // Support mode switching
        else if (incoming == 'R')
        {
            forceSystemStateUpdate = true; // Imediately go to this new state
            changeState(STATE_ROVER_NOT_STARTED);
        }
        else if (incoming == 'W')
        {
            forceSystemStateUpdate = true; // Imediately go to this new state
            changeState(STATE_WIFI_CONFIG_NOT_STARTED);
        }
        else if (incoming == 'S')
        {
            systemPrintln("Shutting down...");
            forceDisplayUpdate = true;
            powerDown(true);
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

// Set WiFi credentials
// Enable TCP connections
void menuWiFi()
{
    bool restartWiFi = false; // Restart WiFi if user changes anything

    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: WiFi Networks");

        for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
        {
            systemPrintf("%d) SSID %d: %s\r\n", (x * 2) + 1, x + 1, settings.wifiNetworks[x].ssid);
            systemPrintf("%d) Password %d: %s\r\n", (x * 2) + 2, x + 1, settings.wifiNetworks[x].password);
        }

        systemPrint("a) Configure device via WiFi Access Point or connect to WiFi: ");
        systemPrintf("%s\r\n", settings.wifiConfigOverAP ? "AP" : "WiFi");

        systemPrint("c) WiFi TCP Client (connect to phone): ");
        systemPrintf("%s", settings.enableTcpClient ? "Enabled" : "Disabled");
        if (settings.enableTcpClient && settings.enableTcpServer)
            systemPrintln(" **");
        else
            systemPrintln("");

        systemPrint("s) WiFi TCP Server: ");
        systemPrintf("%s", settings.enableTcpServer ? "Enabled" : "Disabled");
        if (settings.enableTcpClient && settings.enableTcpServer)
            systemPrintln(" **");
        else
            systemPrintln("");

        if (settings.enableTcpServer == true || settings.enableTcpClient == true)
            systemPrintf("p) WiFi TCP Port: %ld\r\n", settings.wifiTcpPort);

        systemPrint("m) MDNS: ");
        systemPrintf("%s\r\n", settings.mdnsEnable ? "Enabled" : "Disabled");

        systemPrintln("x) Exit");

        if (settings.enableTcpClient && settings.enableTcpServer)
            systemPrintln(
                "\r\n** TCP Server and Client can not be enabled at the same time. Please disable one of them");

        byte incoming = getCharacterNumber();

        if (incoming >= 1 && incoming <= MAX_WIFI_NETWORKS * 2)
        {
            int arraySlot = ((incoming - 1) / 2); // Adjust incoming to array starting at 0

            if (incoming % 2 == 1)
            {
                systemPrintf("Enter SSID network %d: ", arraySlot + 1);
                getString(settings.wifiNetworks[arraySlot].ssid, sizeof(settings.wifiNetworks[arraySlot].ssid));
                restartWiFi = true; // If we are modifying the SSID table, force restart of WiFi
            }
            else
            {
                systemPrintf("Enter Password for %s: ", settings.wifiNetworks[arraySlot].ssid);
                getString(settings.wifiNetworks[arraySlot].password, sizeof(settings.wifiNetworks[arraySlot].password));
                restartWiFi = true; // If we are modifying the SSID table, force restart of WiFi
            }
        }
        else if (incoming == 'a')
        {
            settings.wifiConfigOverAP ^= 1;
            restartWiFi = true;
        }

        else if (incoming == 'c')
        {
            // Toggle WiFi NEMA client (connect to phone)
            settings.enableTcpClient ^= 1;
            restartWiFi = true;
        }
        else if (incoming == 's')
        {
            // Toggle WiFi NEMA server
            settings.enableTcpServer ^= 1;
            if ((!settings.enableTcpServer) && online.tcpServer)
            {
                // Tell the UART2 tasks that the TCP server is shutting down
                online.tcpServer = false;

                // Wait for the UART2 tasks to close the TCP client connections
                while (wifiTcpServerActive())
                    delay(5);
                systemPrintln("TCP Server offline");
            }
            restartWiFi = true;
        }
        else if (incoming == 'p')
        {
            systemPrint("Enter the TCP port to use (0 to 65535): ");
            int wifiTcpPort = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((wifiTcpPort != INPUT_RESPONSE_GETNUMBER_EXIT) && (wifiTcpPort != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (wifiTcpPort < 0 || wifiTcpPort > 65535)
                    systemPrintln("Error: TCP Port out of range");
                else
                {
                    settings.wifiTcpPort = wifiTcpPort; // Recorded to NVM and file at main menu exit
                    restartWiFi = true;
                }
            }
        }
        else if (incoming == 'm')
        {
            settings.mdnsEnable ^= 1;
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

    // Erase passwords from empty SSID entries
    for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
    {
        if (strlen(settings.wifiNetworks[x].ssid) == 0)
            strcpy(settings.wifiNetworks[x].password, "");
    }

    // Restart WiFi if anything changes
    if (restartWiFi == true)
    {
        // Restart the AP webserver if we are in that state
        if (systemState == STATE_WIFI_CONFIG)
            requestChangeState(STATE_WIFI_CONFIG_NOT_STARTED);
        else
        {
            // Restart WiFi if we are not in AP config mode
            if (wifiIsConnected())
            {
                log_d("Menu caused restarting of WiFi");
                wifiStop();
                wifiStart();
                wifiConnectionAttempts = 0; // Reset the timeout
            }
        }
    }

    clearBuffer(); // Empty buffer of any newline chars
}

// Toggle control of heap reports and I2C GNSS debug
void menuDebug()
{
    while (1)
    {
        systemPrintln();
        systemPrintln("Menu: Debug");

        systemPrintf("Filtered by parser: %d NMEA / %d RTCM / %d UBX\r\n", failedParserMessages_NMEA,
                     failedParserMessages_RTCM, failedParserMessages_UBX);

        systemPrint("1) u-blox I2C Debugging Output: ");
        if (settings.enableI2Cdebug == true)
            systemPrintln("Enabled");
        else
            systemPrintln("Disabled");

        systemPrint("2) Heap Reporting: ");
        if (settings.enableHeapReport == true)
            systemPrintln("Enabled");
        else
            systemPrintln("Disabled");

        systemPrint("3) Task Highwater Reporting: ");
        if (settings.enableTaskReports == true)
            systemPrintln("Enabled");
        else
            systemPrintln("Disabled");

        systemPrint("4) Set SPI/SD Interface Frequency: ");
        systemPrint(settings.spiFrequency);
        systemPrintln(" MHz");

        systemPrint("5) Set SPP RX Buffer Size: ");
        systemPrintln(settings.sppRxQueueSize);

        systemPrint("6) Set SPP TX Buffer Size: ");
        systemPrintln(settings.sppTxQueueSize);

        systemPrintf("8) Display Reset Counter: %d - ", settings.resetCount);
        if (settings.enableResetDisplay == true)
            systemPrintln("Enabled");
        else
            systemPrintln("Disabled");

        systemPrint("9) GNSS Serial Timeout: ");
        systemPrintln(settings.serialTimeoutGNSS);

        systemPrint("10) Periodically print WiFi IP Address: ");
        systemPrintf("%s\r\n", settings.enablePrintWifiIpAddress ? "Enabled" : "Disabled");

        systemPrint("11) Periodically print state: ");
        systemPrintf("%s\r\n", settings.enablePrintState ? "Enabled" : "Disabled");

        systemPrint("12) Periodically print WiFi state: ");
        systemPrintf("%s\r\n", settings.enablePrintWifiState ? "Enabled" : "Disabled");


        systemPrint("15) Periodically print position: ");
        systemPrintf("%s\r\n", settings.enablePrintPosition ? "Enabled" : "Disabled");

        systemPrint("16) Periodically print CPU idle time: ");
        systemPrintf("%s\r\n", settings.enablePrintIdleTime ? "Enabled" : "Disabled");

        systemPrintln("17) Mirror ZED-F9x's UART1 settings to USB");

        systemPrint("18) Print battery status messages: ");
        systemPrintf("%s\r\n", settings.enablePrintBatteryMessages ? "Enabled" : "Disabled");

        systemPrint("19) Print Rover accuracy messages: ");
        systemPrintf("%s\r\n", settings.enablePrintRoverAccuracy ? "Enabled" : "Disabled");

        systemPrint("20) Print messages with bad checksums or CRCs: ");
        systemPrintf("%s\r\n", settings.enablePrintBadMessages ? "Enabled" : "Disabled");

        systemPrint("21) Print log file messages: ");
        systemPrintf("%s\r\n", settings.enablePrintLogFileMessages ? "Enabled" : "Disabled");

        systemPrint("22) Print log file status: ");
        systemPrintf("%s\r\n", settings.enablePrintLogFileStatus ? "Enabled" : "Disabled");

        systemPrint("23) Print ring buffer offsets: ");
        systemPrintf("%s\r\n", settings.enablePrintRingBufferOffsets ? "Enabled" : "Disabled");

        systemPrint("29) Run Logging Test: ");
        systemPrintf("%s\r\n", settings.runLogTest ? "Enabled" : "Disabled");

        systemPrintln("30) Run Bluetooth Test");

        systemPrint("31) Print TCP status: ");
        systemPrintf("%s\r\n", settings.enablePrintTcpStatus ? "Enabled" : "Disabled");

        systemPrint("32) ESP-Now Broadcast Override: ");
        systemPrintf("%s\r\n", settings.espnowBroadcast ? "Enabled" : "Disabled");

        systemPrint("33) Print buffer overruns: ");
        systemPrintf("%s\r\n", settings.enablePrintBufferOverrun ? "Enabled" : "Disabled");

        systemPrint("34) Set UART Receive Buffer Size: ");
        systemPrintln(settings.uartReceiveBufferSize);

        systemPrint("35) Set GNSS Handler Buffer Size: ");
        systemPrintln(settings.gnssHandlerBufferSize);

        systemPrint("36) Print SD and UART buffer sizes: ");
        systemPrintf("%s\r\n", settings.enablePrintSDBuffers ? "Enabled" : "Disabled");

        systemPrint("37) Print RTC resyncs: ");
        systemPrintf("%s\r\n", settings.enablePrintRtcSync ? "Enabled" : "Disabled");

        systemPrint("38) Print NTP Request diagnostics: ");
        systemPrintf("%s\r\n", settings.enablePrintNTPDiag ? "Enabled" : "Disabled");

        systemPrint("39) Print Ethernet diagnostics: ");
        systemPrintf("%s\r\n", settings.enablePrintEthernetDiag ? "Enabled" : "Disabled");

        systemPrint("40) Set L-Band RTK Fix Timeout (seconds): ");
        if (settings.lbandFixTimeout_seconds > 0)
            systemPrintln(settings.lbandFixTimeout_seconds);
        else
            systemPrintln("Disabled - no resets");

        systemPrint("41) Set BT Read Task Priority: ");
        systemPrintln(settings.btReadTaskPriority);

        systemPrint("42) Set GNSS Read Task Priority: ");
        systemPrintln(settings.gnssReadTaskPriority);

        systemPrint("43) Set GNSS Data Handler Task Priority: ");
        systemPrintln(settings.handleGnssDataTaskPriority);

        systemPrint("44) Set BT Read Task Core: ");
        systemPrintln(settings.btReadTaskCore);

        systemPrint("45) Set GNSS Read Task Core: ");
        systemPrintln(settings.gnssReadTaskCore);

        systemPrint("46) Set GNSS Data Handler Core: ");
        systemPrintln(settings.handleGnssDataTaskCore);

        systemPrint("47) Set Serial GNSS RX Full Threshold: ");
        systemPrintln(settings.serialGNSSRxFullThreshold);

        systemPrint("48) Set Core used for GNSS UART Interrupts: ");
        systemPrintln(settings.gnssUartInterruptsCore);

        systemPrint("49) Set Core used for Bluetooth Interrupts: ");
        systemPrintln(settings.bluetoothInterruptsCore);

        systemPrint("50) Set Core used for I2C Interrupts: ");
        systemPrintln(settings.i2cInterruptsCore);

        systemPrintln("t) Enter Test Screen");

        systemPrintln("e) Erase LittleFS");

        systemPrintln("r) Force system reset");

        systemPrintln("x) Exit");

        byte incoming = getCharacterNumber();

        if (incoming == 1)
        {
            settings.enableI2Cdebug ^= 1;

            if (settings.enableI2Cdebug)
            {
              theGNSS.enableDebugging(Serial, true); // Enable only the critical debug messages over Serial
            }
            else
                theGNSS.disableDebugging();
        }
        else if (incoming == 2)
        {
            settings.enableHeapReport ^= 1;
        }
        else if (incoming == 3)
        {
            settings.enableTaskReports ^= 1;
        }
        else if (incoming == 4)
        {
            systemPrint("Enter SPI frequency in MHz (1 to 16): ");
            int freq = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((freq != INPUT_RESPONSE_GETNUMBER_EXIT) && (freq != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (freq < 1 || freq > 16) // Arbitrary 16MHz limit
                    systemPrintln("Error: SPI frequency out of range");
                else
                    settings.spiFrequency = freq; // Recorded to NVM and file at main menu exit
            }
        }
        else if (incoming == 5)
        {
            systemPrint("Enter SPP RX Queue Size in Bytes (32 to 16384): ");
            int queSize = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (queSize < 32 || queSize > 16384) // Arbitrary 16k limit
                    systemPrintln("Error: Queue size out of range");
                else
                    settings.sppRxQueueSize = queSize; // Recorded to NVM and file at main menu exit
            }
        }
        else if (incoming == 6)
        {
            systemPrint("Enter SPP TX Queue Size in Bytes (32 to 16384): ");
            int queSize = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (queSize < 32 || queSize > 16384) // Arbitrary 16k limit
                    systemPrintln("Error: Queue size out of range");
                else
                    settings.sppTxQueueSize = queSize; // Recorded to NVM and file at main menu exit
            }
        }
        else if (incoming == 8)
        {
            settings.enableResetDisplay ^= 1;
            if (settings.enableResetDisplay == true)
            {
                settings.resetCount = 0;
                recordSystemSettings(); // Record to NVM
            }
        }
        else if (incoming == 9)
        {
            systemPrint("Enter GNSS Serial Timeout in milliseconds (0 to 1000): ");
            int serialTimeoutGNSS = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((serialTimeoutGNSS != INPUT_RESPONSE_GETNUMBER_EXIT) &&
                (serialTimeoutGNSS != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (serialTimeoutGNSS < 0 || serialTimeoutGNSS > 1000) // Arbitrary 1s limit
                    systemPrintln("Error: Timeout is out of range");
                else
                    settings.serialTimeoutGNSS = serialTimeoutGNSS; // Recorded to NVM and file at main menu exit
            }
        }
        else if (incoming == 10)
        {
            settings.enablePrintWifiIpAddress ^= 1;
        }
        else if (incoming == 11)
        {
            settings.enablePrintState ^= 1;
        }
        else if (incoming == 12)
        {
            settings.enablePrintWifiState ^= 1;
        }
        else if (incoming == 15)
        {
            settings.enablePrintPosition ^= 1;
        }
        else if (incoming == 16)
        {
            settings.enablePrintIdleTime ^= 1;
        }
        else if (incoming == 17)
        {
            bool response = setMessagesUSB(MAX_SET_MESSAGES_RETRIES);

            if (response == false)
                systemPrintln(F("Failed to enable USB messages"));
            else
                systemPrintln(F("USB messages successfully enabled"));
        }
        else if (incoming == 18)
        {
            settings.enablePrintBatteryMessages ^= 1;
        }
        else if (incoming == 19)
        {
            settings.enablePrintRoverAccuracy ^= 1;
        }
        else if (incoming == 20)
        {
            settings.enablePrintBadMessages ^= 1;
        }
        else if (incoming == 21)
        {
            settings.enablePrintLogFileMessages ^= 1;
        }
        else if (incoming == 22)
        {
            settings.enablePrintLogFileStatus ^= 1;
        }
        else if (incoming == 23)
        {
            settings.enablePrintRingBufferOffsets ^= 1;
        }
        
        else if (incoming == 30)
        {
            bluetoothTest(true);
        }
        else if (incoming == 31)
        {
            settings.enablePrintTcpStatus ^= 1;
        }
        else if (incoming == 32)
        {
            settings.espnowBroadcast ^= 1;
        }
        else if (incoming == 33)
        {
            settings.enablePrintBufferOverrun ^= 1;
        }
        else if (incoming == 34)
        {
            systemPrintln("Warning: changing the Receive Buffer Size will restart the RTK. Enter 0 to abort");
            systemPrint("Enter UART Receive Buffer Size in Bytes (32 to 16384): ");
            int queSize = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (queSize < 32 || queSize > 16384) // Arbitrary 16k limit
                    systemPrintln("Error: Queue size out of range");
                else
                {
                    settings.uartReceiveBufferSize = queSize; // Recorded to NVM and file
                    recordSystemSettings();
                    ESP.restart();
                }
            }
        }
        else if (incoming == 35)
        {
            systemPrintln("Warning: changing the Handler Buffer Size will restart the RTK. Enter 0 to abort");
            systemPrint("Enter GNSS Handler Buffer Size in Bytes (32 to 65535): ");
            int queSize = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((queSize != INPUT_RESPONSE_GETNUMBER_EXIT) && (queSize != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (queSize < 32 || queSize > 65535) // Arbitrary 64k limit
                    systemPrintln("Error: Queue size out of range");
                else
                {
                    settings.gnssHandlerBufferSize = queSize; // Recorded to NVM and file
                    recordSystemSettings();
                    ESP.restart();
                }
            }
        }
        else if (incoming == 36)
        {
            settings.enablePrintSDBuffers ^= 1;
        }
        else if (incoming == 37)
        {
            settings.enablePrintRtcSync ^= 1;
        }
        else if (incoming == 38)
        {
            settings.enablePrintNTPDiag ^= 1;
        }
        else if (incoming == 39)
        {
            settings.enablePrintEthernetDiag ^= 1;
        }
        else if (incoming == 40)
        {
            systemPrint("Enter number of seconds in RTK float before hot-start (0-disable to 3600): ");
            int timeout = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((timeout != INPUT_RESPONSE_GETNUMBER_EXIT) && (timeout != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (timeout < 0 || timeout > 3600) // Arbitrary 60 minute limit
                    systemPrintln("Error: Timeout out of range");
                else
                    settings.lbandFixTimeout_seconds = timeout; // Recorded to NVM and file at main menu exit
            }
        }

        else if (incoming == 41)
        {
            systemPrint("Enter BT Read Task Priority (0 to 3): ");
            int btReadTaskPriority = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((btReadTaskPriority != INPUT_RESPONSE_GETNUMBER_EXIT) && (btReadTaskPriority != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (btReadTaskPriority < 0 || btReadTaskPriority > 3)
                    systemPrintln("Error: Task priority out of range");
                else
                {
                    settings.btReadTaskPriority = btReadTaskPriority; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 42)
        {
            systemPrint("Enter GNSS Read Task Priority (0 to 3): ");
            int gnssReadTaskPriority = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((gnssReadTaskPriority != INPUT_RESPONSE_GETNUMBER_EXIT) && (gnssReadTaskPriority != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (gnssReadTaskPriority < 0 || gnssReadTaskPriority > 3)
                    systemPrintln("Error: Task priority out of range");
                else
                {
                    settings.gnssReadTaskPriority = gnssReadTaskPriority; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 43)
        {
            systemPrint("Enter GNSS Data Handle Task Priority (0 to 3): ");
            int handleGnssDataTaskPriority = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((handleGnssDataTaskPriority != INPUT_RESPONSE_GETNUMBER_EXIT) && (handleGnssDataTaskPriority != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (handleGnssDataTaskPriority < 0 || handleGnssDataTaskPriority > 3)
                    systemPrintln("Error: Task priority out of range");
                else
                {
                    settings.handleGnssDataTaskPriority = handleGnssDataTaskPriority; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 44)
        {
            systemPrint("Enter BT Read Task Core (0 or 1): ");
            int btReadTaskCore = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((btReadTaskCore != INPUT_RESPONSE_GETNUMBER_EXIT) && (btReadTaskCore != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (btReadTaskCore < 0 || btReadTaskCore > 1)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.btReadTaskCore = btReadTaskCore; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 45)
        {
            systemPrint("Enter GNSS Read Task Core (0 or 1): ");
            int gnssReadTaskCore = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((gnssReadTaskCore != INPUT_RESPONSE_GETNUMBER_EXIT) && (gnssReadTaskCore != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (gnssReadTaskCore < 0 || gnssReadTaskCore > 1)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.gnssReadTaskCore = gnssReadTaskCore; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 46)
        {
            systemPrint("Enter GNSS Data Handler Task Core (0 or 1): ");
            int handleGnssDataTaskCore = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((handleGnssDataTaskCore != INPUT_RESPONSE_GETNUMBER_EXIT) && (handleGnssDataTaskCore != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (handleGnssDataTaskCore < 0 || handleGnssDataTaskCore > 1)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.handleGnssDataTaskCore = handleGnssDataTaskCore; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 47)
        {
            systemPrint("Enter Serial GNSS RX Full Threshold (1 to 127): ");
            int serialGNSSRxFullThreshold = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((serialGNSSRxFullThreshold != INPUT_RESPONSE_GETNUMBER_EXIT) && (serialGNSSRxFullThreshold != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (serialGNSSRxFullThreshold < 1 || serialGNSSRxFullThreshold > 127)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.serialGNSSRxFullThreshold = serialGNSSRxFullThreshold; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 48)
        {
            systemPrint("Enter Core used for GNSS UART Interrupts (0 or 1): ");
            int gnssUartInterruptsCore = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((gnssUartInterruptsCore != INPUT_RESPONSE_GETNUMBER_EXIT) && (gnssUartInterruptsCore != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (gnssUartInterruptsCore < 0 || gnssUartInterruptsCore > 1)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.gnssUartInterruptsCore = gnssUartInterruptsCore; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 49)
        {
            systemPrint("Not yet implemented! - Enter Core used for Bluetooth Interrupts (0 or 1): ");
            int bluetoothInterruptsCore = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((bluetoothInterruptsCore != INPUT_RESPONSE_GETNUMBER_EXIT) && (bluetoothInterruptsCore != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (bluetoothInterruptsCore < 0 || bluetoothInterruptsCore > 1)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.bluetoothInterruptsCore = bluetoothInterruptsCore; // Recorded to NVM and file
                }
            }
        }
        else if (incoming == 50)
        {
            systemPrint("Enter Core used for I2C Interrupts (0 or 1): ");
            int i2cInterruptsCore = getNumber(); // Returns EXIT, TIMEOUT, or long
            if ((i2cInterruptsCore != INPUT_RESPONSE_GETNUMBER_EXIT) && (i2cInterruptsCore != INPUT_RESPONSE_GETNUMBER_TIMEOUT))
            {
                if (i2cInterruptsCore < 0 || i2cInterruptsCore > 1)
                    systemPrintln("Error: Core out of range");
                else
                {
                    settings.i2cInterruptsCore = i2cInterruptsCore; // Recorded to NVM and file
                }
            }
        }
        
        else if (incoming == 't')
        {
            requestChangeState(STATE_TEST); // We'll enter test mode once exiting all serial menus
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
#endif //COMPILE_MENUS

// Print the current long/lat/alt/HPA/SIV
// From Example11_GetHighPrecisionPositionUsingDouble
void printCurrentConditions()
{
    if (online.gnss == true)
    {
        systemPrint("SIV: ");
        systemPrint(numSV);

        systemPrint(", HPA (m): ");
        systemPrint(horizontalAccuracy, 3);

        systemPrint(", Lat: ");
        systemPrint(latitude, haeNumberOfDecimals);
        systemPrint(", Lon: ");
        systemPrint(longitude, haeNumberOfDecimals);

        systemPrint(", Altitude (m): ");
        systemPrint(altitude, 1);

        systemPrintln();
    }
}

void printCurrentConditionsNMEA()
{
    if (online.gnss == true)
    {
        char systemStatus[100];
        snprintf(systemStatus, sizeof(systemStatus),
                 "%02d%02d%02d.%02d,%02d%02d%02d,%0.3f,%d,%0.9f,%0.9f,%0.2f,%d,%d,%d", gnssHour, gnssMinute, gnssSecond,
                 mseconds, gnssDay, gnssMonth, gnssYear % 2000, // Limit to 2 digits
                 horizontalAccuracy, numSV, latitude, longitude, altitude, fixType, carrSoln, battLevel);

        char nmeaMessage[100]; // Max NMEA sentence length is 82
        createNMEASentence(CUSTOM_NMEA_TYPE_STATUS, nmeaMessage, sizeof(nmeaMessage),
                           systemStatus); // textID, buffer, sizeOfBuffer, text
        systemPrintln(nmeaMessage);
    }
    else
    {
        char nmeaMessage[100]; // Max NMEA sentence length is 82
        createNMEASentence(CUSTOM_NMEA_TYPE_STATUS, nmeaMessage, sizeof(nmeaMessage),
                           (char *)"OFFLINE"); // textID, buffer, sizeOfBuffer, text
        systemPrintln(nmeaMessage);
    }
}


