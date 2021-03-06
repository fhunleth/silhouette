#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QStandardPaths>
#include <QProcess>
#include <QTemporaryFile>
#include <QDebug>

#include "math.h"

#define OBSTRUCTION_RES 4096

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    obstructionResolutionWidth_(OBSTRUCTION_RES),
    obstructionResolutionHeight_(OBSTRUCTION_RES),
    obstructionWidthCm_(30),
    obstructionHeightCm_(30),
    obstructionZCm_(10),

    lightXCm_(19.5),
    lightYCm_(2),

    shadowZCm_(110),
    shadowYCm_(0),
    shadowWidthCm_(100),
    shadowHeightCm_(100),

    addPedestal_(false),
    mirrorSilhouette_(false)
{
    ui->setupUi(this);
    connect(ui->actionQuit, SIGNAL(triggered()), SLOT(close()));
    ui->silhouetteSizeCmDoubleSpinBox->setValue(shadowHeightCm_);
    ui->obstructionSizeCmDoubleSpinBox->setValue(obstructionHeightCm_);
    ui->lightDistanceFromWallCmDoubleSpinBox->setValue(lightXCm_);
    ui->lightHeightCmDoubleSpinBox->setValue(lightYCm_);
    ui->shadowHeightOffsetDoubleSpinBox->setValue(shadowYCm_);
    ui->shadowOffsetFromObstructionDoubleSpinBox->setValue(shadowZCm_ - shadowWidthCm_ - obstructionZCm_);
    ui->distanceBetweenLightAndObstructionDoubleSpinBox->setValue(obstructionZCm_);

    connect(ui->silhouetteSizeCmDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(shadowSizeChanged(double)));
    connect(ui->obstructionSizeCmDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(obstructionSizeChanged(double)));
    connect(ui->lightDistanceFromWallCmDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(lightDistanceFromWallChanged(double)));
    connect(ui->lightHeightCmDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(lightHeightChanged(double)));
    connect(ui->shadowHeightOffsetDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(shadowHeightOffsetChanged(double)));
    connect(ui->shadowOffsetFromObstructionDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(shadowWallOffsetChanged(double)));
    connect(ui->distanceBetweenLightAndObstructionDoubleSpinBox, SIGNAL(valueChanged(double)), SLOT(lightObstructionDistanceChanged(double)));
    connect(ui->addPedestalAndDotCheckBox, SIGNAL(stateChanged(int)), SLOT(addPedestalChanged(int)));
    connect(ui->mirrorSilhouetteCheckBox, SIGNAL(stateChanged(int)), SLOT(mirrorSilhouetteChanged(int)));

    setSilouette(QImage(":res/polar-bear-silhouette.png"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setSilouette(const QImage &image)
{
    double sizeMultiplier = 8;
    int side = qRound(qMax(image.width(), image.height()) * sizeMultiplier);
    QImage squareImage(side, side, QImage::Format_ARGB32);
    QPainter p(&squareImage);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(0, 0, side, side, Qt::white);
    QRectF target((side - sizeMultiplier *image.width()) / 2,  side - sizeMultiplier *image.height(), sizeMultiplier *image.width(), sizeMultiplier *image.height());
    QRectF source(0, 0, image.width(), image.height());
    p.drawImage(target, image, source);
    p.end();

    originalShadowImage_ = squareImage;
    if (mirrorSilhouette_)
        shadowImage_ = originalShadowImage_.mirrored(true, false);
    else
        shadowImage_ = originalShadowImage_;

    ui->silhouetteImage->setPixmap(QPixmap::fromImage(shadowImage_));
    recalculate();
}

static QString getHomeDir()
{
    QStringList homeLocations = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    return homeLocations.isEmpty() ? QString() : homeLocations.at(0);
}

void MainWindow::on_actionLoad_image_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
          tr("Open Shadow Image"), getHomeDir(), tr("Image Files (*.png *.jpg *.bmp)"));
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
          tr("Save Obstruction Image"), getHomeDir(), tr("Image Files (*.png)"));
    if (!filename.isEmpty()) {
        if (!obstructionImage_.save(filename))
            QMessageBox::warning(this, tr("Save Obstruction Image"), tr("Error saving image!"));
    }
}

void MainWindow::on_actionSave_obstruction_to_dxf_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this,
          tr("Save Obstruction Image"), getHomeDir(), tr("Image Files (*.dxf)"));
    if (!filename.isEmpty()) {
        QTemporaryFile tmp;
        if (!tmp.open() || !obstructionImage_.save(&tmp, "PPM")) {
            QMessageBox::warning(this, tr("Save Obstruction Image"), tr("Error saving image!"));
            return;
        }

        QStringList arguments;
        arguments << "-o" << filename << "-b" << "dxf"
                  << "-W" << QString::number(obstructionWidthCm_ * 10, 'f')
                  << "-H" << QString::number(obstructionHeightCm_ * 10, 'f')
                  << tmp.fileName();
        qDebug() << arguments;
        QProcess potrace;
        potrace.start("/usr/local/bin/potrace", arguments);
        if (!potrace.waitForFinished() ||
                potrace.exitCode() != 0) {
            qDebug() << potrace.exitCode();
            qDebug() << potrace.readAllStandardError();
            qDebug() << potrace.readAllStandardOutput();

            QMessageBox::warning(this, tr("Save Obstruction Image"), tr("Error running potrace!"));
            return;
        }
    }
}

static double sq(double x)
{
    return x * x;
}

static double calculateAngle(double x1, double y1, double z1, double x2, double y2, double z2)
{
    double dotProduct = x1*x2 + y1*y2 + z1*z2;
    double magnitudeLT = sqrt(sq(x1) + sq(y1) + sq(z1));
    double magnitudeRB = sqrt(sq(x2) + sq(y2) + sq(z2));
    return acos(dotProduct / (magnitudeLT*magnitudeRB)) * 180.0 / M_PI;
}

void MainWindow::recalculate()
{
    QImage obstruction(obstructionResolutionWidth_, obstructionResolutionHeight_, QImage::Format_ARGB32);
    int sumx = 0;
    int sumy = 0;
    int pixelCount = 0;

    int minx = obstructionResolutionWidth_;
    int miny = obstructionResolutionHeight_;
    int maxx = 0;
    int maxy = 0;

    for (int y = 0; y < obstructionResolutionHeight_; y++) {
        for (int x = 0; x < obstructionResolutionWidth_; x++) {
            bool isOn = calculatePixel(x, y);
            if (isOn) {
                obstruction.setPixelColor(x, y, Qt::black);
                sumx += x;
                sumy += y;
                pixelCount++;
                maxx = qMax(maxx, x);
                maxy = qMax(maxy, y);
                minx = qMin(minx, x);
                miny = qMin(miny, y);
            } else {
                obstruction.setPixelColor(x, y, Qt::white);
            }
        }
    }

    if (pixelCount > 0) {
        obstructionCenterXCm_ = obstructionWidthCm_ / obstructionResolutionWidth_ * sumx / pixelCount;
        obstructionCenterYCm_ = obstructionHeightCm_ / obstructionResolutionHeight_ * (obstructionResolutionHeight_ - ((double) sumy / pixelCount));

        double left = obstructionWidthCm_ / obstructionResolutionWidth_ * minx;
        double right = obstructionWidthCm_ / obstructionResolutionWidth_ * maxx;
        double top = obstructionHeightCm_ / obstructionResolutionHeight_ * (obstructionResolutionHeight_ - miny);
        double bottom = obstructionHeightCm_ / obstructionResolutionHeight_ * (obstructionResolutionHeight_ - maxy);

        // calculate the angle between the lines that start at the light and go to (left, top) and (right, bottom)
        // if light->(left,top) is a and light->(right,bottom) is b, then cos θ = a·b/|a||b|
        obstructionLightAngle_ = calculateAngle(left - lightXCm_, top - lightYCm_, obstructionZCm_,
                                                right - lightXCm_, bottom - lightYCm_, obstructionZCm_);

        lightPitchAngle_ = calculateAngle(obstructionCenterXCm_ - lightXCm_, obstructionCenterYCm_ - lightYCm_, obstructionZCm_,
                                          obstructionCenterXCm_ - lightXCm_, 0, obstructionZCm_);
        if (obstructionCenterYCm_ < lightYCm_)
            lightPitchAngle_ = -lightPitchAngle_;

        lightYawAngle_ = calculateAngle(obstructionCenterXCm_ - lightXCm_, 0, obstructionZCm_,
                                        0, 0, obstructionZCm_);

        if (addPedestal_) {
            obstruction.setPixelColor(0, 0, Qt::black);

            QPainter p(&obstruction);
            p.fillRect(0, maxy - 4, maxx, obstructionResolutionHeight_, Qt::black);
        }
    } else {
        obstructionCenterXCm_ = 0;
        obstructionCenterYCm_ = 0;
        obstructionLightAngle_ = 0;
        lightPitchAngle_ = 0;
        lightYawAngle_ = 0;
    }

    ui->shadowInfoLabel->setText(tr("Obstruction: Center is (%1 cm, %2 cm), Min LED \"viewing\" angle is %3°, LED pitch %4°, LED yaw %5°")
                                 .arg(obstructionCenterXCm_, 0, 'f', 1)
                                 .arg(obstructionCenterYCm_, 0, 'f', 1)
                                 .arg(obstructionLightAngle_ / 2.0, 0, 'f', 1)
                                 .arg(lightPitchAngle_, 0, 'f', 1)
                                 .arg(lightYawAngle_, 0, 'f', 1));

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
    double obYCm = (obstructionResolutionHeight_ - 1 - y) * obstructionHeightCm_ / obstructionResolutionHeight_;

    double slopex = (obXCm - lightXCm_) / obstructionZCm_;
    double slopey = (obYCm - lightYCm_) / obstructionZCm_;

    // If the light never hits the wall, then don't bother calculating the intersection
    if (slopex >= 0.0)
        return false;

    // Calculate where the light ray hits the wall (x=0)
    double wallZCm = -lightXCm_ / slopex;
    double wallYCm = lightYCm_ + wallZCm * slopey;

    // Calculate where the light ray hits in the image
    // We're choosing the bottom-left of the image to be (0, 0)
    double pictureXCm = shadowZCm_ - wallZCm;
    if (pictureXCm < 0 || pictureXCm >= shadowWidthCm_)
        return false;

    double pictureYCm = wallYCm - shadowYCm_;
    if (pictureYCm <= 0 || pictureYCm > shadowHeightCm_)
        return false;

    // Point sample...
    int picturePixelX = (int) floor(pictureXCm / shadowWidthCm_ * shadowImage_.width());
    int picturePixelY = shadowImage_.height() - (int) ceil(pictureYCm / shadowHeightCm_ * shadowImage_.height());

    if (picturePixelX >= shadowImage_.width() || picturePixelY >= shadowImage_.height() || picturePixelX < 0 || picturePixelY < 0)
        qDebug("oops");

    QRgb color = shadowImage_.pixel(picturePixelX, picturePixelY);
    if (qAlpha(color) > 128 && qGreen(color) < 128) {
      //  qDebug("%f, %f", obXCm, obYCm);
        return true;
    } else
        return false;
}

void MainWindow::shadowSizeChanged(double v)
{
    shadowHeightCm_ = shadowWidthCm_ = v;
    shadowZCm_ = obstructionZCm_ + shadowWidthCm_ + ui->shadowOffsetFromObstructionDoubleSpinBox->value();

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

void MainWindow::lightObstructionDistanceChanged(double v)
{
    double offset = shadowZCm_ - shadowWidthCm_ - obstructionZCm_;
    obstructionZCm_ = v;
    shadowZCm_ = shadowWidthCm_ + obstructionZCm_ + offset;
    recalculate();
}

void MainWindow::addPedestalChanged(int)
{
    addPedestal_ = ui->addPedestalAndDotCheckBox->isChecked();
    recalculate();
}

void MainWindow::mirrorSilhouetteChanged(int)
{
    mirrorSilhouette_ = ui->mirrorSilhouetteCheckBox->isChecked();
    if (mirrorSilhouette_)
        shadowImage_ = originalShadowImage_.mirrored(true, false);
    else
        shadowImage_ = originalShadowImage_;

    ui->silhouetteImage->setPixmap(QPixmap::fromImage(shadowImage_));
    recalculate();
}
