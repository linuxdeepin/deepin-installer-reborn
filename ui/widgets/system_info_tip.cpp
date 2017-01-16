// Copyright (c) 2016 Deepin Ltd. All rights reserved.
// Use of this source is governed by General Public License that can be found
// in the LICENSE file.

#include "ui/widgets/system_info_tip.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

namespace installer {

namespace {

// Top margin of window relative to its original position.
const int kWindowTopMargin = -3;

const int kBorderRadius = 4;
const int kBorderDiameter = kBorderRadius * 2;

// _____
// \ | /
//  \|/
const int kTriangleHeight = 6;
const int kTriangleWidth = 10;
const int kTriangleHalfWidth = kTriangleWidth / 2;
const int kTriangleLeftMargin = 36;

// Same width as line edit.
const int kMaximumWindowWidth = 550;
const int kMinimumWindowHeight = 32;
const int kWindowMarginLeft = 8;
const int kWindowMarginTop = 8;
const int kWindowMarginRight = 8;
const int kWindowMarginBottom = 6;
const int kMaximumLabelWidth = kMaximumWindowWidth - kWindowMarginLeft -
                               kWindowMarginRight;

}  // namespace

SystemInfoTip::SystemInfoTip(QWidget* parent) : QFrame(parent) {
  this->setObjectName("system_info_tip");

  this->initUI();
}

void SystemInfoTip::setText(const QString& text) {
  label_->setText(text);

  // Width of label is based on its content.
  const QFontMetrics metrics(label_->font());
  const int content_width = metrics.width(text);
  // Set world-wrap flag of label based on its content.
  if (content_width < kMaximumLabelWidth) {
    label_->setWordWrap(false);
    label_->setFixedWidth(content_width);
  } else {
    label_->setWordWrap(true);
    label_->setFixedWidth(kMaximumWindowWidth);
  }

  // Also resize container to fit size of label.
  this->adjustSize();
}

void SystemInfoTip::showBottom(QWidget* widget) {
  // Move this to bottom of |widget|.
  const int y = widget->y() + widget->height() + kWindowTopMargin;
  this->move(widget->x(), y);
  this->show();
  this->raise();
}

void SystemInfoTip::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing,
                         true);
  const int kWidth = this->width();
  const int kHeight = this->height();

  // First draw background with round corner.
  QPainterPath background_path;
  background_path.moveTo(kWidth, kBorderRadius + kTriangleHeight);
  background_path.arcTo(kWidth - kBorderDiameter, kTriangleHeight,
                        kBorderDiameter, kBorderDiameter,
                        0.0, 90.0);
  // Draw isosceles right-angled triangle 10px to top left side
  background_path.lineTo(kTriangleLeftMargin + kTriangleWidth, kTriangleHeight);
  background_path.lineTo(kTriangleLeftMargin + kTriangleHeight, 0);
  background_path.lineTo(kTriangleLeftMargin, kTriangleHeight);
  background_path.lineTo(kBorderRadius, kTriangleHeight);
  background_path.arcTo(0, kTriangleHeight,
                        kBorderDiameter, kBorderDiameter,
                        90.0, 90.0);
  background_path.lineTo(0, kHeight - kBorderRadius);
  background_path.arcTo(0, kHeight - kBorderDiameter,
                        kBorderDiameter, kBorderDiameter,
                        180.0, 90.0);
  background_path.lineTo(kWidth - kBorderRadius, kHeight);
  background_path.arcTo(kWidth - kBorderDiameter, kHeight - kBorderDiameter,
                        kBorderDiameter, kBorderDiameter,
                        270.0, 90.0);
  background_path.closeSubpath();
  painter.fillPath(background_path, QBrush(Qt::white));
}

void SystemInfoTip::initUI() {
  label_ = new QLabel();
  label_->setStyleSheet("background: transparent;"
                        "color: #f9704f;"
                        "font-size: 12px;");
  label_->setWordWrap(false);
  label_->setMaximumWidth(kMaximumLabelWidth);

  QHBoxLayout* layout = new QHBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(label_);

  this->setLayout(layout);
  this->setContentsMargins(kWindowMarginLeft, kWindowMarginTop,
                           kWindowMarginRight, kWindowMarginBottom);
  this->setMaximumWidth(kMaximumWindowWidth);
  this->setMinimumHeight(kMinimumWindowHeight);
}

}  // namespace installer