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

    def test_query_password
        # Prepare a request
        parameters = {}
        parameters['UserName'] = "Average Joe"
        parameters['QueryPassword'] = true
        parameters['QueryMessage'] = "Joe, please enter your password!"
        parameters['Title'] = "Password needed"
        expected_password = "Yes, I'll do that"
        loop = DBus::Main.new
        loop << @bus
        @proxy.queryDialog(parameters) do |msg, resp|
            verify_equal(expected_password) { resp['Secret'] }
            loop.quit
        end

        # The UI should have appeared now; check its contents
        @app.force_refresh()
        verify() { @app.SignOnUi__Dialog(:name => 'LoginDialog') }
        verify_equal("false") {
            @app.QLineEdit(:name => 'UsernameField')['enabled']
        }
        verify_equal("true") {
            @app.QLineEdit(:name => 'PasswordField')['enabled']
        }
        verify_equal("Average Joe") {
            @app.QLineEdit(:name => 'UsernameField')['text']
        }

        # Fill the password and click the button
        @app.QLineEdit(:name => 'PasswordField')['text'] = expected_password
        @app.QPushButton(:default => true).tap
        loop.run
        verify_not(3, "Dialog still open") {
            @app.SignOnUi__Dialog()
        }
    end

    def test_captcha
        # Prepare a request
        parameters = {}
        parameters['UserName'] = "Average Joe"
        parameters['QueryPassword'] = true
        parameters['Title'] = "Password needed"
        parameters['CaptchaUrl'] = "123.png"
        expected_password = "pwd"
        expected_captcha_text = "123"
        loop = DBus::Main.new
        loop << @bus
        @proxy.queryDialog(parameters) do |msg, resp|
            puts "Response: #{resp}"
            verify_equal(expected_password) { resp['Secret'] }
            verify_equal(expected_captcha_text) { resp['CaptchaResponse'] }
            loop.quit
        end

        # The UI should have appeared now; check its contents
        @app.force_refresh()
        verify() { @app.SignOnUi__Dialog(:name => 'LoginDialog') }
        verify() { @app.QLineEdit(:name => 'CaptchaField') }

        # Fill the password, the captcha solution and click the button
        @app.QLineEdit(:name => 'PasswordField')['text'] = expected_password
        @app.QLineEdit(:name => 'CaptchaField')['text'] = expected_captcha_text
        @app.QPushButton(:default => true).tap
        loop.run
        verify_not(3, "Dialog still open") {
            @app.SignOnUi__Dialog()
        }
    end

end
