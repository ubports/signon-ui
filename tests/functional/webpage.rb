#!/usr/bin/env ruby

require 'tdriver'
require 'test/unit'
require 'dbus'
include TDriverVerify

class TestDialog < Test::Unit::TestCase
    # Run before each test case begins
    def setup
        @sut = TDriver.connect_sut(:Id => 'sut_qt')
        @app = @sut.application(:name => "signon-ui")
        @bus = DBus::SessionBus.instance
        service = @bus.service("com.nokia.singlesignonui")
        object = service.object("/SignonUi")
        object.introspect
        @proxy = object["com.nokia.singlesignonui"]
    end

    # Run after each test case completes
    def teardown
    end

    # Test cases:

    def test_login
        # Prepare a request
        parameters = {}
        parameters['OpenUrl'] = 'http://localhost:8000/'
        parameters['FinalUrl'] = 'http://localhost:8000/logged'
        parameters['UserName'] = 'user'
        loop = DBus::Main.new
        loop << @bus
        @proxy.queryDialog(parameters) do |msg, resp|
            verify_equal('http://localhost:8000/login.html') {
                resp['UrlResponse']
            }
            loop.quit
        end

        @app.force_refresh()
        verify(3) { @app.SignOnUi__Dialog() }
        # Currently, it's not possible to interact with QWebView from tdriver:
        # https://projects.developer.nokia.com/Testabilitydriver/ticket/36
        #
        # So, we just close the window
        @app.press_key(:qt_kEscape)
        loop.run
    end

    def test_autologin
        # Prepare a request
        parameters = {}
        parameters['OpenUrl'] = 'http://localhost:8000/'
        parameters['FinalUrl'] = 'http://localhost:8000/logged'
        parameters['UserName'] = 'user'
        parameters['Secret'] = 'pwd'
        loop = DBus::Main.new
        loop << @bus
        @proxy.queryDialog(parameters) do |msg, resp|
            verify_equal('http://localhost:8000/logged#userpwd') {
                resp['UrlResponse']
            }
            loop.quit
        end

        # The UI should not appear in this test
        @app.force_refresh()
        verify_not(2, "Unrequested dialog appeared!") {
            @app.SignOnUi__Dialog()
        }
        loop.run
    end

end
