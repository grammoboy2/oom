//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "OOStudio.h"
#include "OOProcess.h"
#include "config.h"
#include <QtGui>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>
#include <QDomNode>

static const char* LS_HOSTNAME  = "localhost";
static const int   LS_PORT      = 8888;
static const char* LS_COMMAND   = "linuxsampler";
static const char* JACK_COMMAND = "/usr/bin/jackd -r -t2000 -dalsa -dhw:0 -r48000 -p512 -n3 -Xraw -P";

lscp_status_t client_callback ( lscp_client_t* /*_client*/, lscp_event_t /*event*/, const char * /*pchData*/, int /*cchData*/, void* pvData )
{
	OOStudio* lsc = (OOStudio*)pvData;
	if(lsc == NULL)
		return LSCP_FAILED;
	return LSCP_OK;
}

OOStudio::OOStudio()
{
	setupUi(this);
	m_oomProcess = 0;
	m_jackProcess = 0;
	m_lsProcess = 0;
	m_current = 0;
	m_incleanup = false;
	createTrayIcon();

	m_cmbTemplate->addItem("Default", "Default");
	m_cmbLSCPMode->addItem("Default", "Default");
	m_cmbLSCPMode->addItem("ON_DEMAND", "ON_DEMAND");
	m_cmbLSCPMode->addItem("ON_DEMAND_HOLD", "ON_DEMAND_HOLD");
	m_cmbLSCPMode->addItem("PERSISTENT", "PERSISTENT");
	
	m_sessionlabels = (QStringList() << "Name" << "Path" );
	m_templatelabels = (QStringList() << "Name" << "Path" );
	m_commandlabels = (QStringList() << "Command" );
	m_loglabels = (QStringList() << "Command" << "Log" );

	m_txtLSPort->setValidator(new QIntValidator());
	createModels();
	createConnections();
	//runJack();

	trayIcon->show();
	setWindowTitle(tr("OOMIDI: Studio"));
	QSettings settings("OOMidi", "OOStudio");
	QSize size = settings.value("OOStudio/size", QSize(548, 526)).toSize();
	resize(size);
	populateSessions();

#ifdef OOM_INSTALL_BIN
	//qDebug() << OOM_INSTALL_BIN;
	//qDebug() << LIBDIR;
	printf("Found install path: %s\n", OOM_INSTALL_BIN);
#endif
}

void OOStudio::createTrayIcon()/*{{{*/
{
	minimizeAction = new QAction(tr("Mi&nimize"), this);
	connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));
	maximizeAction = new QAction(tr("Ma&ximize"), this);
	connect(maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));
	restoreAction = new QAction(tr("&Restore"), this);
	connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));
	quitAction = new QAction(tr("&Quit"), this);
	connect(quitAction, SIGNAL(triggered()), this, SLOT(shutdown()));

	trayMenu = new QMenu(this);
	trayMenu->addAction(minimizeAction);
	trayMenu->addAction(maximizeAction);
	trayMenu->addAction(restoreAction);
	trayMenu->addSeparator();
	trayMenu->addAction(quitAction);

	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setContextMenu(trayMenu);
	QIcon icon(":/images/oom_icon.png");
	trayIcon->setIcon(icon);
}/*}}}*/

void OOStudio::createModels()/*{{{*/
{
	m_sessionModel = new QStandardItemModel(m_sessionTable);
	m_sessionTable->setModel(m_sessionModel);
	m_sessionModel->setHorizontalHeaderLabels(m_sessionlabels);
	
	m_templateModel = new QStandardItemModel(m_templateTable);
	m_templateTable->setModel(m_templateModel);
	m_templateModel->setHorizontalHeaderLabels(m_templatelabels);
	
	m_commandModel = new QStandardItemModel(m_commandTable);
	m_commandTable->setModel(m_commandModel);
	m_commandModel->setHorizontalHeaderLabels(m_commandlabels);
	
	m_loggerModel = new QStandardItemModel(m_loggerTable);
	m_loggerTable->setModel(m_loggerModel);
	m_loggerModel->setHorizontalHeaderLabels(m_loglabels);

	m_sessionSelectModel = new QItemSelectionModel(m_sessionModel);
	m_templateSelectModel = new QItemSelectionModel(m_templateModel);
	m_sessionTable->setSelectionModel(m_sessionSelectModel);
	m_templateTable->setSelectionModel(m_templateSelectModel);
}/*}}}*/

void OOStudio::createConnections()/*{{{*/
{
	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
	connect(m_btnLoadSession, SIGNAL(clicked()), this, SLOT(loadSessionClicked()));
	connect(m_btnLoadTemplate, SIGNAL(clicked()), this, SLOT(loadTemplateClicked()));
	connect(m_btnClearCreate, SIGNAL(clicked()), this, SLOT(resetCreate()));
	connect(m_btnDoCreate, SIGNAL(clicked()), this, SLOT(createSession()));
	connect(m_btnImport, SIGNAL(clicked()), this, SLOT(importSession()));
	connect(m_btnBrowsePath, SIGNAL(clicked()), this, SLOT(browseLocation()));
	connect(m_btnBrowseLSCP, SIGNAL(clicked()), this, SLOT(browseLSCP()));
	connect(m_btnBrowseOOM, SIGNAL(clicked()), this, SLOT(browseOOM()));
	connect(m_btnAddCommand, SIGNAL(clicked()), this, SLOT(addCommand()));
	connect(m_btnDeleteCommand, SIGNAL(clicked()), this, SLOT(deleteCommand()));
	connect(m_btnClearLog, SIGNAL(clicked()), this, SLOT(clearLogger()));
	connect(m_cmbTemplate, SIGNAL(currentIndexChanged(int)), this, SLOT(templateSelectionChanged(int)));
}/*}}}*/

void OOStudio::updateHeaders()/*{{{*/
{
	m_sessionModel->setHorizontalHeaderLabels(m_sessionlabels);
	
	m_templateModel->setHorizontalHeaderLabels(m_templatelabels);
	
	m_commandModel->setHorizontalHeaderLabels(m_commandlabels);

	m_loggerModel->setHorizontalHeaderLabels(m_loglabels);
}/*}}}*/

void OOStudio::populateSessions()/*{{{*/
{
	m_sessionModel->clear();
	m_templateModel->clear();
	while(m_cmbTemplate->count() > 1)
	{
		m_cmbTemplate->removeItem((m_cmbTemplate->count()-1));
	}
	QSettings settings("OOMidi", "OOStudio");
	int sess = settings.beginReadArray("Sessions");
	printf("OOStudio::populateSession: sessions: %d\n", sess);
	for(int i = 0; i < sess; ++i)
	{
		settings.setArrayIndex(i);
		printf("Processing session %s \n", settings.value("name").toString().toUtf8().constData());
		OOSession* session = readSession(settings.value("path").toString());
		if(session)
		{
			printf("Found valid session\n");
			QStandardItem* name = new QStandardItem(session->name);
			QStandardItem* path = new QStandardItem(session->path);
			QList<QStandardItem*> rowData;
			rowData << name << path;
			if(session->istemplate)
			{
				m_templateModel->appendRow(rowData);
				m_cmbTemplate->addItem(name->text(), path->text());
			}
			else
			{
				m_sessionModel->appendRow(rowData);
			}
			m_sessionMap[session->name] = session;
		}
	}
	updateHeaders();
}/*}}}*/

void OOStudio::clearLogger()/*{{{*/
{
	m_loggerModel->clear();
	updateHeaders();
}/*}}}*/

void OOStudio::browseLocation()/*{{{*/
{
	browse(0);
}/*}}}*/

void OOStudio::browseLSCP()/*{{{*/
{
	browse(1);
}/*}}}*/

void OOStudio::browseOOM()/*{{{*/
{
	browse(2);
}/*}}}*/

void OOStudio::importSession()/*{{{*/
{
	browse(3);
}/*}}}*/

QString OOStudio::getValidName(QString name)
{
	QString base("_Copy");
	int i = 1;
	QString newName(name.append(base));
	while(m_sessionMap.contains(newName))
	{
		newName = name.append(base).append(QString::number(i));
		++i;
	}
	return newName;
}

void OOStudio::browse(int form)/*{{{*/
{
	switch(form)
	{
		case 0: //Location path
		{
			QString dirname = QFileDialog::getExistingDirectory(
				this,
				tr("Select a Directory"),
				QDir::currentPath());
			if(!dirname.isNull())
			{
				m_txtLocation->setText(dirname);
			}
		}
		break;
		case 1: //LSCP file path
		{
			QString filename = QFileDialog::getOpenFileName(
				this,
				tr("Open LSCP File"),
				QDir::currentPath(),
				tr("LSCP Files (*.lscp);;All Files (*.*)"));
			if(!filename.isNull())
			{
				m_txtLSCP->setText(filename);
			}
		}
		break;
		case 2: //OOM file path
		{
			QString filename = QFileDialog::getOpenFileName(
				this,
				tr("Open OOM File"),
				QDir::currentPath(),
				tr("OOM Song Files (*.oom);;All Files (*.*)"));
			if(!filename.isNull())
			{
				m_txtOOMPath->setText(filename);
			}
		}
		break;
		case 3: //Import session
		{
			QString filename = QFileDialog::getOpenFileName(
				this,
				tr("Import OOStudio File"),
				QDir::currentPath(),
				tr("OOStudio Session Files (*.oos);;All Files (*.*)"));
			if(!filename.isNull())
			{
				OOSession* session = readSession(filename);
				if(session)
				{
					if(m_sessionMap.contains(session->name))
					{
						session->name = getValidName(session->name);
					}
					QStandardItem* name = new QStandardItem(session->name);
					QStandardItem* path = new QStandardItem(session->path);
					QList<QStandardItem*> rowData;
					rowData << name << path;
					if(session->istemplate)
					{
						m_templateModel->appendRow(rowData);
						m_cmbTemplate->addItem(name->text(), path->text());
						m_sessionBox->setCurrentIndex(1);
					}
					else
					{
						m_sessionModel->appendRow(rowData);
						m_sessionBox->setCurrentIndex(0);
					}
					updateHeaders();
					m_sessionMap[session->name] = session;
					saveSettings();
					//TODO: message to tell what type was imported
				}
				else
				{
					//TODO: send error messages
				}
			}
		}
		break;
	}
}/*}}}*/

void OOStudio::addCommand()/*{{{*/
{
	if(m_txtCommand->text().isEmpty())
		return;
	QStandardItem* item = new QStandardItem(m_txtCommand->text());
	item->setCheckable(true);
	m_commandModel->appendRow(item);
	updateHeaders();
}/*}}}*/

void OOStudio::deleteCommand()/*{{{*/
{
	for(int i = 0; i < m_commandModel->rowCount(); ++i)
	{
		QStandardItem* item = m_commandModel->item(i, 0);
		if(item->checkState() == Qt::Checked)
		{
			m_commandModel->takeRow(i);
		}
	}
}/*}}}*/

void OOStudio::resetCreate(bool fromClear)/*{{{*/
{
	m_txtLSCP->setText("");
	m_txtOOMPath->setText("");
	m_txtCommand->setText("");
	if(fromClear)
	{
		m_txtName->setText("");
		m_txtLocation->setText("");
		m_cmbTemplate->setCurrentIndex(0);
	}
	m_txtLSHost->setText(LS_HOSTNAME);
	m_txtLSPort->setText(QString::number(LS_PORT));
	m_txtLSCommand->setText(LS_COMMAND);
	m_txtJackCommand->setText(JACK_COMMAND);
	m_cmbLSCPMode->setCurrentIndex(0);
	m_commandModel->clear();
	updateHeaders();
}/*}}}*/

void OOStudio::cleanupProcessList()/*{{{*/
{
	printf("Shutting down processes\n");
	m_incleanup = false;
	if(m_oomProcess && m_oomProcess->state() == QProcess::Running)
	{
		m_oomProcess->terminate();
		m_oomProcess->waitForFinished();
		m_oomProcess = 0;
	}
	if(m_lsProcess && m_lsProcess->state() == QProcess::Running)
	{
		m_lsProcess->terminate();
		m_lsProcess->waitForFinished();
		m_lsProcess = 0;
	}
	if(m_jackProcess && m_jackProcess->state() == QProcess::Running)
	{
		m_jackProcess->kill();
		m_jackProcess->waitForFinished();
		m_jackProcess = 0;
	}
	QList<long> keys = m_procMap.keys();
	foreach(long key, keys)
	{
		OOProcess* p = m_procMap.take(key);
		if(p)
		{
			p->terminate();
			p->waitForFinished();
			delete p;
		}
	}
	m_incleanup = false;
}/*}}}*/

void OOStudio::shutdown()/*{{{*/
{
	cleanupProcessList();
	saveSettings();
	qApp->quit();
}/*}}}*/

void OOStudio::saveSettings()/*{{{*/
{
	QSettings settings("OOMidi", "OOStudio");
	if(!m_sessionMap.isEmpty())
	{
		QMap<QString, OOSession*>::const_iterator i = m_sessionMap.constBegin();
		int id = 0;
		settings.beginWriteArray("Sessions");
		while (i != m_sessionMap.constEnd())
		{
			settings.setArrayIndex(id);
			OOSession* session = i.value();
			if(session)
			{
				settings.setValue("name", i.key());
				settings.setValue("path", session->path);
			}
			++i;
			++id;
		}
		settings.endArray();
	}
	settings.beginGroup("OOStudio");
	settings.setValue("size", size());
	settings.endGroup();
	settings.sync();
}/*}}}*/

bool OOStudio::runJack(OOSession* session)/*{{{*/
{
	if(session)
	{
		if(session->loadJack)
		{
			printf("Launching jackd ");
			m_jackProcess = new QProcess(this);
			connect(m_jackProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processJackMessages()));
			connect(m_jackProcess, SIGNAL(readyReadStandardError()), this, SLOT(processJackErrors()));
			QString jackCmd = session->jackcommand;
			QStringList args = jackCmd.split(" ");
			jackCmd = args.takeFirst();
			if(args.isEmpty())
				m_jackProcess->start(jackCmd);
			else
				m_jackProcess->start(jackCmd, args);
			bool rv = m_jackProcess->waitForStarted();
			printf("%s\n", rv ? "Complete" : "FAILED");
			return rv;
		}
		else
			return true;
	}
	return false;
}/*}}}*/

bool OOStudio::runLinuxsampler(OOSession* session)/*{{{*/
{
	if(session)
	{
		if(session->loadls)
		{
			printf("Launching linuxsampler ");
			m_lsProcess = new QProcess(this);
			connect(m_lsProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processLSMessages()));
			connect(m_lsProcess, SIGNAL(readyReadStandardError()), this, SLOT(processLSErrors()));
			QString lscmd = session->lscommand;
			QStringList lsargs = lscmd.split(" ");
			lscmd = lsargs.takeFirst();
			if(lsargs.isEmpty())
			{
				m_lsProcess->start(lscmd);
			}
			else
			{
				m_lsProcess->start(lscmd, lsargs);
			}
			if(!m_lsProcess->waitForStarted())
			{
				printf("FAILED\n");
				return false;
			}
			int retry = 0;
			bool rv = pingLinuxsampler(session);
			while(!rv && retry < 5)
			{
				sleep(1);
				rv = pingLinuxsampler(session);
				++retry;
			}
			printf("%s\n", rv ? "Complete" : "FAILED");
			return rv;
		}
		else
			return true; //You did not want it loaded here so all is good
	}
	return false;
}/*}}}*/

bool OOStudio::pingLinuxsampler(OOSession* session)
{
	if(session)
	{
		lscp_client_t* client = ::lscp_client_create(session->lshostname.toUtf8().constData(), session->lsport, client_callback, this);
		if(client == NULL)
			return false;
		printf("Query LS info: ");
		lscp_server_info_t* info = lscp_get_server_info(client);
		if(info == NULL)
		{
			printf("FAILED!!\n");
			return false;
		}
		else
		{
			printf("Description: %s, Version: %s, Protocol Version: %s\n", info->description, info->version, info->protocol_version);
			return true;
		}
	}
	return true;
}

bool OOStudio::loadLSCP(OOSession* session)
{
	if(session)
	{
		printf("Loading LSCP ");
		lscp_client_t* client = ::lscp_client_create(session->lshostname.toUtf8().constData(), session->lsport, client_callback, this);
		if(client == NULL)
			return false;
		QFile lsfile(session->lscpPath);
		if(!lsfile.open(QIODevice::ReadOnly))
		{
			//TODO:We need to error here
			lscp_client_destroy(client);
			printf("FAILED to open file\n");
			return false;
		}
		QTextStream in(&lsfile);
		QString command = in.readLine();
		while(!command.isNull())
		{
			if(!command.startsWith("#") && !command.isEmpty())
			{
				QString cmd = command.append("\r\n");
				::lscp_client_query(client, cmd.toUtf8().constData());
				//return true;
				/*if(::lscp_client_query(client, cmd.toUtf8().constData()) != LSCP_OK)
				{
					lsfile.close();
					lscp_client_destroy(client);
					printf("FAILED to perform query\n");
					printf("%s\n", command.toUtf8().constData());
					return false;
				}*/
			}
			command = in.readLine();
		}
		lsfile.close();
		lscp_client_destroy(client);
		printf("Complete\n");
		return true;
	}
	return false;
}

void OOStudio::runCommands(OOSession* session, bool post)/*{{{*/
{
	if(session)
	{
		QStringList commands = post ? session->postCommands : session->commands;
		foreach(QString command, commands)
		{
			QString cmd = session->lscommand;
			QStringList args = cmd.split(" ");
			cmd = args.takeFirst();
			OOProcess* process = new OOProcess(cmd, this);
			connect(process, SIGNAL(readyReadStandardOutput(QString, long)), this, SLOT(processCustomMessages(QString, long)));
			connect(process, SIGNAL(readyReadStandardError(QString, long)), this, SLOT(processCustomErrors(QString, long)));
			if(args.isEmpty())
			{
				process->start(cmd);
			}
			else
			{
				process->start(cmd, args);
			}
			if(process->waitForStarted())
			{
				long pid = process->pid();
				m_procMap[pid] = process;
			}
		}
	}
}/*}}}*/

bool OOStudio::runOOM(OOSession* session)/*{{{*/
{
	if(session)
	{
		printf("Loading OOMidi ");
		m_oomProcess = new QProcess(this);
		connect(m_oomProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processOOMMessages()));
		connect(m_oomProcess, SIGNAL(readyReadStandardError()), this, SLOT(processOOMErrors()));
		connect(m_oomProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processOOMExit(int, QProcess::ExitStatus)));
		QStringList args;
		args << session->songfile;
		m_oomProcess->start(QString(OOM_INSTALL_BIN), args);
		bool rv = m_oomProcess->waitForStarted();
		printf("%s\n", rv ? "Complete" : "FAILED");
		return rv;
	}
	return false;
}/*}}}*/

void OOStudio::loadSessionClicked()/*{{{*/
{
	if(m_sessionSelectModel && m_sessionSelectModel->hasSelection())
	{
		QModelIndexList	selected = m_sessionSelectModel->selectedRows(0);
		if(selected.size())
		{
			QModelIndex index = selected.at(0);
			QStandardItem* item = m_sessionModel->itemFromIndex(index);
			if(item)
			{
				OOSession* session = m_sessionMap.value(item->text());
				if(session)
				{
					loadSession(session);
				}
			}
		}
	}
}/*}}}*/

void OOStudio::loadTemplateClicked()/*{{{*/
{
	if(m_templateSelectModel && m_templateSelectModel->hasSelection())
	{
		QModelIndexList	selected = m_templateSelectModel->selectedRows(0);
		if(selected.size())
		{
			QModelIndex index = selected.at(0);
			QStandardItem* item = m_templateModel->itemFromIndex(index);
			if(item)
			{
				OOSession* session = m_sessionMap.value(item->text());
				if(session)
				{
					if(QMessageBox::warning(
						this,
						tr("Open Template"),
						tr("You are about to open a template session \nAre you sure you want to do this "),
						QMessageBox::Ok, QMessageBox::Cancel
					) == QMessageBox::Ok)
					{
						loadSession(session);
					}
				}
			}
		}
	}
}/*}}}*/

void OOStudio::loadSession(OOSession* session)
{
	if(session)
	{
		printf("OOStudio::loadSession : name: %s\n", session->name.toUtf8().constData());
		printf("OOStudio::loadSession : path: %s\n", session->path.toUtf8().constData());
		cleanupProcessList();
		if(runJack(session))
		{
			int retry = 0;
			bool lsrunning = runLinuxsampler(session);
			while(!lsrunning && retry < 5)
			{
				if(m_lsProcess)
				{
					m_lsProcess->kill();
					m_lsProcess = 0;
				}
				lsrunning = runLinuxsampler(session);
				++retry;
			}
			if(lsrunning)
			{
				retry = 0;
				bool lscpLoaded = loadLSCP(session);
				while(!lscpLoaded && retry < 5)
				{
					sleep(1);
					lscpLoaded = loadLSCP(session);
					++retry;
				}
				if(lscpLoaded)
				{
					runCommands(session);
					if(runOOM(session))
					{
						runCommands(session, true);
						hide();
					}
				}
			}
			else
			{
				m_current = 0;
				//TODO: report error and shutdown everything we started
			}
		}
	}
}

bool OOStudio::validateCreate()/*{{{*/
{
	if(m_txtName->text().isEmpty() || m_txtLocation->text().isEmpty() ||
		m_txtOOMPath->text().isEmpty() || m_txtLSCP->text().isEmpty() ||
		m_txtLSHost->text().isEmpty() || m_txtLSPort->text().isEmpty())
		return false;
	if((m_chkStartJack->isChecked() && m_txtJackCommand->text().isEmpty()) || 
		(m_chkStartLS->isChecked() && m_txtLSCommand->text().isEmpty()))
		return false;
	if(m_sessionMap.contains(m_txtName->text()))
	{
		QMessageBox::warning(
			this,
			tr("Dubplate Name"),
			tr("There is already a session with the name you selected\nPlease change the name and try again")
			);
		m_txtName->setText(getValidName(m_txtName->text()));
		m_txtName->setFocus(Qt::OtherFocusReason);
		return false;
	}
	return true;
}/*}}}*/

void OOStudio::createSession()
{
	if(validateCreate())
	{
		bool istemplate = m_chkTemplate->isChecked();
		QDomDocument doc("OOStudioSession");
		QDomElement root = doc.createElement("OOStudioSession");
		doc.appendChild(root);
		QString basepath = m_txtLocation->text();
		QString basename = m_txtName->text();
		basename = basename.simplified().replace(" ", "_");
		OOSession* newSession = new OOSession;
		newSession->name = basename;
		newSession->istemplate = istemplate;
		root.setAttribute("istemplate", istemplate);
		root.setAttribute("name", basename);

		QDir basedir(basepath);
		if(basedir.exists() && basedir.mkdir(basename) && basedir.cd(basename))
		{
			QString newPath(basedir.absolutePath());
			newPath.append(QDir::separator()).append(basename);

			QString strSong = m_txtOOMPath->text();
			bool oomok = QFile::copy(strSong, newPath+".oom");
			bool lscpok = QFile::copy(m_txtLSCP->text(), newPath+".lscp");
			if(oomok && lscpok)
			{
				QDomElement esong = doc.createElement("songfile");
				root.appendChild(esong);
				esong.setAttribute("path", newPath+".oom");
				newSession->songfile = newPath+".oom";

				QDomElement lscp = doc.createElement("lscpfile");
				root.appendChild(lscp);
				lscp.setAttribute("path", newPath+".lscp");
				lscp.setAttribute("mode", m_cmbLSCPMode->currentIndex());
				newSession->lscpPath = newPath+".lscp";
				newSession->lscpMode = m_cmbLSCPMode->currentIndex();

				QDomElement jack = doc.createElement("jackd");
				root.appendChild(jack);
				jack.setAttribute("path", m_txtJackCommand->text());
				jack.setAttribute("checked", m_chkStartJack->isChecked());
				newSession->loadJack = m_chkStartJack->isChecked();
				newSession->jackcommand = m_txtJackCommand->text();

				QDomElement lsnode = doc.createElement("linuxsampler");
				root.appendChild(lsnode);
				lsnode.setAttribute("path", m_txtLSCommand->text());
				lsnode.setAttribute("checked", m_chkStartLS->isChecked());
				lsnode.setAttribute("hostname", m_txtLSHost->text());
				lsnode.setAttribute("port", m_txtLSPort->text());
				newSession->loadls = m_chkStartLS->isChecked();
				newSession->lscommand = m_txtLSCommand->text();
				newSession->lshostname = m_txtLSHost->text();
				newSession->lsport = m_txtLSPort->text().toInt();

				for(int i= 0; i < m_commandModel->rowCount(); ++i)
				{
					QStandardItem* item = m_commandModel->item(i);
					QDomElement cmd = doc.createElement("command");
					root.appendChild(cmd);
					cmd.setAttribute("path", item->text());
					newSession->commands << item->text();
				}
				//QString xml = doc.toString();
				QString filename(newPath+".oos");
				QFile xmlfile(filename);
				if(xmlfile.open(QIODevice::WriteOnly) )
				{
					QTextStream ts( &xmlfile );
					ts << doc.toString();

					xmlfile.close();
					//TODO: Write out the session to settings and update table
					newSession->path = filename;
					m_sessionMap[basename] = newSession;
					QStandardItem* nitem = new QStandardItem(basename);
					QStandardItem* npath = new QStandardItem(filename);
					QList<QStandardItem*> rowData;
					rowData << nitem << npath;
					if(istemplate)
					{
						m_templateModel->appendRow(rowData);
					}
					else
					{
						m_sessionModel->appendRow(rowData);
						loadSession(newSession);
						//Not a template so load it now and minimize;
					}
					updateHeaders();
					saveSettings();
					resetCreate();
				}
			}
			else
			{//Failed run cleanup
			}
		}
	}
}

void OOStudio::templateSelectionChanged(int index)/*{{{*/
{
	QString tpath = m_cmbTemplate->itemText(index);
	printf("OOStudio::templateSelectionChanged index: %d, name: %s sessions:%d\n", index, tpath.toUtf8().constData(), m_sessionMap.size());
	if(index == 0)
	{
		resetCreate(false);
		return;
	}
	//Copy the values from the other template into this
	if(!m_sessionMap.isEmpty() && m_sessionMap.contains(tpath))
	{
		OOSession* session = m_sessionMap.value(tpath);
		if(session)
		{
			for(int i = 0; i < session->commands.size(); ++i)
			{
				QStandardItem* item = new QStandardItem(session->commands.at(i));
				item->setCheckable(true);
				m_commandModel->appendRow(item);
			}
			for(int i = 0; i < session->postCommands.size(); ++i)
			{
				QStandardItem* item = new QStandardItem(session->postCommands.at(i));
				item->setCheckable(true);
				m_commandModel->appendRow(item);
			}
			m_chkStartJack->setChecked(session->loadJack);
			m_txtJackCommand->setText(session->jackcommand);
			m_chkStartLS->setChecked(session->loadls);
			m_txtLSCommand->setText(session->lscommand);
			m_txtLSHost->setText(session->lshostname);
			m_txtLSPort->setText(QString::number(session->lsport));
			m_cmbLSCPMode->setCurrentIndex(session->lscpMode);
			m_txtLSCP->setText(session->lscpPath);
			m_txtOOMPath->setText(session->songfile);
			updateHeaders();
		}
	}
}/*}}}*/

OOSession* OOStudio::readSession(QString filename)/*{{{*/
{
	OOSession* session = 0;
	QDomDocument temp("OOStudioSession");
	QFile tempFile(filename);
	if(!tempFile.open(QIODevice::ReadOnly))
	{
		//TODO:We need to error here
		return session;
	}
	if(!temp.setContent(&tempFile))
	{
		//TODO:We need to error here
		return session;
	}
	//We now have a loaded file so we get the data from it.
	QDomElement root = temp.documentElement();
	if(!root.isNull())
	{
		session = new OOSession;
		session->name = root.attribute("name");
		session->istemplate = root.attribute("istemplate").toInt();
		session->path = filename;
		QDomNodeList flist = root.elementsByTagName("command");
		for(int i = 0; i < flist.count(); ++i)
		{
			QDomElement command = flist.at(i).toElement();
			bool post = command.attribute("post").toInt();
			if(post)
				session->postCommands.append(command.attribute("path"));
			else
				session->commands.append(command.attribute("path"));
		}
		QDomNodeList ljack = root.elementsByTagName("jackd");
		for(int i = 0; i < ljack.count(); ++i)
		{
			QDomElement jack = ljack.at(i).toElement();
			session->loadJack = jack.attribute("checked").toInt();
			session->jackcommand = jack.attribute("path");
		}
		QDomNodeList lls = root.elementsByTagName("linuxsampler");
		for(int i = 0; i < lls.count(); ++i)
		{
			QDomElement ls = lls.at(i).toElement();
			session->loadls = ls.attribute("checked").toInt();
			session->lscommand = ls.attribute("path");
			session->lshostname = ls.attribute("hostname", "localhost");
			session->lsport = ls.attribute("port", "8888").toInt();
		}

		QDomNodeList llscp = root.elementsByTagName("lscpfile");
		for(int i = 0; i < llscp.count(); ++i)
		{
			QDomElement ls = llscp.at(i).toElement();
			session->lscpPath = ls.attribute("path");
			session->lscpMode = ls.attribute("mode").toInt();
		}
		QDomNodeList lsong = root.elementsByTagName("songfile");
		for(int i = 0; i < lsong.count(); ++i)
		{
			QDomElement song = lsong.at(i).toElement();
			session->songfile = song.attribute("path");
		}
		return session;
	}
	return 0;
}/*}}}*/

void OOStudio::setVisible(bool visible)/*{{{*/
{
	minimizeAction->setEnabled(visible);
	maximizeAction->setEnabled(!isMaximized());
	restoreAction->setEnabled(isMaximized() || !visible);
	QDialog::setVisible(visible);
}/*}}}*/

void OOStudio::closeEvent(QCloseEvent* ev)/*{{{*/
{
	if(trayIcon->isVisible())
	{
		hide();
		ev->ignore();
	}
}/*}}}*/

void OOStudio::iconActivated(QSystemTrayIcon::ActivationReason reason)/*{{{*/
{
	switch (reason) {
		case QSystemTrayIcon::Trigger:
		case QSystemTrayIcon::DoubleClick:
			setVisible(!isVisible());
	    break;
		case QSystemTrayIcon::MiddleClick:
		break;
		default:
    		;
	}
}/*}}}*/

void OOStudio::processOOMExit(int code, QProcess::ExitStatus status)
{
	if(!m_incleanup)
	{
		switch(status)
		{
			case QProcess::NormalExit:
			{
				restoreAction->trigger();
				cleanupProcessList();
			}
			break;
			case QProcess::CrashExit:
			{
				cleanupProcessList();
				restoreAction->trigger();
			}
			break;
		}
	}
	qDebug() << "OOMidi exited with error code: " << code;
}

void OOStudio::processJackMessages()/*{{{*/
{
	processMessages(0, tr("Jackd"), m_jackProcess);
}/*}}}*/

void OOStudio::processJackErrors()/*{{{*/
{
	processMessages(1, tr("Jackd"), m_jackProcess);
}/*}}}*/

void OOStudio::processLSMessages()/*{{{*/
{
	processMessages(0, tr("LinuxSampler"), m_lsProcess);
}/*}}}*/

void OOStudio::processLSErrors()/*{{{*/
{
	processMessages(1, tr("LinuxSampler"), m_lsProcess);
}/*}}}*/

void OOStudio::processOOMMessages()/*{{{*/
{
	processMessages(0, tr("OOMidi"), m_oomProcess);
}/*}}}*/

void OOStudio::processOOMErrors()/*{{{*/
{
	processMessages(1, tr("OOMidi"), m_oomProcess);
}/*}}}*/

void OOStudio::processCustomMessages(QString name, long pid)/*{{{*/
{
	if(!m_procMap.isEmpty() && m_procMap.contains(pid))
	{
		processMessages(0, name, m_procMap.value(pid));
	}
}/*}}}*/

void OOStudio::processCustomErrors(QString name, long pid)/*{{{*/
{
	if(!m_procMap.isEmpty() && m_procMap.contains(pid))
	{
		processMessages(1, name, m_procMap.value(pid));
	}
}/*}}}*/

void OOStudio::processMessages(int type, QString name, QProcess* p)/*{{{*/
{
	if(p)
	{
		switch(type)
		{
			case 0: //Output
			{
				QString messages = QString::fromUtf8(p->readAllStandardOutput().constData());
				QList<QStandardItem*> rowData;
				QStandardItem* command = new QStandardItem(name);
				QStandardItem* log = new QStandardItem(messages);
				rowData << command << log;
				m_loggerModel->appendRow(rowData);
				updateHeaders();
			}
			break;
			case 1: //Error
			{
				QString messages = QString::fromUtf8(p->readAllStandardError().constData());
				QList<QStandardItem*> rowData;
				QStandardItem* command = new QStandardItem(name);
				command->setBackground(QColor(119, 62, 62));
				QStandardItem* log = new QStandardItem(messages);
				rowData << command << log;
				m_loggerModel->appendRow(rowData);
				updateHeaders();
			}
			break;
		}
	}
}/*}}}*/
