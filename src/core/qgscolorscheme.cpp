/***************************************************************************
                             qgscolorscheme.cpp
                             -------------------
    begin                : July 2014
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

#include "qgscolorscheme.h"

#include "qgsproject.h"
#include "qgssymbollayerutils.h"
#include "qgsapplication.h"
#include "qgssettings.h"

#include <QDir>

bool QgsColorScheme::setColors( const QgsNamedColorList &colors, const QString &context, const QColor &baseColor )
{
  //base implementation does nothing
  Q_UNUSED( colors );
  Q_UNUSED( context );
  Q_UNUSED( baseColor );
  return false;
}


//
// QgsRecentColorScheme
//

QgsRecentColorScheme::QgsRecentColorScheme() : QgsColorScheme()
{

}

QgsNamedColorList QgsRecentColorScheme::fetchColors( const QString &context, const QColor &baseColor )
{
  Q_UNUSED( context );
  Q_UNUSED( baseColor );

  //fetch recent colors
  QgsSettings settings;
  QList< QVariant > recentColorVariants = settings.value( QStringLiteral( "colors/recent" ) ).toList();

  //generate list from recent colors
  QgsNamedColorList colorList;
  Q_FOREACH ( const QVariant &color, recentColorVariants )
  {
    colorList.append( qMakePair( color.value<QColor>(), QgsSymbolLayerUtils::colorToName( color.value<QColor>() ) ) );
  }
  return colorList;
}

QgsRecentColorScheme *QgsRecentColorScheme::clone() const
{
  return new QgsRecentColorScheme();
}

void QgsRecentColorScheme::addRecentColor( const QColor &color )
{
  if ( !color.isValid() )
  {
    return;
  }

  //strip alpha from color
  QColor opaqueColor = color;
  opaqueColor.setAlpha( 255 );

  QgsSettings settings;
  QList< QVariant > recentColorVariants = settings.value( QStringLiteral( "colors/recent" ) ).toList();

  //remove colors by name
  for ( int colorIdx = recentColorVariants.length() - 1; colorIdx >= 0; --colorIdx )
  {
    if ( ( recentColorVariants.at( colorIdx ).value<QColor>() ).name() == opaqueColor.name() )
    {
      recentColorVariants.removeAt( colorIdx );
    }
  }

  //add color
  QVariant colorVariant = QVariant( opaqueColor );
  recentColorVariants.prepend( colorVariant );

  //trim to 20 colors
  while ( recentColorVariants.count() > 20 )
  {
    recentColorVariants.pop_back();
  }

  settings.setValue( QStringLiteral( "colors/recent" ), recentColorVariants );
}

QColor QgsRecentColorScheme::lastUsedColor()
{
  //fetch recent colors
  QgsSettings settings;
  QList< QVariant > recentColorVariants = settings.value( QStringLiteral( "colors/recent" ) ).toList();

  if ( recentColorVariants.isEmpty() )
    return QColor();

  return recentColorVariants.at( 0 ).value<QColor>();
}


QgsCustomColorScheme::QgsCustomColorScheme() : QgsColorScheme()
{

}

QgsNamedColorList QgsCustomColorScheme::fetchColors( const QString &context, const QColor &baseColor )
{
  Q_UNUSED( context );
  Q_UNUSED( baseColor );

  //fetch predefined custom colors
  QgsNamedColorList colorList;
  QgsSettings settings;

  //check if settings contains custom palette
  if ( !settings.contains( QStringLiteral( "/colors/palettecolors" ) ) )
  {
    //no custom palette, return default colors
    colorList.append( qMakePair( QColor( "#000000" ), QString() ) );
    colorList.append( qMakePair( QColor( "#ffffff" ), QString() ) );
    colorList.append( qMakePair( QColor( "#a6cee3" ), QString() ) );
    colorList.append( qMakePair( QColor( "#1f78b4" ), QString() ) );
    colorList.append( qMakePair( QColor( "#b2df8a" ), QString() ) );
    colorList.append( qMakePair( QColor( "#33a02c" ), QString() ) );
    colorList.append( qMakePair( QColor( "#fb9a99" ), QString() ) );
    colorList.append( qMakePair( QColor( "#e31a1c" ), QString() ) );
    colorList.append( qMakePair( QColor( "#fdbf6f" ), QString() ) );
    colorList.append( qMakePair( QColor( "#ff7f00" ), QString() ) );

    return colorList;
  }

  QList< QVariant > customColorVariants = settings.value( QStringLiteral( "colors/palettecolors" ) ).toList();
  QList< QVariant > customColorLabels = settings.value( QStringLiteral( "colors/palettelabels" ) ).toList();

  //generate list from custom colors
  int colorIndex = 0;
  for ( QList< QVariant >::iterator it = customColorVariants.begin();
        it != customColorVariants.end(); ++it )
  {
    QColor color = ( *it ).value<QColor>();
    QString label;
    if ( customColorLabels.length() > colorIndex )
    {
      label = customColorLabels.at( colorIndex ).toString();
    }

    colorList.append( qMakePair( color, label ) );
    colorIndex++;
  }

  return colorList;
}

bool QgsCustomColorScheme::setColors( const QgsNamedColorList &colors, const QString &context, const QColor &baseColor )
{
  Q_UNUSED( context );
  Q_UNUSED( baseColor );

  // save colors to settings
  QgsSettings settings;
  QList< QVariant > customColors;
  QList< QVariant > customColorLabels;

  QgsNamedColorList::const_iterator colorIt = colors.constBegin();
  for ( ; colorIt != colors.constEnd(); ++colorIt )
  {
    QVariant color = ( *colorIt ).first;
    QVariant label = ( *colorIt ).second;
    customColors.append( color );
    customColorLabels.append( label );
  }
  settings.setValue( QStringLiteral( "colors/palettecolors" ), customColors );
  settings.setValue( QStringLiteral( "colors/palettelabels" ), customColorLabels );
  return true;
}

QgsCustomColorScheme *QgsCustomColorScheme::clone() const
{
  return new QgsCustomColorScheme();
}


QgsProjectColorScheme::QgsProjectColorScheme()
{

}

QgsNamedColorList QgsProjectColorScheme::fetchColors( const QString &context, const QColor &baseColor )
{
  Q_UNUSED( context );
  Q_UNUSED( baseColor );

  QgsNamedColorList colorList;

  QStringList colorStrings = QgsProject::instance()->readListEntry( QStringLiteral( "Palette" ), QStringLiteral( "/Colors" ) );
  QStringList colorLabels = QgsProject::instance()->readListEntry( QStringLiteral( "Palette" ), QStringLiteral( "/Labels" ) );

  //generate list from custom colors
  int colorIndex = 0;
  for ( QStringList::iterator it = colorStrings.begin();
        it != colorStrings.end(); ++it )
  {
    QColor color = QgsSymbolLayerUtils::decodeColor( *it );
    QString label;
    if ( colorLabels.length() > colorIndex )
    {
      label = colorLabels.at( colorIndex );
    }

    colorList.append( qMakePair( color, label ) );
    colorIndex++;
  }

  return colorList;
}

bool QgsProjectColorScheme::setColors( const QgsNamedColorList &colors, const QString &context, const QColor &baseColor )
{
  Q_UNUSED( context );
  Q_UNUSED( baseColor );

  // save colors to project
  QStringList customColors;
  QStringList customColorLabels;

  QgsNamedColorList::const_iterator colorIt = colors.constBegin();
  for ( ; colorIt != colors.constEnd(); ++colorIt )
  {
    QString color = QgsSymbolLayerUtils::encodeColor( ( *colorIt ).first );
    QString label = ( *colorIt ).second;
    customColors.append( color );
    customColorLabels.append( label );
  }
  QgsProject::instance()->writeEntry( QStringLiteral( "Palette" ), QStringLiteral( "/Colors" ), customColors );
  QgsProject::instance()->writeEntry( QStringLiteral( "Palette" ), QStringLiteral( "/Labels" ), customColorLabels );
  return true;
}

QgsProjectColorScheme *QgsProjectColorScheme::clone() const
{
  return new QgsProjectColorScheme();
}


//
// QgsGplColorScheme
//

QgsGplColorScheme::QgsGplColorScheme()
  : QgsColorScheme()
{

}

QgsNamedColorList QgsGplColorScheme::fetchColors( const QString &context, const QColor &baseColor )
{
  Q_UNUSED( context );
  Q_UNUSED( baseColor );

  QString sourceFilePath = gplFilePath();
  if ( sourceFilePath.isEmpty() )
  {
    QgsNamedColorList noColors;
    return noColors;
  }

  bool ok;
  QString name;
  QFile sourceFile( sourceFilePath );
  return QgsSymbolLayerUtils::importColorsFromGpl( sourceFile, ok, name );
}

bool QgsGplColorScheme::setColors( const QgsNamedColorList &colors, const QString &context, const QColor &baseColor )
{
  Q_UNUSED( context );
  Q_UNUSED( baseColor );

  QString destFilePath = gplFilePath();
  if ( destFilePath.isEmpty() )
  {
    return false;
  }

  QFile destFile( destFilePath );
  return QgsSymbolLayerUtils::saveColorsToGpl( destFile, schemeName(), colors );
}


//
// QgsUserColorScheme
//

QgsUserColorScheme::QgsUserColorScheme( const QString &filename )
  : QgsGplColorScheme()
  , mFilename( filename )
{
  QFile sourceFile( gplFilePath() );

  //read in name
  if ( sourceFile.open( QIODevice::ReadOnly ) )
  {
    QTextStream in( &sourceFile );

    //find name line
    QString line;
    while ( !in.atEnd() && !line.startsWith( QLatin1String( "Name:" ) ) )
    {
      line = in.readLine();
    }
    if ( !in.atEnd() )
    {
      QRegExp rx( "Name:\\s*(\\S.*)$" );
      if ( rx.indexIn( line ) != -1 )
      {
        mName = rx.cap( 1 );
      }
    }
  }
  if ( mName.isEmpty() )
  {
    mName = mFilename;
  }
}

QString QgsUserColorScheme::schemeName() const
{
  return mName;
}

QgsUserColorScheme *QgsUserColorScheme::clone() const
{
  return new QgsUserColorScheme( mFilename );
}

QgsColorScheme::SchemeFlags QgsUserColorScheme::flags() const
{
  QgsColorScheme::SchemeFlags f = QgsGplColorScheme::flags();

  QgsSettings s;
  QStringList showInMenuSchemes = s.value( QStringLiteral( "/colors/showInMenuList" ) ).toStringList();

  if ( showInMenuSchemes.contains( mName ) )
  {
    f |= QgsColorScheme::ShowInColorButtonMenu;
  }

  return f;
}

bool QgsUserColorScheme::erase()
{
  QString filePath = gplFilePath();
  if ( filePath.isEmpty() )
  {
    return false;
  }

  //try to erase gpl file
  return QFile::remove( filePath );
}

void QgsUserColorScheme::setShowSchemeInMenu( bool show )
{
  QgsSettings s;
  QStringList showInMenuSchemes = s.value( QStringLiteral( "/colors/showInMenuList" ) ).toStringList();

  if ( show && !showInMenuSchemes.contains( mName ) )
  {
    showInMenuSchemes << mName;
  }
  else if ( !show && showInMenuSchemes.contains( mName ) )
  {
    showInMenuSchemes.removeAll( mName );
  }

  s.setValue( QStringLiteral( "/colors/showInMenuList" ), showInMenuSchemes );
}

QString QgsUserColorScheme::gplFilePath()
{
  QString palettesDir = QgsApplication::qgisSettingsDirPath() + "/palettes";

  QDir localDir;
  if ( !localDir.mkpath( palettesDir ) )
  {
    return QString();
  }

  return QDir( palettesDir ).filePath( mFilename );
}
