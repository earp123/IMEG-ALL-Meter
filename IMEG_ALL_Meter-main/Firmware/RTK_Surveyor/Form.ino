// Once connected to the access point for WiFi Config, the ESP32 sends current setting values in one long string to
// websocket After user clicks 'save', data is validated via main.js and a long string of values is returned.

bool websocketConnected = false;

#ifdef COMPILE_WEBSERVER
// Start webserver in AP mode
void startWebServer(bool startWiFi = true, int httpPort = 80); // Header
void startWebServer(bool startWiFi, int httpPort)
{
#ifdef COMPILE_WIFI
#ifdef COMPILE_AP

    ntripClientStop(true); // Do not allocate new wifiClient
    

    if (startWiFi)
        if (wifiStartAP() == false) // Exits calling wifiConnect()
            return;
/*
    if (settings.mdnsEnable == true)
    {
        if (MDNS.begin("rtk") == false) // This should make the module findable from 'rtk.local' in browser
            log_d("Error setting up MDNS responder!");
        else
            MDNS.addService("http", "tcp", 80); // Add service to MDNS-SD
    }
*/
    incomingSettings = (char *)malloc(AP_CONFIG_SETTING_SIZE);
    memset(incomingSettings, 0, AP_CONFIG_SETTING_SIZE);

    // Pre-load settings CSV
    settingsCSV = (char *)malloc(AP_CONFIG_SETTING_SIZE);
    

    webserver = new AsyncWebServer(httpPort);
    websocket = new AsyncWebSocket("/ws");

    websocket->onEvent(onWsEvent);
    webserver->addHandler(websocket);

    // * index.html (not gz'd)
    // * favicon.ico

    // * /src/bootstrap.bundle.min.js - Needed for popper
    // * /src/bootstrap.min.css
    // * /src/bootstrap.min.js
    // * /src/jquery-3.6.0.min.js
    // * /src/main.js (not gz'd)
    // * /src/rtk-setup.png
    // * /src/style.css

    // * /src/fonts/icomoon.eot
    // * /src/fonts/icomoon.svg
    // * /src/fonts/icomoon.ttf
    // * /src/fonts/icomoon.woof

    // * /listfiles responds with a CSV of files and sizes in root
    // * /listMessages responds with a CSV of messages supported by this platform
    // * /listMessagesBase responds with a CSV of RTCM Base messages supported by this platform
    // * /file allows the download or deletion of a file

    webserver->onNotFound(notFound);

    
    
    webserver->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html, sizeof(index_html));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    webserver->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "text/plain", favicon_ico, sizeof(favicon_ico));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    webserver->on("/src/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "text/javascript", bootstrap_bundle_min_js, sizeof(bootstrap_bundle_min_js));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    webserver->on("/src/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "text/css", bootstrap_min_css, sizeof(bootstrap_min_css));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    webserver->on("/src/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "text/javascript", bootstrap_min_js, sizeof(bootstrap_min_js));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    webserver->on("/src/jquery-3.6.0.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "text/javascript", jquery_js, sizeof(jquery_js));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    webserver->on("/src/main.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", main_js, sizeof(main_js));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    webserver->on("/src/rtk-setup.png", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response;
        response = request->beginResponse_P(200, "image/png", rtkSetupWiFi_png, sizeof(rtkSetupWiFi_png));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    // Battery icons
    webserver->on("/src/BatteryBlank.png", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "image/png", batteryBlank_png, sizeof(batteryBlank_png));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    webserver->on("/src/Battery0.png", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "image/png", battery0_png, sizeof(battery0_png));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    webserver->on("/src/Battery1.png", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "image/png", battery1_png, sizeof(battery1_png));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    webserver->on("/src/Battery2.png", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "image/png", battery2_png, sizeof(battery2_png));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    webserver->on("/src/Battery3.png", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "image/png", battery3_png, sizeof(battery3_png));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    webserver->on("/src/Battery0_Charging.png", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "image/png", battery0_Charging_png, sizeof(battery0_Charging_png));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    webserver->on("/src/Battery1_Charging.png", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "image/png", battery1_Charging_png, sizeof(battery1_Charging_png));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    webserver->on("/src/Battery2_Charging.png", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "image/png", battery2_Charging_png, sizeof(battery2_Charging_png));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    webserver->on("/src/Battery3_Charging.png", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "image/png", battery3_Charging_png, sizeof(battery3_Charging_png));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    webserver->on("/src/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", style_css, sizeof(style_css));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    webserver->on("/src/fonts/icomoon.eot", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "text/plain", icomoon_eot, sizeof(icomoon_eot));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    webserver->on("/src/fonts/icomoon.svg", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "text/plain", icomoon_svg, sizeof(icomoon_svg));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    webserver->on("/src/fonts/icomoon.ttf", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "text/plain", icomoon_ttf, sizeof(icomoon_ttf));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    webserver->on("/src/fonts/icomoon.woof", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response =
            request->beginResponse_P(200, "text/plain", icomoon_woof, sizeof(icomoon_woof));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    



    // Handler for supported messages list
    webserver->on("/listMessages", HTTP_GET, [](AsyncWebServerRequest *request) {
        String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
        systemPrintln(logmessage);
        String messages;
        createMessageList(messages);
        request->send(200, "text/plain", messages);
    });



    

    webserver->begin();

    log_d("Web Server Started");
    reportHeapNow();

#endif // COMPILE_AP
#endif // COMPILE_WIFI
}

void stopWebServer()
{
#ifdef COMPILE_WIFI
#ifdef COMPILE_AP

    if (webserver != nullptr)
    {
        webserver->end();
        free(webserver);
        webserver = nullptr;

        if (websocket != nullptr)
        {
            delete websocket;
            websocket = nullptr;
        }

        if (settingsCSV != nullptr)
        {
            free(settingsCSV);
            settingsCSV = nullptr;
        }

        if (incomingSettings != nullptr)
        {
            free(incomingSettings);
            incomingSettings = nullptr;
        }
    }

    log_d("Web Server Stopped");
    reportHeapNow();

#endif  // COMPILE_AP
#endif  // COMPILE_WIFI
}

#ifdef COMPILE_WIFI
#ifdef COMPILE_AP
void notFound(AsyncWebServerRequest *request)
{
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    systemPrintln(logmessage);
    request->send(404, "text/plain", "Not found");
}
#endif  // COMPILE_AP
#endif  // COMPILE_WIFI



// Events triggered by web sockets
#ifdef COMPILE_WIFI
#ifdef COMPILE_AP

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data,
               size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        log_d("Websocket client connected");
        client->text(settingsCSV);
        lastDynamicDataUpdate = millis();
        websocketConnected = true;
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        log_d("Websocket client disconnected");

        // User has either refreshed the page or disconnected. Recompile the current settings.
        
        websocketConnected = false;
    }
    else if (type == WS_EVT_DATA)
    {
        if (currentlyParsingData == false)
        {
            for (int i = 0; i < len; i++)
            {
                incomingSettings[incomingSettingsSpot++] = data[i];
                incomingSettingsSpot %= AP_CONFIG_SETTING_SIZE;
            }
            timeSinceLastIncomingSetting = millis();
        }
    }
    else
        log_d("onWsEvent: unrecognised AwsEventType %d", type);
}
#endif  // COMPILE_AP
#endif  // COMPILE_WIFI



// Create a csv string with the dynamic data to update (current coordinates, battery level, etc)
void createDynamicDataString(char *settingsCSV)
{
#ifdef COMPILE_AP
    settingsCSV[0] = '\0'; // Erase current settings string

    // Current coordinates come from HPPOSLLH call back
    stringRecord(settingsCSV, "geodeticLat", latitude, haeNumberOfDecimals);
    stringRecord(settingsCSV, "geodeticLon", longitude, haeNumberOfDecimals);
    stringRecord(settingsCSV, "geodeticAlt", altitude, 3);

    double ecefX = 0;
    double ecefY = 0;
    double ecefZ = 0;

    geodeticToEcef(latitude, longitude, altitude, &ecefX, &ecefY, &ecefZ);

    stringRecord(settingsCSV, "ecefX", ecefX, 3);
    stringRecord(settingsCSV, "ecefY", ecefY, 3);
    stringRecord(settingsCSV, "ecefZ", ecefZ, 3);

    if (HAS_NO_BATTERY) // Ref Stn does not have a battery
    {
        stringRecord(settingsCSV, "batteryIconFileName", (char *)"src/BatteryBlank.png");
        stringRecord(settingsCSV, "batteryPercent", (char *)" ");
    }
    else
    {
        // Determine battery icon
        int iconLevel = 0;
        if (battLevel < 25)
            iconLevel = 0;
        else if (battLevel < 50)
            iconLevel = 1;
        else if (battLevel < 75)
            iconLevel = 2;
        else // batt level > 75
            iconLevel = 3;

        char batteryIconFileName[sizeof("src/Battery2_Charging.png__")]; // sizeof() includes 1 for \0 termination

        if (externalPowerConnected)
            snprintf(batteryIconFileName, sizeof(batteryIconFileName), "src/Battery%d_Charging.png", iconLevel);
        else
            snprintf(batteryIconFileName, sizeof(batteryIconFileName), "src/Battery%d.png", iconLevel);

        stringRecord(settingsCSV, "batteryIconFileName", batteryIconFileName);

        // Determine battery percent
        char batteryPercent[sizeof("+100%__")];
        if (externalPowerConnected)
            snprintf(batteryPercent, sizeof(batteryPercent), "+%d%%", battLevel);
        else
            snprintf(batteryPercent, sizeof(batteryPercent), "%d%%", battLevel);
        stringRecord(settingsCSV, "batteryPercent", batteryPercent);
    }

    strcat(settingsCSV, "\0");
#endif  // COMPILE_AP
}

// Given a settingName, and string value, update a given setting
void updateSettingWithValue(const char *settingName, const char *settingValueStr)
{
#ifdef COMPILE_AP
    char *ptr;
    double settingValue = strtod(settingValueStr, &ptr);

    bool settingValueBool = false;
    if (strcmp(settingValueStr, "true") == 0)
        settingValueBool = true;

    if (strcmp(settingName, "maxLogTime_minutes") == 0)
        settings.maxLogTime_minutes = settingValue;
    else if (strcmp(settingName, "maxLogLength_minutes") == 0)
        settings.maxLogLength_minutes = settingValue;
    else if (strcmp(settingName, "measurementRateHz") == 0)
    {
        settings.measurementRate = (int)(1000.0 / settingValue);

        // This is one of the first settings to be received. If seen, remove the station files.
        
        log_d("Station coordinate files removed");
    }
    else if (strcmp(settingName, "dynamicModel") == 0)
        settings.dynamicModel = settingValue;
    else if (strcmp(settingName, "baseTypeFixed") == 0)
        settings.fixedBase = settingValueBool;
    else if (strcmp(settingName, "observationSeconds") == 0)
        settings.observationSeconds = settingValue;
    else if (strcmp(settingName, "observationPositionAccuracy") == 0)
        settings.observationPositionAccuracy = settingValue;
    else if (strcmp(settingName, "fixedBaseCoordinateTypeECEF") == 0)
        settings.fixedBaseCoordinateType =
            !settingValueBool; // When ECEF is true, fixedBaseCoordinateType = 0 (COORD_TYPE_ECEF)
    else if (strcmp(settingName, "fixedEcefX") == 0)
        settings.fixedEcefX = settingValue;
    else if (strcmp(settingName, "fixedEcefY") == 0)
        settings.fixedEcefY = settingValue;
    else if (strcmp(settingName, "fixedEcefZ") == 0)
        settings.fixedEcefZ = settingValue;
    else if (strcmp(settingName, "fixedLatText") == 0)
    {
        double newCoordinate = 0.0;
        CoordinateInputType newCoordinateInputType =
            coordinateIdentifyInputType((char *)settingValueStr, &newCoordinate);
        if (newCoordinateInputType == COORDINATE_INPUT_TYPE_INVALID_UNKNOWN)
            settings.fixedLat = 0.0;
        else
        {
            settings.fixedLat = newCoordinate;
            settings.coordinateInputType = newCoordinateInputType;
        }
    }
    else if (strcmp(settingName, "fixedLongText") == 0)
    {
        double newCoordinate = 0.0;
        if (coordinateIdentifyInputType((char *)settingValueStr, &newCoordinate) ==
            COORDINATE_INPUT_TYPE_INVALID_UNKNOWN)
            settings.fixedLong = 0.0;
        else
            settings.fixedLong = newCoordinate;
    }
    else if (strcmp(settingName, "fixedAltitude") == 0)
        settings.fixedAltitude = settingValue;
    else if (strcmp(settingName, "dataPortBaud") == 0)
        settings.dataPortBaud = settingValue;
    else if (strcmp(settingName, "radioPortBaud") == 0)
        settings.radioPortBaud = settingValue;
    else if (strcmp(settingName, "enableUART2UBXIn") == 0)
        settings.enableUART2UBXIn = settingValueBool;
    else if (strcmp(settingName, "enableLogging") == 0)
        settings.enableLogging = settingValueBool;
    else if (strcmp(settingName, "enableARPLogging") == 0)
        settings.enableARPLogging = settingValueBool;
    else if (strcmp(settingName, "ARPLoggingInterval") == 0)
        settings.ARPLoggingInterval_s = settingValue;
    else if (strcmp(settingName, "dataPortChannel") == 0)
        settings.dataPortChannel = (muxConnectionType_e)settingValue;
    else if (strcmp(settingName, "autoIMUmountAlignment") == 0)
        settings.autoIMUmountAlignment = settingValueBool;
    else if (strcmp(settingName, "enableSensorFusion") == 0)
        settings.enableSensorFusion = settingValueBool;
    else if (strcmp(settingName, "enableResetDisplay") == 0)
        settings.enableResetDisplay = settingValueBool;

    else if (strcmp(settingName, "enableExternalPulse") == 0)
        settings.enableExternalPulse = settingValueBool;
    else if (strcmp(settingName, "externalPulseTimeBetweenPulse_us") == 0)
        settings.externalPulseTimeBetweenPulse_us = settingValue;
    else if (strcmp(settingName, "externalPulseLength_us") == 0)
        settings.externalPulseLength_us = settingValue;
    else if (strcmp(settingName, "externalPulsePolarity") == 0)
        settings.externalPulsePolarity = (pulseEdgeType_e)settingValue;
    else if (strcmp(settingName, "enableExternalHardwareEventLogging") == 0)
        settings.enableExternalHardwareEventLogging = settingValueBool;



    else if (strcmp(settingName, "serialTimeoutGNSS") == 0)
        settings.serialTimeoutGNSS = settingValue;
    else if (strcmp(settingName, "pointPerfectDeviceProfileToken") == 0)
        strcpy(settings.pointPerfectDeviceProfileToken, settingValueStr);
    else if (strcmp(settingName, "enablePointPerfectCorrections") == 0)
        settings.enablePointPerfectCorrections = settingValueBool;
    else if (strcmp(settingName, "autoKeyRenewal") == 0)
        settings.autoKeyRenewal = settingValueBool;
    else if (strcmp(settingName, "antennaHeight") == 0)
        settings.antennaHeight = settingValue;
    else if (strcmp(settingName, "antennaReferencePoint") == 0)
        settings.antennaReferencePoint = settingValue;
    else if (strcmp(settingName, "bluetoothRadioType") == 0)
        settings.bluetoothRadioType = (BluetoothRadioType_e)settingValue; // 0 = SPP, 1 = BLE, 2 = Off
    else if (strcmp(settingName, "espnowBroadcast") == 0)
        settings.espnowBroadcast = settingValueBool;
    else if (strcmp(settingName, "radioType") == 0)
        settings.radioType = (RadioType_e)settingValue; // 0 = Radio off, 1 = ESP-Now
    else if (strcmp(settingName, "baseRoverSetup") == 0)
    {
        // 0 = Rover, 1 = Base, 2 = NTP
        settings.lastState = STATE_ROVER_NOT_STARTED; // Default
        if (settingValue == 2)
            settings.lastState = STATE_NTPSERVER_NOT_STARTED;
    }
    
    else if (strcmp(settingName, "wifiTcpPort") == 0)
        settings.wifiTcpPort = settingValue;
    else if (strcmp(settingName, "wifiConfigOverAP") == 0)
    {
        if (settingValue == 1) // Drop downs come back as a value
            settings.wifiConfigOverAP = true;
        else
            settings.wifiConfigOverAP = false;
    }

    else if (strcmp(settingName, "enableTcpClient") == 0)
        settings.enableTcpClient = settingValueBool;
    else if (strcmp(settingName, "enableTcpServer") == 0)
        settings.enableTcpServer = settingValueBool;
    else if (strcmp(settingName, "enableRCFirmware") == 0)
        enableRCFirmware = settingValueBool;
    else if (strcmp(settingName, "minElev") == 0)
        settings.minElev = settingValue;
    else if (strcmp(settingName, "imuYaw") == 0)
        settings.imuYaw = settingValue * 100; // Comes in as 0 to 360.0 but stored as 0 to 36,000
    else if (strcmp(settingName, "imuPitch") == 0)
        settings.imuPitch = settingValue * 100; // Comes in as -90 to 90.0 but stored as -9000 to 9000
    else if (strcmp(settingName, "imuRoll") == 0)
        settings.imuRoll = settingValue * 100; // Comes in as -180 to 180.0 but stored as -18000 to 18000
    else if (strcmp(settingName, "sfDisableWheelDirection") == 0)
        settings.sfDisableWheelDirection = settingValueBool;
    else if (strcmp(settingName, "sfCombineWheelTicks") == 0)
        settings.sfCombineWheelTicks = settingValueBool;
    else if (strcmp(settingName, "rateNavPrio") == 0)
        settings.rateNavPrio = settingValue;
    else if (strcmp(settingName, "minCNO") == 0)
    {
        if (zedModuleType == PLATFORM_F9R)
            settings.minCNO_F9R = settingValue;
        else
            settings.minCNO_F9P = settingValue;
    }

    else if (strcmp(settingName, "ethernetDHCP") == 0)
        settings.ethernetDHCP = settingValueBool;
    else if (strcmp(settingName, "ethernetIP") == 0)
    {
        String tempString = String(settingValueStr);
        settings.ethernetIP.fromString(tempString);
    }
    else if (strcmp(settingName, "ethernetDNS") == 0)
    {
        String tempString = String(settingValueStr);
        settings.ethernetDNS.fromString(tempString);
    }
    else if (strcmp(settingName, "ethernetGateway") == 0)
    {
        String tempString = String(settingValueStr);
        settings.ethernetGateway.fromString(tempString);
    }
    else if (strcmp(settingName, "ethernetSubnet") == 0)
    {
        String tempString = String(settingValueStr);
        settings.ethernetSubnet.fromString(tempString);
    }
    else if (strcmp(settingName, "ethernetHttpPort") == 0)
        settings.ethernetHttpPort = settingValue;
    else if (strcmp(settingName, "ethernetNtpPort") == 0)
        settings.ethernetNtpPort = settingValue;
    else if (strcmp(settingName, "enableTcpClientEthernet") == 0)
        settings.enableTcpClientEthernet = settingValueBool;
    else if (strcmp(settingName, "ethernetTcpPort") == 0)
        settings.ethernetTcpPort = settingValue;
    else if (strcmp(settingName, "hostForTCPClient") == 0)
        strcpy(settings.hostForTCPClient, settingValueStr);

    // NTP
    else if (strcmp(settingName, "ntpPollExponent") == 0)
        settings.ntpPollExponent = settingValue;
    else if (strcmp(settingName, "ntpPrecision") == 0)
        settings.ntpPrecision = settingValue;
    else if (strcmp(settingName, "ntpRootDelay") == 0)
        settings.ntpRootDelay = settingValue;
    else if (strcmp(settingName, "ntpRootDispersion") == 0)
        settings.ntpRootDispersion = settingValue;
    else if (strcmp(settingName, "ntpReferenceId") == 0)
    {
        strcpy(settings.ntpReferenceId, settingValueStr);
        for (int i = strlen(settingValueStr); i < 5; i++)
            settings.ntpReferenceId[i] = 0;
    }
    else if (strcmp(settingName, "mdnsEnable") == 0)
        settings.mdnsEnable = settingValueBool;

    // Unused variables - read to avoid errors
    else if (strcmp(settingName, "measurementRateSec") == 0)
    {
    }
    else if (strcmp(settingName, "baseTypeSurveyIn") == 0)
    {
    }
    else if (strcmp(settingName, "fixedBaseCoordinateTypeGeo") == 0)
    {
    }
    else if (strcmp(settingName, "saveToArduino") == 0)
    {
    }
    else if (strcmp(settingName, "enableFactoryDefaults") == 0)
    {
    }
    else if (strcmp(settingName, "enableFirmwareUpdate") == 0)
    {
    }
    else if (strcmp(settingName, "enableForgetRadios") == 0)
    {
    }
    else if (strcmp(settingName, "nicknameECEF") == 0)
    {
    }
    else if (strcmp(settingName, "nicknameGeodetic") == 0)
    {
    }
    else if (strcmp(settingName, "fileSelectAll") == 0)
    {
    }
    else if (strcmp(settingName, "fixedHAE_APC") == 0)
    {
    }
    else if (strcmp(settingName, "measurementRateSecBase") == 0)
    {
    }


    else if (strcmp(settingName, "factoryDefaultReset") == 0)
        factoryReset(false); //We do not have the sdSemaphore
    else if (strcmp(settingName, "exitAndReset") == 0)
    {
        // Confirm receipt
        log_d("Sending reset confirmation");
        websocket->textAll("confirmReset,1,");
        delay(500); // Allow for delivery

        if (configureViaEthernet)
            systemPrintln("Reset after Configure-Via-Ethernet");
        else
            systemPrintln("Reset after AP Config");

        if (configureViaEthernet)
        {
            

            // We need to exit configure-via-ethernet mode.
            // But if the settings have not been saved then lastState will still be STATE_CONFIG_VIA_ETH_STARTED.
            // If that is true, then force exit to Base mode. I think it is the best we can do.
            //(If the settings have been saved, then the code will restart in NTP, Base or Rover mode as desired.)
            if (settings.lastState == STATE_CONFIG_VIA_ETH_STARTED)
            {
                systemPrintln("Settings were not saved. Resetting into Rover mode.");
                settings.lastState = STATE_ROVER_NOT_STARTED;
                recordSystemSettings();
            }
        }

        ESP.restart();
    }

    else if (strcmp(settingName, "forgetEspNowPeers") == 0)
    {
        // Forget all ESP-Now Peers
        for (int x = 0; x < settings.espnowPeerCount; x++)
            espnowRemovePeer(settings.espnowPeers[x]);
        settings.espnowPeerCount = 0;
    }
    


    // Check for bulk settings (constellations and message rates)
    // Must be last on else list
    else
    {
        bool knownSetting = false;

        // Scan for WiFi credentials
        if (knownSetting == false)
        {
            for (int x = 0; x < MAX_WIFI_NETWORKS; x++)
            {
                char tempString[100]; // wifiNetwork0Password=parachutes
                snprintf(tempString, sizeof(tempString), "wifiNetwork%dSSID", x);
                if (strcmp(settingName, tempString) == 0)
                {
                    strcpy(settings.wifiNetworks[x].ssid, settingValueStr);
                    knownSetting = true;
                    break;
                }
                else
                {
                    snprintf(tempString, sizeof(tempString), "wifiNetwork%dPassword", x);
                    if (strcmp(settingName, tempString) == 0)
                    {
                        strcpy(settings.wifiNetworks[x].password, settingValueStr);
                        knownSetting = true;
                        break;
                    }
                }
            }
        }

        // Scan for constellation settings
        if (knownSetting == false)
        {
            for (int x = 0; x < MAX_CONSTELLATIONS; x++)
            {
                char tempString[50]; // ubxConstellationsSBAS
                snprintf(tempString, sizeof(tempString), "ubxConstellations%s", settings.ubxConstellations[x].textName);

                if (strcmp(settingName, tempString) == 0)
                {
                    settings.ubxConstellations[x].enabled = settingValueBool;
                    knownSetting = true;
                    break;
                }
            }
        }

        // Scan for message settings
        if (knownSetting == false)
        {
            char tempString[50];

            for (int x = 0; x < MAX_UBX_MSG; x++)
            {
                snprintf(tempString, sizeof(tempString), "%s", ubxMessages[x].msgTextName); // UBX_RTCM_1074
                if (strcmp(settingName, tempString) == 0)
                {
                    settings.ubxMessageRates[x] = settingValue;
                    knownSetting = true;
                    break;
                }
            }
        }


        // Last catch
        if (knownSetting == false)
        {
            systemPrintf("Unknown '%s': %0.3lf\r\n", settingName, settingValue);
        }
    } // End last strcpy catch
#endif  // COMPILE_AP
}

// Add record with int
void stringRecord(char *settingsCSV, const char *id, int settingValue)
{
    char record[100];
    snprintf(record, sizeof(record), "%s,%d,", id, settingValue);
    strcat(settingsCSV, record);
}

// Add record with uint32_t
void stringRecord(char *settingsCSV, const char *id, uint32_t settingValue)
{
    char record[100];
    snprintf(record, sizeof(record), "%s,%d,", id, settingValue);
    strcat(settingsCSV, record);
}

// Add record with double
void stringRecord(char *settingsCSV, const char *id, double settingValue, int decimalPlaces)
{
    char format[10];
    snprintf(format, sizeof(format), "%%0.%dlf", decimalPlaces); // Create '%0.09lf'

    char formattedValue[20];
    snprintf(formattedValue, sizeof(formattedValue), format, settingValue);

    char record[100];
    snprintf(record, sizeof(record), "%s,%s,", id, formattedValue);
    strcat(settingsCSV, record);
}

// Add record with bool
void stringRecord(char *settingsCSV, const char *id, bool settingValue)
{
    char temp[10];
    if (settingValue == true)
        strcpy(temp, "true");
    else
        strcpy(temp, "false");

    char record[100];
    snprintf(record, sizeof(record), "%s,%s,", id, temp);
    strcat(settingsCSV, record);
}

// Add record with string
void stringRecord(char *settingsCSV, const char *id, char *settingValue)
{
    char record[100];
    snprintf(record, sizeof(record), "%s,%s,", id, settingValue);
    strcat(settingsCSV, record);
}

// Add record with uint64_t
void stringRecord(char *settingsCSV, const char *id, uint64_t settingValue)
{
    char record[100];
    snprintf(record, sizeof(record), "%s,%lld,", id, settingValue);
    strcat(settingsCSV, record);
}

// Break CSV into setting constituents
// Can't use strtok because we may have two commas next to each other, ie
// measurementRateHz,4.00,measurementRateSec,,dynamicModel,0,
bool parseIncomingSettings()
{
    char settingName[100] = {'\0'};
    char valueStr[150] = {'\0'}; // stationGeodetic1,ANameThatIsTooLongToBeDisplayed 40.09029479 -105.18505761 1560.089

    char *commaPtr = incomingSettings;
    char *headPtr = incomingSettings;

    int counter = 0;
    int maxAttempts = 500;
    while (*headPtr) // Check if we've reached the end of the string
    {
        // Spin to first comma
        commaPtr = strstr(headPtr, ",");
        if (commaPtr != nullptr)
        {
            *commaPtr = '\0';
            strcpy(settingName, headPtr);
            headPtr = commaPtr + 1;
        }

        commaPtr = strstr(headPtr, ",");
        if (commaPtr != nullptr)
        {
            *commaPtr = '\0';
            strcpy(valueStr, headPtr);
            headPtr = commaPtr + 1;
        }

        // log_d("settingName: %s value: %s", settingName, valueStr);

        updateSettingWithValue(settingName, valueStr);

        // Avoid infinite loop if response is malformed
        counter++;
        if (counter == maxAttempts)
        {
            systemPrintln("Error: Incoming settings malformed.");
            break;
        }
    }

    if (counter < maxAttempts)
    {
        // Confirm receipt
        log_d("Sending receipt confirmation of settings");
#ifdef COMPILE_AP
        websocket->textAll("confirmDataReceipt,1,");
#endif  // COMPILE_AP
    }

    return (true);
}



// When called, responds with the messages supported on this platform
// Message name and current rate are formatted in CSV, formatted to html by JS
void createMessageList(String &returnText)
{
    returnText = "";

    for (int messageNumber = 0; messageNumber < MAX_UBX_MSG; messageNumber++)
    {
        if (messageSupported(messageNumber) == true)
            returnText += String(ubxMessages[messageNumber].msgTextName) + "," +
                          String(settings.ubxMessageRates[messageNumber]) + ","; // UBX_RTCM_1074,4,
    }

    log_d("returnText (%d bytes): %s\r\n", returnText.length(), returnText.c_str());
}


// Make size of files human readable
void stringHumanReadableSize(String &returnText, uint64_t bytes)
{
    char suffix[5] = {'\0'};
    char readableSize[50] = {'\0'};
    float cardSize = 0.0;

    if (bytes < 1024)
        strcpy(suffix, "B");
    else if (bytes < (1024 * 1024))
        strcpy(suffix, "KB");
    else if (bytes < (1024 * 1024 * 1024))
        strcpy(suffix, "MB");
    else
        strcpy(suffix, "GB");

    if (bytes < (1024))
        cardSize = bytes; // B
    else if (bytes < (1024 * 1024))
        cardSize = bytes / 1024.0; // KB
    else if (bytes < (1024 * 1024 * 1024))
        cardSize = bytes / 1024.0 / 1024.0; // MB
    else
        cardSize = bytes / 1024.0 / 1024.0 / 1024.0; // GB

    if (strcmp(suffix, "GB") == 0)
        snprintf(readableSize, sizeof(readableSize), "%0.1f %s", cardSize, suffix); // Print decimal portion
    else if (strcmp(suffix, "MB") == 0)
        snprintf(readableSize, sizeof(readableSize), "%0.1f %s", cardSize, suffix); // Print decimal portion
    else if (strcmp(suffix, "KB") == 0)
        snprintf(readableSize, sizeof(readableSize), "%0.1f %s", cardSize, suffix); // Print decimal portion
    else
        snprintf(readableSize, sizeof(readableSize), "%.0f %s", cardSize, suffix); // Don't print decimal portion

    returnText = String(readableSize);
}

#endif //COMPILE_WEBSERVER

