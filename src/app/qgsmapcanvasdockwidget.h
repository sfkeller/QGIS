/***************************************************************************
    qgsmapcanvasdockwidget.h
    ------------------------
    begin                : February 2017
    copyright            : (C) 2017 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSMAPCANVASDOCKWIDGET_H
#define QGSMAPCANVASDOCKWIDGET_H

#include <ui_qgsmapcanvasdockwidgetbase.h>

#include "qgsdockwidget.h"
#include "qgspoint.h"
#include "qgis_app.h"
#include <QWidgetAction>
#include <QTimer>
#include <memory>

class QgsMapCanvas;
class QgsScaleComboBox;
class QgsDoubleSpinBox;
class QgsStatusBarMagnifierWidget;
class QgsMapToolPan;
class QgsVertexMarker;
class QCheckBox;

/**
 * \class QgsMapCanvasDockWidget
 * A dock widget with an embedded map canvas, for additional map views.
 * \note added in QGIS 3.0
 */
class APP_EXPORT QgsMapCanvasDockWidget : public QgsDockWidget, private Ui::QgsMapCanvasDockWidgetBase
{
    Q_OBJECT
  public:
    explicit QgsMapCanvasDockWidget( const QString &name, QWidget *parent = nullptr );

    /**
     * Sets the main app map canvas.
     */
    void setMainCanvas( QgsMapCanvas *canvas );

    /**
     * Returns the map canvas contained in the dock widget.
     */
    QgsMapCanvas *mapCanvas();

    /**
     * Sets whether the view center should be synchronized with the main canvas center.
     * @see isViewCenterSynchronized()
     */
    void setViewCenterSynchronized( bool enabled );

    /**
     * Returns true if the view extent is synchronized with the main canvas extent.
     * @see setViewCenterSynchronized()
     */
    bool isViewCenterSynchronized() const;

    /**
     * Sets whether the cursor position marker is visible.
     * @see isCursorMarkerVisible()
     */
    void setCursorMarkerVisible( bool visible );

    /**
     * Returns true if the cursor position marker is visible.
     * @see setCursorMarkerVisible()
     */
    bool isCursorMarkerVisible() const;

    /**
     * Returns the scaling factor for main canvas scale to view scale.
     * @see setScaleFactor()
     * @see isViewScaleSynchronized()
     */
    double scaleFactor() const;

    /**
     * Sets the scaling \a factor for main canvas scale to view scale.
     * @see scaleFactor()
     * @see setViewScaleSynchronized()
     */
    void setScaleFactor( double factor );

    /**
     * Sets whether the view scale should be synchronized with the main canvas center.
     * @see isViewScaleSynchronized()
     * @see setScaleFactor()
     */
    void setViewScaleSynchronized( bool enabled );

    /**
     * Returns true if the view scale is synchronized with the main canvas extent.
     * @see setViewScaleSynchronized()
     * @see scaleFactor()
     */
    bool isViewScaleSynchronized() const;

  signals:

    void renameTriggered();

  protected:

    void resizeEvent( QResizeEvent *e ) override;

  private slots:

    void setMapCrs();
    void mapExtentChanged();
    void mapCrsChanged();
    void menuAboutToShow();
    void settingsMenuAboutToShow();
    void syncMarker( const QgsPoint &p );
    void mapScaleChanged();

  private:

    QgsMapCanvas *mMapCanvas = nullptr;
    QgsMapCanvas *mMainCanvas = nullptr;
    QMenu *mMenu = nullptr;
    QList<QAction *> mMenuPresetActions;
    QgsScaleComboBox *mScaleCombo = nullptr;
    QgsDoubleSpinBox *mRotationEdit = nullptr;
    QgsDoubleSpinBox *mMagnificationEdit = nullptr;
    QgsDoubleSpinBox *mScaleFactorWidget = nullptr;
    QCheckBox *mSyncScaleCheckBox = nullptr;
    bool mBlockScaleUpdate = false;
    bool mBlockRotationUpdate = false;
    bool mBlockMagnificationUpdate = false;
    bool mBlockExtentSync = false;
    QgsMapToolPan *mPanTool = nullptr;
    QTimer mResizeTimer;
    QgsVertexMarker *mXyMarker = nullptr;
    void syncViewCenter( QgsMapCanvas *sourceCanvas );
};

/**
 * \class QgsMapSettingsAction
 * Allows embedding a scale, rotation and other map settings into a menu.
 * \note added in QGIS 3.0
 */

class QgsMapSettingsAction: public QWidgetAction
{
    Q_OBJECT

  public:

    QgsMapSettingsAction( QWidget *parent = nullptr );

    QgsScaleComboBox *scaleCombo() { return mScaleCombo; }
    QgsDoubleSpinBox *rotationSpinBox() { return mRotationWidget; }
    QgsDoubleSpinBox *magnifierSpinBox() { return mMagnifierWidget; }
    QgsDoubleSpinBox *scaleFactorSpinBox() { return mScaleFactorWidget; }
    QCheckBox *syncScaleCheckBox() { return mSyncScaleCheckBox; }

  private:
    QgsScaleComboBox *mScaleCombo = nullptr;
    QgsDoubleSpinBox *mRotationWidget = nullptr;
    QgsDoubleSpinBox *mMagnifierWidget = nullptr;
    QCheckBox *mSyncScaleCheckBox = nullptr;
    QgsDoubleSpinBox *mScaleFactorWidget = nullptr;
};


#endif // QGSMAPCANVASDOCKWIDGET_H
