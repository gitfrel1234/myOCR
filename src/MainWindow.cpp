#include "MainWindow.h"

#include <QtGui>
#include "ImageWidget.h"
#include "ImageAdapter.h"


MainWindow::MainWindow()
{
	mImage = NULL;
	tessBaseAPI = NULL;
	boxes = NULL;
	mEnableMasks = false;

	resize(960, 600);

	imageWidget = new ImageWidget();
	textEdit = new QTextEdit();
	textEdit->setFixedWidth(200);
	QFont fontTextEdit = textEdit->font();
	fontTextEdit.setPixelSize(18);
	textEdit->setFont(fontTextEdit);

	QHBoxLayout* hbox = new QHBoxLayout();

	QScrollArea* scrollArea = new QScrollArea();
	scrollArea->setWidget(imageWidget);
	scrollArea->setWidgetResizable(true);
	hbox->addWidget(scrollArea,1);
	hbox->addWidget(textEdit);
	
	QWidget* centerWidget = new QWidget();
	centerWidget->setLayout(hbox);
	setCentralWidget(centerWidget);

	menuFile = menuBar()->addMenu(tr("File"));
	menuImage = menuBar()->addMenu(tr("Image"));
	menuHelp = menuBar()->addMenu(tr("Help"));

	toolBarFile = addToolBar(tr("File"));
	toolBarFile->setMovable(false);
	toolBarImage = addToolBar(tr("Image"));
	toolBarImage->setMovable(false);
	
	actionOpenImage = new QAction(tr("Open Image"), this);
	actionSaveImage = new QAction(tr("Save Image"), this);
	actionRecognizeText = new QAction(tr("Recognize Text"), this);
	actionProcessImage = new QAction(tr("Process Image"), this);
	actionAbout = new QAction(tr("About"), this);
	actionEnableMasks = new QAction(tr("Enable Masks"), this);
	actionEnableMasks->setCheckable(true);
	actionSaveMasks = new QAction(tr("Save Masks"), this);
	actionLoadMasks = new QAction(tr("Load Masks"), this);
	actionClearMasks = new QAction(tr("Clear Masks"), this);
	actionEnableMesure = new QAction(tr("Enable Mesure"), this);
	actionEnableMesure->setCheckable(true);
	actionEnableChinese = new QAction(tr("Enable Chinese"), this);
	actionEnableChinese->setCheckable(true);
	
	actionOpenImage->setIcon(QIcon(":/open.png"));
	actionSaveImage->setIcon(QIcon(":/save.png"));
	actionRecognizeText->setIcon(QIcon(":/ocr.png"));
	actionProcessImage->setIcon(QIcon(":/paint.png"));
	actionEnableMasks->setIcon(QIcon(":/new2.png"));
	actionSaveMasks->setIcon(QIcon(":/save2.png"));
	actionLoadMasks->setIcon(QIcon(":/load2.png"));
	actionClearMasks->setIcon(QIcon(":/clear2.png"));
	actionEnableMesure->setIcon(QIcon(":/mesure.png"));
	actionEnableChinese->setIcon(QIcon(":/chinese.png"));

	connect(actionOpenImage, SIGNAL(triggered()), this, SLOT(openImageFile()));
	connect(actionSaveImage, SIGNAL(triggered()), this, SLOT(saveImageFile()));
	connect(actionRecognizeText, SIGNAL(triggered()), this, SLOT(recognizeText()));
	connect(actionProcessImage, SIGNAL(triggered()), this, SLOT(processImage()));
	connect(actionEnableMasks, SIGNAL(triggered()), this, SLOT(enableMasks()));
	connect(actionSaveMasks, SIGNAL(triggered()), this, SLOT(saveMasks()));
	connect(actionLoadMasks, SIGNAL(triggered()), this, SLOT(loadMasks()));
	connect(actionClearMasks, SIGNAL(triggered()), this, SLOT(clearMasks()));
	connect(actionEnableMesure, SIGNAL(triggered()), this, SLOT(enableMesure()));
	connect(actionEnableChinese, SIGNAL(triggered()), this, SLOT(enableChinese()));
	
	menuFile->addAction(actionOpenImage);
	menuFile->addAction(actionSaveImage);
	menuImage->addAction(actionEnableMesure);
	menuImage->addSeparator();
	menuImage->addAction(actionEnableMasks);
	menuImage->addAction(actionSaveMasks);
	menuImage->addAction(actionLoadMasks);
	menuImage->addAction(actionClearMasks);
	menuImage->addSeparator();
	menuImage->addAction(actionProcessImage);
	menuImage->addAction(actionRecognizeText);
	menuImage->addSeparator();
	menuImage->addAction(actionEnableChinese);
	menuHelp->addAction(actionAbout);
	
	toolBarFile->addAction(actionOpenImage);
	toolBarFile->addAction(actionSaveImage);
	toolBarImage->addAction(actionEnableMesure);
	toolBarImage->addSeparator();
	toolBarImage->addAction(actionEnableMasks);
	toolBarImage->addAction(actionSaveMasks);
	toolBarImage->addAction(actionLoadMasks);
	toolBarImage->addAction(actionClearMasks);
	toolBarImage->addSeparator();
	toolBarImage->addAction(actionProcessImage);
	toolBarImage->addAction(actionRecognizeText);
	toolBarImage->addSeparator();
	toolBarImage->addAction(actionEnableChinese);
}

MainWindow::~MainWindow()
{
	if(mImage)
	{
		cvReleaseImage(&mImage);
	}
	if(tessBaseAPI)
	{
		tessBaseAPI->End();
	}
}

void MainWindow::openImageFile()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open Image"), ".",
		tr("Image files (*.png *.jpg *.bmp *.tiff)"));
    if (!fileName.isEmpty())
	{
		QTextCodec::setCodecForCStrings(QTextCodec::codecForName("gbk"));//解决中文路径的问题
		if(mImage)
		{
			cvReleaseImage(&mImage);
		}
		mImage = cvLoadImage(fileName.toStdString().c_str());
		if(mImage)
		{
			QImage* image = ImageAdapter::IplImage2QImage(mImage);
			imageWidget->setImage(image);
		}
	}
}

void MainWindow::saveImageFile()
{
	QString fileName = QFileDialog::getSaveFileName(this,
		tr("Save Image"), ".",
		tr("Image files (*.png *.jpg *.bmp *.tiff)"));
    if (!fileName.isEmpty())
	{
		QTextCodec::setCodecForCStrings(QTextCodec::codecForName("gbk"));//解决中文路径的问题
		if(mImage)
		{
			cvSaveImage(fileName.toStdString().c_str(), mImage);
		}
	}
}

void MainWindow::enableMesure()
{
	imageWidget->enableMesure(actionEnableMesure->isChecked());
}

void MainWindow::enableChinese()
{
	
}

void MainWindow::enableMasks()
{
	mEnableMasks = actionEnableMasks->isChecked();
	imageWidget->enableMasks(mEnableMasks);
}

void MainWindow::saveMasks()
{
	QVector<QRect>* masks = imageWidget->getMasks();
	if(masks->size() == 0)
	{
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setText(tr("No masks exists.\nPlease add some masks before save."));
		msgBox.exec();
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(this,
		tr("Save Masks"), ".",
		tr("XML files (*.xml)"));
    if (!fileName.isEmpty())
	{
		QFile file(fileName);
		if(!file.open(QIODevice::WriteOnly))
		{
			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setText(tr("Cannot open file to write.\nMaybe this file is read only."));
			msgBox.exec();
			return;
		}
		QXmlStreamWriter* xmlWriter = new QXmlStreamWriter();
		xmlWriter->setDevice(&file);
		xmlWriter->writeStartDocument();
		xmlWriter->writeStartElement("masks");

		for(int i=0; i<masks->size(); i++)
		{
			QRect rect = masks->at(i);
			xmlWriter->writeStartElement("mask");

			xmlWriter->writeAttribute("x", QString::number(rect.x()));
			xmlWriter->writeAttribute("y", QString::number(rect.y()));
			xmlWriter->writeAttribute("w", QString::number(rect.width()));
			xmlWriter->writeAttribute("h", QString::number(rect.height()));

			xmlWriter->writeEndElement();
		}

		xmlWriter->writeEndElement();
		xmlWriter->writeEndDocument();
        delete xmlWriter;
	}
}

void MainWindow::loadMasks()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Load Masks"), ".",
		tr("XML files (*.xml)"));
    if (!fileName.isEmpty())
	{
		QFile file(fileName);
		if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setText(tr("Cannot open file to read.\nMaybe this file does not exist."));
			msgBox.exec();
			return;
		}

		QVector<QRect>* masks = imageWidget->getMasks();
		masks->clear();

		QXmlStreamReader xml;
		xml.setDevice(&file);

		while(!xml.atEnd() && !xml.hasError())
		{
			QXmlStreamReader::TokenType token = xml.readNext();
			if(token == QXmlStreamReader::StartDocument)
			{
				continue;
			}
			if(token == QXmlStreamReader::StartElement)
			{
				if(xml.name() == "masks")
				{
					continue;
				}
				if(xml.name() == "mask")
				{
					QRect rect;
					QXmlStreamAttributes attributes = xml.attributes();
					rect.setX(attributes.value("x").toString().toInt());
					rect.setY(attributes.value("y").toString().toInt());
					rect.setWidth(attributes.value("w").toString().toInt());
					rect.setHeight(attributes.value("h").toString().toInt());

					masks->push_back(rect);
				}
			}
		}
		if(xml.hasError())
		{
			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setText(tr("Parse XML failed.\nMaybe this file is not the right one."));
			msgBox.exec();
		}
		xml.clear();
		imageWidget->update();
	}
}

void MainWindow::clearMasks()
{
	imageWidget->getMasks()->clear();
	imageWidget->update();
}

void MainWindow::processImage()
{
	if(mImage == NULL)
	{
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setText(tr("No image loaded.\nPlease load an image file before OpenCV."));
		msgBox.exec();
		return;
	}

	IplImage* grayImage = cvCreateImage(cvGetSize(mImage), 8, 1);
	if(mImage->nChannels == 3)
	{
		cvCvtColor(mImage, grayImage, CV_RGB2GRAY);
		cvReleaseImage(&mImage);
		mImage = grayImage;
	}
	
	//cvAdaptiveThreshold(mImage, mImage, 255);

	QImage* image = ImageAdapter::IplImage2QImage(mImage);
	imageWidget->setImage(image);
}

void MainWindow::recognizeText()
{
	if(!mImage)
	{
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setText(tr("No image loaded.\nPlease load an image file before OCR."));
		msgBox.exec();
		return;
	}
	else
	{
		//设置环境变量TESSDATA_PREFIX
		if(tessBaseAPI == NULL)
		{
			tessBaseAPI = new tesseract::TessBaseAPI(); 
		}
		//或者在Init函数中设置datapath
		if(actionEnableChinese->isChecked())
		{

			if (tessBaseAPI->Init(NULL, "chi_sim+eng")) {
				QMessageBox msgBox;
				msgBox.setIcon(QMessageBox::Critical);
				msgBox.setText(tr("Could not initialize tesseract: chinese"));
				msgBox.exec();
				return;
			}
			if(true)
			{
				tessBaseAPI->SetVariable("chop_enable", "T");
				tessBaseAPI->SetVariable("use_new_state_cost", "F");
				tessBaseAPI->SetVariable("segment_segcost_rating", "F");
				tessBaseAPI->SetVariable("enable_new_segsearch", "0");
				tessBaseAPI->SetVariable("language_model_ngram_on", "0");
				tessBaseAPI->SetVariable("textord_force_make_prop_words", "F");
			}
		}
		else
		{
			if (tessBaseAPI->Init(NULL, "eng")) {
				QMessageBox msgBox;
				msgBox.setIcon(QMessageBox::Critical);
				msgBox.setText(tr("Could not initialize tesseract: english"));
				msgBox.exec();
				return;
			}
			if(true)
			{
				tessBaseAPI->SetVariable("tessedit_char_whitelist", "0123456789.+-*/<>");
			}
		}

		tessBaseAPI->SetImage((uchar*)mImage->imageData, mImage->width, mImage->height, mImage->nChannels, mImage->widthStep);

		QVector<QRect>* masks = imageWidget->getMasks();
		if(masks->size() == 0)
		{

			boxes = tessBaseAPI->GetComponentImages(tesseract::RIL_SYMBOL, true, NULL, NULL);
		
			QRect* rects = new QRect[boxes->n];
			for(int i = 0; i< boxes->n; i++)
			{
				rects[i].setX(boxes->box[i]->x);
				rects[i].setY(boxes->box[i]->y);
				rects[i].setWidth(boxes->box[i]->w);
				rects[i].setHeight(boxes->box[i]->h);
			}
			imageWidget->setBoxes(rects, boxes->n);

			char *outText = tessBaseAPI->GetUTF8Text();

			QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf-8"));
			textEdit->setText(outText);
		}
		else
		{
			textEdit->clear();

			for(int i=0;i<masks->size();i++)
			{
				QRect rect = masks->at(i);
				tessBaseAPI->SetRectangle(rect.x(), rect.y(), rect.width(), rect.height());
				char* ocrResult = tessBaseAPI->GetUTF8Text();
				int conf = tessBaseAPI->MeanTextConf();
				QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf-8"));
				textEdit->setText(QString("%1%2%3").arg(textEdit->toPlainText(),"\n",ocrResult));
			}
		}
	}
}