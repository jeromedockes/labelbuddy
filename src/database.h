#ifndef LABELBUDDY_DATABASE_H
#define LABELBUDDY_DATABASE_H

#include <memory>

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QObject>
#include <QProgressDialog>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVector>
#include <QTemporaryFile>
#include <QTextStream>
#include <QVariant>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

/// \file
/// Utilities for manipulating databases.

namespace labelbuddy {

struct DocRecord {
  QString content;
  QByteArray extra_data;
  QString declared_md5;
  QJsonArray annotations;
  bool valid_content = true;
  QString title{};
  QString short_title{};
  QString long_title{};
};

class DocsReader {

public:
  DocsReader(const QString& file_path);
  bool is_open() const;
  virtual bool read_next();
  const DocRecord* get_current_record() const;
  virtual int progress_max() const;
  virtual int current_progress() const;

protected:
  QFile* get_file();
  void set_current_record(std::unique_ptr<DocRecord>);

private:
  QFile file;
  std::unique_ptr<DocRecord> current_record{nullptr};
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
  void read_title();
  void read_short_title();
  void read_long_title();
  std::unique_ptr<DocRecord> new_record{};
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

class AnnotationsWriter {
public:
  struct Annotation {
    int start_char;
    int end_char;
    QString label_name;
  };

  AnnotationsWriter(const QString& file_path);
  bool is_open() const;
  virtual void add_document(const QString& md5, const QVariant& content,
                            const QJsonObject& extra_data,
                            const QList<Annotation> annotations,
                            const QString& user_name, const QString& title,
                            const QString& short_title,
                            const QString& long_title);
  virtual void write_prefix();
  virtual void write_suffix();

protected:
  QFile* get_file();

private:
  QFile file;
};

class AnnotationsJsonLinesWriter : public AnnotationsWriter {
public:
  AnnotationsJsonLinesWriter(const QString& file_path);
  void add_document(const QString& md5, const QVariant& content,
                    const QJsonObject& extra_data,
                    const QList<Annotation> annotations,
                    const QString& user_name, const QString& title,
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

class AnnotationsJsonWriter : public AnnotationsJsonLinesWriter {
public:
  AnnotationsJsonWriter(const QString& file_path);
  void add_document(const QString& md5, const QVariant& content,
                    const QJsonObject& extra_data,
                    const QList<Annotation> annotations,
                    const QString& user_name, const QString& title,
                    const QString& short_title,
                    const QString& long_title) override;
  void write_prefix() override;
  void write_suffix() override;
};

class AnnotationsXmlWriter : public AnnotationsWriter {
public:
  AnnotationsXmlWriter(const QString& file_path);
  void add_document(const QString& md5, const QVariant& content,
                    const QJsonObject& extra_data,
                    const QList<Annotation> annotations,
                    const QString& user_name, const QString& title,
                    const QString& short_title,
                    const QString& long_title) override;
  void write_prefix() override;
  void write_suffix() override;

private:
  QXmlStreamWriter xml;

  void add_annotations(const QList<Annotation> annotations);
};

struct LabelRecord {
  QString name;
  QString color;
  QString shortcut_key{};
};

LabelRecord json_to_label_record(const QJsonValue& json);
QJsonArray read_json_array(const QString& file_path);

/// Class to handle connections to databases and import and export operations.

/// Maintains a catalog of sqlite3 file paths. For each file, the Qt database
/// name is exactly the file path. If we ask to open the same file again the
/// existing database is reused.
///
class DatabaseCatalog : public QObject {

  Q_OBJECT

public:
  /// Open a connection to a SQLite database.

  /// Returns true if the database was opened sucessfully and false otherwise.
  /// If database_path is empty, attempts to open the last databased that was
  /// opened (according to the QSettings), and if it cannot be opened, the
  /// default database (~/labelbuddy_data.sqlite3). If database_path is not
  /// empty and it cannot be opened, returns false without considering the
  /// default databases.
  bool open_database(const QString& database_path = QString(),
                     bool remember = true);

  /// Open a temporary database.

  /// Only one is opened per execution of the program, if we call this function
  /// again the current database is set to the previously created temporary
  /// database.
  QString open_temp_database();

  /// Imports documents in .json, .jsonl, .xml or .txt format

  /// If `progress` is not `nullptr`, used to display current progress.
  /// Returns the number of imported (new) documents and annotations
  /// ie `{n docs, n annotations}`.
  QList<int> import_documents(const QString& file_path,
                              QProgressDialog* progress = nullptr);

  /// Imports labels in .txt or .json format

  /// Returns number of imported (new) labels.
  int import_labels(const QString& file_path);

  /// Exports annotations to .xml, .jsonl or .json formats.

  /// \param file_path file in which to write the exported docs and annotations
  /// \param labelled_docs_only only documents with at least one annotation are
  /// exported.
  /// \param include_documents the documents' text is included in the exported
  /// data -- ie exported docs will have a `text` key.
  /// \param user_name value for the `annotation_approver` key. If an empty
  /// string this key won't be added.
  /// \param progress if not `nullptr`, used to display the export progress
  /// \returns `{n exported docs, n exported annotations}`
  QList<int> export_annotations(const QString& file_path,
                                bool labelled_docs_only = true,
                                bool include_documents = true,
                                const QString& user_name = "",
                                QProgressDialog* progress = nullptr);

  /// Exports labels to a .json file. Returns number of exported labels
  int export_labels(const QString& file_path);

  /// Return the name (which is also the path) of the currently used database
  QString get_current_database() const;
  /// Return the directory of the current database

  /// to be stored in QSettings so that we start from the same directory
  /// next time the user clicks File > Open or New
  QString get_current_directory() const;

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

private:
  QSet<QString> databases{};
  QString current_database;
  std::unique_ptr<QTemporaryFile> temp_database = nullptr;

  bool create_tables(QSqlDatabase& database);

  /// Last used database if it can be opened else `~/labelbuddy_data.sqlite3`
  QString get_default_database_path();

  /// Find which database to open and return its absolute path.

  /// If `database_path` is not empty, user asked for this one explicitly
  /// - if it can be opened for reading and writing, return it
  /// - otherwise return empty string to indicate failure without considering
  ///   the default ones.
  ///
  /// If `database_path` is empty, user didn't specify a database, find a
  /// default one:
  /// - Last used if it exists and can be opened
  /// - default `~/labelbuddy_data.sqlite3` if it can be opened
  /// - otherwise return empty string to indicate failure
  QString check_database_path(const QString& database_path);
  int insert_doc_record(const DocRecord& record, QSqlQuery& query);
  void insert_label(QSqlQuery& query, const QString& label_name,
                    const QString& color = QString(),
                    const QString& shortcut_key = QString());
  int write_doc_and_annotations(AnnotationsWriter& writer, int doc_id,
                                bool include_document,
                                const QString& user_name);

  const QVector<QString> label_colors{
      "#aec7e8", "#ffbb78", "#98df8a", "#ff9896", "#c5b0d5",
      "#c49c94", "#f7b6d2", "#c7c7c7", "#dbdb8d", "#9edae5"};
  int color_index{};
};

/// Perform import, export, or vacuum operations without the GUI.

/// Returns `true` if successful. Starts by importing labels, then docs, then
/// exporting labels, then exporting docs.
/// If vacuum is `true`, executes `VACUUM` and does not consider any of the
/// other operations.
bool batch_import_export(const QString& db_path,
                         const QList<QString>& labels_files,
                         const QList<QString>& docs_files,
                         const QString& export_labels_file,
                         const QString& export_docs_file,
                         bool labelled_docs_only, bool include_documents,
                         const QString& user_name, bool vacuum);

} // namespace labelbuddy

#endif
