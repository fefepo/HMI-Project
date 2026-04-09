#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QRandomGenerator>
#include <QVBoxLayout>
#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentTime(0)
    , vibration(0), forceX(0), forceY(0), forceZ(0), acceleration(0)
    , temperature(20.0)
{
    ui->setupUi(this);
    connect(ui->resetButton, &QPushButton::clicked, this, &MainWindow::on_resetButton_clicked);


    // 1. 로그 파일 초기화
    QString filename = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "_log.csv";
    logFile = new QFile(filename, this);
    if (logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(logFile);
        out << "Time,ForceX,ForceY,ForceZ,Acceleration,Vibration,Temperature,Status\n";
    }

    //  읽어올 CSV 파일 열기
    inputFile = new QFile("sensor_data.csv");
    if (inputFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
        inputStream = new QTextStream(inputFile);
        inputStream->readLine(); // 헤더 스킵
    }

    server = new Server(this);
    client = new Client(this);
    timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &MainWindow::updateSensorData);
    connect(server, &Server::dataReceived, this, &MainWindow::on_dataReceived);

    timer->start(20); // 20ms 주기 유지

    // 2. 차트 설정
    QChart *chart = new QChart();
    chart->setTitle("장비 통합 제어 HMI");
    chart->legend()->setAlignment(Qt::AlignRight);

    vibrationSeries = new QLineSeries();
    vibrationSeries->setName("진동 (검정)");
    vibrationSeries->setPen(QPen(Qt::black, 1));

    forceSeriesX = new QLineSeries();
    forceSeriesX->setName("압력X (빨강)");
    forceSeriesX->setPen(QPen(Qt::red, 2));

    forceSeriesY = new QLineSeries();
    forceSeriesY->setName("압력Y (초록)");
    forceSeriesY->setPen(QPen(QColor("#228B22"), 2));

    forceSeriesZ = new QLineSeries();
    forceSeriesZ->setName("압력Z (파랑)");
    forceSeriesZ->setPen(QPen(Qt::blue, 2));

    accelerationSeries = new QLineSeries();
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

    // 4. 초기 시간을 10초 전으로 설정하여 첫 경고는 즉시 뜨게 함
    lastVibWarningTime = QDateTime::currentDateTime().addSecs(-10);
    lastTempWarningTime = QDateTime::currentDateTime().addSecs(-10);

}

MainWindow::~MainWindow()
{
    if (logFile && logFile->isOpen()) logFile->close();
    if (inputFile && inputFile->isOpen()) inputFile->close();
    delete ui;
}

void MainWindow::updateSensorData()
{
    // [중요] CSV 파일에서 데이터를 읽어오는 로직
    if (inputStream && !inputStream->atEnd()) {
        QString line = inputStream->readLine();
        QStringList values = line.split(",");

        if (values.size() >= 4) {
            // [수정] 각 센서별로 수동 조작 중이 아닐 때만 파일 값 적용
            if (!manualForceX) forceX = values[0].toDouble();
            if (!manualForceY) forceY = values[1].toDouble();
            if (!manualForceZ) forceZ = values[2].toDouble();
            if (!manualAcceleration) acceleration = values[3].toDouble();
        }
    } else if (inputStream) {
        // 파일 끝에 도달하면 다시 처음으로 (반복 읽기)
        inputStream->device()->seek(0);
        inputStream->readLine();
    }

    // [수정] 수동 조작 시 미세 떨림 (값이 누적되지 않도록 슬라이더 값 기준 계산)
    if (manualForceX) {
        // -0.5 ~ 0.5 정도 노이즈
        double noise = (QRandomGenerator::global()->bounded(-50, 51)) / 100.0;
        forceX = ui->forceXSlider->value() + noise;
    }

    if (manualForceY) {
        double noise = (QRandomGenerator::global()->bounded(-50, 51)) / 100.0;
        forceY = ui->forceYSlider->value() + noise;
    }

    if (manualForceZ) {
        double noise = (QRandomGenerator::global()->bounded(-50, 51)) / 100.0;
        forceZ = ui->forceZSlider->value() + noise;
    }

    if (manualAcceleration) {
        double noise = (QRandomGenerator::global()->bounded(-50, 51)) / 100.0;
        acceleration = ui->accelerationSlider->value() + noise;
    }


    // 1. 데이터 계산
    double average = (forceX + forceY + forceZ + acceleration) / 4.0;
    vibration = (average * 0.95) + QRandomGenerator::global()->bounded(0, 11);
    double targetTemp = 25.0 + (average * 0.7);
    temperature += (targetTemp - temperature) * 0.005;


    // 현재 시각을 확인
    QDateTime currentTimeInstance = QDateTime::currentDateTime();

    // [추가] 1.5 워닝(Warning) 체크 - 시스템은 멈추지 않음
    // 진동 경고 (80 이상일 때)
    if (vibration >= 80.0) {
        // 마지막 경고로부터 5000ms(5초)가 지났는지 확인
        if (lastVibWarningTime.msecsTo(currentTimeInstance) >= 5000) {
            lastVibWarningTime = currentTimeInstance; // 시간 갱신 (이제부터 다시 5초 카운트)
            QMessageBox::warning(this, "⚠️ 경고", "진동 수치가 위험 수준(80.0)에 도달했습니다!");
        }
        ui->vibrationLabel->setStyleSheet("color: red; font-weight: bold; font-size: 16pt;");
    } else {
        ui->vibrationLabel->setStyleSheet("color: black; font-weight: bold; font-size: 14pt;");

    }

    // 온도 경고 (70 이상일 때)
    if (temperature >= 70.0 && temperature < 85.0) {
        if (lastTempWarningTime.msecsTo(currentTimeInstance) >= 5000) {
            lastTempWarningTime = currentTimeInstance; // 시간 갱신
            QMessageBox::warning(this, "⚠️ 경고", "온도가 주의 수준(70.0°C)에 도달했습니다!");
        }
        ui->tempLabel->setStyleSheet("color: red; font-weight: bold; font-size: 16pt;");
    } else if (temperature < 70.0) {
        ui->tempLabel->setStyleSheet("color: #FF8000; font-weight: bold; font-size: 14pt;");
    }

    // 2. 인터락 체크 (핵심: 개별 항목 검사)
    if (forceX >= 90.0 || forceY >= 90.0 || forceZ >= 90.0 || acceleration >= 90.0 || temperature >= 85.0 || vibration >= 90.0) {
        timer->stop();

        QString errorMsg;
        QString logStatus;

        if (temperature >= 85.0) {
            // [원인: 과열]
            logStatus = "OVERHEAT";
            errorMsg = QString("장비 과열(Overheat) 인터락 발생!\n현재 온도: %1°C (최대치: 85.0°C)").arg(temperature, 0, 'f', 1);
        }
        else {
            // [원인: 과부하] - 어떤 항목이 범인인지 특정
            logStatus = "OVERLOAD";

            QString sensorName;
            double sensorValue = 0.0;

            // 우선순위에 따라 가장 먼저 임계치를 넘은 항목을 찾음
            if (forceX >= 90.0) { sensorName = "X축 힘"; sensorValue = forceX; }
            else if (forceY >= 90.0) { sensorName = "Y축 힘"; sensorValue = forceY; }
            else if (forceZ >= 90.0) { sensorName = "Z축 힘"; sensorValue = forceZ; }
            else { sensorName = "가속도"; sensorValue = acceleration; }

            // "평균"을 빼고 구체적인 센서 명칭과 수치를 표시
            errorMsg = QString("시스템 과부하(Overload) 인터락 발생!\n"
                               "감지 항목: %1\n"
                               "현재 수치: %2 (최대치: 90.0)")
                           .arg(sensorName)
                           .arg(sensorValue, 0, 'f', 1);
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

    // 3. 정기 로깅
    if (currentTime > 0 && currentTime % 150 == 0) {
        if (logFile->isOpen()) {
            QTextStream out(logFile);
            out << currentTime << "," << forceX << "," << forceY << "," << forceZ << ","
                << acceleration << "," << vibration << "," << temperature << ",NORMAL\n";
            out.flush();
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

    // 라벨 및 슬라이더 위치 동기화 (CSV 모드일 때 슬라이더가 같이 움직이게 함)
    ui->vibrationLabel->setText(QString::number(vibration, 'f', 1));
    ui->forceXLabel->setText(QString::number(forceX, 'f', 1));
    ui->forceYLabel->setText(QString::number(forceY, 'f', 1));
    ui->forceZLabel->setText(QString::number(forceZ, 'f', 1));
    ui->accelerationLabel->setText(QString::number(acceleration, 'f', 1));
    ui->tempLabel->setText(QString::number(temperature, 'f', 1) + " °C");
}

void MainWindow::on_resetButton_clicked()
{
    qDebug() << "시스템 재시작 및 로그 기록 복구 중...";


    // 2. 쓰기용 로그 파일이 닫혀있다면 다시 열기
    // 인터락 발생 시 close()된 파일을 다시 Append(추가) 모드로 열기.
    if (logFile && !logFile->isOpen()) {
        if (logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            qDebug() << "로그 파일이 다시 연결되었습니다.";
        }
    }

    // 3. 읽기용 CSV 파일 포인터 처음으로
    if (inputStream) {
        inputStream->device()->seek(0);
        inputStream->readLine(); // 헤더 스킵
    }

    // 4. 시간 및 차트 데이터 리셋 (즉시 표시를 위해)
    currentTime = 0;
    vibrationSeries->clear();
    forceSeriesX->clear();
    forceSeriesY->clear();
    forceSeriesZ->clear();
    accelerationSeries->clear();
    temperatureSeries->clear();

    // 5. X축 범위 리셋
    if (!vibrationSeries->chart()->axes(Qt::Horizontal).isEmpty()) {
        vibrationSeries->chart()->axes(Qt::Horizontal).first()->setRange(0, 500);
    }

    // 6. 슬라이더 초기화
    ui->forceXSlider->setValue(0);
    ui->forceYSlider->setValue(0);
    ui->forceZSlider->setValue(0);
    ui->accelerationSlider->setValue(0);

    // 7. 스타일 초기화
    ui->vibrationLabel->setStyleSheet("color: black; font-weight: bold; font-size: 14pt;");
    ui->tempLabel->setStyleSheet("color: #FF8000; font-weight: bold; font-size: 14pt;");

    // 8. 모든 센서를 다시 CSV 모드로 복구
    manualForceX = false;
    manualForceY = false;
    manualForceZ = false;
    manualAcceleration = false;

    // 9. 시스템 재시작 (20ms 주기)
    if (timer && !timer->isActive()) {
        timer->start(20);
        qDebug() << "시스템이 재시작 시작되었습니다.";
    }
}

//  슬라이더를 건드리면 CSV 모드를 해제하여 수동 제어가 가능하게 함
void MainWindow::on_forceXSlider_valueChanged(int value) {
    forceX = value;
    manualForceX = true; // X축만 수동 모드로 전환
}
void MainWindow::on_forceYSlider_valueChanged(int value) {
    forceY = value;
    manualForceY = true; // Y축만 수동 모드로 전환
}
void MainWindow::on_forceZSlider_valueChanged(int value) {
    forceZ = value;
    manualForceZ = true; // Z축만 수동 모드로 전환
}
void MainWindow::on_accelerationSlider_valueChanged(int value) {
    acceleration = value;
    manualAcceleration = true; // 가속도만 수동 모드로 전환
}
void MainWindow::on_dataReceived(QString data) { qDebug() << "Recv:" << data; }
