#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QtCharts>
#include <QFile>
#include <QTextStream>

#include "client.h"
#include "server.h"

QT_USE_NAMESPACE // 네임스페이스 오류 방지

    QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateSensorData();
    void on_forceXSlider_valueChanged(int value);
    void on_forceYSlider_valueChanged(int value);
    void on_forceZSlider_valueChanged(int value);
    void on_accelerationSlider_valueChanged(int value);
    void on_dataReceived(QString data);
    void on_resetButton_clicked(); // 재시작 버튼

private:
    Ui::MainWindow *ui;
    QTimer *timer;
    QFile *logFile;

    // CSV 읽기용 추가 변수
    QFile *inputFile = nullptr;
    QTextStream *inputStream = nullptr;
    bool isCsvMode = true;

    Server *server = nullptr;
    Client *client = nullptr;

    QLineSeries *vibrationSeries = nullptr;
    QLineSeries *forceSeriesX = nullptr;
    QLineSeries *forceSeriesY = nullptr;
    QLineSeries *forceSeriesZ = nullptr;
    QLineSeries *accelerationSeries = nullptr;
    QSplineSeries *temperatureSeries = nullptr;

    double vibration = 0, forceX = 0, forceY = 0, forceZ = 0, acceleration = 0;
    int currentTime = 0;
    double temperature = 20.0;

    QDateTime lastVibWarningTime; // 마지막 진동 경고 발생 시각
    QDateTime lastTempWarningTime; // 마지막 온도 경고 발생 시각

};

#endif // MAINWINDOW_H
