/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
// #include "console.h"
#include "settingsdialog.h"

#include <QLabel>
#include <QMessageBox>

class pvtelegram
{
    QByteArray buffer_;
public:
    pvtelegram() {}

    void generate_read(int addr, int par)
    {
        buffer_.clear();
        buffer_ = QString("%1").arg(addr,3,10,QChar('0')).toLatin1(); // adress: 000 Global, 001, 002, etc indv
        buffer_ += QByteArray("00");
        buffer_ += QString("%1").arg(par,3,10,QChar('0')).toLatin1(); // parameter: zero padded int, e.g. 033
        buffer_ += QByteArray("02=?");
        int chksum = 0;
        for(int i=0; i<buffer_.length(); i++) chksum += buffer_.at(i);
        chksum %= 256;
        buffer_ += QString("%1\n").arg(chksum,3,10,QChar('0')).toLatin1(); // checksum: zero padded int <256, e.g. 033
    }
    void generate_write(int addr, int par, const QByteArray& data)
    {
        buffer_.clear();
        buffer_ = QString("%1").arg(addr,3,10,QChar('0')).toLatin1(); // adress: 000 Global, 001, 002, etc indv
        buffer_ += QByteArray("10");
        buffer_ += QString("%1").arg(par,3,10,QChar('0')).toLatin1(); // parameter: zero padded int, e.g. 033
        buffer_ += QString("%1").arg(data.length(),2,10,QChar('0')).toLatin1(); // data length: zero padded int, e.g. 03
        buffer_ += data;
        int chksum = 0;
        for(int i=0; i<buffer_.length(); i++) chksum += buffer_.at(i);
        chksum %= 256;
        buffer_ += QString("%1\n").arg(chksum,3,10,QChar('0')).toLatin1(); // checksum: zero padded int <256, e.g. 033
    }
    QByteArray get() const
    {
        return buffer_;
    }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow),
    m_status(new QLabel),
    m_settings(new SettingsDialog),
    m_serial(new QSerialPort(this)),
    m_telegram(new pvtelegram())
{
    m_ui->setupUi(this);

    //m_console->setEnabled(false);
    //setCentralWidget(m_console);
    //centralWidget()->setEnabled(false);


    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionQuit->setEnabled(true);
    m_ui->actionConfigure->setEnabled(true);

    m_ui->statusBar->addWidget(m_status);

    initActionsConnections();

    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);


    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);

    //connect(m_ui->btSend, SIGNAL(), &Console::getData, this, &MainWindow::writeData);
    connect(m_ui->btSend, SIGNAL(pressed()), this, SLOT(writeData()));
    connect(m_ui->btUpdateRaw, SIGNAL(pressed()), this, SLOT(updateTelegram()));
}


MainWindow::~MainWindow()
{
    delete m_settings;
    delete m_ui;
}

//! [4]
void MainWindow::openSerialPort()
{
    const SettingsDialog::Settings p = m_settings->settings();
    m_serial->setPortName(p.name);
    m_serial->setBaudRate(p.baudRate);
    m_serial->setDataBits(p.dataBits);
    m_serial->setParity(p.parity);
    m_serial->setStopBits(p.stopBits);
    m_serial->setFlowControl(p.flowControl);
    if (m_serial->open(QIODevice::ReadWrite)) {
        centralWidget()->setEnabled(true);

        m_ui->actionConnect->setEnabled(false);
        m_ui->actionDisconnect->setEnabled(true);
        m_ui->actionConfigure->setEnabled(false);
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
    } else {
        QMessageBox::critical(this, tr("Error"), m_serial->errorString());

        showStatusMessage(tr("Open error"));
    }
}
//! [4]

//! [5]
void MainWindow::closeSerialPort()
{
    if (m_serial->isOpen())
        m_serial->close();
    centralWidget()->setEnabled(false);
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionConfigure->setEnabled(true);
    showStatusMessage(tr("Disconnected"));

}
//! [5]

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Simple Terminal"),
                       tr("The <b>Simple Terminal</b> example demonstrates how to "
                          "use the Qt Serial Port module in modern GUI applications "
                          "using Qt, with a menu bar, toolbars, and a status bar."));
}

void MainWindow::clearCom()
{
    m_serial->clear();
}

void MainWindow::updateTelegram()
{
    if (m_ui->btRead->isChecked())
        m_telegram->generate_read(m_ui->sbAddress->value(), m_ui->sbParameter->value());
    else
        m_telegram->generate_write(m_ui->sbAddress->value(), m_ui->sbParameter->value(), m_ui->lnData->text().toLatin1());
    m_ui->lnRawMaster->setText(m_telegram->get());
}

//! [6]
void MainWindow::writeData()
{
    updateTelegram();
    m_serial->write(m_telegram->get());
}
//! [6]

//! [7]
void MainWindow::readData()
{
    const QByteArray data = m_serial->readAll();
    //m_console->putData(data);
    m_ui->lnRawSlave->setText(data);
}
//! [7]

//! [8]
void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
        closeSerialPort();
    }
}
//! [8]

void MainWindow::initActionsConnections()
{
    connect(m_ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(m_ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(m_ui->actionConfigure, &QAction::triggered, m_settings, &SettingsDialog::show);
    connect(m_ui->actionClear, &QAction::triggered, this, &MainWindow::clearCom);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(m_ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::showStatusMessage(const QString &message)
{
    m_status->setText(message);
}
