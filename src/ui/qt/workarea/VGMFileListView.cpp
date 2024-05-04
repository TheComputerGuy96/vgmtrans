/*
 * VGMTrans (c) 2002-2019
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */

#include <QHeaderView>

#include "VGMFileListView.h"
#include "Helpers.h"
#include "QtVGMRoot.h"
#include "MdiArea.h"
#include "services/commands/GeneralCommands.h"
#include "services/NotificationCenter.h"
#include "VGMFile.h"
#include "VGMInstrSet.h"
#include "VGMSeq.h"
#include "VGMMiscFile.h"
#include "VGMSampColl.h"

/*
 *  VGMFileListModel
 */

VGMFileListModel::VGMFileListModel(QObject *parent) : QAbstractTableModel(parent) {
  connect(&qtVGMRoot, &QtVGMRoot::UI_AddedVGMFile, this, &VGMFileListModel::AddVGMFile);
}

QVariant VGMFileListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  VGMFileVariant vgmfilevariant = qtVGMRoot.vVGMFile.at(index.row());
  VGMFile* vgmfile = variantToVGMFile(vgmfilevariant);

  switch (index.column()) {
    case Property::Name: {
      if (role == Qt::DisplayRole) {
        return QString::fromStdString(*vgmfile->GetName());
      } else if (role == Qt::DecorationRole) {
        return iconForFile(vgmfilevariant);
      }
      break;
    }

    case Property::Format: {
      if (role == Qt::DisplayRole) {
        return QString::fromStdString(vgmfile->GetFormatName());
      }
      break;
    }
  }

  return {};
}

QVariant VGMFileListModel::headerData(int column, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Vertical || role != Qt::DisplayRole) {
    return {};
  }

  switch (column) {
    case Property::Name: {
      return "Name";
    }

    case Property::Format: {
      return "Format";
    }

    default: {
      return {};
    }
  }
}

int VGMFileListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }

  return static_cast<int>(qtVGMRoot.vVGMFile.size());
}

int VGMFileListModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }

  return 2;
}

void VGMFileListModel::AddVGMFile() {
  int position = static_cast<int>(qtVGMRoot.vVGMFile.size()) - 1;
  beginInsertRows(QModelIndex(), position, position);
  endInsertRows();
}

void VGMFileListModel::RemoveVGMFile() {
  int position = static_cast<int>(qtVGMRoot.vVGMFile.size()) - 1;
  if (position < 0) {
    return;
  }

  beginRemoveRows(QModelIndex(), position, position);
  endRemoveRows();
}

/*
 *  VGMFileListView
 */

VGMFileListView::VGMFileListView(QWidget *parent) : TableView(parent) {
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setSelectionBehavior(QAbstractItemView::SelectRows);

  view_model = new VGMFileListModel();
  setModel(view_model);

  connect(&qtVGMRoot, &QtVGMRoot::UI_RemoveVGMFile, this, &VGMFileListView::removeVGMFile);
  connect(this, &QAbstractItemView::customContextMenuRequested, this, &VGMFileListView::itemMenu);
  connect(this, &QAbstractItemView::doubleClicked, this, &VGMFileListView::requestVGMFileView);
  connect(NotificationCenter::the(), &NotificationCenter::vgmFileSelected, this, &VGMFileListView::onVGMFileSelected);
}

void VGMFileListView::itemMenu(const QPoint &pos) {

  auto selectedFiles = std::make_shared<std::vector<VGMFile*>>();

  if (!selectionModel()->hasSelection()) {
    return;
  }

  QModelIndexList list = selectionModel()->selectedRows();

  selectedFiles->reserve(list.size());
  for (const auto &index : list) {
    if (index.isValid()) {
      selectedFiles->push_back(variantToVGMFile(qtVGMRoot.vVGMFile[index.row()]));
    }
  }
  auto menu = menuManager.CreateMenuForItems<VGMItem>(selectedFiles);
  menu->exec(mapToGlobal(pos));
  menu->deleteLater();
}

void VGMFileListView::keyPressEvent(QKeyEvent *input) {
  switch (input->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
      if (currentIndex().isValid())
        requestVGMFileView(currentIndex());
      break;
    case Qt::Key_Delete:
    case Qt::Key_Backspace: {
      if (!selectionModel()->hasSelection())
        return;

      QModelIndexList list = selectionModel()->selectedRows();
      pRoot->UI_BeginRemoveVGMFiles();
      for (auto it = list.rbegin(); it != list.rend(); ++it) {
        qtVGMRoot.RemoveVGMFile(qtVGMRoot.vVGMFile[it->row()]);
      }
      pRoot->UI_EndRemoveVGMFiles();

      this->clearSelection();
      return;
    }

    // Pass the event back to the base class, needed for keyboard navigation
    default:
      QTableView::keyPressEvent(input);
  }
}

void VGMFileListView::removeVGMFile(VGMFile *file) {
  MdiArea::the()->removeView(file);
  view_model->RemoveVGMFile();
}

void VGMFileListView::requestVGMFileView(QModelIndex index) {
  MdiArea::the()->newView(variantToVGMFile(qtVGMRoot.vVGMFile[index.row()]));
}

// Update the status bar for the current selection
void VGMFileListView::updateStatusBar() {
  if (!currentIndex().isValid()) {
    NotificationCenter::the()->updateStatusForItem(nullptr);
    return;
  }
  VGMFile* file = variantToVGMFile(qtVGMRoot.vVGMFile[currentIndex().row()]);
  NotificationCenter::the()->updateStatusForItem(file);
}

// Update the status bar on focus
void VGMFileListView::focusInEvent(QFocusEvent *event) {
  TableView::focusInEvent(event);
  updateStatusBar();
}

void VGMFileListView::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
  TableView::currentChanged(current, previous);

  if (!current.isValid()) {
    NotificationCenter::the()->selectVGMFile(nullptr, this);
    return;
  }

  VGMFile *file = variantToVGMFile(qtVGMRoot.vVGMFile[current.row()]);
  NotificationCenter::the()->selectVGMFile(file, this);

  if (this->hasFocus())
    updateStatusBar();
}

void VGMFileListView::onVGMFileSelected(VGMFile* file, QWidget* caller) {
  if (caller == this)
    return;

  if (file == nullptr) {
    this->clearSelection();
    return;
  }

  auto it = std::find_if(qtVGMRoot.vVGMFile.begin(), qtVGMRoot.vVGMFile.end(), [&](const VGMFileVariant& var) {
      // Use std::visit to access the actual object within the variant
      bool matches = false;

      std::visit([&](auto&& arg) {
        matches = (file == arg);
      }, var);

      return matches;
    });
  if (it == qtVGMRoot.vVGMFile.end())
    return;
  int row = static_cast<int>(std::distance(qtVGMRoot.vVGMFile.begin(), it));

  // Select the row corresponding to the file
  QModelIndex firstIndex = model()->index(row, 0); // First column of the row
  QModelIndex lastIndex = model()->index(row, model()->columnCount() - 1); // Last column of the row

  if (firstIndex == currentIndex())
    return;

  QItemSelection selection(firstIndex, lastIndex);
  selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  setCurrentIndex(firstIndex);

  scrollTo(firstIndex, QAbstractItemView::EnsureVisible);
}