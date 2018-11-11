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
    ui->shadowHeightOffsetDoubleSpinBox->setValue(shadowYCm_);
    ui->shadowOffsetFromObstructionDoubleSpinBox->setValue(shadowZCm_ - shadowWidthCm_ - obstructionZCm_);

    connect(ui->silhouetteSizeCmDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(shadowSizeChanged(double)));
    connect(ui->obstructionSizeCmDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(obstructionSizeChanged(double)));
    connect(ui->lightDistanceFromWallCmDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(lightDistanceFromWallChanged(double)));
    connect(ui->lightHeightCmDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(lightHeightChanged(double)));
    connect(ui->shadowHeightOffsetDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(shadowHeightOffsetChanged(double)));
    connect(ui->shadowOffsetFromObstructionDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(shadowWallOffsetChanged(double)));

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

    shadowImage_ = squareImage;
    ui->silhouetteImage->setPixmap(QPixmap::fromImage(shadowImage_));
    recalculate();
}

void MainWindow::on_actionLoad_image_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
          tr("Open Shadow Image"), QString(), tr("Image Files (*.png *.jpg *.bmp)"));
    if (filename.isEmpty())
        return;

    QImage image = QImage(filename);
    if (image.isNull()) {
        QMessageBox::warning(this, tr("Open Shadow Image"), tr("Error opening image!"));
    } else {
        // Put the image at the bottom of a square image
        setSilouette(image);
    }
}

void MainWindow::on_actionSave_obstruction_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this,
          tr("Save Obstruction Image"), QString(), tr("Image Files (*.png *.jpg *.bmp)"));
    if (!filename.isEmpty()) {
        if (!obstructionImage_.save(filename))
            QMessageBox::warning(this, tr("Save Obstruction Image"), tr("Error saving image!"));
    }
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

    obstructionImage_ = obstruction;
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

    if (slopex == 0 || slopey == 0)
        return false;

    // Calculate where the light ray hits the wall
    double wallZCm = -lightXCm_ / slopex;
    double wallYCm = -lightYCm_ / slopey;

    // Calculate where the light ray hits in the image
    // We're choosing the bottom-left of the image to be (0, 0)
    double pictureXCm = shadowZCm_ - wallZCm;
    if (pictureXCm < 0 || pictureXCm >= shadowWidthCm_)
        return false;

    double pictureYCm = shadowYCm_ - wallYCm;
    if (pictureYCm <= 0 || pictureYCm > shadowHeightCm_)
        return false;

    // Point sample...
    int picturePixelX = (int) floor(pictureXCm / shadowWidthCm_ * shadowImage_.width());
    int picturePixelY = shadowImage_.height() - (int) ceil(pictureYCm / shadowHeightCm_ * shadowImage_.height());

    if (picturePixelX >= shadowImage_.width() || picturePixelY >= shadowImage_.height() || picturePixelX < 0 || picturePixelY < 0)
        qDebug("oops");

    QRgb color = shadowImage_.pixel(picturePixelX, picturePixelY);
    if (qAlpha(color) > 128 && qGreen(color) < 128)
        return true;
    else
        return false;
}

void MainWindow::shadowSizeChanged(double v)
{
    double oldSize = shadowWidthCm_;

    shadowHeightCm_ = shadowWidthCm_ = v;
    shadowZCm_ = obstructionZCm_ - oldSize + shadowWidthCm_ + ui->shadowOffsetFromObstructionDoubleSpinBox->value();

    recalculate();
}

void MainWindow::obstructionSizeChanged(double v)
{
    obstructionHeightCm_ = obstructionWidthCm_ = v;
    recalculate();
}

void MainWindow::lightDistanceFromWallChanged(double v)
{
    lightXCm_ = v;
    recalculate();
}

void MainWindow::lightHeightChanged(double v)
{
    lightYCm_ = v;
    recalculate();
}

void MainWindow::shadowHeightOffsetChanged(double v)
{
    shadowYCm_ = v;
    recalculate();
}

void MainWindow::shadowWallOffsetChanged(double v)
{
    double oldOffset = shadowZCm_ - shadowWidthCm_ - obstructionZCm_;
    shadowZCm_ += v - oldOffset;
    recalculate();
}
