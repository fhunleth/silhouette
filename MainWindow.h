#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionLoad_image_triggered();
    void on_actionSave_obstruction_triggered();

    void shadowSizeChanged(double v);
    void obstructionSizeChanged(double v);
    void lightDistanceFromWallChanged(double v);
    void lightHeightChanged(double v);
    void shadowHeightOffsetChanged(double v);
    void shadowWallOffsetChanged(double v);
    void lightObstructionDistanceChanged(double v);

    void addPedestalChanged(int);
    void mirrorSilhouetteChanged(int);
    void on_actionSave_obstruction_to_dxf_triggered();
private:
    void recalculate();

private:
    Ui::MainWindow *ui;

    QImage originalShadowImage_;
    QImage shadowImage_;
    QImage obstructionImage_;

    int obstructionResolutionWidth_;
    int obstructionResolutionHeight_;
    double obstructionWidthCm_;
    double obstructionHeightCm_;
    double obstructionZCm_;
    double obstructionLightAngle_;

    double lightXCm_;
    double lightYCm_;

    double shadowZCm_;
    double shadowYCm_;
    double shadowWidthCm_;
    double shadowHeightCm_;

    // In shadow coordinates (lower left is (0, 0))
    double obstructionCenterXCm_;
    double obstructionCenterYCm_;
    double lightPitchAngle_;
    double lightYawAngle_;

    bool addPedestal_;
    bool mirrorSilhouette_;

    bool calculatePixel(int x, int y);
    void setSilouette(const QImage &image);
};

#endif // MAINWINDOW_H
