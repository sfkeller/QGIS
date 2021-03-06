# -*- coding: utf-8 -*-
"""
Test the QgsSettings class

Run with: ctest -V -R PyQgsSettings

.. note:: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
"""
from __future__ import print_function
from future import standard_library
import os
import tempfile
from qgis.core import (QgsSettings,)
from qgis.testing import start_app, unittest
from qgis.PyQt.QtCore import QSettings

standard_library.install_aliases()

__author__ = 'Alessandro Pasotti'
__date__ = '02/02/2017'
__copyright__ = 'Copyright 2017, The QGIS Project'
# This will get replaced with a git SHA1 when you do a git archive
__revision__ = '$Format:%H$'


start_app()


class TestQgsSettings(unittest.TestCase):

    cnt = 0

    def setUp(self):
        self.cnt += 1
        h, path = tempfile.mkstemp('.ini')
        assert QgsSettings.setGlobalSettingsPath(path)
        self.settings = QgsSettings('testqgissettings', 'testqgissettings%s' % self.cnt)
        self.globalsettings = QSettings(self.settings.globalSettingsPath(), QSettings.IniFormat)

    def tearDown(self):
        settings_file = self.settings.fileName()
        settings_default_file = self.settings.globalSettingsPath()
        del(self.settings)
        try:
            os.unlink(settings_file)
        except:
            pass
        try:
            os.unlink(settings_default_file)
        except:
            pass

    def addToDefaults(self, key, value):
        self.globalsettings.setValue(key, value)
        self.globalsettings.sync()

    def addArrayToDefaults(self, prefix, key, values):
        defaults = QSettings(self.settings.globalSettingsPath(), QSettings.IniFormat)  # NOQA
        self.globalsettings.beginWriteArray(prefix)
        i = 0
        for v in values:
            self.globalsettings.setArrayIndex(i)
            self.globalsettings.setValue(key, v)
            i += 1
        self.globalsettings.endArray()
        self.globalsettings.sync()

    def test_basic_functionality(self):
        self.assertEqual(self.settings.value('testqgissettings/doesnotexists', 'notexist'), 'notexist')
        self.settings.setValue('testqgissettings/name', 'qgisrocks')
        self.settings.sync()
        self.assertEqual(self.settings.value('testqgissettings/name'), 'qgisrocks')

    def test_defaults(self):
        self.assertIsNone(self.settings.value('testqgissettings/name'))
        self.addToDefaults('testqgissettings/name', 'qgisrocks')
        self.assertEqual(self.settings.value('testqgissettings/name'), 'qgisrocks')

    def test_allkeys(self):
        self.assertEqual(self.settings.allKeys(), [])
        self.addToDefaults('testqgissettings/name', 'qgisrocks')
        self.addToDefaults('testqgissettings/name2', 'qgisrocks2')
        self.settings.setValue('nepoti/eman', 'osaple')

        self.assertEqual(3, len(self.settings.allKeys()))
        self.assertIn('testqgissettings/name', self.settings.allKeys())
        self.assertIn('nepoti/eman', self.settings.allKeys())
        self.assertEqual('qgisrocks', self.settings.value('testqgissettings/name'))
        self.assertEqual('qgisrocks2', self.settings.value('testqgissettings/name2'))
        self.assertEqual('qgisrocks', self.globalsettings.value('testqgissettings/name'))
        self.assertEqual('osaple', self.settings.value('nepoti/eman'))
        self.assertEqual(3, len(self.settings.allKeys()))
        self.assertEqual(2, len(self.globalsettings.allKeys()))

    def test_precedence(self):
        self.assertEqual(self.settings.allKeys(), [])
        self.addToDefaults('testqgissettings/names/name1', 'qgisrocks1')
        self.settings.setValue('testqgissettings/names/name1', 'qgisrocks-1')

        self.assertEqual(self.settings.value('testqgissettings/names/name1'), 'qgisrocks-1')

    def test_uft8(self):
        self.assertEqual(self.settings.allKeys(), [])
        self.addToDefaults('testqgissettings/names/namèé↓1', 'qgisrocks↓1')
        self.assertEqual(self.settings.value('testqgissettings/names/namèé↓1'), 'qgisrocks↓1')

        self.settings.setValue('testqgissettings/names/namèé↓2', 'qgisrocks↓2')
        self.assertEqual(self.settings.value('testqgissettings/names/namèé↓2'), 'qgisrocks↓2')
        self.settings.setValue('testqgissettings/names/namèé↓1', 'qgisrocks↓-1')
        self.assertEqual(self.settings.value('testqgissettings/names/namèé↓1'), 'qgisrocks↓-1')

    def test_groups(self):
        self.assertEqual(self.settings.allKeys(), [])
        self.addToDefaults('testqgissettings/names/name1', 'qgisrocks1')
        self.addToDefaults('testqgissettings/names/name2', 'qgisrocks2')
        self.addToDefaults('testqgissettings/names/name3', 'qgisrocks3')
        self.addToDefaults('testqgissettings/name', 'qgisrocks')

        self.settings.beginGroup('testqgissettings')
        self.assertEqual(['names'], self.settings.childGroups())

        self.settings.setValue('surnames/name1', 'qgisrocks-1')
        self.assertEqual(['surnames', 'names'], self.settings.childGroups())

        self.settings.setValue('names/name1', 'qgisrocks-1')
        self.assertEqual('qgisrocks-1', self.settings.value('names/name1'))
        self.settings.endGroup()
        self.settings.beginGroup('testqgissettings/names')
        self.settings.setValue('name4', 'qgisrocks-4')
        keys = sorted(self.settings.childKeys())
        self.assertEqual(keys, ['name1', 'name2', 'name3', 'name4'])
        self.settings.endGroup()
        self.assertEqual('qgisrocks-1', self.settings.value('testqgissettings/names/name1'))
        self.assertEqual('qgisrocks-4', self.settings.value('testqgissettings/names/name4'))

    def test_array(self):
        self.assertEqual(self.settings.allKeys(), [])
        self.addArrayToDefaults('testqgissettings', 'key', ['qgisrocks1', 'qgisrocks2', 'qgisrocks3'])
        self.assertEqual(self.settings.allKeys(), ['testqgissettings/1/key', 'testqgissettings/2/key', 'testqgissettings/3/key', 'testqgissettings/size'])
        self.assertEqual(self.globalsettings.allKeys(), ['testqgissettings/1/key', 'testqgissettings/2/key', 'testqgissettings/3/key', 'testqgissettings/size'])

        self.assertEqual(3, self.globalsettings.beginReadArray('testqgissettings'))
        self.globalsettings.endArray()
        self.assertEqual(3, self.settings.beginReadArray('testqgissettings'))

        values = []
        for i in range(3):
            self.settings.setArrayIndex(i)
            values.append(self.settings.value("key"))

        self.assertEqual(values, ['qgisrocks1', 'qgisrocks2', 'qgisrocks3'])

    def test_section_getters_setters(self):
        self.assertEqual(self.settings.allKeys(), [])

        self.settings.setValue('key1', 'core1', section=QgsSettings.Core)
        self.settings.setValue('key2', 'core2', section=QgsSettings.Core)

        self.settings.setValue('key1', 'server1', section=QgsSettings.Server)
        self.settings.setValue('key2', 'server2', section=QgsSettings.Server)

        self.settings.setValue('key1', 'gui1', section=QgsSettings.Gui)
        self.settings.setValue('key2', 'gui2', QgsSettings.Gui)

        self.settings.setValue('key1', 'plugins1', section=QgsSettings.Plugins)
        self.settings.setValue('key2', 'plugins2', section=QgsSettings.Plugins)

        self.settings.setValue('key1', 'misc1', section=QgsSettings.Misc)
        self.settings.setValue('key2', 'misc2', section=QgsSettings.Misc)

        # Test that the values are namespaced
        self.assertEqual(self.settings.value('core/key1'), 'core1')
        self.assertEqual(self.settings.value('core/key2'), 'core2')

        self.assertEqual(self.settings.value('server/key1'), 'server1')
        self.assertEqual(self.settings.value('server/key2'), 'server2')

        self.assertEqual(self.settings.value('gui/key1'), 'gui1')
        self.assertEqual(self.settings.value('gui/key2'), 'gui2')

        self.assertEqual(self.settings.value('plugins/key1'), 'plugins1')
        self.assertEqual(self.settings.value('plugins/key2'), 'plugins2')

        self.assertEqual(self.settings.value('misc/key1'), 'misc1')
        self.assertEqual(self.settings.value('misc/key2'), 'misc2')

        # Test getters
        self.assertEqual(self.settings.value('key1', None, section=QgsSettings.Core), 'core1')
        self.assertEqual(self.settings.value('key2', None, section=QgsSettings.Core), 'core2')

        self.assertEqual(self.settings.value('key1', None, section=QgsSettings.Server), 'server1')
        self.assertEqual(self.settings.value('key2', None, section=QgsSettings.Server), 'server2')

        self.assertEqual(self.settings.value('key1', None, section=QgsSettings.Gui), 'gui1')
        self.assertEqual(self.settings.value('key2', None, section=QgsSettings.Gui), 'gui2')

        self.assertEqual(self.settings.value('key1', None, section=QgsSettings.Plugins), 'plugins1')
        self.assertEqual(self.settings.value('key2', None, section=QgsSettings.Plugins), 'plugins2')

        self.assertEqual(self.settings.value('key1', None, section=QgsSettings.Misc), 'misc1')
        self.assertEqual(self.settings.value('key2', None, section=QgsSettings.Misc), 'misc2')

        # Test default values on Section getter
        self.assertEqual(self.settings.value('key_not_exist', 'misc_not_exist', section=QgsSettings.Misc), 'misc_not_exist')

    def test_contains(self):
        self.assertEqual(self.settings.allKeys(), [])
        self.addToDefaults('testqgissettings/name', 'qgisrocks1')
        self.addToDefaults('testqgissettings/name2', 'qgisrocks2')

        self.assertTrue(self.settings.contains('testqgissettings/name'))
        self.assertTrue(self.settings.contains('testqgissettings/name2'))

        self.settings.setValue('testqgissettings/name3', 'qgisrocks3')
        self.assertTrue(self.settings.contains('testqgissettings/name3'))

    def test_remove(self):
        self.settings.setValue('testQgisSettings/temp', True)
        self.assertEqual(self.settings.value('testQgisSettings/temp'), True)
        self.settings.remove('testQgisSettings/temp')
        self.assertEqual(self.settings.value('testqQgisSettings/temp'), None)


if __name__ == '__main__':
    unittest.main()
