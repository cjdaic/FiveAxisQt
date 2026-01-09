#include "MainWindow.h"
#include "view/DrawingPanel.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QStringList>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_client(new FiveAxisClient(this)) {
    buildUi();

    connect(m_client, &FiveAxisClient::replyReceived, this, &MainWindow::onReply);
    connect(m_client, &FiveAxisClient::errorReceived, this, &MainWindow::onError);
}

void MainWindow::buildUi() {
    setWindowTitle(QStringLiteral("FiveAxis Qt6"));
    resize(1280, 720);

    m_splitter = new QSplitter(this);
    m_projectTree = new QTreeWidget(m_splitter);
    m_projectTree->setHeaderLabel(QStringLiteral("Objects"));
    m_projectTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_sceneRoot = new QTreeWidgetItem(QStringList(QStringLiteral("SCENE")));
    m_projectTree->addTopLevelItem(m_sceneRoot);
    connect(m_projectTree, &QTreeWidget::customContextMenuRequested, this, &MainWindow::showProjectContextMenu);
    connect(m_projectTree, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onProjectSelectionChanged);

    m_scenePreview = new DrawingPanel(m_splitter);
    connect(m_scenePreview, &DrawingPanel::shapeCreated, this, &MainWindow::onShapeCreated);
    connect(m_scenePreview, &DrawingPanel::shapeSelected, this, &MainWindow::onShapeSelected);
    connect(m_scenePreview, &DrawingPanel::shapeMoved, this, &MainWindow::onShapeMoved);
    connect(m_scenePreview, &DrawingPanel::shapeRemoved, this, &MainWindow::onShapeRemoved);

    m_propertyTabs = new QTabWidget(m_splitter);
    m_propertyTabs->addTab(buildLineTab(), tr("Line"));
    m_propertyTabs->addTab(buildCircleTab(), tr("Circle / Concentric"));
    m_propertyTabs->addTab(buildRectangleTab(), tr("Rectangle"));
    m_propertyTabs->addTab(buildEllipseTab(), tr("Ellipse"));
    m_propertyTabs->addTab(buildSettingsTab(), tr("Delay / Freq"));

    m_splitter->addWidget(m_projectTree);
    m_splitter->addWidget(m_scenePreview);
    m_splitter->addWidget(m_propertyTabs);
    m_splitter->setStretchFactor(1, 2);
    setCentralWidget(m_splitter);

    m_log = new QTextEdit(this);
    m_log->setReadOnly(true);
    auto dockLog = new QDockWidget(tr("Processing Log"), this);
    dockLog->setWidget(m_log);
    addDockWidget(Qt::BottomDockWidgetArea, dockLog);

    auto fileMenu = menuBar()->addMenu(tr("Connect"));
    auto actionConnect = fileMenu->addAction(tr("Connect gRPC Service"));
    connect(actionConnect, &QAction::triggered, this, &MainWindow::connectToServer);
    auto modelMenu = menuBar()->addMenu(tr("3D model"));
    auto actionImport = modelMenu->addAction(tr("importing model ..."));
    auto actionSample = modelMenu->addAction(tr("show sample"));
    connect(actionImport, &QAction::triggered, this, &MainWindow::importModel);
    connect(actionSample, &QAction::triggered, this, &MainWindow::showSampleModel);

    statusBar()->showMessage(tr("Not connected"));
}

void MainWindow::connectToServer() {
    const auto endpoint = QUrl(QStringLiteral("grpc://localhost:50051"));
    const QString details = m_client->connectToServer(endpoint);
    statusBar()->showMessage(tr("gRPC channel state: %1").arg(m_client->channelStateString()));
    m_log->append(tr("Attempting to connect to gRPC service with details:\n%1").arg(details));
}

void MainWindow::sendLine() {
    LineData request;
    request.set_speed(m_lineSpeed->value());
    request.set_times(m_lineTimes->value());
    request.set_x1(m_lineX1->value());
    request.set_y1(m_lineY1->value());
    request.set_z1(m_lineZ1->value());
    request.set_a1(m_lineA1->value());
    request.set_b1(m_lineB1->value());
    request.set_x2(m_lineX2->value());
    request.set_y2(m_lineY2->value());
    request.set_z2(m_lineZ2->value());
    request.set_a2(m_lineA2->value());
    request.set_b2(m_lineB2->value());
    request.set_islast(true);

    m_client->processLine(request);
    m_log->append(tr("Submitted line process: %1").arg(formatLine(request)));
}

void MainWindow::sendCircle() {
    CircleData request;
    request.set_speed(m_circleSpeed->value());
    request.set_times(m_circleTimes->value());
    request.set_x1(m_circleX1->value());
    request.set_y1(m_circleY1->value());
    request.set_x2(m_circleX2->value());
    request.set_y2(m_circleY2->value());
    request.set_m(m_circleM->value());
    request.set_angle(m_circleAngle->value());
    request.set_taper(m_circleTaper->value());
    request.set_filled(m_circleFilled->isChecked());
    request.set_r_min(m_circleRMin->value());
    request.set_r_interval(m_circleRInterval->value());
    request.set_z_start(m_circleZStart->value());
    request.set_z_end(m_circleZEnd->value());
    request.set_z_interval(m_circleZInterval->value());
    request.set_circle_num_repair(m_circleRepairNum->value());
    request.set_times_repair(m_circleRepairTimes->value());
    request.set_islast(true);

    m_client->processCircle(request);
    m_log->append(tr("Submitted circle/concentric process: %1").arg(formatCircle(request)));
}

void MainWindow::sendRectangle() {
    RectangleData request;
    request.set_x0(m_rectX0->value());
    request.set_y0(m_rectY0->value());
    request.set_x1(m_rectX1->value());
    request.set_y1(m_rectY1->value());
    request.set_taper_a_max(m_rectTaperA->value());
    request.set_taper_b_max(m_rectTaperB->value());
    request.set_feedspacing_x(m_rectFeedX->value());
    request.set_feedspacing_y(m_rectFeedY->value());
    request.set_speed(m_rectSpeed->value());
    request.set_z_start(m_rectZStart->value());
    request.set_z_end(m_rectZEnd->value());
    request.set_z_interval(m_rectZInterval->value());
    request.set_times(m_rectTimes->value());
    request.set_circle_num_repair(m_rectRepairNum->value());
    request.set_times_repair(m_rectRepairTimes->value());
    request.set_x2(m_rectX2->value());
    request.set_y2(m_rectY2->value());
    request.set_islast(true);

    m_client->processRectangle(request);
    m_log->append(tr("Submitted rectangle process: %1").arg(formatRectangle(request)));
}

void MainWindow::sendEllipse() {
    EllipseData request;
    request.set_speed(m_ellipseSpeed->value());
    request.set_times(m_ellipseTimes->value());
    request.set_x0(m_ellipseX0->value());
    request.set_y0(m_ellipseY0->value());
    request.set_a_max(m_ellipseAMax->value());
    request.set_b_max(m_ellipseBMax->value());
    request.set_a_min(m_ellipseAMin->value());
    request.set_b_min(m_ellipseBMin->value());
    request.set_taper_a_max(m_ellipseTaperA->value());
    request.set_taper_b_max(m_ellipseTaperB->value());
    request.set_feedspacing_x(m_ellipseFeedX->value());
    request.set_feedspacing_y(m_ellipseFeedY->value());
    request.set_z_start(m_ellipseZStart->value());
    request.set_z_end(m_ellipseZEnd->value());
    request.set_z_interval(m_ellipseZInterval->value());
    request.set_circle_num_repair(m_ellipseRepairNum->value());
    request.set_times_repair(m_ellipseRepairTimes->value());
    request.set_islast(true);

    m_client->processEllipse(request);
    m_log->append(tr("Submitted ellipse process: %1").arg(formatEllipse(request)));
}

void MainWindow::applyDelay() {
    DelayData request;
    request.set_jump_speed(m_jumpSpeed->value());
    request.set_laser_on_delay(m_laserOnDelay->value());
    request.set_laser_off_delay(m_laserOffDelay->value());
    request.set_mark_delay(m_markDelay->value());
    request.set_jump_delay(m_jumpDelay->value());
    request.set_polygon_delay(m_polygonDelay->value());

    m_client->setDelay(request);
    m_log->append(tr("Applied delay parameters: %1").arg(formatDelay(request)));
}

void MainWindow::applyFreq() {
    FreqData request;
    request.set_freq(m_freq->value());
    m_client->setLaserFreq(request);
    m_log->append(tr("Applied laser frequency: %1").arg(formatFreq(request)));
}

void MainWindow::importModel() {
    if (!m_modelViewer) {
        return;
    }
    m_modelViewer->loadModelFromDialog();
}

void MainWindow::showSampleModel() {
    if (!m_modelViewer) {
        return;
    }
    m_modelViewer->showSampleModel();
    m_log->append(tr("refreshed 3d"));
}

void MainWindow::onReply(const QString& operation, const QString& message) {
    m_log->append(tr("[%1] Success: %2").arg(operation, message));
}

void MainWindow::onError(const QString& operation, int code, const QString& message) {
    m_log->append(QStringLiteral("[%1] Error %2: %3").arg(operation, QString::number(code), message));
}

void MainWindow::onShapeCreated(const QString& id, const QString& type) {
    auto* item = new QTreeWidgetItem(QStringList(id));
    item->setData(0, Qt::UserRole, id);
    item->setData(0, Qt::UserRole + 1, type);
    m_sceneRoot->addChild(item);
    m_treeItems.insert(id, item);
    m_sceneRoot->setExpanded(true);
    m_log->append(tr("Preview: Added %1 (%2)").arg(type, id));
}

void MainWindow::onShapeSelected(const QString& id, const QString& type) {
    if (auto* item = m_treeItems.value(id, nullptr)) {
        m_projectTree->setCurrentItem(item);
    }
    DrawingPanel::ShapeInfo info;
    if (m_scenePreview->shapeInfo(id, info)) {
        if (type == QStringLiteral("Line")) {
            m_propertyTabs->setCurrentIndex(0);
            m_lineX1->setValue(info.p1.x());
            m_lineY1->setValue(info.p1.y());
            m_lineX2->setValue(info.p2.x());
            m_lineY2->setValue(info.p2.y());
        }
        else if (type == QStringLiteral("Circle")) {
            m_propertyTabs->setCurrentIndex(1);
            const auto center = info.rect.center();
            const double r = info.rect.width() / 2.0;
            m_circleX1->setValue(center.x());
            m_circleY1->setValue(center.y());
            m_circleX2->setValue(center.x() + r);
            m_circleY2->setValue(center.y());
        }
        else if (type == QStringLiteral("Rectangle")) {
            m_propertyTabs->setCurrentIndex(2);
            m_rectX0->setValue(info.rect.left());
            m_rectY0->setValue(info.rect.top());
            m_rectX1->setValue(info.rect.right());
            m_rectY1->setValue(info.rect.bottom());
        }
        else if (type == QStringLiteral("Ellipse")) {
            m_propertyTabs->setCurrentIndex(3);
            const auto center = info.rect.center();
            m_ellipseX0->setValue(center.x());
            m_ellipseY0->setValue(center.y());
            m_ellipseAMax->setValue(info.rect.width() / 2.0);
            m_ellipseBMax->setValue(info.rect.height() / 2.0);
        }
    }
}

void MainWindow::onShapeMoved(const QString& id, const QString& type) {
    onShapeSelected(id, type);
    m_log->append(tr("Preview: Moved %1").arg(id));
}

void MainWindow::onShapeRemoved(const QString& id, const QString& type) {
    Q_UNUSED(type);
    if (auto* item = m_treeItems.take(id)) {
        delete item;
    }
    m_log->append(tr("Preview: Deleted %1").arg(id));
}

void MainWindow::showProjectContextMenu(const QPoint& pos) {
    auto* current = m_projectTree->itemAt(pos);
    if (!current || current == m_sceneRoot) {
        return;
    }
    const QString id = current->data(0, Qt::UserRole).toString();
    QMenu menu(this);
    auto* actionSelect = menu.addAction(tr("Move"));
    auto* actionRemove = menu.addAction(tr("Delete"));
    const auto chosen = menu.exec(m_projectTree->viewport()->mapToGlobal(pos));
    if (!chosen) {
        return;
    }
    if (chosen == actionRemove) {
        m_scenePreview->removeShape(id);
    }
    else if (chosen == actionSelect) {
        m_scenePreview->setMode(DrawingView::Mode::Pointer);
        m_scenePreview->selectShape(id);
    }
}

void MainWindow::onProjectSelectionChanged() {
    auto* current = m_projectTree->currentItem();
    if (!current || current == m_sceneRoot) {
        return;
    }
    const QString id = current->data(0, Qt::UserRole).toString();
    m_scenePreview->setMode(DrawingView::Mode::Pointer);
    m_scenePreview->selectShape(id);
}

QWidget* MainWindow::buildLineTab() {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);

    m_lineSpeed = new QDoubleSpinBox(page);
    m_lineSpeed->setRange(0, 10000);
    m_lineSpeed->setValue(10.0);

    m_lineTimes = new QSpinBox(page);
    m_lineTimes->setRange(1, 10000);
    m_lineTimes->setValue(1);

    m_lineX1 = new QDoubleSpinBox(page);
    m_lineY1 = new QDoubleSpinBox(page);
    m_lineZ1 = new QDoubleSpinBox(page);
    m_lineA1 = new QDoubleSpinBox(page);
    m_lineB1 = new QDoubleSpinBox(page);
    m_lineX2 = new QDoubleSpinBox(page);
    m_lineY2 = new QDoubleSpinBox(page);
    m_lineZ2 = new QDoubleSpinBox(page);
    m_lineA2 = new QDoubleSpinBox(page);
    m_lineB2 = new QDoubleSpinBox(page);

    for (auto* spin : { m_lineX1, m_lineY1, m_lineZ1, m_lineA1, m_lineB1, m_lineX2, m_lineY2, m_lineZ2, m_lineA2, m_lineB2 }) {
        spin->setRange(-100000, 100000);
        spin->setDecimals(3);
    }

    layout->addRow(tr("Speed (mm/s)"), m_lineSpeed);
    layout->addRow(tr("Times"), m_lineTimes);
    layout->addRow(tr("Start X"), m_lineX1);
    layout->addRow(tr("Start Y"), m_lineY1);
    layout->addRow(tr("Start Z"), m_lineZ1);
    layout->addRow(tr("Start A"), m_lineA1);
    layout->addRow(tr("Start B"), m_lineB1);
    layout->addRow(tr("End X"), m_lineX2);
    layout->addRow(tr("End Y"), m_lineY2);
    layout->addRow(tr("End Z"), m_lineZ2);
    layout->addRow(tr("End A"), m_lineA2);
    layout->addRow(tr("End B"), m_lineB2);

    auto* submit = new QPushButton(tr("Process"), page);
    layout->addRow(submit);
    connect(submit, &QPushButton::clicked, this, &MainWindow::sendLine);

    return page;
}

QWidget* MainWindow::buildCircleTab() {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);

    m_circleSpeed = new QDoubleSpinBox(page);
    m_circleSpeed->setRange(0, 10000);
    m_circleSpeed->setValue(10.0);

    m_circleTimes = new QSpinBox(page);
    m_circleTimes->setRange(1, 10000);
    m_circleTimes->setValue(1);

    m_circleX1 = new QDoubleSpinBox(page);
    m_circleY1 = new QDoubleSpinBox(page);
    m_circleX2 = new QDoubleSpinBox(page);
    m_circleY2 = new QDoubleSpinBox(page);
    for (auto* spin : { m_circleX1, m_circleY1, m_circleX2, m_circleY2 }) {
        spin->setRange(-100000, 100000);
        spin->setDecimals(3);
    }

    m_circleM = new QSpinBox(page);
    m_circleM->setRange(-1, 1);
    m_circleM->setValue(-1);

    m_circleAngle = new QDoubleSpinBox(page);
    m_circleAngle->setRange(0, 360);
    m_circleAngle->setValue(360);

    m_circleTaper = new QDoubleSpinBox(page);
    m_circleTaper->setRange(-1000, 1000);
    m_circleTaper->setDecimals(3);

    m_circleFilled = new QCheckBox(tr("Generate concentric circles"), page);

    m_circleRMin = new QDoubleSpinBox(page);
    m_circleRInterval = new QDoubleSpinBox(page);
    m_circleZStart = new QDoubleSpinBox(page);
    m_circleZEnd = new QDoubleSpinBox(page);
    m_circleZInterval = new QDoubleSpinBox(page);
    for (auto* spin : { m_circleRMin, m_circleRInterval, m_circleZStart, m_circleZEnd, m_circleZInterval }) {
        spin->setRange(-100000, 100000);
        spin->setDecimals(3);
    }
    m_circleRMin->setValue(0.01);
    m_circleRInterval->setValue(0.01);

    m_circleRepairNum = new QSpinBox(page);
    m_circleRepairNum->setRange(0, 1000);
    m_circleRepairTimes = new QSpinBox(page);
    m_circleRepairTimes->setRange(0, 1000);

    layout->addRow(tr("Speed (mm/s)"), m_circleSpeed);
    layout->addRow(tr("Times"), m_circleTimes);
    layout->addRow(tr("Center X1"), m_circleX1);
    layout->addRow(tr("Center Y1"), m_circleY1);
    layout->addRow(tr("Point X2"), m_circleX2);
    layout->addRow(tr("Point Y2"), m_circleY2);
    layout->addRow(tr("Direction (-1/1)"), m_circleM);
    layout->addRow(tr("Angle"), m_circleAngle);
    layout->addRow(tr("Taper"), m_circleTaper);
    layout->addRow(m_circleFilled);
    layout->addRow(tr("r_min"), m_circleRMin);
    layout->addRow(tr("r_interval"), m_circleRInterval);
    layout->addRow(tr("z_start"), m_circleZStart);
    layout->addRow(tr("z_end"), m_circleZEnd);
    layout->addRow(tr("z_interval"), m_circleZInterval);
    layout->addRow(tr("Repair rings"), m_circleRepairNum);
    layout->addRow(tr("Repair times"), m_circleRepairTimes);

    auto* submit = new QPushButton(tr("Process"), page);
    layout->addRow(submit);
    connect(submit, &QPushButton::clicked, this, &MainWindow::sendCircle);

    return page;
}

QWidget* MainWindow::buildRectangleTab() {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);

    m_rectX0 = new QDoubleSpinBox(page);
    m_rectY0 = new QDoubleSpinBox(page);
    m_rectX1 = new QDoubleSpinBox(page);
    m_rectY1 = new QDoubleSpinBox(page);
    m_rectX2 = new QDoubleSpinBox(page);
    m_rectY2 = new QDoubleSpinBox(page);
    for (auto* spin : { m_rectX0, m_rectY0, m_rectX1, m_rectY1, m_rectX2, m_rectY2 }) {
        spin->setRange(-100000, 100000);
        spin->setDecimals(3);
    }

    m_rectTaperA = new QDoubleSpinBox(page);
    m_rectTaperA->setRange(-10000, 10000);
    m_rectTaperA->setDecimals(3);
    m_rectTaperB = new QDoubleSpinBox(page);
    m_rectTaperB->setRange(-10000, 10000);
    m_rectTaperB->setDecimals(3);

    m_rectFeedX = new QDoubleSpinBox(page);
    m_rectFeedY = new QDoubleSpinBox(page);
    for (auto* spin : { m_rectFeedX, m_rectFeedY }) {
        spin->setRange(0.001, 100000);
        spin->setDecimals(3);
        spin->setValue(0.5);
    }

    m_rectSpeed = new QDoubleSpinBox(page);
    m_rectSpeed->setRange(0, 10000);
    m_rectSpeed->setValue(300.0);

    m_rectZStart = new QDoubleSpinBox(page);
    m_rectZEnd = new QDoubleSpinBox(page);
    m_rectZInterval = new QDoubleSpinBox(page);
    for (auto* spin : { m_rectZStart, m_rectZEnd, m_rectZInterval }) {
        spin->setRange(-100000, 100000);
        spin->setDecimals(3);
    }
    m_rectTimes = new QSpinBox(page);
    m_rectTimes->setRange(1, 10000);
    m_rectTimes->setValue(1);

    m_rectRepairNum = new QSpinBox(page);
    m_rectRepairNum->setRange(0, 1000);
    m_rectRepairTimes = new QSpinBox(page);
    m_rectRepairTimes->setRange(0, 1000);

    layout->addRow(tr("X0"), m_rectX0);
    layout->addRow(tr("Y0"), m_rectY0);
    layout->addRow(tr("X1"), m_rectX1);
    layout->addRow(tr("Y1"), m_rectY1);
    layout->addRow(tr("Offset X2"), m_rectX2);
    layout->addRow(tr("Offset Y2"), m_rectY2);
    layout->addRow(tr("Taper B"), m_rectTaperB);
    layout->addRow(tr("Taper A"), m_rectTaperA);
    layout->addRow(tr("Feed X"), m_rectFeedX);
    layout->addRow(tr("Feed Y"), m_rectFeedY);
    layout->addRow(tr("Speed (mm/s)"), m_rectSpeed);
    layout->addRow(tr("Z Start"), m_rectZStart);
    layout->addRow(tr("Z End"), m_rectZEnd);
    layout->addRow(tr("Z Step"), m_rectZInterval);
    layout->addRow(tr("Times"), m_rectTimes);
    layout->addRow(tr("Repair rings"), m_rectRepairNum);
    layout->addRow(tr("Repair times"), m_rectRepairTimes);

    auto* submit = new QPushButton(tr("Process"), page);
    layout->addRow(submit);
    connect(submit, &QPushButton::clicked, this, &MainWindow::sendRectangle);

    return page;
}

QWidget* MainWindow::buildEllipseTab() {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);

    m_ellipseSpeed = new QDoubleSpinBox(page);
    m_ellipseSpeed->setRange(0, 10000);
    m_ellipseSpeed->setValue(10.0);

    m_ellipseTimes = new QSpinBox(page);
    m_ellipseTimes->setRange(1, 10000);
    m_ellipseTimes->setValue(1);

    m_ellipseX0 = new QDoubleSpinBox(page);
    m_ellipseY0 = new QDoubleSpinBox(page);
    m_ellipseAMax = new QDoubleSpinBox(page);
    m_ellipseBMax = new QDoubleSpinBox(page);
    m_ellipseAMin = new QDoubleSpinBox(page);
    m_ellipseBMin = new QDoubleSpinBox(page);
    for (auto* spin : { m_ellipseX0, m_ellipseY0, m_ellipseAMax, m_ellipseBMax, m_ellipseAMin, m_ellipseBMin }) {
        spin->setRange(-100000, 100000);
        spin->setDecimals(3);
    }

    m_ellipseTaperA = new QDoubleSpinBox(page);
    m_ellipseTaperA->setRange(-10000, 10000);
    m_ellipseTaperA->setDecimals(3);
    m_ellipseTaperB = new QDoubleSpinBox(page);
    m_ellipseTaperB->setRange(-10000, 10000);
    m_ellipseTaperB->setDecimals(3);

    m_ellipseFeedX = new QDoubleSpinBox(page);
    m_ellipseFeedY = new QDoubleSpinBox(page);
    for (auto* spin : { m_ellipseFeedX, m_ellipseFeedY }) {
        spin->setRange(0.001, 100000);
        spin->setDecimals(3);
        spin->setValue(0.5);
    }

    m_ellipseZStart = new QDoubleSpinBox(page);
    m_ellipseZEnd = new QDoubleSpinBox(page);
    m_ellipseZInterval = new QDoubleSpinBox(page);
    for (auto* spin : { m_ellipseZStart, m_ellipseZEnd, m_ellipseZInterval }) {
        spin->setRange(-100000, 100000);
        spin->setDecimals(3);
    }

    m_ellipseRepairNum = new QSpinBox(page);
    m_ellipseRepairNum->setRange(0, 1000);
    m_ellipseRepairTimes = new QSpinBox(page);
    m_ellipseRepairTimes->setRange(0, 1000);

    layout->addRow(tr("Speed (mm/s)"), m_ellipseSpeed);
    layout->addRow(tr("Times"), m_ellipseTimes);
    layout->addRow(tr("Center X0"), m_ellipseX0);
    layout->addRow(tr("Center Y0"), m_ellipseY0);
    layout->addRow(tr("a Max"), m_ellipseAMax);
    layout->addRow(tr("b Max"), m_ellipseBMax);
    layout->addRow(tr("a Min"), m_ellipseAMin);
    layout->addRow(tr("b Min"), m_ellipseBMin);
    layout->addRow(tr("Taper A"), m_ellipseTaperA);
    layout->addRow(tr("Taper B"), m_ellipseTaperB);
    layout->addRow(tr("Feed X"), m_ellipseFeedX);
    layout->addRow(tr("Feed Y"), m_ellipseFeedY);
    layout->addRow(tr("Z Start"), m_ellipseZStart);
    layout->addRow(tr("Z End"), m_ellipseZEnd);
    layout->addRow(tr("Z Step"), m_ellipseZInterval);
    layout->addRow(tr("Repair rings"), m_ellipseRepairNum);
    layout->addRow(tr("Repair times"), m_ellipseRepairTimes);

    auto* submit = new QPushButton(tr("Process"), page);
    layout->addRow(submit);
    connect(submit, &QPushButton::clicked, this, &MainWindow::sendEllipse);

    return page;
}

QWidget* MainWindow::buildSettingsTab() {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);

    m_jumpSpeed = new QSpinBox(page);
    m_jumpSpeed->setRange(0, 20000);
    m_jumpSpeed->setValue(500);
    m_laserOnDelay = new QSpinBox(page);
    m_laserOnDelay->setRange(0, 5000);
    m_laserOnDelay->setValue(100);
    m_laserOffDelay = new QSpinBox(page);
    m_laserOffDelay->setRange(0, 5000);
    m_laserOffDelay->setValue(580);
    m_markDelay = new QSpinBox(page);
    m_markDelay->setRange(0, 5000);
    m_markDelay->setValue(600);
    m_jumpDelay = new QSpinBox(page);
    m_jumpDelay->setRange(0, 5000);
    m_jumpDelay->setValue(150);
    m_polygonDelay = new QSpinBox(page);
    m_polygonDelay->setRange(0, 5000);
    m_polygonDelay->setValue(450);

    m_freq = new QSpinBox(page);
    m_freq->setRange(1, 200000);
    m_freq->setValue(20000);

    layout->addRow(tr("Jump speed"), m_jumpSpeed);
    layout->addRow(tr("Laser on delay"), m_laserOnDelay);
    layout->addRow(tr("Laser off delay"), m_laserOffDelay);
    layout->addRow(tr("Mark delay"), m_markDelay);
    layout->addRow(tr("Jump delay"), m_jumpDelay);
    layout->addRow(tr("Polygon delay"), m_polygonDelay);

    auto* applyDelayBtn = new QPushButton(tr("Apply delays"), page);
    layout->addRow(applyDelayBtn);
    connect(applyDelayBtn, &QPushButton::clicked, this, &MainWindow::applyDelay);

    layout->addRow(new QLabel(QStringLiteral("-"), page));
    layout->addRow(tr("Laser frequency (Hz)"), m_freq);
    auto* applyFreqBtn = new QPushButton(tr("Apply frequency"), page);
    layout->addRow(applyFreqBtn);
    connect(applyFreqBtn, &QPushButton::clicked, this, &MainWindow::applyFreq);

    return page;
}

QString MainWindow::formatLine(const LineData& request) const {
    const QString start = tr("start=(%1,%2,%3,%4,%5)")
        .arg(request.x1())
        .arg(request.y1())
        .arg(request.z1())
        .arg(request.a1())
        .arg(request.b1());
    const QString end = tr("end=(%1,%2,%3,%4,%5)")
        .arg(request.x2())
        .arg(request.y2())
        .arg(request.z2())
        .arg(request.a2())
        .arg(request.b2());
    QStringList parts{
        tr("speed=%1").arg(request.speed()),
        tr("times=%1").arg(request.times()),
        start,
        end,
        tr("isLast=%1").arg(request.islast() ? tr("true") : tr("false"))
    };
    return parts.join(QStringLiteral(", "));
}

QString MainWindow::formatCircle(const CircleData& request) const {
    QStringList parts{
        tr("speed=%1").arg(request.speed()),
        tr("times=%1").arg(request.times()),
        tr("center=(%1,%2)").arg(request.x1()).arg(request.y1()),
        tr("point=(%1,%2)").arg(request.x2()).arg(request.y2()),
        tr("direction=%1").arg(request.m()),
        tr("angle=%1").arg(request.angle()),
        tr("taper=%1").arg(request.taper()),
        tr("filled=%1").arg(request.filled() ? tr("true") : tr("false")),
        tr("r_min=%1").arg(request.r_min()),
        tr("r_interval=%1").arg(request.r_interval()),
        tr("z_start=%1").arg(request.z_start()),
        tr("z_end=%1").arg(request.z_end()),
        tr("z_interval=%1").arg(request.z_interval()),
        tr("repair_rings=%1").arg(request.circle_num_repair()),
        tr("repair_times=%1").arg(request.times_repair()),
        tr("isLast=%1").arg(request.islast() ? tr("true") : tr("false"))
    };
    return parts.join(QStringLiteral(", "));
}

QString MainWindow::formatRectangle(const RectangleData& request) const {
    QStringList parts{
        tr("x0=%1").arg(request.x0()),
        tr("y0=%1").arg(request.y0()),
        tr("x1=%1").arg(request.x1()),
        tr("y1=%1").arg(request.y1()),
        tr("offset=(%1,%2)").arg(request.x2()).arg(request.y2()),
        tr("taperA=%1").arg(request.taper_a_max()),
        tr("taperB=%1").arg(request.taper_b_max()),
        tr("feed=(%1,%2)").arg(request.feedspacing_x()).arg(request.feedspacing_y()),
        tr("speed=%1").arg(request.speed()),
        tr("z_start=%1").arg(request.z_start()),
        tr("z_end=%1").arg(request.z_end()),
        tr("z_step=%1").arg(request.z_interval()),
        tr("times=%1").arg(request.times()),
        tr("repair_rings=%1").arg(request.circle_num_repair()),
        tr("repair_times=%1").arg(request.times_repair()),
        tr("isLast=%1").arg(request.islast() ? tr("true") : tr("false"))
    };
    return parts.join(QStringLiteral(", "));
}

QString MainWindow::formatEllipse(const EllipseData& request) const {
    QStringList parts{
        tr("center=(%1,%2)").arg(request.x0()).arg(request.y0()),
        tr("a_max=%1").arg(request.a_max()),
        tr("b_max=%1").arg(request.b_max()),
        tr("a_min=%1").arg(request.a_min()),
        tr("b_min=%1").arg(request.b_min()),
        tr("taperA=%1").arg(request.taper_a_max()),
        tr("taperB=%1").arg(request.taper_b_max()),
        tr("feed=(%1,%2)").arg(request.feedspacing_x()).arg(request.feedspacing_y()),
        tr("speed=%1").arg(request.speed()),
        tr("z_start=%1").arg(request.z_start()),
        tr("z_end=%1").arg(request.z_end()),
        tr("z_step=%1").arg(request.z_interval()),
        tr("times=%1").arg(request.times()),
        tr("repair_rings=%1").arg(request.circle_num_repair()),
        tr("repair_times=%1").arg(request.times_repair()),
        tr("isLast=%1").arg(request.islast() ? tr("true") : tr("false"))
    };
    return parts.join(QStringLiteral(", "));
}

QString MainWindow::formatDelay(const DelayData& request) const {
    QStringList parts{
        tr("jump=%1").arg(request.jump_speed()),
        tr("laser_on=%1").arg(request.laser_on_delay()),
        tr("laser_off=%1").arg(request.laser_off_delay()),
        tr("mark=%1").arg(request.mark_delay()),
        tr("jump_delay=%1").arg(request.jump_delay()),
        tr("polygon=%1").arg(request.polygon_delay())
    };
    return parts.join(QStringLiteral(", "));
}

QString MainWindow::formatFreq(const FreqData& request) const {
    return tr("freq=%1").arg(request.freq());
}