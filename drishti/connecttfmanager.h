#ifndef CONNECTTFMANAGER_H
#define CONNECTTFMANAGER_H

  connect(m_tfManager,
	  SIGNAL(changeTransferFunctionDisplay(int, QList<bool>)),
	  this,
	  SLOT(changeTransferFunctionDisplay(int, QList<bool>)));

  connect(m_tfManager,
	  SIGNAL(checkStateChanged(int, int, bool)),
	  this,
	  SLOT(checkStateChanged(int, int, bool)));

#endif
