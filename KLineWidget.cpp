#include "StdAfx.h"
#include "KLineWidget.h"
#include "DataEngine.h"

#define	KLINE_BORDER	2

bool getLinerItemByDays(stLinerItem* pItem,const QList<qRcvHistoryData*>& list)
{

	return true;
}

int getLinerDayItem(QList<stLinerItem*>& listItems,const QList<qRcvHistoryData*>& historys, int nDay)
{
	if(nDay==1)
	{
		foreach(qRcvHistoryData* p,historys)
		{
			stLinerItem* pItem = new stLinerItem;
			pItem->time = p->time;
			pItem->fOpen = p->fOpen;
			pItem->fClose = p->fClose;
			pItem->fHigh = p->fHigh;
			pItem->fLow = p->fLow;
			pItem->fAmount = p->fAmount;
			pItem->fVolume = p->fVolume;
			pItem->wAdvance = p->wAdvance;
			pItem->wDecline = p->wDecline;
			listItems.push_back(pItem);
		}
	}
	else
	{

	}
	return listItems.size();
}

int getLinerWeekItem(QList<stLinerItem*>& listItems,const QList<qRcvHistoryData*>& historys)
{
	if(historys.size()<1)
		return 0;
	int iCurYear = 0;
	int iCurWeek = 0;
	{
		//first day's week and year.
		QDate tmDate = QDateTime::fromTime_t(historys.first()->time).date();
		iCurYear = tmDate.year();
		iCurWeek = tmDate.weekNumber();
	}

	QList<qRcvHistoryData*> weekHis;		//按星期进行归类以后的日线数据
	for(int i=0;i<historys.size();++i)
	{
		qRcvHistoryData* pHistory = historys[i];
		QDate tmDate = QDateTime::fromTime_t(pHistory->time).date();
		if(tmDate.year()!=iCurYear)
		{
			iCurYear = tmDate.year();
			iCurWeek = tmDate.weekNumber();
			if(tmDate.dayOfWeek()==1)
			{
				stLinerItem* pItem = new stLinerItem;
				getLinerItemByDays(pItem,weekHis);
				listItems.push_back(pItem);
				weekHis.clear();
			}
		}
		else if(tmDate.weekNumber()!=iCurWeek)
		{
			iCurWeek = tmDate.weekNumber();

			stLinerItem* pItem = new stLinerItem;
			getLinerItemByDays(pItem,weekHis);
			listItems.push_back(pItem);
			weekHis.clear();
		}
	}

	return listItems.size();
}

CKLineWidget::CKLineWidget( CBaseWidget* parent /*= 0*/ )
	: CBaseWidget(parent,CBaseWidget::KLine)
	, m_pMenuCustom(0)
	, m_typeCircle(CKLineWidget::Day)
	, m_pStockItem(0)
	, m_iShowCount(100)
	, m_pLinerMain(0)
	, m_pLinerDeputy(0)
	, m_iTitleHeight(16)
	, m_iCoorYWidth(50)
	, m_iCoorXHeight(16)
	, m_iMainLinerHeight(200)
{
//	setMinimumSize(200,200);
	setMouseTracking(true);
	setStockCode(QString("600000"));
	m_pMenuCustom = new QMenu("K线图操作");
	m_pMenuCustom->addAction(tr("设置股票代码"),this,SLOT(onSetStockCode()));

	m_pLinerMain = new CMultiLiner(CMultiLiner::Main);
	m_pLinerMain->setExpression(QString(""));
}

CKLineWidget::~CKLineWidget(void)
{
	clearTmpData();
	delete m_pMenuCustom;
}

bool CKLineWidget::loadPanelInfo( const QDomElement& eleWidget )
{
	if(!CBaseWidget::loadPanelInfo(eleWidget))
		return false;

	QDomElement eleCode = eleWidget.firstChildElement("code");
	if(eleCode.isElement())
		setStockCode(eleCode.text());

	return true;
}

bool CKLineWidget::savePanelInfo( QDomDocument& doc,QDomElement& eleWidget )
{
	if(!CBaseWidget::savePanelInfo(doc,eleWidget))
		return false;
	if(m_pStockItem)
	{
		QDomElement eleCode = doc.createElement("code");
		eleCode.appendChild(doc.createTextNode(m_pStockItem->getCode()));
		eleWidget.appendChild(eleCode);
	}

	return true;
}

void CKLineWidget::setStockCode( const QString& code )
{
	CStockInfoItem* pItem = CDataEngine::getDataEngine()->getStockInfoItem(code);
	if(pItem)
	{
		disconnect(this,SLOT(updateKLine(const QString&)));		//移除所有和 updateKLine关联的 信号/槽

		m_pStockItem = pItem;

		//建立更新机制
		connect(pItem,SIGNAL(stockInfoItemChanged(const QString&)),this,SLOT(updateKLine(const QString&)));

		//更新K线图
		updateKLine(code);
	}

	return CBaseWidget::setStockCode(code);
}

void CKLineWidget::updateKLine( const QString& code )
{
	if(m_pStockItem&&m_pStockItem->getCode()!=code)
		return;

	resetTmpData();
}

void CKLineWidget::paintEvent( QPaintEvent* )
{
	QPainter p(this);
	QRect rtClient = this->rect();
	p.fillRect(rtClient,QColor(0,0,0));
	if(!m_pStockItem)
		return;

	/*画头部*/
	QRect rtTitle = QRect(rtClient.left(),rtClient.top(),rtClient.width(),m_iTitleHeight);
	drawTitle(p,rtTitle);

	/*画X坐标轴*/
	QRect rtCoordX = QRect(rtClient.left(),rtClient.bottom()-m_iCoorXHeight+1,rtClient.width()-m_iCoorYWidth,m_iCoorXHeight);
	drawCoordX(p,rtCoordX);
	/*画右下角的两个按钮*/
	QRect rtShowBtns = QRect(rtClient.right()-m_iCoorYWidth,rtClient.bottom()-m_iCoorXHeight,m_iCoorYWidth,m_iCoorXHeight);
	drawShowBtns(p,rtShowBtns);

	rtClient.adjust(3,m_iTitleHeight,-m_iCoorYWidth-2,-m_iCoorXHeight);			//改变为可画图区域的大小

	/*绘制主图*/
	int iMainHeight = rtClient.height();
	if(m_listLiners.size()>0)
		iMainHeight = iMainHeight/2;

	m_pLinerMain->Draw(p,listItems,QRectF(rtClient.left(),rtClient.top(),rtClient.width(),rtClient.height()),m_iShowCount);


	rtClient.adjust(KLINE_BORDER,m_iTitleHeight+KLINE_BORDER,-50,-15);
	if(!rtClient.isValid())
		return;
}

void CKLineWidget::mouseMoveEvent( QMouseEvent* e )
{
	if(!m_pStockItem)
	{
		QToolTip::hideText();
		return;
	}
	QRectF rtClient = this->rect();
	rtClient.adjust(KLINE_BORDER,KLINE_BORDER,-50,-15);

	float fBegin = rtClient.left();
	//int iIndex = (e->posF().x() - fBegin)/fItemWidth;
	//if(iIndex>=0&&iIndex<listHistory.size())
	//{
	//	qRcvHistoryData* pHistory = listHistory[iIndex];
	//	QString qsTooltip = QString("股票代码:%1\r\n时间:%2\r\n最高价:%3\r\n最低价:%4\r\n开盘价:%5\r\n收盘价:%6\r\n成交量:%7\r\n成交额:%8")
	//		.arg(m_pStockItem->getCode()).arg(QDateTime::fromTime_t(pHistory->time).toString("yyyy/MM/dd"))
	//		.arg(pHistory->fHigh).arg(pHistory->fLow).arg(pHistory->fOpen).arg(pHistory->fClose)
	//		.arg(pHistory->fVolume).arg(pHistory->fAmount);

	//	QToolTip::showText(e->globalPos(),qsTooltip,this);
	//}
	//else
	//{
	//	QToolTip::hideText();
	//}
}

void CKLineWidget::mousePressEvent( QMouseEvent* e )
{
	QPoint ptCur = e->pos();
	if(m_rtAddShow.contains(ptCur))
	{
		onClickedAddShow();
	}
	else if(m_rtSubShow.contains(ptCur))
	{
		onClickedSubShow();
	}
}

QMenu* CKLineWidget::getCustomMenu()
{
	QAction* pAction = m_pMenu->menuAction();
	if(!m_pMenuCustom->actionGeometry(pAction).isValid())
		m_pMenuCustom->addMenu(m_pMenu);

	return m_pMenuCustom;
}

void CKLineWidget::onSetStockCode()
{
	QDialog dlg(this);
	QVBoxLayout layout(this);
	QLineEdit edit(this);
	QPushButton btnOk(this);
	dlg.setLayout(&layout);
	layout.addWidget(&edit);
	layout.addWidget(&btnOk);
	btnOk.setText(tr("确定"));
	connect(&btnOk,SIGNAL(clicked()),&dlg,SLOT(accept()));

	if(QDialog::Accepted != dlg.exec())
		return;

	QString code = edit.text();
	setStockCode(code);
}

void CKLineWidget::onClickedAddShow()
{
	if((m_iShowCount+10)<=listItems.size())
	{
		m_iShowCount += 10;
		update();
	}
}

void CKLineWidget::onClickedSubShow()
{
	if((m_iShowCount-10)>0)
	{
		m_iShowCount -= 10;
		update();
	}
}

void CKLineWidget::drawTitle( QPainter& p,const QRect& rtTitle )
{
	if(!rtTitle.isValid())
		return;

	QRect rtClient = rtTitle.adjusted(2,0,-2,0);
	QString qsName = m_pStockItem->getName();
	if(qsName.isEmpty())
		qsName = m_pStockItem->getCode();

	p.setPen(QColor(127,0,0));
	p.drawRect(rtClient);

	p.setPen(QColor(255,255,255));
	p.drawText(rtClient,Qt::AlignLeft|Qt::AlignVCenter,qsName);
}

void CKLineWidget::drawCoordX( QPainter& p,const QRect& rtCoordX )
{
	if(!rtCoordX.isValid())
		return;

	//绘制单线
	float fTopLine = rtCoordX.top()+1;
	p.setPen(QColor(255,0,0));
	p.drawLine(rtCoordX.right(),fTopLine,rtCoordX.left(),fTopLine);


	//左空余3Pix，右空余2Pix
	fItemWidth = float(rtCoordX.width()-2-3)/float(m_iShowCount);
	//从右向左绘制
	float fCurX = rtCoordX.right()-2;
	float fLastX = fCurX+100;
	//绘制刻度
	if(m_typeCircle<Day)
	{

	}
	else
	{
		int iCurMonth = 0;
		int iCurIndex = listItems.size()-1;
		int iCount = 0;
		while(iCount<m_iShowCount)
		{
			stLinerItem* pItem = listItems[iCurIndex];
			QDate tmDate = QDateTime::fromTime_t(pItem->time).date();
			if(tmDate.month()!=iCurMonth)
			{
				iCurMonth = tmDate.month();
				if((fLastX-fCurX)>16)
				{
					p.drawLine(fCurX,fTopLine,fCurX,rtCoordX.bottom()-1);
					p.drawText(fCurX+2,rtCoordX.bottom()-3,QString("%1").arg(iCurMonth));
					fLastX = fCurX;
				}
			}

			fCurX -= fItemWidth;
			++iCount;
			--iCurIndex;
		}
	}
}

void CKLineWidget::drawShowBtns( QPainter& p,const QRect& rtBtns )
{
	p.setPen(QColor(255,0,0));
	int iBtnWidth = 16;
	int iBtnHeight = rtBtns.height()-1;
	m_rtAddShow = QRect(rtBtns.right()-2*iBtnWidth,rtBtns.top()+1,iBtnWidth,iBtnHeight);
	m_rtSubShow = QRect(rtBtns.right()-iBtnWidth,rtBtns.top()+1,iBtnWidth,iBtnHeight);
	
	p.drawRect(m_rtAddShow);
	p.drawRect(m_rtSubShow);

	p.drawText(m_rtAddShow,Qt::AlignCenter,QString("+"));
	p.drawText(m_rtSubShow,Qt::AlignCenter,QString("-"));
}

void CKLineWidget::clearTmpData()
{
	foreach(stLinerItem* p,listItems)
		delete p;
	listItems.clear();
}

void CKLineWidget::resetTmpData()
{
	clearTmpData();
	if(m_typeCircle<Day)
	{
		//获取分钟数据，进行计算
		QList<qRcvMinuteData*> minutes = m_pStockItem->getMinuteList();
		if(m_typeCircle == Min1)
		{

		}
	}
	else
	{
		//获取日线数据
		QList<qRcvHistoryData*> historys = m_pStockItem->getHistoryList();
		if(m_typeCircle == Day)
		{
			getLinerDayItem(listItems,historys,1);
		}
		else if(m_typeCircle == DayN)
		{
			//目前未使用
		//	getLinerItem(listItems,historys,3);
		}

		{
			//清楚获取的日线数据
			foreach(qRcvHistoryData* p,historys)
				delete p;
			historys.clear();
		}
	}
	if(listItems.size()<m_iShowCount)
		m_iShowCount = listItems.size();

	//更新界面
	update();
}
