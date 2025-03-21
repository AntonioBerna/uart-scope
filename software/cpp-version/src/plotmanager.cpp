#include "plotmanager.h"

PlotManager::PlotManager(QCustomPlot *plot, int channels, int maxPoints, QObject *parent) : QObject(parent), plot(plot), channelCount(channels), maxPlotPoints(maxPoints) {
    colors = {
        QColor(255, 82, 82),   // Modern red (Channel 1)
        QColor(33, 150, 243),  // Modern blue (Channel 2)
        QColor(76, 175, 80),   // Modern green (Channel 3)
        QColor(255, 193, 7)    // Modern amber (Channel 4)
    };
}

PlotManager::~PlotManager(void) {
    // Cleanup if needed
}

void PlotManager::setupPlot(void) {
    plotItems.clear();
    
    plot->setNotAntialiasedElements(QCP::aeAll);
    plot->setNoAntialiasingOnDrag(true);
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    
    plot->setPlottingHint(QCP::phCacheLabels, true);
    plot->setPlottingHint(QCP::phFastPolylines, true);
    
    plot->setMouseTracking(false);
    
    plot->setAttribute(Qt::WA_OpaquePaintEvent);
    
    connect(plot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(onXRangeChanged(QCPRange)));
    connect(plot->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(onYRangeChanged(QCPRange)));
    
    plot->axisRect()->setMinimumMargins(QMargins(5, 5, 5, 5));
    plot->axisRect()->setMargins(QMargins(10, 10, 10, 10));
    
    for (int i = 0; i < channelCount; ++i) {
        QColor color = colors[i % colors.size()];
        QPen pen(color, 2);
        
        plot->addGraph();
        plot->graph(i)->setPen(pen);
        plot->graph(i)->setVisible(i == 0);
        plot->graph(i)->setAdaptiveSampling(true);
        plot->graph(i)->setLineStyle(QCPGraph::lsLine);
        plot->graph(i)->setScatterStyle(QCPScatterStyle::ssNone);
        
        plotItems.append(plot->graph(i));
    }
    
    plot->xAxis->setRange(0, maxPlotPoints);
    plot->yAxis->setRange(0, 1023);
    
    plot->xAxis->setSubTickCount(1);
    plot->yAxis->setSubTickCount(1);
    
    QFont axisFont = plot->xAxis->labelFont();
    axisFont.setBold(true);
    axisFont.setPointSize(axisFont.pointSize() + 1);
    
    plot->xAxis->setLabelFont(axisFont);
    plot->yAxis->setLabelFont(axisFont);
    plot->xAxis->setLabel("Time (ms)");
    plot->yAxis->setLabel("Voltage (V)");
    
    plot->xAxis->setLabelColor(QColor(240, 240, 240));
    plot->yAxis->setLabelColor(QColor(240, 240, 240));
    
    plot->xAxis->grid()->setVisible(true);
    plot->yAxis->grid()->setVisible(true);
    
    plot->xAxis->grid()->setSubGridVisible(false);
    plot->yAxis->grid()->setSubGridVisible(false);
    
    plot->setBackground(QBrush(QColor(30, 30, 30)));
    plot->xAxis->setBasePen(QPen(QColor(240, 240, 240)));
    plot->yAxis->setBasePen(QPen(QColor(240, 240, 240)));
    plot->xAxis->setTickPen(QPen(QColor(240, 240, 240)));
    plot->yAxis->setTickPen(QPen(QColor(240, 240, 240)));
    plot->xAxis->setSubTickPen(QPen(QColor(240, 240, 240)));
    plot->yAxis->setSubTickPen(QPen(QColor(240, 240, 240)));
    plot->xAxis->setTickLabelColor(QColor(240, 240, 240));
    plot->yAxis->setTickLabelColor(QColor(240, 240, 240));
    plot->xAxis->grid()->setPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    plot->yAxis->grid()->setPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    plot->xAxis->grid()->setSubGridPen(QPen(QColor(60, 60, 60), 1, Qt::DotLine));
    plot->yAxis->grid()->setSubGridPen(QPen(QColor(60, 60, 60), 1, Qt::DotLine));
    plot->xAxis->grid()->setZeroLinePen(QPen(QColor(0, 120, 212)));
    plot->yAxis->grid()->setZeroLinePen(QPen(QColor(0, 120, 212)));
}

void PlotManager::updatePlotData(const QVector<QVector<double>> &data, const QVector<double> &xData, int currentLength, const QVector<bool> &channelVisibility) {
    plot->setUpdatesEnabled(false);
    
    static bool isDragging = false;
    if (plot->xAxis->selectedParts() != QCPAxis::spNone || plot->yAxis->selectedParts() != QCPAxis::spNone) {
        isDragging = true;
        plot->setNotAntialiasedElements(QCP::aeAll);
        return;
    } else if (isDragging) {
        isDragging = false;
        plot->replot();
    }

    bool hasData = false;
    for (const auto &channel : data) {
        if (!channel.isEmpty()) {
            hasData = true;
            break;
        }
    }

    if (hasData) {
        for (int i = 0; i < channelCount; ++i) {
            if (channelVisibility[i]) {
                int startIdx = maxPlotPoints - currentLength;
                QVector<double> visibleYData(data[i].mid(startIdx));
                QVector<double> visibleXData(xData.mid(startIdx));
                
                plotItems[i]->setData(visibleXData, visibleYData);
            }
        }
    } else {
        for (auto plotItem : plotItems) {
            plotItem->data()->clear();
        }
    }
    
    static QElapsedTimer updateTimer;
    if (updateTimer.isValid() && updateTimer.elapsed() < 33) {
        return;
    }
    updateTimer.restart();
    
    plot->setUpdatesEnabled(true);
    
    plot->replot();
}

void PlotManager::clearPlot(void) {
    for (auto plotItem : plotItems) {
        plotItem->data()->clear();
    }
    plot->replot();
}

void PlotManager::autoPosition(void) {
    plot->rescaleAxes();
    plot->replot();
}

void PlotManager::onXRangeChanged(const QCPRange &range) {
    QCPRange boundedRange = range;
    
    if (boundedRange.lower < 0)
        boundedRange.lower = 0;
    
    double minRange = 10;
    if (boundedRange.size() < minRange) {
        boundedRange.upper = qMax(boundedRange.lower + minRange, boundedRange.center() + minRange / 2);
        boundedRange.lower = qMax(0.0, boundedRange.upper - minRange);
    }
    
    plot->xAxis->setRange(boundedRange);
}

void PlotManager::onYRangeChanged(const QCPRange &range) {
    QCPRange boundedRange = range;
    
    if (boundedRange.lower < 0) {
        boundedRange.lower = 0;
    }

    double minRange = 10;
    if (boundedRange.size() < minRange) {
        boundedRange.upper = qMax(boundedRange.lower + minRange, boundedRange.center() + minRange / 2);
        boundedRange.lower = qMax(0.0, boundedRange.upper - minRange);
    }
    
    plot->yAxis->setRange(boundedRange);
}