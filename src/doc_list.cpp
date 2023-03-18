#include <algorithm>
#include <cassert>

#include <QAbstractItemView>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QObject>
#include <QProgressDialog>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include "doc_list.h"
#include "user_roles.h"
#include "utils.h"

namespace labelbuddy {
DocListButtons::DocListButtons(QWidget* parent) : QFrame(parent) {
  auto mainLayout = new QVBoxLayout();
  setLayout(mainLayout);
  auto buttonsLayout = new QHBoxLayout();
  mainLayout->addLayout(buttonsLayout);
  auto filtersLayout = new QHBoxLayout();
  mainLayout->addLayout(filtersLayout);
  auto navLayout = new QHBoxLayout();
  mainLayout->addLayout(navLayout);

  selectAllButton_ = new QPushButton("Select all");
  buttonsLayout->addWidget(selectAllButton_);
  deleteButton_ = new QPushButton("Delete");
  buttonsLayout->addWidget(deleteButton_);
  deleteAllButton_ = new QPushButton("Delete all docs");
  buttonsLayout->addWidget(deleteAllButton_);
  annotateButton_ = new QPushButton("Annotate selected doc");
  buttonsLayout->addWidget(annotateButton_);

  filtersLayout->addWidget(new QLabel("Filter by label: "));
  filterChoice_ = new QComboBox();
  filtersLayout->addWidget(filterChoice_);
  filtersLayout->addWidget(new QLabel{"Search documents: "});
  searchPatternEdit_ = new QLineEdit{};
  filtersLayout->addWidget(searchPatternEdit_);
  filtersLayout->addStretch();

  firstPageButton_ = new QPushButton(QIcon(":data/icons/go-first.png"), "");
  navLayout->addWidget(firstPageButton_);
  firstPageButton_->setToolTip("First page of results");
  prevPageButton_ = new QPushButton(QIcon(":data/icons/go-previous.png"), "");
  prevPageButton_->setToolTip("Previous page of results");
  navLayout->addWidget(prevPageButton_);
  currentPageLabel_ = new QLabel();
  navLayout->addWidget(currentPageLabel_);
  currentPageLabel_->setAlignment(Qt::AlignHCenter);
  nextPageButton_ = new QPushButton(QIcon(":data/icons/go-next.png"), "");
  nextPageButton_->setToolTip("Next page of results");
  navLayout->addWidget(nextPageButton_);
  lastPageButton_ = new QPushButton(QIcon(":data/icons/go-last.png"), "");
  lastPageButton_->setToolTip("Last page of results");
  navLayout->addWidget(lastPageButton_);

  addConnections();
}

void DocListButtons::addConnections() {
  QObject::connect(nextPageButton_, &QPushButton::clicked, this,
                   &DocListButtons::goToNextPage);

  QObject::connect(prevPageButton_, &QPushButton::clicked, this,
                   &DocListButtons::goToPrevPage);

  QObject::connect(firstPageButton_, &QPushButton::clicked, this,
                   &DocListButtons::goToFirstPage);

  QObject::connect(lastPageButton_, &QPushButton::clicked, this,
                   &DocListButtons::goToLastPage);

  QObject::connect(selectAllButton_, &QPushButton::clicked, this,
                   &DocListButtons::selectAll);

  QObject::connect(deleteButton_, &QPushButton::clicked, this,
                   &DocListButtons::deleteSelectedRows);

  QObject::connect(deleteAllButton_, &QPushButton::clicked, this,
                   &DocListButtons::deleteAllDocs);

  QObject::connect(annotateButton_, &QPushButton::clicked, this,
                   &DocListButtons::visitDoc);

  void (QComboBox::*comboboxActivated)(int) = &QComboBox::activated;
  QObject::connect(filterChoice_, comboboxActivated, this,
                   &DocListButtons::updateFilter);
  // note above could be simplified: QOverload<int>::of(&QComboBox::activated)
  // but QOverload introduced in qt 5.7 and xenial comes with 5.5

  QObject::connect(searchPatternEdit_, &QLineEdit::returnPressed, this,
                   &DocListButtons::updateSearchPattern);
}

void DocListButtons::fillFilterChoice() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  filterChoice_->clear();
  QVariant var{};
  var.setValue(
      QPair<int, int>(-1, static_cast<int>(DocListModel::DocFilter::all)));
  filterChoice_->addItem("All documents", var);
  var.setValue(
      QPair<int, int>(-1, static_cast<int>(DocListModel::DocFilter::labelled)));
  filterChoice_->addItem("Documents with any label", var);
  var.setValue(QPair<int, int>(
      -1, static_cast<int>(DocListModel::DocFilter::unlabelled)));
  filterChoice_->addItem("Documents without labels", var);
  auto labelNames = model_->getLabelNames();
  if (!labelNames.empty()) {
    filterChoice_->insertSeparator(filterChoice_->count());
  }
  for (const auto& labelInfo : labelNames) {
    var.setValue(QPair<int, int>(
        labelInfo.second,
        static_cast<int>(DocListModel::DocFilter::hasGivenLabel)));
    filterChoice_->addItem(labelInfo.first, var);
  }
  if (!labelNames.empty()) {
    filterChoice_->insertSeparator(filterChoice_->count());
  }
  for (const auto& labelInfo : labelNames) {
    var.setValue(QPair<int, int>(
        labelInfo.second,
        static_cast<int>(DocListModel::DocFilter::notHasGivenLabel)));
    filterChoice_->addItem(QString("NOT  %0").arg(labelInfo.first), var);
  }
  int labelIdx{-1};
  switch (currentFilter_) {
  case DocListModel::DocFilter::all:
    filterChoice_->setCurrentIndex(0);
    break;
  case DocListModel::DocFilter::labelled:
    filterChoice_->setCurrentIndex(1);
    break;
  case DocListModel::DocFilter::unlabelled:
    filterChoice_->setCurrentIndex(2);
    break;
  case DocListModel::DocFilter::hasGivenLabel:
  case DocListModel::DocFilter::notHasGivenLabel:
    var.setValue(
        QPair<int, int>(currentLabelId_, static_cast<int>(currentFilter_)));
    labelIdx = filterChoice_->findData(var);
    if (labelIdx != -1) {
      filterChoice_->setCurrentIndex(labelIdx);
    } else {
      filterChoice_->setCurrentIndex(0);
    }
    break;
  default:
    assert(false);
    filterChoice_->setCurrentIndex(0);
    break;
  }
  updateFilter();
}

void DocListButtons::afterDatabaseChange() {
  currentFilter_ = DocListModel::DocFilter::all;
  currentLabelId_ = -1;
  offset_ = 0;
  searchPattern_ = "";
  searchPatternEdit_->clear();
}

void DocListButtons::goToNextPage() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  int total = model_->nDocsCurrentQuery();
  if (offset_ + pageSize_ >= total) {
    return;
  }
  offset_ += pageSize_;
  emit docFilterChanged(currentFilter_, currentLabelId_, searchPattern_,
                        pageSize_, offset_);
}

void DocListButtons::goToPrevPage() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  if (offset_ == 0) {
    return;
  }
  offset_ = std::max(0, offset_ - pageSize_);
  emit docFilterChanged(currentFilter_, currentLabelId_, searchPattern_,
                        pageSize_, offset_);
}

void DocListButtons::goToLastPage() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  int total = model_->nDocsCurrentQuery();
  offset_ = std::max(0, total - 1);
  offset_ -= offset_ % pageSize_;
  emit docFilterChanged(currentFilter_, currentLabelId_, searchPattern_,
                        pageSize_, offset_);
}

void DocListButtons::goToFirstPage() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  offset_ = 0;
  emit docFilterChanged(currentFilter_, currentLabelId_, searchPattern_,
                        pageSize_, offset_);
}

void DocListButtons::updateAfterDataChange() {
  int total = model_->nDocsCurrentQuery();
  int prevOffset = offset_;
  offset_ = std::max(0, std::min(total - 1, offset_));
  offset_ = offset_ - offset_ % pageSize_;
  if (prevOffset != offset_) {
    emit docFilterChanged(currentFilter_, currentLabelId_, searchPattern_,
                          pageSize_, offset_);
  }
  updateButtonStates();
  assert(offset_ >= 0);
  assert(!(offset_ % pageSize_));
  assert(total == 0 || offset_ < total);
}

void DocListButtons::updateButtonStates() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }

  int total = model_->nDocsCurrentQuery();
  int end = offset_ + model_->rowCount();
  assert(total == 0 || offset_ < total);
  assert(end <= total);

  if (total == 0) {
    currentPageLabel_->setText("0 / 0");
  } else if (offset_ == end - 1) {
    currentPageLabel_->setText(QString("%1 / %2").arg(end).arg(total));
  } else {
    currentPageLabel_->setText(
        QString("%1 - %2 / %3").arg(offset_ + 1).arg(end).arg(total));
  }

  if (offset_ == 0) {
    prevPageButton_->setDisabled(true);
    firstPageButton_->setDisabled(true);
  } else {
    prevPageButton_->setEnabled(true);
    firstPageButton_->setEnabled(true);
  }

  if (end == total) {
    nextPageButton_->setDisabled(true);
    lastPageButton_->setDisabled(true);
  } else {
    nextPageButton_->setEnabled(true);
    lastPageButton_->setEnabled(true);
  }
}

void DocListButtons::updateTopRowButtons(int nSelected, int nRows,
                                         int totalNDocs) {
  selectAllButton_->setEnabled(nRows > 0);
  deleteButton_->setEnabled(nSelected > 0);
  deleteAllButton_->setEnabled(totalNDocs > 0);
  annotateButton_->setEnabled(nSelected == 1);
}

void DocListButtons::updateFilter() {
  auto prevFilter = currentFilter_;
  auto prevLabelId = currentLabelId_;
  auto data = filterChoice_->currentData().value<QPair<int, int>>();
  currentLabelId_ = data.first;
  currentFilter_ = static_cast<DocListModel::DocFilter>(data.second);
  if ((currentLabelId_ != prevLabelId) || (currentFilter_ != prevFilter)) {
    offset_ = 0;
    emit docFilterChanged(currentFilter_, currentLabelId_, searchPattern_,
                          pageSize_, offset_);
  }
}

void DocListButtons::updateSearchPattern() {
  auto prevPattern = searchPattern_;
  searchPattern_ = searchPatternEdit_->text();
  if (searchPattern_ != prevPattern) {
    offset_ = 0;
    emit docFilterChanged(currentFilter_, currentLabelId_, searchPattern_,
                          pageSize_, offset_);
  }
}

void DocListButtons::setModel(DocListModel* newModel) {
  assert(newModel != nullptr);
  model_ = newModel;
  QObject::connect(this, &DocListButtons::docFilterChanged, model_,
                   &DocListModel::adjustQuery);
  QObject::connect(model_, &DocListModel::modelReset, this,
                   &DocListButtons::updateAfterDataChange);
  QObject::connect(model_, &DocListModel::databaseChanged, this,
                   &DocListButtons::afterDatabaseChange);
  QObject::connect(model_, &DocListModel::labelsChanged, this,
                   &DocListButtons::fillFilterChoice);
  fillFilterChoice();
  updateButtonStates();
}

DocList::DocList(QWidget* parent) : QFrame(parent) {

  auto layout = new QVBoxLayout();
  setLayout(layout);

  buttonsFrame_ = new DocListButtons();
  layout->addWidget(buttonsFrame_);

  docView_ = new QListView();
  layout->addWidget(docView_);
  docView_->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QObject::connect(buttonsFrame_, &DocListButtons::selectAll, docView_,
                   &QListView::selectAll);

  QObject::connect(buttonsFrame_, &DocListButtons::deleteSelectedRows, this,
                   &DocList::deleteSelectedRows);

  QObject::connect(buttonsFrame_, &DocListButtons::deleteAllDocs, this,
                   &DocList::deleteAllDocs);

  QObject::connect(buttonsFrame_, &DocListButtons::visitDoc, this,
                   [=]() { this->visitDoc(); });

  QObject::connect(docView_, &QListView::doubleClicked, this,
                   &DocList::visitDoc);
}

void DocList::setModel(DocListModel* newModel) {
  assert(newModel != nullptr);
  buttonsFrame_->setModel(newModel);
  docView_->setModel(newModel);
  model_ = newModel;
  QObject::connect(docView_->selectionModel(),
                   &QItemSelectionModel::selectionChanged, this,
                   &DocList::updateSelectDeleteButtons);
  QObject::connect(model_, &DocListModel::modelReset, this,
                   &DocList::updateSelectDeleteButtons);
  updateSelectDeleteButtons();
}

int DocList::nSelectedDocs() const {
  return docView_->selectionModel()->selectedIndexes().size();
}

void DocList::showEvent(QShowEvent* event) {
  if (model_ != nullptr) {
    model_->refreshCurrentQueryIfOutdated();
  }
  QFrame::showEvent(event);
}

void DocList::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::InsertParagraphSeparator)) {
    visitDoc();
    return;
  }
}

void DocList::deleteAllDocs() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  int resp = QMessageBox::question(this, "labelbuddy",
                                   QString("Really delete ALL documents?"),
                                   QMessageBox::Ok | QMessageBox::Cancel);
  if (resp != QMessageBox::Ok) {
    return;
  }
  int nDeleted{};
  {
    QProgressDialog progress("Deleting documents...", "Stop", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(deleteDocsDialogMinDurationMs_);
    nDeleted = model_->deleteAllDocs(&progress);
  }
  docView_->reset();
  QMessageBox::information(this, "labelbuddy",
                           QString("Deleted %0 document%1")
                               .arg(nDeleted)
                               .arg(nDeleted > 1 ? "s" : ""),
                           QMessageBox::Ok);
}

void DocList::deleteSelectedRows() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  QModelIndexList selected = docView_->selectionModel()->selectedIndexes();
  if (selected.length() == 0) {
    return;
  }
  int resp = QMessageBox::question(this, "labelbuddy",
                                   QString("Really delete selected documents?"),
                                   QMessageBox::Ok | QMessageBox::Cancel);
  if (resp != QMessageBox::Ok) {
    return;
  }
  int nBefore = model_->totalNDocs(DocListModel::DocFilter::all);
  model_->deleteDocs(selected);
  int nAfter = model_->totalNDocs(DocListModel::DocFilter::all);
  int nDeleted = nBefore - nAfter;
  QMessageBox::information(this, "labelbuddy",
                           QString("Deleted %0 document%1")
                               .arg(nDeleted)
                               .arg(nDeleted > 1 ? "s" : ""),
                           QMessageBox::Ok);
  docView_->reset();
}

void DocList::visitDoc(const QModelIndex& index) {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  if (index.isValid() && index.column() == 0) {
    auto docId = model_->data(index, Roles::RowIdRole).toInt();
    emit visitDocRequested(docId);
    return;
  }
  auto selected = docView_->selectionModel()->selectedIndexes();
  // items outside of first (visible) column shouldn't be selectable
  assert(selected.empty() || selected[0].column() == 0);
  if (selected.size() != 1 || selected[0].column() != 0) {
    return;
  }
  auto docId = model_->data(selected[0], Roles::RowIdRole).toInt();
  assert(docId != -1);
  emit visitDocRequested(docId);
}

void DocList::updateSelectDeleteButtons() {
  if (model_ == nullptr) {
    assert(false);
    return;
  }
  auto selected = docView_->selectionModel()->selectedIndexes();
  int nRows{};
  for (const auto& sel : selected) {
    if (sel.column() == 0) {
      ++nRows;
    } else {
      assert(false);
    }
  }
  buttonsFrame_->updateTopRowButtons(nRows, model_->rowCount(),
                                     model_->totalNDocs());
  emit nSelectedDocsChanged(nRows);
}
} // namespace labelbuddy
