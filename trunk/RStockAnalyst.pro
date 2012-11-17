TEMPLATE = app
CONFIG += console

TARGET = RStockAnalyst
TEMPLATE = app

QT += core \
    gui \
	xml \
	sql
	

PRECOMPILED_HEADER = StdAfx.h


HEADERS += StdAfx.h StockDrv.h STKDRV.h MainWindow.h BaseMarketWidget.h \
			DataEngine.h BaseMarketTreeModel.h StockInfoItem.h \
			BaseWidget.h BaseMarketTreeView.h RMdiCtrl.h KLineWidget.h \
			MarketTrendWidget.h

SOURCES += main.cpp STKDRV.cpp MainWindow.cpp BaseMarketWidget.cpp \
			DataEngine.cpp BaseMarketTreeModel.cpp StockInfoItem.cpp \
			BaseWidget.cpp BaseMarketTreeView.cpp RMdiCtrl.cpp KLineWidget.cpp \
			MarketTrendWidget.cpp


RESOURCES += RStockAnalyst.qrc