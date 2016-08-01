// Copyright (c) 2016 Deepin Ltd. All rights reserved.
// Use of this source is governed by General Public License that can be found
// in the LICENSE file.

#include "ui/frames/select_language_frame.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPalette>
#include <QPixmap>
#include <QScrollArea>
#include <QVBoxLayout>

#include "base/file_util.h"
#include "service/settings_manager.h"
#include "service/settings_name.h"
#include "ui/frames/consts.h"
#include "ui/frames/delegates/language_list_delegate.h"
#include "ui/frames/models/language_list_model.h"
#include "ui/widgets/comment_label.h"
#include "ui/widgets/frosted_frame.h"
#include "ui/widgets/nav_button.h"

namespace ui {

SelectLanguageFrame::SelectLanguageFrame(QWidget* parent) : QFrame(parent) {
  this->setObjectName(QStringLiteral("select_language_frame"));

  this->initUI();
  this->initConnections();
}

void SelectLanguageFrame::initConnections() {
  connect(next_button_, &QPushButton::clicked,
          this, &SelectLanguageFrame::onNextButtonClicked);
  connect(list_view_, &QListView::clicked,
          this, &SelectLanguageFrame::onLanguageListSelected);
}

void SelectLanguageFrame::initUI() {
  QLabel* logo_label = new QLabel();
  logo_label->setPixmap(QPixmap(service::GetVendorLogo()));
  QHBoxLayout* logo_layout = new QHBoxLayout();
  logo_layout->setAlignment(Qt::AlignCenter);
  logo_layout->addWidget(logo_label);

  CommentLabel* comment_label =
      new CommentLabel("Please select the language you want to use\n"
                       "请选择您的语言");
  QHBoxLayout* comment_layout = new QHBoxLayout();
  comment_layout->addWidget(comment_label);

  list_view_ = new QListView();
  list_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  QPalette list_palette = list_view_->palette();
  list_palette.setBrush(QPalette::Base, Qt::transparent);
  list_view_->setPalette(list_palette);
  list_view_->setSelectionMode(QAbstractItemView::SingleSelection);
  list_view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  list_view_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  list_model_ = new LanguageListModel(this);
  list_view_->setModel(list_model_);
  list_view_->setFixedSize(400, 500);
//  LanguageListDelegate* list_delegate = new LanguageListDelegate(this);
//  list_view_->setItemDelegate(list_delegate);
  list_view_->setStyleSheet(
      base::ReadTextFileContent(":/styles/language_list.css"));

  QHBoxLayout* list_layout = new QHBoxLayout();
  list_layout->setSpacing(0);
  list_layout->setMargin(0);
  list_layout->setAlignment(Qt::AlignCenter);
  list_layout->addStretch();
  list_layout->addWidget(list_view_);
  list_layout->addStretch();

  next_button_ = new NavButton(tr("Next"));
  QHBoxLayout* next_layout = new QHBoxLayout();
  next_layout->addWidget(next_button_);

  QVBoxLayout* layout = new QVBoxLayout();
  layout->setSpacing(kMainLayoutSpacing);
  layout->addStretch();
  layout->addLayout(logo_layout);
  layout->addLayout(comment_layout);
  layout->addStretch();
  layout->addLayout(list_layout);
  layout->addStretch();
  layout->addLayout(next_layout);

  this->setLayout(layout);
}

void SelectLanguageFrame::onLanguageListSelected() {
  language_is_selected_ = true;
}

void SelectLanguageFrame::onNextButtonClicked() {
  if (language_is_selected_) {
    // TODO(xushaohua): Set language
    const QModelIndexList selected_items =
        list_view_->selectionModel()->selectedIndexes();
    if (selected_items.length() == 1) {
      const QString locale = list_model_->locale(selected_items.at(0));
      service::WriteLocale(locale);

      emit this->finished();
    } else {
      qWarning() << "No locale is selected!";
    }
  }
}

}  // namespace ui