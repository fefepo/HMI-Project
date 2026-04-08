#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QtCharts> // 필수 헤더

QT_USE_NAMESPACE // 네임스페이스 오류 방지

#include "client.h"
#include "server.h"

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

private:
    QFile *logFile;
    Ui::MainWindow *ui;
    QTimer *timer;

    Server *server = nullptr;
    Client *client = nullptr;

    // 가독성을 위해 각 시리즈 선언
    QLineSeries *vibrationSeries = nullptr;
    QLineSeries *forceSeriesX = nullptr;
    QLineSeries *forceSeriesY = nullptr;
    QLineSeries *forceSeriesZ = nullptr;
    QLineSeries *accelerationSeries = nullptr;
    QSplineSeries *temperatureSeries = nullptr;

    int vibration = 0, forceX = 0, forceY = 0, forceZ = 0, acceleration = 0;
    int currentTime = 0;
    double temperature = 0;
};

#endif // MAINWINDOW_H