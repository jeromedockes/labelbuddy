#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QItemSelectionModel>
#include <QLabel>
#include <QListView>
#include <QRect>
#include <QSortFilterProxyModel>
#include <QStyle>
#include <QTextDocument>
#include <QVBoxLayout>

#include "annotations_list.h"
#include "user_roles.h"

namespace labelbuddy {

PainterRestore::PainterRestore(QPainter* painter) : painter_{painter} {
  if (painter_ != nullptr) {
    painter->save();
  }
}

PainterRestore::~PainterRestore() {
  if (painter_ != nullptr) {
    painter_->restore();
  }
}

AnnotationDelegate::AnnotationDelegate(QObject* parent)
    : QStyledItemDelegate{parent} {
  QFontMetrics metrics{QFont{}};
  em_ = metrics.height();
}

static const QString annotationItemTemplate = R"(
<div style='font-size:medium;color:#000'>
<h3 style='margin:0;margin-bottom:%6;font-size:medium;background-color:%1'>%2</h3>
<p style='margin:0;'>
%3<span style='background-color:%1;'>%4</span>%5
</p>
</div>
)";

static const QString selectedAnnotationItemTemplate = R"(
<div style='font-size:medium;color:#000;'>
<h3 style='margin:0;margin-bottom:%6;font-size:large;background-color:%1'>%2</h3>
<p style='margin:0;'>
%3<span style='background-color:white;font-size:large;font-weight:bold;'>%4</span>%5
</p>
</div>
)";

void AnnotationDelegate::paint(QPainter* painter,
                               const QStyleOptionViewItem& option,
                               const QModelIndex& index) const {
  auto labelName = index.data(Roles::LabelNameRole).value<QString>();
  auto labelColor = index.data(Qt::BackgroundRole).value<QColor>().name();
  auto selectedText = index.data(Roles::SelectedTextRole).value<QString>();
  auto prefix = index.data(Roles::AnnotationPrefixRole).value<QString>();
  auto suffix = index.data(Roles::AnnotationSuffixRole).value<QString>();
  auto margin = QString::number(.3 * em_);
  auto itemTemplate = annotationItemTemplate;
  if (option.state & QStyle::State_Selected) {
    margin = "1px";
    itemTemplate = selectedAnnotationItemTemplate;
  }
  QTextDocument document{};
  document.setHtml(itemTemplate.arg(
      labelColor, labelName.toHtmlEscaped(), prefix.toHtmlEscaped(),
      selectedText.toHtmlEscaped(), suffix.toHtmlEscaped(), margin));
  if (option.state & QStyle::State_Selected) {
    PainterRestore restore{painter};
    painter->fillRect(option.rect, labelColor);
    painter->setPen(QColor{0, 0, 0, 70});
    painter->drawLine(option.rect.topLeft(), option.rect.topRight());
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
  }
  {
    PainterRestore restore{painter};
    painter->translate(option.rect.left(), option.rect.top());
    QRect clip(0, 0, option.rect.width(), option.rect.height());
    document.drawContents(painter, clip);
  }
}

QSize AnnotationDelegate::sizeHint(const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const {
  auto defaultSize = QStyledItemDelegate::sizeHint(option, index);
  return QSize{defaultSize.width(), int(3.5 * em_)};
}

AnnotationsList::AnnotationsList(QWidget* parent) : QFrame(parent) {
  auto layout = new QVBoxLayout();
  setLayout(layout);
  auto title = new QLabel{"Annotations in this document:"};
  layout->addWidget(title);
  annotationsView_ = new QListView{};
  layout->addWidget(annotationsView_);
  auto delegate = new AnnotationDelegate{this};
  annotationsView_->setItemDelegate(delegate);
}

void AnnotationsList::setModel(AnnotationsModel* model) {
  assert(model != nullptr);
  annotationsModel_ = model;
  annotationsListModel_.reset(new AnnotationsListModel{});
  annotationsListModel_->setSourceModel(model);
  proxyModel_.reset(new QSortFilterProxyModel{});
  proxyModel_->setSourceModel(annotationsListModel_.get());
  proxyModel_->setSortRole(Roles::AnnotationStartCharRole);
  proxyModel_->sort(0);
  annotationsView_->setModel(proxyModel_.get());
  QObject::connect(annotationsView_->selectionModel(),
                   &QItemSelectionModel::selectionChanged, this,
                   &AnnotationsList::onSelectionChange);
}

void AnnotationsList::selectAnnotation(int annotationId) {
  if (annotationId == -1) {
    annotationsView_->selectionModel()->clearCurrentIndex();
    annotationsView_->selectionModel()->clearSelection();
  }
  auto modelIndex = annotationsListModel_->indexForAnnotationId(annotationId);
  auto sortedIndex = proxyModel_->mapFromSource(modelIndex);
  annotationsView_->selectionModel()->setCurrentIndex(
      sortedIndex, QItemSelectionModel::Clear | QItemSelectionModel::Select |
                       QItemSelectionModel::Current);
}

void AnnotationsList::resetAnnotations() {
  annotationsListModel_->resetAnnotations();
}

void AnnotationsList::onSelectionChange(const QItemSelection& selected) {
  if (!selected.indexes().size()) {
    return;
  }
  auto annotationId =
      selected.indexes()[0].data(Roles::AnnotationIdRole).value<int>();
  emit selectedAnnotationIdChanged(annotationId);
}

} // namespace labelbuddy
