#ifndef LABELBUDDY_ANNOTATIONS_MODEL_H
#define LABELBUDDY_ANNOTATIONS_MODEL_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QSqlQuery>
#include <QString>
#include <QStringList>

#include "char_indices.h"
#include "user_roles.h"

/// \file
/// Model providing information to the Annotator

namespace labelbuddy {

struct LabelInfo {
  int id;
  QString color;
  QString name;
};

struct AnnotationInfo {
  int id;
  int labelId;
  int startChar;
  int endChar;
  QString extraData;
};

/// Model providing information to the Annotator

/// It is positionned on one particular document and provides information such
/// as its text and annotations.
/// Can be moved to a different document using `visitNext`,
/// `visitNextLabelled`, `visitDoc` etc.
class AnnotationsModel : public QObject {
  Q_OBJECT

public:
  AnnotationsModel(QObject* parent = nullptr);

  /// Get the `content` (text) of the current document.

  /// Returns the empty string if current doc not in database (happens for
  /// example if database is empty). The text is stored in the model so this is
  /// cheap to use thanks to the QString's implicit sharing
  QString getContent() const;

  /// display_title for current document if it exists else ''
  QString getTitle() const;

  /// Insert an annotation for the current doc.

  /// If insertion fails (eg due to db violation constraint) returns -1
  /// Otherwise returns the new annotation's `rowid`. If this is the document's
  /// first annotation, emits `documentStatusChanged` (it changed from
  /// unlabelled to labelled)
  /// If it is its first annotation with this label emit
  /// `documentGainedLabel`.
  int addAnnotation(int labelId, int startChar, int endChar);

  /// Delete annotation provided its `rowid` in the database

  /// Returns the number of deleted annotations (0 or 1)
  ///
  /// If after delete operation the current doc has 0 annotations, emits
  /// `documentStatusChanged` (as it changed from labelled to unlabelled).
  /// If after delete it has 0 annotations with this label, emits
  /// `documentLostLabel`.
  int deleteAnnotation(int annotationId);

  bool updateAnnotationExtraData(int annotationId, const QString& newData);

  /// Info for all labels in the database

  /// Mapping label id -> annotation info
  QMap<int, LabelInfo> getLabelsInfo() const;

  /// Info for annotations on the current document.

  /// Mapping annotation id -> annotation info
  QMap<int, AnnotationInfo> getAnnotationsInfo() const;

  /// Get all the extra data for annotations with this label in current doc
  QStringList existingExtraDataForLabel(int labelId) const;

  /// false when db is empty
  bool isPositionedOnValidDoc() const;

  /// Current doc's position in the list of all docs sorted by id

  /// Note this is not the same as the id
  int currentDocPosition() const;

  /// number of docs in the database
  int totalNDocs() const;
  bool hasNext() const;
  bool hasPrev() const;
  bool hasNextLabelled() const;
  bool hasPrevLabelled() const;
  bool hasNextUnlabelled() const;
  bool hasPrevUnlabelled() const;

  /// get the `id` of label that has shortcut key `shortcut`

  /// returns -1 if no label has that shortcut.
  int shortcutToId(const QString& shortcut) const;

  /// QString (utf-16) index to index in unicode sequence
  int qStringIdxToUnicodeIdx(int qStringIndex) const;

  /// convert index in unicode sequence to QString (utf-16) index
  int unicodeIdxToQStringIdx(int unicodeIndex) const;

public slots:

  void visitNext();
  void visitPrev();
  void visitNextLabelled();
  void visitPrevLabelled();
  void visitNextUnlabelled();
  void visitPrevUnlabelled();
  void visitDoc(int docId);
  void visitFirstDoc();

  /// check that the current doc still exists and go to first doc if not

  /// called after delete operations in the Dataset tab
  void checkCurrentDoc();

  void setDatabase(const QString& newDatabaseName);

signals:

  /// current document changed, ie we are now visiting a different doc
  void documentChanged();

  /// current document's status (labelled or unlabelled) changed
  void documentStatusChanged(DocumentStatus newStatus);

  void documentGainedLabel(int labelId, int docId);
  void documentLostLabel(int labelId, int docId);

  void annotationAdded(AnnotationInfo annotation);
  void annotationDeleted(int annotationId);
  void extraDataChanged(int annotationId, QString extraData);

private:
  int currentDocId_ = -1;
  QString databaseName_;

  QString text_{};
  CharIndices charIndices_;

  QSqlQuery getQuery() const;

  void updateText();

  /// query must return the id of a document to visit
  /// if there are no results, returns false
  bool visitQueryResult(const QString& queryText);
  int getQueryResult(const QString& queryText) const;

  int lastDocId() const;
  int firstDocId() const;
  int lastLabelledDocId() const;
  int firstLabelledDocId() const;
  int lastUnlabelledDocId() const;
  int firstUnlabelledDocId() const;
};
} // namespace labelbuddy

#endif
