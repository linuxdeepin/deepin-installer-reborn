// Copyright (c) 2016 Deepin Ltd. All rights reserved.
// Use of this source is governed by General Public License that can be found
// in the LICENSE file.

#ifndef INSTALLER_PARTMAN_PARTITION_MANAGER_H
#define INSTALLER_PARTMAN_PARTITION_MANAGER_H

#include <QList>
#include <QObject>

#include "partman/device.h"
#include "partman/operation.h"

namespace installer {

class PartitionManager : public QObject {
  Q_OBJECT

 public:
  explicit PartitionManager(QObject* parent = nullptr);
  ~PartitionManager();

 signals:
  // Notify PartitionManager to scan devices.
  // If |umount| is true, umount partitions before scanning.
  void refreshDevices(bool umount);
  void devicesRefreshed(const DeviceList& devices);

  // Run auto part script at |script_path|.
  void autoPart(const QString& script_path);
  // Emitted after auto_part.sh script is executed and exited.
  // |ok| is true if that script exited 0.
  void autoPartDone(bool ok);

  void manualPart(const OperationList& operations);
  void manualPartDone(bool ok);

 private:
  void initConnections();

 private slots:
  void doRefreshDevices(bool umount);
  void doAutoPart(const QString& script_path);
  void doManualPart(const OperationList& operations);
};

// Scan all disk devices on this machine.
// Do not call this function directly, use PartitionManager instead.
DeviceList ScanDevices();

}  // namespace installer

#endif  // INSTALLER_PARTMAN_PARTITION_MANAGER_H