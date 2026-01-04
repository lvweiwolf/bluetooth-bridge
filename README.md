# bluetooth-bridge

Linux设备上蓝牙域MQTT通信转发工具

MQTT发布/订阅描述：

#### 1. /org/booway/bluetooth/getAdapters (发布Topic) 

消息格式:
```json
{
  "adapters": [
    {
      "Address": "9C:B8:B4:67:11:6D",
      "AddressType": "public",
      "Alias": "RK356X",
      "Class": 8126464,
      "Discoverable": true,
      "DiscoverableTimeout": 0,
      "Discovering": true,
      "Modalias": "usb:v1D6Bp0246d0537",
      "Name": "RK356X",
      "Pairable": true,
      "PairableTimeout": 0,
      "Powered": true,
      "UUIDs": [
        "00001133-0000-1000-8000-00805f9b34fb",
        "0000110e-0000-1000-8000-00805f9b34fb",
        "00001105-0000-1000-8000-00805f9b34fb",
        "00001132-0000-1000-8000-00805f9b34fb",
        "00001200-0000-1000-8000-00805f9b34fb",
        "00001104-0000-1000-8000-00805f9b34fb",
        "00005005-0000-1000-8000-0002ee000001",
        "00001108-0000-1000-8000-00805f9b34fb",
        "0000110c-0000-1000-8000-00805f9b34fb",
        "00001801-0000-1000-8000-00805f9b34fb",
        "0000112f-0000-1000-8000-00805f9b34fb",
        "0000110b-0000-1000-8000-00805f9b34fb",
        "0000180a-0000-1000-8000-00805f9b34fb",
        "00001800-0000-1000-8000-00805f9b34fb",
        "00001112-0000-1000-8000-00805f9b34fb",
        "0000110a-0000-1000-8000-00805f9b34fb",
        "0000112d-0000-1000-8000-00805f9b34fb",
        "00001106-0000-1000-8000-00805f9b34fb"
      ]
    },
    ...
  ]
}
```

#### 1. /org/booway/bluetooth/getDevices (发布Topic) 
    
消息格式：
```json  
{
  "devices": [
    {
      "adapter": "/org/bluez/hci0",
      "address": "00:14:BE:80:3A:8C",
      "address_type": "public",
      "alias": "",
      "blocked": false,
      "bonded": false,
      "connected": false,
      "legacyPairing": false,
      "name": "JZS115",
      "paired": false,
      "rssi": -69,
      "servicesResolved": false,
      "trusted": false,
      "uuids": [
        "0000180f-0000-1000-8000-00805f9b34fb",
        "0000fd92-0000-1000-8000-00805f9b34fb"
      ]
    },
    ...
  ]
}
```

#### 2. /org/booway/bluetooth/connectDevice (订阅Topic)

消息格式
```json
{
  "device":{
    "address": "04:25:09:10:01:C4",
    "pincode": "123132",
    "publishId": "019af669-232c-7433-9e1a-50613d2803b4",
    "publishTime": "2024-06-12 10:20:30"
  }
}
```

#### 3. /org/booway/bluetooth/disconnectDevice (订阅Topic)
```json
{
  "device":{
    "address": "00:14:BE:80:3A:8C",
    "publishId": "019af669-232c-7433-9e1a-50613d2803b4",
    "publishTime": "2024-06-12 10:20:30"
  }
}
```


#### 4. /org/booway/bluetooth/newConnection (发布Topic)
```json
{
  "device": {
    "address": "00:14:BE:80:3A:8C",
    "name": "Huawei mate 60Pro",
    "publishId": "019af669-232c-7433-9e1a-50613d2803b4",
    "publishTime": "2024-06-12 10:20:30"
  }
}
```

#### 5. /org/booway/bluetooth/loseConnection (发布Topic)
```json
{
  "device": {
    "address": "00:14:BE:80:3A:8C",
    "name": "Huawei mate 60Pro",
    "publishId": "019af669-232c-7433-9e1a-50613d2803b4",
    "publishTime": "2024-06-12 10:20:30"
  }
}
```

#### 4. /org/booway/bluetooth/sendToDevice (订阅Topic)

消息格式
```json
{
  "device": {
    "address": "04:25:09:10:01:A3",
    "data": "QkVHDwAAAAEAAAAAAIkO",
    "size": 15,
    "publishId": "019af669-232c-7433-9e1a-50613d2803b4",
    "publishTime": "2024-06-12 10:20:30"
  }
}
```


#### 5. /org/booway/bluetooth/removeDevices (订阅Topic)

消息格式
```json
{
    "address": ["04:25:09:10:01:C4"],
    "publishId": "019af669-232c-7433-9e1a-50613d2803b4",
    "publishTime": "2024-06-12 10:20:30"
}
```

#### 6. /org/booway/bluetooth/receiveFromDevice (发布Topc)

消息格式
```json
{
  "device": {
    "address": "00:14:BE:80:3A:8C",
    "data": "Hello, I am recevied data from device client.",
    "size": 44,
    "publishId": "019af669-232c-7433-9e1a-50613d2803b4",
    "publishTime": "2024-06-12 10:20:30"
  }
}
```


#### 7. /org/booway/bluetooth/getLastError (发布Topic)

#### 作用： bluetooth-bridge 转发订阅MQTT过程中发生的错误

消息格式
```json
{
  "message": "解析JSON消息失败, ...",
  "subscirbeId":  "019af669-232c-7433-9e1a-50613d2803b4",
}
```


#### 8. /org/booway/bluetooth/connectBenchmarkTest (订阅)
#### 作用： 蓝牙连接测试

消息格式
```json
{
  "address": "04:25:09:10:01:A3",
  "times": 10
}
````