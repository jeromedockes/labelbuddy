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
  int n_docs;
  int n_annotations;
  ErrorCode error_code;
  QString error_message;
};

struct ExportDocsResult {
  int n_docs;
  int n_annotations;
  ErrorCode error_code;
  QString error_message;
};

struct ImportLabelsResult {
  int n_labels;
  ErrorCode error_code;
  QString error_message;
};

struct ExportLabelsResult {
  int n_labels;
  ErrorCode error_code;
  QString error_message;
};

/// Class to handle connections to databases and import and export operations.

/// For each SQLite file, the Qt connection name is exactly the file path. If we
/// ask to open the same file again the existing connection is reused.
///
/// A SQLite temporary database (ie database name = "") is also created when
/// constructing the object. After construction this temp db has the correct
/// schema (but its tables are empty) and it is the `current_database` -- the
/// `current_database` is always positioned on a valid database. The
/// corresponding connection name is `:LABELBUDDY_TEMPORARY_DATABASE:`.
class DatabaseCatalog : public QObject {

  Q_OBJECT

public:
  enum class Action { Import, Export };
  enum class ItemKind { Document, Label };

  DatabaseCatalog(QObject* parent = nullptr);

  /// Open a connection to a SQLite database or return the existing one.

  /// Returns true if the database was opened sucessfully (or already existed)
  /// and false otherwise. If database_path is empty, attempts to open the last
  /// database that was opened (according to the QSettings).
  ///
  /// If database_path is :LABELBUDDY_TEMPORARY_DATABASE:, opens the sqlite
  /// temporary db
  ///
  /// If remember is true, stores the path in QSettings for next execution of
  /// the program.
  bool open_database(const QString& database_path = QString(),
                     bool remember = true);

  /// Open the temporary database.

  /// Only one is opened per execution of the program; if we call this function
  /// again the current database is set to the previously created temporary
  /// database.
  ///
  /// It is a sqlite temporary database so in memory by default but flushed to
  /// disk if it becomes big: https://www.sqlite.org/inmemorydb.html#temp_db
  /// https://doc.qt.io/qt-5/sql-driver.html#qsqlite
  ///
  /// The database is created at construction, but example docs and labels are
  /// only inserted when this function is called with `load_data = true` for the
  /// first time.
  QString open_temp_database(bool load_data = true);

  /// Imports documents in .json, .jsonl, or .txt format

  /// If `progress` is not `nullptr`, used to display current progress.
  ImportDocsResult import_documents(const QString& file_path,
                                    QProgressDialog* progress = nullptr);

  /// Imports labels in .txt or .json format

  ImportLabelsResult import_labels(const QString& file_path);

  /// Exports annotations to .jsonl or .json formats.

  /// \param file_path file in which to write the exported docs and annotations
  /// \param labelled_docs_only only documents with at least one annotation are
  /// exported.
  /// \param include_text the documents' text is included in the exported
  /// data -- ie exported docs will have a `text` key.
  /// \param include_annotations the annotations are included -- exported docs
  /// will have a `labels` key.
  /// \param progress if not `nullptr`, used to display the export progress
  ExportDocsResult export_documents(const QString& file_path,
                                    bool labelled_docs_only = true,
                                    bool include_text = true,
                                    bool include_annotations = true,
                                    QProgressDialog* progress = nullptr) const;

  /// Exports labels to a .json file.
  ExportLabelsResult export_labels(const QString& file_path) const;

  /// Returns an error message if file extension is not appropriate

  /// If the file extension corresponds to one of the recognized formats (ie no
  /// error), returns an empty string.
  static QString file_extension_error_message(const QString& file_path,
                                              Action action, ItemKind kind,
                                              bool accept_default);

  /// Return the name of the currently used database

  /// Unless it is the temporary database, this is also the path to the db file.
  QString get_current_database() const;

  /// Last directory in which a database was opened or if there isn't any, the
  /// home directory.

  /// Used for file dialogs
  static QString get_last_opened_directory();

  /// Retrieve a value from the `app_state_extra` table in the current database.
  QVariant get_app_state_extra(const QString& key,
                               const QVariant& default_value) const;

  /// Set a value in the `app_state_extra` table in the current database.
  void set_app_state_extra(const QString& key, const QVariant& value) const;

  /// execute SQLite's VACUUM
  void vacuum_db() const;

signals:
  /// emitted after opening a connection to a database for the first time
  void new_database_opened(const QString& database_name);

  /// emitted after loading example docs in the temporary database
  void temporary_database_filled(const QString& database_name);

private:
  // first 4 bytes of the md5 checksum of "labelbuddy" (ascii-encoded) read as a
  // big-endian signed int
  static constexpr int32_t sqlite_application_id_ = -14315518;
  static constexpr int32_t sqlite_user_version_ = 3;

  QString current_database_;

  bool store_db_path(const QString& db_path) const;

  /// whether db_path is an sqlite file as opposed to temp or in-memory db
  bool is_persistent_database(const QString& db_path) const;

  /// Check database, set foreign_keys pragma, create tables if necessary
  static bool initialize_database(QSqlDatabase& database);

  static bool create_tables(QSqlQuery& query);

  /// transform to absolute path unless it is the temp db, :memory:, or ""
  QString absolute_database_path(const QString& database_path) const;

  int insert_doc_record(const DocRecord& record, QSqlQuery& query);

  int insert_doc_annotations(int doc_id, const QList<Annotation>& annotations,
                             QSqlQuery& query);

  void insert_label(QSqlQuery& query, const QString& label_name,
                    const QString& color = QString(),
                    const QString& shortcut_key = QString());

  int write_doc(DocsWriter& writer, int doc_id, bool include_text,
                bool include_annotations) const;

  int color_index_{};
  bool tmp_db_data_loaded_{};
  const QString tmp_db_name_{":LABELBUDDY_TEMPORARY_DATABASE:"};
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
int batch_import_export(const QString& db_path,
                        const QList<QString>& labels_files,
                        const QList<QString>& docs_files,
                        const QString& export_labels_file,
                        const QString& export_docs_file,
                        bool labelled_docs_only, bool include_text,
                        bool include_annotations, bool vacuum);

} // namespace labelbuddy

#endif
