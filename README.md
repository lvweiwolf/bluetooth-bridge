# bluetooth-bridge

Linux设备上蓝牙域MQTT通信转发工具

MQTT发布/订阅描述：

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
    "address": "00:14:BE:80:3A:8C",
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
    "address": "00:14:BE:80:3A:8C",
    "data": "Hello, I am example data!",
    "size": 24, // 字节数
    "publishId": "019af669-232c-7433-9e1a-50613d2803b4",
    "publishTime": "2024-06-12 10:20:30"
  }
}
```

#### 5. /org/booway/bluetooth/receiveFromDevice (发布Topc)

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


#### 6. /org/booway/bluetooth/getLastError (发布Topic)

#### 作用： bluetooth-bridge 转发订阅MQTT过程中发生的错误

消息格式
```json
{
  "message": "解析JSON消息失败, ...",
  "subscirbeId":  "019af669-232c-7433-9e1a-50613d2803b4",
}
```