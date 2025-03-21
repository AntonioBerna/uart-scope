#pragma once

#include <QObject>
#include <QVector>
#include <QColor>

#include "qcustomplot.h"

class PlotManager : public QObject {
    Q_OBJECT

public:
    explicit PlotManager(QCustomPlot *plot, int channels, int maxPoints, QObject *parent = nullptr);
    ~PlotManager(void);

    void setupPlot(void);
    void updatePlotData(const QVector<QVector<double>> &data, const QVector<double> &xData, int currentLength, const QVector<bool> &channelVisibility);
    void clearPlot(void);
    void autoPosition(void);
    QVector<QColor> getColors(void) const { return colors; }
    QVector<QCPGraph*> getPlotItems(void) const { return plotItems; }

public slots:
    void onXRangeChanged(const QCPRange &range);
    void onYRangeChanged(const QCPRange &range);

private:
    QCustomPlot *plot;
    int channelCount;
    int maxPlotPoints;
    QVector<QColor> colors;
    QVector<QCPGraph*> plotItems;
};