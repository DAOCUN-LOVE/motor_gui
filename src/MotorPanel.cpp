#include "MotorPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QtCharts>
#include <QDebug>

using namespace QtCharts;

MotorPanel::MotorPanel(int motorId, QWidget *parent)
    : QWidget(parent), m_motorId(motorId)
{
    setupUI();

    m_worker = new MotorWorker(motorId);
    m_workerThread = new QThread(this);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &MotorWorker::dataUpdated, this, &MotorPanel::onDataReceived);
    connect(m_worker, &MotorWorker::statusMessage, this, &MotorPanel::onStatusMessage);

    m_workerThread->start();

    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
}

MotorPanel::~MotorPanel()
{
    if (m_worker && m_workerThread && m_workerThread->isRunning()) {
        QMetaObject::invokeMethod(m_worker, "stop", Qt::BlockingQueuedConnection);
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void MotorPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *titleLayout = new QHBoxLayout();
    titleLayout->addWidget(new QLabel(QString("电机 ID: %1").arg(m_motorId)));
    titleLayout->addStretch();
    QPushButton *closeBtn = new QPushButton("删除电机");
    connect(closeBtn, &QPushButton::clicked, this, [this](){ emit removeRequested(m_motorId); });
    titleLayout->addWidget(closeBtn);
    mainLayout->addLayout(titleLayout);

    m_rpmSeries = new QLineSeries();
    m_rpmSeries->setName("转速 (RPM)");
    m_rpmSeries->setColor(Qt::red);
    m_targetSeries = new QLineSeries();
    m_targetSeries->setName("目标转速 (RPM)");
    m_targetSeries->setColor(Qt::darkGreen);
    m_currentSeries = new QLineSeries();
    m_currentSeries->setName("电流 (mA)");
    m_currentSeries->setColor(Qt::blue);

    QChart *chart = new QChart();
    chart->addSeries(m_rpmSeries);
    chart->addSeries(m_targetSeries);
    chart->addSeries(m_currentSeries);
    chart->setTitle("实时数据");
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->legend()->setVisible(true);

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("时间 (x10ms)");
    axisX->setRange(0, 500);
    chart->addAxis(axisX, Qt::AlignBottom);
    m_rpmSeries->attachAxis(axisX);
    m_targetSeries->attachAxis(axisX);
    m_currentSeries->attachAxis(axisX);

    QValueAxis *axisYRpm = new QValueAxis();
    axisYRpm->setTitleText("转速 (RPM)");
    axisYRpm->setRange(-10, 100);
    chart->addAxis(axisYRpm, Qt::AlignLeft);
    m_rpmSeries->attachAxis(axisYRpm);
    m_targetSeries->attachAxis(axisYRpm);

    QValueAxis *axisYCurrent = new QValueAxis();
    axisYCurrent->setTitleText("电流 (mA)");
    axisYCurrent->setRange(0, 2000);
    chart->addAxis(axisYCurrent, Qt::AlignRight);
    m_currentSeries->attachAxis(axisYCurrent);

    m_chartView = new QChartView(chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    mainLayout->addWidget(m_chartView, 3);

    QHBoxLayout *curveToggleLayout = new QHBoxLayout();
    curveToggleLayout->addWidget(new QLabel("曲线显示:"));
    curveToggleLayout->addWidget(new QLabel("实际转速(固定)"));
    m_showTargetCheck = new QCheckBox("目标转速");
    m_showCurrentCheck = new QCheckBox("电流");
    m_showTargetCheck->setChecked(false);
    m_showCurrentCheck->setChecked(false);
    connect(m_showTargetCheck, &QCheckBox::toggled, this, &MotorPanel::updateSeriesVisibility);
    connect(m_showCurrentCheck, &QCheckBox::toggled, this, &MotorPanel::updateSeriesVisibility);
    curveToggleLayout->addWidget(m_showTargetCheck);
    curveToggleLayout->addWidget(m_showCurrentCheck);
    curveToggleLayout->addWidget(new QLabel("刷新间隔(ms):"));
    m_refreshIntervalSpin = new QSpinBox();
    m_refreshIntervalSpin->setRange(20, 200);
    m_refreshIntervalSpin->setValue(33);
    m_refreshIntervalSpin->setSingleStep(5);
    curveToggleLayout->addWidget(m_refreshIntervalSpin);
    curveToggleLayout->addStretch();
    mainLayout->addLayout(curveToggleLayout);

    QHBoxLayout *ctrlLayout = new QHBoxLayout();

    QGroupBox *pidGroup = new QGroupBox("PID 参数");
    QFormLayout *pidForm = new QFormLayout();
    m_kpEdit = new QLineEdit("500");
    m_kiEdit = new QLineEdit("100");
    m_kdEdit = new QLineEdit("0.2");
    pidForm->addRow("Kp:", m_kpEdit);
    pidForm->addRow("Ki:", m_kiEdit);
    pidForm->addRow("Kd:", m_kdEdit);
    QPushButton *applyPidBtn = new QPushButton("应用");
    connect(applyPidBtn, &QPushButton::clicked, this, &MotorPanel::onApplyPidClicked);
    pidForm->addRow(applyPidBtn);
    pidGroup->setLayout(pidForm);
    ctrlLayout->addWidget(pidGroup);

    QGroupBox *targetGroup = new QGroupBox("目标转速");
    QFormLayout *targetForm = new QFormLayout();
    m_targetRpmEdit = new QLineEdit("3");
    targetForm->addRow("RPM:", m_targetRpmEdit);
    QPushButton *setTargetBtn = new QPushButton("设置");
    connect(setTargetBtn, &QPushButton::clicked, this, &MotorPanel::onSetTargetClicked);
    targetForm->addRow(setTargetBtn);
    targetGroup->setLayout(targetForm);
    ctrlLayout->addWidget(targetGroup);

    m_startBtn = new QPushButton("启动");
    m_stopBtn = new QPushButton("停止");
    m_stopBtn->setEnabled(false);
    connect(m_startBtn, &QPushButton::clicked, this, &MotorPanel::onStartClicked);
    connect(m_stopBtn, &QPushButton::clicked, this, &MotorPanel::onStopClicked);
    QVBoxLayout *btnLayout = new QVBoxLayout();
    btnLayout->addWidget(m_startBtn);
    btnLayout->addWidget(m_stopBtn);
    ctrlLayout->addLayout(btnLayout);

    mainLayout->addLayout(ctrlLayout);

    m_logWidget = new QTextEdit();
    m_logWidget->setReadOnly(true);
    m_logWidget->setMaximumHeight(150);
    mainLayout->addWidget(new QLabel("状态日志:"));
    mainLayout->addWidget(m_logWidget);

    m_uiRefreshTimer = new QTimer(this);
    m_uiRefreshTimer->setInterval(m_refreshIntervalSpin->value());
    connect(m_uiRefreshTimer, &QTimer::timeout, this, &MotorPanel::onUiRefreshTick);
    connect(m_refreshIntervalSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int intervalMs) {
        if (m_uiRefreshTimer) {
            m_uiRefreshTimer->setInterval(intervalMs);
        }
    });
    m_uiRefreshTimer->start();

    updateSeriesVisibility();
}

void MotorPanel::startMotor()
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "start", Qt::QueuedConnection);
        m_startBtn->setEnabled(false);
        m_stopBtn->setEnabled(true);
        m_logWidget->append("正在启动电机控制...");
    }
}

void MotorPanel::stopMotor()
{
    if (m_worker && m_workerThread && m_workerThread->isRunning()) {
        QMetaObject::invokeMethod(m_worker, "stop", Qt::QueuedConnection);
        m_startBtn->setEnabled(true);
        m_stopBtn->setEnabled(false);
        m_logWidget->append("已发送停止请求");
    }
}

void MotorPanel::onStartClicked() { startMotor(); }
void MotorPanel::onStopClicked()  { stopMotor(); }

void MotorPanel::onApplyPidClicked()
{
    if (!m_worker) return;
    bool ok1, ok2, ok3;
    float kp = m_kpEdit->text().toFloat(&ok1);
    float ki = m_kiEdit->text().toFloat(&ok2);
    float kd = m_kdEdit->text().toFloat(&ok3);
    if (ok1 && ok2 && ok3) {
        QMetaObject::invokeMethod(m_worker, "setPidParams", Qt::QueuedConnection,
                                  Q_ARG(float, kp), Q_ARG(float, ki), Q_ARG(float, kd));
    } else {
        m_logWidget->append("错误: PID 参数必须是数字");
    }
}

void MotorPanel::onSetTargetClicked()
{
    if (!m_worker) return;
    bool ok;
    float rpm = m_targetRpmEdit->text().toFloat(&ok);
    if (ok) {
        QMetaObject::invokeMethod(m_worker, "setTargetRpm", Qt::QueuedConnection,
                                  Q_ARG(float, rpm));
    } else {
        m_logWidget->append("错误: 目标转速必须是数字");
    }
}

void MotorPanel::onDataReceived(float rpm, float current, float target)
{
    m_latestRpm = rpm;
    m_latestCurrent = current;
    m_latestTarget = target;
    m_hasNewSample = true;
}

void MotorPanel::onUiRefreshTick()
{
    if (!m_hasNewSample) {
        return;
    }

    m_hasNewSample = false;
    m_timeCounter++;
    m_rpmBuffer.append(QPointF(m_timeCounter, m_latestRpm));
    m_targetBuffer.append(QPointF(m_timeCounter, m_latestTarget));
    m_currentBuffer.append(QPointF(m_timeCounter, m_latestCurrent));
    while (m_rpmBuffer.size() > 500) m_rpmBuffer.removeFirst();
    while (m_targetBuffer.size() > 500) m_targetBuffer.removeFirst();
    while (m_currentBuffer.size() > 500) m_currentBuffer.removeFirst();

    m_rpmSeries->replace(m_rpmBuffer);
    if (m_targetSeries->isVisible()) {
        m_targetSeries->replace(m_targetBuffer);
    }
    if (m_currentSeries->isVisible()) {
        m_currentSeries->replace(m_currentBuffer);
    }

    if (m_timeCounter > 500) {
        QValueAxis *axisX = qobject_cast<QValueAxis*>(m_rpmSeries->attachedAxes().first());
        if (axisX) axisX->setRange(m_timeCounter - 500, m_timeCounter);
    }

    if (++m_logCount >= 50) {
        m_logCount = 0;
        m_logWidget->append(QString("目标: %1 RPM, 实际: %2 RPM, 电流: %3 mA")
                            .arg(m_latestTarget).arg(m_latestRpm).arg(m_latestCurrent));
        m_logWidget->verticalScrollBar()->setValue(m_logWidget->verticalScrollBar()->maximum());
    }
}

void MotorPanel::onStatusMessage(const QString &msg)
{
    m_logWidget->append(msg);
    qDebug() << "[Motor" << m_motorId << "]" << msg;
}

void MotorPanel::updateSeriesVisibility()
{
    if (!m_targetSeries || !m_currentSeries) return;
    m_targetSeries->setVisible(m_showTargetCheck && m_showTargetCheck->isChecked());
    m_currentSeries->setVisible(m_showCurrentCheck && m_showCurrentCheck->isChecked());
}