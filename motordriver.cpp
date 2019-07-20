#include "motordriver.h"

Flywheel4NMDriver::Flywheel4NMDriver(QObject *parent) : MotorDriver(parent)
{
    spd_array_.spd = 0;
    tor_array_.torque = 0;
    recv_spd_.spd = 0;



}

bool SerialDriver::SerialDriver::init()
{
    serial_port_ = new QSerialPort;
    connect(this->serial_port_,SIGNAL(readyRead()),this,SLOT(resolveDataFromSerialport()));

    if (!serial_port_->isOpen()){

        serial_port_->setPortName(port_name);
        serial_port_->setBaudRate(baud_rate.toInt());
        serial_port_->setParity(QSerialPort::OddParity);


        if(!serial_port_->open(QIODevice::ReadWrite)){

            emit sendErrText(serial_port_->errorString());
            return false;

        }

    }
    qDebug()<<port_name<<" has init";
    setInit(true);
    return true;
}

QByteArray Flywheel4NMDriver::calSpdData(QString spd)
{
    QByteArray spd_arr;
    spd_array_.spd = spd.toInt() * 2;
    spd_arr.resize(7);
    spd_arr[0] = 0xD3;
    spd_arr[1] = 0x00;
    spd_arr[2] = 0x00;
    spd_arr[3] = spd_array_.array[1];
    spd_arr[4] = spd_array_.array[0];
    spd_arr[5] = 0x5B;
    // cal total
    uchar t = 0x00;
    for (uint i = 1; i < 6;++i){
        t += spd_arr[i];
    }
    spd_arr[6] = t;
    return spd_arr;
}

QByteArray Flywheel4NMDriver::calTorData(QString tor)
{
    QByteArray tor_arr;
    tor_array_.torque = tor.toInt() / 0.058;
    tor_arr.resize(7);
    tor_arr[0] = 0x1C;
    tor_arr[1] = 0x00;
    tor_arr[2] = 0x00;
    tor_arr[3] = tor_array_.array[1];
    tor_arr[4] = tor_array_.array[0];
    tor_arr[5] = 0x5B;
    // cal total
    uchar t = 0x00;
    for (uint i = 1; i < 6;++i){
        t += tor_arr[i];
    }
    tor_arr[6] = t;

    return tor_arr;
}

void Flywheel4NMDriver::ctlMotorSpd(double spd)
{
    if (!getInit()){
        emit sendErrText(QString("driver not init"));
    }
    else{
        QByteArray spd_str = calSpdData(QString::number(spd));
        if(serial_port_->write(spd_str) != spd_str.size()){
            emit sendErrText(QString("driver control error"));
        }
    }
}

void Flywheel4NMDriver::ctlMotorTor(double tor)
{
    if (!getInit()){
        emit sendErrText(QString("driver not init"));
    }
    else{
        QByteArray tor_str = calTorData(QString::number(tor));
        if(serial_port_->write(tor_str) != tor_str.size()){
            emit sendErrText(QString("driver control error"));
        }
    }
}

void Flywheel4NMDriver::getMotorData()
{
    QByteArray get_str;
    get_str.resize(1);
    get_str[0] = 0xa5;
    serial_port_->write(get_str);
}

void Flywheel4NMDriver::resolveDataFromSerialport()
{
    QByteArray tmp_data = serial_port_->readAll();
    for (int i = 0;i < tmp_data.size();++i){
        recv_data_buf.push_back(tmp_data.at(i));
        //a full frame detected
        if (recv_data_buf.size() >= 7){
            //check frame
            uchar t = 0x00;
            for (uint i = 0; i < 6;++i){
                t += recv_data_buf[i];
            }
            if (t != recv_data_buf[6]){
                recv_data_buf = recv_data_buf.mid(1);
                emit sendErrText("recv message error! chack communication!");
                return;
            }



            recv_spd_.array[1] = recv_data_buf[0];
            recv_spd_.array[0] = recv_data_buf[1];

            emit sendMotorSpd(recv_spd_.spd * 0.05);

            recv_cur_.array[1] = recv_data_buf[2];
            recv_cur_.array[0] = recv_data_buf[3];
            emit sendMotorCur(recv_cur_.cur * 0.0017);

            uchar tmp = recv_data_buf[4];
            emit sendMotorTmp(tmp);

            uchar status = recv_data_buf[5];
            emit sendMotorStatus(status);

            if (recv_data_buf.size() == 7){
                recv_data_buf.clear();
            }
            else{
                recv_data_buf = recv_data_buf.mid(7);
            }
        }

    }

}
