#include "MainWindow.h"
#include <QMenuBar>
#include <QToolBar>
#include <QInputDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
}

MainWindow::~MainWindow()
{
    // 所有面板会在析构时自动清理
}

void MainWindow::setupUI()
{
    setWindowTitle("GM6020 多电机控制");
    resize(1200, 800);

    // 创建菜单/工具栏
    QToolBar *toolBar = addToolBar("控制");
    toolBar->addAction("添加电机", this, &MainWindow::onAddMotor);
    toolBar->addAction("全部启动", this, &MainWindow::onStartAll);
    toolBar->addAction("全部停止", this, &MainWindow::onStopAll);
    toolBar->addAction("保存配置", this, &MainWindow::onSaveConfig);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, [this](int index){
        MotorPanel *panel = qobject_cast<MotorPanel*>(m_tabWidget->widget(index));
        if (panel) {
            onRemoveMotor(panel->motorId());
        }
    });
    setCentralWidget(m_tabWidget);
}

void MainWindow::onAddMotor()
{
    bool ok;
    int id = QInputDialog::getInt(this, "添加电机", "请输入电机 ID (1-10):", 1, 1, 10, 1, &ok);
    if (!ok) return;
    if (m_panels.contains(id)) {
        QMessageBox::warning(this, "错误", QString("电机 ID %1 已存在").arg(id));
        return;
    }
    MotorPanel *panel = new MotorPanel(id, this);
    m_tabWidget->addTab(panel, QString("电机 %1").arg(id));
    m_panels[id] = panel;
    connect(panel, &MotorPanel::removeRequested, this, &MainWindow::onRemoveMotor);
}

void MainWindow::onStartAll()
{
    for (auto panel : m_panels) {
        panel->startMotor();
    }
}

void MainWindow::onStopAll()
{
    for (auto panel : m_panels) {
        panel->stopMotor();
    }
}

void MainWindow::onRemoveMotor(int motorId)
{
    if (!m_panels.contains(motorId)) return;
    MotorPanel *panel = m_panels.take(motorId);
    int index = m_tabWidget->indexOf(panel);
    if (index >= 0) {
        m_tabWidget->removeTab(index);
    }
    panel->deleteLater();   // 析构时会停止线程
}

void MainWindow::onSaveConfig()
{
    QString filename = QFileDialog::getSaveFileName(this, "保存配置", ".", "JSON Files (*.json)");
    if (filename.isEmpty()) return;
    
    QJsonObject config;
    QJsonArray motorsArray;
    
    // 收集每个电机的配置
    for (auto it = m_panels.constBegin(); it != m_panels.constEnd(); ++it) {
        int motorId = it.key();
        MotorPanel *panel = it.value();
        
        QJsonObject motorConfig;
        motorConfig["id"] = motorId;
        motorConfig["kp"] = panel->getKp();
        motorConfig["ki"] = panel->getKi();
        motorConfig["kd"] = panel->getKd();
        motorConfig["targetRpm"] = panel->getTargetRpm();
        motorsArray.append(motorConfig);
    }
    
    config["motors"] = motorsArray;
    config["timestamp"] = QDateTime::currentDateTime().toString();
    
    // 写入文件
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument doc(config);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        QMessageBox::information(this, "成功", "配置已保存");
    } else {
        QMessageBox::warning(this, "错误", "无法保存配置文件");
    }
}