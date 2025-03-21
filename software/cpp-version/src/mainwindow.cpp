#include <QStyleFactory>
#include <QPalette>
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), serialPort(nullptr), baudRate(0), isAcquiring(false), isPaused(false), plotManager(nullptr) {
    setWindowTitle("UART Scope");
    setupUi();
    setupSerial();
    
    plotManager = new PlotManager(graphicsView, CHANNELS, MAX_PLOT_POINTS, this);
    plotManager->setupPlot();
    
    colors = plotManager->getColors();
    plotDataItems = plotManager->getPlotItems();
    
    plotData.resize(CHANNELS);
    for (int i = 0; i < CHANNELS; ++i) {
        plotData[i].resize(MAX_PLOT_POINTS);
        plotData[i].fill(0);
    }
    
    xData.resize(MAX_PLOT_POINTS);
    for (int i = 0; i < MAX_PLOT_POINTS; ++i) {
        xData[i] = i;
    }
    
    pauseResumeButton->setEnabled(false);
    clearButton->setEnabled(false);
}

MainWindow::~MainWindow(void) {
    if (serialPort) {
        if (serialPort->isOpen()) {
            serialPort->close();
        }
        delete serialPort;
    }
    delete timer;

    serialScanTimer->stop();
    delete serialScanTimer;
}

void MainWindow::updateScaleValue(int value) {
    scaleXValueLabel->setText(QString("%1 points").arg(value));
}

void MainWindow::autoPosition(void) {
    plotManager->autoPosition();
}

void MainWindow::startSerialRead(void) {
    if (serialPort && baudRate > 0) {
        isAcquiring = true;
    }
}

void MainWindow::pauseResume(void) {
    isPaused = !isPaused;
    if (isPaused) {
        pauseResumeButton->setText("Resume");
        timer->stop();
        if (serialPort && serialPort->isOpen()) {
            serialPort->close();
        }
    } else {
        pauseResumeButton->setText("Pause");

        serialData.clear();

        if (serialPort && baudRate > 0) {
            if (serialPort->open(QIODevice::ReadWrite)) {
                timer->start();
                startSerialRead();
                isAcquiring = true;
            } else {
                QMessageBox::critical(this, "Serial Port Error", QString("Error reopening serial port: %1").arg(serialPort->errorString()));
                isPaused = true;
                pauseResumeButton->setText("Resume");
                return;
            }
        }
    }
}

void MainWindow::clearPlot(void) {
    for (int i = 0; i < CHANNELS; ++i) {
        plotData[i].fill(0, MAX_PLOT_POINTS);
    }
    
    QVector<bool> channelVisibility;
    for (int i = 0; i < CHANNELS; ++i) {
        channelVisibility.append(channelButtons[i]->isChecked());
    }
    
    int currentPlotLength = scaleXSlider->value();
    plotManager->updatePlotData(plotData, xData, currentPlotLength, channelVisibility);
}

void MainWindow::toggleChannel(int index, bool checked) {
    if (checked) {
        QString styleSheet = QString("background-color: %1; color: white;").arg(colors[index].name());
        channelButtons[index]->setStyleSheet(styleSheet);
        plotDataItems[index]->setVisible(true);
    } else {
        channelButtons[index]->setStyleSheet("background-color: rgb(45, 45, 45); color: white;");
        plotDataItems[index]->setVisible(false);
    }
    graphicsView->replot();
}

void MainWindow::selectBaudRate(int index) {
    if (index == 0) {
        baudRate = 0;
        startButton->setEnabled(false);
        return;
    }
    
    QString baudRateStr = baudRates->itemText(index);
    bool ok;
    int rate = baudRateStr.toInt(&ok);
    
    if (ok) {
        baudRate = rate;
    } else {
        qDebug() << "Error converting baud rate";
    }

    if (serialPort && serialPort->isOpen()) {
        serialPort->setBaudRate(baudRate);
        if (!isPaused) {
            timer->start();
        }
    }
    
    if (serialPorts->currentIndex() > 0) {
        startButton->setEnabled(true);
    }
}

void MainWindow::selectSerialPort(int index) {
    if (index == 0) {
        startButton->setEnabled(false);
        return;
    }
    
    QString portName = serialPorts->itemText(index);
    
    if (baudRate <= 0) {
        QMessageBox::warning(this, "Missing Baud Rate", "Please select a baud rate before selecting a serial port.");
        serialPorts->blockSignals(true);
        serialPorts->setCurrentIndex(0);
        serialPorts->blockSignals(false);
        return;
    }
    
    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }
    
    if (!serialPort) {
        serialPort = new QSerialPort(this);
    }
    
    serialPort->setPortName(portName);
    serialPort->setBaudRate(baudRate);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    
    startButton->setEnabled(true);
}

void MainWindow::startAcquisition(void) {
    if (!serialPort || baudRate <= 0) {
        QMessageBox::warning(this, "Missing Serial Port or Baud Rate", "Please select both a serial port and a baud rate before starting acquisition.");
        return;
    }
    
    if (!isAcquiring) {
        try {
            if (serialPort->isOpen()) {
                serialPort->close();
            }
            
            if (!serialPort->open(QIODevice::ReadWrite)) {
                QMessageBox::critical(this, "Serial Port Error", QString("Error opening serial port: %1").arg(serialPort->errorString()));
                stopAcquisition();
                return;
            }
            
            timer->start(33);
            startSerialRead();
            isAcquiring = true;
            
            pauseResumeButton->setEnabled(true);
            clearButton->setEnabled(true);
            
            serialData.clear();
            
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Serial Port Error", QString("Error: %1").arg(e.what()));
            stopAcquisition();
        }
    } else {
        QMessageBox::warning(this, "Already Acquiring", "Data acquisition is already running.");
    }
}

void MainWindow::stopAcquisition(void) {
    if (serialPort) {
        timer->stop();
        if (serialPort->isOpen()) {
            serialPort->close();
        }
        isAcquiring = false;
        pauseResumeButton->setEnabled(false);
        clearButton->setEnabled(false);
        
        plotManager->clearPlot();
        for (int i = 0; i < CHANNELS; ++i) {
            plotData[i].fill(0, MAX_PLOT_POINTS);
        }
    }
}

void MainWindow::applyDarkMode(void) {
    QColor accentColor = QColor(0, 120, 212);
    QColor darkBackground = QColor(30, 30, 30);
    QColor darkWidgetBg = QColor(45, 45, 45);
    QColor textColor = QColor(240, 240, 240);
    
    QString styleSheet = QString(
        "QWidget { background-color: %1; color: %2; }"
        "QLabel { background-color: transparent; }"
        "QGroupBox { font-weight: bold; border: 1px solid %4; border-radius: 6px; margin-top: 0.5em; padding-top: 0.5em; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; background-color: %1; }"
        "QComboBox { background-color: %3; border: 1px solid %4; border-radius: 4px; padding: 4px; min-height: 25px; }"
        "QComboBox::drop-down { border: 0px; width: 24px; }"
        "QComboBox::down-arrow { image: url(:/icons/down_arrow.png); width: 12px; height: 12px; }"
        "QComboBox QAbstractItemView { background-color: %3; border: 1px solid %4; selection-background-color: %5; }"
        "QPushButton { background-color: %3; border: 1px solid %4; border-radius: 4px; padding: 6px; min-height: 25px; }"
        "QPushButton:hover { background-color: %4; }"
        "QPushButton:pressed { background-color: %5; }"
        "QSlider::groove:horizontal { "
        "   height: 10px; background: %3; border: 1px solid %4; border-radius: 5px; }"
        "QSlider::handle:horizontal { "
        "   background: %5; border: 1px solid %4; width: 20px; height: 20px; "
        "   margin: -6px 0; border-radius: 10px; }"
        "QSlider::handle:horizontal:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %5, stop:1 #4080ff); }"
        "QSlider::sub-page:horizontal { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %5, stop:1 %3); "
        "   border: 1px solid %4; border-radius: 5px; }"
        "QStatusBar { background-color: %3; border-top: 1px solid %4; }"
    ).arg(darkBackground.name(), textColor.name(), darkWidgetBg.name(), QColor(70, 70, 70).name(), accentColor.name());
    
    setStyleSheet(styleSheet);
}

void MainWindow::updatePlotData(void) {
    QVector<bool> channelVisibility;
    for (int i = 0; i < CHANNELS; ++i) {
        channelVisibility.append(channelButtons[i]->isChecked());
    }
    
    int currentPlotLength = scaleXSlider->value();
    plotManager->updatePlotData(plotData, xData, currentPlotLength, channelVisibility);
}

void MainWindow::updatePlot(void) {
    if (isAcquiring && !isPaused && serialPort && serialPort->isOpen()) {
        try {
            QByteArray newData = serialPort->readAll();
            if (!newData.isEmpty()) {
                serialData.append(newData);
                
                if (serialData.size() > 100000) {
                    serialData = serialData.right(50000);
                }
                
                QList<QByteArray> lines = serialData.split('\n');
                
                if (lines.size() > 1) {
                    int batchSize = qMin(10, lines.size() - 1);

                    for (int i = 0; i < CHANNELS; ++i) {
                        for (int j = 0; j < MAX_PLOT_POINTS - batchSize; ++j) {
                            plotData[i][j] = plotData[i][j + batchSize];
                        }
                    }
                    
                    for (int l = 0; l < batchSize; ++l) {
                        QByteArray line = lines[l].trimmed();
                        if (!line.isEmpty()) {
                            QList<QByteArray> parts = line.split('\t');
                            if (parts.size() == CHANNELS) {
                                for (int i = 0; i < CHANNELS; ++i) {
                                    bool ok;
                                    int value = parts[i].toInt(&ok);
                                    if (ok) {
                                        plotData[i][MAX_PLOT_POINTS - batchSize + l] = value;
                                    }
                                }
                            }
                        }
                    }
                    
                    for (int i = 0; i < batchSize; ++i) {
                        lines.removeFirst();
                    }
                    serialData = lines.join('\n');
                    
                    static QElapsedTimer plotTimer;
                    if (!plotTimer.isValid() || plotTimer.elapsed() > 33) {
                        plotTimer.restart();
                        updatePlotData();
                    }
                }
            }
        } catch (const std::exception& e) {
            qDebug() << "Error updating plot:" << e.what();
        }
    }
}

void MainWindow::setupSerial(void) {
    serialPort = nullptr;
    baudRate = 0;
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updatePlot);
    timer->setInterval(33);
    serialData.clear();
    
    serialScanTimer = new QTimer(this);
    connect(serialScanTimer, &QTimer::timeout, this, &MainWindow::scanSerialPorts);
    serialScanTimer->start(1000);
}

void MainWindow::scanSerialPorts() {
    int currentIndex = serialPorts->currentIndex();
    QString currentPort = currentIndex > 0 ? serialPorts->itemText(currentIndex) : "";
    
    QStringList currentPorts;
    for (int i = 1; i < serialPorts->count(); i++) {
        currentPorts << serialPorts->itemText(i);
    }
    
    QStringList availablePorts;
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        availablePorts << port.portName();
    }
    
    if (currentPorts.size() == availablePorts.size()) {
        bool same = true;
        for (const QString &port : availablePorts) {
            if (!currentPorts.contains(port)) {
                same = false;
                break;
            }
        }
        if (same) return;
    }
    
    bool wasEmpty = serialPorts->count() <= 1;
    
    serialPorts->clear();
    serialPorts->addItem("Select port...");
    for (const QString &port : availablePorts) {
        serialPorts->addItem(port);
    }
    
    if (!currentPort.isEmpty()) {
        int index = serialPorts->findText(currentPort);
        if (index > 0) {
            serialPorts->setCurrentIndex(index);
        } else if (wasEmpty || currentIndex == 0) {
            serialPorts->setCurrentIndex(0);
        }
    }
}

void MainWindow::setupUi(void) {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(4);
    mainLayout->addLayout(leftLayout, 8);

    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(6);
    mainLayout->addLayout(rightLayout, 2);

    graphicsView = new QCustomPlot();
    graphicsView->setMinimumHeight(600);
    leftLayout->addWidget(graphicsView);

    QHBoxLayout *scaleLayout = new QHBoxLayout();
    scaleLayout->setContentsMargins(0, 2, 0, 2);
    scaleLayout->setSpacing(4);
    
    scaleXLabel = new QLabel("Points to show:");
    scaleXLabel->setStyleSheet("font-weight: bold;");
    scaleLayout->addWidget(scaleXLabel);
    
    scaleXValueLabel = new QLabel(QString("%1 points").arg(MAX_PLOT_POINTS / 4));
    scaleLayout->addWidget(scaleXValueLabel);
    
    scaleXSlider = new QSlider(Qt::Horizontal);
    scaleXSlider->setMinimum(10);
    scaleXSlider->setMaximum(MAX_PLOT_POINTS);
    scaleXSlider->setValue(MAX_PLOT_POINTS / 4);
    scaleXSlider->setTickPosition(QSlider::NoTicks);
    scaleXSlider->setFixedHeight(16);
    connect(scaleXSlider, &QSlider::valueChanged, this, &MainWindow::updateScaleValue);
    
    scaleLayout->addSpacing(10);
    scaleLayout->addWidget(scaleXSlider, 1);

    leftLayout->addLayout(scaleLayout);
    
    QGroupBox *controlGroup = new QGroupBox("Control Panel");
    QVBoxLayout *buttonsLayout = new QVBoxLayout(controlGroup);
    buttonsLayout->setSpacing(6);
    buttonsLayout->setContentsMargins(6, 12, 6, 6);
    rightLayout->addWidget(controlGroup, 1);
    
    autoPositionButton = new QPushButton("Auto Position");
    connect(autoPositionButton, &QPushButton::clicked, this, &MainWindow::autoPosition);
    buttonsLayout->addWidget(autoPositionButton);

    pauseResumeButton = new QPushButton("Pause");
    pauseResumeButton->setCheckable(true);
    connect(pauseResumeButton, &QPushButton::clicked, this, &MainWindow::pauseResume);
    buttonsLayout->addWidget(pauseResumeButton);

    clearButton = new QPushButton("Clear");
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearPlot);
    buttonsLayout->addWidget(clearButton);

    colors = {
        QColor(255, 82, 82),   // Modern red
        QColor(33, 150, 243),  // Modern blue
        QColor(76, 175, 80),   // Modern green
        QColor(255, 193, 7)    // Modern amber
    };
    
    QGroupBox *channelsGroup = new QGroupBox("Channels");
    QVBoxLayout *channelsLayout = new QVBoxLayout(channelsGroup);
    channelsLayout->setSpacing(6);
    channelsLayout->setContentsMargins(6, 12, 6, 6);
    
    QGridLayout *channelsGridLayout = new QGridLayout();
    channelsGridLayout->setSpacing(6);
    channelsLayout->addLayout(channelsGridLayout);
    
    for (int i = 0; i < CHANNELS; ++i) {
        QPushButton *button = new QPushButton(QString("Channel %1").arg(i + 1));
        button->setCheckable(true);
        
        if (i == 0) {
            button->setStyleSheet(QString("background-color: %1; color: white;").arg(colors[i].name()));
            button->setChecked(true);
        } else {
            button->setStyleSheet("background-color: rgb(45, 45, 45); color: white;");
            button->setChecked(false);
        }
        
        connect(button, &QPushButton::toggled, [=](bool checked) { toggleChannel(i, checked); });
        channelButtons.append(button);
        
        int row = i / 2;
        int col = i % 2;
        channelsGridLayout->addWidget(button, row, col);
    }
    
    rightLayout->addWidget(channelsGroup, 1);

    QGroupBox *connectionGroup = new QGroupBox("Connection Settings");
    QGridLayout *gridLayout = new QGridLayout(connectionGroup);
    gridLayout->setSpacing(6);
    gridLayout->setContentsMargins(6, 12, 6, 6);
    rightLayout->addWidget(connectionGroup, 1);

    QLabel *baudLabel = new QLabel("Baud Rate:");
    baudLabel->setStyleSheet("font-weight: bold; background-color: transparent;");
    gridLayout->addWidget(baudLabel, 0, 0);
    
    baudRates = new QComboBox();
    baudRates->setStyleSheet("padding-left: 8px;");
    baudRates->addItem("Select baud rate...");
    QVector<int> baudRateOptions = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
    for (int baud : baudRateOptions) {
        baudRates->addItem(QString::number(baud));
    }
    gridLayout->addWidget(baudRates, 0, 1);
    connect(baudRates, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::selectBaudRate);

    QLabel *portLabel = new QLabel("Serial Port:");
    portLabel->setStyleSheet("font-weight: bold; background-color: transparent;");
    gridLayout->addWidget(portLabel, 1, 0);
    
    serialPorts = new QComboBox();
    serialPorts->setStyleSheet("padding-left: 8px;");
    serialPorts->addItem("Select port...");
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        serialPorts->addItem(port.portName());
    }
    gridLayout->addWidget(serialPorts, 1, 1);
    connect(serialPorts, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::selectSerialPort);

    QGroupBox *actionGroup = new QGroupBox("Actions");
    QHBoxLayout *actionLayout = new QHBoxLayout(actionGroup);
    actionLayout->setSpacing(4);
    actionLayout->setContentsMargins(4, 8, 4, 4);
    rightLayout->addWidget(actionGroup, 1);

    startButton = new QPushButton("Start");
    startButton->setFixedHeight(26);
    startButton->setStyleSheet("background-color: #2E7D32; color: white;");
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startAcquisition);
    startButton->setEnabled(false);
    actionLayout->addWidget(startButton);
    
    actionLayout->addSpacing(2);
    
    stopButton = new QPushButton("Stop");
    stopButton->setFixedHeight(26);
    stopButton->setStyleSheet("background-color: #C62828; color: white;");
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopAcquisition);
    actionLayout->addWidget(stopButton);

    QStatusBar *statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready");

    applyDarkMode();
}
