#ifndef CONNECTTFEDITOR_H
#define CONNECTTFEDITOR_H

  connect(m_tfEditor, SIGNAL(updateComposite()),
	  this, SLOT(updateComposite()));

  connect(m_tfEditor, SIGNAL(giveHistogram(int)),
	  this, SLOT(changeHistogram(int)));

  connect(m_tfEditor, SIGNAL(applyUndo(bool)),
	  this, SLOT(applyTFUndo(bool)));

  connect(m_tfEditor, SIGNAL(transferFunctionUpdated()),
	  this, SLOT(transferFunctionUpdated()));

  connect(this, SIGNAL(histogramUpdated(QImage, QImage)),
	  m_tfEditor, SLOT(setHistogramImage(QImage, QImage)));

  connect(this, SIGNAL(histogramUpdated(int*)),
	  m_tfEditor, SLOT(setHistogram2D(int*)));

#endif
