// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// NTP Packet storage and utilities

struct NTPpacket
{
    static const uint8_t NTPpacketSize = 48;

    uint8_t packet[NTPpacketSize]; // Copy of the NTP packet
    void setPacket(uint8_t *ptr)
    {
        memcpy(packet, ptr, NTPpacketSize);
    }
    void getPacket(uint8_t *ptr)
    {
        memcpy(ptr, packet, NTPpacketSize);
    }

    const uint32_t NTPtoUnixOffset = 2208988800; // NTP starts at Jan 1st 1900. Unix starts at Jan 1st 1970.

    uint8_t LiVnMode; // Leap Indicator, Version Number, Mode

    // Leap Indicator is 2 bits :
    // 00 : No warning
    // 01 : Last minute of the day has 61s
    // 10 : Last minute of the day has 59 s
    // 11 : Alarm condition (clock not synchronized)
    const uint8_t defaultLeapInd = 0;
    uint8_t LI()
    {
        return LiVnMode >> 6;
    }
    void LI(uint8_t val)
    {
        LiVnMode = (LiVnMode & 0x3F) | ((val & 0x03) << 6);
    }

    // Version Number is 3 bits. NTP version is currently four (4)
    const uint8_t defaultVersion = 4;
    uint8_t VN()
    {
        return (LiVnMode >> 3) & 0x07;
    }
    void VN(uint8_t val)
    {
        LiVnMode = (LiVnMode & 0xC7) | ((val & 0x07) << 3);
    }

    // Mode is 3 bits:
    // 0 : Reserved
    // 1 : Symmetric active
    // 2 : Symmetric passive
    // 3 : Client
    // 4 : Server
    // 5 : Broadcast
    // 6 : NTP control message
    // 7 : Reserved for private use
    const uint8_t defaultMode = 4;
    uint8_t mode()
    {
        return (LiVnMode & 0x07);
    }
    void mode(uint8_t val)
    {
        LiVnMode = (LiVnMode & 0xF8) | (val & 0x07);
    }

    // Stratum is 8 bits:
    // 0 : Unspecified
    // 1 : Reference clock (e.g., radio clock)
    // 2-15 : Secondary server (via NTP)
    // 16-255 : Unreachable
    //
    // We'll use 1 = Reference Clock
    const uint8_t defaultStratum = 1;
    uint8_t stratum;

    // Poll exponent
    // This is an eight-bit unsigned integer indicating the maximum interval between successive messages,
    // in seconds to the nearest power of two.
    // In the reference implementation, the values can range from 3 (8 s) through 17 (36 h).
    //
    // RFC 5905 suggests 6-10. We'll use 6. 2^6 = 64 seconds
    const uint8_t defaultPollExponent = 6;
    uint8_t pollExponent;

    // Precision
    // This is an eight-bit signed integer indicating the precision of the system clock,
    // in seconds to the nearest power of two. For instance, a value of -18 corresponds to a precision of about 4us.
    //
    // tAcc is usually around 1us. So we'll use -20 (0xEC). 2^-20 = 0.95us
    const int8_t defaultPrecision = -20; // 0xEC
    int8_t precision;

    // Root delay
    // This is a 32-bit, unsigned, fixed-point number indicating the total round-trip delay to the reference clock,
    // in seconds with fraction point between bits 15 and 16. In contrast to the calculated peer round-trip delay,
    // which can take both positive and negative values, this value is always positive.
    //
    // We are the reference clock, so we'll use zero (0x00000000).
    const uint32_t defaultRootDelay = 0x00000000;
    uint32_t rootDelay;

    // Root dispersion
    // This is a 32-bit, unsigned, fixed-point number indicating the maximum error relative to the reference clock,
    // in seconds with fraction point between bits 15 and 16.
    //
    // Tricky... Could depend on interrupt service time? Maybe go with ~1ms?
    const uint32_t defaultRootDispersion = 0x00000042; // 1007us
    uint32_t rootDispersion;

    // Reference identifier
    // This is a 32-bit code identifying the particular reference clock. The interpretation depends on the value in
    // the stratum field. For stratum 0 (unsynchronized), this is a four-character ASCII (American Standard Code for
    // Information Interchange) string called the kiss code, which is used for debugging and monitoring purposes.
    // GPS : Global Positioning System
    const uint8_t referenceIdLen = 4;
    const char defaultReferenceId[4] = {'G', 'P', 'S', 0};
    char referenceId[4];

    // Reference timestamp
    // This is the local time at which the system clock was last set or corrected, in 64-bit NTP timestamp format.
    uint32_t referenceTimestampSeconds;
    uint32_t referenceTimestampFraction;

    // Originate timestamp
    // This is the local time at which the request departed the client for the server, in 64-bit NTP timestamp format.
    uint32_t originateTimestampSeconds;
    uint32_t originateTimestampFraction;

    // Receive timestamp
    // This is the local time at which the request arrived at the server, in 64-bit NTP timestamp format.
    uint32_t receiveTimestampSeconds;
    uint32_t receiveTimestampFraction;

    // Transmit timestamp
    // This is the local time at which the reply departed the server for the client, in 64-bit NTP timestamp format.
    uint32_t transmitTimestampSeconds;
    uint32_t transmitTimestampFraction;

    typedef union {
        int8_t signed8;
        uint8_t unsigned8;
    } unsignedSigned8;

    uint32_t extractUnsigned32(uint8_t *ptr)
    {
        uint32_t val = 0;
        val |= *ptr++ << 24; // NTP data is Big-Endian
        val |= *ptr++ << 16;
        val |= *ptr++ << 8;
        val |= *ptr++;
        return val;
    }

    void insertUnsigned32(uint8_t *ptr, uint32_t val)
    {
        *ptr++ = val >> 24; // NTP data is Big-Endian
        *ptr++ = (val >> 16) & 0xFF;
        *ptr++ = (val >> 8) & 0xFF;
        *ptr++ = val & 0xFF;
    }

    // Extract the data from an NTP packet into the correct fields
    void extract()
    {
        uint8_t *ptr = packet;

        LiVnMode = *ptr++;
        stratum = *ptr++;
        pollExponent = *ptr++;

        unsignedSigned8 converter8;
        converter8.unsigned8 = *ptr++; // Convert to int8_t without ambiguity
        precision = converter8.signed8;

        rootDelay = extractUnsigned32(ptr);
        ptr += 4;
        rootDispersion = extractUnsigned32(ptr);
        ptr += 4;

        for (uint8_t i = 0; i < referenceIdLen; i++)
            referenceId[i] = *ptr++;

        referenceTimestampSeconds = extractUnsigned32(ptr);
        ptr += 4;
        referenceTimestampFraction =
            extractUnsigned32(ptr); // Note: the fraction is in increments of (1 / 2^32) secs, not microseconds
        ptr += 4;
        originateTimestampSeconds = extractUnsigned32(ptr);
        ptr += 4;
        originateTimestampFraction = extractUnsigned32(ptr);
        ptr += 4;
        receiveTimestampSeconds = extractUnsigned32(ptr);
        ptr += 4;
        receiveTimestampFraction = extractUnsigned32(ptr);
        ptr += 4;
        transmitTimestampSeconds = extractUnsigned32(ptr);
        ptr += 4;
        transmitTimestampFraction = extractUnsigned32(ptr);
        ptr += 4;
    }

    // Insert the data from the fields into an NTP packet
    void insert()
    {
        uint8_t *ptr = packet;

        *ptr++ = LiVnMode;
        *ptr++ = stratum;
        *ptr++ = pollExponent;

        unsignedSigned8 converter8;
        converter8.signed8 = precision;
        *ptr++ = converter8.unsigned8; // Convert to uint8_t without ambiguity

        insertUnsigned32(ptr, rootDelay);
        ptr += 4;
        insertUnsigned32(ptr, rootDispersion);
        ptr += 4;

        for (uint8_t i = 0; i < 4; i++)
            *ptr++ = referenceId[i];

        insertUnsigned32(ptr, referenceTimestampSeconds);
        ptr += 4;
        insertUnsigned32(
            ptr,
            referenceTimestampFraction); // Note: the fraction is in increments of (1 / 2^32) secs, not microseconds
        ptr += 4;
        insertUnsigned32(ptr, originateTimestampSeconds);
        ptr += 4;
        insertUnsigned32(ptr, originateTimestampFraction);
        ptr += 4;
        insertUnsigned32(ptr, receiveTimestampSeconds);
        ptr += 4;
        insertUnsigned32(ptr, receiveTimestampFraction);
        ptr += 4;
        insertUnsigned32(ptr, transmitTimestampSeconds);
        ptr += 4;
        insertUnsigned32(ptr, transmitTimestampFraction);
        ptr += 4;
    }

    uint32_t convertMicrosToSecsAndFraction(uint32_t val) // 16-bit fraction used by root delay and dispersion
    {
        double secs = val;
        secs /= 1000000.0;  // Convert micros to seconds
        secs = floor(secs); // Convert to integer, round down

        double microsecs = val;
        microsecs -= secs * 1000000.0; // Subtract the seconds
        microsecs /= 1000000.0;        // Convert micros to seconds
        microsecs *= pow(2.0, 16.0);   // Convert to 16-bit fraction

        uint32_t result = ((uint32_t)secs) << 16;
        result |= ((uint32_t)microsecs) & 0xFFFF;
        return (result);
    }

    uint32_t convertMicrosToFraction(uint32_t val) // 32-bit fraction used by the timestamps
    {
        val %= 1000000;      // Just in case
        double v = val;      // Convert micros to double
        v /= 1000000.0;      // Convert micros to seconds
        v *= pow(2.0, 32.0); // Convert to fraction
        return (uint32_t)v;
    }

    uint32_t convertFractionToMicros(uint32_t val) // 32-bit fraction used by the timestamps
    {
        double v = val;      // Convert fraction to double
        v /= pow(2.0, 32.0); // Convert fraction to seconds
        v *= 1000000.0;      // Convert to micros
        uint32_t ret = (uint32_t)v;
        ret %= 1000000; // Just in case
        return ret;
    }

    uint32_t convertNTPsecondsToUnix(uint32_t val)
    {
        return (val - NTPtoUnixOffset);
    }

    uint32_t convertUnixSecondsToNTP(uint32_t val)
    {
        return (val + NTPtoUnixOffset);
    }
};

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Process one NTP request
// recTv contains the timeval the NTP packet was received - from the W5500 interrupt
// syncTv contains the timeval when the RTC was last sync'd
// ntpDiag will contain useful diagnostics
bool processOneNTPRequest(bool process, const timeval *recTv, const timeval *syncTv, char *ntpDiag = nullptr,
                          size_t ntpDiagSize = 0); // Header
bool processOneNTPRequest(bool process, const timeval *recTv, const timeval *syncTv, char *ntpDiag, size_t ntpDiagSize)
{
    bool processed = false;

    if (ntpDiag != nullptr)
        *ntpDiag = 0; // Clear any existing diagnostics

    return processed;
}

// Configure specific aspects of the receiver for NTP mode
bool configureUbloxModuleNTP()
{
    if (!HAS_GNSS_TP_INT)
        return (false);

    if (online.gnss == false)
        return (false);

    // If our settings haven't changed, and this is first config since power on, trust ZED's settings
    // Unless this is a Ref Syn - where the GNSS has no battery-backed RAM
    if (productVariant != REFERENCE_STATION && settings.updateZEDSettings == false && firstPowerOn == true)
    {
        firstPowerOn = false; // Next time user switches modes, new settings will be applied
        log_d("Skipping ZED NTP configuration");
        return (true);
    }

    firstPowerOn = false; // If we switch between rover/base in the future, force config of module.

    theGNSS.checkUblox();     // Regularly poll to get latest data and any RTCM
    theGNSS.checkCallbacks(); // Process any callbacks: ie, storePVTdata

    theGNSS.setNMEAGPGGAcallbackPtr(
        nullptr); // Disable GPGGA call back that may have been set during Rover NTRIP Client mode

    int tryNo = -1;
    bool success = false;

    // Try up to MAX_SET_MESSAGES_RETRIES times to configure the GNSS
    // This corrects occasional failures seen on the Reference Station where the GNSS is connected via SPI
    // instead of I2C and UART1. I believe the SETVAL ACK is occasionally missed due to the level of messages being
    // processed.
    while ((++tryNo < MAX_SET_MESSAGES_RETRIES) && !success)
    {
        bool response = true;

        // In NTP mode we force 1Hz
        response &= theGNSS.newCfgValset();
        response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_MEAS, 1000);
        response &= theGNSS.addCfgValset(UBLOX_CFG_RATE_NAV, 1);

        // Survey mode is only available on ZED-F9P modules
        if (zedModuleType == PLATFORM_F9P)
            response &= theGNSS.addCfgValset(UBLOX_CFG_TMODE_MODE, 0); // Disable survey-in mode

        // Set dynamic model to stationary
        response &= theGNSS.addCfgValset(UBLOX_CFG_NAVSPG_DYNMODEL, DYN_MODEL_STATIONARY); // Set dynamic model

        // Set time pulse to 1Hz (100:900)
        response &= theGNSS.addCfgValset(UBLOX_CFG_TP_PULSE_DEF, 0);        // Time pulse definition is a period (in us)
        response &= theGNSS.addCfgValset(UBLOX_CFG_TP_PULSE_LENGTH_DEF, 1); // Define timepulse by length (not ratio)
        response &=
            theGNSS.addCfgValset(UBLOX_CFG_TP_USE_LOCKED_TP1,
                                 1); // Use CFG-TP-PERIOD_LOCK_TP1 and CFG-TP-LEN_LOCK_TP1 as soon as GNSS time is valid
        response &= theGNSS.addCfgValset(UBLOX_CFG_TP_TP1_ENA, 1); // Enable timepulse
        response &= theGNSS.addCfgValset(UBLOX_CFG_TP_POL_TP1, 1); // 1 = rising edge

        // While the module is _locking_ to GNSS time, turn off pulse
        response &= theGNSS.addCfgValset(UBLOX_CFG_TP_PERIOD_TP1, 1000000); // Set the period between pulses in us
        response &= theGNSS.addCfgValset(UBLOX_CFG_TP_LEN_TP1, 0);          // Set the pulse length in us

        // When the module is _locked_ to GNSS time, make it generate 1Hz (100ms high, 900ms low)
        response &= theGNSS.addCfgValset(UBLOX_CFG_TP_PERIOD_LOCK_TP1, 1000000); // Set the period between pulses is us
        response &= theGNSS.addCfgValset(UBLOX_CFG_TP_LEN_LOCK_TP1, 100000);     // Set the pulse length in us

        // Ensure pulse is aligned to top-of-second. This is the default. Set it here just to make sure.
        response &= theGNSS.addCfgValset(UBLOX_CFG_TP_ALIGN_TO_TOW_TP1, 1);

        // Set the time grid to UTC. This is the default. Set it here just to make sure.
        response &= theGNSS.addCfgValset(UBLOX_CFG_TP_TIMEGRID_TP1, 0); // 0=UTC; 1=GPS

        // Sync to GNSS. This is the default. Set it here just to make sure.
        response &= theGNSS.addCfgValset(UBLOX_CFG_TP_SYNC_GNSS_TP1, 1);

        response &= theGNSS.addCfgValset(UBLOX_CFG_NAVSPG_INFIL_MINELEV, settings.minElev); // Set minimum elevation

        // Ensure PVT, HPPOSLLH and TP messages are being output at 1Hz on the correct port
        if (USE_I2C_GNSS)
        {
            response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_PVT_I2C, 1);
            response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSLLH_I2C, 1);
            response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_TIM_TP_I2C, 1);
            if (zedModuleType == PLATFORM_F9R)
            {
                response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_ESF_STATUS_I2C, 1);
            }
        }
        else
        {
            response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_PVT_SPI, 1);
            response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSLLH_SPI, 1);
            response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_TIM_TP_SPI, 1);
            if (zedModuleType == PLATFORM_F9R)
            {
                response &= theGNSS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_ESF_STATUS_SPI, 1);
            }
        }

        response &= theGNSS.sendCfgValset(); // Closing value

        if (response)
            success = true;
    }

    if (!success)
        systemPrintln("NTP config fail");

    return (success);
}

void menuNTP()
{

}
