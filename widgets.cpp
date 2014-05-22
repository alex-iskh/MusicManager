#include "widgets.h"
#include "musmanager.h"

namespace MusM {

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    setWindowTitle("Music Manager");
    resize(800, 600);

    QPushButton* editTags = new QPushButton("Edit tags by filenames");
    QPushButton* editFilenames = new QPushButton("Edit filenames by tags");
    QPushButton* deleteTags = new QPushButton("Delete tags");
    QPushButton* setArtist = new QPushButton("Set artist");
    QPushButton* setAlbum = new QPushButton("Set album");
    QPushButton* setGenre = new QPushButton("Set genre");
    //QPushButton* browse = new QPushButton("Browse...");
    PathLine* path = new PathLine;
    ElementList* dirList = new ElementList;
    ElementList* songList = new ElementList;

    QVBoxLayout* column = new QVBoxLayout;//think of realizing it with QSplitter
    QGridLayout* funcBar = new QGridLayout;
    QHBoxLayout* pathBar = new QHBoxLayout;
    funcBar->addWidget(editTags, 0, 0);
    funcBar->addWidget(editFilenames, 0, 1);
    funcBar->addWidget(deleteTags, 0, 2);
    funcBar->addWidget(setArtist, 1, 0);
    funcBar->addWidget(setAlbum, 1, 1);
    funcBar->addWidget(setGenre, 1, 2);
    //pathBar->addWidget(browse);
    pathBar->addWidget(path);
    column->addLayout(funcBar, 2);
    column->addLayout(pathBar, 1);
    column->addWidget(dirList, 2);
    column->addWidget(songList, 10);

    QWidget* area = new QWidget;
    area->setLayout(column);
    setCentralWidget(area);

    dirList->setWindowTitle("dirList");
    songList->setWindowTitle("songList");
    dirList->setColumnCount(1);
    songList->setColumnCount(5);
    QStringList headers;
    headers<<"Directories";
    dirList->setHorizontalHeaderLabels(headers);
    headers.clear();
    headers<<"Filename"<<"Name"<<"Artist"<<"Album"<<"Genre";
    songList->setHorizontalHeaderLabels(headers);

    FuncWnd* editTagsWnd = new FuncWnd(this);
    editTagsWnd->setWindowTitle("Editing tags of files in directory");
    editTagsWnd->setLabelText("Press OK to fill tags of all mp3 files in directory from filenames by temolplate from the string below\n\nYou may use following expressions:\n<num> - to point to a number in a current order\n<name> - to point to a song name\n<artist> - to point to a name of performer\n<album> - to point to album name\n<genre> - to point to a genre\nSymbols \\, /, :, *, ?, \", | and <, > (out of expressions above) are forbidden");
    FuncWnd* editFilenamesWnd = new FuncWnd(this);
    editFilenamesWnd->setWindowTitle("Editing filenames of files in directory");
    editFilenamesWnd->setLabelText("Press OK to set filenames of all mp3 files in directory to form from the string below\n\nYou may use following expressions:\n<num> - to insert a number in a current order\n<name> - to insert a song name\n<artist> - to insert a name of performer\n<album> - to insert album name\n<genre> - to insert a genre\nSymbols \\, /, :, *, ?, \", | and <, > (out of expressions above) are forbidden");
    FuncWnd* deleteTagsWnd = new FuncWnd(this);
    deleteTagsWnd->setWindowTitle("Deleting tags from all files in directory");
    deleteTagsWnd->setLabelText("If you press OK, all main tags in mp3 files will be deleted. Are you sure?");
    FuncWnd* setArtistWnd = new FuncWnd(this);
    setArtistWnd->setWindowTitle("Set one artist to all files in directory");
    setArtistWnd->setLabelText("If you press OK, tags \"artist\" in all mp3 files in derectory will be filled with the value in the string below");
    FuncWnd* setAlbumWnd = new FuncWnd(this);
    setAlbumWnd->setWindowTitle("Set one album to all files in directory");
    setAlbumWnd->setLabelText("If you press OK, tags \"album\" in all mp3 files in derectory will be filled with the value in the string below");
    FuncWnd* setGenreWnd = new FuncWnd(this);
    setGenreWnd->setWindowTitle("Set one genre to all files in directory");
    setGenreWnd->setLabelText("If you press OK, tags \"genre\" in all mp3 files in derectory will be filled with the value in the string below");

    connect(editTags, SIGNAL(clicked()), editTagsWnd, SLOT(openMenu()));
    connect(editFilenames, SIGNAL(clicked()), editFilenamesWnd, SLOT(openMenu()));
    connect(deleteTags, SIGNAL(clicked()), deleteTagsWnd, SLOT(openMenu()));
    connect(setArtist, SIGNAL(clicked()), setArtistWnd, SLOT(openMenu()));
    connect(setAlbum, SIGNAL(clicked()), setAlbumWnd, SLOT(openMenu()));
    connect(setGenre, SIGNAL(clicked()), setGenreWnd, SLOT(openMenu()));
    //connect(browse, SIGNAL(clicked()), this, SLOT(openBrowseWnd()));
    connect(path, SIGNAL(editingFinished()), path, SLOT(handleLine()));
    connect(path, SIGNAL(pathChanged()), dirList, SLOT(openDir()));
    connect(path, SIGNAL(pathChanged()), songList, SLOT(openDir()));
    connect(songList, SIGNAL(procedureProceeded()), songList, SLOT(openDir()));

    connect(editTagsWnd, SIGNAL(accepted()), editTagsWnd, SLOT(prepareData()));
    connect(editTagsWnd, SIGNAL(transferData(char,QString)), songList, SLOT(executeProcedure(char,QString)));
    connect(editFilenamesWnd, SIGNAL(accepted()), editFilenamesWnd, SLOT(prepareData()));
    connect(editFilenamesWnd, SIGNAL(transferData(char,QString)), songList, SLOT(executeProcedure(char,QString)));
    connect(deleteTagsWnd, SIGNAL(accepted()), deleteTagsWnd, SLOT(prepareData()));
    connect(deleteTagsWnd, SIGNAL(transferData(char,QString)), songList, SLOT(executeProcedure(char,QString)));
    connect(setArtistWnd, SIGNAL(accepted()), setArtistWnd, SLOT(prepareData()));
    connect(setArtistWnd, SIGNAL(transferData(char,QString)), songList, SLOT(executeProcedure(char,QString)));
    connect(setAlbumWnd, SIGNAL(accepted()), setAlbumWnd, SLOT(prepareData()));
    connect(setAlbumWnd, SIGNAL(transferData(char,QString)), songList, SLOT(executeProcedure(char,QString)));
    connect(setGenreWnd, SIGNAL(accepted()), setGenreWnd, SLOT(prepareData()));
    connect(setGenreWnd, SIGNAL(transferData(char,QString)), songList, SLOT(executeProcedure(char,QString)));
}

void PathLine::handleLine() //при вводе нового пути проверяем его существования и если не сущ., то возвр. старый путь
{
    if (this->text() == curDir) {return;}
    if ((this->text()).isEmpty()) {this->setText(curDir); return;}
    QDir* newDirectory = new QDir(this->text());
    if (newDirectory->exists())
    {
        this->setText(newDirectory->canonicalPath());
        curDir = this->text();
        emit pathChanged();
    }
    else {this->setText(curDir);}
}

void ElementList::openDir() //если введенная строка сущ., то запускаем процесс построения списка папок и песен
{
    QDir* directory = new QDir(curDir);
    QTableWidgetItem* newItem = new QTableWidgetItem;
    QFileInfoList entry = directory->entryInfoList();
    while (this->rowCount() > 0)
        this->removeRow(0);
    int rowCount = 0;
    if (this->windowTitle()=="dirList")//определяем, что за таблицу мы заполняем - с директориями или с файлами
    {
        for(QFileInfoList::iterator it = entry.begin(); it != entry.end(); ++it)
        {
            if ((it->isDir())&&(it->fileName()!="."))
            {//строим список папок, выбирая из списка вхождений в директорию те, что явл. папками
                newItem->setText(it->fileName());
                this->setRowCount(rowCount+1);
                this->setItem(rowCount, 0, newItem);
                newItem = new QTableWidgetItem;
                ++rowCount;
            }
        }
    }
    else
    {
        for(QFileInfoList::iterator it = entry.begin(); it != entry.end(); ++it)
        {
            if ((it->isFile())&&(it->suffix()=="mp3"))
            {//строим список mp3-файлов, выбирая их из списка вхождений в директорию
                newItem->setText(it->fileName());
                this->setRowCount(rowCount+1);
                this->setItem(rowCount, 0, newItem);
                newItem = new QTableWidgetItem;

                QString* tags = new QString [4];
                QFile file(it->absoluteFilePath());
                file.open(QIODevice::ReadOnly);
                DecodeInfo(tags, file);
                file.close();
                for(int i = 0; i < 4; ++i)
                {
                    newItem->setText(tags[i]);
                    this->setItem(rowCount, i+1, newItem);
                    newItem = new QTableWidgetItem;
                }
                delete tags;
                ++rowCount;
            }
        }
    }
}

FuncWnd::FuncWnd(QWidget *parent): QInputDialog(parent) {setModal(true);}

void FuncWnd::prepareData()
{
    if (textValue()!="")
    {
        if (windowTitle() == "Editing tags of files in directory"){emit transferData(1, textValue());}
        if (windowTitle() == "Editing filenames of files in directory"){emit transferData(2, textValue());}
        if (windowTitle() == "Deleting tags from all files in directory"){emit transferData(3, textValue());}
        if (windowTitle() == "Set one artist to all files in directory"){emit transferData(4, textValue());}
        if (windowTitle() == "Set one album to all files in directory"){emit transferData(5, textValue());}
        if (windowTitle() == "Set one genre to all files in directory"){emit transferData(6, textValue());}
    }
}

void ElementList::executeProcedure(char func, QString string)
{
    switch(func)
    {
    case 1: getTagsFromTemplate(this, string); break;
    case 2: editFilenameByTemplate(this, string); break;
    }
    emit procedureProceeded();
}

MainWindow::~MainWindow() {}
ElementList::ElementList(QWidget *parent) :QTableWidget(parent) {}
ElementList::~ElementList() {}
PathLine::PathLine(QWidget *parent) :QLineEdit(parent) {}
PathLine::~PathLine() {}
FuncWnd::~FuncWnd() {}
void FuncWnd::openMenu() {this->show();}
}
