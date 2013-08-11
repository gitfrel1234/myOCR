#include <QtCore/QCoreApplication>
#include <QTextCodec>
#include <QtCore/QDebug>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	qDebug()<<"hello qt!";
	
	char *outText;
	tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
	if (api->Init(NULL, "sim")) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }

	Pix *image = pixRead("C:\\Users\\liqilei\\Desktop\\test3.png");
    api->SetImage(image);
    // Get OCR result
    outText = api->GetUTF8Text();
	qDebug()<<outText;
    // Destroy used object and release memory
    api->End();
    //delete [] outText;
    pixDestroy(&image);
	
	app.exec();
    return 0;
}