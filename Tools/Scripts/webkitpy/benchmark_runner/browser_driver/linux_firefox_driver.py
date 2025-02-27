# Copyright (C) 2016 Igalia S.L. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os

from webkitpy.benchmark_runner.browser_driver.linux_browser_driver import LinuxBrowserDriver


class LinuxFirefoxDriver(LinuxBrowserDriver):
    browser_name = 'firefox'
    process_search_list = ['firefox', 'firefox-bin']

    def launch_url(self, url, options, browser_build_path, browser_path):
        self._browser_arguments = ['-new-instance', '-profile', self._temp_profiledir,
                                   '-width', str(self._screen_size().width),
                                   '-height', str(self._screen_size().height),
                                   url]
        super(LinuxFirefoxDriver, self).launch_url(url, options, browser_build_path, browser_path)

    def launch_driver(self, url, options, browser_build_path):
        import webkitpy.thirdparty.autoinstalled.selenium
        from selenium import webdriver
        from selenium.webdriver.firefox.options import Options
        options = Options()
        if browser_build_path:
            binary_path = os.path.join(browser_build_path, 'firefox-bin')
            options.binary_location = binary_path
        driver_executable = self.webdriver_binary_path
        driver = webdriver.Firefox(firefox_options=options, executable_path=driver_executable)
        super(LinuxFirefoxDriver, self).launch_webdriver(url, driver)
        return driver
