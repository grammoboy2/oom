//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#include "instrumenttree.h"
#include "globals.h"
#include "config.h"
#include "gconfig.h"
#include "midiport.h"
#include "minstrument.h"
#include "mididev.h"
#include "song.h"
#include <QtGui>

InstrumentTree::InstrumentTree(QWidget* parent, MidiTrack* t, bool popup) : QTreeView(parent)
{
	m_track = t;
	m_popup = popup;
	_patchModel = new QStandardItemModel(0, 2, this);
	_patchSelModel = new QItemSelectionModel(_patchModel);
	setExpandsOnDoubleClick(true);
	setModel(_patchModel);
	connect(this, SIGNAL( doubleClicked(const QModelIndex&) ), this, SLOT(patchDoubleClicked(const QModelIndex&) ) );
	connect(this, SIGNAL( clicked(const QModelIndex&) ), this, SLOT(patchClicked(const QModelIndex&) ) );
	if(popup)
	{
		setWindowFlags(Qt::SplashScreen | Qt::WindowStaysOnTopHint);
	}
	updateModel();
}

void InstrumentTree::updateModel()
{
	if(!m_track)
	{
		_patchModel->clear();
		return;
	}
	//printf("InstrumentTree::updateModel() trackName: %s\n",m_track->name().toUtf8().constData());
	int channel = m_track->outChannel();
	int port = m_track->outPort();
	MidiInstrument* instr = midiPorts[port].instrument();
	if(instr)
		instr->populatePatchModel(_patchModel, channel, song->mtype(), m_track->type() == Track::DRUM);
	updateHeader();
}

void InstrumentTree::updateHeader()/*{{{*/
{
	//update the patchList headers as well
	QStandardItem* pid = new QStandardItem("");
	QStandardItem* patch = new QStandardItem(tr("Select Patch"));
	patch->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	_patchModel->setHorizontalHeaderItem(0, patch);
	_patchModel->setHorizontalHeaderItem(1, pid);
	setColumnHidden(1, true);
}/*}}}*/

void InstrumentTree::patchDoubleClicked(QModelIndex index)/*{{{*/
{
	if(!m_track)
		return;
	QStandardItem* nItem = _patchModel->itemFromIndex(index);

	if(nItem->hasChildren()) //Its a folder perform expand collapse
	{
		setExpanded(index, !isExpanded(index));
	}
	else
	{
		int row = nItem->row();
		QStandardItem* p = nItem->parent();
		QStandardItem *idItem;
		QString pg = "";
		if(p && p != _patchModel->invisibleRootItem() && p->columnCount() == 2)
		{
			//We are in group mode
			idItem = p->child(row, 1);
			pg = p->text();
		}
		else
		{
			idItem = _patchModel->item(row, 1);
		}
		int id = idItem->text().toInt();
		QString name = nItem->text();
		//printf("Found patch Name: %s - ID: %d\n",name.toUtf8().constData(), id);

		if (!name.isEmpty() && id >= 0)
		{
			emit patchSelected(id, name);
			if(m_popup)
			{
				hide();
			}
		}
	}
}/*}}}*/

void InstrumentTree::patchClicked(QModelIndex index)/*{{{*/
{
	if(!m_track)
		return;
	QStandardItem* nItem = _patchModel->itemFromIndex(index);

	if(!nItem->hasChildren())
	{
		int row = nItem->row();
		QStandardItem* p = nItem->parent();
		QStandardItem *idItem;
		QString pg = "";
		if(p && p != _patchModel->invisibleRootItem() && p->columnCount() == 2)
		{
			//We are in group mode
			idItem = p->child(row, 1);
			pg = p->text();
		}
		else
		{
			idItem = _patchModel->item(row, 1);
		}
		int id = idItem->text().toInt();
		QString name = nItem->text();
		//printf("Found patch Name: %s - ID: %d\n",name.toUtf8().constData(), id);

		if (!name.isEmpty() && id >= 0)
		{
			emit patchSelected(id, name);
			if(m_popup)
			{
				hide();
			}
		}
	}
}/*}}}*/

void InstrumentTree::focusOutEvent(QFocusEvent*)
{
	if(m_popup)
	{
		emit treeFocusLost();
		hide();
	}
}

QSize InstrumentTree::sizeHint()
{
	return QSize(200, 500);
}
