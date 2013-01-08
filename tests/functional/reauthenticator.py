#! /usr/bin/python3

import unittest
import logging
import sys
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
        # Store the session data and mechanism, so that later they can be
        # verified by the test functions
        self.session_data = session_data
        self.mechanism = mechanism
        # signon-ui does not care what dictionary is returned, as long as it's
        # not an error; so let's just return the same dictionary
        return session_data


class MockSignond(dbus.service.Object):
    def __init__(self, bus_name):
        super().__init__(bus_name, '/com/google/code/AccountsSSO/SingleSignOn')
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
    credentials_id = 45000

    @classmethod
    def setUpClass(cls):
        manager = Accounts.Manager()
        account = manager.create_account("any provider")
        account.set_enabled(True)

        v_credentials_id = GObject.Value()
        v_credentials_id.init(GObject.TYPE_UINT)
        v_credentials_id.set_uint(AuthenticatorTests.credentials_id)
        account.set_value('CredentialsId', v_credentials_id)
        
        account.store_blocking()
        AuthenticatorTests.account_id = account.id

        # Get a proxy for signon-ui
        bus = dbus.SessionBus()
        signon_ui_proxy = bus.get_object('com.nokia.singlesignonui',
                '/SignonUi')
        signon_ui = dbus.Interface(signon_ui_proxy,
                'com.nokia.singlesignonui')
        webcredentials_proxy = bus.get_object('com.canonical.indicators.webcredentials',
                '/com/canonical/indicators/webcredentials')
        webcredentials = dbus.Interface(webcredentials_proxy,
                'com.canonical.indicators.webcredentials')
        AuthenticatorTests.signon_ui = signon_ui
        AuthenticatorTests.webcredentials = webcredentials

        signond_bus_name = dbus.service.BusName(SIGNOND_BUS_NAME,
                bus=dbus.SessionBus())
        AuthenticatorTests.signond_bus_name = signond_bus_name

    def setUp(self):
        self.test_failed = False
        self.signond = MockSignond(AuthenticatorTests.signond_bus_name)
        self.loop = GLib.MainLoop()

    def tearDown(self):
        self.signond.remove_from_connection()
        self.signond = None
        self.loop = None

    def failed_test(self, e):
        print(e)
        self.test_failed = True
        self.loop.quit()

    def test01_single_failure(self):
        log = logging.getLogger('AuthenticatorTests')
        self.method = 'TheMethod'
        self.mechanism = 'TheMechanism'
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

        reply = AuthenticatorTests.signon_ui.queryDialog(params)
        log.debug('Got reply: %s' % (reply,))
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
        log.debug('Account id: ', self.account_id)
        AuthenticatorTests.webcredentials.ReauthenticateAccount(self.account_id, extra_data,
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

        AuthenticatorTests.webcredentials.ReauthenticateAccount(self.account_id, extra_data,
                reply_handler=reauthenticate_cb,
                error_handler=self.failed_test)
        self.loop.run()

    def test02_many_failures(self):
        log = logging.getLogger('AuthenticatorTests')
        num_sessions = 8
        sessions = {}

        def query_dialog_cb(reply):
            log.debug('Got reply: %s (count = %s)' % (reply, query_dialog_cb.call_count))
            try:
                self.assertIn('QueryErrorCode', reply)
                self.assertEqual(reply['QueryErrorCode'], 10)
            except:
                self.test_failed = True
            query_dialog_cb.call_count += 1
            if query_dialog_cb.call_count == num_sessions:
                self.loop.quit()
        query_dialog_cb.call_count = 0

        for i in range(num_sessions):
            client_data = dbus.Dictionary({
                'OneKey': 'OneValue',
                'AnotherKey': i
            }, signature='sv')

            params = {
                'OpenUrl': 'https://localhost/dontopen',
                'Identity': self.credentials_id,
                'Method': 'method{}'.format(int(i / 2)),
                'Mechanism': 'mechanism{}'.format(int(i / 3)),
                'ClientData': client_data,
            }

            reply = AuthenticatorTests.signon_ui.queryDialog(params,
                    reply_handler=query_dialog_cb,
                    error_handler=self.failed_test)
            sessions[i] = params

        self.loop.run()
        self.assertFalse(self.test_failed)


        def reauthenticate_cb(success):
            log.debug('reauthenticate_cb: {}'.format(success))
            try:
                self.assertTrue(success)
            except:
                self.test_failed = True
            self.loop.quit()

        extra_data = dbus.Dictionary({
            'ExtraKey': 'ExtraValue',
        }, signature='sv')
        AuthenticatorTests.webcredentials.ReauthenticateAccount(self.account_id, extra_data,
                reply_handler=reauthenticate_cb,
                error_handler=self.failed_test)
        self.loop.run()
        self.assertFalse(self.test_failed)

        # Now verify that the authentication data is consistent
        self.assertEqual(len(self.signond.sessions), num_sessions)
        actual_sessions = []
        for session in self.signond.sessions.values():
            s = {
                'Method': session.method,
                'Mechanism': session.mechanism,
                'SessionData': session.session_data
            }
            actual_sessions.append(s)

        for session in sessions.values():
            full_data = session['ClientData'].copy()
            full_data.update(extra_data)
            s = {
                'Method': session['Method'],
                'Mechanism': session['Mechanism'],
                'SessionData': full_data
            }
            found = False
            for actual in actual_sessions:
                if s == actual:
                    found = True
                    break

            self.assertTrue(found)


if __name__ == '__main__':
    DBusGMainLoop(set_as_default=True)
    logging.basicConfig(stream=sys.stderr)
    logging.getLogger('AuthenticatorTests').setLevel(logging.DEBUG)
    unittest.main(failfast=True, buffer=False, verbosity=2)
