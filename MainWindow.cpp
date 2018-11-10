#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    obstructionResolutionWidth_(1024),
    obstructionResolutionHeight_(1024),
    obstructionWidthCm_(10),
    obstructionHeightCm_(10),
    obstructionZCm_(20),

    lightXCm_(5),
    lightYCm_(3),

    silouetteZCm_(45),
    silouetteYCm_(0),
    silouetteWidthCm_(25),
    silouetteHeightCm_(25)

{
    ui->setupUi(this);
    connect(ui->actionQuit, SIGNAL(triggered()), SLOT(close()));

    silouetteImage_ = QImage(":res/polar-bear-silhouette.png");
    ui->silhouetteImage->setPixmap(QPixmap::fromImage(silouetteImage_));
    recalculate();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionLoad_image_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
          tr("Open Silouette Image"), QString(), tr("Image Files (*.png *.jpg *.bmp)"));

    QImage image = QImage(filename);
    if (image.isNull()) {
        QMessageBox::warning(this, tr("Open Silouette Image"), tr("The image couldn't be opened."));
    } else {
        silouetteImage_ = image;
        ui->silhouetteImage->setPixmap(QPixmap::fromImage(silouetteImage_));
        recalculate();
    }
}

void MainWindow::on_actionSave_obstruction_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this,
          tr("Save Obstruction Image"), QString(), tr("Image Files (*.png *.jpg *.bmp)"));

}

void MainWindow::recalculate()
{
    QImage obstruction(obstructionResolutionWidth_, obstructionResolutionHeight_, QImage::Format_ARGB32);

    for (int y = 0; y < obstructionResolutionHeight_; y++) {
        for (int x = 0; x < obstructionResolutionWidth_; x++) {
            bool isOn = calculatePixel(x, y);
            obstruction.setPixelColor(x, y, isOn ? Qt::black : Qt::white);
        }
    }

    ui->obstructionImage->setPixmap(QPixmap::fromImage(obstruction));
}

bool MainWindow::calculatePixel(int x, int y)
{
    double obXCm = x * obstructionWidthCm_ / obstructionResolutionWidth_;
    double obYCm = y * obstructionHeightCm_ / obstructionResolutionHeight_;

    double slopex = (obXCm - lightXCm_) / obstructionZCm_;
    double slopey = (obYCm - lightYCm_) / obstructionZCm_;

    // Calculate where the light ray hits the wall
    double wallZCm = -lightXCm_ / slopex;
    double wallYCm = -lightYCm_ / slopey;

    // Calculate where the light ray hits in the image
    // We're choosing the bottom-left of the image to be (0, 0)
    double pictureXCm = silouetteZCm_ - wallZCm;
    if (pictureXCm < 0 || pictureXCm > silouetteWidthCm_)
        return false;

    double pictureYCm = silouetteYCm_ - wallYCm;
    if (pictureYCm < 0 || pictureYCm > silouetteHeightCm_)
        return false;

    // Point sample...
    int picturePixelX = qRound(pictureXCm / silouetteWidthCm_ * silouetteImage_.width());
    int picturePixelY = silouetteImage_.height() - qRound(pictureYCm / silouetteHeightCm_ * silouetteImage_.height());

    QRgb color = silouetteImage_.pixel(picturePixelX, picturePixelY);
    if (qGreen(color) < 128)
        return true;
    else
        return false;
}
