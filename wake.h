#ifndef WAKE_H
#define WAKE_H

#include <QObject>
#include <spdlog/spdlog.h>

enum class Commands : uint8_t {
    SetVoltage = 0x04,
    Telemetry = 0x55,
};

class Wake : public QObject {
    Q_OBJECT

public:
    explicit Wake(QObject *parent = nullptr);
    ~Wake();

    enum Status {
        INIT,
        RECEIVING,
        READY
    };

    Status ProcessInByte(uint8_t data);
    uint16_t command() const { return m_receivedCommand; }
    const QList<uint8_t> &data() const { return m_receivedData; }

    static QByteArray PrepareTx(Commands command, const QByteArray &data);

signals:
    void recvValid(const QList<uint8_t> &data, uint8_t command);
    void recvInvalid(const QList<uint8_t> &data, uint8_t command);

private:
    std::shared_ptr<spdlog::logger> logger;
    
    QList<uint8_t> m_receivedData;
    uint8_t m_receivedCommand;

	enum FsmRxState {
		WAIT_FEND,          /// ожидание приема FEND
		WAIT_ADDR_OR_CMD,   /// ожидание приема адреса
		WAIT_CMD,           /// ожидание приема команды
		WAIT_NBT,           /// ожидание приема количества байт в пакете
		WAIT_DATA,          /// прием данных
		WAIT_CRC,           /// ожидание окончания приема CRC
		WAIT_CARR 	        /// ожидание несущей
	};

    uint8_t m_iAddressLocal = 0;
    uint8_t m_iAddressRemote = 0;
    uint8_t Rx_FSM = WAIT_FEND;
    uint8_t Rx_Pre;	// Предыдущий принятый байт
    uint8_t Rx_Crc;
    uint8_t Rx_Ptr = 0;

    static QByteArray &StuffTx(QByteArray &buf, uint8_t tx);
};


#endif // WAKE_H
