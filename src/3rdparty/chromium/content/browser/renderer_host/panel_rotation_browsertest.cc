// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/stringprintf.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/shell/browser/shell.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace content {

// TODO(https://crbug.com/998131): Add test coverage across all platforms.
#if defined(OS_CHROMEOS)
class PanelRotationBrowserTest : public ContentBrowserTest {
 protected:
  void SetPanelRotation(display::Display::Rotation rotation) {
    display::Screen* screen = display::Screen::GetScreen();
    screen->SetPanelRotationForTesting(screen->GetPrimaryDisplay().id(),
                                       rotation);
  }
  int ReadScreenOrientationAngle() {
    return EvalJs(CreateBrowser()->web_contents(), "screen.orientation.angle")
        .ExtractInt();
  }
};

IN_PROC_BROWSER_TEST_F(PanelRotationBrowserTest, ScreenOrientationAPI) {
  SetPanelRotation(display::Display::ROTATE_0);
  EXPECT_EQ(ReadScreenOrientationAngle(), 0);

  SetPanelRotation(display::Display::ROTATE_90);
  EXPECT_EQ(ReadScreenOrientationAngle(), 270);

  SetPanelRotation(display::Display::ROTATE_180);
  EXPECT_EQ(ReadScreenOrientationAngle(), 180);

  SetPanelRotation(display::Display::ROTATE_270);
  EXPECT_EQ(ReadScreenOrientationAngle(), 90);
}
#endif  // defined(OS_CHROMEOS)

}  // namespace content