#pragma once

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QVector>
#include <QElapsedTimer>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>
#include <QMessageBox>

#include "qcustomplot.h"
#include "plotmanager.h"

#define CHANNELS 4
#define MAX_PLOT_POINTS 1000

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow(void);

private slots:
    void updateScaleValue(int value);
    void autoPosition(void);
    void pauseResume(void);
    void clearPlot(void);
    void toggleChannel(int index, bool checked);
    void selectBaudRate(int index);
    void selectSerialPort(int index);
    void startAcquisition(void);
    void stopAcquisition(void);
    void updatePlot(void);

private:
    void setupUi(void);
    void setupSerial(void);
    void startSerialRead(void);
    void applyDarkMode(void);
    void updatePlotData(void);
    void scanSerialPorts(void);

    QWidget *centralWidget;
    QCustomPlot *graphicsView;
    QLabel *scaleXLabel;
    QLabel *scaleXValueLabel;
    QSlider *scaleXSlider;
    QPushButton *autoPositionButton;
    QPushButton *pauseResumeButton;
    QPushButton *clearButton;
    QVector<QPushButton*> channelButtons;
    QComboBox *baudRates;
    QComboBox *serialPorts;
    QPushButton *startButton;
    QPushButton *stopButton;

    QSerialPort *serialPort;
    QTimer *timer;
    QTimer *serialScanTimer;
    int baudRate;
    bool isAcquiring;
    bool isPaused;
    QByteArray serialData;

    QVector<QVector<double>> plotData;
    QVector<double> xData;
    QVector<QColor> colors;
    QVector<QCPGraph*> plotDataItems;
    
    PlotManager *plotManager;
};