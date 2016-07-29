#ifndef CONNECTPREFERENCES_H
#define CONNECTPREFERENCES_H

    connect(m_preferencesWidget, SIGNAL(updateGL()),
	    m_Viewer, SLOT(update()));

    connect(m_preferencesWidget, SIGNAL(tagClicked(int)),
	    m_Viewer, SLOT(setTag(int)));

    connect(m_preferencesWidget, SIGNAL(dofChanged(int, float)),
	    m_Viewer, SLOT(setDOF(int, float)));

    connect(m_preferencesWidget, SIGNAL(updateLookupTable()),
	    m_Viewer, SLOT(updateLookupTable()));

    connect(m_preferencesWidget, SIGNAL(stereoSettings(float, float, float)),
	    m_Viewer, SLOT(updateStereoSettings(float, float, float)));

    connect(m_preferencesWidget, SIGNAL(tagColorChanged()),
	    m_Viewer, SLOT(updateTagColors()));

    connect(m_Viewer, SIGNAL(stereoSettings(float, float, float)),
	    m_preferencesWidget, SLOT(updateStereoSettings(float, float, float)));

    connect(m_Viewer, SIGNAL(focusSetting(float)),
	    m_preferencesWidget, SLOT(updateFocusSetting(float)));

    connect(m_Viewer, SIGNAL(changeStill(int)),
	    m_preferencesWidget, SLOT(changeStill(int)));

    connect(m_Viewer, SIGNAL(changeDrag(int)),
	    m_preferencesWidget, SLOT(changeDrag(int)));

#endif
