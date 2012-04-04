#!/usr/bin/env ruby

require 'tdriver'
require 'test/unit'
require 'dbus'
include TDriverVerify

class TestDialog < Test::Unit::TestCase
    # Run before each test case begins
    def setup
	puts "Starting"
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

    def test_startup
        # Make sure signon-ui doesn't pop-up unrequested dialogs
        verify_not(3, "Unrequested dialog appeared!") {
            @app.SignOnUi__Dialog()
        }
    end

    def test_query_username
        # Prepare a request
        parameters = {}
        parameters['QueryUserName'] = true
        parameters['QueryMessage'] = "The field should get filled automatically"
        parameters['Title'] = "Enter your username"
        expected_username = "The User Name"
        loop = DBus::Main.new
        loop << @bus
        @proxy.queryDialog(parameters) do |msg, resp|
            verify_equal(expected_username) { resp['UserName'] }
            loop.quit
        end

        # The UI should have appeared now; check its contents
        @app.force_refresh()
        verify() { @app.QLabel(:name => "Message") }
        verify() { @app.SignOnUi__Dialog(:name => 'LoginDialog') }
        verify_equal("true") { @app.QLabel(:name => 'Message')['enabled'] }
        verify_equal("The field should get filled automatically") {
            @app.QLabel(:name => 'Message')['text']
        }

        # Fill the username and click the button
        @app.QLineEdit['text'] = expected_username
        @app.QPushButton(:default => true).tap
        loop.run
        verify_not(3, "Dialog still open") {
            @app.SignOnUi__Dialog()
        }
    end

end
