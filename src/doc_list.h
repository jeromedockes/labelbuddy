#ifndef LABELBUDDY_DOC_LIST_H
#define LABELBUDDY_DOC_LIST_H

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QShowEvent>
#include <QSqlDatabase>

#include "doc_list_model.h"

/// \file
/// document list in the Dataset tab

namespace labelbuddy {

/// Set of buttons to navigate the doc list or delete documents
class DocListButtons : public QFrame {
  Q_OBJECT

public:
  DocListButtons(QWidget* parent = nullptr);
  void setModel(DocListModel*);

  /// Update the state of the "delete" and "annotate" buttons
  /// \param nSelected number of docs selected in the doc list view
  /// \param nRows number of rows in the doc list view
  /// \param totalNDocs total number of documents in the database
  void updateTopRowButtons(int nSelected, int nRows, int totalNDocs);

signals:

  void docFilterChanged(DocListModel::DocFilter docFilter, int filterLabelId,
                        QString searchPattern, int limit, int offset);

  void selectAll();
  void deleteSelectedRows();
  void deleteAllDocs();

  /// User asked to see selected document in Annotate tab
  void visitDoc();

private slots:

  /// prepare the combobox

  /// the items' `itemData` is a pair (label id, doc filter)
  void fillFilterChoice();

  /// reset filter and offset when database changes
  void afterDatabaseChange();

  void updateButtonStates();

  /// adjust offset and button states when model data changes
  void updateAfterDataChange();

  void goToNextPage();

  void goToLastPage();

  void goToPrevPage();

  void goToFirstPage();

  /// adjust doc filter when combobox selection changes
  void updateFilter();

  void updateSearchPattern();

private:
  int offset_ = 0;
  int pageSize_ = 100;
  DocListModel::DocFilter currentFilter_ = DocListModel::DocFilter::all;
  // label used to either include or exclude docs
  int currentLabelId_ = -1;
  QString searchPattern_{};

  DocListModel* model_ = nullptr;
  QLabel* currentPageLabel_ = nullptr;

  QPushButton* selectAllButton_ = nullptr;
  QPushButton* deleteButton_ = nullptr;
  QPushButton* deleteAllButton_ = nullptr;
  QPushButton* annotateButton_ = nullptr;

  QPushButton* prevPageButton_ = nullptr;
  QPushButton* nextPageButton_ = nullptr;
  QPushButton* firstPageButton_ = nullptr;
  QPushButton* lastPageButton_ = nullptr;

  QComboBox* filterChoice_ = nullptr;
  QLineEdit* searchPatternEdit_ = nullptr;

  void addConnections();
};

/// document list in the Dataset tab
class DocList : public QFrame {
  Q_OBJECT
public:
  DocList(QWidget* parent = nullptr);

  void setModel(DocListModel*);

  int nSelectedDocs() const;

protected:
  void showEvent(QShowEvent* event) override;

  void keyPressEvent(QKeyEvent* event) override;

private slots:

  /// ask for confirmation and tell the model to delete selected docs
  void deleteSelectedRows();
  /// ask for confirmation and tell the model to delete all docs
  void deleteAllDocs();
  /// get the doc's id and emit `visitDocRequested`
  void visitDoc(const QModelIndex& = QModelIndex());
  void updateSelectDeleteButtons();

signals:

  void visitDocRequested(int docId);
  void nSelectedDocsChanged(int nDocs);

private:
  static constexpr int deleteDocsDialogMinDurationMs_ = 2000;
  DocListButtons* buttonsFrame_;
  QListView* docView_;
  DocListModel* model_ = nullptr;
};
} // namespace labelbuddy

#endif // LABELBUDDY_DOC_LIST_H
