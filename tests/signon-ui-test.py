#!/usr/bin/python

import logging
logging.basicConfig()
log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)

import glib
import dbus
from dbus.mainloop.glib import DBusGMainLoop
DBusGMainLoop(set_as_default=True)

from mago import TestCase
import ldtp
from WebServer import BasicLogin

class Test(TestCase):
    launcher = None
    def setUp(self):
        bus = dbus.SessionBus()
        self.proxy = bus.get_object('com.nokia.singlesignonui',
                                    '/SignonUi')
        self.signonui = dbus.Interface(self.proxy,
            dbus_interface='com.nokia.singlesignonui')
        self.timeout = 3600
        self.loop = glib.MainLoop()
        super(Test, self).setUp()

    def test_username(self):
        glib.timeout_add(500, self.username_query_dialog)
        self.loop.run()

    def username_query_dialog(self):
        parameters = dict()
        parameters['QueryUserName'] = True
        parameters['QueryMessage'] = 'Write user@example.com and press OK'
        parameters['Title'] = 'Enter your username'
        log.debug('Calling!')
        self.signonui.queryDialog(parameters,
                reply_handler = self.queryDialogReply,
                error_handler = self.error_cb,
                timeout = self.timeout)
        window = 'dlgEnteryourusername'
        assert ldtp.waittillguiexist(window) == 1
        log.debug('Window appeared')
        log.debug('Objects: %s' % ldtp.getobjectlist(window))
        ldtp.settextvalue(window, 'txtusername', 'user@example.com')
        ldtp.click(window, 'btnOK')
        log.debug('Window list: %s' % ldtp.getwindowlist())

    def queryDialogReply(self, reply):
        log.debug("Signon-ui replied: %s" % reply)
        self.loop.quit()
        assert reply['UserName'] == 'user@example.com'

    def error_cb(self, error):
        log.debug("Got error: %s" % error)
        self.loop.quit()
        assert False

    def test_browser_autologin(self):
        self.webserver = BasicLogin.Server()
        glib.io_add_watch(self.webserver.fileno(),
                glib.IO_IN | glib.IO_OUT,
                self.browserAutologinIOCb)
        glib.idle_add(self.browserAutologinStart)
        self.loop.run()

    def browserAutologinStart(self):
        parameters = dict()
        parameters['OpenUrl'] = 'http://localhost:8000/'
        parameters['FinalUrl'] = 'http://localhost:8000/logged'
        parameters['UserName'] = 'user'
        parameters['Secret'] = 'pwd'
        log.debug('Calling!')
        self.signonui.queryDialog(parameters,
                reply_handler = self.browserAutologinQueryDialogCb,
                error_handler = self.error_cb,
                timeout = self.timeout)

    def browserAutologinIOCb(self, condition, user_data):
        self.webserver.handle_request()
        return True

    def browserAutologinQueryDialogCb(self, reply):
        log.debug("Signon-ui replied: %s" % reply)
        self.server_running = False
        assert 'UrlResponse' in reply
        assert reply['UrlResponse'] == 'http://localhost:8000/logged#userpwd'
        self.loop.quit()

if __name__ == '__main__':
    test = Test()
    test.setUp()
    test.test_username()
