#! /usr/bin/python3

import unittest
import dbus, dbus.service
from dbus.mainloop.glib import DBusGMainLoop

from gi.repository import Accounts
from gi.repository import GLib
from gi.repository import GObject

SIGNOND_BUS_NAME = 'com.google.code.AccountsSSO.SingleSignOn'
AUTH_SERVICE_IFACE = SIGNOND_BUS_NAME + '.AuthService'
AUTH_SESSION_IFACE = SIGNOND_BUS_NAME + '.AuthSession'

class MockAuthSession(dbus.service.Object):
    last_session = 0
    def __init__(self, connection, identity, method):
        self.identity = identity
        self.method = method
        MockAuthSession.last_session += 1
        path = '/session/{}'.format(MockAuthSession.last_session)
        super().__init__(connection, path)

    @dbus.service.method(dbus_interface=AUTH_SESSION_IFACE,
            in_signature='a{sv}s', out_signature='a{sv}')
    def process(self, session_data, mechanism):
        print('process: {}, mechanism {}'.format(session_data, mechanism))
        # Store the session data and mechanism, so that later they can be
        # verified by the test functions
        self.session_data = session_data
        self.mechanism = mechanism
        # signon-ui does not care what dictionary is returned, as long as it's
        # not an error; so let's just return the same dictionary
        return session_data


class MockSignond(dbus.service.Object):
    def __init__(self):
        connection = dbus.service.BusName(SIGNOND_BUS_NAME, bus=dbus.SessionBus())
        super().__init__(connection, '/com/google/code/AccountsSSO/SingleSignOn')
        self.sessions = {}

    @dbus.service.method(dbus_interface=AUTH_SERVICE_IFACE,
            in_signature='u', out_signature='oa{sv}')
    def getIdentity(self, identity):
        return ('/identity/{}'.format(identity), {})

    @dbus.service.method(dbus_interface=AUTH_SERVICE_IFACE,
            in_signature='us', out_signature='s')
    def getAuthSessionObjectPath(self, identity, method):
        session = MockAuthSession(self.connection, identity, method)
        self.sessions[session._object_path] = session
        return session._object_path

class AuthenticatorTests(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        self.credentials_id = 45000
        self.method = 'TheMethod'
        self.mechanism = 'TheMechanism'

        manager = Accounts.Manager()
        account = manager.create_account("any provider")
        account.set_enabled(True)

        v_credentials_id = GObject.Value()
        v_credentials_id.init(GObject.TYPE_UINT)
        v_credentials_id.set_uint(self.credentials_id)
        account.set_value('CredentialsId', v_credentials_id)
        
        account.store_blocking()
        self.account_id = account.id

        # Get a proxy for signon-ui
        bus = dbus.SessionBus()
        self.signon_ui_proxy = bus.get_object('com.nokia.singlesignonui',
                '/SignonUi')
        self.signon_ui = dbus.Interface(self.signon_ui_proxy,
                'com.nokia.singlesignonui')
        self.webcredentials_proxy = bus.get_object('com.canonical.indicators.webcredentials',
                '/com/canonical/indicators/webcredentials')
        self.webcredentials = dbus.Interface(self.webcredentials_proxy,
                'com.canonical.indicators.webcredentials')
        super().__init__(*args, **kwargs)

    def setUp(self):
        self.test_failed = False
        self.loop = GLib.MainLoop()
        self.signond = MockSignond()

    def tearDown(self):
        self.signond = None
        self.loop = None

    def failed_test(self, e):
        print(e)
        self.test_failed = True
        self.loop.quit()

    def test_single_failure(self):
        client_data = dbus.Dictionary({
            'OneKey': 'OneValue',
            'AnotherKey': 'AnotherValue'
        }, signature='sv')

        params = {
            'OpenUrl': 'https://localhost/dontopen',
            'Identity': self.credentials_id,
            'Method': self.method,
            'Mechanism': self.mechanism,
            'ClientData': client_data,
        }

        reply = self.signon_ui.queryDialog(params)
        print('Got reply: %s' % (reply,))
        self.assertIn('QueryErrorCode', reply)
        self.assertEqual(reply['QueryErrorCode'], 10)

        def reauthenticate_cb(success):
            try:
                self.assertTrue(success)
            except:
                self.test_failed = True
            self.loop.quit()

        extra_data = dbus.Dictionary({
            'ExtraKey': 'ExtraValue',
        }, signature='sv')
        self.webcredentials.ReauthenticateAccount(self.account_id, extra_data,
                reply_handler=reauthenticate_cb,
                error_handler=self.failed_test)
        self.loop.run()
        self.assertFalse(self.test_failed)

        # Now verify that the authentication data is consistent
        self.assertEqual(len(self.signond.sessions), 1)
        session = list(self.signond.sessions.values())[0]
        self.assertEqual(session.method, self.method)
        self.assertEqual(session.mechanism, self.mechanism)
        full_data = client_data.copy()
        full_data.update(extra_data)
        self.assertEqual(session.session_data, full_data)


if __name__ == '__main__':
    DBusGMainLoop(set_as_default=True)
    unittest.main(failfast=True)
