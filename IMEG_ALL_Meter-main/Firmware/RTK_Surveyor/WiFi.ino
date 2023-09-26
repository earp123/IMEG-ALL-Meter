/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  WiFi Status Values:
    WL_CONNECTED: assigned when connected to a WiFi network
    WL_CONNECTION_LOST: assigned when the connection is lost
    WL_CONNECT_FAILED: assigned when the connection fails for all the attempts
    WL_DISCONNECTED: assigned when disconnected from a network
    WL_IDLE_STATUS: it is a temporary status assigned when WiFi.begin() is called and
                    remains active until the number of attempts expires (resulting in
                    WL_CONNECT_FAILED) or a connection is established (resulting in
                    WL_CONNECTED)
    WL_NO_SHIELD: assigned when no WiFi shield is present
    WL_NO_SSID_AVAIL: assigned when no SSID are available
    WL_SCAN_COMPLETED: assigned when the scan networks is completed

  WiFi Station States:

                                  WIFI_OFF<--------------------.
                                    |                          |
                       wifiStart()  |                          |
                                    |                          | WL_CONNECT_FAILED (Bad password)
                                    |                          | WL_NO_SSID_AVAIL (Out of range)
                                    v                 Fail     |
                                  WIFI_CONNECTING------------->+
                                    |    ^                     ^
                     wifiConnect()  |    |                     | wifiStop()
                                    |    | WL_CONNECTION_LOST  |
                                    |    | WL_DISCONNECTED     |
                                    v    |                     |
                                  WIFI_CONNECTED --------------'
  =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

//----------------------------------------
// Constants
//----------------------------------------

// Interval to use when displaying the IP address
static const int WIFI_IP_ADDRESS_DISPLAY_INTERVAL = 12 * 1000; // Milliseconds

#define WIFI_MAX_TCP_CLIENTS 4

// Throttle the time between connection attempts
// ms - Max of 4,294,967,295 or 4.3M seconds or 71,000 minutes or 1193 hours or 49 days between attempts
static int wifiConnectionAttempts = 0;       // Count the number of connection attempts between restarts
static uint32_t wifiConnectionAttemptsTotal; // Count the number of connection attempts absolutely
static uint32_t wifiConnectionAttemptTimeout = 0;

//----------------------------------------
// Locals - compiled out
//----------------------------------------

#ifdef COMPILE_WIFI

static uint32_t wifiLastConnectionAttempt = 0;

// WiFi Timer usage:
//  * Measure interval to display IP address
static unsigned long wifiDisplayTimer = 0;

// Last time the WiFi state was displayed
static uint32_t lastWifiState = 0;

// TCP server
static WiFiServer *wifiTcpServer = nullptr;
static WiFiClient wifiTcpClient[WIFI_MAX_TCP_CLIENTS];

//----------------------------------------
// WiFi Routines - compiled out
//----------------------------------------

void wifiDisplayIpAddress()
{
    systemPrint("WiFi IP address: ");
    systemPrint(WiFi.localIP());
    systemPrintf(" RSSI: %d\r\n", WiFi.RSSI());

    wifiDisplayTimer = millis();
}

IPAddress wifiGetIpAddress()
{
    return WiFi.localIP();
}

byte wifiGetStatus()
{
    return WiFi.status();
}

void wifiPeriodicallyDisplayIpAddress()
{
    if (settings.enablePrintWifiIpAddress && (wifiGetStatus() == WL_CONNECTED))
        if ((millis() - wifiDisplayTimer) >= WIFI_IP_ADDRESS_DISPLAY_INTERVAL)
            wifiDisplayIpAddress();
}

// Update the state of the WiFi state machine
void wifiSetState(byte newState)
{
    if (wifiState == newState)
        systemPrint("*");
    wifiState = newState;

    switch (newState)
    {
    default:
        systemPrintf("Unknown WiFi state: %d\r\n", newState);
        break;
    case WIFI_OFF:
        systemPrintln("WIFI_OFF");
        break;
    case WIFI_START:
        systemPrintln("WIFI_START");
        break;
    case WIFI_CONNECTING:
        systemPrintln("WIFI_CONNECTING");
        break;
    case WIFI_CONNECTED:
        systemPrintln("WIFI_CONNECTED");
        break;
    }
}

//----------------------------------------
// WiFi Config Support Routines - compiled out
//----------------------------------------

// Start the access point for user to connect to and configure device
// We can also start as a WiFi station and attempt to connect to local WiFi for config
bool wifiStartAP()
{
    if (settings.wifiConfigOverAP == true)
    {
        // Stop any current WiFi activity
        wifiStop();

        // Start in AP mode
        WiFi.mode(WIFI_AP);

        // Before starting AP mode, be sure we have default WiFi protocols enabled.
        // esp_wifi_set_protocol requires WiFi to be started
        esp_err_t response =
            esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);//SWR changed to WIFI_IF_AP
        if (response != ESP_OK)
            systemPrintf("wifiStartAP: Error setting WiFi protocols: %s\r\n", esp_err_to_name(response));
        else
            log_d("WiFi protocols set");

        IPAddress local_IP(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);

        WiFi.softAPConfig(local_IP, gateway, subnet);
        if (WiFi.softAP("IMEG ALL Meter") == false) 
        {
            systemPrintln("WiFi AP failed to start");
            return (false);
        }
        systemPrint("WiFi AP Started with IP: ");
        systemPrintln(WiFi.softAPIP());
    }
    else
    {
        // Start webserver on local WiFi instead of AP

        // Attempt to connect to local WiFi with increasing timeouts
        int timeout = 0;
        int x = 0;
        const int maxTries = 2;
        for (; x < maxTries; x++)
        {
            timeout += 5000;
            if (wifiConnect(timeout) == true) // Attempt to connect to any SSID on settings list
            {
                wifiPrintNetworkInfo();
                break;
            }
        }
        if (x == maxTries)
        {
          requestChangeState(STATE_ROVER_NOT_STARTED); // If WiFi failed, return to Rover mode.
          return (false);
        }
    }

    return (true);
}

#endif // COMPILE_WIFI

//----------------------------------------
// Global WiFi Routines
//----------------------------------------

// Advance the WiFi state from off to connected
// Throttle connection attempts as needed
void wifiUpdate()
{
    // Skip if in configure-via-ethernet mode
    if (configureViaEthernet)
    {
        // log_d("configureViaEthernet: skipping wifiUpdate");
        return;
    }

#ifdef COMPILE_WIFI

    // Periodically display the WiFi state
    if (settings.enablePrintWifiState && ((millis() - lastWifiState) > 15000))
    {
        wifiSetState(wifiState);
        lastWifiState = millis();
    }

    switch (wifiState)
    {
    default:
        systemPrintf("Unknown wifiState: %d\r\n", wifiState);
        break;

    case WIFI_OFF:
        // Any service that needs WiFi will call wifiStart()
        break;

    case WIFI_CONNECTING:
        // Pause until connection timeout has passed
        if (millis() - wifiLastConnectionAttempt > wifiConnectionAttemptTimeout)
        {
            wifiLastConnectionAttempt = millis();

            if (wifiConnect(10000) == true) // Attempt to connect to any SSID on settings list
            {
                if (espnowState > ESPNOW_OFF)
                    espnowStart();

                wifiSetState(WIFI_CONNECTED);
            }
            else
            {
                // We failed to connect
                if (wifiConnectLimitReached() == false) // Increases wifiConnectionAttemptTimeout
                {
                    if (wifiConnectionAttemptTimeout / 1000 < 120)
                        systemPrintf("Next WiFi attempt in %d seconds.\r\n", wifiConnectionAttemptTimeout / 1000);
                    else
                        systemPrintf("Next WiFi attempt in %d minutes.\r\n", wifiConnectionAttemptTimeout / 1000 / 60);
                }
                else
                {
                    systemPrintln("WiFi connection failed. Giving up.");
                    wifiStop(); // Move back to WIFI_OFF
                }
            }
        }

        break;

    case WIFI_CONNECTED:
        // Verify link is still up
        if (wifiIsConnected() == false)
        {
            systemPrintln("WiFi link lost");
            wifiConnectionAttempts = 0; // Reset the timeout
            wifiSetState(WIFI_CONNECTING);
        }
        else
        {
            wifiPeriodicallyDisplayIpAddress(); // Periodically display the IP address

            // If WiFi is connected, and no services require WiFi, shut it off
            if (wifiIsNeeded() == false)
                wifiStop();
        }
        break;
    }

#endif // COMPILE_WIFI
}

// Starts the WiFi connection state machine (moves from WIFI_OFF to WIFI_CONNECTING)
// Sets the appropriate protocols (WiFi + ESP-Now)
// If radio is off entirely, start WiFi
// If ESP-Now is active, only add the LR protocol
void wifiStart()
{
#ifdef COMPILE_WIFI
    if (wifiNetworkCount() == 0)
    {
        systemPrintln("Error: Please enter at least one SSID before using WiFi");
        // paintNoWiFi(2000); //TODO
        wifiStop();
        return;
    }

    if (wifiIsConnected() == true)
        return; // We don't need to do anything

    if (wifiState > WIFI_OFF)
        return; // We're in the midst of connecting

    log_d("Starting WiFi");

    wifiSetState(WIFI_CONNECTING); // This starts the state machine running

    // Display the heap state
    reportHeapNow();
#endif // COMPILE_WIFI
}

// Stop WiFi and release all resources
// If ESP NOW is active, leave WiFi on enough for ESP NOW
void wifiStop()
{
#ifdef COMPILE_WIFI
    stopWebServer();

    // Shutdown the TCP client
    if (online.tcpClient)
    {
        // Tell the UART2 tasks that the TCP client is shutting down
        online.tcpClient = false;
        delay(5);
        systemPrintln("TCP client offline");
    }

    // Shutdown the TCP server connection
    if (online.tcpServer)
    {
        // Tell the UART2 tasks that the TCP server is shutting down
        online.tcpServer = false;

        // Wait for the UART2 tasks to close the TCP client connections
        while (wifiTcpServerActive())
            delay(5);
        systemPrintln("TCP Server offline");
    }

    if (settings.mdnsEnable == true)
        MDNS.end();

    wifiSetState(WIFI_OFF);

    wifiConnectionAttempts = 0; // Reset the timeout

    // If ESP-Now is active, change protocol to only Long Range and re-start WiFi
    if (espnowState > ESPNOW_OFF)
    {
        if (WiFi.getMode() == WIFI_OFF)
            WiFi.mode(WIFI_STA);

        // Enable long range, PHY rate of ESP32 will be 512Kbps or 256Kbps
        // esp_wifi_set_protocol requires WiFi to be started
        esp_err_t response = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
        if (response != ESP_OK)
            systemPrintf("wifiStop: Error setting ESP-Now lone protocol: %s\r\n", esp_err_to_name(response));
        else
            log_d("WiFi disabled, ESP-Now left in place");
    }
    else
    {
        WiFi.mode(WIFI_OFF);
        log_d("WiFi Stopped");
    }

    // Display the heap state
    reportHeapNow();
#endif // COMPILE_WIFI
}

bool wifiIsConnected()
{
#ifdef COMPILE_WIFI
    bool isConnected = (wifiGetStatus() == WL_CONNECTED);
    if (isConnected)
        wifiPeriodicallyDisplayIpAddress();

    return isConnected;
#else   // COMPILE_WIFI
    return false;
#endif  // COMPILE_WIFI
}

// Attempts a connection to all provided SSIDs
// Returns true if successful
// Gives up if no SSID detected or connection times out
bool wifiConnect(unsigned long timeout)
{
#ifdef COMPILE_WIFI

    if (wifiIsConnected())
        return (true); // Nothing to do

    // Before we can issue esp_wifi_() commands WiFi must be started
    if (WiFi.getMode() == WIFI_OFF)
        WiFi.mode(WIFI_STA);

    // Verify that the necessary protocols are set
    uint8_t protocols = 0;
    esp_err_t response = esp_wifi_get_protocol(WIFI_IF_STA, &protocols);
    if (response != ESP_OK)
        systemPrintf("wifiConnect: Failed to get protocols: %s\r\n", esp_err_to_name(response));

    // If ESP-NOW is running, blend in ESP-NOW protocol.
    if (espnowState > ESPNOW_OFF)
    {
        if (protocols != (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR))
        {
            esp_err_t response =
                esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N |
                                                       WIFI_PROTOCOL_LR); // Enable WiFi + ESP-Now.
            if (response != ESP_OK)
                systemPrintf("wifiConnect: Error setting WiFi + ESP-NOW protocols: %s\r\n", esp_err_to_name(response));
        }
    }
    else
    {
        // Make sure default WiFi protocols are in place
        if (protocols != (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N))
        {
            esp_err_t response = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G |
                                                                        WIFI_PROTOCOL_11N); // Enable WiFi.
            if (response != ESP_OK)
                systemPrintf("wifiConnect: Error setting WiFi + ESP-NOW protocols: %s\r\n", esp_err_to_name(response));
        }
    }

    WiFiMulti wifiMulti;

    // Load SSIDs
    for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
    {
        if (strlen(settings.wifiNetworks[x].ssid) > 0)
            wifiMulti.addAP(settings.wifiNetworks[x].ssid, settings.wifiNetworks[x].password);
    }

    systemPrint("Connecting WiFi... ");

    int wifiResponse = wifiMulti.run(timeout);
    if (wifiResponse == WL_CONNECTED)
    {
        if (settings.enableTcpClient == true || settings.enableTcpServer == true)
        {
            if (settings.mdnsEnable == true)
            {
                if (MDNS.begin("rtk") == false) // This should make the module findable from 'rtk.local' in browser
                    log_d("Error setting up MDNS responder!");
                else
                    MDNS.addService("http", "tcp", settings.wifiTcpPort); // Add service to MDNS
            }
        }

        systemPrintln();
        return true;
    }
    else if (wifiResponse == WL_DISCONNECTED)
        systemPrint("No friendly WiFi networks detected. ");
    else
        systemPrintf("WiFi failed to connect: error #%d. ", wifiResponse);

#endif  // COMPILE_WIFI

    return false;
}

// Based on the current settings and system states, determine if we need WiFi on or not
// This function does not start WiFi. Any service that needs it should call wifiStart().
// This function is used to turn WiFi off if nothing needs it.
bool wifiIsNeeded()
{
    bool needed = false;

    if (settings.enableTcpClient == true)
        needed = true;
    if (settings.enableTcpServer == true)
        needed = true;

    // Handle WiFi within systemStates
    if (systemState <= STATE_ROVER_RTK_FIX && settings.enableNtripClient == true)
        needed = true;




    // If WiFi is on while we are in the following states, allow WiFi to continue to operate
    if (systemState >= STATE_BUBBLE_LEVEL && systemState <= STATE_PROFILE)
    {
        // Keep WiFi on if user presses setup button, enters bubble level, is in AP config mode, etc
        needed = true;
    }



    return needed;
}

// Counts the number of entered SSIDs
int wifiNetworkCount()
{
    // Count SSIDs
    int networkCount = 0;
    for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
    {
        if (strlen(settings.wifiNetworks[x].ssid) > 0)
            networkCount++;
    }
    return networkCount;
}

// Send data to the TCP clients
void wifiSendTcpData(uint8_t *data, uint16_t length)
{
#ifdef COMPILE_WIFI

    if (!length)
        return;

    // Send the data to the connected clients
    if ((settings.enableTcpServer && online.tcpServer) || (settings.enableTcpClient && online.tcpClient))
    {
        // Walk the list of TCP clients
        for (int index = 0; index < WIFI_MAX_TCP_CLIENTS; index++)
        {
            if (wifiTcpConnected & (1 << index))
            {
                if (wifiTcpClient[index].write(data, length) == length)
                {
                    if ((settings.enablePrintTcpStatus) && (!inMainMenu))
                        systemPrintf("%d bytes written over WiFi TCP\r\n", length);
                }
                // Failed to write the data
                else
                {
                    // Done with this client connection
                    if (!inMainMenu)
                    {
                        systemPrintf("Breaking WiFi TCP client %d connection\r\n", index);
                    }

                    wifiTcpClient[index].stop();
                    wifiTcpConnected &= ~(1 << index);

                    // Shutdown the TCP server if necessary
                    if (settings.enableTcpServer || online.tcpServer)
                        wifiTcpServerActive();
                }
            }
        }
    }
#endif // COMPILE_WIFI
}

// Check for TCP server active
bool wifiTcpServerActive()
{
#ifdef COMPILE_WIFI
    if ((settings.enableTcpServer && online.tcpServer) || wifiTcpConnected)
        return true;

    log_d("Stopping WiFi TCP Server");

    // Shutdown the TCP server
    online.tcpServer = false;

    // Stop the TCP server
    wifiTcpServer->stop();

    if (wifiTcpServer != nullptr)
        delete wifiTcpServer;
#endif // COMPILE_WIFI
    return false;
}

// Determine if another connection is possible or if the limit has been reached
bool wifiConnectLimitReached()
{
    // Retry the connection a few times
    bool limitReached = false;
    if (wifiConnectionAttempts++ >= wifiMaxConnectionAttempts)
        limitReached = true;

    wifiConnectionAttemptsTotal++;

    if (limitReached == false)
    {
        wifiConnectionAttemptTimeout =
            wifiConnectionAttempts * 15 * 1000L; // Wait 15, 30, 45, etc seconds between attempts

        reportHeapNow();
    }
    else
    {
        // No more connection attempts
        systemPrintln("WiFi connection attempts exceeded!");
    }
    return limitReached;
}

void tcpUpdate()
{
    // Skip if in configure-via-ethernet mode
    if (configureViaEthernet)
    {
        // log_d("configureViaEthernet: skipping tcpUpdate");
        return;
    }

#ifdef COMPILE_WIFI

    static IPAddress ipAddress[WIFI_MAX_TCP_CLIENTS];
    static uint32_t lastTcpConnectAttempt;

    if (settings.enableTcpClient == false && settings.enableTcpServer == false)
        return; // Nothing to do

    if (wifiInConfigMode())
        return; // Do not service TCP during WiFi config

    // If TCP is enabled, but WiFi is not connected, start WiFi
    if (wifiIsConnected() == false && (settings.enableTcpClient == true || settings.enableTcpServer == true))
    {
        // Verify WIFI_MAX_TCP_CLIENTS
        if ((sizeof(wifiTcpConnected) * 8) < WIFI_MAX_TCP_CLIENTS)
        {
            systemPrintf("Please set WIFI_MAX_TCP_CLIENTS <= %d or increase size of wifiTcpConnected\r\n",
                         sizeof(wifiTcpConnected) * 8);
            while (true)
                ; // Freeze
        }

        log_d("tpcUpdate starting WiFi");
        wifiStart();
    }

    // Start the TCP client if enabled
    if (settings.enableTcpClient && (!online.tcpClient) && (!settings.enableTcpServer) && wifiIsConnected())
    {
        online.tcpClient = true;
        systemPrint("WiFi TCP client online, local IP ");
        systemPrint(WiFi.localIP());
        systemPrint(", gateway IP ");
        systemPrintln(WiFi.gatewayIP());
    }

    // Start the TCP server if enabled
    if (settings.enableTcpServer && (wifiTcpServer == nullptr) && (!settings.enableTcpClient) && wifiIsConnected())
    {
        wifiTcpServer = new WiFiServer(settings.wifiTcpPort);

        wifiTcpServer->begin();
        online.tcpServer = true;
        systemPrint("WiFi TCP Server online, IP Address ");
        systemPrintln(WiFi.localIP());
    }

    int index = 0;

    // Connect the TCP client if enabled
    if (online.tcpClient)
    {
        if (((!wifiTcpClient[0]) || (!wifiTcpClient[0].connected())) && ((millis() - lastTcpConnectAttempt) >= 1000))
        {
            lastTcpConnectAttempt = millis();
            ipAddress[0] = WiFi.gatewayIP();
            if (settings.enablePrintTcpStatus)
            {
                systemPrint("Trying to connect WiFi TCP client to ");
                systemPrintln(ipAddress[0]);
            }
            if (wifiTcpClient[0].connect(ipAddress[0], settings.wifiTcpPort))
            {
                online.tcpClient = true;
                systemPrint("WiFi TCP client connected to ");
                systemPrintln(ipAddress[0]);
                wifiTcpConnected |= 1 << index;
            }
            else
            {
                // Release any allocated resources
                // if (wifiTcpClient[0])
                wifiTcpClient[0].stop();
            }
        }
    }

    // Check for another client
    if (online.tcpServer)
    {
        for (index = 0; index < WIFI_MAX_TCP_CLIENTS; index++)
            if (!(wifiTcpConnected & (1 << index)))
            {
                if ((!wifiTcpClient[index]) || (!wifiTcpClient[index].connected()))
                {
                    wifiTcpClient[index] = wifiTcpServer->available();
                    if (!wifiTcpClient[index])
                        break;
                    ipAddress[index] = wifiTcpClient[index].remoteIP();
                    systemPrintf("Connected WiFi TCP client %d to ", index);
                    systemPrintln(ipAddress[index]);
                    wifiTcpConnected |= 1 << index;
                }
            }
    }

    // Walk the list of TCP clients
    for (index = 0; index < WIFI_MAX_TCP_CLIENTS; index++)
    {
        if (wifiTcpConnected & (1 << index))
        {
            // Check for a broken connection
            if ((!wifiTcpClient[index]) || (!wifiTcpClient[index].connected()))
            {
                // Done with this client connection
                if (!inMainMenu)
                {
                    systemPrintf("Disconnected TCP client %d from ", index);
                    systemPrintln(ipAddress[index]);
                }

                wifiTcpClient[index].stop();
                wifiTcpConnected &= ~(1 << index);

                // Shutdown the TCP server if necessary
                if (settings.enableTcpServer || online.tcpServer)
                    wifiTcpServerActive();
            }
        }
    }

#endif  // COMPILE_WIFI
}

void wifiPrintNetworkInfo()
{
#ifdef COMPILE_WIFI
    systemPrintln("\nNetwork Configuration:");
    systemPrintln("----------------------");
    systemPrint("         SSID: ");
    systemPrintln(WiFi.SSID());
    systemPrint("  WiFi Status: ");
    systemPrintln(WiFi.status());
    systemPrint("WiFi Strength: ");
    systemPrint(WiFi.RSSI());
    systemPrintln(" dBm");
    systemPrint("          MAC: ");
    systemPrintln(WiFi.macAddress());
    systemPrint("           IP: ");
    systemPrintln(WiFi.localIP());
    systemPrint("       Subnet: ");
    systemPrintln(WiFi.subnetMask());
    systemPrint("      Gateway: ");
    systemPrintln(WiFi.gatewayIP());
    systemPrint("        DNS 1: ");
    systemPrintln(WiFi.dnsIP(0));
    systemPrint("        DNS 2: ");
    systemPrintln(WiFi.dnsIP(1));
    systemPrint("        DNS 3: ");
    systemPrintln(WiFi.dnsIP(2));
    systemPrintln();
#endif  // COMPILE_WIFI
}

// Returns true if unit is in config mode
// Used to disallow services (NTRIP, TCP, etc) from updating
bool wifiInConfigMode()
{
    if (systemState >= STATE_WIFI_CONFIG_NOT_STARTED && systemState <= STATE_WIFI_CONFIG)
        return true;
    return false;
}
