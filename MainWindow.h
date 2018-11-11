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
private:
    void recalculate();

private:
    Ui::MainWindow *ui;

    QImage silouetteImage_;

    int obstructionResolutionWidth_;
    int obstructionResolutionHeight_;
    double obstructionWidthCm_;
    double obstructionHeightCm_;
    double obstructionZCm_;

    double lightXCm_;
    double lightYCm_;

    double shadowZCm_;
    double shadowYCm_;
    double shadowWidthCm_;
    double shadowHeightCm_;

    bool calculatePixel(int x, int y);
    void setSilouette(const QImage &image);
};

#endif // MAINWINDOW_H
