#pragma once

#include <QMainWindow>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QHash>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeWidget>
#include <QUrl>
#include <QTextEdit>
#include "view/ModelViewerWidget.h"
#include "grpc/FiveAxisClient.h"
#include "view/DrawingPanel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void connectToServer();
    void sendLine();
    void sendCircle();
    void sendRectangle();
    void sendEllipse();
    void applyDelay();
    void applyFreq();
    void onReply(const QString& operation, const QString& message);
    void onError(const QString& operation, int code, const QString& message);
    void onShapeCreated(const QString& id, const QString& type);
    void onShapeSelected(const QString& id, const QString& type);
    void onShapeMoved(const QString& id, const QString& type);
    void onShapeRemoved(const QString& id, const QString& type);
    void showProjectContextMenu(const QPoint& pos);
    void onProjectSelectionChanged();
    void importModel();
    void showSampleModel();
private:
    void buildUi();
    QWidget* buildLineTab();
    QWidget* buildCircleTab();
    QWidget* buildRectangleTab();
    QWidget* buildEllipseTab();
    QWidget* buildSettingsTab();
    QString formatLine(const LineData& request) const;
    QString formatCircle(const CircleData& request) const;
    QString formatRectangle(const RectangleData& request) const;
    QString formatEllipse(const EllipseData& request) const;
    QString formatDelay(const DelayData& request) const;
    QString formatFreq(const FreqData& request) const;

    QSplitter* m_splitter{};
    QTreeWidget* m_projectTree{};
    QTreeWidgetItem* m_sceneRoot{};
    DrawingPanel* m_scenePreview{};
    ModelViewerWidget* m_modelViewer{};
    QTabWidget* m_propertyTabs{};
    QTextEdit* m_log{};
    FiveAxisClient* m_client{};
    QHash<QString, QTreeWidgetItem*> m_treeItems;

    // Line widgets
    QDoubleSpinBox* m_lineSpeed{};
    QSpinBox* m_lineTimes{};
    QDoubleSpinBox* m_lineX1{};
    QDoubleSpinBox* m_lineY1{};
    QDoubleSpinBox* m_lineZ1{};
    QDoubleSpinBox* m_lineA1{};
    QDoubleSpinBox* m_lineB1{};
    QDoubleSpinBox* m_lineX2{};
    QDoubleSpinBox* m_lineY2{};
    QDoubleSpinBox* m_lineZ2{};
    QDoubleSpinBox* m_lineA2{};
    QDoubleSpinBox* m_lineB2{};

    // Circle widgets
    QDoubleSpinBox* m_circleSpeed{};
    QSpinBox* m_circleTimes{};
    QDoubleSpinBox* m_circleX1{};
    QDoubleSpinBox* m_circleY1{};
    QDoubleSpinBox* m_circleX2{};
    QDoubleSpinBox* m_circleY2{};
    QSpinBox* m_circleM{};
    QDoubleSpinBox* m_circleAngle{};
    QDoubleSpinBox* m_circleTaper{};
    QCheckBox* m_circleFilled{};
    QDoubleSpinBox* m_circleRMin{};
    QDoubleSpinBox* m_circleRInterval{};
    QDoubleSpinBox* m_circleZStart{};
    QDoubleSpinBox* m_circleZEnd{};
    QDoubleSpinBox* m_circleZInterval{};
    QSpinBox* m_circleRepairNum{};
    QSpinBox* m_circleRepairTimes{};

    // Rectangle widgets
    QDoubleSpinBox* m_rectX0{};
    QDoubleSpinBox* m_rectY0{};
    QDoubleSpinBox* m_rectX1{};
    QDoubleSpinBox* m_rectY1{};
    QDoubleSpinBox* m_rectTaperA{};
    QDoubleSpinBox* m_rectTaperB{};
    QDoubleSpinBox* m_rectFeedX{};
    QDoubleSpinBox* m_rectFeedY{};
    QDoubleSpinBox* m_rectSpeed{};
    QDoubleSpinBox* m_rectZStart{};
    QDoubleSpinBox* m_rectZEnd{};
    QDoubleSpinBox* m_rectZInterval{};
    QSpinBox* m_rectTimes{};
    QSpinBox* m_rectRepairNum{};
    QSpinBox* m_rectRepairTimes{};
    QDoubleSpinBox* m_rectX2{};
    QDoubleSpinBox* m_rectY2{};

    // Ellipse widgets
    QDoubleSpinBox* m_ellipseSpeed{};
    QSpinBox* m_ellipseTimes{};
    QDoubleSpinBox* m_ellipseX0{};
    QDoubleSpinBox* m_ellipseY0{};
    QDoubleSpinBox* m_ellipseAMax{};
    QDoubleSpinBox* m_ellipseBMax{};
    QDoubleSpinBox* m_ellipseAMin{};
    QDoubleSpinBox* m_ellipseBMin{};
    QDoubleSpinBox* m_ellipseTaperA{};
    QDoubleSpinBox* m_ellipseTaperB{};
    QDoubleSpinBox* m_ellipseFeedX{};
    QDoubleSpinBox* m_ellipseFeedY{};
    QDoubleSpinBox* m_ellipseZStart{};
    QDoubleSpinBox* m_ellipseZEnd{};
    QDoubleSpinBox* m_ellipseZInterval{};
    QSpinBox* m_ellipseRepairNum{};
    QSpinBox* m_ellipseRepairTimes{};

    // Settings widgets
    QSpinBox* m_jumpSpeed{};
    QSpinBox* m_laserOnDelay{};
    QSpinBox* m_laserOffDelay{};
    QSpinBox* m_markDelay{};
    QSpinBox* m_jumpDelay{};
    QSpinBox* m_polygonDelay{};
    QSpinBox* m_freq{};
};