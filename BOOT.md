# Проверка загрузки

1. USB не воткнут: b0, r1
2. USB втыкаем: b1, r1
3. USB вытыкаем: b0, r1

RTS# - NRST

DTS# - BOOT0, pulldown 4.7k

## Без контроря потока

```cpp
    serial.setPortName(currentPortName);
    serial.setBaudRate(921600);
    serial.setReadBufferSize(1024 * 1024);
    if (!serial.open(QIODevice::ReadWrite)) {
        return;
    }
```

1. USB воткнут: b1, r1
2. Порт открыт: b0, r0
3. Порт закрыт: b1 r1

## 

```cpp
    serial.setReadBufferSize(1024 * 1024);
    serial.setFlowControl(QSerialPort::NoFlowControl);
    serial.setRequestToSend(true);
    if (!serial.open(QIODevice::ReadWrite)) {
        return;
    }
```

1. USB воткнут: b1, r1
2. Порт открыт: b0, r0
3. Порт закрыт: b1 r1


## 

```cpp
    serial.setPortName(currentPortName);
    serial.setBaudRate(921600);
    serial.setReadBufferSize(1024 * 1024);
    serial.setFlowControl(QSerialPort::NoFlowControl);
    if (!serial.open(QIODevice::ReadWrite)) {
        return;
    }
    serial.setRequestToSend(true);
```

1. USB воткнут: b1, r1
2. Порт открыт: b0, r0
3. Порт закрыт: b1 r1
