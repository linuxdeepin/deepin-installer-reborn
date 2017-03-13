// Copyright (c) 2016 Deepin Ltd. All rights reserved.
// Use of this source is governed by General Public License that can be found
// in the LICENSE file.

#include "oem/models/oem_settings_model.h"

#include <QDebug>
#include <QDir>

namespace installer {

namespace {

const char kOemJsonFile[] = "oem_settings.json";
const char kSettingsName[] = "settings.ini";
const char kDefaultSettingsFile[] = RESOURCES_DIR "/default_settings.ini";
const char kDefaultOemJsonFile[] = RESOURCES_DIR "/oem_settings.json";

// Returns absolute path to $HOME/oem/ folder.
QString GetOemFolder() {
  return QDir::home().absoluteFilePath("oem");
}

}  // namespace

OemSettingsModel::OemSettingsModel(QObject* parent)
    : QAbstractListModel(parent),
      items_() {
  this->setObjectName("oem_settings_model");
}

QVariant OemSettingsModel::data(const QModelIndex& index, int role) const {
  const int row = index.row();

  if (!index.isValid() || row < 0 || row >= items_.length()) {
    return QVariant();
  }

  if (role == Qt::DisplayRole) {
    return items_.at(row).title();
  } else if (role == Qt::ToolTipRole) {
    return items_.at(row).description();
  } else {
    return QVariant();
  }
}

int OemSettingsModel::rowCount(const QModelIndex& parent) const {
  Q_UNUSED(parent);
  return items_.length();
}

bool OemSettingsModel::dump() {
  const QDir oem_dir(GetOemFolder());
  const QString oem_json_file = oem_dir.absoluteFilePath(kOemJsonFile);
  const QString settings_file = oem_dir.absoluteFilePath(kSettingsName);
  return DumpSettingsItems(items_, oem_json_file, settings_file);
}

OemSettingsItem OemSettingsModel::getItem(const QModelIndex& index) const {
  if (index.isValid()) {
    return items_.at(index.row());
  } else {
    return OemSettingsItem();
  }
}

bool OemSettingsModel::load() {
  const QDir oem_dir(GetOemFolder());
  const QString oem_json_file = oem_dir.absoluteFilePath(kOemJsonFile);
  return LoadSettingsItems(items_,
                           kDefaultOemJsonFile,
                           oem_json_file,
                           kDefaultSettingsFile);
}

}  // namespace installer