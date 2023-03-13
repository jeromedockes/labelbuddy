#ifndef LABELBUDDY_ANNOTATOR_H
#define LABELBUDDY_ANNOTATOR_H

#include <list>
#include <memory>
#include <set>

#include <QCompleter>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QSplitter>
#include <QSqlQueryModel>
#include <QWidget>

#include "annotations_list.h"
#include "annotations_list_model.h"
#include "annotations_model.h"
#include "label_list.h"
#include "label_list_model.h"
#include "no_deselect_all_view.h"
#include "searchable_text.h"

/// \file
/// Implementation of the Annotator tab

namespace labelbuddy {

/// Widget use to set the label for the selection
class AnnotationEditor : public QWidget {
  Q_OBJECT

public:
  AnnotationEditor(QWidget* parent = nullptr);
  void setLabelsModel(LabelListModel*);
  void setAnnotationsModel(AnnotationsModel*);

  /// Set the currently selected label based on its `id` in the db
  void setSelectedLabelId(int labelId);
  bool isLabelChoiceEnabled() const;

signals:
  void selectedLabelChanged(int labelId);
  void deleteButtonClicked();
  void extraDataChanged(const QString& newData);
  void extraDataEditFinished();
  void clicked();

public slots:
  void setAnnotation(AnnotationInfo annotation);
  void enableLabelChoice();
  void disableLabelChoice();

protected:
  bool eventFilter(QObject* object, QEvent* event) override;

private slots:
  void onLabelSelectionChange();

private:
  LabelListModel* labelListModel_ = nullptr;
  AnnotationsModel* annotationsModel_ = nullptr;
  std::unique_ptr<QCompleter> extraDataCompleter_ = nullptr;

  QLabel* labelsViewTitle_ = nullptr;
  NoDeselectAllView* labelsView_ = nullptr;
  QLabel* extraDataTitle_ = nullptr;
  QLineEdit* extraDataEdit_ = nullptr;
  QPushButton* deleteButton_ = nullptr;

  void enableDeleteAndEdit();
  void disableDeleteAndEdit();

  /// The currently selected label's `id` in the database
  int selectedLabelId() const;

  /// Set the extra data in the line edit
  void setExtraData(const QString& newData);
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

  StatusBarInfo currentStatusInfo() const;

signals:

  void activeAnnotationIdChanged(int activeAnnotation);
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
  void activateAnnotation(int annotationId);
  void activateClusterAtCursorPos();
  void setLabelForSelectedRegion(int labelId);
  void updateAnnotationEditor();
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

  void emitActiveAnnotationChanged();

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
  AnnotationEditor* annotationEditor_;
  AnnotationsModel* annotationsModel_ = nullptr;
  AnnotationsNavButtons* navButtons_ = nullptr;
  AnnotationsList* annotationsList_;
  QTextCharFormat defaultFormat_;
  bool useBoldFont_ = true;

  /// font size scaling for the active annotation

  /// Only applied when the "show selected annotation in bold font" option is
  /// checked (in the menu).
  static constexpr double activeAnnotationScaling_ = 1.25;
};

} // namespace labelbuddy

#endif
