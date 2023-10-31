/*
  This is the main state machine for the device. It's big but controls each step of the system.
  See system state diagram for a visual representation of how states can change to/from.
  Statemachine diagram:
  https://lucid.app/lucidchart/53519501-9fa5-4352-aa40-673f88ca0c9b/edit?invitationId=inv_ebd4b988-513d-4169-93fd-c291851108f8
*/

static uint32_t lastStateTime = 0;

// Given the current state, see if conditions have moved us to a new state
// A user pressing the setup button (change between rover/base) is handled by checkpin_setupButton()
void updateSystemState()
{
    if (millis() - lastSystemStateUpdate > 500 || forceSystemStateUpdate == true)
    {
        lastSystemStateUpdate = millis();
        forceSystemStateUpdate = false;

        // Check to see if any external sources need to change state
        if (newSystemStateRequested == true)
        {
            newSystemStateRequested = false;
            if (systemState != requestedSystemState)
            {
                changeState(requestedSystemState);
                lastStateTime = millis();
            }
        }

        if (settings.enablePrintState && ((millis() - lastStateTime) > 15000))
        {
            changeState(systemState);
            lastStateTime = millis();
        }

        // Move between states as needed
        switch (systemState)
        {
        /*
                          .-----------------------------------.
                          |      STATE_ROVER_NOT_STARTED      |
                          | Text: 'Rover' and 'Rover Started' |
                          '-----------------------------------'
                                            |
                                            |
                                            |
                                            V
                          .-----------------------------------.
                          |         STATE_ROVER_NO_FIX        |
                          |           SIV Icon Blink          |
                          |            "HPA: >30m"            |
                          |             "SIV: 0"              |
                          '-----------------------------------'
                                            |
                                            | GPS Lock
                                            | 3D, 3D+DR
                                            V
                          .-----------------------------------.
                          |          STATE_ROVER_FIX          | Carrier
                          |           SIV Icon Solid          | Solution = 2
                .-------->|            "HPA: .513"            |---------.
                |         |             "SIV: 30"             |         |
                |         '-----------------------------------'         |
                |                           |                           |
                |                           | Carrier Solution = 1      |
                |                           V                           |
                |         .-----------------------------------.         |
                |         |       STATE_ROVER_RTK_FLOAT       |         |
                |  No RTK |     Double Crosshair Blinking     |         |
                +<--------|           "*HPA: .080"            |         |
                ^         |             "SIV: 30"             |         |
                |         '-----------------------------------'         |
                |                        ^         |                    |
                |                        |         | Carrier            |
                |                        |         | Solution = 2       |
                |                        |         V                    |
                |                Carrier |         +<-------------------'
                |           Solution = 1 |         |
                |                        |         V
                |         .-----------------------------------.
                |         |        STATE_ROVER_RTK_FIX        |
                |  No RTK |       Double Crosshair Solid      |
                '---------|           "*HPA: .014"            |
                          |             "SIV: 30"             |
                          '-----------------------------------'

        */
        case (STATE_ROVER_NOT_STARTED): {
            if (online.gnss == false)
            {
                firstRoverStart = false; // If GNSS is offline, we still need to allow button use
                return;
            }
         
            // If we are survey'd in, but switch is rover then disable survey
            if (configureUbloxModuleRover() == false)
            {
                systemPrintln("Rover config failed");
                
                return;
            }



            wifiStop();       // Stop WiFi, ntripClient will start as needed.
            bluetoothStart(); // Turn on Bluetooth with 'Rover' name
            radioStart();     // Start internal radio if enabled, otherwise disable

            tasksStartUART2(); // Start monitoring the UART1 from ZED for NMEA and UBX data (enables logging)

            settings.updateZEDSettings = false; // On the next boot, no need to update the ZED on this profile
            settings.lastState = STATE_ROVER_NOT_STARTED;
            recordSystemSettings(); // Record this state for next POR


            ntripClientStart();
            changeState(STATE_ROVER_NO_FIX);

            firstRoverStart = false; // Do not allow entry into test menu again
        }
        break;

        case (STATE_ROVER_NO_FIX): {
            if (fixType == 3 || fixType == 4) // 3D, 3D+DR
                changeState(STATE_ROVER_FIX);
        }
        break;

        case (STATE_ROVER_FIX): {
            updateAccuracyLEDs();

            if (carrSoln == 1) // RTK Float
                changeState(STATE_ROVER_RTK_FLOAT);
            else if (carrSoln == 2) // RTK Fix
                changeState(STATE_ROVER_RTK_FIX);
        }
        break;

        case (STATE_ROVER_RTK_FLOAT): {
            updateAccuracyLEDs();

            if (carrSoln == 0) // No RTK
                changeState(STATE_ROVER_FIX);
            if (carrSoln == 2) // RTK Fix
                changeState(STATE_ROVER_RTK_FIX);
        }
        break;

        case (STATE_ROVER_RTK_FIX): {
            updateAccuracyLEDs();

            if (carrSoln == 0) // No RTK
                changeState(STATE_ROVER_FIX);
            if (carrSoln == 1) // RTK Float
                changeState(STATE_ROVER_RTK_FLOAT);
        }
        break;

        case (STATE_BUBBLE_LEVEL): {
            // Do nothing - display only
        }
        break;

        case (STATE_PROFILE): {
            // Do nothing - display only
        }
        break;

        

        case (STATE_DISPLAY_SETUP): {
            if (millis() - lastSetupMenuChange > 1500)
            {
                forceSystemStateUpdate = true; // Imediately go to this new state
                changeState(setupState);       // Change to last setup state
            }
        }
        break;

        case (STATE_WIFI_CONFIG_NOT_STARTED): {
            


            bluetoothStop();
            espnowStop();

            tasksStopUART2(); // Delete F9 serial tasks if running
            startWebServer(); // Start in AP mode and show config html page

            changeState(STATE_WIFI_CONFIG);
        }
        break;

        case (STATE_WIFI_CONFIG): {
            if (incomingSettingsSpot > 0)
            {
                // Allow for 750ms before we parse buffer for all data to arrive
                if (millis() - timeSinceLastIncomingSetting > 750)
                {
                    currentlyParsingData =
                        true; // Disallow new data to flow from websocket while we are parsing the current data

                    systemPrint("Parsing: ");
                    for (int x = 0; x < incomingSettingsSpot; x++)
                        systemWrite(incomingSettings[x]);
                    systemPrintln();

                    parseIncomingSettings();
                    settings.updateZEDSettings =
                        true;               // When this profile is loaded next, force system to update ZED settings.
                    recordSystemSettings(); // Record these settings to unit

                    // Clear buffer
                    incomingSettingsSpot = 0;
                    memset(incomingSettings, 0, AP_CONFIG_SETTING_SIZE);

                    currentlyParsingData = false; // Allow new data from websocket
                }
            }

#ifdef COMPILE_WIFI
#ifdef COMPILE_AP
            // Dynamically update the coordinates on the AP page
            if (websocketConnected == true)
            {
                if (millis() - lastDynamicDataUpdate > 1000)
                {
                    lastDynamicDataUpdate = millis();
                    createDynamicDataString(settingsCSV);

                    // log_d("Sending coordinates: %s", settingsCSV);
                    websocket->textAll(settingsCSV);
                }
            }
#endif // COMPILE_AP
#endif // COMPILE_WIFI
        }
        break;

        // Setup device for testing
        case (STATE_TEST): {
            // Debounce entry into test menu
            if (millis() - lastTestMenuChange > 500)
            {
                tasksStopUART2(); // Stop absoring ZED serial via task
                zedUartPassed = false;

                // Enable RTCM 1230. This is the GLONASS bias sentence and is transmitted
                // even if there is no GPS fix. We use it to test serial output.
                theGNSS.newCfgValset(); // Create a new Configuration Item VALSET message
                theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_UART2, 1); // Enable message 1230 every second
                theGNSS.sendCfgValset();                                          // Send the VALSET

                changeState(STATE_TESTING);
            }
        }
        break;

        // Display testing screen - do nothing
        case (STATE_TESTING): {
            // Exit via button press task
        }
        break;

        case (STATE_ESPNOW_PAIRING_NOT_STARTED): {
#ifdef COMPILE_ESPNOW

            // Start ESP-Now if needed, put ESP-Now into broadcast state
            espnowBeginPairing();

            changeState(STATE_ESPNOW_PAIRING);
#else  // COMPILE_ESPNOW
            changeState(STATE_ROVER_NOT_STARTED);
#endif // COMPILE_ESPNOW
        }
        break;

        case (STATE_ESPNOW_PAIRING): {
            if (espnowIsPaired() == true)
            {

                // Return to the previous state
                changeState(lastSystemState);
            }
            else
            {
                uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                espnowSendPairMessage(broadcastMac); // Send unit's MAC address over broadcast, no ack, no encryption
            }
        }
        break;

        case (STATE_SHUTDOWN): {
            forceDisplayUpdate = true;
            powerDown(true);
        }
        break;

        default: {
            systemPrintf("Unknown state: %d\r\n", systemState);
        }
        break;
        }
    }
}

// System state changes may only occur within main state machine
// To allow state changes from external sources (ie, Button Tasks) requests can be made
// Requests are handled at the start of updateSystemState()
void requestChangeState(SystemState requestedState)
{
    newSystemStateRequested = true;
    requestedSystemState = requestedState;
    log_d("Requested System State: %d", requestedSystemState);
}

// Change states and print the new state
void changeState(SystemState newState)
{
    // Log the heap size at the state change
    reportHeapNow();

    // Debug print of new state, add leading asterisk for repeated states
    if ((!settings.enablePrintDuplicateStates) && (newState == systemState))
        return;

    if (settings.enablePrintStates && (newState == systemState))
        systemPrint("*");

    // Set the new state
    systemState = newState;

    if (settings.enablePrintStates)
    {
        switch (systemState)
        {
        case (STATE_ROVER_NOT_STARTED):
            systemPrint("State: Rover - Not Started");
            break;
        case (STATE_ROVER_NO_FIX):
            systemPrint("State: Rover - No Fix");
            break;
        case (STATE_ROVER_FIX):
            systemPrint("State: Rover - Fix");
            break;
        case (STATE_ROVER_RTK_FLOAT):
            systemPrint("State: Rover - RTK Float");
            break;
        case (STATE_ROVER_RTK_FIX):
            systemPrint("State: Rover - RTK Fix");
            break;
        case (STATE_BUBBLE_LEVEL):
            systemPrint("State: Bubble level");
            break;
        case (STATE_MARK_EVENT):
            systemPrint("State: Mark Event");
            break;
        case (STATE_DISPLAY_SETUP):
            systemPrint("State: Display Setup");
            break;
        case (STATE_WIFI_CONFIG_NOT_STARTED):
            systemPrint("State: WiFi Config Not Started");
            break;
        case (STATE_WIFI_CONFIG):
            systemPrint("State: WiFi Config");
            break;
        case (STATE_TEST):
            systemPrint("State: System Test Setup");
            break;
        case (STATE_TESTING):
            systemPrint("State: System Testing");
            break;
        case (STATE_PROFILE):
            systemPrint("State: Profile");
            break;

        case (STATE_ESPNOW_PAIRING_NOT_STARTED):
            systemPrint("State: ESP-Now Pairing Not Started");
            break;
        case (STATE_ESPNOW_PAIRING):
            systemPrint("State: ESP-Now Pairing");
            break;

        case (STATE_SHUTDOWN):
            systemPrint("State: Shut Down");
            break;
        case (STATE_NOT_SET):
            systemPrint("State: Not Set");
            break;
        default:
            systemPrintf("Change State Unknown: %d", systemState);
            break;
        }

        if (online.rtc)
        {
            // Timestamp the state change
            //          1         2
            // 12345678901234567890123456
            // YYYY-mm-dd HH:MM:SS.xxxrn0
            struct tm timeinfo = rtc.getTimeStruct();
            char s[30];
            strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", &timeinfo);
            systemPrintf(", %s.%03ld", s, rtc.getMillis());
        }

        systemPrintln();
    }
}
