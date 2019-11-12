/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "EntityAttributeGrid.h"

#include "Model/EntityAttributes.h"
#include "StringUtils.h"
#include "View/BorderLine.h"
#include "View/EntityAttributeItemDelegate.h"
#include "View/EntityAttributeModel.h"
#include "View/EntityAttributeTable.h"
#include "View/MapDocument.h"
#include "View/ViewConstants.h"
#include "View/QtUtils.h"

#include <QHeaderView>
#include <QTableView>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QAbstractButton>
#include <QShortcut>
#include <QKeySequence>
#include <QDebug>
#include <QKeyEvent>
#include <QSortFilterProxyModel>

namespace TrenchBroom {
    namespace View {
        EntityAttributeGrid::EntityAttributeGrid(MapDocumentWPtr document, QWidget* parent) :
        QWidget(parent),
        m_document(document) {
            createGui(document);
            createShortcuts();
            updateShortcuts();
            bindObservers();
        }

        EntityAttributeGrid::~EntityAttributeGrid() {
            unbindObservers();
        }

        void EntityAttributeGrid::addAttribute() {
            MapDocumentSPtr document = lock(m_document);
            const String newAttributeName = AttributeRow::newAttributeNameForAttributableNodes(document->allSelectedAttributableNodes());

            document->setAttribute(newAttributeName, "");

            // Force an immediate update to the table rows (by default, updates are delayed - see EntityAttributeGrid::updateControls),
            // so we can select the new row.
            m_model->updateFromMapDocument();

            const int row = m_model->rowForAttributeName(newAttributeName);
            ensure(row != -1, "row should have been inserted");

            // Select the newly inserted attribute name
            const QModelIndex mi = m_proxyModel->mapFromSource(m_model->index(row, 0));

            m_table->setCurrentIndex(mi);
            m_table->setFocus();
        }

        void EntityAttributeGrid::removeSelectedAttributes() {
            assert(canRemoveSelectedAttributes());

            const auto selectedRows = selectedRowsAndCursorRow();

            StringList attributes;
            for (const int row : selectedRows) {
                attributes.push_back(m_model->attributeName(row));
            }

            const size_t numRows = attributes.size();
            MapDocumentSPtr document = lock(m_document);

            {
                Transaction transaction(document, StringUtils::safePlural(numRows, "Remove Attribute", "Remove Attributes"));

                bool success = true;
                for (const String& attribute : attributes) {
                    success = success && document->removeAttribute(attribute);
                }

                if (!success) {
                    transaction.rollback();
                }
            }
        }

        bool EntityAttributeGrid::canRemoveSelectedAttributes() const {
            const auto rows = selectedRowsAndCursorRow();
            if (rows.empty())
                return false;

            for (const int row : rows) {
                if (!m_model->canRemove(row))
                    return false;
            }
            return true;
        }

        /**
         * returns rows indices in the model (not proxy model).
         */
        std::set<int> EntityAttributeGrid::selectedRowsAndCursorRow() const {
            std::set<int> result;

            QItemSelectionModel* selection = m_table->selectionModel();

            // current row
            const QModelIndex currentIndexInSource = m_proxyModel->mapToSource(selection->currentIndex());
            if (currentIndexInSource.isValid()) {
                result.insert(currentIndexInSource.row());
            }

            // selected rows
            for (const QModelIndex& index : selection->selectedIndexes()) {
                const QModelIndex indexInSource = m_proxyModel->mapToSource(index);
                if (indexInSource.isValid()) {
                    result.insert(indexInSource.row());
                }
            }

            return result;
        }

        class EntitySortFilterProxyModel : public QSortFilterProxyModel {
        public:
            EntitySortFilterProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

        protected:
            bool lessThan(const QModelIndex& left, const QModelIndex& right) const {
                const EntityAttributeModel& source = dynamic_cast<const EntityAttributeModel&>(*sourceModel());

                return source.lessThan(static_cast<size_t>(left.row()), static_cast<size_t>(right.row()));
            }
        };

        void EntityAttributeGrid::createGui(MapDocumentWPtr document) {
            m_table = new EntityAttributeTable();

            m_model = new EntityAttributeModel(document, this);
            m_model->setParent(m_table); // ensure the table takes ownership of the model in setModel // FIXME: why? this looks unnecessary

            m_proxyModel = new EntitySortFilterProxyModel(this);
            m_proxyModel->setSourceModel(m_model);
            m_proxyModel->sort(0);
            m_table->setModel(m_proxyModel);

            m_table->setItemDelegate(new EntityAttributeItemDelegate(m_table, m_model, m_proxyModel, m_table));

            autoResizeRows(m_table);

            m_table->setStyleSheet("QTableView { border: none; }");
            m_table->verticalHeader()->setVisible(false);
            m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
            m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
            m_table->horizontalHeader()->setSectionsClickable(false);
            m_table->setSelectionBehavior(QAbstractItemView::SelectItems);

            m_addAttributeButton = createBitmapButton("Add.png", tr("Add a new property"), this);
            connect(m_addAttributeButton, &QAbstractButton::clicked, this, [=](const bool /* checked */){
                addAttribute();
            });

            m_removePropertiesButton = createBitmapButton("Remove.png", tr("Remove the selected properties"), this);
            connect(m_removePropertiesButton, &QAbstractButton::clicked, this, [=](const bool /* checked */){
                removeSelectedAttributes();
            });

            m_showDefaultPropertiesCheckBox = new QCheckBox(tr("Show default properties"));
            connect(m_showDefaultPropertiesCheckBox, &QCheckBox::stateChanged, this, [=](const int state){
                m_model->setShowDefaultRows(state == Qt::Checked);
            });
            m_showDefaultPropertiesCheckBox->setChecked(m_model->showDefaultRows());

            connect(m_table->selectionModel(), &QItemSelectionModel::currentChanged, this, [=](const QModelIndex& current, const QModelIndex& previous){
                qDebug() << "current changed form " << previous << " to " << current;
                emit selectedRow();
            });

            // Shortcuts

            auto* toolBar = createMiniToolBarLayout(
                m_addAttributeButton,
                m_removePropertiesButton,
                LayoutConstants::WideHMargin,
                m_showDefaultPropertiesCheckBox);

            auto* layout = new QVBoxLayout();
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);
            layout->addWidget(m_table, 1);
            layout->addWidget(new BorderLine(BorderLine::Direction_Horizontal), 0);
            layout->addLayout(toolBar, 0);
            setLayout(layout);

            m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked | QAbstractItemView::AnyKeyPressed);
        }

        void EntityAttributeGrid::createShortcuts() {
            m_insertRowShortcut = new QShortcut(QKeySequence("Ctrl-Return"), this);
            m_insertRowShortcut->setContext(Qt::WidgetWithChildrenShortcut);
            connect(m_insertRowShortcut, &QShortcut::activated, this, [=](){
                addAttribute();
            });

            m_removeRowShortcut = new QShortcut(QKeySequence("Delete"), this);
            m_removeRowShortcut->setContext(Qt::WidgetWithChildrenShortcut);
            connect(m_removeRowShortcut, &QShortcut::activated, this, [=](){
                removeSelectedAttributes();
            });

            m_removeRowAlternateShortcut = new QShortcut(QKeySequence("Backspace"), this);
            m_removeRowAlternateShortcut->setContext(Qt::WidgetWithChildrenShortcut);
            connect(m_removeRowAlternateShortcut, &QShortcut::activated, this, [=](){
                removeSelectedAttributes();
            });
       }

       void EntityAttributeGrid::updateShortcuts() {
           m_insertRowShortcut->setEnabled(true);
           m_removeRowShortcut->setEnabled(canRemoveSelectedAttributes());
           m_removeRowAlternateShortcut->setEnabled(canRemoveSelectedAttributes());
        }

        void EntityAttributeGrid::bindObservers() {
            MapDocumentSPtr document = lock(m_document);
            document->documentWasNewedNotifier.addObserver(this, &EntityAttributeGrid::documentWasNewed);
            document->documentWasLoadedNotifier.addObserver(this, &EntityAttributeGrid::documentWasLoaded);
            document->nodesDidChangeNotifier.addObserver(this, &EntityAttributeGrid::nodesDidChange);
            document->selectionWillChangeNotifier.addObserver(this, &EntityAttributeGrid::selectionWillChange);
            document->selectionDidChangeNotifier.addObserver(this, &EntityAttributeGrid::selectionDidChange);
        }

        void EntityAttributeGrid::unbindObservers() {
            if (!expired(m_document)) {
                MapDocumentSPtr document = lock(m_document);
                document->documentWasNewedNotifier.removeObserver(this, &EntityAttributeGrid::documentWasNewed);
                document->documentWasLoadedNotifier.removeObserver(this, &EntityAttributeGrid::documentWasLoaded);
                document->nodesDidChangeNotifier.removeObserver(this, &EntityAttributeGrid::nodesDidChange);
                document->selectionWillChangeNotifier.removeObserver(this, &EntityAttributeGrid::selectionWillChange);
                document->selectionDidChangeNotifier.removeObserver(this, &EntityAttributeGrid::selectionDidChange);
            }
        }

        void EntityAttributeGrid::documentWasNewed(MapDocument*) {
            updateControls();
        }

        void EntityAttributeGrid::documentWasLoaded(MapDocument*) {
            updateControls();
        }

        void EntityAttributeGrid::nodesDidChange(const Model::NodeList&) {
            updateControls();
        }

        void EntityAttributeGrid::selectionWillChange() {
        }

        void EntityAttributeGrid::selectionDidChange(const Selection&) {
            updateControls();
        }

        void EntityAttributeGrid::updateControls() {
            // When you change the selected entity in the map, there's a brief intermediate state where worldspawn
            // is selected. If we call this directly, it'll cause the table to be rebuilt based on that intermediate
            // state. Everything is fine except you lose the selected row in the table, unless it's a key
            // name that exists in worldspawn. To avoid that problem, make a delayed call to update the table.
            QMetaObject::invokeMethod(m_model, "updateFromMapDocument", Qt::QueuedConnection);

            // Update buttons/checkboxes
            MapDocumentSPtr document = lock(m_document);
            const auto nodes = document->allSelectedAttributableNodes();
            m_table->setEnabled(!nodes.empty());
            m_addAttributeButton->setEnabled(!nodes.empty());
            m_removePropertiesButton->setEnabled(!nodes.empty() && canRemoveSelectedAttributes());
            m_showDefaultPropertiesCheckBox->setChecked(m_model->showDefaultRows());

            // Update shortcuts
            updateShortcuts();
        }

        Model::AttributeName EntityAttributeGrid::selectedRowName() const {
            QModelIndex current = m_proxyModel->mapToSource(m_table->currentIndex());
            const AttributeRow* rowModel = m_model->dataForModelIndex(current);
            if (rowModel == nullptr) {
                return "";
            }

            return rowModel->name();
        }
    }
}
