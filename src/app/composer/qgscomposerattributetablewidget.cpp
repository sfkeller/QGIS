/***************************************************************************
                         qgscomposerattributetablewidget.cpp
                         -----------------------------------
    begin                : September 2014
    copyright            : (C) 2014 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscomposerattributetablewidget.h"
#include "qgsatlascomposition.h"
#include "qgscomposition.h"
#include "qgscomposerframe.h"
#include "qgsattributeselectiondialog.h"
#include "qgscomposeritemwidget.h"
#include "qgscomposerattributetablev2.h"
#include "qgscomposermultiframecommand.h"
#include "qgscomposertablecolumn.h"
#include "qgscomposermap.h"
#include "qgsvectorlayer.h"
#include "qgsexpressionbuilderdialog.h"
#include "qgsproject.h"
#include "qgsrelationmanager.h"
#include "qgisgui.h"
#include "qgscomposertablebackgroundcolorsdialog.h"

QgsComposerAttributeTableWidget::QgsComposerAttributeTableWidget( QgsComposerAttributeTableV2 *table, QgsComposerFrame *frame )
  : QgsComposerItemBaseWidget( nullptr, table )
  , mComposerTable( table )
  , mFrame( frame )
{
  setupUi( this );
  setPanelTitle( tr( "Table properties" ) );

  blockAllSignals( true );

  mResizeModeComboBox->addItem( tr( "Use existing frames" ), QgsComposerMultiFrame::UseExistingFrames );
  mResizeModeComboBox->addItem( tr( "Extend to next page" ), QgsComposerMultiFrame::ExtendToNextPage );
  mResizeModeComboBox->addItem( tr( "Repeat until finished" ), QgsComposerMultiFrame::RepeatUntilFinished );

  mEmptyModeComboBox->addItem( tr( "Draw headers only" ), QgsComposerTableV2::HeadersOnly );
  mEmptyModeComboBox->addItem( tr( "Hide entire table" ), QgsComposerTableV2::HideTable );
  mEmptyModeComboBox->addItem( tr( "Show set message" ), QgsComposerTableV2::ShowMessage );

  mWrapBehaviorComboBox->addItem( tr( "Truncate text" ), QgsComposerTableV2::TruncateText );
  mWrapBehaviorComboBox->addItem( tr( "Wrap text" ), QgsComposerTableV2::WrapText );

  bool atlasEnabled = atlasComposition() && atlasComposition()->enabled();
  mSourceComboBox->addItem( tr( "Layer features" ), QgsComposerAttributeTableV2::LayerAttributes );
  toggleAtlasSpecificControls( atlasEnabled );

  //update relations combo when relations modified in project
  connect( QgsProject::instance()->relationManager(), SIGNAL( changed() ), this, SLOT( updateRelationsCombo() ) );

  mLayerComboBox->setFilters( QgsMapLayerProxyModel::VectorLayer );
  connect( mLayerComboBox, SIGNAL( layerChanged( QgsMapLayer * ) ), this, SLOT( changeLayer( QgsMapLayer * ) ) );

  mComposerMapComboBox->setComposition( mComposerTable->composition() );
  mComposerMapComboBox->setItemType( QgsComposerItem::ComposerMap );
  connect( mComposerMapComboBox, SIGNAL( itemChanged( QgsComposerItem * ) ), this, SLOT( composerMapChanged( QgsComposerItem * ) ) );

  mHeaderFontColorButton->setColorDialogTitle( tr( "Select header font color" ) );
  mHeaderFontColorButton->setAllowAlpha( true );
  mHeaderFontColorButton->setContext( QStringLiteral( "composer" ) );
  mContentFontColorButton->setColorDialogTitle( tr( "Select content font color" ) );
  mContentFontColorButton->setAllowAlpha( true );
  mContentFontColorButton->setContext( QStringLiteral( "composer" ) );
  mGridColorButton->setColorDialogTitle( tr( "Select grid color" ) );
  mGridColorButton->setAllowAlpha( true );
  mGridColorButton->setContext( QStringLiteral( "composer" ) );
  mGridColorButton->setDefaultColor( Qt::black );
  mBackgroundColorButton->setColorDialogTitle( tr( "Select background color" ) );
  mBackgroundColorButton->setAllowAlpha( true );
  mBackgroundColorButton->setContext( QStringLiteral( "composer" ) );
  mBackgroundColorButton->setShowNoColor( true );
  mBackgroundColorButton->setNoColorString( tr( "No background" ) );

  updateGuiElements();

  if ( mComposerTable )
  {
    QObject::connect( mComposerTable, SIGNAL( changed() ), this, SLOT( updateGuiElements() ) );

    QgsAtlasComposition *atlas = atlasComposition();
    if ( atlas )
    {
      // repopulate relations combo box if atlas layer changes
      connect( atlas, SIGNAL( coverageLayerChanged( QgsVectorLayer * ) ),
               this, SLOT( updateRelationsCombo() ) );
      connect( atlas, SIGNAL( toggled( bool ) ), this, SLOT( atlasToggled() ) );
    }
  }

  //embed widget for general options
  if ( mFrame )
  {
    //add widget for general composer item properties
    QgsComposerItemWidget *itemPropertiesWidget = new QgsComposerItemWidget( this, mFrame );
    mainLayout->addWidget( itemPropertiesWidget );
  }
}

QgsComposerAttributeTableWidget::~QgsComposerAttributeTableWidget()
{
}

void QgsComposerAttributeTableWidget::on_mRefreshPushButton_clicked()
{
  if ( !mComposerTable )
  {
    return;
  }

  mComposerTable->refreshAttributes();
}

void QgsComposerAttributeTableWidget::on_mAttributesPushButton_clicked()
{
  if ( !mComposerTable )
  {
    return;
  }

  //make deep copy of current columns, so we can restore them in case of cancelation
  QList<QgsComposerTableColumn *> currentColumns;
  QList<QgsComposerTableColumn *>::const_iterator it = mComposerTable->columns()->constBegin();
  for ( ; it != mComposerTable->columns()->constEnd() ; ++it )
  {
    QgsComposerTableColumn *copy = ( *it )->clone();
    currentColumns.append( copy );
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table attribute settings" ) );
  }

  //temporarily block updates for the window, to stop table trying to repaint under windows (#11462)
  window()->setUpdatesEnabled( false );

  QgsAttributeSelectionDialog d( mComposerTable, mComposerTable->sourceLayer(), this );
  if ( d.exec() == QDialog::Accepted )
  {
    mComposerTable->refreshAttributes();
    //safe to unblock updates
    window()->setUpdatesEnabled( true );
    mComposerTable->update();
    if ( composition )
    {
      composition->endMultiFrameCommand();
    }

    //clear currentColumns to free memory
    qDeleteAll( currentColumns );
    currentColumns.clear();
  }
  else
  {
    //undo changes
    mComposerTable->setColumns( currentColumns );
    window()->setUpdatesEnabled( true );
    if ( composition )
    {
      composition->cancelMultiFrameCommand();
    }
  }
}

void QgsComposerAttributeTableWidget::composerMapChanged( QgsComposerItem *item )
{
  if ( !mComposerTable )
  {
    return;
  }

  const QgsComposition *tableComposition = mComposerTable->composition();
  if ( tableComposition )
  {
    QgsComposition *composition = mComposerTable->composition();
    if ( composition )
    {
      composition->beginMultiFrameCommand( mComposerTable, tr( "Table map changed" ) );
    }
    mComposerTable->setComposerMap( dynamic_cast< const QgsComposerMap * >( item ) );
    mComposerTable->update();
    if ( composition )
    {
      composition->endMultiFrameCommand();
    }
  }
}

void QgsComposerAttributeTableWidget::on_mMaximumRowsSpinBox_valueChanged( int i )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table maximum columns" ), QgsComposerMultiFrameMergeCommand::TableMaximumFeatures );
  }
  mComposerTable->setMaximumNumberOfFeatures( i );
  mComposerTable->update();
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mMarginSpinBox_valueChanged( double d )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table margin changed" ), QgsComposerMultiFrameMergeCommand::TableMargin );
  }
  mComposerTable->setCellMargin( d );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mHeaderFontPushButton_clicked()
{
  if ( !mComposerTable )
    return;

  bool ok;
  QFont newFont = QgisGui::getFont( ok, mComposerTable->headerFont(), tr( "Select Font" ) );
  if ( ok )
  {
    QgsComposition *composition = mComposerTable->composition();
    if ( composition )
      composition->beginMultiFrameCommand( mComposerTable, tr( "Table header font" ) );

    mComposerTable->setHeaderFont( newFont );

    if ( composition )
      composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mHeaderFontColorButton_colorChanged( const QColor &newColor )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table header font color" ), QgsComposerMultiFrameMergeCommand::TableHeaderFontColor );
  }
  mComposerTable->setHeaderFontColor( newColor );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mContentFontPushButton_clicked()
{
  if ( !mComposerTable )
  {
    return;
  }

  bool ok;
  QFont newFont = QgisGui::getFont( ok, mComposerTable->contentFont(), tr( "Select Font" ) );
  if ( ok )
  {
    QgsComposition *composition = mComposerTable->composition();
    if ( composition )
      composition->beginMultiFrameCommand( mComposerTable, tr( "Table content font" ) );

    mComposerTable->setContentFont( newFont );

    if ( composition )
      composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mContentFontColorButton_colorChanged( const QColor &newColor )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table content font color" ), QgsComposerMultiFrameMergeCommand::TableContentFontColor );
  }
  mComposerTable->setContentFontColor( newColor );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mGridStrokeWidthSpinBox_valueChanged( double d )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table grid line" ), QgsComposerMultiFrameMergeCommand::TableGridStrokeWidth );
  }
  mComposerTable->setGridStrokeWidth( d );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mGridColorButton_colorChanged( const QColor &newColor )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table grid color" ), QgsComposerMultiFrameMergeCommand::TableGridColor );
  }
  mComposerTable->setGridColor( newColor );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mDrawHorizontalGrid_toggled( bool state )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table horizontal grid toggled" ) );
  }
  mComposerTable->setHorizontalGrid( state );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mDrawVerticalGrid_toggled( bool state )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table vertical grid toggled" ) );
  }
  mComposerTable->setVerticalGrid( state );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mShowGridGroupCheckBox_toggled( bool state )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table grid toggled" ) );
  }
  mComposerTable->setShowGrid( state );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mBackgroundColorButton_colorChanged( const QColor &newColor )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table background color" ), QgsComposerMultiFrameMergeCommand::TableBackgroundColor );
  }
  mComposerTable->setBackgroundColor( newColor );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::updateGuiElements()
{
  if ( !mComposerTable )
  {
    return;
  }

  blockAllSignals( true );

  mSourceComboBox->setCurrentIndex( mSourceComboBox->findData( mComposerTable->source() ) );
  mRelationsComboBox->setCurrentIndex( mRelationsComboBox->findData( mComposerTable->relationId() ) );

  //layer combo box
  if ( mComposerTable->vectorLayer() )
  {
    mLayerComboBox->setLayer( mComposerTable->vectorLayer() );
    if ( mComposerTable->vectorLayer()->geometryType() == QgsWkbTypes::NullGeometry )
    {
      //layer has no geometry, so uncheck & disable controls which require geometry
      mShowOnlyVisibleFeaturesCheckBox->setChecked( false );
      mShowOnlyVisibleFeaturesCheckBox->setEnabled( false );
    }
    else
    {
      mShowOnlyVisibleFeaturesCheckBox->setEnabled( true );
    }
  }

  mComposerMapComboBox->setItem( mComposerTable->composerMap() );
  mMaximumRowsSpinBox->setValue( mComposerTable->maximumNumberOfFeatures() );
  mMarginSpinBox->setValue( mComposerTable->cellMargin() );
  mGridStrokeWidthSpinBox->setValue( mComposerTable->gridStrokeWidth() );
  mGridColorButton->setColor( mComposerTable->gridColor() );
  mDrawHorizontalGrid->setChecked( mComposerTable->horizontalGrid() );
  mDrawVerticalGrid->setChecked( mComposerTable->verticalGrid() );
  if ( mComposerTable->showGrid() )
  {
    mShowGridGroupCheckBox->setChecked( true );
  }
  else
  {
    mShowGridGroupCheckBox->setChecked( false );
  }
  mBackgroundColorButton->setColor( mComposerTable->backgroundColor() );

  mHeaderFontColorButton->setColor( mComposerTable->headerFontColor() );
  mContentFontColorButton->setColor( mComposerTable->contentFontColor() );

  if ( mComposerTable->displayOnlyVisibleFeatures() && mShowOnlyVisibleFeaturesCheckBox->isEnabled() )
  {
    mShowOnlyVisibleFeaturesCheckBox->setCheckState( Qt::Checked );
    mComposerMapComboBox->setEnabled( true );
    mComposerMapLabel->setEnabled( true );
  }
  else
  {
    mShowOnlyVisibleFeaturesCheckBox->setCheckState( Qt::Unchecked );
    mComposerMapComboBox->setEnabled( false );
    mComposerMapLabel->setEnabled( false );
  }

  mUniqueOnlyCheckBox->setChecked( mComposerTable->uniqueRowsOnly() );
  mIntersectAtlasCheckBox->setChecked( mComposerTable->filterToAtlasFeature() );
  mFeatureFilterEdit->setText( mComposerTable->featureFilter() );
  mFeatureFilterCheckBox->setCheckState( mComposerTable->filterFeatures() ? Qt::Checked : Qt::Unchecked );
  mFeatureFilterEdit->setEnabled( mComposerTable->filterFeatures() );
  mFeatureFilterButton->setEnabled( mComposerTable->filterFeatures() );

  mHeaderHAlignmentComboBox->setCurrentIndex( ( int )mComposerTable->headerHAlignment() );
  mHeaderModeComboBox->setCurrentIndex( ( int )mComposerTable->headerMode() );

  mEmptyModeComboBox->setCurrentIndex( mEmptyModeComboBox->findData( mComposerTable->emptyTableBehavior() ) );
  mEmptyMessageLineEdit->setText( mComposerTable->emptyTableMessage() );
  mEmptyMessageLineEdit->setEnabled( mComposerTable->emptyTableBehavior() == QgsComposerTableV2::ShowMessage );
  mEmptyMessageLabel->setEnabled( mComposerTable->emptyTableBehavior() == QgsComposerTableV2::ShowMessage );
  mDrawEmptyCheckBox->setChecked( mComposerTable->showEmptyRows() );
  mWrapStringLineEdit->setText( mComposerTable->wrapString() );
  mWrapBehaviorComboBox->setCurrentIndex( mWrapBehaviorComboBox->findData( mComposerTable->wrapBehavior() ) );

  mResizeModeComboBox->setCurrentIndex( mResizeModeComboBox->findData( mComposerTable->resizeMode() ) );
  mAddFramePushButton->setEnabled( mComposerTable->resizeMode() == QgsComposerMultiFrame::UseExistingFrames );

  mEmptyFrameCheckBox->setChecked( mFrame->hidePageIfEmpty() );
  mHideEmptyBgCheckBox->setChecked( mFrame->hideBackgroundIfEmpty() );

  toggleSourceControls();

  blockAllSignals( false );
}

void QgsComposerAttributeTableWidget::atlasToggled()
{
  //display/hide atlas options in source combobox depending on atlas status
  bool atlasEnabled = atlasComposition() && atlasComposition()->enabled();
  toggleAtlasSpecificControls( atlasEnabled );

  if ( !mComposerTable )
    return;

  whileBlocking( mSourceComboBox )->setCurrentIndex( mSourceComboBox->findData( mComposerTable->source() ) );

  if ( !atlasEnabled && mComposerTable->filterToAtlasFeature() )
  {
    mComposerTable->setFilterToAtlasFeature( false );
  }
}

void QgsComposerAttributeTableWidget::updateRelationsCombo()
{
  mRelationsComboBox->blockSignals( true );
  mRelationsComboBox->clear();

  QgsVectorLayer *atlasLayer = atlasCoverageLayer();
  if ( atlasLayer )
  {
    QList<QgsRelation> relations = QgsProject::instance()->relationManager()->referencedRelations( atlasLayer );
    Q_FOREACH ( const QgsRelation &relation, relations )
    {
      mRelationsComboBox->addItem( relation.name(), relation.id() );
    }
    if ( mComposerTable )
    {
      mRelationsComboBox->setCurrentIndex( mRelationsComboBox->findData( mComposerTable->relationId() ) );
    }
  }

  mRelationsComboBox->blockSignals( false );
}

void QgsComposerAttributeTableWidget::toggleAtlasSpecificControls( const bool atlasEnabled )
{
  if ( !atlasEnabled )
  {
    if ( mComposerTable->source() == QgsComposerAttributeTableV2::AtlasFeature )
    {
      mComposerTable->setSource( QgsComposerAttributeTableV2::LayerAttributes );
    }
    mSourceComboBox->removeItem( mSourceComboBox->findData( QgsComposerAttributeTableV2::AtlasFeature ) );
    mSourceComboBox->removeItem( mSourceComboBox->findData( QgsComposerAttributeTableV2::RelationChildren ) );
    mRelationsComboBox->blockSignals( true );
    mRelationsComboBox->setEnabled( false );
    mRelationsComboBox->clear();
    mRelationsComboBox->blockSignals( false );
    mIntersectAtlasCheckBox->setEnabled( false );
  }
  else
  {
    if ( mSourceComboBox->findData( QgsComposerAttributeTableV2::AtlasFeature ) == -1 )
    {
      //add missing atlasfeature option to combobox
      mSourceComboBox->addItem( tr( "Current atlas feature" ), QgsComposerAttributeTableV2::AtlasFeature );
    }
    if ( mSourceComboBox->findData( QgsComposerAttributeTableV2::RelationChildren ) == -1 )
    {
      //add missing relation children option to combobox
      mSourceComboBox->addItem( tr( "Relation children" ), QgsComposerAttributeTableV2::RelationChildren );
    }

    //add relations for coverage layer
    updateRelationsCombo();
    mRelationsComboBox->setEnabled( true );
    mIntersectAtlasCheckBox->setEnabled( true );
  }
}

void QgsComposerAttributeTableWidget::blockAllSignals( bool b )
{
  mSourceComboBox->blockSignals( b );
  mLayerComboBox->blockSignals( b );
  mComposerMapComboBox->blockSignals( b );
  mMaximumRowsSpinBox->blockSignals( b );
  mMarginSpinBox->blockSignals( b );
  mGridColorButton->blockSignals( b );
  mGridStrokeWidthSpinBox->blockSignals( b );
  mBackgroundColorButton->blockSignals( b );
  mDrawHorizontalGrid->blockSignals( b );
  mDrawVerticalGrid->blockSignals( b );
  mShowGridGroupCheckBox->blockSignals( b );
  mShowOnlyVisibleFeaturesCheckBox->blockSignals( b );
  mUniqueOnlyCheckBox->blockSignals( b );
  mIntersectAtlasCheckBox->blockSignals( b );
  mFeatureFilterEdit->blockSignals( b );
  mFeatureFilterCheckBox->blockSignals( b );
  mHeaderHAlignmentComboBox->blockSignals( b );
  mHeaderModeComboBox->blockSignals( b );
  mHeaderFontColorButton->blockSignals( b );
  mContentFontColorButton->blockSignals( b );
  mResizeModeComboBox->blockSignals( b );
  mRelationsComboBox->blockSignals( b );
  mEmptyModeComboBox->blockSignals( b );
  mEmptyMessageLineEdit->blockSignals( b );
  mEmptyFrameCheckBox->blockSignals( b );
  mHideEmptyBgCheckBox->blockSignals( b );
  mDrawEmptyCheckBox->blockSignals( b );
  mWrapStringLineEdit->blockSignals( b );
  mWrapBehaviorComboBox->blockSignals( b );
}

void QgsComposerAttributeTableWidget::setMaximumNumberOfFeatures( int n )
{
  whileBlocking( mMaximumRowsSpinBox )->setValue( n );
}

void QgsComposerAttributeTableWidget::on_mShowOnlyVisibleFeaturesCheckBox_stateChanged( int state )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table visible only toggled" ) );
  }
  bool showOnlyVisibleFeatures = ( state == Qt::Checked );
  mComposerTable->setDisplayOnlyVisibleFeatures( showOnlyVisibleFeatures );
  mComposerTable->update();
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }

  //enable/disable map combobox based on state of checkbox
  mComposerMapComboBox->setEnabled( state == Qt::Checked );
  mComposerMapLabel->setEnabled( state == Qt::Checked );
}

void QgsComposerAttributeTableWidget::on_mUniqueOnlyCheckBox_stateChanged( int state )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table remove duplicates changed" ) );
  }
  mComposerTable->setUniqueRowsOnly( state == Qt::Checked );
  mComposerTable->update();
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mEmptyFrameCheckBox_toggled( bool checked )
{
  if ( !mFrame )
  {
    return;
  }

  mFrame->beginCommand( tr( "Empty frame mode toggled" ) );
  mFrame->setHidePageIfEmpty( checked );
  mFrame->endCommand();
}

void QgsComposerAttributeTableWidget::on_mHideEmptyBgCheckBox_toggled( bool checked )
{
  if ( !mFrame )
  {
    return;
  }

  mFrame->beginCommand( tr( "Hide background if empty toggled" ) );
  mFrame->setHideBackgroundIfEmpty( checked );
  mFrame->endCommand();
}

void QgsComposerAttributeTableWidget::on_mIntersectAtlasCheckBox_stateChanged( int state )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table filter to atlas changed" ) );
  }
  bool filterToAtlas = ( state == Qt::Checked );
  mComposerTable->setFilterToAtlasFeature( filterToAtlas );
  mComposerTable->update();
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mFeatureFilterCheckBox_stateChanged( int state )
{
  if ( !mComposerTable )
  {
    return;
  }

  if ( state == Qt::Checked )
  {
    mFeatureFilterEdit->setEnabled( true );
    mFeatureFilterButton->setEnabled( true );
  }
  else
  {
    mFeatureFilterEdit->setEnabled( false );
    mFeatureFilterButton->setEnabled( false );
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table feature filter toggled" ) );
  }
  mComposerTable->setFilterFeatures( state == Qt::Checked );
  mComposerTable->update();
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mFeatureFilterEdit_editingFinished()
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table feature filter modified" ) );
  }
  mComposerTable->setFeatureFilter( mFeatureFilterEdit->text() );
  mComposerTable->update();
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mFeatureFilterButton_clicked()
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsExpressionContext context = mComposerTable->createExpressionContext();
  QgsExpressionBuilderDialog exprDlg( mComposerTable->sourceLayer(), mFeatureFilterEdit->text(), this, QStringLiteral( "generic" ), context );
  exprDlg.setWindowTitle( tr( "Expression based filter" ) );
  if ( exprDlg.exec() == QDialog::Accepted )
  {
    QString expression =  exprDlg.expressionText();
    if ( !expression.isEmpty() )
    {
      mFeatureFilterEdit->setText( expression );
      QgsComposition *composition = mComposerTable->composition();
      if ( composition )
      {
        composition->beginMultiFrameCommand( mComposerTable, tr( "Table feature filter modified" ) );
      }
      mComposerTable->setFeatureFilter( mFeatureFilterEdit->text() );
      mComposerTable->update();
      if ( composition )
      {
        composition->endMultiFrameCommand();
      }
    }
  }
}

void QgsComposerAttributeTableWidget::on_mHeaderHAlignmentComboBox_currentIndexChanged( int index )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table header alignment changed" ) );
  }
  mComposerTable->setHeaderHAlignment( ( QgsComposerTableV2::HeaderHAlignment )index );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mHeaderModeComboBox_currentIndexChanged( int index )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table header mode changed" ) );
  }
  mComposerTable->setHeaderMode( ( QgsComposerTableV2::HeaderMode )index );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mWrapStringLineEdit_editingFinished()
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table wrap string changed" ) );
  }
  mComposerTable->setWrapString( mWrapStringLineEdit->text() );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::changeLayer( QgsMapLayer *layer )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsVectorLayer *vl = dynamic_cast<QgsVectorLayer *>( layer );
  if ( !vl )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Table layer changed" ) );
  }
  mComposerTable->setVectorLayer( vl );
  mComposerTable->update();
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }

  if ( vl->geometryType() == QgsWkbTypes::NullGeometry )
  {
    //layer has no geometry, so uncheck & disable controls which require geometry
    mShowOnlyVisibleFeaturesCheckBox->setChecked( false );
    mShowOnlyVisibleFeaturesCheckBox->setEnabled( false );
  }
  else
  {
    mShowOnlyVisibleFeaturesCheckBox->setEnabled( true );
  }
}

void QgsComposerAttributeTableWidget::on_mAddFramePushButton_clicked()
{
  if ( !mComposerTable || !mFrame )
  {
    return;
  }

  //create a new frame based on the current frame
  QPointF pos = mFrame->pos();
  //shift new frame so that it sits 10 units below current frame
  pos.ry() += mFrame->rect().height() + 10;

  QgsComposerFrame *newFrame = mComposerTable->createNewFrame( mFrame, pos, mFrame->rect().size() );
  mComposerTable->recalculateFrameSizes();

  //set new frame as selection
  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->setSelectedItem( newFrame );
  }
}

void QgsComposerAttributeTableWidget::on_mResizeModeComboBox_currentIndexChanged( int index )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Change resize mode" ) );
    mComposerTable->setResizeMode( ( QgsComposerMultiFrame::ResizeMode )mResizeModeComboBox->itemData( index ).toInt() );
    composition->endMultiFrameCommand();
  }

  mAddFramePushButton->setEnabled( mComposerTable->resizeMode() == QgsComposerMultiFrame::UseExistingFrames );
}

void QgsComposerAttributeTableWidget::on_mSourceComboBox_currentIndexChanged( int index )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Change table source" ) );
    mComposerTable->setSource( ( QgsComposerAttributeTableV2::ContentSource )mSourceComboBox->itemData( index ).toInt() );
    composition->endMultiFrameCommand();
  }

  toggleSourceControls();
}

void QgsComposerAttributeTableWidget::on_mRelationsComboBox_currentIndexChanged( int index )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Change table source relation" ) );
    mComposerTable->setRelationId( mRelationsComboBox->itemData( index ).toString() );
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mEmptyModeComboBox_currentIndexChanged( int index )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Change empty table behavior" ) );
    mComposerTable->setEmptyTableBehavior( ( QgsComposerTableV2::EmptyTableMode ) mEmptyModeComboBox->itemData( index ).toInt() );
    composition->endMultiFrameCommand();
    mEmptyMessageLineEdit->setEnabled( mComposerTable->emptyTableBehavior() == QgsComposerTableV2::ShowMessage );
    mEmptyMessageLabel->setEnabled( mComposerTable->emptyTableBehavior() == QgsComposerTableV2::ShowMessage );
  }
}

void QgsComposerAttributeTableWidget::on_mWrapBehaviorComboBox_currentIndexChanged( int index )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Change table wrap mode" ) );
    mComposerTable->setWrapBehavior( ( QgsComposerTableV2::WrapBehavior ) mWrapBehaviorComboBox->itemData( index ).toInt() );
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mAdvancedCustomisationButton_clicked()
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposerTableBackgroundColorsDialog d( mComposerTable, this );
  d.exec();
}

void QgsComposerAttributeTableWidget::on_mDrawEmptyCheckBox_toggled( bool checked )
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Show empty rows changed" ) );
  }
  mComposerTable->setShowEmptyRows( checked );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::on_mEmptyMessageLineEdit_editingFinished()
{
  if ( !mComposerTable )
  {
    return;
  }

  QgsComposition *composition = mComposerTable->composition();
  if ( composition )
  {
    composition->beginMultiFrameCommand( mComposerTable, tr( "Empty table message changed" ) );
  }
  mComposerTable->setEmptyTableMessage( mEmptyMessageLineEdit->text() );
  if ( composition )
  {
    composition->endMultiFrameCommand();
  }
}

void QgsComposerAttributeTableWidget::toggleSourceControls()
{
  switch ( mComposerTable->source() )
  {
    case QgsComposerAttributeTableV2::LayerAttributes:
      mLayerComboBox->setEnabled( true );
      mLayerComboBox->setVisible( true );
      mLayerLabel->setVisible( true );
      mRelationsComboBox->setEnabled( false );
      mRelationsComboBox->setVisible( false );
      mRelationLabel->setVisible( false );
      mMaximumRowsSpinBox->setEnabled( true );
      mMaxNumFeaturesLabel->setEnabled( true );
      mShowOnlyVisibleFeaturesCheckBox->setEnabled( true );
      mComposerMapComboBox->setEnabled( mComposerTable->displayOnlyVisibleFeatures() );
      mComposerMapLabel->setEnabled( mComposerTable->displayOnlyVisibleFeatures() );
      break;
    case QgsComposerAttributeTableV2::AtlasFeature:
      mLayerComboBox->setEnabled( false );
      mLayerComboBox->setVisible( false );
      mLayerLabel->setVisible( false );
      mRelationsComboBox->setEnabled( false );
      mRelationsComboBox->setVisible( false );
      mRelationLabel->setVisible( false );
      mMaximumRowsSpinBox->setEnabled( false );
      mMaxNumFeaturesLabel->setEnabled( false );
      mShowOnlyVisibleFeaturesCheckBox->setEnabled( false );
      mComposerMapComboBox->setEnabled( false );
      mComposerMapLabel->setEnabled( false );
      break;
    case QgsComposerAttributeTableV2::RelationChildren:
      mLayerComboBox->setEnabled( false );
      mLayerComboBox->setVisible( false );
      mLayerLabel->setVisible( false );
      mRelationsComboBox->setEnabled( true );
      mRelationsComboBox->setVisible( true );
      mRelationLabel->setVisible( true );
      mMaximumRowsSpinBox->setEnabled( true );
      mMaxNumFeaturesLabel->setEnabled( true );
      mShowOnlyVisibleFeaturesCheckBox->setEnabled( true );
      mComposerMapComboBox->setEnabled( true );
      mComposerMapLabel->setEnabled( true );
      break;
  }
}
