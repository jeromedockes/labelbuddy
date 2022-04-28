#ifndef LABELBUDDY_UTILS_H
#define LABELBUDDY_UTILS_H

#include <QCommandLineParser>
#include <QFont>
#include <QModelIndexList>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QUrl>
#include <QWidget>

/// \file
/// Misc. utilities

namespace labelbuddy {

/// The labelbuddy version 'x.y.z'
QString get_version();

QString get_welcome_message();

/// URL to the html documentation

/// Searched in /usr/share/doc, the parent directory of the running program, and
/// if not found, return URL to the online doc.
QUrl get_doc_url();

void prepare_parser(QCommandLineParser& parser);

/// Find the first index that is in the column 0

/// If none of the indices are in column 0 return `indices.constEnd()`.
/// LabelList and DocList display column 0 but also have a hidden column
/// containing the row id. This is used to find the first selected item that is
/// in the displayed column -- hidden items could be selected by clicking
/// "select all".
///
/// In practice this is a redundant check because the corresponding models'
/// `flags` make the other columns non-selectable.
QModelIndexList::const_iterator
find_first_in_col_0(const QModelIndexList& indices);

/// ATM single lower-case letters. If `accept_empty` the empty string also
/// allowed.

/// Empty string can be acceptable to clear the label's shortcut in Dataset tab.
QRegularExpression shortcut_key_pattern(bool accept_empty = false);

/// Display string for the database name (path or helpful message if temp db)

/// `database_name` is the Qt database name; if not temporary database it is the
/// path to the sqlite file. If `full_path` is false only filename is returned;
/// if `temp_warning` and database is temporary or in-memory a message saying it
/// will disappear is added.
QString database_name_display(const QString& database_name,
                              bool full_path = true, bool temp_warning = true);

enum class Side { Left, Right };

void scale_margin(QWidget& widget, Side side, float scale = .5);

const QString& suggest_label_color(int color_index);
const QString& suggest_label_color();

int cast_progress_to_range(double current, double maximum,
                           double range_max = 1000);

/// compare sequences as sets ie irrespective of order or duplicates
template <typename C1, typename C2>
bool set_compare(const C1& container_1, const C2& container_2) {
  QSet<typename C1::value_type> set_1{};
  for (const auto& val_1 : container_1) {
    set_1 << val_1;
  }
  QSet<typename C2::value_type> set_2{};
  for (const auto& val_2 : container_2) {
    set_2 << val_2;
  }
  return set_1 == set_2;
}

/// The directory containing a file
QString parent_directory(const QString& file_path);

} // namespace labelbuddy
#endif
