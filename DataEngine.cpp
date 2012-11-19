#include "StdAfx.h"
#include "DataEngine.h"
#include "STKDRV.h"
#include <time.h>
#include <QApplication>
#include <QtSql>
#include <QtXml>

CDataEngine* CDataEngine::m_pDataEngine = NULL;

time_t CDataEngine::m_tmCurrentDay = NULL;
time_t* CDataEngine::m_tmLast5Day = new time_t[5];

void CDataEngine::importData()
{
	QString qsDir = qApp->applicationDirPath();
	{
		//����F10 ����
		QString qsBaseInfo = qsDir+"/baseinfo.rsqfin";
		if(QFile::exists(qsBaseInfo))
		{
			qDebug()<<"Import F10 data...";
			int iCount = importBaseInfo(qsBaseInfo);
			if(iCount<1)
			{
				QFile::remove(qsBaseInfo);
				qDebug()<<"Import F10 data from "<<qsBaseInfo<<" error!";
			}
			else
				qDebug()<<iCount<<" F10 data had been imported.";
		}
		else
		{
			//���������F10���ݣ����Ե���������F10 ���ݡ�
			QSettings settings("HKEY_LOCAL_MACHINE\\SOFTWARE\\StockDrv",QSettings::NativeFormat);
			QString qsF10File = QFileInfo(settings.value("driver").toString()).absolutePath() + "/��������.fin";
			if(QFile::exists(qsF10File))
			{
				qDebug()<<"Import F10 Data from "<<qsF10File;
				CDataEngine::importBaseInfoFromFinFile(qsF10File);
			}
		}
	}
	{
		//���뵱���Reports����
		/*
		QString qsReportFile = QString("%1/reports/%2").arg(qsDir).arg(QDate::currentDate().toJulianDay());
		qDebug()<<"Import reports data from "<<qsReportFile;
		int iCount = importReportsInfo(qsReportFile);
		if(iCount<1)
		{
			QFile::remove(qsReportFile);
			qDebug()<<"Import reports data from "<<qsReportFile<<" error!";
		}
		else
			qDebug()<<iCount<<" reports data had been imported.";
			*/
	}
}

void CDataEngine::exportData()
{
	QString qsDir = qApp->applicationDirPath();
	QString qsBaseInfo = qsDir+"/baseinfo.rsqfin";
	{
		qDebug()<<"Export F10 data...";
		int iCount = exportBaseInfo(qsBaseInfo);
		qDebug()<<iCount<<" F10 data had been exported.";
	}

	{
		//���������Reports����
		/*
		QString qsReportDir = qsDir + "/reports";
		if(!QDir().exists(qsReportDir))
			QDir().mkpath(qsReportDir);
		QString qsReportFile = QString("%1/%2").arg(qsReportDir).arg(QDate::currentDate().toJulianDay());
		qDebug()<<"Export reports data to "<<qsReportFile;
		int iCount = exportReportsInfo(qsReportFile);
		qDebug()<<iCount<<" reports data had been exported.";
		*/
	}
}

int CDataEngine::importBaseInfoFromFinFile( const QString& qsFile )
{
	QFile file(qsFile);
	if(!file.open(QFile::ReadOnly))
		return 0;

	int iFlag;
	file.read((char*)&iFlag,4);
	int iTotal;
	file.read((char*)&iTotal,4);

	int iCout = 0;

	while(true)
	{
		WORD wMarket;
		if(file.read((char*)&wMarket,2)!=2) break;
		if(!file.seek(file.pos()+2)) break;

		char chCode[STKLABEL_LEN];
		if(file.read(chCode,STKLABEL_LEN)!=STKLABEL_LEN) break;


		float fVal[38];
		if(file.read((char*)fVal,sizeof(float)*38)!=(sizeof(float)*38)) break;

		qRcvBaseInfoData d(fVal);
		d.wMarket = wMarket;
		memcpy(d.code,chCode,STKLABEL_LEN);

		QString qsCode = QString::fromLocal8Bit(chCode);

		CStockInfoItem* pItem = CDataEngine::getDataEngine()->getStockInfoItem(qsCode);
		if(pItem)
		{
			pItem->setBaseInfo(d);
		}
		else
		{
			CStockInfoItem* pItem = new CStockInfoItem(d);
			CDataEngine::getDataEngine()->setStockInfoItem(pItem);
		}

		++iCout;
	}

	return iCout;
}

int CDataEngine::importBaseInfo( const QString& qsFile )
{
	QFile file(qsFile);
	if(!file.open(QFile::ReadOnly))
		return -1;

	QDataStream out(&file);

	int iCount = 0;
	while(true)
	{
		qRcvBaseInfoData baseInfo;
		int iSize = out.readRawData((char*)&baseInfo,sizeof(qRcvBaseInfoData));
		if(iSize!=sizeof(qRcvBaseInfoData))
			break;

		QString qsCode = QString::fromLocal8Bit(baseInfo.code);
		if(qsCode.isEmpty())
			return -1;

		CStockInfoItem* pItem = CDataEngine::getDataEngine()->getStockInfoItem(qsCode);
		if(pItem)
		{
			pItem->setBaseInfo(baseInfo);
		}
		else
		{
			CStockInfoItem* pItem = new CStockInfoItem(baseInfo);
			CDataEngine::getDataEngine()->setStockInfoItem(pItem);
		}

		++iCount;
	}

	file.close();
	return iCount;
}

int CDataEngine::importReportsInfo( const QString& qsFile )
{
	QFile file(qsFile);
	if(!file.open(QFile::ReadOnly))
		return -1;

	QDataStream out(&file);

	int iCount = 0;
	while(true)
	{
		qRcvReportData* pReport = new qRcvReportData;
		quint32 _t;
		out>>_t>>pReport->wMarket>>pReport->qsCode>>pReport->qsName;
		pReport->tmTime = _t;

		int iSize = out.readRawData((char*)&pReport->fLastClose,sizeof(float)*27);
		if(iSize!=(sizeof(float)*27))
			break;

		CStockInfoItem* pItem = CDataEngine::getDataEngine()->getStockInfoItem(pReport->qsCode);
		if(pItem==NULL)
		{
			pItem = new CStockInfoItem(pReport->qsCode,pReport->wMarket);
			CDataEngine::getDataEngine()->setStockInfoItem(pItem);
		}
		pItem->setReport(pReport);

		++iCount;
	}

	file.close();
	return iCount;
}


int CDataEngine::exportBaseInfo( const QString& qsFile )
{
	if(QFile::exists(qsFile))
		QFile::remove(qsFile);
	if(QFile::exists(qsFile))
		return -1;

	QFile file(qsFile);
	if(!file.open(QFile::WriteOnly))
		return -1;

	QDataStream out(&file);

	QList<CStockInfoItem*> listItem = CDataEngine::getDataEngine()->getStockInfoList();
	int iCount = 0;

	foreach( CStockInfoItem* pItem, listItem)
	{
		qRcvBaseInfoData* pBaseInfo = pItem->getBaseInfo();

		int iSize = out.writeRawData((char*)pBaseInfo,sizeof(qRcvBaseInfoData));
		if(iSize!=sizeof(qRcvBaseInfoData))
			break;
		++iCount;
	}

	file.close();
	return iCount;
}

int CDataEngine::exportReportsInfo( const QString& qsFile )
{
	if(QFile::exists(qsFile))
		QFile::remove(qsFile);
	if(QFile::exists(qsFile))
		return -1;

	QFile file(qsFile);
	if(!file.open(QFile::WriteOnly))
		return -1;

	QDataStream out(&file);

	QList<CStockInfoItem*> listItem = CDataEngine::getDataEngine()->getStockInfoList();
	int iCount = 0;

	foreach( CStockInfoItem* pItem, listItem)
	{
		//���浱�����е�Reports
		qRcvReportData* pReport = pItem->getCurrentReport();
		if(pReport->tmTime>m_tmCurrentDay)
		{
			out<<pReport->tmTime<<pReport->wMarket<<pReport->qsCode<<pReport->qsName;
			//ֱ�ӿ���ʣ�������float����
			if(out.writeRawData((char*)&pReport->fLastClose,sizeof(float)*27)!=(sizeof(float)*27))
				break;
			//		int iSize = out.writeRawData((char*)pBaseInfo,sizeof(qRcvBaseInfoData));
			//		if(iSize!=sizeof(qRcvBaseInfoData))
			//			break;
			++iCount;
		}
	}

	file.close();
	return iCount;
}


bool CDataEngine::isStockOpenDay( time_t tmDay )
{
	QDate tmDate = QDateTime::fromTime_t(tmDay).date();
	int iWeek = tmDate.dayOfWeek();
	if((iWeek == 6)||(iWeek == 7))
		return false;

	return true;
}

time_t* CDataEngine::getLast5DayTime()
{
	time_t tmCur = time(NULL);
	tmCur = tmCur-(tmCur%(3600*24));
	tmCur = tmCur-(3600*R_TIME_ZONE);

	if(tmCur==m_tmCurrentDay)
		return m_tmLast5Day;

	m_tmCurrentDay = tmCur;

	for(int i=0;i<5;++i)
	{
		tmCur = tmCur-(3600*24);
		while(!isStockOpenDay(tmCur))
		{
			tmCur = tmCur-(3600*24);
		}
		m_tmLast5Day[i] = tmCur;
	}
	QDateTime tmDataTime = QDateTime::fromTime_t(tmCur);

	return m_tmLast5Day;
}

time_t CDataEngine::getOpenSeconds()
{
	return 3600*4;
}

time_t CDataEngine::getOpenSeconds( time_t tmTime )
{
	time_t tmEnd2 = m_tmCurrentDay+3600*15;
	if(tmTime>tmEnd2)
		return 3600*4;

	time_t tmBegin2 = m_tmCurrentDay+3600*13;
	if(tmTime>tmBegin2)
		return 3600*2+(tmTime-tmBegin2);

	time_t tmEnd1 = m_tmCurrentDay+1800*23;
	if(tmTime>tmEnd1)
		return 3600*2;

	time_t tmBegin1 = m_tmCurrentDay + 1800*19;
	if(tmTime>tmBegin2)
		return tmTime-tmBegin1;

	return 0;
}



CDataEngine::CDataEngine(void)
{
	getLast5DayTime();

	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","HISTORY");
	db.setDatabaseName(qApp->applicationDirPath()+"/RStockAnalyst.dat");
	if(!db.open())
	{
		qDebug()<<"Open history database error!";
	}

	m_qsHistroyDir = qApp->applicationDirPath()+"/data/history/";
	QDir().mkpath(m_qsHistroyDir);

	m_qsBlocksDir = qApp->applicationDirPath()+"/data/blocks/";
	QDir().mkpath(m_qsBlocksDir);

	m_qsCommonBlocks = qApp->applicationDirPath()+"/data/CommonBlocks.xml";
	initCommonBlocks();
}

CDataEngine::~CDataEngine(void)
{
}


void CDataEngine::initCommonBlocks()
{
	m_listCommonBlocks.clear();

	QDomDocument doc("CommonBlocks");
	QFile file(m_qsCommonBlocks);
	if (!file.open(QIODevice::ReadOnly))
		return;
	if (!doc.setContent(&file)) {
		file.close();
		return;
	}
	file.close();

	QDomElement eleRoot = doc.documentElement();
	if(eleRoot.isNull())
		return;
	QDomElement eleNode = eleRoot.firstChildElement("block");
	while(!eleNode.isNull())
	{
		QString qsName = eleNode.attribute("name");
		if(!(qsName.isEmpty()||isBlockInCommon(qsName)))
		{
			QString qsRegexp = eleNode.text();
			if(!qsRegexp.isEmpty())
			{
				QRegExp r(qsRegexp);
				if(eleNode.hasAttribute("RegType"))
				{
					r.setPatternSyntax(static_cast<QRegExp::PatternSyntax>(eleNode.attribute("RegType").toInt()));
				}
				m_listCommonBlocks.push_back(QPair<QString,QRegExp>(qsName,r));
			}
		}
		eleNode = eleNode.nextSiblingElement("block");
	}
}

QList<QString> CDataEngine::getStockBlocks()
{
	QList<QString> listBlocks;
	QList<QPair<QString,QRegExp>>::iterator iter = m_listCommonBlocks.begin();
	while(iter!=m_listCommonBlocks.end())
	{
		listBlocks.push_back(iter->first);
		++iter;
	}

	QDir dir(m_qsBlocksDir);
	QStringList listEntity = dir.entryList(QDir::Files);
	foreach(const QString& e,listEntity)
	{
		if(!isBlockInCommon(e))
		{
			listBlocks.push_back(e);
		}
	}

	return listBlocks;
}

bool CDataEngine::isHadBlock( const QString& block )
{
	if(isBlockInCommon(block))
		return true;

	return QFile::exists(m_qsBlocksDir+block);
}

QList<CStockInfoItem*> CDataEngine::getStocksByMarket( WORD wMarket )
{
	QList<CStockInfoItem*> listStocks;
	QMap<QString,CStockInfoItem*>::iterator iter = m_mapStockInfos.begin();
	while(iter!=m_mapStockInfos.end())
	{
		if((*iter)->getMarket() == wMarket)
			listStocks.push_back(iter.value());
		++iter;
	}

	return listStocks;
}

QList<CStockInfoItem*> CDataEngine::getStocksByBlock( const QString& block )
{
	QList<CStockInfoItem*> listStocks;
	if(isBlockInCommon(block))
	{
		QRegExp r = getRegexpByBlock(block);
		QMap<QString,CStockInfoItem*>::iterator iter = m_mapStockInfos.begin();
		while(iter!=m_mapStockInfos.end())
		{
			if(r.exactMatch((*iter)->getCode()))
			{
				listStocks.push_back(*iter);
			}
			++iter;
		}
	}
	else
	{
		QFile file(m_qsBlocksDir+block);
		if(file.open(QFile::ReadOnly))
		{
			while(!file.atEnd())
			{
				QString code = file.readLine();
				code = code.trimmed();
				if((!code.isEmpty())&&(m_mapStockInfos.contains(code)))
				{
					listStocks.push_back(m_mapStockInfos[code]);
				}
			}
		}
	}

	return listStocks;
}

QList<CStockInfoItem*> CDataEngine::getStockInfoList()
{
	return m_mapStockInfos.values();
}

CStockInfoItem* CDataEngine::getStockInfoItem( const QString& qsCode )
{
	if(m_mapStockInfos.contains(qsCode))
	{
		return m_mapStockInfos[qsCode];
	}
	return NULL;
}

void CDataEngine::setStockInfoItem( CStockInfoItem* p )
{
	m_mapStockInfos[p->getCode()] = p;
	emit stockInfoChanged(p->getCode());
	connect(p,SIGNAL(stockInfoItemChanged(const QString&)),this,SIGNAL(stockInfoChanged(const QString&)));
}

CDataEngine* CDataEngine::getDataEngine()
{
	if(m_pDataEngine == NULL)
		m_pDataEngine = new CDataEngine;
	return m_pDataEngine;
}

bool CDataEngine::exportHistoryData( const QString& qsCode, const QList<qRcvHistoryData*>& list )
{
	/*���ݿ����
	QSqlDatabase db = QSqlDatabase::database("HISTORY");
	if(!db.isOpen())
		return false;
	db.transaction();
	QSqlQuery q(db);

	foreach(qRcvHistoryData* pData, list)
	{
		QString qsQuery = QString("replace into [History] ('code','time','open','high','low','close','volume','amount') \
								  values ('%1',%2,%3,%4,%5,%6,%7,%8)")
								  .arg(qsCode).arg(pData->time).arg(pData->fOpen).arg(pData->fHigh)
								  .arg(pData->fLow).arg(pData->fClose).arg(pData->fVolume).arg(pData->fAmount);
		q.exec(qsQuery);
	}
	db.commit();
	return true;
	*/

	QString qsDayData = QString("%1%2").arg(m_qsHistroyDir).arg(qsCode);

	QFile file(qsDayData);
	if(!file.open(QFile::WriteOnly|QFile::Truncate))
		return false;

	QDataStream out(&file);

	foreach(qRcvHistoryData* pData, list)
	{
		int iSize = out.writeRawData((char*)pData,sizeof(qRcvHistoryData));
		if(iSize!=sizeof(qRcvHistoryData))
			return false;
	}

	file.close();
	return true;
}

QList<qRcvHistoryData*> CDataEngine::getHistoryList( const QString& code )
{
	QList<qRcvHistoryData*> list;

	QString qsDayData = QString("%1%2").arg(m_qsHistroyDir).arg(code);
	QFile file(qsDayData);
	if(!file.open(QFile::ReadOnly))
		return list;

	QDataStream inStream(&file);
	while(!inStream.atEnd())
	{
		qRcvHistoryData* pData = new qRcvHistoryData;
		int iSize = inStream.readRawData(reinterpret_cast<char*>(pData),sizeof(qRcvHistoryData));
		if(iSize!=sizeof(qRcvHistoryData))
		{
			delete pData;
			break;
		}
		list.push_back(pData);
	}

	file.close();
	return list;
}

QList<qRcvHistoryData*> CDataEngine::getHistoryList( const QString& code, int count )
{
	QList<qRcvHistoryData*> list;

	QString qsDayData = QString("%1%2").arg(m_qsHistroyDir).arg(code);
	QFile file(qsDayData);
	if(!file.open(QFile::ReadOnly))
		return list;
	int iDataSize = sizeof(qRcvHistoryData);

	int iPos = file.size()-iDataSize;
	int iCount = 0;
	while(iPos>=0&&iCount<count)
	{
		++iCount;
		file.seek(iPos);
		qRcvHistoryData* pData = new qRcvHistoryData;
		int iSize = file.read(reinterpret_cast<char*>(pData),iDataSize);
		if(iSize!=iDataSize)
		{
			delete pData;
			break;
		}
		list.push_front(pData);
		iPos = iPos-iDataSize;
	}

	file.close();
	return list;
}

bool CDataEngine::isBlockInCommon( const QString& block )
{
	QList<QPair<QString,QRegExp>>::iterator iter = m_listCommonBlocks.begin();
	while(iter!=m_listCommonBlocks.end())
	{
		if(iter->first==block)
			return true;
		++iter;
	}

	return false;
}

QRegExp CDataEngine::getRegexpByBlock( const QString& block )
{
	QList<QPair<QString,QRegExp>>::iterator iter = m_listCommonBlocks.begin();
	while(iter!=m_listCommonBlocks.end())
	{
		if(iter->first==block)
			return iter->second;
		++iter;
	}

	return QRegExp("*");
}