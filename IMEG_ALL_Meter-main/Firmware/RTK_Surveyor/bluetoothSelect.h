#ifdef COMPILE_BT

//We use a local copy of the BluetoothSerial library so that we can increase the RX buffer. See issues: 
//https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/23
//https://github.com/sparkfun/SparkFun_RTK_Firmware/issues/469
#include "src/BluetoothSerial/BluetoothSerial.h"

#include <BleSerialLux.h> 

class BTSerialInterface
{
  public:
    virtual bool begin(String deviceName, bool isMaster, uint16_t rxQueueSize, uint16_t txQueueSize) = 0;
    virtual void disconnect() = 0;
    virtual void end() = 0;
    virtual esp_err_t register_callback(esp_spp_cb_t * callback) = 0;
    virtual void setTimeout(unsigned long timeout) = 0;

    virtual int available() = 0;
    virtual size_t readBytes(uint8_t *buffer, size_t bufferSize) = 0;
    virtual int read() = 0;

    // virtual bool isCongested() = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) = 0;
    virtual size_t write(uint8_t value) = 0;
    virtual void flush() = 0;
    virtual void writeLux(uint16_t lux_val) = 0;
};


class BTLESerial : public virtual BTSerialInterface, public BleSerial
{
  public:
    // Missing from BleSerial
    bool begin(String deviceName, bool isMaster, uint16_t rxQueueSize, uint16_t txQueueSize)
    {
        BleSerial::begin(deviceName.c_str());
        return true;
    }

    void disconnect()
    {
        Server->disconnect(Server->getConnId());
    }

    void end()
    {
        BleSerial::end();
    }

    esp_err_t register_callback(esp_spp_cb_t * callback)
    {
        connectionCallback = callback;
        return ESP_OK;
    }

    void setTimeout(unsigned long timeout)
    {
        BleSerial::setTimeout(timeout);
    }

    int available()
    {
        return BleSerial::available();
    }

    size_t readBytes(uint8_t *buffer, size_t bufferSize)
    {
        return BleSerial::readBytes(buffer, bufferSize);
    }

    int read()
    {
        return BleSerial::read();
    }

    size_t write(const uint8_t *buffer, size_t size)
    {
        return BleSerial::write(buffer, size);
    }

    size_t write(uint8_t value)
    {
        return BleSerial::write(value);
    }

    void flush()
    {
      BleSerial::flush();
    }

    void writeLux(uint16_t lux_val)
    {
      BleSerial::writeLux(lux_val);
    }

    // override BLEServerCallbacks
    void onConnect(BLEServer *pServer)
    {
        // bleConnected = true; Removed until PR is accepted
        connectionCallback(ESP_SPP_SRV_OPEN_EVT, nullptr);
    }

    void onDisconnect(BLEServer *pServer)
    {
        // bleConnected = false; Removed until PR is accepted
        connectionCallback(ESP_SPP_CLOSE_EVT, nullptr);
        Server->startAdvertising();
    }

  private:
    esp_spp_cb_t *connectionCallback;
};

#endif  // COMPILE_BT
