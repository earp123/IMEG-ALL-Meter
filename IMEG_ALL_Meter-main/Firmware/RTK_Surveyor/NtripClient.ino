
void ntripClientStart()
{
    systemPrintln("NTRIP Client not available: Ethernet and WiFi not compiled");
}
void ntripClientStop(bool clientAllocated) {online.ntripClient = false;}
void ntripClientUpdate() {}


