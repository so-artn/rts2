#!/usr/bin/python
# (C) 2013, Markus Wildi, markus.wildi@bluewin.ch
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software Foundation,
#   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#   Or visit http://www.gnu.org/licenses/gpl.html.
#

__author__ = 'markus.wildi@bluewin.ch'

class Ds9Region(object):
    def __init__(self, dataSex=None, display=None, logger=None):
        self.dataSex=dataSex
        self.display=display
        self.logger=logger

    def displayWithRegion(self): 
        try:
            self.display.set('file {0}'.format(self.dataSex.fitsFn))
        except Exception, e:
            self.logger.warn('analyze: plotting fits with regions failed, file:{0}:error\n{1}'.format(self.dataSex.fitsFn,e))
            return False

        try:
            self.display.set('zoom to fit')
            self.display.set('zscale')
            i_x = self.dataSex.fields.index('X_IMAGE')
            i_y = self.dataSex.fields.index('Y_IMAGE')
            i_f = self.dataSex.fields.index('FLAGS')
        except Exception, e:
            self.logger.warn('analyze: plotting fits with regions failed:\n{0}'.format(e))
            return False

        for x in self.dataSex.catalog:
            if not x:
                continue

            if x[i_f] == 0:
                color='green'
            else:
                color='red'
            try:
                self.display.set('regions', 'image; circle {0} {1} 10 # color={{{2}}}'.format(x[i_x],x[i_y], color))
            except Exception, e:
                self.logger.warn('analyze: plotting fits with regions failed:\n{0}'.format(e))
                return False

        else:
            return True

        return False