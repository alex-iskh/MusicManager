#ifndef MUSMANAGER_H
#define MUSMANAGER_H

#include "widgets.h"

QString curDir;

namespace MusM{

void DecodeFrame(QString& dest, QByteArray& source, int& len)   //декодирование текстового фрейма
{
    if (source[0] == 0) //0x00 стоит перед строками в 8битной кодировке Latin-1
    {
        for (int i = 1; i < len; ++i)
            dest.append(QLatin1Char(source[i]));
        return;
    }
    else    //в теге находится строка в кодировке Unicode
    {
        int code;
        bool BOMisBE = true;    //BE значит big-endian - Юникод-символ начинается со старшего байта
        if ((source[1]==0xFF)&&(source[2]==0xFE))
            BOMisBE = false; //символ $FF$FE в начале строки трактуется как порядок little-endian - Юникод-символ начинается с младшего байта
        else if ((source[1]!=0xFE)||(source[2]!=0xFF))
        {
            code = 256*source[1] + source[2];
            dest.append(QChar(code));
        }
        for (int i = 3; i < len; i=i+2)
        {
            code = BOMisBE?(256*source[i] + source[i+1]):(source[i] + 256*source[i+1]);
            dest.append(QChar(code));
        }
    }
}

void DecodeInfo(QString*& tags, QFile& file, char toDecode = 0xF)   //ф-я декодирует основные данные из тегов формата IDv2.3 и IDv1
{
    unsigned char decoded = 0x00, genreNum = 255;//toDecode и decoded содержат информацию о том, какие именно фреймы надо декодировать/уже декодированы
    int tagLen, frameLen, extHeaderLen;
    bool extHeader = false, isInGenreList;
    QString name = "", artist = "", accompaniment = "", album = "", genre = "";
    QByteArray curSection, tag;
    //ищем IDv2.3 тег
    curSection = file.read(3);
    if (curSection == "ID3")
    {
        curSection = file.read(1);
        if (curSection[0] == 0x03)
        {//найден ID3v2.3-тег
            //определим, есть ли расширенный заголовок
            file.read(1);
            curSection = file.read(1);
            if ((curSection[0] & 0x40)==0x40)
            {
                extHeader = true;
            }
            //определим размер тега без заголовка, вычленим при наличии расш. заголовок, и считаем оставшееся в tag
            curSection = file.read(4);
            tagLen = (curSection[3]&0x7F) + 128*(curSection[2]&0x7F) + 16384*(curSection[1]&0x7F) + 2097152*(curSection[0]&0x7F);
            if (extHeader)
            {
                curSection = file.read(4);
                extHeaderLen = (unsigned char)(curSection[3]) + 256*(unsigned char)(curSection[2]) + 65536*(unsigned char)(curSection[1]) + 16777216*(unsigned char)(curSection[0]);
                file.read(extHeaderLen - 4);
            }
            tag = file.read(tagLen);
            //пройдем по фреймам, выделяя информацию из нужных нам фреймов
            while(tag.size()>10)
            {
                if (tag[0] == 0) break;
                curSection = "";
                for(int i = 0; i < 4; ++ i) curSection += tag[i];
                tag.remove(0,4);
                frameLen = (unsigned char)(tag[3]) + 256*(unsigned char)(tag[2]) + 65536*(unsigned char)(tag[1]) + 16777216*(unsigned char)(tag[0]);
                tag.remove(0,6);
                if (curSection == "TIT2")
                {
                    MusM::DecodeFrame(name, tag, frameLen);
                    decoded = decoded|0x08;
                }
                if (curSection == "TPE1")
                {
                    MusM::DecodeFrame(artist, tag, frameLen);
                    decoded = decoded|0x04;
                }
                if (curSection == "TPE2")
                {
                    MusM::DecodeFrame(accompaniment, tag, frameLen);
                    decoded = decoded|0x04;
                }
                if (curSection == "TALB")
                {
                    MusM::DecodeFrame(album, tag, frameLen);
                    decoded = decoded|0x02;
                }
                if (curSection == "TCON")
                {
                    MusM::DecodeFrame(genre, tag, frameLen);
                    if(genre[0]=='(')   //выделяем жанры, указанные в виде "(число)" - их надо брать из списка
                    {
                        genre.remove(0, 1);
                        genre.remove(genre.indexOf(')'), genre.size() - genre.indexOf(')'));
                        genreNum = genre.toInt(&isInGenreList);
                        if(!isInGenreList) {genreNum=255; decoded = decoded|0x01;}
                    }
                    else decoded = decoded|0x01;
                }
                tag.remove(0, frameLen);
            }
        }
        //можно дополнить декодированием устаревшего тега ID3v2.2 или нераспространенного ID3v2.4
    }
    if ((toDecode&decoded)!=toDecode)   //если декодированы не все нужные данные, пробуем найти тег ID3v1 в конце файла
    {
        file.seek(file.size()-128);
        curSection = file.read(3);
        if (curSection == "TAG")
        {//читаем необх. информацию
            if ((decoded&0x08) == 0)
            {
                curSection = file.read(30);
                curSection = curSection.trimmed();
                if (curSection!="")
                {
                    name = curSection;
                    decoded = decoded|0x08;
                }
            }
            else file.read(30);
            if ((decoded&0x04) == 0)
            {
                curSection = file.read(30);
                curSection = curSection.trimmed();
                if (curSection!="")
                {
                    artist = curSection;
                    decoded = decoded|0x04;
                }
            }
            else file.read(30);
            if ((decoded&0x02) == 0)
            {
                curSection = file.read(30);
                curSection = curSection.trimmed();
                if (curSection!="")
                {
                    album = curSection;
                    decoded = decoded|0x02;
                }
            }
            else file.read(30);
            file.read(34);
            if ((decoded&0x01) == 0)
            {
                curSection = file.read(1);
                genreNum = curSection[0];
            }
        }
    }
    switch (genreNum)
    {
    case 255: break;
    case 0: genre = "Blues"; decoded = decoded|0x01; break;
    case 1: genre = "Classic Rock"; decoded = decoded|0x01; break;
    case 2: genre = "Country"; decoded = decoded|0x01; break;
    case 3: genre = "Dance"; decoded = decoded|0x01; break;
    case 4: genre = "Disco"; decoded = decoded|0x01; break;
    case 5: genre = "Funk"; decoded = decoded|0x01; break;
    case 6: genre = "Grunge"; decoded = decoded|0x01; break;
    case 7: genre = "Hip-Hop"; decoded = decoded|0x01; break;
    case 8: genre = "Jazz"; decoded = decoded|0x01; break;
    case 9: genre = "Metal"; decoded = decoded|0x01; break;
    case 10: genre = "New Age"; decoded = decoded|0x01; break;
    case 11: genre = "Oldies"; decoded = decoded|0x01; break;
    case 12: genre = "Other"; decoded = decoded|0x01; break;
    case 13: genre = "Pop"; decoded = decoded|0x01; break;
    case 14: genre = "R&B"; decoded = decoded|0x01; break;
    case 15: genre = "Rap"; decoded = decoded|0x01; break;
    case 16: genre = "Reggae"; decoded = decoded|0x01; break;
    case 17: genre = "Rock"; decoded = decoded|0x01; break;
    case 18: genre = "Techno"; decoded = decoded|0x01; break;
    case 19: genre = "Industrial"; decoded = decoded|0x01; break;
    case 20: genre = "Alternative"; decoded = decoded|0x01; break;
    case 21: genre = "Ska"; decoded = decoded|0x01; break;
    case 22: genre = "Death Metal"; decoded = decoded|0x01; break;
    case 23: genre = "Pranks"; decoded = decoded|0x01; break;
    case 24: genre = "Soundtrack"; decoded = decoded|0x01; break;
    case 25: genre = "Euro-Techno"; decoded = decoded|0x01; break;
    case 26: genre = "Ambient"; decoded = decoded|0x01; break;
    case 27: genre = "Trip-Hop"; decoded = decoded|0x01; break;
    case 28: genre = "Vocal"; decoded = decoded|0x01; break;
    case 29: genre = "Jazz+Funk"; decoded = decoded|0x01; break;
    case 30: genre = "Fusion"; decoded = decoded|0x01; break;
    case 31: genre = "Trance"; decoded = decoded|0x01; break;
    case 32: genre = "Classical"; decoded = decoded|0x01; break;
    case 33: genre = "Instrumental"; decoded = decoded|0x01; break;
    case 34: genre = "Acid"; decoded = decoded|0x01; break;
    case 35: genre = "House"; decoded = decoded|0x01; break;
    case 36: genre = "Game"; decoded = decoded|0x01; break;
    case 37: genre = "Sound Clip"; decoded = decoded|0x01; break;
    case 38: genre = "Gospel"; decoded = decoded|0x01; break;
    case 39: genre = "Noise"; decoded = decoded|0x01; break;
    case 40: genre = "AlternRock"; decoded = decoded|0x01; break;
    case 41: genre = "Bass"; decoded = decoded|0x01; break;
    case 42: genre = "Soul"; decoded = decoded|0x01; break;
    case 43: genre = "Punk"; decoded = decoded|0x01; break;
    case 44: genre = "Space"; decoded = decoded|0x01; break;
    case 45: genre = "Meditative"; decoded = decoded|0x01; break;
    case 46: genre = "Instrumental Pop"; decoded = decoded|0x01; break;
    case 47: genre = "Instrumental Rock"; decoded = decoded|0x01; break;
    case 48: genre = "Ethnic"; decoded = decoded|0x01; break;
    case 49: genre = "Gothic"; decoded = decoded|0x01; break;
    case 50: genre = "Darkwave"; decoded = decoded|0x01; break;
    case 51: genre = "Techno-Industrial"; decoded = decoded|0x01; break;
    case 52: genre = "Electronic"; decoded = decoded|0x01; break;
    case 53: genre = "Pop-Folk"; decoded = decoded|0x01; break;
    case 54: genre = "Eurodance"; decoded = decoded|0x01; break;
    case 55: genre = "Dream"; decoded = decoded|0x01; break;
    case 56: genre = "Southern Rock"; decoded = decoded|0x01; break;
    case 57: genre = "Comedy"; decoded = decoded|0x01; break;
    case 58: genre = "Cult"; decoded = decoded|0x01; break;
    case 59: genre = "Gangsta"; decoded = decoded|0x01; break;
    case 60: genre = "Top 40"; decoded = decoded|0x01; break;
    case 61: genre = "Christian Rap"; decoded = decoded|0x01; break;
    case 62: genre = "Pop/Funk"; decoded = decoded|0x01; break;
    case 63: genre = "Jungle"; decoded = decoded|0x01; break;
    case 64: genre = "Native American"; decoded = decoded|0x01; break;
    case 65: genre = "Cabaret"; decoded = decoded|0x01; break;
    case 66: genre = "New Wave"; decoded = decoded|0x01; break;
    case 67: genre = "Psychadelic"; decoded = decoded|0x01; break;
    case 68: genre = "Rave"; decoded = decoded|0x01; break;
    case 69: genre = "Showtunes"; decoded = decoded|0x01; break;
    case 70: genre = "Trailer"; decoded = decoded|0x01; break;
    case 71: genre = "Lo-Fi"; decoded = decoded|0x01; break;
    case 72: genre = "Tribal"; decoded = decoded|0x01; break;
    case 73: genre = "Acid Punk"; decoded = decoded|0x01; break;
    case 74: genre = "Acid Jazz"; decoded = decoded|0x01; break;
    case 75: genre = "Polka"; decoded = decoded|0x01; break;
    case 76: genre = "Retro"; decoded = decoded|0x01; break;
    case 77: genre = "Musical"; decoded = decoded|0x01; break;
    case 78: genre = "Rock & Roll"; decoded = decoded|0x01; break;
    case 79: genre = "Hard Rock"; decoded = decoded|0x01; break;
    case 80: genre = "Folk"; decoded = decoded|0x01; break;
    case 81: genre = "Folk-Rock"; decoded = decoded|0x01; break;
    case 82: genre = "National Folk"; decoded = decoded|0x01; break;
    case 83: genre = "Swing"; decoded = decoded|0x01; break;
    case 84: genre = "Fast Fusion"; decoded = decoded|0x01; break;
    case 85: genre = "Bebob"; decoded = decoded|0x01; break;
    case 86: genre = "Latin"; decoded = decoded|0x01; break;
    case 87: genre = "Revival"; decoded = decoded|0x01; break;
    case 88: genre = "Celtic"; decoded = decoded|0x01; break;
    case 89: genre = "Bluegrass"; decoded = decoded|0x01; break;
    case 90: genre = "Avantgarde"; decoded = decoded|0x01; break;
    case 91: genre = "Gothic Rock"; decoded = decoded|0x01; break;
    case 92: genre = "Progressive Rock"; decoded = decoded|0x01; break;
    case 93: genre = "Psychedelic Rock"; decoded = decoded|0x01; break;
    case 94: genre = "Symphonic Rock"; decoded = decoded|0x01; break;
    case 95: genre = "Slow Rock"; decoded = decoded|0x01; break;
    case 96: genre = "Big Band"; decoded = decoded|0x01; break;
    case 97: genre = "Chorus"; decoded = decoded|0x01; break;
    case 98: genre = "Easy Listening"; decoded = decoded|0x01; break;
    case 99: genre = "Acoustic"; decoded = decoded|0x01; break;
    case 100: genre = "Humour"; decoded = decoded|0x01; break;
    case 101: genre = "Speech"; decoded = decoded|0x01; break;
    case 102: genre = "Chanson"; decoded = decoded|0x01; break;
    case 103: genre = "Opera"; decoded = decoded|0x01; break;
    case 104: genre = "Chamber Music"; decoded = decoded|0x01; break;
    case 105: genre = "Sonata"; decoded = decoded|0x01; break;
    case 106: genre = "Symphony"; decoded = decoded|0x01; break;
    case 107: genre = "Booty Bass"; decoded = decoded|0x01; break;
    case 108: genre = "Primus"; decoded = decoded|0x01; break;
    case 109: genre = "Porn Groove"; decoded = decoded|0x01; break;
    case 110: genre = "Satire"; decoded = decoded|0x01; break;
    case 111: genre = "Slow Jam"; decoded = decoded|0x01; break;
    case 112: genre = "Club"; decoded = decoded|0x01; break;
    case 113: genre = "Tango"; decoded = decoded|0x01; break;
    case 114: genre = "Samba"; decoded = decoded|0x01; break;
    case 115: genre = "Folklore"; decoded = decoded|0x01; break;
    case 116: genre = "Ballad"; decoded = decoded|0x01; break;
    case 117: genre = "Power Ballad"; decoded = decoded|0x01; break;
    case 118: genre = "Rhythmic Soul"; decoded = decoded|0x01; break;
    case 119: genre = "Freestyle"; decoded = decoded|0x01; break;
    case 120: genre = "Duet"; decoded = decoded|0x01; break;
    case 121: genre = "Punk Rock"; decoded = decoded|0x01; break;
    case 122: genre = "Drum Solo"; decoded = decoded|0x01; break;
    case 123: genre = "Acapella"; decoded = decoded|0x01; break;
    case 124: genre = "Euro-House"; decoded = decoded|0x01; break;
    case 125: genre = "Dance Hall"; decoded = decoded|0x01; break;
    }
    //записываем в выходной список тегов те теги, что были запрошены
    if (toDecode&0x08) {if (decoded&0x08) tags[0]=name; else tags[0]="";}
    if (toDecode&0x04)
    {
        if (decoded&0x04) {if (artist!="") tags[1]=artist; else tags[1] = accompaniment;}
        else tags[1]="";
    }
    if (toDecode&0x02) {if (decoded&0x02) tags[2]=album; else tags[2]="";}
    if (toDecode&0x01) {if (decoded&0x01) tags[3]=genre; else tags[3]="";}
}

void EncodeFrame (QByteArray& fileContent, QByteArray newValue, int begPos, int len, int tagLen)
{
    int newLen = newValue.size()+1;
    fileContent.replace(begPos+11, len, newValue);
    fileContent[begPos+10] = 0;
    fileContent[begPos+4] = newLen&266387456;
    fileContent[begPos+5] = newLen&2080768;
    fileContent[begPos+6] = newLen&16256;
    fileContent[begPos+7] = newLen&127;
    int newTagLen = tagLen+(newLen-len);
    fileContent[6] = newTagLen&266387456;
    fileContent[7] = newTagLen&2080768;
    fileContent[8] = newTagLen&16256;
    fileContent[9] = newTagLen&127;
}

void EncodeInfo(QString*& tags, QFile& file, char toEncode = 0xF)
{
    unsigned char encoded;
    QByteArray fileContent;
    int tagLen, frameLen, extHeaderLen, curPos, endPos;
    bool extHeader = false;
    fileContent = file.read(file.size());
    if((fileContent.mid(0, 3) == "ID3")&&(fileContent[3] == 0x03))
    {
        if ((fileContent[5] & 0x40)==0x40) extHeader = true;
        curPos = 6;
        tagLen = (fileContent[curPos+3]&0x7F) + 128*(fileContent[curPos+2]&0x7F) + 16384*(fileContent[curPos+1]&0x7F) + 2097152*(fileContent[curPos]&0x7F);
        if (extHeader)
        {
            curPos+=4;
            extHeaderLen = (unsigned char)(fileContent[curPos+3]) + 256*(unsigned char)(fileContent[curPos+2]) + 65536*(unsigned char)(fileContent[curPos+1]) + 16777216*(unsigned char)(fileContent[curPos]);
            curPos+=extHeaderLen - 4;
        }
        while (curPos < (tagLen+10))
        {
            if (fileContent[curPos] == 0) {endPos = curPos; break;}
            frameLen = (unsigned char)(fileContent[curPos+7]) + 256*(unsigned char)(fileContent[curPos+6]) + 65536*(unsigned char)(fileContent[curPos+5]) + 16777216*(unsigned char)(fileContent[curPos+4]);
            if ((fileContent.mid(curPos, 4)=="TIT2")&&((toEncode&0x08)==0x08))
            {
                EncodeFrame(fileContent, tags[0].toLatin1(), curPos, frameLen, tagLen);
                encoded = encoded|0x08;
            }
            if ((fileContent.mid(curPos, 4)=="TPE1")&&((toEncode&0x04)==0x04))
            {
                EncodeFrame(fileContent, tags[1].toLatin1(), curPos, frameLen, tagLen);
                encoded = encoded|0x04;
            }
            if ((fileContent.mid(curPos, 4)=="TALB")&&((toEncode&0x02)==0x02))
            {
                EncodeFrame(fileContent, tags[2].toLatin1(), curPos, frameLen, tagLen);
                encoded = encoded|0x01;
            }
            if ((fileContent.mid(curPos, 4)=="TCON")&&((toEncode&0x01)==0x01))
            {
                EncodeFrame(fileContent, tags[3].toLatin1(), curPos, frameLen, tagLen);
                encoded = encoded|0x01;
            }
        }
    }
    else
    {
        fileContent.insert(0, "ID3");
        fileContent.insert(3, 0x03);
        fileContent.insert(4, "\0\0\0\0\0\0");
        curPos = 10;
        tagLen = 0;
    }
    if ((toEncode&encoded)!=toEncode)
    {
        if ((toEncode&0x08)==0x08)
        {
            fileContent.insert(curPos, "TIT2\0\0\0\0\0\0\0");
            EncodeFrame(fileContent, tags[0].toLatin1(), curPos, 1, tagLen);
            encoded = encoded|0x08;
        }
        if ((toEncode&0x04)==0x04)
        {
            fileContent.insert(curPos, "TPE1\0\0\0\0\0\0\0");
            EncodeFrame(fileContent, tags[1].toLatin1(), curPos, 1, tagLen);
            encoded = encoded|0x04;
        }
        if ((toEncode&0x02)==0x02)
        {
            fileContent.insert(curPos, "TALB\0\0\0\0\0\0\0");
            EncodeFrame(fileContent, tags[2].toLatin1(), curPos, 1, tagLen);
            encoded = encoded|0x01;
        }
        if ((toEncode&0x01)==0x01)
        {
            fileContent.insert(curPos, "TCON\0\0\0\0\0\0\0");
            EncodeFrame(fileContent, tags[3].toLatin1(), curPos, 1, tagLen);
            encoded = encoded|0x01;
        }
    }
    file.resize(0);
    file.write(fileContent);
}

void getTagsFromTemplate(ElementList* table, QString tmplt)//процедура вычленяет теги из названий файлов, руководствуясь введенным шаблоном
{
    QString tag, curFilename, tmpltCopy, curTplSegment, tagValue;
    QString* tags = new QString[4];
    int i;
    unsigned char toEncode;
    for(int row = 0; row < table->rowCount(); ++row)
    {
        curFilename = table->item(i,0)->text();
        tmpltCopy = tmplt;
        while(tmplt!=0)
        {
            i = tmpltCopy.indexOf('<', 0);
            if(i==-1) break;  //если в шаблоне не осталось больше выражений вида <...>, то работа с шаблоном закончена
            curTplSegment = tmpltCopy.left(i);
            if (curFilename.indexOf(curTplSegment, 0) != 0) {toEncode = 0; break;} //имя файла не подходит под шаблон
            tmpltCopy.remove(0,i+1);
            curFilename.remove(0,i);
            i = tmpltCopy.indexOf('>', 0);
            if(i==-1) return;   //шаблон написан с ошибкой
            tag = tmpltCopy.left(i);
            tmpltCopy.remove(0,i+1);
            i = tmpltCopy.indexOf('<', 0);
            if (i==-1) i = tmpltCopy.size();    //если эта строчка выполняется, то мы дошли до конца шаблона на этом шаге
            curTplSegment = tmpltCopy.left(i);
            i = curFilename.indexOf(curTplSegment, 0);
            if (i==-1) {toEncode = 0; break;} //имя файла не подходит под шаблон
            tagValue = curFilename.left(i);
            if (tag=="name")
            {
                if ((toEncode&0x08)==0x08) {if (tags[0] != tagValue) {toEncode = 0; break;}}   //неподходящее имя файла или ошибка в шаблоне
                else {toEncode = toEncode|0x08; tags[0] = tagValue;}
            }
            else
            {
                if (tag=="artist")
                {
                    if ((toEncode&0x04)==0x04) {if (tags[1] != tagValue) {toEncode = 0; break;}}   //неподходящее имя файла или ошибка в шаблоне
                    else {toEncode = toEncode|0x04; tags[1] = tagValue;}
                }
                else
                {
                    if (tag=="album")
                    {
                        if ((toEncode&0x02)==0x02) {if (tags[2] != tagValue) {toEncode = 0; break;}}   //неподходящее имя файла или ошибка в шаблоне
                        else {toEncode = toEncode|0x02; tags[2] = tagValue;}
                    }
                    else
                    {
                        if (tag=="genre")
                        {
                            if ((toEncode&0x01)==0x01) {if (tags[3] != tagValue) {toEncode = 0; break;}}   //неподходящее имя файла или ошибка в шаблоне
                            else {toEncode = toEncode|0x01; tags[3] = tagValue;}
                        }
                        else
                        {
                            return; //ошибка в шаблоне - выражение в угловых скобках не распознано
                        }
                    }
                }
            }
        }
        if (toEncode != 0)
        {
            QFile file(curDir+table->item(i,0)->text());
            file.open(QIODevice::ReadWrite);
            EncodeInfo(tags, file, toEncode);
            file.close();
        }
    }
}

void editFilenameByTemplate(ElementList* table, QString tmplt)//процедура составляет имя файла из информации о нем в таблице по заданному шаблону
{
    if (tmplt.contains('\\')||tmplt.contains('/')||tmplt.contains(':')||tmplt.contains('*')||tmplt.contains('?')||tmplt.contains('|')) return;
    QString tmpltCopy;
    QString number;
    QString path;
    for (int i = 0; i < table->rowCount(); ++i)
    {
        tmpltCopy = tmplt;
        tmpltCopy.replace("<num>", number.setNum(i));
        tmpltCopy.replace ("<name>", table->item(i, 1)->text());
        tmpltCopy.replace ("<artist>", table->item(i, 2)->text());
        tmpltCopy.replace ("<album>", table->item(i, 3)->text());
        tmpltCopy.replace ("<genre>", table->item(i, 4)->text());
        if ((tmpltCopy != table->item(i,0)->text())&&(tmpltCopy!="")&&!(tmpltCopy.contains('>')||tmpltCopy.contains('<')||tmpltCopy.contains('\\')||tmpltCopy.contains('/')||tmpltCopy.contains(':')||tmpltCopy.contains('*')||tmpltCopy.contains('?')||tmpltCopy.contains('|')))
        {
            path = curDir+"/"+tmpltCopy+".mp3";
            while (QFile(path).exists()) tmpltCopy+="+";
            path = curDir+"/"+table->item(i,0)->text();
            QFile file(path);
            path = curDir+"/"+tmpltCopy;
            if (path.size()>256) path.remove(257, path.size()-256);
            path += ".mp3";
            file.rename(path);
        }
    }
}

}

#endif // MUSMANAGER_H
