#ifndef LABELBUDDY_ANNOTATOR_H
#define LABELBUDDY_ANNOTATOR_H

#include <list>
#include <memory>
#include <set>

#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QSplitter>
#include <QSqlQueryModel>
#include <QWidget>

#include "annotations_model.h"
#include "label_list.h"
#include "label_list_model.h"
#include "no_deselect_all_view.h"
#include "searchable_text.h"

/// \file
/// Implementation of the Annotator tab

namespace labelbuddy {

/// Widget use to set the label for the selection
class LabelChoices : public QWidget {
  Q_OBJECT

public:
  LabelChoices(QWidget* parent = nullptr);
  void setModel(LabelListModel*);
  QModelIndexList selectedIndexes() const;
  /// The currently selected label's `id` in the database
  int selectedLabelId() const;
  /// Set the currently selected label based on its `id` in the db
  void setSelectedLabelId(int labelId);
  /// Set the extra data in the line edit
  void setExtraData(const QString& newData);
  bool isLabelChoiceEnabled() const;

signals:
  void selectionChanged();
  void deleteButtonClicked();
  void extraDataEdited(const QString& newData);
  void extraDataEditFinished();

public slots:
  void enableDeleteAndEdit();
  void disableDeleteAndEdit();
  void enableLabelChoice();
  void disableLabelChoice();

private slots:
  void onSelectionChange();
  void onDeleteButtonClick();

private:
  QLabel* instructionLabel_ = nullptr;
  QPushButton* deleteButton_ = nullptr;
  NoDeselectAllView* labelsView_ = nullptr;
  LabelListModel* labelListModel_ = nullptr;
  std::unique_ptr<LabelDelegate> labelDelegate_ = nullptr;
  QLineEdit* extraDataEdit_ = nullptr;
  QLabel* extraDataLabel_ = nullptr;
};

/// Navigation buttons above the text: next, next labelled etc.
class AnnotationsNavButtons : public QWidget {
  Q_OBJECT

public:
  AnnotationsNavButtons(QWidget* parent = nullptr);
  void setModel(AnnotationsModel*);

public slots:
  void updateButtonStates();
  void setSkipUpdating(bool skip);

private:
  // if updating the button states once takes longer than this (*huge*
  // database), do not update them next time.
  static constexpr int skipUpdateButtonsDurationThresholdMs_{500};
  AnnotationsModel* annotationsModel_ = nullptr;
  QPushButton* prevLabelledButton_;
  QPushButton* prevUnlabelledButton_;
  QPushButton* prevButton_;

  QLabel* currentDocLabel_;

  QPushButton* nextButton_;
  QPushButton* nextUnlabelledButton_;
  QPushButton* nextLabelledButton_;

  bool skipUpdatingButtons_{};

signals:
  void visitNext();
  void visitPrev();
  void visitNextLabelled();
  void visitPrevLabelled();
  void visitNextUnlabelled();
  void visitPrevUnlabelled();
};

struct AnnotationCursor {
  int id;
  int labelId;
  int startChar;
  int endChar;
  QString extraData;
  QTextCursor cursor;
};

struct AnnotationIndex {
  int startChar;
  int id;
};

bool operator<(const AnnotationIndex& lhs, const AnnotationIndex& rhs);
bool operator>(const AnnotationIndex& lhs, const AnnotationIndex& rhs);
bool operator<=(const AnnotationIndex& lhs, const AnnotationIndex& rhs);
bool operator>=(const AnnotationIndex& lhs, const AnnotationIndex& rhs);
bool operator==(const AnnotationIndex& lhs, const AnnotationIndex& rhs);
bool operator!=(const AnnotationIndex& lhs, const AnnotationIndex& rhs);

struct Cluster {
  AnnotationIndex firstAnnotation;
  AnnotationIndex lastAnnotation;
  int startChar;
  int endChar;
};

struct StatusBarInfo {
  QString docInfo;
  QString annotationInfo;
  QString annotationLabel;
};

/// The widget that allows creating and manipulating annotations

/// There can be several annotations for the document, highlighted with their
/// label's color. Moreover, one of them can be "active"; it is the one whose
/// label is changed if we use the label list, or that can be deleted, and its
/// text is underlined and shown in bold font (by default).
///
/// Groups of overlapping annotations form "clusters", that are shown in gray
/// with white text except possibly for the active annotation.
class Annotator : public QSplitter {
  Q_OBJECT

public:
  Annotator(QWidget* parent = nullptr);

  void setAnnotationsModel(AnnotationsModel*);
  void setLabelListModel(LabelListModel*);

  /// If there is a currently active (selected) annotation return its label's
  /// `id`, otherwise return -1 .
  int activeAnnotationLabel() const;

  /// If there is a currently active annotation return its extra data
  /// "" if there is no active annotation or its data is NULL
  QString activeAnnotationExtraData() const;

  StatusBarInfo currentStatusInfo() const;

signals:

  void activeAnnotationChanged();
  void currentStatusDisplayChanged();

public slots:

  /// Update the list of annotations e.g. after changing the document or changes
  /// to the labels in the database.
  void updateAnnotations();

  /// Refresh the states of navigation buttons eg if documents or annotations
  /// have been added or removed
  void updateNavButtons();

  /// jump to next annotation and make it active
  void selectNextAnnotation(bool forward = true);

  /// Remember window state (slider position) in QSettings
  void storeState();
  void setFont(const QFont& newFont);
  void setUseBoldFont(bool useBold);

  void resetSkipUpdatingNavButtons();

protected:
  /// Filter installed on the textedit to override the behaviour of Space key
  bool eventFilter(QObject* object, QEvent* event) override;
  void keyPressEvent(QKeyEvent*) override;

  /// remember that mouse was pressed
  void mousePressEvent(QMouseEvent*) override;

  /// update annotations if cursorPositionChanged signal hasn't happened since
  /// the last mousePressEvent
  ///
  /// cursorPositionChanged can be due to mouse press but also to keyboard
  /// actions. Moreover, a mouse click on the same position will not trigger a
  /// cursorPositionChanged. Therefore when the mouse is pressed we set
  /// `needUpdateActiveAnno_` to true, and when it is released if it hasn't
  /// been set to false we update the active annotation and repaint annotations.
  void mouseReleaseEvent(QMouseEvent*) override;

private slots:
  void setDefaultFocus();
  void deleteActiveAnnotation();
  void updateExtraDataForActiveAnnotation(const QString& newData);
  void activateClusterAtCursorPos();
  void setLabelForSelectedRegion();
  void updateLabelChoicesButtonStates();
  void resetDocument();

private:
  void clearAnnotations();
  void fetchLabelsInfo();
  void fetchAnnotationsInfo();
  void clearTextSelection();
  QTextEdit::ExtraSelection
  makePaintedRegion(int startChar, int endChar, const QString& color,
                    const QString& textColor = "black", bool underline = false);
  void paintAnnotations();
  bool addAnnotation(int labelId, int startChar, int endChar);
  void deleteAnnotation(int);
  void deactivateActiveAnnotation();
  std::list<Cluster>::const_iterator clusterAtPos(int pos) const;

  /// pos: {startChar, id}
  int findNextAnnotation(AnnotationIndex pos, bool forward = true) const;

  /// Update clusters with a new annotation.

  /// Clusters that overlap with the new annotation are merged
  void addAnnotationToClusters(const AnnotationCursor& annotation,
                               std::list<Cluster>& clusters);

  /// Remove an annotation and update the clusters
  void removeAnnotationFromClusters(const AnnotationCursor& annotation,
                                    std::list<Cluster>& clusters);

  int activeAnnotation_ = -1;
  bool needUpdateActiveAnno_{};
  bool activeAnnoFormatIsSet_{};

  /// clusters of overlapping annotations
  std::list<Cluster> clusters_{};
  QMap<int, AnnotationCursor> annotations_{};
  QMap<int, LabelInfo> labels_{};

  /// Sorting annotations by {startChar, id}
  std::set<AnnotationIndex> sortedAnnotations_{};

  QLabel* titleLabel_;
  SearchableText* text_;
  LabelChoices* labelChoices_;
  AnnotationsModel* annotationsModel_ = nullptr;
  AnnotationsNavButtons* navButtons_ = nullptr;
  QTextCharFormat defaultFormat_;
  bool useBoldFont_ = true;
};

} // namespace labelbuddy

#endif
