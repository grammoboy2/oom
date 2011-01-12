//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: waveview.cpp,v 1.10.2.16 2009/11/14 03:37:48 terminator356 Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <values.h>
#include <sys/wait.h>

#include <QPainter>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QMouseEvent>

#include "editgain.h"
#include "globals.h"
#include "wave.h"
#include "waveview.h"
#include "song.h"
#include "event.h"
#include "waveedit.h"
#include "audio.h"
#include "gconfig.h"

bool modifyWarnedYet = false;
//---------------------------------------------------------
//   WaveView
//---------------------------------------------------------

WaveView::WaveView(MidiEditor* pr, QWidget* parent, int xscale, int yscale)
: View(parent, xscale, 1)
{
	editor = pr;
	setVirt(true);
	pos[0] = tempomap.tick2frame(song->cpos());
	pos[1] = tempomap.tick2frame(song->lpos());
	pos[2] = tempomap.tick2frame(song->rpos());
	yScale = yscale;
	mode = NORMAL;
	selectionStart = 0;
	selectionStop = 0;
	lastGainvalue = 100;

	setFocusPolicy(Qt::StrongFocus); // Tim.

	setMouseTracking(true);
	setBg(QColor(0, 0, 0));

	if (editor->parts()->empty())
	{
		curPart = 0;
		curPartId = -1;
	}
	else
	{
		curPart = (WavePart*) (editor->parts()->begin()->second);
		curPartId = curPart->sn();
	}


	connect(song, SIGNAL(posChanged(int, unsigned, bool)), SLOT(setPos(int, unsigned, bool)));
	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	songChanged(SC_SELECTION);
}

//---------------------------------------------------------
//   setYScale
//---------------------------------------------------------

void WaveView::setYScale(int val)
{
	yScale = val;
	redraw();
}

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void WaveView::pdraw(QPainter& p, const QRect& rr)
{
	int x1 = rr.x();
	int x2 = rr.right() + 1;
	if (x1 < 0)
		x1 = 0;
	if (x2 > width())
		x2 = width();
	int hh = height();
	int h = hh / 2;
	int y = rr.y() + h;

	// Added by T356.
	int xScale = xmag;
	if (xScale < 0)
		xScale = -xScale;

	for (iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
	{
		WavePart* wp = (WavePart*) (ip->second);
		int channels = wp->track()->channels();
		int px = wp->frame();

		EventList* el = wp->events();
		for (iEvent e = el->begin(); e != el->end(); ++e)
		{
			Event event = e->second;
			if (event.empty())
				continue;
			SndFileR f = event.sndFile();
			if (f.isNull())
				continue;

			unsigned peoffset = px + event.frame() - event.spos();
			int sx, ex;

			sx = event.frame() + px + xScale / 2;
			ex = sx + event.lenFrame();
			sx = sx / xScale - xpos;
			ex = ex / xScale - xpos;

			if (sx < x1)
				sx = x1;
			if (ex > x2)
				ex = x2;

			int pos = (xpos + sx) * xScale + event.spos() - event.frame() - px;

			//printf("pos=%d xpos=%d sx=%d ex=%d xScale=%d event.spos=%d event.frame=%d px=%d\n",
			//      pos, xpos, sx, ex, xScale, event.spos(), event.frame(), px);

			h = hh / (channels * 2);
			int cc = hh % (channels * 2) ? 0 : 1;

			for (int i = sx; i < ex; i++)
			{
				y = rr.y() + h;
				SampleV sa[f.channels()];
				f.read(sa, xScale, pos);
				pos += xScale;
				if (pos < event.spos())
					continue;

				int selectionStartPos = selectionStart - peoffset; // Offset transformed to event coords
				int selectionStopPos = selectionStop - peoffset;

				for (int k = 0; k < channels; ++k)
				{
				
					int kk = k % f.channels();
					int peak = (sa[kk].peak * (h - 1)) / yScale;
					int rms = (sa[kk].rms * (h - 1)) / yScale;
					if (peak > h)
						peak = h;
					if (rms > h)
						rms = h;
					QColor peak_color = QColor(Qt::darkGray);
					QColor rms_color = QColor(0,19,23);
				
					// Changed by T356. Reduces (but not eliminates) drawing artifacts.
					//if (pos > selectionStartPos && pos < selectionStopPos) {
					if (pos > selectionStartPos && pos <= selectionStopPos)
					{

						peak_color = QColor(Qt::lightGray);
						rms_color = QColor(214,214,214);
						// Draw inverted
						p.setPen(QColor(0,10,15));
						p.drawLine(i, y - h + cc, i, y + h - cc);
					}
					else
					{
						//p.setPen(QColor(203,211,212));
						//p.drawLine(i, y - h + cc, i, y + h - cc);
					}
					//p.drawLine(i, y - peak - cc, i, y + peak);
					QColor green = QColor(49, 175, 197);
					QColor yellow = QColor(127,12,128);
					QColor red = QColor(197, 49, 87);
					if(k == 0)
					{
						QLinearGradient vuGrad(QPointF(0, 0), QPointF(0, h*2));
						vuGrad.setColorAt(1, red);
						//vuGrad.setColorAt(0.90, yellow);
						vuGrad.setColorAt(0.6, green);
						vuGrad.setColorAt(0.5, green);
						vuGrad.setColorAt(0.4, green);
						//vuGrad.setColorAt(0.10, yellow);
						vuGrad.setColorAt(0, red);
						QPen myPen = QPen();
						myPen.setBrush(QBrush(vuGrad));
						p.setPen(myPen);
						p.drawLine(i, y - peak - cc, i, y + peak);
					}
					else
					{
						//QLinearGradient vuGrad(QPointF(0, 0), QPointF(0, hh*2));
						QLinearGradient vuGrad(QPointF(0, h*2), QPointF(0, hh));
						vuGrad.setColorAt(1, red);
						vuGrad.setColorAt(0.6, green);
						vuGrad.setColorAt(0.5, green);
						vuGrad.setColorAt(0.4, green);
						vuGrad.setColorAt(0, red);
						QPen myPen = QPen();
						myPen.setBrush(QBrush(vuGrad));
						p.setPen(myPen);
						p.drawLine(i, y - peak - cc, i, y + peak);
					}
					
					p.setPen(rms_color);
					p.drawLine(i, y - rms - cc, i, y + rms);
					y += 2 * h;
				}
			}
		}
	}
	View::pdraw(p, rr);
}

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void WaveView::draw(QPainter& p, const QRect& r)
{
	unsigned x = r.x() < 0 ? 0 : r.x();
	unsigned y = r.y() < 0 ? 0 : r.y();
	int w = r.width();
	int h = r.height();

	unsigned x2 = x + w;
	unsigned y2 = y + h;

	//
	//    draw marker & centerline
	//
	p.setPen(QColor(0,186,255));
	if (pos[0] >= x && pos[0] < x2)
	{
		p.drawLine(pos[0], y, pos[0], y2);
	}
	p.setPen(QColor(139,255,69));
	if (pos[1] >= x && pos[1] < x2)
	{
		p.drawLine(pos[1], y, pos[1], y2);
	}
	if (pos[2] >= x && pos[2] < x2)
		p.drawLine(pos[2], y, pos[2], y2);

	// Changed by T356. Support multiple (or none) selected parts.
	//int n  = curPart->track()->channels();
	int n = 1;
	if (curPart)
		n = curPart->track()->channels();

	int hn = h / n;
	int hh = hn / 2;
	for (int i = 0; i < n; ++i)
	{
		int h2 = hn * i;
		int center = hh + h2;
		if(i == 0)
		{
			//blue middle marker
			p.setPen(QColor(102,177,205));
			p.drawLine(x, center-1, x2, center-1);
		}
		else
		{
			//red middle marker
			p.setPen(QColor(213,93,93));
			p.drawLine(x, center-3, x2, center-3);
		}
		p.setPen(QColor(Qt::black));
		p.drawLine(x, h2, x2, h2);
	}
}

//---------------------------------------------------------
//   getCaption
//---------------------------------------------------------

QString WaveView::getCaption() const
{

	// Changed by T356. Support multiple (or none) selected parts.
	//return QString("Part ") + curPart->name();
	if (curPart)
		return QString("Part ") + curPart->name();
	else
		return QString("Part ");

}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void WaveView::songChanged(int flags)
{
	// Is it simply a midi controller value adjustment? Forget it.
	if (flags == SC_MIDI_CONTROLLER)
		return;

	if (flags & SC_SELECTION)
	{
		startSample = MAXINT;
		endSample = 0;
		curPart = 0;
		for (iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p)
		{
			WavePart* part = (WavePart*) (p->second);
			if (part->sn() == curPartId)
				curPart = part;
			int ssample = part->frame();
			int esample = ssample + part->lenFrame();
			if (ssample < startSample)
			{
				startSample = ssample;
				//printf("startSample = %d\n", startSample);
			}
			if (esample > endSample)
			{
				endSample = esample;
				//printf("endSample = %d\n", endSample);
			}
		}
	}
	if (flags & SC_CLIP_MODIFIED)
	{
		redraw(); // Boring, but the only thing possible to do
	}
	if (flags & SC_TEMPO)
	{
		setPos(0, song->cpos(), false);
		setPos(1, song->lpos(), false);
		setPos(2, song->rpos(), false);
	}
	redraw();
}

//---------------------------------------------------------
//   setPos
//    set one of three markers
//    idx   - 0-cpos  1-lpos  2-rpos
//    flag  - emit followEvent()
//---------------------------------------------------------

void WaveView::setPos(int idx, unsigned val, bool adjustScrollbar)
{
	val = tempomap.tick2frame(val);
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
					int ppos = val - xorg - rmapxDev(width() / 4);
					if (ppos < 0)
						ppos = 0;
					emit followEvent(ppos);
					opos = mapx(pos[idx]);
					npos = mapx(val);
				}
				else if (npos < 0)
				{
					int ppos = val - xorg - rmapxDev(width()*3 / 4);
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
					int ppos = pos[idx] - xorg - rmapxDev(width()*5 / 8);
					if (ppos < 0)
						ppos = 0;
					emit followEvent(ppos);
					opos = mapx(pos[idx]);
					npos = mapx(val);
				}
				else if (npos < (width()*3) / 8)
				{
					int ppos = pos[idx] - xorg - rmapxDev(width()*3 / 8);
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
	redraw(QRect(x, 0, w, height()));
}

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void WaveView::viewMousePressEvent(QMouseEvent* event)
{
	button = event->button();
	unsigned x = event->x();

	switch (button)
	{
		case Qt::LeftButton:
			if (mode == NORMAL)
			{
				// redraw and reset:
				if (selectionStart != selectionStop)
				{
					selectionStart = selectionStop = 0;
					redraw();
				}
				mode = DRAG;
				dragstartx = x;
				selectionStart = selectionStop = x;
			}
			break;

		case Qt::MidButton:
		case Qt::RightButton:
		default:
			break;
	}
	viewMouseMoveEvent(event);
}


//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void WaveView::wheelEvent(QWheelEvent* event)
{
	emit mouseWheelMoved(event->delta() / 10);
}

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void WaveView::viewMouseReleaseEvent(QMouseEvent* /*event*/)
{
	button = Qt::NoButton;

	if (mode == DRAG)
	{
		mode = NORMAL;
		//printf("selectionStart=%d selectionStop=%d\n", selectionStart, selectionStop);
	}
}

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void WaveView::viewMouseMoveEvent(QMouseEvent* event)
{
	unsigned x = event->x();
	emit timeChanged(x);

	int i;
	switch (button)
	{
		case Qt::LeftButton:
			i = 0;
			if (mode == DRAG)
			{
				if (x < dragstartx)
				{
					selectionStart = x;
					selectionStop = dragstartx;
				}
				else
				{
					selectionStart = dragstartx;
					selectionStop = x;
				}
			}
			break;
		case Qt::MidButton:
			i = 1;
			break;
		case Qt::RightButton:
			i = 2;
			break;
		default:
			return;
	}
	Pos p(tempomap.frame2tick(x), true);
	song->setPos(i, p);
}

//---------------------------------------------------------
//   range
//    returns range in samples
//---------------------------------------------------------

void WaveView::range(int* s, int *e)
{

	PartList* lst = editor->parts();
	if (lst->empty())
	{
		*s = 0;
		*e = tempomap.tick2frame(song->len());
		return;
	}
	int ps = song->len(), pe = 0;
	int tps, tpe;
	for (iPart ip = lst->begin(); ip != lst->end(); ++ip)
	{
		tps = ip->second->tick();
		if (tps < ps)
			ps = tps;
		tpe = tps + ip->second->lenTick();
		if (tpe > pe)
			pe = tpe;
	}
	*s = tempomap.tick2frame(ps);
	*e = tempomap.tick2frame(pe);
}

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void WaveView::cmd(int n)
{
	int modifyoperation = -1;
	double paramA = 0.0;

	switch (n)
	{
		case WaveEdit::CMD_SELECT_ALL:
			if (!editor->parts()->empty())
			{
				iPart iBeg = editor->parts()->begin();
				iPart iEnd = editor->parts()->end();
				iEnd--;
				WavePart* beg = (WavePart*) iBeg->second;
				WavePart* end = (WavePart*) iEnd->second;
				selectionStart = beg->frame();
				selectionStop = end->frame() + end->lenFrame();
				redraw();
			}
			break;

		case WaveEdit::CMD_EDIT_EXTERNAL:
			modifyoperation = EDIT_EXTERNAL;
			break;

		case WaveEdit::CMD_SELECT_NONE:
			selectionStart = selectionStop = 0;
			redraw();
			break;

		case WaveEdit::CMD_MUTE:
			modifyoperation = MUTE;
			break;

		case WaveEdit::CMD_NORMALIZE:
			modifyoperation = NORMALIZE;
			break;

		case WaveEdit::CMD_FADE_IN:
			modifyoperation = FADE_IN;
			break;

		case WaveEdit::CMD_FADE_OUT:
			modifyoperation = FADE_OUT;
			break;

		case WaveEdit::CMD_REVERSE:
			modifyoperation = REVERSE;
			break;

		case WaveEdit::CMD_GAIN_FREE:
		{
			EditGain* editGain = new EditGain(this, lastGainvalue);
			if (editGain->exec() == QDialog::Accepted)
			{
				lastGainvalue = editGain->getGain();
				modifyoperation = GAIN;
				paramA = (double) lastGainvalue / 100.0;
			}
			delete editGain;
		}
			break;

		case WaveEdit::CMD_GAIN_200:
			modifyoperation = GAIN;
			paramA = 2.0;
			break;

		case WaveEdit::CMD_GAIN_150:
			modifyoperation = GAIN;
			paramA = 1.5;
			break;

		case WaveEdit::CMD_GAIN_75:
			modifyoperation = GAIN;
			paramA = 0.75;
			break;

		case WaveEdit::CMD_GAIN_50:
			modifyoperation = GAIN;
			paramA = 0.5;
			break;

		case WaveEdit::CMD_GAIN_25:
			modifyoperation = GAIN;
			paramA = 0.25;
			break;

		default:
			break;
	}

	if (modifyoperation != -1)
	{
		if (selectionStart == selectionStop)
		{
			printf("No selection. Ignoring\n"); //@!TODO: Disable menu options when no selection
			QMessageBox::information(this,
					QString("OOMidi"),
					QWidget::tr("No selection. Ignoring"));

			return;
		}

		//if(!modifyWarnedYet)
		//{
		//  modifyWarnedYet = true;
		//  if(QMessageBox::warning(this, QString("OOMidi"),
		//     tr("Warning! OOMidi currently operates directly on the sound file.\n"
		//        "Undo is supported, but NOT after exit, WITH OR WITHOUT A SAVE!\n"
		//        "If you are stuck, try deleting the associated .wca file and reloading."), tr("&Ok"), tr("&Cancel"),
		//     QString::null, 0, 1 ) != 0)
		//   return;
		//}
		modifySelection(modifyoperation, selectionStart, selectionStop, paramA);
	}
}


//---------------------------------------------------------
//   getSelection
//---------------------------------------------------------

WaveSelectionList WaveView::getSelection(unsigned startpos, unsigned stoppos)
{
	WaveSelectionList selection;

	for (iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
	{
		WavePart* wp = (WavePart*) (ip->second);
		unsigned part_offset = wp->frame();

		EventList* el = wp->events();
		//printf("eventlist length=%d\n",el->size());

		for (iEvent e = el->begin(); e != el->end(); ++e)
		{
			Event event = e->second;
			if (event.empty())
				continue;
			SndFileR file = event.sndFile();
			if (file.isNull())
				continue;

			unsigned event_offset = event.frame() + part_offset;
			unsigned event_startpos = event.spos();
			unsigned event_length = event.lenFrame() + event.spos();
			unsigned event_end = event_offset + event_length;
			//printf("startpos=%d stoppos=%d part_offset=%d event_offset=%d event_startpos=%d event_length=%d event_end=%d\n", startpos, stoppos, part_offset, event_offset, event_startpos, event_length, event_end);

			if (!(event_end <= startpos || event_offset > stoppos))
			{
				int tmp_sx = startpos - event_offset + event_startpos;
				int tmp_ex = stoppos - event_offset + event_startpos;
				unsigned sx;
				unsigned ex;

				tmp_sx < (int) event_startpos ? sx = event_startpos : sx = tmp_sx;
				tmp_ex > (int) event_length ? ex = event_length : ex = tmp_ex;

				//printf("Event data affected: %d->%d filename:%s\n", sx, ex, file.name().toLatin1().constData());
				WaveEventSelection s;
				s.file = file;
				s.startframe = sx;
				s.endframe = ex + 1;
				//printf("sx=%d ex=%d\n",sx,ex);
				selection.push_back(s);
			}
		}
	}

	return selection;
}

//---------------------------------------------------------
//   modifySelection
//---------------------------------------------------------

void WaveView::modifySelection(int operation, unsigned startpos, unsigned stoppos, double paramA)
{
	song->startUndo();

	WaveSelectionList selection = getSelection(startpos, stoppos);
	for (iWaveSelection i = selection.begin(); i != selection.end(); i++)
	{
		WaveEventSelection w = *i;
		SndFileR& file = w.file;
		unsigned sx = w.startframe;
		unsigned ex = w.endframe;
		unsigned file_channels = file.channels();

		QString tmpWavFile = QString::null;
		if (!getUniqueTmpfileName(tmpWavFile))
		{
			break;
		}

		audio->msgIdle(true); // Not good with playback during operations
		SndFile tmpFile(tmpWavFile);
		tmpFile.setFormat(file.format(), file_channels, file.samplerate());
		if (tmpFile.openWrite())
		{
			audio->msgIdle(false);
			printf("Could not open temporary file...\n");
			break;
		}

		//
		// Write out data that will be changed to temp file
		//
		unsigned tmpdatalen = ex - sx;
		off_t tmpdataoffset = sx;
		float* tmpdata[file_channels];

		for (unsigned i = 0; i < file_channels; i++)
		{
			tmpdata[i] = new float[tmpdatalen];
		}
		file.seek(tmpdataoffset, 0);
		file.readWithHeap(file_channels, tmpdata, tmpdatalen);
		file.close();
		tmpFile.write(file_channels, tmpdata, tmpdatalen);
		tmpFile.close();

		switch (operation)
		{
			case MUTE:
				muteSelection(file_channels, tmpdata, tmpdatalen);
				break;

			case NORMALIZE:
				normalizeSelection(file_channels, tmpdata, tmpdatalen);
				break;

			case FADE_IN:
				fadeInSelection(file_channels, tmpdata, tmpdatalen);
				break;

			case FADE_OUT:
				fadeOutSelection(file_channels, tmpdata, tmpdatalen);
				break;

			case REVERSE:
				reverseSelection(file_channels, tmpdata, tmpdatalen);
				break;

			case GAIN:
				applyGain(file_channels, tmpdata, tmpdatalen, paramA);
				break;

			case EDIT_EXTERNAL:
				editExternal(file.format(), file.samplerate(), file_channels, tmpdata, tmpdatalen);
				break;

			default:
				printf("Error: Default state reached in modifySelection\n");
				break;

		}

		file.openWrite();
		file.seek(tmpdataoffset, 0);
		file.write(file_channels, tmpdata, tmpdatalen);
		file.update();
		file.close();
		file.openRead();

		for (unsigned i = 0; i < file_channels; i++)
		{
			delete[] tmpdata[i];
		}

		// Undo handling
		song->cmdChangeWave(file.dirPath() + "/" + file.name(), tmpWavFile, sx, ex);
		audio->msgIdle(false); // Not good with playback during operations
	}
	song->endUndo(SC_CLIP_MODIFIED);
	redraw();
}

//---------------------------------------------------------
//   muteSelection
//---------------------------------------------------------

void WaveView::muteSelection(unsigned channels, float** data, unsigned length)
{
	// Set everything to 0!
	for (unsigned i = 0; i < channels; i++)
	{
		for (unsigned j = 0; j < length; j++)
		{
			data[i][j] = 0;
		}
	}
}

//---------------------------------------------------------
//   normalizeSelection
//---------------------------------------------------------

void WaveView::normalizeSelection(unsigned channels, float** data, unsigned length)
{
	float loudest = 0.0;

	for (unsigned i = 0; i < channels; i++)
	{
		for (unsigned j = 0; j < length; j++)
		{
			if (data[i][j] > loudest)
				loudest = data[i][j];
		}
	}

	double scale = 0.99 / (double) loudest;

	for (unsigned i = 0; i < channels; i++)
	{
		for (unsigned j = 0; j < length; j++)
		{
			data[i][j] = (float) ((double) data[i][j] * scale);
		}
	}
}

//---------------------------------------------------------
//   fadeInSelection
//---------------------------------------------------------

void WaveView::fadeInSelection(unsigned channels, float** data, unsigned length)
{
	for (unsigned i = 0; i < channels; i++)
	{
		for (unsigned j = 0; j < length; j++)
		{
			double scale = (double) j / (double) length;
			data[i][j] = (float) ((double) data[i][j] * scale);
		}
	}
}

//---------------------------------------------------------
//   fadeOutSelection
//---------------------------------------------------------

void WaveView::fadeOutSelection(unsigned channels, float** data, unsigned length)
{
	for (unsigned i = 0; i < channels; i++)
	{
		for (unsigned j = 0; j < length; j++)
		{
			double scale = (double) (length - j) / (double) length;
			data[i][j] = (float) ((double) data[i][j] * scale);
		}
	}
}

//---------------------------------------------------------
//   reverseSelection
//---------------------------------------------------------

void WaveView::reverseSelection(unsigned channels, float** data, unsigned length)
{
	if (length <= 1)
		return;
	for (unsigned i = 0; i < channels; i++)
	{
		for (unsigned j = 0; j < length / 2; j++)
		{
			float tmpl = data[i][j];
			float tmpr = data[i][length - j - 1];
			data[i][j] = tmpr;
			data[i][length - j - 1] = tmpl;
		}
	}
}
//---------------------------------------------------------
//   applyGain
//---------------------------------------------------------

void WaveView::applyGain(unsigned channels, float** data, unsigned length, double gain)
{
	for (unsigned i = 0; i < channels; i++)
	{
		for (unsigned j = 0; j < length; j++)
		{
			data[i][j] = (float) ((double) data[i][j] * gain);
		}
	}
}

//---------------------------------------------------------
//   editExternal
//---------------------------------------------------------

void WaveView::editExternal(unsigned file_format, unsigned file_samplerate, unsigned file_channels, float** tmpdata, unsigned tmpdatalen)
{
	// Create yet another tmp-file
	QString exttmpFileName;
	if (!getUniqueTmpfileName(exttmpFileName))
	{
		printf("Could not create temp file - aborting...\n");
		return;
	}

	SndFile exttmpFile(exttmpFileName);
	exttmpFile.setFormat(file_format, file_channels, file_samplerate);
	if (exttmpFile.openWrite())
	{
		printf("Could not open temporary file...\n");
		return;
	}
	// Write out change-data to this file:
	exttmpFile.write(file_channels, tmpdata, tmpdatalen);
	exttmpFile.close();

	// Forkaborkabork
	int pid = fork();
	if (pid == 0)
	{
		if (execlp(config.externalWavEditor.toLatin1().constData(), config.externalWavEditor.toLatin1().constData(), exttmpFileName.toLatin1().constData(), NULL) == -1)
		{
			perror("Failed to launch external editor");
			// Get out of here


			// cannot report error through gui, we are in another fork!
			//@!TODO: Handle unsuccessful attempts
			exit(99);
		}
		exit(0);
	}
	else if (pid == -1)
	{
		perror("fork failed");
	}
	else
	{
		int status;
		waitpid(pid, &status, 0);
		//printf ("status=%d\n",status);
		if (WEXITSTATUS(status) != 0)
		{
			QMessageBox::warning(this, tr("OOMidi - external editor failed"),
					tr("OOMidi was unable to launch the external editor\ncheck if the editor setting in:\n"
					"Global Settings->Audio:External Waveditor\nis set to a valid editor."));
		}

		if (exttmpFile.openRead())
		{
			printf("Could not reopen temporary file!\n");
		}
		else
		{
			// Re-read file again
			exttmpFile.seek(0, 0);
			size_t sz = exttmpFile.readWithHeap(file_channels, tmpdata, tmpdatalen);
			if (sz != tmpdatalen)
			{
				// File must have been shrunken - not good. Alert user.
				QMessageBox::critical(this, tr("OOMidi - file size changed"),
						tr("When editing in external editor - you should not change the filesize\nsince it must fit the selected region.\n\nMissing data is muted"));
				for (unsigned i = 0; i < file_channels; i++)
				{
					for (unsigned j = sz; j < tmpdatalen; j++)
					{
						tmpdata[i][j] = 0;
					}
				}
			}
		}
		QDir dir = exttmpFile.dirPath();
		dir.remove(exttmpFileName);
		dir.remove(exttmpFile.basename() + ".wca");
	}
}

//---------------------------------------------------------
//   getUniqueTmpfileName
//---------------------------------------------------------

bool WaveView::getUniqueTmpfileName(QString& newFilename)
{
	// Check if tmp-directory exists under project path
	QString tmpWavDir = oomProject + "/tmp_oomwav"; //!@TODO: Don't hardcode like this
	QFileInfo tmpdirfi(tmpWavDir);
	if (!tmpdirfi.isDir())
	{
		// Try to create a tmpdir
		QDir projdir(oomProject);
		if (!projdir.mkdir("tmp_oomwav"))
		{
			printf("Could not create undo dir!\n");
			return false;
		}
	}


	tmpdirfi.setFile(tmpWavDir);

	if (!tmpdirfi.isWritable())
	{
		printf("Temp directory is not writable - aborting\n");
		return false;
	}

	QDir tmpdir = tmpdirfi.dir();

	// Find a new filename
	for (int i = 0; i < 10000; i++)
	{
		QString filename = "oom_tmp";
		filename.append(QString::number(i));
		filename.append(".wav");

		if (!tmpdir.exists(tmpWavDir + "/" + filename))
		{
			newFilename = tmpWavDir + "/" + filename;
			return true;
		}

	}

	printf("Could not find a suitable tmpfilename (more than 10000 tmpfiles in tmpdir - clean up!\n");
	return false;
}


