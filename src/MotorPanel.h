#ifndef MOTORPANEL_H
#define MOTORPANEL_H

#include <QWidget>
#include <QThread>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QTimer>
#include <QtCharts>                  // 包含整个 Charts 模块
#include "MotorWorker.h"

using namespace QtCharts;            // 引入 QtCharts 命名空间

class MotorPanel : public QWidget
{
    Q_OBJECT
public:
    explicit MotorPanel(int motorId, QWidget *parent = nullptr);
    ~MotorPanel();

    int motorId() const { return m_motorId; }
    void startMotor();
    void stopMotor();
    float getKp() const { return m_kpEdit->text().toFloat(); }
    float getKi() const { return m_kiEdit->text().toFloat(); }
    float getKd() const { return m_kdEdit->text().toFloat(); }
    float getTargetRpm() const { return m_targetRpmEdit->text().toFloat(); }

signals:
    void removeRequested(int motorId);

private slots:
    void onStartClicked();
    void onStopClicked();
    void onApplyPidClicked();
    void onSetTargetClicked();
    void onLoadConfigClicked();
    void onDataReceived(float rpm, float current, float target, float targetCurrent, float, float);
    void onStatusMessage(const QString &msg);
    void updateSeriesVisibility();
    void onUiRefreshTick();

private:
    void setupUI();
    void updateCurve(float rpm, float current);

    int m_motorId;
    MotorWorker *m_worker = nullptr;
    QThread *m_workerThread = nullptr;

    // GUI 控件（直接使用类名，无需 QtCharts:: 前缀）
    QLineSeries *m_rpmSeries;
    QLineSeries *m_targetSeries;
    QLineSeries *m_currentSeries;
    QLineSeries *m_targetCurrentSeries;
    QChartView *m_chartView;
    QLineEdit *m_kpEdit, *m_kiEdit, *m_kdEdit;
    QLineEdit *m_targetRpmEdit;
    QTextEdit *m_logWidget;
    QPushButton *m_startBtn, *m_stopBtn, *m_loadConfigBtn;
    QCheckBox *m_showTargetCheck;
    QCheckBox *m_showCurrentCheck;
    QCheckBox *m_showTargetCurrentCheck;
    QSpinBox *m_refreshIntervalSpin = nullptr;

    QList<QPointF> m_rpmBuffer;
    QList<QPointF> m_targetBuffer;
    QList<QPointF> m_currentBuffer;
    QList<QPointF> m_targetCurrentBuffer;
    qint64 m_timeCounter = 0;
    int m_logCount = 0;
    QTimer *m_uiRefreshTimer = nullptr;
    float m_latestRpm = 0.0f;
    float m_latestCurrent = 0.0f;
    float m_latestTarget = 0.0f;
    float m_latestTargetCurrent = 0.0f;
    bool m_hasNewSample = false;
};

#endif