#ifndef LABELBUDDY_DATABASE_H
#define LABELBUDDY_DATABASE_H

#include <memory>

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QMap>
#include <QObject>
#include <QProgressDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QTextStream>
#include <QVariant>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "csv.h"

/// \file
/// Utilities for manipulating databases.

namespace labelbuddy {

struct DocRecord {
  QString content{};
  QByteArray metadata{};
  QString user_provided_id{};
  QString declared_md5{};
  QJsonArray annotations{};
  bool valid_content = true;
  QString short_title{};
  QString long_title{};
};

class DocsReader {

public:
  DocsReader(const QString& file_path,
             QIODevice::OpenMode mode = QIODevice::ReadOnly | QIODevice::Text);
  virtual ~DocsReader();
  bool is_open() const;
  virtual bool read_next();
  const DocRecord* get_current_record() const;
  virtual int progress_max() const;
  virtual int current_progress() const;

protected:
  QFile* get_file();
  void set_current_record(std::unique_ptr<DocRecord>);
  static const int progress_range_max_{1000};

private:
  QFile file;
  std::unique_ptr<DocRecord> current_record{nullptr};
  double file_size_{};
};

class TxtDocsReader : public DocsReader {

public:
  TxtDocsReader(const QString& file_path);
  bool read_next() override;

private:
  QTextStream stream;
};

class XmlDocsReader : public DocsReader {

public:
  XmlDocsReader(const QString& file_path);
  bool read_next() override;

private:
  QXmlStreamReader xml;
  bool found_error{};
  void read_document();
  void read_document_text();
  void read_md5();
  void read_meta();
  void read_annotation_set();
  void read_annotation();
  void read_user_provided_id();
  void read_short_title();
  void read_long_title();
  std::unique_ptr<DocRecord> new_record{};
};

class CsvDocsReader : public DocsReader {
public:
  CsvDocsReader(const QString& file_path);
  bool read_next() override;

private:
  QTextStream stream;
  CsvMapReader csv;
  bool found_error_{};
  void read_annotation(const QMap<QString, QString>& csv_record,
                       QJsonArray& annotations);
};

std::unique_ptr<DocRecord> json_to_doc_record(const QJsonDocument&);
std::unique_ptr<DocRecord> json_to_doc_record(const QJsonValue&);
std::unique_ptr<DocRecord> json_to_doc_record(const QJsonObject&);
std::unique_ptr<DocRecord> json_to_doc_record(const QJsonArray&);

class JsonDocsReader : public DocsReader {

public:
  JsonDocsReader(const QString& file_path);
  bool read_next() override;
  int progress_max() const override;
  int current_progress() const override;

private:
  QJsonArray all_docs;
  QJsonArray::const_iterator current_doc;
  int total_n_docs;
  int n_seen{};
};

class JsonLinesDocsReader : public DocsReader {

public:
  JsonLinesDocsReader(const QString& file_path);
  bool read_next() override;

private:
  QTextStream stream;
};

class DocsWriter {
public:
  struct Annotation {
    int start_char;
    int end_char;
    QString label_name;
    QString extra_data;
  };

  DocsWriter(const QString& file_path, bool include_text,
             bool include_annotations, bool include_user_name,
             QIODevice::OpenMode mode = QIODevice::WriteOnly | QIODevice::Text);
  virtual ~DocsWriter();

  /// after constructing the file should be open and is_open should return true
  bool is_open() const;
  bool is_including_text() const;
  bool is_including_annotations() const;
  bool is_including_user_name() const;

  virtual void add_document(const QString& md5, const QString& content,
                            const QJsonObject& metadata,
                            const QList<Annotation>& annotations,
                            const QString& user_name,
                            const QString& user_provided_id,
                            const QString& short_title,
                            const QString& long_title);
  /// at the start of the output file
  virtual void write_prefix();

  /// at the end of the output file
  virtual void write_suffix();

protected:
  QFile* get_file();

private:
  QFile file;
  bool include_text_;
  bool include_annotations_;
  bool include_user_name_;
};

class DocsJsonLinesWriter : public DocsWriter {
public:
  DocsJsonLinesWriter(const QString& file_path, bool include_text,
                      bool include_annotations, bool include_user_name);
  void add_document(const QString& md5, const QString& content,
                    const QJsonObject& metadata,
                    const QList<Annotation>& annotations,
                    const QString& user_name, const QString& user_provided_id,
                    const QString& short_title,
                    const QString& long_title) override;

  void write_suffix() override;

protected:
  int get_n_docs() const;
  QTextStream& get_stream();

private:
  QTextStream stream;
  int n_docs{};
};

class DocsJsonWriter : public DocsJsonLinesWriter {
public:
  DocsJsonWriter(const QString& file_path, bool include_text,
                 bool include_annotations, bool include_user_name);

  void add_document(const QString& md5, const QString& content,
                    const QJsonObject& metadata,
                    const QList<Annotation>& annotations,
                    const QString& user_name, const QString& user_provided_id,
                    const QString& short_title,
                    const QString& long_title) override;
  void write_prefix() override;
  void write_suffix() override;
};

class DocsCsvWriter : public DocsWriter {
public:
  DocsCsvWriter(const QString& file_path, bool include_text,
                bool include_annotations, bool include_user_name);
  void add_document(const QString& md5, const QString& content,
                    const QJsonObject& metadata,
                    const QList<Annotation>& annotations,
                    const QString& user_name, const QString& user_provided_id,
                    const QString& short_title,
                    const QString& long_title) override;
  void write_prefix() override;

private:
  QTextStream stream;
  CsvWriter csv;
};

class DocsXmlWriter : public DocsWriter {
public:
  DocsXmlWriter(const QString& file_path, bool include_text,
                bool include_annotations, bool include_user_name);
  void add_document(const QString& md5, const QString& content,
                    const QJsonObject& metadata,
                    const QList<Annotation>& annotations,
                    const QString& user_name, const QString& user_provided_id,
                    const QString& short_title,
                    const QString& long_title) override;
  void write_prefix() override;
  void write_suffix() override;

private:
  QXmlStreamWriter xml;

  void add_annotations(const QList<Annotation>& annotations);
};

struct LabelRecord {
  QString name;
  QString color;
  QString shortcut_key{};
};

enum class ErrorCode { NoError = 0, FileSystemError };

struct ImportDocsResult {
  int n_docs;
  int n_annotations;
  ErrorCode error_code;
};

struct ExportDocsResult {
  int n_docs;
  int n_annotations;
  ErrorCode error_code;
};

struct ImportLabelsResult {
  int n_labels;
  ErrorCode error_code;
};

struct ExportLabelsResult {
  int n_labels;
  ErrorCode error_code;
};

LabelRecord json_to_label_record(const QJsonValue& json);
QPair<QJsonArray, ErrorCode>
load_labels_into_json_array(const QString& file_path);

class RemoveConnection {
  QString connection_name_;
  bool cancelled_;

public:
  RemoveConnection(const QString& connection_name);
  ~RemoveConnection();
  void execute() const;
  void cancel();
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

  enum class Action { Import, Export };
  enum class ItemKind { Document, Label };
  QPair<QStringList, QString> accepted_and_default_formats(Action action,
                                                           ItemKind kind) const;

  /// Returns an error message if file extension is not appropriate

  /// If the file extension corresponds to one of the recognized formats (ie no
  /// error), returns an empty string.
  QString file_extension_error_message(const QString& file_path, Action action,
                                       ItemKind kind,
                                       bool accept_default) const;

  /// Imports documents in .json, .jsonl, .csv, .xml or .txt format

  /// If `progress` is not `nullptr`, used to display current progress.
  ImportDocsResult import_documents(const QString& file_path,
                                    QProgressDialog* progress = nullptr);

  /// Imports labels in .txt, .csv or .json format

  ImportLabelsResult import_labels(const QString& file_path);

  /// Exports annotations to .xml, .csv, .jsonl or .json formats.

  /// \param file_path file in which to write the exported docs and annotations
  /// \param labelled_docs_only only documents with at least one annotation are
  /// exported.
  /// \param include_text the documents' text is included in the exported
  /// data -- ie exported docs will have a `text` key.
  /// \param include_annotations the annotations are included -- exported docs
  /// will have a `labels` key.
  /// \param user_name value for the `annotation_approver` key. If an empty
  /// string this key won't be added.
  /// \param progress if not `nullptr`, used to display the export progress
  ExportDocsResult export_documents(const QString& file_path,
                                    bool labelled_docs_only = true,
                                    bool include_text = true,
                                    bool include_annotations = true,
                                    const QString& user_name = "",
                                    QProgressDialog* progress = nullptr);

  /// Exports labels to a .json or .csv file.
  ExportLabelsResult export_labels(const QString& file_path);

  /// Return the name of the currently used database

  /// Unless it is the temporary database, this is also the path to the db file.
  QString get_current_database() const;

  /// Last directory in which a database was opened or if there isn't any, the
  /// home directory.

  /// Used for file dialogs
  QString get_last_opened_directory() const;

  /// The directory containing a file
  QString parent_directory(const QString& file_path) const;

  /// Retrieve a value from the `app_state_extra` table in the current database.
  QVariant get_app_state_extra(const QString& key,
                               const QVariant& default_value) const;

  /// Set a value in the `app_state_extra` table in the current database.
  void set_app_state_extra(const QString& key, const QVariant& value);

  /// execute SQLite's VACUUM
  void vacuum_db();

signals:
  /// emitted after opening a connection to a database for the first time
  void new_database_opened(const QString& database_name);

  /// emitted after loading example docs in the temporary database
  void temporary_database_filled(const QString& database_name);

private:
  QString current_database;

  bool store_db_path(const QString& db_path);

  /// whether db_path is an sqlite file as opposed to temp or in-memory db
  bool is_persistent_database(const QString& db_path) const;

  /// Check database, set foreign_keys pragma, create tables if necessary
  bool initialize_database(QSqlDatabase& database);
  bool create_tables(QSqlQuery& query);

  /// Last used database if it is found in QSettings and exists else ""
  QString get_default_database_path() const;

  /// transform to absolute path unless it is the temp db, :memory:, or ""
  QString absolute_database_path(const QString& database_path) const;

  int insert_doc_record(const DocRecord& record, QSqlQuery& query);
  int insert_doc_annotations(int doc_id, const QJsonArray& annotations,
                             QSqlQuery& query);

  void insert_label(QSqlQuery& query, const QString& label_name,
                    const QString& color = QString(),
                    const QString& shortcut_key = QString());

  int write_doc(DocsWriter& writer, int doc_id, bool include_text,
                bool include_annotations, const QString& user_name);

  ErrorCode write_labels_to_json(const QJsonArray& labels,
                                 const QString& file_path);
  ErrorCode write_labels_to_csv(const QJsonArray& labels,
                                const QString& file_path);

  int color_index{};
  bool tmp_db_data_loaded_{};
  static const QString tmp_db_name_;
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
int batch_import_export(
    const QString& db_path, const QList<QString>& labels_files,
    const QList<QString>& docs_files, const QString& export_labels_file,
    const QString& export_docs_file, bool labelled_docs_only, bool include_text,
    bool include_annotations, const QString& user_name, bool vacuum);

} // namespace labelbuddy

#endif
