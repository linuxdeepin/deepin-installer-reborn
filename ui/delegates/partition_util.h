// Copyright (c) 2016 Deepin Ltd. All rights reserved.
// Use of this source is governed by General Public License that can be found
// in the LICENSE file.

#ifndef INSTALLER_UI_DELEGATES_PARTITION_UTIL_H
#define INSTALLER_UI_DELEGATES_PARTITION_UTIL_H

#include <QtCore/QtGlobal>

#include "partman/fs.h"
#include "partman/partition.h"

namespace installer {

// Get partition name based on |path|.
QString GetPartitionName(const QString& path);

QString GetPartitionLabelAndPath(const Partition& partition);

// Get human readable partition usage.
QString GetPartitionUsage(const Partition& partition);

// Get partition usage percentage (0-100).
int GetPartitionUsageValue(const Partition& partition);

// Get icon path of os type
QString GetOsTypeIcon(OsType os_type);
QString GetOsTypeLargeIcon(OsType os_type);

// Returns human readable file system name.
QString GetLocalFsTypeName(FsType fs_type);

// Check whether specific fs type can be mounted by user.
// linux-swap and efi are mounted at fixed position and thus returns false.1
bool SupportMountPoint(FsType fs_type);

// Convert |size| in byte to gibibyte.
int ToGigByte(qint64 size);

}  // namespace installer

#endif  // INSTALLER_UI_DELEGATES_PARTITION_UTIL_H
