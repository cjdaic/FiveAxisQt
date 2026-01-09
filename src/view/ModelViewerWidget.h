#pragma once

#include <QWidget>

#include <vtkSmartPointer.h>

class QVTKOpenGLNativeWidget;
class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class vtkActor;

class ModelViewerWidget : public QWidget {
    Q_OBJECT
public:
    explicit ModelViewerWidget(QWidget* parent = nullptr);

public slots:
    void loadModelFromDialog();
    bool loadModel(const QString& filePath);
    void showSampleModel();

signals:
    void modelLoaded(const QString& filePath);
    void modelLoadFailed(const QString& filePath, const QString& reason);

private:
    void setupRenderer();
    void resetScene();

    QVTKOpenGLNativeWidget* m_vtkWidget{};
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkActor> m_modelActor;
};