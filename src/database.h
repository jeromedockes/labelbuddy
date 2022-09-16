#ifndef LABELBUDDY_DATABASE_H
#define LABELBUDDY_DATABASE_H

#include <memory>

#include <QByteArray>
#include <QFile>
#include <QMap>
#include <QObject>
#include <QProgressDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QTextStream>
#include <QVariant>

/// \file
/// Utilities for manipulating databases.

namespace labelbuddy {

struct Annotation;
struct DocRecord;
class DocsWriter;

enum class ErrorCode { NoError = 0, CriticalParsingError, FileSystemError };

struct ImportDocsResult {
  int nDocs;
  int nAnnotations;
  ErrorCode errorCode;
  QString errorMessage;
};

struct ExportDocsResult {
  int nDocs;
  int nAnnotations;
  ErrorCode errorCode;
  QString errorMessage;
};

struct ImportLabelsResult {
  int nLabels;
  ErrorCode errorCode;
  QString errorMessage;
};

struct ExportLabelsResult {
  int nLabels;
  ErrorCode errorCode;
  QString errorMessage;
};

/// Class to handle connections to databases and import and export operations.

/// For each SQLite file, the Qt connection name is exactly the file path. If we
/// ask to open the same file again the existing connection is reused.
///
/// A SQLite temporary database (ie database name = "") is also created when
/// constructing the object. After construction this temp db has the correct
/// schema (but its tables are empty) and it is the `currentDatabase_` -- the
/// `currentDatabase_` is always positioned on a valid database. The
/// corresponding connection name is `:LABELBUDDY_TEMPORARY_DATABASE:`.
class DatabaseCatalog : public QObject {

  Q_OBJECT

public:
  enum class Action { Import, Export };
  enum class ItemKind { Document, Label };

  DatabaseCatalog(QObject* parent = nullptr);

  /// Open a connection to a SQLite database or return the existing one.

  /// Returns true if the database was opened sucessfully (or already existed)
  /// and false otherwise. If databasePath is empty, attempts to open the last
  /// database that was opened (according to the QSettings).
  ///
  /// If databasePath is :LABELBUDDY_TEMPORARY_DATABASE:, opens the sqlite
  /// temporary db
  ///
  /// If remember is true, stores the path in QSettings for next execution of
  /// the program.
  bool openDatabase(const QString& databasePath = QString(),
                    bool remember = true);

  /// Open the temporary database.

  /// Only one is opened per execution of the program; if we call this function
  /// again the current database is set to the previously created temporary
  /// database.
  ///
  /// It is a sqlite temporary database so in memory by default but flushed to
  /// disk if it becomes big: https://www.sqlite.org/inmemorydb.html#tempDb
  /// https://doc.qt.io/qt-5/sql-driver.html#qsqlite
  ///
  /// The database is created at construction, but example docs and labels are
  /// only inserted when this function is called with `loadData = true` for the
  /// first time.
  QString openTempDatabase(bool loadData = true);

  /// Imports documents in .json, .jsonl, or .txt format

  /// If `progress` is not `nullptr`, used to display current progress.
  ImportDocsResult importDocuments(const QString& filePath,
                                   QProgressDialog* progress = nullptr);

  /// Imports labels in .txt or .json format

  ImportLabelsResult importLabels(const QString& filePath);

  /// Exports annotations to .jsonl or .json formats.

  /// \param filePath file in which to write the exported docs and annotations
  /// \param labelledDocsOnly only documents with at least one annotation are
  /// exported.
  /// \param includeText the documents' text is included in the exported
  /// data -- ie exported docs will have a `text` key.
  /// \param includeAnnotations the annotations are included -- exported docs
  /// will have a `labels` key.
  /// \param progress if not `nullptr`, used to display the export progress
  ExportDocsResult exportDocuments(const QString& filePath,
                                   bool labelledDocsOnly = true,
                                   bool includeText = true,
                                   bool includeAnnotations = true,
                                   QProgressDialog* progress = nullptr) const;

  /// Exports labels to a .json file.
  ExportLabelsResult exportLabels(const QString& filePath) const;

  /// Returns an error message if file extension is not appropriate

  /// If the file extension corresponds to one of the recognized formats (ie no
  /// error), returns an empty string.
  static QString fileExtensionErrorMessage(const QString& filePath,
                                           Action action, ItemKind kind,
                                           bool acceptDefault);

  /// Return the name of the currently used database

  /// Unless it is the temporary database, this is also the path to the db file.
  QString getCurrentDatabase() const;

  /// Last directory in which a database was opened or if there isn't any, the
  /// home directory.

  /// Used for file dialogs
  static QString getLastOpenedDirectory();

  /// Retrieve a value from the `appStateExtra` table in the current database.
  QVariant getAppStateExtra(const QString& key,
                            const QVariant& defaultValue) const;

  /// Set a value in the `appStateExtra` table in the current database.
  void setAppStateExtra(const QString& key, const QVariant& value) const;

  /// execute SQLite's VACUUM
  void vacuumDb() const;

signals:
  /// emitted after opening a connection to a database for the first time
  void newDatabaseOpened(const QString& databaseName);

  /// emitted after loading example docs in the temporary database
  void temporaryDatabaseFilled(const QString& databaseName);

private:
  // first 4 bytes of the md5 checksum of "labelbuddy" (ascii-encoded) read as a
  // big-endian signed int
  static constexpr int32_t sqliteApplicationId_ = -14315518;
  static constexpr int32_t sqliteUserVersion_ = 3;

  QString currentDatabase_;

  bool storeDbPath(const QString& dbPath) const;

  /// whether dbPath is an sqlite file as opposed to temp or in-memory db
  bool isPersistentDatabase(const QString& dbPath) const;

  /// Check database, set foreignKeys pragma, create tables if necessary
  static bool initializeDatabase(QSqlDatabase& database);

  static bool createTables(QSqlQuery& query);

  /// transform to absolute path unless it is the temp db, :memory:, or ""
  QString absoluteDatabasePath(const QString& databasePath) const;

  int insertDocRecord(const DocRecord& record, QSqlQuery& query);

  int getDocLength(int docId, QSqlQuery& query);

  int insertDocAnnotations(int docId, const QList<Annotation>& annotations,
                           QSqlQuery& query);

  void insertLabel(QSqlQuery& query, const QString& labelName,
                   const QString& color = QString(),
                   const QString& shortcutKey = QString());

  int writeDoc(DocsWriter& writer, int docId, bool includeText,
               bool includeAnnotations) const;

  QList<Annotation> getDocAnnotations(int docId, const QString& content) const;

  int colorIndex_{};
  bool tmpDbDataLoaded_{};
  const QString tmpDbName_{":LABELBUDDY_TEMPORARY_DATABASE:"};
};

/// Perform import, export, or vacuum operations without the GUI.

/// Returns 0 if there were no errors and 1 otherwise. Starts by importing
/// labels, then docs, then exporting labels, then exporting docs. If vacuum is
/// `true`, executes `VACUUM` and does not consider any of the other operations.
///
/// If one of the import files doesn't have a recognized extension it is
/// skipped to avoid inserting incorrect data in the database.
/// If one of the export files doesn't have a recognized extension a message is
/// printed, but the export is still performed in a default format (json) and it
/// is not considered an error -- this function can still return 0 if there were
/// no other errors.
int batchImportExport(const QString& dbPath, const QList<QString>& labelsFiles,
                      const QList<QString>& docsFiles,
                      const QString& exportLabelsFile,
                      const QString& exportDocsFile, bool labelledDocsOnly,
                      bool includeText, bool includeAnnotations, bool vacuum);

} // namespace labelbuddy

#endif
