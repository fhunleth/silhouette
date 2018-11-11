#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include "math.h"

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

    shadowZCm_(120),
    shadowYCm_(0),
    shadowWidthCm_(100),
    shadowHeightCm_(100)

{
    ui->setupUi(this);
    connect(ui->actionQuit, SIGNAL(triggered()), SLOT(close()));
    ui->silhouetteSizeCmDoubleSpinBox->setValue(shadowHeightCm_);
    ui->obstructionSizeCmDoubleSpinBox->setValue(obstructionHeightCm_);
    ui->lightDistanceFromWallCmDoubleSpinBox->setValue(lightXCm_);
    ui->lightHeightCmDoubleSpinBox->setValue(lightYCm_);

    connect(ui->silhouetteSizeCmDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(shadowSizeChanged(double)));

    setSilouette(QImage(":res/polar-bear-silhouette.png"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setSilouette(const QImage &image)
{
    int side = qMax(image.width(), image.height());
    QImage squareImage(side, side, QImage::Format_ARGB32);
    QPainter p(&squareImage);
    p.fillRect(0, 0, side, side, Qt::white);
    p.drawImage((side - image.width()) / 2, side - image.height(), image);
    p.end();

    silouetteImage_ = squareImage;
    ui->silhouetteImage->setPixmap(QPixmap::fromImage(silouetteImage_));
    recalculate();
}

void MainWindow::on_actionLoad_image_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
          tr("Open Silouette Image"), QString(), tr("Image Files (*.png *.jpg *.bmp)"));

    QImage image = QImage(filename);
    if (image.isNull()) {
        QMessageBox::warning(this, tr("Open Silouette Image"), tr("The image couldn't be opened."));
    } else {
        // Put the image at the bottom of a square image
        setSilouette(image);
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
    ui->obstructionWidthLabel->setText(tr("%1cm").arg(obstructionWidthCm_));
    ui->obstructionHeightLabel->setText(tr("%1cm").arg(obstructionHeightCm_));
    ui->silhouetteWidthLabel->setText(tr("%1cm").arg(shadowWidthCm_));
    ui->silhouetteHeightLabel->setText(tr("%1cm").arg(shadowHeightCm_));
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
    double pictureXCm = shadowZCm_ - wallZCm;
    if (pictureXCm < 0 || pictureXCm >= shadowWidthCm_)
        return false;

    double pictureYCm = shadowYCm_ - wallYCm;
    if (pictureYCm < 0 || pictureYCm >= shadowHeightCm_)
        return false;

    // Point sample...
    int picturePixelX = (int) floor(pictureXCm / shadowWidthCm_ * silouetteImage_.width());
    int picturePixelY = silouetteImage_.height() - (int) ceil(pictureYCm / shadowHeightCm_ * silouetteImage_.height());

    QRgb color = silouetteImage_.pixel(picturePixelX, picturePixelY);
    if (qAlpha(color) > 128 && qGreen(color) < 128)
        return true;
    else
        return false;
}

void MainWindow::shadowSizeChanged(double v)
{
    shadowHeightCm_ = shadowWidthCm_ = v;
    recalculate();
}
