#include "wake.h"

/// Frame End
#define WAKE_CODE_FEND          (0xC0)
/// Frame Escape
#define WAKE_CODE_FESC          (0xDB)
/// Transposed Frame End
#define WAKE_CODE_TFEND         (0xDC)
/// Transposed Frame Escape
#define WAKE_CODE_TFESC         (0xDD)

#define CRC_INIT                (0x00) 			// Innitial CRC value

#define FRAME_SIZE_MAXIMUM      (128)	///< Максимальный размер буфера приёмника Wake



static void Do_Crc8(uint8_t b, uint8_t *crc);


Wake::Wake(QObject *parent) : QObject(parent) {
    logger = spdlog::get("Wake");
}

Wake::~Wake() {
    logger->info("Destroy");
}

const QByteArray Wake::dataArray() const {
    if (m_receivedData.size() == 0) {
        return QByteArray();
    }

QByteArray arr(reinterpret_cast<const char *>(m_receivedData.constData()), m_receivedData.size());
    return arr;
}

Wake::Status Wake::ProcessInByte(uint8_t data) {
Wake::Status ret = Wake::Status::INIT;
    if (data == WAKE_CODE_FEND) {
        Rx_Pre = data;
        Rx_Crc = CRC_INIT;
        Rx_FSM = WAIT_ADDR_OR_CMD;
        Do_Crc8(data, &Rx_Crc);
        return Wake::Status::INIT;
    }

    if (Rx_FSM == WAIT_FEND)
        return Wake::Status::INIT;
    
    uint8_t Pre = Rx_Pre;
    Rx_Pre = data;
    if (Pre == WAKE_CODE_FESC) {
        if (data == WAKE_CODE_TFESC)
            data = WAKE_CODE_FESC;
        else if (data == WAKE_CODE_TFEND)
            data = WAKE_CODE_FEND;
        else {
            Rx_FSM = WAIT_FEND;
            // CMD
            return Wake::Status::INIT;
        }
    } else {
        if (data == WAKE_CODE_FESC)
            return Wake::Status::INIT;
    }

    switch (Rx_FSM) {
        case WAIT_ADDR_OR_CMD: {
            if (data & 0x80) {	// Адрес
                // Принят адрес, старший бит - 1, все адреса принимаем
                data = data & 0x7F;
                Do_Crc8(data, &Rx_Crc);
                Rx_FSM = WAIT_CMD;
                break;
            } else {
                // Принята команда, старший бит - 0
                m_receivedCommand = data;
                Do_Crc8(data, &Rx_Crc);
                Rx_FSM = WAIT_NBT;
            }
            break;
        }

        case WAIT_CMD: {
            if (data & 0x80) {
                // CMD not valid. Upper bit is High
                Rx_FSM = WAIT_FEND;
                break;
            }

            m_receivedCommand = data;
            Do_Crc8(data, &Rx_Crc);
            Rx_FSM = WAIT_NBT;
            break;
        }

        case WAIT_NBT: {
            if (data > FRAME_SIZE_MAXIMUM) {
                Rx_FSM = WAIT_FEND;
                break;
            }

            m_receivedData.clear();
            m_receivedData.reserve(data);
            m_receivedData.fill(0, data);
            Do_Crc8(data, &Rx_Crc);
            Rx_Ptr = 0;
            Rx_FSM = WAIT_DATA;
            break;
        }

        case WAIT_DATA: {
            if (Rx_Ptr < m_receivedData.size()) {
                m_receivedData[Rx_Ptr++] = data;
                Do_Crc8(data, &Rx_Crc);
                break;
            }
            if (data != Rx_Crc) {
                Rx_FSM = WAIT_FEND;
                emit recvInvalid(m_receivedData, m_receivedCommand);
                break;
            }
            Rx_FSM = WAIT_FEND;
            ret = Wake::READY;
            emit recvValid(m_receivedData, m_receivedCommand);
            break;
        }
    }

    return ret;
}

QByteArray Wake::PrepareTx(uint8_t command, const QByteArray &data) {
QByteArray out;
uint8_t crc = CRC_INIT;

    Do_Crc8(WAKE_CODE_FEND, &crc);
    out.append(WAKE_CODE_FEND);

    Do_Crc8(command, &crc);
    out = StuffTx(out, command);

    auto nbt = data.size();
    Do_Crc8(nbt, &crc);
    out = StuffTx(out, nbt);

    for (int i = 0; i < nbt; i++) {
        Do_Crc8(data[i], &crc);
        out = StuffTx(out, data[i]);
    }

    out = StuffTx(out, crc);
    return out;
}

QByteArray &Wake::StuffTx(QByteArray &buf, uint8_t tx) {
    if (tx == WAKE_CODE_FEND) {
        buf.append(WAKE_CODE_FESC);
        buf.append(WAKE_CODE_TFEND);
        return buf;
    } else if (tx == WAKE_CODE_FESC) {
        buf.append(WAKE_CODE_FESC);
        buf.append(WAKE_CODE_TFESC);
        return buf;
    }
    buf.append(tx);
    return buf;
}


static void Do_Crc8(uint8_t b, uint8_t *crc) {
	for (uint8_t i = 0; i < 8; b = b >> 1, i++) {
        if ((b ^ *crc) & 1) {
            *crc = ((*crc ^ 0x18) >> 1) | 0x80;
        } else {
            *crc = (*crc >> 1) & ~0x80;
        }
    }
}
