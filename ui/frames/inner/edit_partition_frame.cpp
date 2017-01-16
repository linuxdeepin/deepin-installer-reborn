// Copyright (c) 2016 Deepin Ltd. All rights reserved.
// Use of this source is governed by General Public License that can be found
// in the LICENSE file.

#include "ui/frames/inner/edit_partition_frame.h"

#include <QCheckBox>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>

#include "base/file_util.h"
#include "service/settings_manager.h"
#include "service/settings_name.h"
#include "ui/frames/consts.h"
#include "ui/delegates/partition_delegate.h"
#include "ui/delegates/partition_util.h"
#include "ui/models/fs_model.h"
#include "ui/models/mount_point_model.h"
#include "ui/widgets/comment_label.h"
#include "ui/widgets/nav_button.h"
#include "ui/widgets/rounded_progress_bar.h"
#include "ui/widgets/table_combo_box.h"
#include "ui/widgets/title_label.h"

namespace installer {

namespace {

const int kWindowWidth = 640;
const int kProgressBarWidth = 280;

// Check whether partition with |mount_point| should be formatted
// compulsively.
bool IsInFormattedMountPointList(const QString& mount_point) {
  const QStringList list(GetSettingsStringList(kPartitionFormattedMountPoints));
  return list.contains(mount_point);
}

}  // namespace

EditPartitionFrame::EditPartitionFrame(PartitionDelegate* delegate,
                                       QWidget* parent)
    : QFrame(parent),
      delegate_(delegate),
      partition_() {
  this->setObjectName("edit_partition_frame");

  this->initUI();
  this->initConnections();
}

void EditPartitionFrame::setPartition(const Partition& partition) {
  partition_ = partition;

  os_label_->setPixmap(QPixmap(GetOsTypeLargeIcon(partition.os)));
  name_label_->setText(GetPartitionLabelAndPath(partition));
  usage_label_->setText(GetPartitionUsage(partition));
  usage_bar_->setValue(GetPartitionUsageValue(partition));

  // Reset fs index.
  int fs_index = fs_model_->index(partition.fs);
  if (fs_index == -1) {
    // If partition fs type not in current fs_list, selected the first one.
    fs_index = 0;
  }
  fs_box_->setCurrentIndex(fs_index);

  // Reset mount point box. partition.mount_point might be empty.
  const int mp_index = mount_point_model_->index(partition.mount_point);
  mount_point_box_->setCurrentIndex(mp_index);

  switch (partition_.status) {
    case PartitionStatus::Format: {
      // Compare between real filesystem type and current one.
      const Partition real_partition(delegate_->getRealPartition(partition_));
      format_check_box_->setEnabled(real_partition.fs == partition_.fs);
      format_check_box_->setChecked(true);
      break;
    }
    case PartitionStatus::New: {
      // Force format for new partition.
      this->forceFormat(true);
      break;
    }
    case PartitionStatus::Real: {
      // If mount point in ins formatted-mount-point list, format this partition
      // compulsively.
      this->forceFormat(IsInFormattedMountPointList(partition.mount_point));
      break;
    }
    default: {
      break;
    }
  }

  this->updateFormatBoxState();
}

void EditPartitionFrame::changeEvent(QEvent* event) {
  if (event->type() == QEvent::LanguageChange) {
    title_label_->setText(tr("Edit Disk"));
    comment_label_->setText(
        tr("Please make sure you have backed up "
           "important data, then select the disk  to install"));
    fs_label_->setText(tr("Filesystem"));
    mount_point_label_->setText(tr("Mount point"));
    format_label_->setText(tr("Format the partition"));
    cancel_button_->setText(tr("Cancel"));
    ok_button_->setText(tr("OK"));
  } else {
    QFrame::changeEvent(event);
  }
}

void EditPartitionFrame::forceFormat(bool force) {
  format_check_box_->setChecked(force);
  format_check_box_->setEnabled(!force);
}

void EditPartitionFrame::updateFormatBoxState() {
  // Format partition forcefully if its type is changed.
  const FsType fs_type = fs_model_->getFs(fs_box_->currentIndex());
  const Partition real_partition(delegate_->getRealPartition(partition_));
  const int mp_index = mount_point_box_->currentIndex();
  const QString mount_point = mount_point_model_->getMountPoint(mp_index);

  // If fs type changed and used, or mount-point is in formatted-mount-point
  // list, format that partition compulsively.
  bool format = (fs_type != FsType::Empty && fs_type != real_partition.fs);
  format |= IsInFormattedMountPointList(mount_point);
  this->forceFormat(format);

  // If it is linux-swap, hide format_box_ option.
  const bool is_swap = (fs_type == FsType::LinuxSwap);
  format_label_->setVisible(!is_swap);
  format_check_box_->setVisible(!is_swap);
}

void EditPartitionFrame::initConnections() {
  connect(fs_box_,
          static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &EditPartitionFrame::onFsChanged);
  connect(mount_point_box_,
          static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &EditPartitionFrame::onMountPointChanged);

  // Does nothing when cancel-button is clicked.
  connect(cancel_button_, &QPushButton::clicked,
          this, &EditPartitionFrame::finished);
  connect(ok_button_, &QPushButton::clicked,
          this, &EditPartitionFrame::onOkButtonClicked);
}

void EditPartitionFrame::initUI() {
  title_label_ = new TitleLabel(tr("Edit Disk"));
  comment_label_ = new CommentLabel(
      tr("Please make sure you have backed up "
         "important data, then select the disk  to install"));
  QHBoxLayout* comment_layout = new QHBoxLayout();
  comment_layout->setContentsMargins(0, 0, 0, 0);
  comment_layout->setSpacing(0);
  comment_layout->addWidget(comment_label_);

  os_label_ = new QLabel();
  os_label_->setObjectName("os_label");
  name_label_ = new QLabel();
  name_label_->setObjectName("name_label");
  usage_label_ = new QLabel();
  usage_label_->setObjectName("usage_label");

  QHBoxLayout* name_layout = new QHBoxLayout();
  name_layout->setContentsMargins(0, 0, 0, 0);
  name_layout->setSpacing(0);
  name_layout->addWidget(name_label_);
  name_layout->addStretch();
  name_layout->addWidget(usage_label_);
  QFrame* name_frame = new QFrame();
  name_frame->setObjectName("name_frame");
  name_frame->setContentsMargins(0, 0, 0, 0);
  name_frame->setLayout(name_layout);
  name_frame->setFixedWidth(kProgressBarWidth);

  usage_bar_ = new RoundedProgressBar();
  usage_bar_->setFixedSize(kProgressBarWidth, 8);

  QLabel* separator_label = new QLabel();
  separator_label->setObjectName("separator_label");
  separator_label->setFixedSize(560, 2);

  fs_label_ = new QLabel(tr("Filesystem"));
  fs_label_->setObjectName("fs_label");
  mount_point_label_ = new QLabel(tr("Mount point"));
  mount_point_label_->setObjectName("mount_point_label");
  format_label_ = new QLabel(tr("Format the partition"));
  format_label_->setObjectName("format_label");

  fs_box_ = new TableComboBox();
  fs_box_->setObjectName("fs_box");
  fs_model_ = new FsModel(delegate_, this);
  fs_box_->setModel(fs_model_);

  mount_point_box_ = new TableComboBox();
  mount_point_box_->setObjectName("mount_point_box");
  mount_point_model_ = new MountPointModel(delegate_->allMountPoints(), this);
  mount_point_box_->setModel(mount_point_model_);

  format_check_box_ = new QCheckBox();
  format_check_box_->setObjectName("format_check_box");
  format_check_box_->setFixedWidth(20);

  QGridLayout* grid_layout = new QGridLayout();
  grid_layout->setHorizontalSpacing(20);
  grid_layout->setVerticalSpacing(20);
  grid_layout->setContentsMargins(0, 0, 0, 0);
  grid_layout->addWidget(fs_label_, 0, 0, Qt::AlignRight);
  grid_layout->addWidget(mount_point_label_, 1, 0, Qt::AlignRight);
  grid_layout->addWidget(fs_box_, 0, 1);
  grid_layout->addWidget(mount_point_box_, 1, 1);
  grid_layout->addWidget(format_check_box_, 2, 0, Qt::AlignRight);
  grid_layout->addWidget(format_label_, 2, 1);

  QVBoxLayout* grid_wrapper_layout = new QVBoxLayout();
  grid_wrapper_layout->setContentsMargins(0, 0, 0, 0);
  grid_wrapper_layout->setSpacing(0);
  grid_wrapper_layout->addLayout(grid_layout);
  grid_wrapper_layout->addStretch();

  QFrame* grid_frame = new QFrame();
  grid_frame->setObjectName("grid_frame");
  grid_frame->setContentsMargins(0, 0, 110, 0);
  grid_frame->setFixedSize(400, 300);
  grid_frame->setLayout(grid_wrapper_layout);

  cancel_button_ = new NavButton(tr("Cancel"));
  ok_button_ = new NavButton(tr("OK"));

  QVBoxLayout* layout = new QVBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addStretch();
  layout->addWidget(title_label_, 0, Qt::AlignHCenter);
  layout->addSpacing(kMainLayoutSpacing);
  layout->addLayout(comment_layout);
  layout->addStretch();
  layout->addWidget(os_label_, 0, Qt::AlignHCenter);
  layout->addSpacing(kMainLayoutSpacing);
  layout->addWidget(name_frame, 0, Qt::AlignHCenter);
  layout->addSpacing(kMainLayoutSpacing);
  layout->addWidget(usage_bar_, 0, Qt::AlignHCenter);
  layout->addSpacing(20 + kMainLayoutSpacing);
  layout->addWidget(separator_label, 0, Qt::AlignHCenter);
  layout->addSpacing(20 + kMainLayoutSpacing);
  layout->addWidget(grid_frame, 0, Qt::AlignHCenter);
  layout->addStretch();
  layout->addWidget(cancel_button_, 0, Qt::AlignHCenter);
  layout->addSpacing(30);
  layout->addWidget(ok_button_, 0, Qt::AlignHCenter);

  this->setLayout(layout);
  this->setContentsMargins(0, 0, 0, 0);
  this->setStyleSheet(ReadFile(":/styles/edit_partition_frame.css"));
}

void EditPartitionFrame::onFsChanged(int index) {
  const FsType fs_type = fs_model_->getFs(index);
  const bool visible = IsMountPointSupported(fs_type);

  mount_point_label_->setVisible(visible);
  mount_point_box_->setVisible(visible);

  this->updateFormatBoxState();
}

void EditPartitionFrame::onMountPointChanged(int index) {
  Q_UNUSED(index);
  this->updateFormatBoxState();
}

void EditPartitionFrame::onOkButtonClicked() {
  QString mount_point;
  if (mount_point_box_->isVisible()) {
    // Set mount_point only if mount_point_box_ is visible.
    const int index = mount_point_box_->currentIndex();
    mount_point = mount_point_model_->getMountPoint(index);
  }
  if(format_check_box_->isChecked()) {
    // Create an OperationFormat object.
    const FsType fs_type = fs_model_->getFs(fs_box_->currentIndex());
    delegate_->formatPartition(partition_, fs_type, mount_point);
    delegate_->refreshVisual();
  } else {
    // If FormatOperation is reverted.
    if (partition_.status == PartitionStatus::Format) {
      delegate_->unFormatPartition(partition_);
      // Reset partition status.
      partition_.status = PartitionStatus::Real;
    }

    // Create an OperationMountPoint object.
    if (partition_.mount_point != mount_point) {
      delegate_->updateMountPoint(partition_, mount_point);
    }
    delegate_->refreshVisual();
  }

  emit this->finished();
}

}  // namespace installer