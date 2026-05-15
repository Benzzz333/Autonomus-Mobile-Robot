#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPushButton>
#include <QHostAddress>
#include <QDebug>
#include <QPen>
#include <QBrush>
#include <QColor>

#include "qcustomplot.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket(new QTcpSocket(this))
{
    ui->setupUi(this);

    setupPlot();

    // Untuk ESP32 Access Point
    ui->ipEdit->setText("192.168.4.1");
    ui->portEdit->setText("8888");
    ui->statusLabel->setText("Disconnected");

    ui->auto_2->setText("AUTO");
    ui->manualButton->setText("MANUAL");
    ui->Stop->setText("Stop");

    setManualButtonsEnabled(false);

    connect(ui->connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);

    connect(ui->disconnectButton, &QPushButton::clicked,
            this, &MainWindow::onDisconnectClicked);

    connect(ui->manualButton, &QPushButton::clicked,
            this, &MainWindow::onManualClicked);

    connect(ui->auto_2, &QPushButton::clicked,
            this, &MainWindow::onAutoClicked);

    // Manual control:
    // Klik tombol arah = jalan terus
    // Klik Stop = berhenti dan tampilkan post-mortem
    connect(ui->Maju, &QPushButton::clicked,
            this, &MainWindow::sendForward);

    connect(ui->Mundur, &QPushButton::clicked,
            this, &MainWindow::sendBackward);

    connect(ui->Kiri, &QPushButton::clicked,
            this, &MainWindow::sendLeft);

    connect(ui->Kanan, &QPushButton::clicked,
            this, &MainWindow::sendRight);

    connect(ui->Stop, &QPushButton::clicked,
            this, &MainWindow::sendStop);

    connect(socket, &QTcpSocket::connected,
            this, &MainWindow::onConnected);

    connect(socket, &QTcpSocket::disconnected,
            this, &MainWindow::onDisconnected);

    connect(socket, &QTcpSocket::readyRead,
            this, &MainWindow::onReadyRead);

    connect(socket, &QTcpSocket::errorOccurred,
            this, &MainWindow::onSocketError);

    // Sambungkan tombol ke fungsi Export
    connect(ui->btnCSV, &QPushButton::clicked, this, &MainWindow::onSaveCsvClicked);
    connect(ui->btnScreenshot, &QPushButton::clicked, this, &MainWindow::onScreenshotClicked);
}

MainWindow::~MainWindow()
{
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->write("S");
        socket->flush();
        socket->disconnectFromHost();
    }

    delete ui;
}

void MainWindow::setupPlot() {
    // Bersihkan grafik jika ada
    ui->widget->clearGraphs();

    // Graph 0: Front US (Merah)
    ui->widget->addGraph();
    ui->widget->graph(0)->setPen(QPen(Qt::red));
    ui->widget->graph(0)->setName("Front US (cm)");

    // Graph 1: Rear US (Hijau)
    ui->widget->addGraph();
    ui->widget->graph(1)->setPen(QPen(Qt::green));
    ui->widget->graph(1)->setName("Rear US (cm)");

    // Graph 2: Laser (Biru)
    ui->widget->addGraph();
    ui->widget->graph(2)->setPen(QPen(Qt::blue));
    ui->widget->graph(2)->setName("Laser (mm)");

    // Graph 3: Steer (Oranye) - DATA BARU
    ui->widget->addGraph();
    ui->widget->graph(3)->setPen(QPen(QColor(255, 165, 0)));
    ui->widget->graph(3)->setName("Steer Value");

    // Graph 4: Speed (Hitam) - DATA BARU
    ui->widget->addGraph();
    ui->widget->graph(4)->setPen(QPen(Qt::black));
    ui->widget->graph(4)->setName("Speed PWM");

    // Konfigurasi Axis
    ui->widget->xAxis->setLabel("Time (seconds)");
    ui->widget->yAxis->setLabel("Value");
    ui->widget->yAxis->setRange(-255, 600); // Agar semua data (termasuk steer -255) kelihatan
    ui->widget->legend->setVisible(true);

    // Agar grafik bisa di-drag dan zoom pakai mouse (Opsional)
    ui->widget->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
}

void MainWindow::startPostMortemSession()
{
    timeData.clear();
    frontData.clear();
    rearData.clear();
    laserData.clear();

    sessionTimer.restart();
    recording = true;

    ui->widget->graph(0)->setData(timeData, frontData);
    ui->widget->graph(1)->setData(timeData, rearData);
    ui->widget->graph(2)->setData(timeData, laserData);

    ui->widget->xAxis->setRange(0, 10);
    ui->widget->yAxis->setRange(0, 150);
    ui->widget->replot();

    qDebug() << "Post-mortem recording started";
}

void MainWindow::recordSensorData(double f, double r, double l, double steer, double speed) {
    if (!sessionTimer.isValid()) sessionTimer.start();
    double secs = sessionTimer.elapsed() / 1000.0;

    // 1. Simpan ke Memori (Untuk Post-Mortem Analysis)
    timeData.append(secs);
    frontData.append(f);
    rearData.append(r);
    laserData.append(l);
    steerData.append(steer);
    speedData.append(speed);

    // === KALKULASI STATISTIK REAL-TIME ===
    totalSamples++;
    if (l < minLaserData && l > 0) minLaserData = l; // Cari jarak terdekat yg valid
    if (speed > maxSpeedData) maxSpeedData = speed;  // Cari Top Speed
    sumLaserData += l;

    double avgLaser = sumLaserData / totalSamples;

    // Tampilkan di UI (Bikin Dosen Kagum!)
    ui->lblStats->setText(QString("📊 STATS | Jarak Terdekat: %1 mm | Rata-rata Jarak: %2 mm | Top Speed: %3 PWM | Total Data: %4")
                              .arg(minLaserData).arg(avgLaser, 0, 'f', 1).arg(maxSpeedData).arg(totalSamples));




    // 2. Tambahkan ke Grafik Real-time
    ui->widget->graph(0)->addData(secs, f);
    ui->widget->graph(1)->addData(secs, r);
    ui->widget->graph(2)->addData(secs, l);
    ui->widget->graph(3)->addData(secs, steer);
    ui->widget->graph(4)->addData(secs, speed);

    // 3. Autoscroll: Geser grafik otomatis ke kanan (tampilan 8 detik terakhir)
    ui->widget->xAxis->setRange(secs, 8, Qt::AlignRight);
    ui->widget->replot();
}

void MainWindow::showPostMortemAnalysis() {
    if (timeData.isEmpty()) return;

    double totalTime = timeData.last();
    int dataCount = timeData.size();

    // Bisa munculkan di kotak pesan (MessageBox)
    QString report = QString("Post-Mortem Report:\n"
                             "Total Duration: %1 seconds\n"
                             "Samples Collected: %2\n"
                             "Final Mode: %3")
                         .arg(totalTime).arg(dataCount).arg(autoMode ? "AUTO" : "MANUAL");

    setStatus(report);
}

void MainWindow::setStatus(const QString &text)
{
    ui->statusLabel->setText(text);
}

void MainWindow::setManualButtonsEnabled(bool enabled)
{
    ui->Maju->setEnabled(enabled);
    ui->Mundur->setEnabled(enabled);
    ui->Kiri->setEnabled(enabled);
    ui->Kanan->setEnabled(enabled);
    ui->Stop->setEnabled(enabled);
}

void MainWindow::onConnectClicked()
{
    QString ip = ui->ipEdit->text().trimmed();
    quint16 port = ui->portEdit->text().toUShort();

    if (ip.isEmpty()) {
        setStatus("Isi IP ESP32 dulu");
        return;
    }

    if (port == 0) {
        setStatus("Port salah");
        return;
    }

    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->abort();
    }

    receiveBuffer.clear();

    setStatus("Connecting...");
    socket->connectToHost(QHostAddress(ip), port);
}

void MainWindow::onDisconnectClicked()
{
    if (socket->state() == QAbstractSocket::ConnectedState) {
        sendCommand("S");
        showPostMortemAnalysis();
        socket->disconnectFromHost();
        setStatus("Disconnecting... Post-mortem ready");
    } else {
        showPostMortemAnalysis();
        setStatus("Already disconnected");
    }

    autoMode = false;
    manualMode = false;
    setManualButtonsEnabled(false);
}

void MainWindow::onConnected()
{
    autoMode = false;
    manualMode = false;

    setManualButtonsEnabled(false);
    setStatus("Connected to ESP32");

    // Saat baru connect, robot dibuat stop dulu
    sendCommand("S");
}

void MainWindow::onDisconnected()
{
    autoMode = false;
    manualMode = false;

    setManualButtonsEnabled(false);
    setStatus("Disconnected");
}

void MainWindow::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);

    autoMode = false;
    manualMode = false;

    setManualButtonsEnabled(false);
    setStatus("Connection error: " + socket->errorString());
}

void MainWindow::sendCommand(const QString &cmd)
{
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(cmd.toUtf8());
        socket->flush();
        qDebug() << "Send:" << cmd;
    } else {
        setStatus("Belum connect ke ESP32");
    }
}

void MainWindow::onManualClicked()
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        setStatus("Connect ESP32 dulu");
        return;
    }

    autoMode = false;
    manualMode = true;

    startPostMortemSession();

    sendCommand("M");
    sendCommand("S");

    setManualButtonsEnabled(true);
    setStatus("Connected | MANUAL mode | Recording data");
}

void MainWindow::onAutoClicked()
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        setStatus("Connect ESP32 dulu");
        return;
    }

    autoMode = true;
    manualMode = false;

    startPostMortemSession();

    setManualButtonsEnabled(false);

    sendCommand("A");

    setStatus("Connected | AUTO mode | Recording data");
}

void MainWindow::sendForward()
{
    if (!manualMode) {
        setStatus("Klik MANUAL dulu");
        return;
    }

    sendCommand("F");
    setStatus("Connected | MANUAL | MAJU | Recording data");
}

void MainWindow::sendBackward()
{
    if (!manualMode) {
        setStatus("Klik MANUAL dulu");
        return;
    }

    sendCommand("B");
    setStatus("Connected | MANUAL | MUNDUR | Recording data");
}

void MainWindow::sendLeft()
{
    if (!manualMode) {
        setStatus("Klik MANUAL dulu");
        return;
    }

    sendCommand("L");
    setStatus("Connected | MANUAL | KIRI | Recording data");
}

void MainWindow::sendRight()
{
    if (!manualMode) {
        setStatus("Klik MANUAL dulu");
        return;
    }

    sendCommand("R");
    setStatus("Connected | MANUAL | KANAN | Recording data");
}

void MainWindow::sendStop()
{
    if (!manualMode) {
        setStatus("Klik MANUAL dulu");
        return;
    }

    sendCommand("S");
    showPostMortemAnalysis();

    setStatus("Connected | MANUAL | STOP | Post-mortem ready");
}

void MainWindow::onReadyRead() {
    while (socket->canReadLine()) {
        QString text = QString::fromUtf8(socket->readLine()).trimmed();

        if (text.startsWith("DATA")) {
            QStringList parts = text.split(",");

            // Kita pastikan format 7 bagian: DATA(0), f(1), r(2), l(3), steer(4), speed(5), mode(6)
            if (parts.size() >= 7) {
                double fUS   = parts[1].toDouble();
                double rUS   = parts[2].toDouble();
                double lsr   = parts[3].toDouble();
                double steer = parts[4].toDouble();
                double speed = parts[5].toDouble();
                QString mode = parts[6];

                // Tampilkan di UI dengan kata-kata lengkap
                setStatus(
                    "Connected | Mode: " + mode +
                    " | Front: " + QString::number(fUS, 'f', 1) + " cm" +
                    " | Rear: " + QString::number(rUS, 'f', 1) + " cm" +
                    " | Laser: " + QString::number(lsr, 'f', 0) + " mm" +
                    " | Steer: " + QString::number(steer) +
                    " | Speed: " + QString::number(speed)
                    );

                // === LOGIKA INDIKATOR ERROR (WARNING SYSTEM) ===
                QString alertMsg = "";
                QString style = "color: black;"; // Default hitam

                if (lsr <= 0 || lsr >= 8000) {
                    alertMsg = " [🚨 ERR: LASER BLIND!]";
                    style = "color: red; font-weight: bold;";
                } else if (qAbs(steer) >= 250) {
                    alertMsg = " [⚠️ WARN: HARD STEER!]";
                    style = "color: orange; font-weight: bold;";
                }

                // Terapkan warna ke label status
                ui->statusLabel->setStyleSheet(style);
                setStatus("Conn | Mode: " + mode + " | Lsr: " + QString::number(lsr) + alertMsg);

                // Rekam data ke grafik & memori
                recordSensorData(fUS, rUS, lsr, steer, speed);
            }
        }
        else if (text.startsWith("MODE:AUTO")) {
            autoMode = true; manualMode = false;
            setManualButtonsEnabled(false);
            setStatus("Connected | AUTO mode active");
        }
        else if (text.startsWith("MODE:MANUAL")) {
            autoMode = false; manualMode = true;
            setManualButtonsEnabled(true);
            setStatus("Connected | MANUAL mode active");
        }
    }
}

// === FUNGSI 1: SAVE BLACKBOX TO CSV ===
void MainWindow::onSaveCsvClicked() {
    if (timeData.isEmpty()) {
        QMessageBox::warning(this, "Kosong", "Belum ada data untuk disimpan!");
        return;
    }

    // Buka jendela dialog save file
    QString fileName = QFileDialog::getSaveFileName(this, "Save Post-Mortem Log",
                                                    "Robot_Blackbox_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".csv",
                                                    "CSV Files (*.csv)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            // Bikin Header Kolom Excel
            out << "Time(s),Front_US(cm),Rear_US(cm),Laser(mm),Steer(PWM),Speed(PWM)\n";

            // Tulis semua data loop
            for (int i = 0; i < timeData.size(); ++i) {
                out << timeData[i] << "," << frontData[i] << "," << rearData[i] << ","
                    << laserData[i] << "," << steerData[i] << "," << speedData[i] << "\n";
            }
            file.close();
            QMessageBox::information(this, "Sukses", "Data Blackbox berhasil diekspor ke CSV!");
        }
    }
}

// === FUNGSI 2: INSTANT SCREENSHOT ===
void MainWindow::onScreenshotClicked() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Screenshot",
                                                    "Graph_Snap_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".png",
                                                    "Images (*.png)");
    if (!fileName.isEmpty()) {
        // QCustomPlot punya fitur sakti untuk langsung save ke PNG!
        if (ui->widget->savePng(fileName, 800, 600)) {
            QMessageBox::information(this, "Sukses", "Screenshot grafik berhasil disimpan!");
        } else {
            QMessageBox::critical(this, "Gagal", "Gagal menyimpan screenshot.");
        }
    }
}
