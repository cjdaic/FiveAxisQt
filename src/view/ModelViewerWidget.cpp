#include "ModelViewerWidget.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QVTKOpenGLNativeWidget.h>

#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkConeSource.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOBJReader.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSTLReader.h>

namespace {
    vtkSmartPointer<vtkPolyDataAlgorithm> readerForFile(const QString& filePath) {
        const QString suffix = QFileInfo(filePath).suffix().toLower();
        if (suffix == QStringLiteral("stl")) {
            auto reader = vtkSmartPointer<vtkSTLReader>::New();
            reader->SetFileName(filePath.toLocal8Bit().constData());
            return reader;
        }
        if (suffix == QStringLiteral("obj")) {
            auto reader = vtkSmartPointer<vtkOBJReader>::New();
            reader->SetFileName(filePath.toLocal8Bit().constData());
            return reader;
        }
        return nullptr;
    }
}

ModelViewerWidget::ModelViewerWidget(QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_vtkWidget = new QVTKOpenGLNativeWidget(this);
    layout->addWidget(m_vtkWidget, 1);

    setupRenderer();
    showSampleModel();
}

void ModelViewerWidget::setupRenderer() {
    auto colors = vtkSmartPointer<vtkNamedColors>::New();

    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->SetBackground(colors->GetColor3d("SlateGray").GetData());
    m_renderWindow->AddRenderer(m_renderer);
    m_vtkWidget->setRenderWindow(m_renderWindow);

    vtkNew<vtkAxesActor> axes;
    axes->AxisLabelsOn();
    axes->SetTotalLength(20.0, 20.0, 20.0);

    vtkNew<vtkOrientationMarkerWidget> markerWidget;
    markerWidget->SetOrientationMarker(axes);
    markerWidget->SetInteractor(m_vtkWidget->interactor());
    markerWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
    markerWidget->SetEnabled(1);
    markerWidget->InteractiveOff();
}

void ModelViewerWidget::resetScene() {
    if (m_modelActor) {
        m_renderer->RemoveActor(m_modelActor);
        m_modelActor = nullptr;
    }
}

bool ModelViewerWidget::loadModel(const QString& filePath) {
    auto reader = readerForFile(filePath);
    if (!reader) {
        emit modelLoadFailed(filePath, tr("unsupported format %1").arg(QFileInfo(filePath).suffix()));
        return false;
    }

    reader->Update();
    if (!reader->GetOutput() || reader->GetOutput()->GetNumberOfPoints() == 0) {
        emit modelLoadFailed(filePath, tr("file geodata unavailable"));
        return false;
    }

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(reader->GetOutputPort());

    auto colors = vtkSmartPointer<vtkNamedColors>::New();
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(colors->GetColor3d("LightSteelBlue").GetData());

    resetScene();
    m_modelActor = actor;
    m_renderer->AddActor(actor);
    m_renderer->ResetCamera();
    m_renderWindow->Render();

    emit modelLoaded(filePath);
    return true;
}

void ModelViewerWidget::showSampleModel() {
    auto cone = vtkSmartPointer<vtkConeSource>::New();
    cone->SetHeight(30.0);
    cone->SetRadius(10.0);
    cone->SetResolution(60);
    cone->Update();

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(cone->GetOutputPort());

    auto colors = vtkSmartPointer<vtkNamedColors>::New();
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(colors->GetColor3d("LightSkyBlue").GetData());

    resetScene();
    m_modelActor = actor;
    m_renderer->AddActor(actor);
    m_renderer->ResetCamera();
    m_renderWindow->Render();
}

void ModelViewerWidget::loadModelFromDialog() {
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("import 3d model"),
        QString(),
        tr("3D 模型文件 (*.stl *.obj);;所有文件 (*)"));
    if (filePath.isEmpty()) {
        return;
    }
    loadModel(filePath);
}
