#include "dataselectionscreen.h"
#include <QCheckBox>
#include <QVBoxLayout>
#include <logging.h>
#include <GraphTreeWidgetItem.h>
DataSelectionScreen::DataSelectionScreen(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
    connect(ui.treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(onItemChanged(QTreeWidgetItem*,int)));
    connect(ui.clearPushButton,SIGNAL(clicked()),this,SLOT(clearSelectionButtonClicked()));
    ui.treeWidget->setHeaderHidden(true);
}

DataSelectionScreen::~DataSelectionScreen()
{
}

void DataSelectionScreen::clearSelectionButtonClicked()
{
    QList<QTreeWidgetItem*> items = ui.treeWidget->findItems("",Qt::MatchContains | Qt::MatchRecursive);
    for (int i=0;i<items.size();i++)
    {
        if (items[i]->parent())
        {
            if (items[i]->checkState(0) == Qt::Checked)
            {
                items[i]->setCheckState(0,Qt::Unchecked);
                // ^^ this will trigger the disabling of the graph automatically
            }
        }
    }
    m_enabledList.clear();
}

void DataSelectionScreen::enableItem(QString name)
{
    QString first = name.split(".")[0];
    QString second = name.split(".")[1];
    QList<QTreeWidgetItem*> items = ui.treeWidget->findItems(second,Qt::MatchExactly | Qt::MatchRecursive,0);
    if (items.size() == 0)
    {
        return;
    }
    for (int i=0;i<items.size();i++)
    {
        if (items[i]->parent())
        {
            if (items[i]->parent()->text(0) == first)
            {
                if (items[i]->checkState(0) != Qt::Checked)
                {
                    items[i]->setCheckState(0,Qt::Checked);
                    ui.treeWidget->scrollToItem(items[i]);
                }
                return;
            }
            else
            {
                QLOG_DEBUG() << "Not found:" << items[i]->parent()->text(0);
            }
        }
    }

}

void DataSelectionScreen::disableItem(QString name)
{
    QString first = name.split(".")[0];
    QString second = name.split(".")[1];
    QList<QTreeWidgetItem*> items = ui.treeWidget->findItems(second,Qt::MatchExactly | Qt::MatchRecursive,0);
    if (items.size() == 0)
    {
        return;
    }
    for (int i=0;i<items.size();i++)
    {
        //If the item has no parent, it's a top level item and we ignore it anyway.
        if (items[i]->parent())
        {
            if (items[i]->parent()->text(0) == first)
            {
                if (items[i]->checkState(0) != Qt::Unchecked)
                {
                    items[i]->setCheckState(0,Qt::Unchecked);
                    return;
                }
            }
        }
    }
    QLOG_ERROR() << "No item found in DataSelectionScreen:disableItem:" << name;
}

QList<QString> DataSelectionScreen::disableAllItems()
{
    // Store enabled List for returning
    QList<QString> tempList(m_enabledList);
    // Clear all selected items - its like perssing the clear button
    clearSelectionButtonClicked();
    return tempList;
}

void DataSelectionScreen::enableItemList(QList<QString> &itemList)
{
    foreach (QString item, itemList)
    {
        enableItem(item);
    }
}

void DataSelectionScreen::addItem(QString name)
{
    if (name.contains(":"))
    {
        QString sysid = name.mid(0,name.indexOf(":"));
        name = name.mid(name.indexOf(":")+1);
        m_nameToSysId[name] = sysid;
    }
    if (name.contains("."))
    {
        //Splitting name a every '.'
        QStringList parts = name.split('.');
        QString groupname;
        QString shortname;

        if(parts.size() == 2)
        {
            // name format is like "ATTITUDE.Pitch" -> groupname = ATTITUDE, shortname = Pitch
            groupname = parts[0];
            shortname = parts[1];
        }
        else if(parts.size() == 3)
        {
            // name format is like "BATTERY_STATUS.voltages.0" -> groupname = BATTERY_STATUS, shortname = voltages.0
            groupname = parts[0];
            shortname = parts[1] + "." + parts[2];
        }
        else
        {
            QLOG_INFO() << "Name schema mismatch dropping item.";
            return;
        }

        QList<QTreeWidgetItem*> findlist = ui.treeWidget->findItems(groupname, Qt::MatchContains);
        if (findlist.size() > 0)
        {
            GraphTreeWidgetItem *child = new GraphTreeWidgetItem(QStringList() << shortname);
            child->setFlags(child->flags() | Qt::ItemIsUserCheckable);
            child->setCheckState(0,Qt::Unchecked);
            findlist[0]->addChild(child);
        }
        else
        {
            GraphTreeWidgetItem *item = new GraphTreeWidgetItem(QStringList() << groupname);
            ui.treeWidget->addTopLevelItem(item);
            GraphTreeWidgetItem *child = new GraphTreeWidgetItem(QStringList() << shortname);

            child->setFlags(child->flags() | Qt::ItemIsUserCheckable);
            child->setCheckState(0,Qt::Unchecked);
            item->addChild(child);
        }
    }
    ui.treeWidget->sortByColumn(0,Qt::AscendingOrder);
}
void DataSelectionScreen::onItemChanged(QTreeWidgetItem* item,int column)
{
    Q_UNUSED(column)
    if (!item->parent())
    {
        return;
    }
    QString name = item->parent()->text(0) + "." + item->text(0);
    if (item->checkState(0) == Qt::Checked)
    {
        if (!m_enabledList.contains(name))
        {
            m_enabledList.append(name);
            emit itemEnabled(name);
        }
    }
    else
    {
        if (m_enabledList.contains(name))
        {
            m_enabledList.removeOne(name);
            emit itemDisabled(name);
        }
    }
}

void DataSelectionScreen::clear()
{
    ui.treeWidget->clear();
    m_enabledList.clear();
}
