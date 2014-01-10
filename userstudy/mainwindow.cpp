#include "DemoGlobal.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Controls.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	// Anti-aliasing when using QGLWidget or subclasses
	QGLFormat glf = QGLFormat::defaultFormat();
	glf.setSamples(8);
	QGLFormat::setDefaultFormat(glf);

    ui->setupUi(this);

	ui->logWidget->setVisible(false);

	// Change window to preferred size
	this->ui->createWidget->setMinimumSize(852, 480);
	this->resize(this->sizeHint());

	// Center to screen
	QDesktopWidget* m = QApplication::desktop();
	QRect desk_rect = m->screenGeometry(m->screenNumber(QCursor::pos()));
	int desk_x = desk_rect.width();
	int desk_y = desk_rect.height();
	int x = this->width();
	int y = this->height();
	this->move(desk_x / 2 - x / 2 + desk_rect.left(), desk_y / 2 - y / 2 + desk_rect.top());

	this->connect(ui->mainTabWidget, SIGNAL(currentChanged ( int )), SLOT(currentTabChanged ( int )));
	
	this->connect(ui->nextButtonWelcome, SIGNAL(clicked()), SLOT(nextTab()));
	this->connect(ui->nextButtonTutorial, SIGNAL(clicked()), SLOT(nextTab()));

	// Start at welcome page
	this->ui->mainTabWidget->setCurrentIndex(0);
}

void MainWindow::currentTabChanged(int index)
{
	lockTabs(index);

	switch(index)
	{
	case 0: // Welcome
		break;
	case 1: // Tutorial
		break;
	case 2: // Create
		prepareCreate();
		break;
	case 3: // Finish
		break;
	}
}

void MainWindow::prepareCreate()
{
	this->ui->createWidget->setMinimumSize(ui->createWidth->geometry().width(), 480);
	prepareDemo();
	this->ui->createWidget->setMinimumSize(0,0);

	// Load data
	{
		dataset = getDataset( "data" );
	}

	// Load tasks
	{
		QString tasksFilename = "data/tasks.txt";
		QFile file(tasksFilename); 
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

		QTextStream in(&file);
		while (!in.atEnd()){
			QStringList line = in.readLine().split(" ");

			QString taskName = line.takeFirst();
			QVector<PropertyMap> taskData;
			foreach(QString d, line) taskData.push_back( dataset[d] );

			session->tasks.push_back( StudyTask(taskName, taskData, session) );
		}
	}

	session->connect(ui->nextButtonCreate, SIGNAL(clicked()), SLOT(displayNextTask()));
	this->connect(session, SIGNAL(allTasksDone()), SLOT(nextTab()));

	this->setMinimumSize(this->width(), this->height());

	session->displayNextTask();
}

void MainWindow::nextTab()
{
	ui->mainTabWidget->setCurrentIndex( ui->mainTabWidget->currentIndex() + 1 );
}

void MainWindow::lockTabs(int except){
	for (int i=0; i<ui->mainTabWidget->count(); i++) {
		if (i!=except) 
			ui->mainTabWidget->setTabEnabled(i, false);
		else
			ui->mainTabWidget->setTabEnabled(i, true);
	}
}

void MainWindow::unlockTabs() {
	for (int i=0; i<ui->mainTabWidget->count(); i++) {
		ui->mainTabWidget->setTabEnabled(i, true);
	}
}

void MainWindow::prepareDemo()
{
	viewport = new QGLWidget();
	viewport->makeCurrent();

	viewport->setFixedSize(ui->createWidget->width(), ui->createWidget->height());

	ui->graphicsView->setViewport( viewport );
	ui->graphicsView->setViewportUpdateMode( QGraphicsView::FullViewportUpdate );
	ui->graphicsView->setScene( (scene = new Scene(this)) );
	ui->graphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	ui->graphicsView->setRenderHint(QPainter::HighQualityAntialiasing, true);
	ui->graphicsView->setRenderHint(QPainter::SmoothPixmapTransform, true);

	scene->setSceneRect(0,0, ui->createWidget->width(), ui->createWidget->height());

	this->connect(scene, SIGNAL(message(QString)), SLOT(message(QString)));
	this->connect(scene, SIGNAL(keyUpEvent(QKeyEvent*)), SLOT(keyUpEvent(QKeyEvent*)));

	// Create controls
	control = new Controls;
	QGraphicsProxyWidget * proxy = scene->addWidget(control);
	proxy->setPos((	scene->width() - proxy->boundingRect().width()) * 0.5,
					scene->height() - proxy->boundingRect().height());

	scene->setProperty("controlsWidgetHeight", control->height());

	Matcher * matcher = new Matcher(scene, "Match parts");
	Blender * blender = new Blender(scene, "Blended shapes");

	// Connect all pages to logger
	this->connect(matcher, SIGNAL(message(QString)), SLOT(message(QString)));
	this->connect(blender, SIGNAL(message(QString)), SLOT(message(QString)));

	// Place log window nicely
	this->ui->logWidget->setGeometry(geometry().x() - (width() * 0.21), geometry().y(), width() * 0.2, height() * 0.5);

	// Connect matcher and blender
	blender->connect(matcher, SIGNAL(corresponderCreated(GraphCorresponder *)), SLOT(setGraphCorresponder(GraphCorresponder *)), Qt::DirectConnection);
	this->connect(blender, SIGNAL(showLogWindow()), SLOT(showLogWindow()));

	// Create session
	session = new Session(scene, control, matcher, blender, this);
	scene->connect(session, SIGNAL(update()), SLOT(update()));

	// Everything is ready, load shapes now:
	QString datasetFolder = "dataset";
	DatasetMap datasetMap = getDataset(datasetFolder);

	// Ask user for path of dataset
	while( datasetMap.isEmpty() ){
		datasetFolder = QFileDialog::getExistingDirectory();
		datasetMap = getDataset(datasetFolder);
		if(datasetMap.isEmpty())
			QMessageBox::critical(this, "Cannot find data", "Cannot find data in this folder. Please restart and choose a correct one.");
	}

	control->loadCategories( datasetFolder );
}

MainWindow::~MainWindow()
{
    delete ui;
}

DatasetMap MainWindow::getDataset(QString datasetPath)
{
    DatasetMap dataset;

    QDir datasetDir(datasetPath);
    QStringList subdirs = datasetDir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);

    foreach(QString subdir, subdirs)
    {
		// Special folders
		if(subdir == "corr") continue;

        QDir d(datasetPath + "/" + subdir);

		// Check if no graph is in this folder
		if( d.entryList(QStringList() << "*.xml", QDir::Files).isEmpty() ) continue;

        dataset[subdir]["Name"] = subdir;
        dataset[subdir]["graphFile"] = d.absolutePath() + "/" + d.entryList(QStringList() << "*.xml", QDir::Files).join("");
        dataset[subdir]["thumbFile"] = d.absolutePath() + "/" + d.entryList(QStringList() << "*.png", QDir::Files).join("");
        dataset[subdir]["objFile"] = d.absolutePath() + "/" + d.entryList(QStringList() << "*.obj", QDir::Files).join("");
    }

    return dataset;
}

void MainWindow::message(QString m)
{
	ui->logger->addItem(m);
	ui->logger->scrollToBottom();
}

void MainWindow::keyUpEvent(QKeyEvent* keyEvent)
{
	// Toggle log window visibility
	if(keyEvent->key() == Qt::Key_L)
	{
		ui->logWidget->setVisible(!ui->logWidget->isVisible());
	}
}

void MainWindow::showLogWindow()
{
	ui->logWidget->setVisible(true);
}
