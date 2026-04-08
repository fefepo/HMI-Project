#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QRandomGenerator>
#include <QVBoxLayout>
#include <QDebug>
#include <QMessageBox>
#include <QtCharts/QSplineSeries>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentTime(0)
    , vibration(0), forceX(0), forceY(0), forceZ(0), acceleration(0)
    , temperature(20.0)
{
    ui->setupUi(this);

    // 1. 로그 파일 초기화
    QString filename = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "_log.csv";
    logFile = new QFile(filename, this);

    if (logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(logFile);
        out << "Time,ForceX,ForceY,ForceZ,Acceleration,Vibration,Temperature,Status\n";
    }

    server = new Server(this);
    client = new Client(this);
    timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &MainWindow::updateSensorData);
    connect(server, &Server::dataReceived, this, &MainWindow::on_dataReceived);

    timer->start(20);

    // 2. 차트 설정
    QChart *chart = new QChart();
    chart->setTitle("장비 통합 제어 HMI");
    chart->legend()->setAlignment(Qt::AlignRight);

    vibrationSeries = new QSplineSeries();
    vibrationSeries->setName("진동 (검정)");
    vibrationSeries->setPen(QPen(Qt::black, 1));

    forceSeriesX = new QSplineSeries();
    forceSeriesX->setName("압력X (빨강)");
    forceSeriesX->setPen(QPen(Qt::red, 2));

    forceSeriesY = new QSplineSeries();
    forceSeriesY->setName("압력Y (초록)");
    forceSeriesY->setPen(QPen(QColor("#228B22"), 2));

    forceSeriesZ = new QSplineSeries();
    forceSeriesZ->setName("압력Z (파랑)");
    forceSeriesZ->setPen(QPen(Qt::blue, 2));

    accelerationSeries = new QSplineSeries();
    accelerationSeries->setName("가속도 (노랑)");
    accelerationSeries->setPen(QPen(QColor("#EEB400"), 3));

    temperatureSeries = new QSplineSeries();
    temperatureSeries->setName("온도 (주황)");
    temperatureSeries->setPen(QPen(QColor(255, 128, 0), 4));

    chart->addSeries(vibrationSeries);
    chart->addSeries(forceSeriesX);
    chart->addSeries(forceSeriesY);
    chart->addSeries(forceSeriesZ);
    chart->addSeries(accelerationSeries);
    chart->addSeries(temperatureSeries);

    chart->createDefaultAxes();
    if (!chart->axes(Qt::Vertical).isEmpty()) chart->axes(Qt::Vertical).first()->setRange(0, 120);
    if (!chart->axes(Qt::Horizontal).isEmpty()) chart->axes(Qt::Horizontal).first()->setRange(0, 500);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    if (ui->chartWidget) {
        if (ui->chartWidget->layout()) delete ui->chartWidget->layout();
        QVBoxLayout *layout = new QVBoxLayout(ui->chartWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(chartView);
        ui->chartWidget->setLayout(layout);
    }

    // 3. UI 라벨 스타일 (진동/온도 강조)
    ui->vibrationLabel->setStyleSheet("color: black; font-weight: bold; font-size: 14pt;");
    ui->forceXLabel->setStyleSheet("color: red; font-weight: bold;");
    ui->forceYLabel->setStyleSheet("color: green; font-weight: bold;");
    ui->forceZLabel->setStyleSheet("color: blue; font-weight: bold;");
    ui->accelerationLabel->setStyleSheet("color: #D4A017; font-weight: bold;");
    ui->tempLabel->setStyleSheet("color: #FF8000; font-weight: bold; font-size: 14pt;");
}

MainWindow::~MainWindow()
{
    if (logFile && logFile->isOpen()) logFile->close();
    delete ui;
}

void MainWindow::updateSensorData()
{
    // 1. 데이터 계산
    double average = (forceX + forceY + forceZ + acceleration) / 4.0;
    vibration = (average * 0.95) + QRandomGenerator::global()->bounded(0, 11);

    double targetTemp = 25.0 + (average * 0.7);
    temperature += (targetTemp - temperature) * 0.005;

    // 2. 인터락 체크 (핵심 수정: 원인별 메시지 분리)
    if (average >= 90.0 || temperature >= 85.0) {
        timer->stop();

        QString errorMsg;
        QString logStatus;

        if (temperature >= 85.0) {
            // [원인: 과열]
            logStatus = "OVERHEAT";
            errorMsg = QString("장비 과열(Overheat) 인터락 발생!\n현재 온도: %1°C (기준: 85.0°C)").arg(temperature, 0, 'f', 1);
        } else {
            // [원인: 과부하] - 이제 온도 타령 안 함
            logStatus = "OVERLOAD";
            errorMsg = QString("시스템 과부하(Overload) 인터락 발생!\n현재 평균 부하: %1 (기준: 90.0)").arg(average, 0, 'f', 1);
        }

        // 즉시 로그 기록
        if (logFile->isOpen()) {
            QTextStream out(logFile);
            out << currentTime << "," << forceX << "," << forceY << "," << forceZ << ","
                << acceleration << "," << vibration << "," << temperature << "," << logStatus << "\n";
            out.flush();
            logFile->close();
        }

        QMessageBox::critical(this, "⚠️ EMERGENCY STOP", errorMsg);
        return;
    }

    // 3. 3초 단위 정기 로깅 (20ms * 150 = 3,000ms)
    if (currentTime > 0 && currentTime % 150 == 0) {
        if (logFile->isOpen()) {
            QTextStream out(logFile);
            out << currentTime << "," << forceX << "," << forceY << "," << forceZ << ","
                << acceleration << "," << vibration << "," << temperature << ",NORMAL\n";
            out.flush();
            qDebug() << "3-second log saved at" << QDateTime::currentDateTime().toString("hh:mm:ss");
        }
    }

    // 4. 차트 및 UI 업데이트
    vibrationSeries->append(currentTime, vibration);
    forceSeriesX->append(currentTime, forceX);
    forceSeriesY->append(currentTime, forceY);
    forceSeriesZ->append(currentTime, forceZ);
    accelerationSeries->append(currentTime, acceleration);
    temperatureSeries->append(currentTime, temperature);

    currentTime++;
    if (currentTime > 500) {
        vibrationSeries->chart()->axes(Qt::Horizontal).first()->setRange(currentTime - 500, currentTime);
    }

    ui->vibrationLabel->setText(QString::number(vibration, 'f', 1));
    ui->forceXLabel->setText(QString::number(forceX));
    ui->forceYLabel->setText(QString::number(forceY));
    ui->forceZLabel->setText(QString::number(forceZ));
    ui->accelerationLabel->setText(QString::number(acceleration));
    ui->tempLabel->setText(QString::number(temperature, 'f', 1) + " °C");
}

void MainWindow::on_accelerationSlider_valueChanged(int value) { acceleration = value; }
void MainWindow::on_forceXSlider_valueChanged(int value) { forceX = value; }
void MainWindow::on_forceYSlider_valueChanged(int value) { forceY = value; }
void MainWindow::on_forceZSlider_valueChanged(int value) { forceZ = value; }
void MainWindow::on_dataReceived(QString data) { qDebug() << "Recv:" << data; }