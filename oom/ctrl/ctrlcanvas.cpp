//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: ctrlcanvas.cpp,v 1.15.2.10 2009/11/14 03:37:48 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <values.h>

#include <QPainter>
#include <QCursor>
#include <QMouseEvent>
#include <QtGui>

#include "globals.h"
#include "ctrledit.h"
#include "midieditor.h"
#include "icons.h"
#include "midiport.h"
#include "song.h"
#include "midictrl.h"
#include "audio.h"
#include "gconfig.h"
#include "ctrlpanel.h"
#include "midiedit/drummap.h"

extern void drawTickRaster(QPainter& p, int x, int y,
		int w, int h, int quant);

static MidiCtrlValList veloList(CTRL_VELOCITY); // dummy

//---------------------------------------------------------
//   computeVal
//---------------------------------------------------------

static int computeVal(MidiController* mc, int y, int height)
{
	int min;
	int max;
	if (mc->num() == CTRL_PROGRAM)
	{
		min = 1;
		max = 128;
	}
	else
	{
		min = mc->minVal();
		max = mc->maxVal();
	}
	int val = max - (y * (max - min) / height);
	if (val < min)
		val = min;
	if (val > max)
		val = max;
	if (mc->num() != CTRL_PROGRAM)
		val += mc->bias();
	return val;
}

//---------------------------------------------------------
//   CEvent
//---------------------------------------------------------

CEvent::CEvent(Event e, MidiPart* pt, int v, bool sel)
{
	_event = e;
	_part = pt;
	_val = v;
	ex = !e.empty() ? e.tick() : 0;
	_selected = sel;
}

//---------------------------------------------------------
//   contains
//---------------------------------------------------------

bool CEvent::contains(int x1, int x2) const
{
	int tick1 = !_event.empty() ? _event.tick() + _part->tick() : 0;
	if (ex == -1)
		return (tick1 < x2);

	int tick2 = ex + _part->tick();
	return ((tick1 >= x1 && tick1 < x2)
			//|| (tick2 >= x1 && tick2 < x2)
			|| (tick2 > x1 && tick2 < x2)
			|| (tick1 < x1 && tick2 >= x2));
}

//---------------------------------------------------------
//   clearDelete
//---------------------------------------------------------

void CEventList::clearDelete()
{
	for (ciCEvent i = begin(); i != end(); ++i)
	{
		CEvent* ce = *i;
		if (ce)
			delete ce;
	}
	clear();
}

//---------------------------------------------------------
//   CtrlCanvas
//---------------------------------------------------------

CtrlCanvas::CtrlCanvas(MidiEditor* e, QWidget* parent, int xmag,
		const char* name, CtrlPanel* pnl) : View(parent, xmag, 1, name)
{
	setBg(QColor(195, 198, 196));

	editor = e;
	drag = DRAG_OFF;
	tool = PointerTool;
	pos[0] = 0;
	pos[1] = 0;
	pos[2] = 0;
	noEvents = false;

	ctrl = &veloList;
	_controller = &veloCtrl;
	_panel = pnl;
	_cnum = CTRL_VELOCITY;
	_dnum = CTRL_VELOCITY;
	_didx = CTRL_VELOCITY;
	connect(song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(setPos(int, unsigned, bool)));

	setMouseTracking(true);
	if (editor->parts()->empty())
	{
		curPart = 0;
		curTrack = 0;
	}
	else
	{
		setCurTrackAndPart();
	}
	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));

	curDrumInstrument = editor->curDrumInstrument();
	//printf("CtrlCanvas::CtrlCanvas curDrumInstrument:%d\n", curDrumInstrument);

	connect(editor, SIGNAL(curDrumInstrumentChanged(int)), SLOT(setCurDrumInstrument(int)));
	updateItems();
}

//---------------------------------------------------------
//   setPos
//    set one of three markers
//    idx   - 0-cpos  1-lpos  2-rpos
//    flag  - emit followEvent()
//---------------------------------------------------------

void CtrlCanvas::setPos(int idx, unsigned val, bool adjustScrollbar)
{
	if (pos[idx] == val)
		return;

	int opos = mapx(pos[idx]);
	int npos = mapx(val);

	if (adjustScrollbar && idx == 0)
	{
		switch (song->follow())
		{
			case Song::NO:
				break;
			case Song::JUMP:
				if (npos >= width())
				{
					int ppos = val - rmapxDev(width() / 4);
					if (ppos < 0)
						ppos = 0;
					emit followEvent(ppos);
					opos = mapx(pos[idx]);
					npos = mapx(val);
				}
				else if (npos < 0)
				{
					int ppos = val - rmapxDev(width()*3 / 4);
					if (ppos < 0)
						ppos = 0;
					emit followEvent(ppos);
					opos = mapx(pos[idx]);
					npos = mapx(val);
				}
				break;
			case Song::CONTINUOUS:
				if (npos > (width()*5) / 8)
				{
					int ppos = pos[idx] - rmapxDev(width()*5 / 8);
					if (ppos < 0)
						ppos = 0;
					emit followEvent(ppos);
					opos = mapx(pos[idx]);
					npos = mapx(val);
				}
				else if (npos < (width()*3) / 8)
				{
					int ppos = pos[idx] - rmapxDev(width()*3 / 8);
					if (ppos < 0)
						ppos = 0;
					emit followEvent(ppos);
					opos = mapx(pos[idx]);
					npos = mapx(val);
				}
				break;
		}
	}

	int x;
	int w = 1;
	if (opos > npos)
	{
		w += opos - npos;
		x = npos;
	}
	else
	{
		w += npos - opos;
		x = opos;
	}
        pos[idx] = val;

        // FIXME: redrawing this rectangle wipes out the controller lane
        // items when cursor passes over them.
        // Redrawing the whole view fixes that for now, but it could be a cpu hog ?
//	redraw(QRect(x, 0, w, height()));
        update();
}

//---------------------------------------------------------
//   setMidiController
//---------------------------------------------------------

void CtrlCanvas::setMidiController(int num)
{
	_cnum = num;
	partControllers(curPart, _cnum, &_dnum, &_didx, &_controller, &ctrl);
	if (_panel)
	{
		if (_cnum == CTRL_VELOCITY)
			_panel->setHWController(curTrack, &veloCtrl);
		else
			_panel->setHWController(curTrack, _controller);
	}
}

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void CtrlCanvas::leaveEvent(QEvent*)
{
	emit xposChanged(MAXINT);
	emit yposChanged(-1);
}

//---------------------------------------------------------
//   raster
//---------------------------------------------------------

QPoint CtrlCanvas::raster(const QPoint& p) const
{
	return p;
}

//---------------------------------------------------------
//   deselectAll
//---------------------------------------------------------

void CtrlCanvas::deselectAll()
{
	//    for (iCEvent i = selection.begin(); i != selection.end(); ++i)
	//            (*i)->setState(CEvent::Normal);
	//      selection.clear();
	//      update();
}

//---------------------------------------------------------
//   selectItem
//---------------------------------------------------------

void CtrlCanvas::selectItem(CEvent*)
{
	//      e->setState(CEvent::Selected);
	//      selection.push_back(e);
	//      update();
}

//---------------------------------------------------------
//   deselectItem
//---------------------------------------------------------

void CtrlCanvas::deselectItem(CEvent*)
{
	/*      e->setState(CEvent::Normal);
		  for (iCEvent i = selection.begin(); i != selection.end(); ++i) {
				if (*i == e) {
					  selection.erase(i);
					  break;
					  }
				}
		  update();
	 */
}

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

void CtrlCanvas::setController(int num)
{
	setMidiController(num);
	updateItems();
}


//---------------------------------------------------------
//   setCurTrackAndPart
//---------------------------------------------------------

bool CtrlCanvas::setCurTrackAndPart()
{
	bool changed = false;
	MidiPart* part = 0;
	MidiTrack* track = 0;

	if (!editor->parts()->empty())
	{
		Part* pt = editor->curCanvasPart();
		if (pt && pt->track())
		{
			if (pt->track()->isMidiTrack())
			{
				part = (MidiPart*) pt;
				track = part->track();
			}
		}
	}

	if (part != curPart)
	{
		curPart = part;
		changed = true;
	}

	if (track != curTrack)
	{
		curTrack = track;
		changed = true;
	}

	return changed;
}

//---------------------------------------------------------
//   songChanged
//    all marked parts are added to the internal event list
//---------------------------------------------------------

void CtrlCanvas::songChanged(int type)
{
	// Is it simply a midi controller value adjustment? Forget it.
	if (type == SC_MIDI_CONTROLLER)
		return;

	bool changed = false;
	if (type & (SC_CONFIG | SC_PART_MODIFIED | SC_SELECTION))
		changed = setCurTrackAndPart();

	// Although changing the instrument/device in the
	//  config window generates a type of -1, we can eliminate
	//  some other useless calls using SC_CONFIG, which was not used
	//  anywhere else in oom before now, except song header.
	if ((type & (SC_CONFIG | SC_DRUMMAP)) || ((type & (SC_PART_MODIFIED | SC_SELECTION)) && changed))
	{
		setMidiController(_cnum);
		//return;
	}

	updateItems();

}

//---------------------------------------------------------
//   partControllers
//---------------------------------------------------------

void CtrlCanvas::partControllers(const MidiPart* part, int num, int* dnum, int* didx, MidiController** mc, MidiCtrlValList** mcvl)
{
	if (num == CTRL_VELOCITY) // special case
	{
		if (mcvl)
			*mcvl = &veloList;
		if (mc)
			*mc = &veloCtrl;
		if (dnum)
			*dnum = num;
		if (didx)
			*didx = num;
	}
	else
	{
		MidiTrack* mt = part->track();
		MidiPort* mp;
		int di;
		int n;

		if ((mt->type() != Track::DRUM) && curDrumInstrument != -1)
			printf("keyfilter != -1 in non drum track?\n");

		if ((mt->type() == Track::DRUM) && (curDrumInstrument != -1) && ((num & 0xff) == 0xff))
		{
			di = (num & ~0xff) | curDrumInstrument;
			n = (num & ~0xff) | drumMap[curDrumInstrument].anote; // construct real controller number
			//num = (num & ~0xff) | curDrumInstrument);  // construct real controller number
			mp = &midiPorts[drumMap[curDrumInstrument].port];
		}
		else
		{
			di = num;
			n = num;
			mp = &midiPorts[mt->outPort()];
		}

		if (dnum)
			*dnum = n;

		if (didx)
			*didx = di;

		if (mc)
			*mc = mp->midiController(n);

		if (mcvl)
		{
			MidiCtrlValList* tmcvl = 0;
			MidiCtrlValListList* cvll = mp->controller();
			for (iMidiCtrlValList i = cvll->begin(); i != cvll->end(); ++i)
			{
				if (i->second->num() == n)
				{
					tmcvl = i->second;
					break;
				}
			}
			*mcvl = tmcvl;

			// Removed by T356.
			// MidiCtrlValList not found is now an acceptable state (for multiple part editing).
			//if (i == cvll->end()) {
			//      printf("CtrlCanvas::setController(0x%x): not found\n", num);
			//      for (i = cvll->begin(); i != cvll->end(); ++i)
			//            printf("  0x%x\n", i->second->num());
			//      return;
			//      }
		}
	}
}

//---------------------------------------------------------
//   updateItems
//---------------------------------------------------------

void CtrlCanvas::updateItems()
{
	items.clearDelete();

	if (!editor->parts()->empty())
	{
		MidiPart* cPart = static_cast<MidiPart*>(editor->curCanvasPart());
		QList<Event> selEvents = editor->getSelectedEvents();
		for (iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p)
		{
			Event last;
			CEvent* lastce = 0;

			MidiPart* part = (MidiPart*) (p->second);
			EventList* el = part->events();
			MidiController* mc;
			MidiCtrlValList* mcvl;
			partControllers(part, _cnum, 0, 0, &mc, &mcvl);
			unsigned len = part->lenTick();
			bool isPart = (cPart && cPart == part);

			for (iEvent i = el->begin(); i != el->end(); ++i)
			{
				Event e = i->second;
				// Added by T356. Do not add events which are past the end of the part.
				if (e.tick() >= len)
					break;
				bool sel = (isPart && (!selEvents.isEmpty() && selEvents.contains(e)));//false;//(isPart && editor->isEventSelected(e));

				if (_cnum == CTRL_VELOCITY && e.type() == Note)
				{
					//printf("CtrlCanvas::updateItems CTRL_VELOCITY curDrumInstrument:%d\n", curDrumInstrument);
					if (curDrumInstrument == -1)
					{
						// This is interesting - it would allow ALL drum note velocities to be shown.
						// But currently the drum list ALWAYS has a selected item so this is not supposed to happen.
						items.add(new CEvent(e, part, e.velo(), sel));
					}
					else if (e.dataA() == curDrumInstrument) //same note
						items.add(new CEvent(e, part, e.velo(), sel));
				}
				else if (e.type() == Controller && e.dataA() == _didx)
				{
					if (mcvl && last.empty())
					{
						lastce = new CEvent(Event(), part, mcvl->value(part->tick()));
						items.add(lastce);
					}
					if (lastce)
						lastce->setEX(e.tick());
					lastce = new CEvent(e, part, e.dataB());
					lastce->setEX(-1);
					items.add(lastce);
					last = e;
				}
			}
			//qApp->processEvents();
		}
	}


	redraw();
}

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void CtrlCanvas::viewMousePressEvent(QMouseEvent* event)
{
	start = event->pos();
	Tool activeTool = tool;
	bool shift = event->modifiers() & Qt::ShiftModifier;

	int xpos = start.x();
	int ypos = start.y();

	MidiController::ControllerType type = midiControllerType(_controller->num());

	switch (activeTool)
	{
		case PointerTool:
			drag = DRAG_LASSO_START;
			break;

		case PencilTool:
			if (shift)
			{
				if (type != MidiController::Velo)
				{
					drag = DRAG_NEW;
					song->startUndo();
					newVal(xpos, xpos, ypos);
				}
			}
			else
			{
				drag = DRAG_RESIZE;
				song->startUndo();
				changeVal(xpos, xpos, ypos);
			}
			break;

		case RubberTool:
			if (type != MidiController::Velo)
			{
				drag = DRAG_DELETE;
				song->startUndo();
				deleteVal(xpos, xpos, ypos);
			}
			break;

		case DrawTool:
			if (drawLineMode)
			{
				line2x = xpos;
				line2y = ypos;
				if (shift)
					newValRamp(line1x, line1y, line2x, line2y);
				else
					changeValRamp(line1x, line1y, line2x, line2y);
				drawLineMode = false;
			}
			else
			{
				line2x = line1x = xpos;
				line2y = line1y = ypos;
				drawLineMode = true;
			}
			redraw();
			break;

		default:
			break;
	}
}

//---------------------------------------------------------
//   newValRamp
//---------------------------------------------------------

void CtrlCanvas::newValRamp(int x1, int y1, int x2, int y2)
{
	int xx1 = editor->rasterVal1(x1);
	int xx2 = editor->rasterVal2(x2);
	int type = _controller->num();

	int raster = editor->raster();
	if (raster == 1) // set reasonable raster
		raster = config.division / 4;

	song->startUndo();

	// delete existing events

	int lastpv = CTRL_VAL_UNKNOWN;
	for (ciCEvent i = items.begin(); i != items.end(); ++i)
	{
		CEvent* ev = *i;
		if (ev->part() != curPart)
			continue;
		Event event = ev->event();
		if (event.empty())
			continue;
		int x = event.tick() + curPart->tick();
		// Added by Tim. p3.3.6
		//printf("CtrlCanvas::newValRamp x:%d xx1:%d xx2:%d len:%d\n", x, xx1, xx2, curPart->lenTick());

		if (x < xx1)
		{
			//      if(event.dataB() != CTRL_VAL_UNKNOWN)
			//        lastpv = event.dataB();
			continue;
		}
		//if (x <= xx1)
		//{
		//      if(type == CTRL_PROGRAM && event.dataB() != CTRL_VAL_UNKNOWN && ((event.dataB() & 0xffffff) != 0xffffff))
		//        lastpv = event.dataB();
		//      if (x < xx1)
		//        continue;
		//}
		if (x >= xx2)
			break;

		// Indicate no undo, and do port controller values and clone parts.
		//audio->msgDeleteEvent(event, ev->part(), false);
		audio->msgDeleteEvent(event, curPart, false, true, true);
	}

	//if(type == CTRL_PROGRAM && lastpv == CTRL_VAL_UNKNOWN)
	if (ctrl)
		lastpv = ctrl->hwVal();

	// insert new events
	for (int x = xx1; x < xx2; x += raster)
	{
		int y = (x2 == x1) ? y1 : (((y2 - y1)*(x - x1)) / (x2 - x1)) + y1;
		int nval = computeVal(_controller, y, height());
		int tick = x - curPart->tick();
		// Do not add events which are past the end of the part.
		if ((unsigned) tick >= curPart->lenTick())
			break;
		Event event(Controller);
		event.setTick(tick);
		event.setA(_didx);
		if (type == CTRL_PROGRAM)
		{
			if (lastpv == CTRL_VAL_UNKNOWN)
			{
				if (song->mtype() == MT_GM)
					event.setB(0xffff00 | (nval - 1));
				else
					event.setB(nval - 1);
			}
			else
				event.setB((lastpv & 0xffff00) | (nval - 1));
		}
		else
			event.setB(nval);

		// Indicate no undo, and do port controller values and clone parts.
		//audio->msgAddEvent(event, curPart, false);
		audio->msgAddEvent(event, curPart, false, true, true);
	}

	song->update(0);
	redraw();
	song->endUndo(SC_EVENT_MODIFIED | SC_EVENT_INSERTED | SC_EVENT_REMOVED);
}

//---------------------------------------------------------
//   changeValRamp
//---------------------------------------------------------

void CtrlCanvas::changeValRamp(int x1, int y1, int x2, int y2)
{
	int h = height();
	bool changed = false;
	int type = _controller->num();
	//int xx1 = editor->rasterVal1(x1);

	song->startUndo();
	for (ciCEvent i = items.begin(); i != items.end(); ++i)
	{
		if ((*i)->contains(x1, x2))
		{
			//if ((*i)->contains(xx1, x2)) {
			CEvent* ev = *i;
			if (ev->part() != curPart)
				continue;
			Event event = ev->event();
			if (event.empty())
				continue;

			//MidiPart* part   = ev->part();
			//int x    = event.tick() + ev->part()->tick();
			int x = event.tick() + curPart->tick();
			int y = (x2 == x1) ? y1 : (((y2 - y1)*(x - x1)) / (x2 - x1)) + y1;
			int nval = computeVal(_controller, y, h);
			if (type == CTRL_PROGRAM)
			{
				if (event.dataB() == CTRL_VAL_UNKNOWN)
				{
					--nval;
					if (song->mtype() == MT_GM)
						nval |= 0xffff00;
				}
				else
					nval = (event.dataB() & 0xffff00) | (nval - 1);
			}

			ev->setVal(nval);

			//MidiController::ControllerType type = midiControllerType(_controller->num());
			//if (type == MidiController::Velo) {
			if (type == CTRL_VELOCITY)
			{
				if ((event.velo() != nval))
				{
					Event newEvent = event.clone();
					newEvent.setVelo(nval);
					// Indicate no undo, and do not do port controller values and clone parts.
					//audio->msgChangeEvent(event, newEvent, part, false);
					audio->msgChangeEvent(event, newEvent, curPart, false, false, false);
					ev->setEvent(newEvent);
					changed = true;
				}
			}
			else
			{
				if (!event.empty())
				{
					if ((event.dataB() != nval))
					{
						Event newEvent = event.clone();
						newEvent.setB(nval);
						// Indicate no undo, and do port controller values and clone parts.
						//audio->msgChangeEvent(event, newEvent, part, false);
						audio->msgChangeEvent(event, newEvent, curPart, false, true, true);
						ev->setEvent(newEvent);
						changed = true;
					}
				}
				else
				{
					//if(!ctrl)
					//{
					//  ctrl =
					//}

					// Removed by T356. Never gets here? A good thing, don't wan't auto-create values.
					//int oval = ctrl->value(0);
					//if (oval != nval) {
					// Changed by T356.
					//ctrl->add(0, nval);
					//      ctrl->add(0, nval, part);
					//      changed = true;
					//      }

				}
			}
		}
	}
	if (changed)
		redraw();
	song->endUndo(SC_EVENT_MODIFIED);
}

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void CtrlCanvas::viewMouseMoveEvent(QMouseEvent* event)
{
	QPoint pos = event->pos();
	QPoint dist = pos - start;
	bool moving = dist.y() >= 3 || dist.y() <= 3 || dist.x() >= 3 || dist.x() <= 3;
	switch (drag)
	{
		case DRAG_LASSO_START:
			if (!moving)
				break;
			drag = DRAG_LASSO;
			// weiter mit DRAG_LASSO:
		case DRAG_LASSO:
			lasso.setRect(start.x(), start.y(), dist.x(), dist.y());
			redraw();
			break;
		case DRAG_RESIZE:
			changeVal(start.x(), pos.x(), pos.y());
			start = pos;
			break;

		case DRAG_NEW:
			newVal(start.x(), pos.x(), pos.y());
			start = pos;
			break;

		case DRAG_DELETE:
			deleteVal(start.x(), pos.x(), pos.y());
			start = pos;
			break;

		default:
			break;
	}
	if (tool == DrawTool && drawLineMode)
	{
		line2x = pos.x();
		line2y = pos.y();
		redraw();
	}
	emit xposChanged(pos.x());


	int val = computeVal(_controller, pos.y(), height());
	emit yposChanged(val);
}

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void CtrlCanvas::viewMouseReleaseEvent(QMouseEvent* event)
{
	bool shift = event->modifiers() & Qt::ShiftModifier;

	switch (drag)
	{
		case DRAG_RESIZE:
		case DRAG_NEW:
		case DRAG_DELETE:
			song->endUndo(SC_EVENT_MODIFIED | SC_EVENT_INSERTED);
			break;

		case DRAG_LASSO_START:
			lasso.setRect(-1, -1, -1, -1);

		case DRAG_LASSO:
			if (!shift)
				deselectAll();
			lasso = lasso.normalized();
			for (iCEvent i = items.begin(); i != items.end(); ++i)
			{
#if 0
				if ((*i)->intersects(lasso))
				{
					if (shift && (*i)->isSelected())
						deselectItem(*i);
					else
						selectItem(*i);
				}
#endif
			}
			drag = DRAG_OFF;
			redraw();
			break;

		default:
			break;
	}
	drag = DRAG_OFF;
}

//---------------------------------------------------------
//   changeVal
//---------------------------------------------------------

void CtrlCanvas::changeVal(int x1, int x2, int y)
{
	bool changed = false;
	int newval = computeVal(_controller, y, height());
	int type = _controller->num();
	//int xx1 = editor->rasterVal1(x1);

	for (ciCEvent i = items.begin(); i != items.end(); ++i)
	{
		if (!(*i)->contains(x1, x2))
			//if (!(*i)->contains(xx1, x2))
			continue;
		CEvent* ev = *i;
		if (ev->part() != curPart)
			continue;
		Event event = ev->event();
		//if(event.tick() >= curPart->lenTick())
		//  break;

		//MidiPart* part   = ev->part();
		//int nval = newval;
		//if(type == CTRL_PROGRAM)
		//{
		//  if(event.dataB() == CTRL_VAL_UNKNOWN)
		//  {
		//    --nval;
		//    if(song->mtype() == MT_GM)
		//      nval |= 0xffff00;
		//  }
		//  else
		//    nval = (event.dataB() & 0xffff00) | (nval - 1);
		//}
		//ev->setVal(nval);

		//MidiController::ControllerType type = midiControllerType(_controller->num());
		//if (type == MidiController::Velo) {
		if (type == CTRL_VELOCITY)
		{
			if ((event.velo() != newval))
			{
				ev->setVal(newval);
				Event newEvent = event.clone();
				newEvent.setVelo(newval);
				// Indicate no undo, and do not do port controller values and clone parts.
				//audio->msgChangeEvent(event, newEvent, part, false);
				audio->msgChangeEvent(event, newEvent, curPart, false, false, false);
				ev->setEvent(newEvent);
				changed = true;
			}
		}
		else
		{
			if (!event.empty())
			{
				int nval = newval;
				if (type == CTRL_PROGRAM)
				{
					if (event.dataB() == CTRL_VAL_UNKNOWN)
					{
						--nval;
						if (song->mtype() == MT_GM)
							nval |= 0xffff00;
					}
					else
						nval = (event.dataB() & 0xffff00) | (nval - 1);
				}
				ev->setVal(nval);

				if ((event.dataB() != nval))
				{
					Event newEvent = event.clone();
					newEvent.setB(nval);
					// Indicate no undo, and do port controller values and clone parts.
					//audio->msgChangeEvent(event, newEvent, part, false);
					audio->msgChangeEvent(event, newEvent, curPart, false, true, true);
					ev->setEvent(newEvent);
					changed = true;
				}
			}
			else
			{
				//if(!ctrl)
				//{
				//  ctrl =
				//}

				// Removed by T356. Never gets here? A good thing, don't wan't auto-create values.
				//int oval = ctrl->value(0);
				//if (oval != nval) {
				// Changed by T356.
				//ctrl->add(0, nval);
				//      ctrl->add(0, nval, part);
				//      changed = true;
				//      }
			}
		}
	}
	if (changed)
		redraw();
}

//---------------------------------------------------------
//   newVal
//---------------------------------------------------------

void CtrlCanvas::newVal(int x1, int x2, int y)
{
	int xx1 = editor->rasterVal1(x1);
	int xx2 = editor->rasterVal2(x2);
	int newval = computeVal(_controller, y, height());
	int type = _controller->num();

	bool found = false;
	bool song_changed = false;

	int lastpv = CTRL_VAL_UNKNOWN;
	if (ctrl)
		lastpv = ctrl->hwVal();

	for (ciCEvent i = items.begin(); i != items.end(); ++i)
	{
		CEvent* ev = *i;
		if (ev->part() != curPart)
			continue;
		//int partTick = ev->part()->tick();
		int partTick = curPart->tick();
		Event event = ev->event();
		if (event.empty())
			continue;
		int ax = event.tick() + partTick;
		// Added by Tim. p3.3.6
		//printf("CtrlCanvas::newVal ax:%d xx1:%d xx2:%d len:%d\n", ax, xx1, xx2, curPart->lenTick());

		if (ax < xx1)
			continue;
		//if(ax <= xx1)
		//{
		//  if(type == CTRL_PROGRAM && event.dataB() != CTRL_VAL_UNKNOWN && ((event.dataB() & 0xffffff) != 0xffffff))
		//    lastpv = event.dataB();
		//  if(ax < xx1)
		//    continue;
		//}
		if (ax >= xx2)
			break;

		// Added by T356. Do not add events which are past the end of the part.
		//if(event.tick() >= curPart->lenTick())
		//  break;

		int nval = newval;
		if (type == CTRL_PROGRAM)
		{
			if (event.dataB() == CTRL_VAL_UNKNOWN)
			{
				//if(lastpv == CTRL_VAL_UNKNOWN)
				//  lastpv = ctrl->hwVal();

				if (lastpv == CTRL_VAL_UNKNOWN)
				{
					--nval;
					if (song->mtype() == MT_GM)
						nval |= 0xffff00;
				}
				else
					nval = (lastpv & 0xffff00) | (nval - 1);
			}
			else
				nval = (event.dataB() & 0xffff00) | (nval - 1);
		}

		if (ax == xx1)
		{
			// change event
			found = true;
			ev->setVal(nval);
			if ((event.dataB() != nval))
			{
				Event newEvent = event.clone();
				newEvent.setB(nval);
				// Added by Tim. p3.3.6
				//printf("CtrlCanvas::newVal change xx1:%d xx2:%d len:%d\n", xx1, xx2, curPart->lenTick());

				// Indicate no undo, and do port controller values and clone parts.
				//audio->msgChangeEvent(event, newEvent, ev->part(), false);
				audio->msgChangeEvent(event, newEvent, curPart, false, true, true);

				ev->setEvent(newEvent);
				song_changed = true;
			}
		}
		else if (ax < xx2)
		{
			// delete event
			// Added by Tim. p3.3.6
			//printf("CtrlCanvas::newVal delete xx1:%d xx2:%d len:%d\n", xx1, xx2, curPart->lenTick());

			// Indicate no undo, and do port controller values and clone parts.
			//audio->msgDeleteEvent(event, ev->part(), false);
			audio->msgDeleteEvent(event, curPart, false, true, true);

			song_changed = true;
		}
	}
	if (!found)
	{
		// new event
		int tick = xx1 - curPart->tick();
		// Do not add events which are past the end of the part.
		if ((unsigned) tick < curPart->lenTick())
		{
			Event event(Controller);
			event.setTick(tick);
			event.setA(_didx);
			if (type == CTRL_PROGRAM)
			{
				if (lastpv == CTRL_VAL_UNKNOWN)
				{
					if (song->mtype() == MT_GM)
						event.setB(0xffff00 | (newval - 1));
					else
						event.setB(newval - 1);
				}
				else
					event.setB((lastpv & 0xffff00) | (newval - 1));
			}
			else
				event.setB(newval);

			// Indicate no undo, and do port controller values and clone parts.
			//audio->msgAddEvent(event, curPart, false);
			audio->msgAddEvent(event, curPart, false, true, true);

			song_changed = true;
		}
	}
	if (song_changed)
	{
		songChanged(0);
		return;
	}
	redraw();
}

//---------------------------------------------------------
//   deleteVal
//---------------------------------------------------------

void CtrlCanvas::deleteVal(int x1, int x2, int)
{
	int xx1 = editor->rasterVal1(x1);
	int xx2 = editor->rasterVal2(x2);

	int partTick = curPart->tick();
	xx1 -= partTick;
	xx2 -= partTick;

	bool song_changed = false;
	for (ciCEvent i = items.begin(); i != items.end(); ++i)
	{
		CEvent* ev = *i;
		if (ev->part() != curPart)
			continue;
		Event event = ev->event();
		if (event.empty())
			continue;
		int x = event.tick();
		if (x < xx1)
			continue;
		if (x >= xx2)
			break;
		if (!event.empty())
		{
			// Indicate no undo, and do port controller values and clone parts.
			//audio->msgDeleteEvent(event, ev->part(), false);
			audio->msgDeleteEvent(event, curPart, false, true, true);
			song_changed = true;
		}
	}
	if (song_changed)
	{
		songChanged(0);
		return;
	}
}

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void CtrlCanvas::setTool(int t)
{
	if (tool == Tool(t))
		return;
	tool = Tool(t);
	switch (tool)
	{
		case PencilTool:
			setCursor(QCursor(*pencilCursorIcon, 6, 15));
			break;
		case DrawTool:
			drawLineMode = false;
			break;
		default:
			setCursor(QCursor(Qt::ArrowCursor));
			break;
	}
}

//---------------------------------------------------------
//   pdrawItems
//---------------------------------------------------------

void CtrlCanvas::pdrawItems(QPainter& p, const QRect& rect, const MidiPart* part, bool velo, bool fg)
{
        // Remon: Do we really have to do that ???? Seems unnesecary to me.
        int x = 0;//rect.x() - 1; // compensate for 3 pixel line width
        int w = rect.width() + 2;
	int wh = height();

	if (velo)
	{
		for (iCEvent i = items.begin(); i != items.end(); ++i)
		{
			CEvent* e = *i;
			// Draw selected part velocity events on top of unselected part events.
			//if((fg && e->part() != part) || (!fg && e->part() == part))
			if (e->part() != part)
				continue;
			int tick = mapx(e->event().tick() + e->part()->tick());
			if (tick <= x)
				continue;
			if (tick > x + w)
				break;
			int y1 = wh - (e->val() * wh / 128);
			// fg means 'draw selected parts'.
			QColor fillColor = QColor(255, 201, 144,255);
			int bgalpha = 127;
			QColor bgfillColor = QColor(255, 201, 144, bgalpha);
			if (fg)
			{
				int velo2 = e->val();
				QColor color = QColor(147, 186, 195, 127);
				if (velo2 <= 11)
				{
					if(e->selected())
					{
						fillColor.setRgb(255, 201, 144, 255);	
						bgfillColor.setRgb(255, 201, 144, bgalpha);	
					}
					color.setRgb(147, 186, 195, 127);
				}
				else if (velo2 <= 22)
				{
					if(e->selected())
					{
						fillColor.setRgb(255, 195, 131, 255);	
						bgfillColor.setRgb(255, 195, 131, bgalpha);	
					}
					color.setRgb(119, 169, 181, 127);
				}
				else if (velo2 <= 33)
				{
					if(e->selected())
					{
						fillColor.setRgb(255, 189, 118, 255);	
						bgfillColor.setRgb(255, 189, 118, bgalpha);	
					}
					color.setRgb(85, 157, 175, 127);
				}
				else if (velo2 <= 44)
				{
					if(e->selected())
					{
						fillColor.setRgb(255, 184, 107, 255);	
						bgfillColor.setRgb(255, 184, 107, bgalpha);	
					}
					color.setRgb(58, 152, 176, 127);
				}
				else if (velo2 <= 55)
				{
					if(e->selected())
					{
						fillColor.setRgb(255, 177, 94, 255);	
						bgfillColor.setRgb(255, 177, 94, bgalpha);	
					}
					color.setRgb(33, 137, 163, 127);
				}
				else if (velo2 <= 66)
				{
					if(e->selected())
					{
						fillColor.setRgb(255, 171, 81, 255);	
						bgfillColor.setRgb(255, 171, 81, bgalpha);	
					}
					color.setRgb(30, 136, 162, 127);
				}
				else if (velo2 <= 77)
				{
					if(e->selected())
					{
						fillColor.setRgb(255, 166, 70, 255);	
						bgfillColor.setRgb(255, 166, 70, bgalpha);	
					}
					color.setRgb(13, 124, 151, 127);
				}
				else if (velo2 <= 88)
				{
					if(e->selected())
					{
						fillColor.setRgb(255, 158, 53, 255);	
						bgfillColor.setRgb(255, 158, 53, bgalpha);	
					}
					color.setRgb(0, 110, 138, 127);
				}
				else if (velo2 <= 99)
				{
					if(e->selected())
					{
						fillColor.setRgb(255, 153, 44, 255);	
						bgfillColor.setRgb(255, 153, 44, bgalpha);	
					}
					color.setRgb(0, 99, 124, 127);
				}
				else if (velo2 <= 110)
				{
					if(e->selected())
					{
						fillColor.setRgb(255, 146, 28, 255);	
						bgfillColor.setRgb(255, 146, 28, bgalpha);	
					}
					color.setRgb(0, 77, 96, 127);
				}
				else if (velo2 <= 121)
				{
					if(e->selected())
					{
						fillColor.setRgb(255, 139, 14, 255);	
						bgfillColor.setRgb(255, 139, 14, bgalpha);	
					}
					color.setRgb(0, 69, 86, 127);
				}
				else
				{
					if(e->selected())
					{
						fillColor.setRgb(255, 132, 0, 255);	
						bgfillColor.setRgb(255, 132, 0, bgalpha);	
					}
					color.setRgb(0, 58, 72, 127);
				}
				p.setPen(QPen(color, 6));
			}
			else
				p.setPen(QPen(QColor(172, 172, 172), 6));

			//int alpha = 255;
			QColor outlineColor = QColor(0,0,0,255);
			QColor bgoutlineColor = QColor(0,0,0,bgalpha);
			//QColor bgfillColor = QColor(224, 123, 23,80);

			//QColor color = QColor();
			//QColor green = QColor(53, 171, 193, 255);
			//QColor red = QColor(169, 72, 107, 255);
			//QLinearGradient vuGrad3(QPointF(0, 0), QPointF(0, height()));
			//vuGrad3.setColorAt(1, green);
			//vuGrad3.setColorAt(0, red);
			//QPen mypen7 = QPen();
			
			if(e->selected())
			{
				QPen mypen6 = QPen(outlineColor, 1, Qt::SolidLine);
				QPen mypen7 = QPen(bgoutlineColor, 1, Qt::SolidLine);
				//p.setBrush(QBrush(bgfillColor));
				//mypen7.setBrush(QBrush(vuGrad3));
				//p.setPen(mypen7);
				//p.setPen(mypen);
				//p.setBrush(QBrush(fillColor));
				p.setPen(mypen7);
				p.setBrush(QBrush(bgfillColor));
				p.drawRect(tick+1, y1-3, 5, wh);
				p.setPen(mypen6);
				p.setBrush(QBrush(fillColor));
				QRect rect(tick+1, y1-3, 5, 5);
				//p.drawRoundedRect(rect,3,3);
				p.drawRect(rect);
			}
			else
			{
				p.drawLine(tick + 4, wh, tick + 4, y1);
			}
		}
	}
	else
	{
		MidiTrack* mt = part->track();
		MidiPort* mp;

		if ((mt->type() == Track::DRUM) && (curDrumInstrument != -1) && ((_cnum & 0xff) == 0xff))
			mp = &midiPorts[drumMap[curDrumInstrument].port];
		else
			mp = &midiPorts[mt->outPort()];

		MidiController* mc = mp->midiController(_cnum);

		int min;
		int max;
		int bias;
		if (_cnum == CTRL_PROGRAM)
		{
			min = 1;
			max = 128;
			bias = 0;
		}
		else
		{
			min = mc->minVal();
			max = mc->maxVal();
			bias = mc->bias();
		}
		int x1 = rect.x();
		int lval = CTRL_VAL_UNKNOWN;
		noEvents = false;
		QColor color = QColor();
		QColor green = QColor(119, 169, 181, 127);
		QColor yellow = QColor(41, 130, 140);
		QColor red = QColor(0, 37, 46, 127);
		QLinearGradient vuGrad(QPointF(0, 0), QPointF(0, height()));
		vuGrad.setColorAt(1, green);
		//vuGrad.setColorAt(0.45, yellow);
		//vuGrad.setColorAt(0.3, yellow);
		vuGrad.setColorAt(0, red);
		QPen myPen = QPen();
		//myPen.setCapStyle(Qt::RoundCap);
		//myPen.setStyle(Qt::DashLine);
		myPen.setBrush(QBrush(vuGrad));
		for (iCEvent i = items.begin(); i != items.end(); ++i)
		{
			CEvent* e = *i;
			// Draw unselected part controller events (lines) on top of selected part events (bars).
			//if((fg && (e->part() == part)) || (!fg && (e->part() != part)))
			if (e->part() != part)
			{
				continue;
			}
			Event ev = e->event();
			int tick = mapx(!ev.empty() ? ev.tick() + e->part()->tick() : 0);
			int val = e->val();
			int pval = val;
			if (_cnum == CTRL_PROGRAM)
			{
				if ((val & 0xff) == 0xff)
					// What to do here? prog = 0xff should not be allowed, but may still be encountered.
					pval = 1;
				else
					pval = (val & 0x7f) + 1;
			}
			if (tick <= x)
			{
				if (val == CTRL_VAL_UNKNOWN)
					lval = CTRL_VAL_UNKNOWN;
				else
				{
					if (_cnum == CTRL_PROGRAM)
						lval = wh - ((pval - min - bias) * wh / (max - min));
					else
						lval = wh - ((val - min - bias) * wh / (max - min));
				}
				continue;
			}
			if (tick > x + w)
				break;
			//int velo2 = e->val();

			if (lval == CTRL_VAL_UNKNOWN)
			{
				// fg means 'draw unselected parts'.
				if (!fg)
					p.fillRect(x1, 0, tick - x1, wh, QColor(192, 192, 192, 127));
			}
			else
			{
				if (fg)
				{
					p.setPen(Qt::gray);
					p.drawLine(x1, lval, tick, lval);
				}
				else
				{
					p.setPen(myPen);
					p.fillRect(x1, lval, tick - x1, wh - lval, QBrush(vuGrad)); //, config.ctrlGraphFg);
				}
			}


			x1 = tick;
			if (val == CTRL_VAL_UNKNOWN)
				lval = CTRL_VAL_UNKNOWN;
			else
			{
				if (_cnum == CTRL_PROGRAM)
					lval = wh - ((pval - min - bias) * wh / (max - min));
				else
					lval = wh - ((val - min - bias) * wh / (max - min));
			}
		}
		if (lval == CTRL_VAL_UNKNOWN)
		{
			if (!fg)
			{
				p.fillRect(x1, 0, (x + w) - x1, wh, QColor(192, 192, 192, 127));
				noEvents = true;
			}
		}
		else
		{
			if (fg)
			{
				p.setPen(QColor(192, 192, 192, 127));
				p.drawLine(x1, lval, x + w, lval);
			}
			else
			{
				p.setPen(myPen);
				p.fillRect(x1, lval, (x + w) - x1, wh - lval, QBrush(vuGrad)); //, config.ctrlGraphFg);
			}
		}
	}
}

//---------------------------------------------------------
//   pdraw
//---------------------------------------------------------

void CtrlCanvas::pdraw(QPainter& p, const QRect& rect)
{

	int x = rect.x() - 1; // compensate for 3 pixel line width
	int y = rect.y();
	int w = rect.width() + 2;
	int h = rect.height();

	//---------------------------------------------------
	// draw the grid
	//---------------------------------------------------

	p.save();
	View::pdraw(p, rect);
	p.restore();

	//---------------------------------------------------
	// draw Canvas Items
	//---------------------------------------------------

	bool velo = (midiControllerType(_controller->num()) == MidiController::Velo);
	if (!velo)
	{
		pdrawItems(p, rect, curPart, false, false);
	}
	for (iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
	{
		MidiPart* part = (MidiPart*) (ip->second);
		//if((velo && part == curPart) || (!velo && part != curPart))
		if (part == curPart)
			continue;
		pdrawItems(p, rect, part, velo, !velo);
	}
	if (velo)
	{
		pdrawItems(p, rect, curPart, true, true);
	}

	//---------------------------------------------------
	//    draw marker
	//---------------------------------------------------

	int xp = mapx(pos[0]);
	if (xp >= x && xp < x + w)
	{
		//p.setPen(Qt::red);
		p.setPen(QColor(0, 186, 255));
		//p.setPen(QColor(139,225,69));
		p.drawLine(xp, y, xp, y + h);
	}
	xp = mapx(pos[1]);
	if (xp >= x && xp < x + w)
	{
		p.setPen(QColor(139, 225, 69));
		//p.setPen(Qt::blue);
		p.drawLine(xp, y, xp, y + h);
	}
	xp = mapx(pos[2]);
	if (xp >= x && xp < x + w)
	{
		p.setPen(QColor(139, 225, 69));
		//p.setPen(Qt::blue);
		p.drawLine(xp, y, xp, y + h);
	}

	//---------------------------------------------------
	//    draw lasso
	//---------------------------------------------------

	if (drag == DRAG_LASSO)
	{
		QColor outlineColor = QColor(55,80,45);
		QColor fillColor = QColor(148, 177, 106,127);
		setPainter(p);
		QPen mypen2 = QPen(outlineColor, 2, Qt::SolidLine);
		p.setPen(mypen2);
		//p.setPen(Qt::blue);
		//p.setBrush(Qt::NoBrush);
		p.setBrush(QBrush(fillColor));
		p.drawRect(lasso);
	}
}

//---------------------------------------------------------
//   drawOverlay
//---------------------------------------------------------

void CtrlCanvas::drawOverlay(QPainter& p, const QRect&)
{
	QString s(_controller->name());
	p.setFont(config.fonts[3]);
	p.setPen(QColor(0,0,0,75));
	QFontMetrics fm(config.fonts[3]);
	int y = fm.lineSpacing() + 2;
	p.drawText(2, y, s);
	if (noEvents)
	{
		p.setFont(config.fonts[3]);
		p.setPen(Qt::black);
                p.drawText(width() / 2 - 100, height() / 2 - 10, "Use shift + pencil or line tool to draw new events");
		//p.drawText(2 , y * 2, "Use shift + pencil or line tool to draw new events");
	}
}

//---------------------------------------------------------
//   overlayRect
//    returns geometry of overlay rectangle
//---------------------------------------------------------

QRect CtrlCanvas::overlayRect() const
{
	QFontMetrics fm(config.fonts[3]);
	QRect r(fm.boundingRect(_controller ? _controller->name() : QString("")));
	r.translate(2, 2); // top/left margin
	return r;
}

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void CtrlCanvas::draw(QPainter& p, const QRect& rect)
{
	drawTickRaster(p, rect.x(), rect.y(),
			//rect.width(), rect.height(), editor->quant());
			rect.width(), rect.height(), editor->raster());

	//---------------------------------------------------
	//    draw line tool
	//---------------------------------------------------

	if (drawLineMode && (tool == DrawTool))
	{
		p.setRenderHint(QPainter::Antialiasing, true);
		//p.setPen(Qt::black);
		QPen mypen = QPen(QColor(55,80,45), 2, Qt::SolidLine);
		p.setPen(mypen);
		p.drawLine(line1x, line1y, line2x, line2y);
	}
}

//---------------------------------------------------------
//   setCurDrumInstrument
//---------------------------------------------------------

void CtrlCanvas::setCurDrumInstrument(int di)
{
	curDrumInstrument = di;
	//printf("CtrlCanvas::setCurDrumInstrument curDrumInstrument:%d\n", curDrumInstrument);

	//
	//  check if current controller is only valid for
	//  a specific drum instrument
	//
	// Removed by T356.
	//if(curTrack && (curTrack->type() == Track::DRUM) && ((_controller->num() & 0xff) == 0xff)) {
	//if(curTrack && (curTrack->type() == Track::DRUM) && ((_cnum & 0xff) == 0xff)) {
	// reset to default
	// TODO: check, if new drum instrument has a similar controller
	//    configured
	//      _cnum = CTRL_VELOCITY;
	//      }
	// Removed by T356
	//songChanged(-1);
}
