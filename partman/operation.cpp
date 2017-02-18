// Copyright (c) 2016 Deepin Ltd. All rights reserved.
// Use of this source is governed by General Public License that can be found
// in the LICENSE file.

#include "partman/operation.h"

#include <QDebug>

#include "partman/libparted_util.h"
#include "partman/partition_format.h"

namespace installer {

QDebug& operator<<(QDebug& debug, const OperationType& op_type) {
  QString type;
  switch (op_type) {
    case OperationType::Create: {
      type = "Create";
      break;
    }
    case OperationType::Delete: {
      type = "Delete";
      break;
    }
    case OperationType::Format: {
      type = "Format";
      break;
    }
    case OperationType::MountPoint: {
      type = "MountPoint";
      break;
    }
    case OperationType::NewPartTable: {
      type = "NewPartTable";
      break;
    }
    case OperationType::Resize: {
      type = "Resize";
      break;
    }
    case OperationType::Invalid: {
      type = "Invalid";
      break;
    }
  }
  debug << type;
  return debug;
}

Operation::Operation(OperationType type,
                     const Partition& orig_partition,
                     const Partition& new_partition)
    : type(type),
      orig_partition(orig_partition),
      new_partition(new_partition) {
}

Operation::~Operation() {
}

bool Operation::applyToDisk() const {
  bool ok;
  switch (type) {
    case OperationType::Create: {
      // TODO(xushaohua): Update partition number.
      ok = CreatePartition(new_partition);
      qDebug() << "applyToDisk() create partition:" << ok;
      // Ignores extended partition.
      // TODO(xushaohua): Check new_partition.fs
      if (ok && new_partition.type != PartitionType::Extended) {
        ok = Mkfs(new_partition);
        qDebug() << "applyToDisk() mkfs:" << ok;
      }
      break;
    }

    case OperationType::Delete: {
      ok = DeletePartition(orig_partition);
      qDebug() << "applyToDisk() delete partition" << ok;
      break;
    }

    case OperationType::Format: {
      ok = SetPartitionType(new_partition);
      qDebug() << "applyToDisk() format set partition type:" << ok;
      if (ok) {
        ok = Mkfs(new_partition);
        qDebug() << "applyToDisk() format mkfs():" << ok;
      }
      break;
    }

    case OperationType::Resize: {
      ok = ResizeMovePartition(new_partition);
      qDebug() << "applyToDisk() resize partition:" << ok;
      break;
    }

    default: {
      ok = false;
      break;
    }
  }

  // Set flags of new_partition.
  if (ok) {
    ok = SetPartitionFlags(new_partition);
    qDebug() << "Set partition flags:" << new_partition.path << ok;
  }

  return ok;
}

void Operation::applyToVisual(PartitionList& partitions) const {
  switch (type) {
    case OperationType::Create: {
      this->applyCreateVisual(partitions);
      break;
    }
    case OperationType::Delete: {
      this->applyDeleteVisual(partitions);
      break;
    }
    case OperationType::Format: {
      this->substitute(partitions);
      break;
    }
    case OperationType::MountPoint: {
      this->substitute(partitions);
      break;
    }
    case OperationType::Resize: {
      this->applyResizeVisual(partitions);
      break;
    }
    default: {
      break;
    }
  }
}

QString Operation::description() const {
  QString desc;
  switch (type) {
    case OperationType::Create: {
      if (new_partition.type == PartitionType::Extended) {
        desc = QObject::tr("Create extended partition %1")
            .arg(new_partition.path);
      }
      else if (new_partition.mount_point.isEmpty()) {
        desc = QObject::tr("Create partition %1 with %2")
            .arg(new_partition.path)
            .arg(GetFsTypeName(new_partition.fs));
      } else {
        desc = QObject::tr("Create partition %1 for %2 with %3")
            .arg(new_partition.path)
            .arg(new_partition.mount_point)
            .arg(GetFsTypeName(new_partition.fs));
      }
      break;
    }
    case OperationType::Delete: {
      desc = QObject::tr("Delete partition %1").arg(orig_partition.path);
      break;
    }
    case OperationType::Format: {
      if (new_partition.mount_point.isEmpty()) {
        desc = QObject::tr("Format %1 with %2")
            .arg(new_partition.path)
            .arg(GetFsTypeName(new_partition.fs));
      } else {
        desc = QObject::tr("Format %1 for %2 with %3")
            .arg(new_partition.path)
            .arg(new_partition.mount_point)
            .arg( GetFsTypeName(new_partition.fs));
      }
      break;
    }
    case OperationType::MountPoint: {
      desc = QObject::tr("Use %1 for %2")
          .arg(new_partition.path)
          .arg(new_partition.mount_point);
      break;
    }
    case OperationType::Resize: {
      desc = QObject::tr("Resize partition %1").arg(new_partition.path);
      break;
    }
    default: {
      // pass
      break;
    }
  }
  return desc;
}

void Operation::applyCreateVisual(PartitionList& partitions) const {
  // Policy:
  // * Create unallocated partition if orig_partition is larger than
  //   new_partition

  qDebug() << "applyCreateVisual():" << partitions
           << "orig partition:" << orig_partition
           << ", new_partition:" << new_partition;
  int index = PartitionIndex(partitions, orig_partition);
  // FIXME(xushaohua): index == -1
  if (index == -1) {
    qCritical() << "applyCreateVisual() Failed to find partition:"
                << orig_partition.path;
    return;
  }

  // Do not remove orig partition when creating extended partition.
  if (new_partition.type == PartitionType::Extended) {
    partitions.insert(index, new_partition);
    // Extended partition does not consume any real sectors, so no need
    // to insert unallocated partitions. Just leave orig_partition unchanged.
    return;
  } else {
    partitions[index] = new_partition;
  }

  Partition unallocated;
  unallocated.device_path = orig_partition.device_path;
  unallocated.sector_size = orig_partition.sector_size;
  unallocated.type = PartitionType::Unallocated;
  const qint64 twoMebiByteSector = 2 * kMebiByte / orig_partition.sector_size;

  // Gap between orig_partition.start <-> new_partition.start.
  if (new_partition.start_sector - orig_partition.start_sector >
      twoMebiByteSector) {
    unallocated.start_sector = orig_partition.start_sector + 1;
    unallocated.end_sector = new_partition.start_sector - 1;
    partitions.insert(index, unallocated);
    index += 1;
  }

  // Gap between new_partition.end <-> orig_partition.end
  if (orig_partition.end_sector - new_partition.end_sector >
      twoMebiByteSector) {
    unallocated.start_sector = new_partition.end_sector + 1;
    unallocated.end_sector = orig_partition.end_sector - 1;
    if (index + 1 == partitions.length()) {
      partitions.append(unallocated);
    } else {
      partitions.insert(index + 1, unallocated);
    }
  }

  MergeUnallocatedPartitions(partitions);
}

void Operation::applyDeleteVisual(PartitionList& partitions) const {
  this->substitute(partitions);
  MergeUnallocatedPartitions(partitions);
}

void Operation::applyResizeVisual(PartitionList& partitions) const {
  // Currently only extended partition is allowed to resize
  if (new_partition.type == PartitionType::Extended) {
    this->substitute(partitions);
  }
}

void Operation::substitute(PartitionList& partitions) const {
  const int index = PartitionIndex(partitions, orig_partition);
  if (index == -1) {
    qCritical() << "substitute() Failed to find partition:"
                << orig_partition.path;
  } else {
    partitions[index] = new_partition;
  }
}

QDebug& operator<<(QDebug& debug, const Operation& operation) {
  debug << "Operation: {"
        << "type:" << operation.type
        << "orig_partition:" << operation.orig_partition
        << "new_partition:" << operation.new_partition
        << "}";
  return debug;
}

void MergeOperations(OperationList& operations, const Operation& operation) {
  Q_UNUSED(operations);
  Q_UNUSED(operation);
}

void MergeUnallocatedPartitions(PartitionList& partitions) {
  // Do not handles empty partition list.
  if (partitions.isEmpty()) {
    return;
  }

  // Looks for gaps in between.
  int global_index = 0;
  while (global_index < partitions.length()) {
    int index;
    // Find unallocated partition.
    for (index = global_index; index < partitions.length(); ++ index) {
      if (partitions.at(index).type == PartitionType::Unallocated) {
        break;
      }
    }

    // No more unallocated partitions.
    if (index >= partitions.length()) {
      break;
    }

    global_index = index;
    // Find all connected unallocated partitions
    while ((index + 1) < partitions.length()) {
      const Partition& next_part = partitions.at(index + 1);
      if (next_part.type == PartitionType::Unallocated) {
        partitions[global_index].end_sector = next_part.end_sector;
        partitions.removeAt(index + 1);
      } else if (next_part.type == PartitionType::Extended) {
        // Ignores extended partition
        ++ index;
      } else {
        // Breaks if next_part is logical partition or normal partition.
        break;
      }
    }

    ++ global_index;
  }
}

}  // namespace installer