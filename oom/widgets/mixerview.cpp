//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include <QtGui>
#include "mixerview.h"
#include "globals.h"
#include "song.h"


MixerView::MixerView(QWidget* parent)
: TrackViewDock(parent)
{
	//m_tracklist = song->visibletracks();
	//m_icons << QIcon(":/images/icons/views_inputs.png") << QIcon(":/images/icons/views_outputs.png") << QIcon(":/images/icons/views_busses.png") << QIcon(":/images/icons/views_auxs.png");
	toggleButtons(false);
	populateTable(-1, true);
	//connect(song, SIGNAL(songChanged(int)), this, SLOT(populateTable(int)));
}

MixerView::~MixerView()
{
}

void MixerView::addButton(QWidget* btn)
{
	horizontalLayout->insertWidget(0, btn);
}

void MixerView::populateTable(int flag, bool startup)/*{{{*/
{
	if(flag & (SC_VIEW_CHANGED | SC_VIEW_DELETED | SC_VIEW_ADDED) || flag == -1)
	{
		_tableModel->clear();
		QList<qint64> *idlist = song->trackViewIndexList();
		TrackViewList* tvlist = song->trackviews();
		for(int i = 0; i < idlist->size(); ++i)
		{
			qint64 tvid = idlist->at(i);
			TrackView* view = tvlist->value(tvid);
			if(view)
			{
				QList<QStandardItem*> rowData;
				QStandardItem *chk = new QStandardItem(true);
				chk->setCheckable(true);
				if(startup)
				{
					chk->setCheckState(view->selected() ? Qt::Checked : Qt::Unchecked);
					if(view->selected())
						m_selectList.append(view->id());
				}
				else
				{
					chk->setCheckState(m_selectList.contains(view->id()) ? Qt::Checked : Qt::Unchecked);
				}
				QStandardItem *tname = new QStandardItem(view->viewName());
				tname->setData(view->id());
				rowData.append(chk);
				rowData.append(tname);
				_tableModel->blockSignals(true);
				_tableModel->insertRow(_tableModel->rowCount(), rowData);
				_tableModel->blockSignals(false);
				tableView->setRowHeight(_tableModel->rowCount(), 25);
			}
		}
		_autoTableModel->clear();
		int icon_index = 0;
		QList<int> list;
		list << Track::MIDI << Track::AUDIO_INPUT << Track::AUDIO_OUTPUT << Track::AUDIO_BUSS << Track::AUDIO_AUX << Track::WAVE;
		idlist = song->autoTrackViewIndexList();
		tvlist = song->autoviews();
		for(int i = 0; i < idlist->size(); ++i)
		{
			qint64 tvid = idlist->at(i);
			TrackView* view = tvlist->value(tvid);
			if(view)
			{
				QList<QStandardItem*> rowData2;
				QStandardItem *chk = new QStandardItem(true);
				chk->setCheckable(true);
				if(startup)
				{
					chk->setCheckState(view->selected() ? Qt::Checked : Qt::Unchecked);
					if(view->selected())
						m_selectList.append(view->id());
				}
				else
				{
					chk->setCheckState(m_selectList.contains(view->id()) ? Qt::Checked : Qt::Unchecked);
				}
				QStandardItem *tname = new QStandardItem(view->viewName());
				tname->setData(view->id());
				if(view->viewName() != "Working View" && view->viewName() != "Comment View")
				{
					chk->setForeground(QBrush(QColor(g_trackColorListSelected.value(list.at(i)))));
					tname->setIcon(m_icons.at(icon_index));
					++icon_index;
				}
				rowData2.append(chk);
				rowData2.append(tname);
				_autoTableModel->blockSignals(true);
				_autoTableModel->insertRow(_autoTableModel->rowCount(), rowData2);
				_autoTableModel->blockSignals(false);
				autoTable->setRowHeight(_autoTableModel->rowCount(), 25);
			}
		}
		updateTrackList();
		updateTableHeader();
		tableView->resizeRowsToContents();
		autoTable->resizeRowsToContents();
	}
}/*}}}*/

void MixerView::trackviewChanged(QStandardItem *item)/*{{{*/
{
	if(item)
	{
		int row = item->row();
		QStandardItem *tname = _tableModel->item(row, 1);
		QStandardItem *chk = _tableModel->item(row, 0);
		if(tname)
		{
			TrackView* tv = song->findTrackViewById(tname->data().toLongLong());
			if(tv)
			{
				if(chk->checkState() == Qt::Checked)
				{
					if(!m_selectList.contains(tv->id()))
						m_selectList.append(tv->id());
				}
				else
				{
					if(m_selectList.contains(tv->id()))
						m_selectList.removeAt(m_selectList.indexOf(tv->id()));
				}
				updateTrackList();
			}
		}
	}
}/*}}}*/

void MixerView::autoTrackviewChanged(QStandardItem *item)/*{{{*/
{
	if(item)
	{
		int row = item->row();
		QStandardItem *tname = _autoTableModel->item(row, 1);
		QStandardItem *chk = _autoTableModel->item(row, 0);
		if(tname)
		{
			TrackView* tv = song->findAutoTrackView(tname->text());
			if(tv)
			{
				//printf("MixerView::autoTrackviewChanged: %s\n", tname->text().toLatin1().constData());
				if(chk->checkState() == Qt::Checked)
				{
					if(!m_selectList.contains(tv->id()))
						m_selectList.append(tv->id());
				}
				else
				{
					if(m_selectList.contains(tv->id()))
						m_selectList.removeAt(m_selectList.indexOf(tv->id()));
				}
				updateTrackList();
			}
		}
	}
}/*}}}*/

void MixerView::updateTrackList()/*{{{*/
{
	//printf("Song::updateTrackViews1()\n");
	m_tracklist.clear();
	//m_tracklist = new TrackList();
	bool viewselected = false;
	bool customview = false;
	bool workview = false;
	bool commentview = false;
	
	QStandardItem *witem = _autoTableModel->item(0);
	QStandardItem *citem = _autoTableModel->item(5);
	//TrackView* wv = song->findAutoTrackView("Working View");
	//TrackView* cv = song->findAutoTrackView("Comment View");
	//if(wv && wv->selected())
	if(witem && witem->checkState() == Qt::Checked)
	{
		workview = true;
		viewselected = true;
	}
	//if(cv && cv->selected())
	if(citem && citem->checkState() == Qt::Checked)
	{
		commentview = true;
		viewselected = true;
	}
	QList<qint64> *tidlist = song->trackViewIndexList();
	for(int i = 0; i < tidlist->size(); ++i)
	{
		TrackView *view = song->findTrackViewById(tidlist->at(i));
		if(view)
		{
			if(m_selectList.contains(view->id()))
			{
				QList<qint64> *tlist = view->tracks();
				for(int t = 0; t < tlist->size(); ++t)
				{
					Track *track = song->findTrackById(tlist->at(t));
					if(track)
					{
						bool found = false;
						if(workview && track->parts()->empty()) {
							continue;
						}
						for (ciTrack i = m_tracklist.begin(); i != m_tracklist.end(); ++i)
						{
							if ((*i)->id() == track->id())
							{
								found = true;
								break;
							}
						}
						if(!found && track->name() != "Master")
						{
							m_tracklist.push_back(track);
							customview = true;
							viewselected = true;
						}
					}
				}
			}
		}
	}
	tidlist = song->autoTrackViewIndexList();
	for(int i = 0; i < tidlist->size(); ++i)
	{
		TrackView *view = song->findAutoTrackViewById(tidlist->at(i));
		if(view)
		{
			if(customview && view->id() == song->workingViewId())
			{
				continue;
			}
			if(m_selectList.contains(view->id()))/*{{{*/
			{
				TrackList* tl = song->tracks();
				for(ciTrack t = tl->begin(); t != tl->end(); ++t)
				{
					bool found = false;
					for (ciTrack i = m_tracklist.begin(); i != m_tracklist.end(); ++i)
					{
						if ((*i)->id() == (*t)->id())
						{
							found = true;
							break;
						}
					}
					if(!found)
					{
						viewselected = true;
						switch((*t)->type())/*{{{*/
						{
							case Track::MIDI:
							case Track::DRUM:
							case Track::AUDIO_SOFTSYNTH:
							case Track::WAVE:
								if(view->id() == song->workingViewId())
								{
									if((*t)->parts()->empty())
										break;
									m_tracklist.push_back((*t));
								}
								else if(view->id() == song->commentViewId())
								{
									if((*t)->comment().isEmpty())
										break;
									m_tracklist.push_back((*t));
								}
								break;
							case Track::AUDIO_OUTPUT:
								if(view->id() == song->outputViewId() && (*t)->id() != song->masterId())
								{
									m_tracklist.push_back((*t));
								}
								else if(view->id() == song->commentViewId() && (*t)->id() != song->masterId())
								{
									if((*t)->comment().isEmpty())
										break;
									m_tracklist.push_back((*t));
								}
								break;
							case Track::AUDIO_BUSS:
								if(view->id() == song->bussViewId())
								{
									m_tracklist.push_back((*t));
								}
								else if(view->id() == song->commentViewId())
								{
									if((*t)->comment().isEmpty())
										break;
									m_tracklist.push_back((*t));
								}
								break;
							case Track::AUDIO_AUX:
								if(view->id() == song->auxViewId())
								{
									m_tracklist.push_back((*t));
								}
								else if(view->id() == song->commentViewId())
								{
									if((*t)->comment().isEmpty())
										break;
									m_tracklist.push_back((*t));
								}
								break;
							case Track::AUDIO_INPUT:
								if(view->id() == song->inputViewId())
								{
									m_tracklist.push_back((*t));
								}
								else if(view->id() == song->commentViewId())
								{
									if((*t)->comment().isEmpty())
										break;
									m_tracklist.push_back((*t));
								}
								break;
							default:
								fprintf(stderr, "unknown track type %d\n", (*t)->type());
								return;
						}/*}}}*/
					}
				}
			}/*}}}*/
		}
	}
	if(!viewselected)
	{
		//Make the viewtracks the artracks
		for(ciTrack it = song->artracks()->begin(); it != song->artracks()->end(); ++it)
		{
			if((*it)->id() != song->masterId())
				m_tracklist.push_back((*it));
		}
	}
	emit trackListChanged(&m_tracklist);
}/*}}}*/
