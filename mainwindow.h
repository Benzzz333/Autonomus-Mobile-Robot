#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QVector>
#include <QElapsedTimer>

#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui {class MainWindow;}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onDisconnectClicked();

    void onManualClicked();
    void onAutoClicked();

    void sendForward();
    void sendBackward();
    void sendLeft();
    void sendRight();
    void sendStop();

    void onConnected();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onReadyRead();

    void onSaveCsvClicked();
    void onScreenshotClicked();

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;

    bool autoMode = false;
    bool manualMode = false;
    bool recording = false;

    QByteArray receiveBuffer;

    // ===== POST-MORTEM ANALYSIS DATA =====
    QVector<double> timeData;
    QVector<double> frontData;
    QVector<double> rearData;
    QVector<double> laserData;
    QVector<double> steerData; // Tambahkan ini
    QVector<double> speedData; // Tambahkan ini

    // === VARIABEL STATISTIK (BARU) ===
    double minLaserData = 9999.0;
    double maxSpeedData = 0.0;
    double sumLaserData = 0.0;
    int totalSamples = 0;

    QElapsedTimer sessionTimer;

    void sendCommand(const QString &cmd);
    void setManualButtonsEnabled(bool enabled);
    void setStatus(const QString &text);
    //Dibawah nih Parameternya jadi 5
    void setupPlot();
    void startPostMortemSession();
    void recordSensorData(double f, double r, double l, double steer, double speed);
    void showPostMortemAnalysis();
};

#endif
